/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVTP Clock Reference Format (CRF) handling functions
 @details
*/

#ifdef CFG_AVTP_1722A
#include "genavb/net_types.h"
#include "os/clock.h"
#include "os/stdlib.h"
#include "common/log.h"

#include "avtp.h"
#include "crf.h"

typedef enum {
	CRF_STATE_LOCKED,
	CRF_STATE_FREE_WHEELING,
	CRF_STATE_FREE_WHEELING_TO_LOCKED,
	CRF_STATE_LOCKED_TO_FREE_WHEELING
} crf_state_t;

typedef enum {
	CRF_EVENT_TIMEOUT,
	CRF_EVENT_TIMESTAMPS
} crf_event_t;

enum {
	CRF_ACTION_TIMER_STOP = (1 << 0),
	CRF_ACTION_GENERATE_TIMESTAMPS = (1 << 1)
} crf_actions_t;


unsigned int crf_stream_presentation_offset(struct stream_talker *stream)
{
	unsigned int latency = stream->latency;

	if (latency > CRF_LATENCY_MAX)
		latency = CRF_LATENCY_MAX;

	return _avtp_stream_presentation_offset(stream->max_transit_time, latency);
}


static void crf_net_tx(struct stream_talker *stream)
{
	const struct avdecc_format_crf_t *crf_fmt = &stream->format.u.s.subtype_u.crf;
	unsigned int ts_n, ts_batch;
	unsigned int flags;
	struct net_tx_desc *net_tx_desc_array[CRF_TX_BATCH], *net_desc;
	u32 ts[CRF_TX_BATCH * CRF_TIMESTAMPS_PER_PDU_MAX * 2];
	int n_now;
	void *buf;
	int rc;
	int i, j;
	unsigned int alignment_ts = 0, do_align = 0;
	u32 tnow_lsb = 0;

	flags = 0;

	rc = net_tx_alloc_multi(&stream->tx, net_tx_desc_array, stream->tx_batch, stream->header_len + crf_fmt->timestamps_per_pdu * 8);
	if (rc <= 0) {
		stream->stats.media_err++;
		goto media_rx_fail;
	}

	if (rc < stream->tx_batch) {
		stream->stats.media_err++;
		goto tx_batch_fail;
	}

	stream->stats.media_rx += rc;

	ts_batch = rc * crf_fmt->timestamps_per_pdu * 2;

	if (ts_batch > (CRF_TX_BATCH * CRF_TIMESTAMPS_PER_PDU_MAX * 2)) {
		os_log(LOG_ERR, "stream(%p) Unexpected number of timestamps needed(%u), clamping down to %u\n", stream, ts_batch, CRF_TX_BATCH * CRF_TIMESTAMPS_PER_PDU_MAX * 2);
		ts_batch = CRF_TX_BATCH * CRF_TIMESTAMPS_PER_PDU_MAX * 2;
	}

	if (stream_domain_phase_change(stream))
		do_align = 1;

	if (!stream->media_count)
		do_align = 1;

	if (do_align) {
		u64 tnow;

		if (os_clock_gettime64(stream->clock_gptp, &tnow) < 0) {
			stream->stats.clock_err++;
			goto media_clock_fail;
		}

		stream->subtype_data.crf.ts_msb = tnow >> 32;
		tnow_lsb = tnow & 0xffffffff;

		alignment_ts = tnow_lsb + avtp_stream_presentation_offset(stream);
		flags |= MCG_FLAGS_DO_ALIGN;
	}

	stream->consumer.gptp_current = stream->gptp_current;
	ts_n = media_clock_gen_get_ts(&stream->consumer, ts, ts_batch, &flags, alignment_ts);
	if (ts_n != ts_batch) {
		stream->stats.clock_err++;
		goto media_clock_fail;
	}

	if (do_align)
		if (tnow_lsb < ts[0])
			stream->subtype_data.crf.ts_msb++;

	n_now = rc;
	ts_n = 0;
	i = 0;
	while (i < n_now) {
		net_desc = net_tx_desc_array[i];

		net_desc->len += stream->header_len + crf_fmt->timestamps_per_pdu * 8;
		net_desc->flags = NET_TX_FLAGS_TS;

		buf = NET_DATA_START(net_desc);

		stream->media_count += stream->frames_per_packet;

		os_memcpy(buf, stream->header_template, stream->header_len);

		((struct avtp_crf_hdr *)stream->avtp_hdr)->sequence_num++;

		net_desc->ts = ts[0] - stream->max_transit_time;

		for (j = 0; j < crf_fmt->timestamps_per_pdu; j++) {
			unsigned int ts_now = ts[ts_n];

			if (ts_now < stream->ts_last)
				stream->subtype_data.crf.ts_msb++;

			*(u32 *)((char *)buf + stream->header_len + j * 8) = htonl(stream->subtype_data.crf.ts_msb);
			*(u32 *)((char *)buf + stream->header_len + j * 8 + 4) = htonl(ts_now);

			stream->ts_last = ts_now;

			/* Skip every other timestamp, dividing the frequency by 2,
			 * workaround for minimum generator frequency */
			ts_n += 2;
		}

		i++;
	}

	if (stream_net_tx(stream, (struct media_rx_desc **)net_tx_desc_array, i))
		goto transmit_fail;

	return;

media_clock_fail:
tx_batch_fail:
	net_free_multi((void **)net_tx_desc_array, rc);

media_rx_fail:
	stream->media_count = 0;

transmit_fail:
	return;
}

void crf_os_timer_handler(struct os_timer *t, int count)
{
	struct stream_talker *stream = container_of(t, struct stream_talker, subtype_data.crf.t);

	stream_net_tx_handler(stream);
}


static unsigned int crf_prepare_header(struct avtp_crf_hdr *hdr, const struct avdecc_format *format, void *stream_id)
{
	const struct avdecc_format_crf_t *crf_fmt = &format->u.s.subtype_u.crf;

	/* AVTP stream common fields */
	hdr->subtype = AVTP_SUBTYPE_CRF;
	hdr->version = AVTP_VERSION_0;
	hdr->sv = 1;

	copy_64(&hdr->stream_id, stream_id);

	/* CRF fields */
	hdr->type = crf_fmt->type;
	hdr->pull = crf_fmt->pull;
	CRF_BASE_FREQUENCY_SET(hdr, AVDECC_FMT_CRF_BASE_FREQUENCY(format));
	hdr->crf_data_length = htons(crf_fmt->timestamps_per_pdu * 8);
	hdr->timestamp_interval = htons(AVDECC_FMT_CRF_TIMESTAMP_INTERVAL(format));

	return sizeof(struct avtp_crf_hdr);
}

static int crf_measure_period(struct stream_listener *stream, struct timestamp *ts, unsigned *ts_n)
{
	struct crf_subtype_data *crf = &stream->subtype_data.crf;
	unsigned int period;
	int i;
	int rc = 0;

	for (i = 0; i < *ts_n; i++) {

		if (crf->timestamp > 0) {
			period = ts[i].ts_nsec - crf->received_ts_last;

			if (os_abs(period - crf->period_nominal) > crf->period_err) {
				rc = -1;
				crf->timestamp = 0;
			} else {
				if (crf->timestamp == 1)
					crf->period = period;
				else
					crf->period = (crf->period * 3 + 2) / 4 + (period + 2) / 4;
			}
		}

		if (crf->timestamp < 0xffffffff) /* Just a big number to avoid overflow */
			crf->timestamp++;

		crf->received_ts_last = ts[i].ts_nsec;
	}

	if (rc < 0)
		return rc;
	else if (crf->timestamp > crf->free_wheeling_to_locked_delay)
		return 1;
	else
		return 0;
}

static void crf_generate_timestamps(struct stream_listener *stream, struct timestamp *ts, unsigned *ts_n)
{
	struct crf_subtype_data *crf = &stream->subtype_data.crf;
	unsigned int ts_last;
	int i;

	if (crf->ts_last_set)
		ts_last = crf->ts_last;
	else
		ts_last = stream->gptp_current + crf->period;

	for (i = 0; i < *ts_n; i++) {
		ts_last += crf->period;
		ts[i].ts_nsec = ts_last;
		ts[i].flags = 0;
	}
}

static unsigned int crf_next_timeout(struct stream_listener *stream)
{
	struct crf_subtype_data *crf = &stream->subtype_data.crf;
	unsigned int dt;
	unsigned int stitch_ts_offset = 0;

	if (stream->source)
		stitch_ts_offset = stream->source->grid.producer.u.stream.stitch_ts_offset;

	/* The first_timestamp in the packet is less than max_transit_time in the future, and it may actually be in the past,
	 * if actual_transit_time + rx_batching_processing_time > max_transit_time.
	 * The next_required_timestamp is the first_timestamp in the next packet, next_required_timestamp = first_timestamp + packet_period
	 * The free-wheeling timer should trigger before the next required timestamp, but after the worst case arrival time
	 * for the next packet. So trigger_time < next_required_timestamp and trigger_time > next_required_timestamp + rx_batching_processing_time.
	 * This, of course, is impossible. It can only work if we offset the timestamps at least maximum_rx_batching_processing_time, and trigger the timer
	 * at that point. The time stamps are offset of at least MCR_DELAY and we trigger the timer at MCR_DELAY / 2 after the next_required_timestamp */
	dt = (unsigned int)(crf->ts_last + stitch_ts_offset + crf->period + MCR_DELAY / 2 - stream->gptp_current) / (unsigned int)NSECS_PER_MS;

	return dt;
}

static const char *crf_state_str[] = {
	[CRF_STATE_LOCKED] = "Locked",
	[CRF_STATE_FREE_WHEELING] = "Free Wheeling",
	[CRF_STATE_FREE_WHEELING_TO_LOCKED] = "Free Wheeling to locked",
	[CRF_STATE_LOCKED_TO_FREE_WHEELING] = "Locked to Free Wheeling"
};

static const char *crf_event_str[] = {
	[CRF_EVENT_TIMEOUT] = "Timeout",
	[CRF_EVENT_TIMESTAMPS] = "Timestamps"
};

static void crf_set_state(struct stream_listener *stream, crf_state_t state, crf_event_t event)
{
	struct crf_subtype_data *crf = &stream->subtype_data.crf;

	os_log(LOG_INFO, "stream_id(%016"PRIx64") gptp(%u) %s => %s on event %s\n",
		ntohll(stream->id), stream->gptp_current, crf_state_str[crf->state], crf_state_str[state], crf_event_str[event]);

	crf->state = state;

	if (stream->source) {
		switch (state) {
		case CRF_STATE_FREE_WHEELING:
			clock_domain_set_state(stream->source->grid.domain, CLOCK_DOMAIN_STATE_FREE_WHEELING);
			break;

		case CRF_STATE_LOCKED:
			clock_domain_clear_state(stream->source->grid.domain, CLOCK_DOMAIN_STATE_FREE_WHEELING);
			break;

		default:
			break;
		}
	}
}

static void crf_state_handler(struct stream_listener *stream, crf_event_t event, struct timestamp *ts, unsigned *ts_n)
{
	struct crf_subtype_data *crf = &stream->subtype_data.crf;
	unsigned int action = 0;
	unsigned int dt;
	int rc;
	int do_stitch = 0;

	switch (crf->state) {
	default:
	case CRF_STATE_LOCKED:
		/* Actively using network timestamps */
		switch (event) {
		default:
		case CRF_EVENT_TIMESTAMPS:
			/* Keep using network timestamps, unless there is a discontinuity */

			action = CRF_ACTION_TIMER_STOP;

			if (crf_measure_period(stream, ts, ts_n) < 0) {
				crf_set_state(stream, CRF_STATE_FREE_WHEELING, event);
				action |= CRF_ACTION_GENERATE_TIMESTAMPS;
			}

			break;

		case CRF_EVENT_TIMEOUT:
			/* Network timestamps not received, start generating locally */
			crf_set_state(stream, CRF_STATE_LOCKED_TO_FREE_WHEELING, event);
			action = CRF_ACTION_GENERATE_TIMESTAMPS;
			break;
		}

		break;

	case CRF_STATE_FREE_WHEELING:
		/* Actively generating timestamps locally */
		switch (event) {
		default:
		case CRF_EVENT_TIMEOUT:
			/* Still no timestamps received, continue generating locally */
			action = CRF_ACTION_GENERATE_TIMESTAMPS;
			break;

		case CRF_EVENT_TIMESTAMPS:
			/* Received network timestamps, start validating them:
			 - if there is no discontinuity (relatively to last _used_ timestamp), switch immediately to LOCKED and use received timestamps
			 - if there is a discontinuity, switch to FREE_WHEELING_TO_LOCKED and generate timestamps locally */

			action = CRF_ACTION_TIMER_STOP;

			/* make sure we check the period against the last used timestamp */
			crf->received_ts_last = crf->ts_last;
			crf->timestamp = 1;

			rc = crf_measure_period(stream, ts, ts_n);
			if (rc < 0) {
				/* At this point the timestamp count was reset */
				crf_set_state(stream, CRF_STATE_FREE_WHEELING_TO_LOCKED, event);

				action |= CRF_ACTION_GENERATE_TIMESTAMPS;
			} else
				crf_set_state(stream, CRF_STATE_LOCKED, event);

			break;
		}

		break;

	case CRF_STATE_FREE_WHEELING_TO_LOCKED:
		/* Transition from free-wheeling to locked */
		switch (event) {
		default:
		case CRF_EVENT_TIMEOUT:
			/* Another packet lost, go back to free-wheeling */
			crf_set_state(stream, CRF_STATE_FREE_WHEELING, event);

			action = CRF_ACTION_GENERATE_TIMESTAMPS;

			break;

		case CRF_EVENT_TIMESTAMPS:
			/* Continue to receive timestamps, continue validating them:
			- if there is a discontinuity (relative to last _received_ timestamp), switch to FREE_WHEELING
			- if there is no discontinuity, stay for N seconds in FREE_WHEELING_TO_LOCKED, continue to generate timestamps locally
			- after N seconds of no discontinuity in received timestamps, switch to LOCKED and start using them */
			action = CRF_ACTION_TIMER_STOP;

			rc = crf_measure_period(stream, ts, ts_n);
			if (rc < 0) {
				crf_set_state(stream, CRF_STATE_FREE_WHEELING, event);
				action |= CRF_ACTION_GENERATE_TIMESTAMPS;
			} else if (rc > 0) {
				crf_set_state(stream, CRF_STATE_LOCKED, event);
				do_stitch = 1;
			} else
				action |= CRF_ACTION_GENERATE_TIMESTAMPS;

			break;
		}

		break;

	case CRF_STATE_LOCKED_TO_FREE_WHEELING:
		/* Transition from locked to free wheeling */
		switch (event) {
		default:
		case CRF_EVENT_TIMESTAMPS:
			/* Received timestamps, if they match generated ones switch back to locked (1 packet lost),
			if they don't (possible discontinuity), continue generating timestamps and switch to free-wheeling */
			crf->received_ts_last = crf->ts_last;

			action = CRF_ACTION_TIMER_STOP;

			rc = crf_measure_period(stream, ts, ts_n);
			if (rc < 0) {
				crf_set_state(stream, CRF_STATE_FREE_WHEELING, event);

				action |= CRF_ACTION_GENERATE_TIMESTAMPS;
			} else
				crf_set_state(stream, CRF_STATE_LOCKED, event);

			break;

		case CRF_EVENT_TIMEOUT:
			/* At least 2 packets lost, switch to free-wheeling */
			crf_set_state(stream, CRF_STATE_FREE_WHEELING, event);
			action = CRF_ACTION_GENERATE_TIMESTAMPS;
			break;
		}

		break;
	}

	if (action & CRF_ACTION_TIMER_STOP)
		timer_stop(&crf->timer);

	if (action & CRF_ACTION_GENERATE_TIMESTAMPS)
		crf_generate_timestamps(stream, ts, ts_n);

	crf->ts_last = ts[(*ts_n) - 1].ts_nsec;
	crf->ts_last_set = 1;

	dt = crf_next_timeout(stream);
	timer_start(&crf->timer, dt);

	if (stream->source) {
		stream->stats.clock_tx += *ts_n;
		clock_producer_stream_rx(&stream->source->grid, ts, ts_n, do_stitch);
		*ts_n = 0;
	}
}

static void crf_timer_handler(void *data)
{
	struct stream_listener *stream = data;
	const struct avdecc_format_crf_t *crf_fmt = &stream->format.u.s.subtype_u.crf;
	struct timestamp ts[NET_RX_BATCH * CRF_TIMESTAMPS_PER_PDU_MAX];
	unsigned int ts_n = crf_fmt->timestamps_per_pdu;

	if (os_clock_gettime32(stream->clock_gptp, &stream->gptp_current) < 0)
		stream->stats.gptp_err++;

	crf_state_handler(stream, CRF_EVENT_TIMEOUT, ts, &ts_n);
}

static inline void crf_desc_flush(struct stream_listener *stream, struct avtp_rx_desc **desc, unsigned int *desc_n,
				   struct timestamp *ts, unsigned *ts_n)
{
	struct crf_subtype_data *crf = &stream->subtype_data.crf;

	if (*ts_n) {
		stream->stats.media_tx += *ts_n;
		crf_state_handler(stream, CRF_EVENT_TIMESTAMPS, ts, ts_n);
	} else if (!timer_is_running(&crf->timer)) {
		/* If CRF packets are received but no valid timestamps were received yet, trigger free-wheeling.
		 The timeout value is small but mostly arbitrary */
		timer_start(&crf->timer, 1);
	}

	if (*desc_n) {

		/* FIXME when the clock locks again we should indicate packets were lost */
		net_free_multi((void **)desc, *desc_n);

		*desc_n = 0;
	}
}

static void crf_net_rx(struct stream_listener *stream, struct avtp_rx_desc **desc, unsigned int n)
{
	const struct avdecc_format_crf_t *crf_fmt = &stream->format.u.s.subtype_u.crf;
	struct avtp_rx_desc **avtp_desc_first = NULL;
	struct avtp_crf_hdr *crf_hdr;
	u32 *hdr;
	unsigned int desc_n, ts_n;
	struct timestamp ts[NET_RX_BATCH * CRF_TIMESTAMPS_PER_PDU_MAX];
	unsigned int flags;
	unsigned int stats = 0;
	int i, j;

	os_log(LOG_DEBUG, "enter stream(%p)\n", stream);

	for (i = 0, desc_n = 0, ts_n = 0; i < n; i++) {

		crf_hdr = (struct avtp_crf_hdr *)((char *)desc[i] + desc[i]->desc.l3_offset);
		flags = 0;

		if (unlikely(crf_hdr->type != stream->subtype_data.crf.type)) {
			stream->stats.format_err++;

			crf_desc_flush(stream, avtp_desc_first, &desc_n, ts, &ts_n);

			net_rx_free(&desc[i]->desc);

			continue;
		}

		hdr = (u32 *)(&crf_hdr->stream_id + 1);

		if (unlikely((hdr[0] != stream->subtype_data.crf.hdr[0]) ||
			     (hdr[1] != stream->subtype_data.crf.hdr[1]))) {
			stream->stats.format_err++;

			crf_desc_flush(stream, avtp_desc_first, &desc_n, ts, &ts_n);

			net_rx_free(&desc[i]->desc);

			/* FIXME we should indicate packets were lost */
			continue;
		}

		if (likely(stream->pkt_received)) {
			if (unlikely(crf_hdr->sequence_num != ((stream->sequence_num + 1) & 0xff)))
				stream->stats.pkt_lost++;
		}

		if (unlikely(crf_hdr->mr != stream->mr)) {
			stream->mr = crf_hdr->mr;
			stream->stats.mr++;
		}

		stream->sequence_num = crf_hdr->sequence_num;

		stream->pkt_received++;

		if (unlikely(crf_hdr->tu)) {
			stream->stats.tu++;
		}

		for (j = 0; j < crf_fmt->timestamps_per_pdu; j++) {
			unsigned int ts32 = *(u32 *)((char *)(crf_hdr + 1) + j * 8 + 4);

			ts32 = ntohl(ts32);

			if (!j) {
				desc[i]->avtp_timestamp = ts32;

				if (!is_avtp_ts_valid(desc[i]->desc.ts, ts32, stream->max_transit_time, stream->max_timing_uncertainty, 0)) {
					stream->stats.media_tx_dropped += crf_fmt->timestamps_per_pdu;
					break;
				}
			}

			ts[ts_n].ts_nsec = ts32;
			ts[ts_n].flags = flags;

			ts_n++;
		}

		if (!desc_n)
			avtp_desc_first = (struct avtp_rx_desc **)&desc[i];

		if (!stats) {
			stats = 1;
			avtp_latency_stats(stream, desc[i]);
		}

		stream_listener_check_timestamps(stream, desc[i]->desc.ts, desc[i]->avtp_timestamp, 0);

		desc_n++;
	}

	crf_desc_flush(stream, avtp_desc_first, &desc_n, ts, &ts_n);
}


static int crf_check_format(const struct avdecc_format *format)
{
	const struct avdecc_format_crf_t *crf_fmt = &format->u.s.subtype_u.crf;
	unsigned int base_freq, freq, pdu_period, p, q;
	int rc = 0;

	/* 1722rev1-2016 Table 26 */
	/* Only audio sample type supported for now */
	switch (crf_fmt->type) {
	default:
	case CRF_TYPE_USER:
	case CRF_TYPE_VIDEO_FRAME:
	case CRF_TYPE_VIDEO_LINE:
	case CRF_TYPE_MACHINE_CYCLE:
		rc = -GENAVB_ERR_STREAM_PARAMS;
		goto err_format;
		break;

	case CRF_TYPE_AUDIO_SAMPLE:
		break;
	}

	base_freq = AVDECC_FMT_CRF_BASE_FREQUENCY(format);

	/* 1722rev1-2016 Table 27 */
	switch (crf_fmt->pull) {
	case CRF_PULL_1_1:
		p = 1, q = 1;
		break;

	case CRF_PULL_1000_1001:
		p = 1000, q = 1001;
		break;

	case CRF_PULL_1001_1000:
		p = 1001, q = 1000;
		break;

	case CRF_PULL_24_25:
		p = 24, q = 25;
		break;

	case CRF_PULL_25_24:
		p = 25, q = 24;
		break;

	case CRF_PULL_1_8:
		p = 1, q = 8;
		break;

	default:
		rc = -GENAVB_ERR_STREAM_PARAMS;
		goto err_format;
		break;
	}

	if ((crf_fmt->timestamps_per_pdu < CRF_TIMESTAMPS_PER_PDU_MIN) || (crf_fmt->timestamps_per_pdu > CRF_TIMESTAMPS_PER_PDU_MAX)) {
		rc = -GENAVB_ERR_STREAM_PARAMS;
		goto err_format;
	}

	if ((AVDECC_FMT_CRF_TIMESTAMP_INTERVAL(format) < CRF_TIMESTAMP_INTERVAL_MIN) || (AVDECC_FMT_CRF_TIMESTAMP_INTERVAL(format) > CRF_TIMESTAMP_INTERVAL_MAX)) {
		rc = -GENAVB_ERR_STREAM_PARAMS;
		goto err_format;
	}

	freq = (base_freq * p) / (AVDECC_FMT_CRF_TIMESTAMP_INTERVAL(format) * q);
	if ((freq < MCG_MIN_FREQUENCY) || (freq > MCG_MAX_FREQUENCY)) {
		rc = -GENAVB_ERR_STREAM_PARAMS;
		goto err_format;
	}

	pdu_period = ((u64)crf_fmt->timestamps_per_pdu * NSECS_PER_SEC * AVDECC_FMT_CRF_TIMESTAMP_INTERVAL(format) * q) / (base_freq * p);

	if (((pdu_period * CRF_TX_BATCH) < CFG_AVTP_MIN_LATENCY) || ((pdu_period * CRF_TX_BATCH) > CFG_AVTP_MAX_LATENCY)) {
		rc = -GENAVB_ERR_STREAM_PARAMS;
		goto err_format;
	}

	return 0;

err_format:
	return rc;
}

static void listener_crf_exit(struct stream_listener *stream)
{
	timer_destroy(&stream->subtype_data.crf.timer);
}

static int listener_crf_init(struct stream_listener *stream)
{
	struct avdecc_format const *format = &stream->format;
	const struct avdecc_format_crf_t *crf_fmt = &format->u.s.subtype_u.crf;
	struct crf_subtype_data *crf = &stream->subtype_data.crf;
	struct avtp_crf_hdr crf_hdr;
	u32 *hdr;

	stream->subtype_data.crf.timer.func = crf_timer_handler;
	stream->subtype_data.crf.timer.data = stream;

	if (timer_create(stream->common.avtp->timer_ctx, &stream->subtype_data.crf.timer, TIMER_TYPE_SYS, 0) < 0)
		goto err_timer;

	os_memset(&crf_hdr, 0, sizeof(crf_hdr));

	stream->subtype_data.crf.type = crf_fmt->type;

	/* Setup receive header pattern match */
	crf_hdr.pull = crf_fmt->pull;
	CRF_BASE_FREQUENCY_SET(&crf_hdr, AVDECC_FMT_CRF_BASE_FREQUENCY(format));
	crf_hdr.crf_data_length = htons(crf_fmt->timestamps_per_pdu * 8);
	crf_hdr.timestamp_interval = htons(AVDECC_FMT_CRF_TIMESTAMP_INTERVAL(format));

	hdr = (u32 *)(&crf_hdr.stream_id + 1);

	stream->subtype_data.crf.hdr[0] = hdr[0];
	stream->subtype_data.crf.hdr[1] = hdr[1];

	stream->common.flags |= STREAM_FLAG_NO_MEDIA;

	stream->net_rx = crf_net_rx;
	stream->exit = listener_crf_exit;

	crf->period_nominal = ((u64)NSECS_PER_SEC * avdecc_fmt_samples_per_timestamp(format, stream->class)) / avdecc_fmt_sample_rate(format);
	crf->period = crf->period_nominal;
	crf->period_err = crf->period / 64;
	crf->state = CRF_STATE_LOCKED;
	crf->ts_last_set = 0;
	crf->free_wheeling_to_locked_delay = (NSECS_PER_MS * (u64)CRF_FREE_WHEELING_TO_LOCKED_DELAY_MS) / crf->period;

	return 0;

err_timer:
	return -1;
}

int listener_crf_check(struct stream_listener *stream, struct avdecc_format const *format, u16 flags)
{
	int rc;

	rc = crf_check_format(format);
	if (rc < 0)
		goto err;

	stream->init = listener_crf_init;

	return 0;

err:
	return rc;
}

static void talker_crf_init(struct stream_talker *stream, unsigned int *hdr_len)
{
	struct avdecc_format const *format = &stream->format;

	*hdr_len = crf_prepare_header((struct avtp_crf_hdr *)stream->avtp_hdr, format, &stream->id);

	stream->subtype_data.crf.ts_period = ((u64)NSECS_PER_SEC * avdecc_fmt_samples_per_timestamp(format, stream->class)) / avdecc_fmt_sample_rate(format);

	stream->common.flags |= STREAM_FLAG_CLOCK_GENERATION | STREAM_FLAG_NO_MEDIA;

	stream->net_tx = crf_net_tx;
}

int talker_crf_check(struct stream_talker *stream, struct avdecc_format const *format, struct ipc_avtp_connect *ipc)
{
	int rc;

	rc = crf_check_format(format);
	if (rc < 0)
		goto err;

	stream->init = talker_crf_init;

err:
	return rc;
}

#endif /* CFG_AVTP_1722A */
