/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Stream handling functions
 @details
*/

#include "os/stdlib.h"
#include "os/clock.h"
#include "os/assert.h"

#include "common/log.h"
#include "common/avtp.h"
#include "common/net.h"
#include "common/61883_iidc.h"
#include "common/cvf.h"
#include "common/avdecc.h"

#include "stream.h"

#include "61883_iidc.h"
#include "cvf.h"
#include "acf.h"
#include "aaf.h"
#include "crf.h"
#include "media_clock.h"

#define STREAM_DESTROYED_FREE_DELAY_NS	1000000		/* Time delay between destroying and freeing stream */
#define STREAM_TALKER_MIN_PRESENTATION_TIME_OFFSET_NS	125000		/* Minimum presentation time offset : 125us */
#define STREAM_TALKER_MAX_PRESENTATION_TIME_OFFSET_NS	50000000	/* Maximun presentation time offset : 50ms */

static const unsigned int sr_class_max_pending_packets[SR_CLASS_MAX + 1] = {
	[SR_CLASS_A] = 8, // 1ms
	[SR_CLASS_B] = 4, // 1ms
	[SR_CLASS_C] = 1,
	[SR_CLASS_D] = 1,
	[SR_CLASS_E] = 1,
	[SR_CLASS_NONE] = 1
};

unsigned int avtp_stream_presentation_offset(struct stream_talker *stream)
{
	if (stream->subtype == AVTP_SUBTYPE_CRF)
		return crf_stream_presentation_offset(stream);
	else
		return _avtp_stream_presentation_offset(stream->max_transit_time, stream->latency);
}

static int stream_talker_format_check(struct stream_talker *stream, struct ipc_avtp_connect *ipc,
				struct avdecc_format const *format)
{
	int rc = GENAVB_SUCCESS;

	if (format->u.s.v != AVTP_VERSION_0) { /* version: 0 describes an AVTP stream payload */
		os_log(LOG_ERR, "stream_id(%016"PRIx64") version(%u) not supported (should be 0 for AVTP)\n",
				ntohll(stream->id), format->u.s.v);
		goto err_format;
	}

	/* FIXME subject to improvement to cover also video formats with variable bit rate*/
	if ((stream->common.flags & STREAM_FLAG_CUSTOM_TSPEC) && (format->u.s.subtype != AVTP_SUBTYPE_TSCF))
		goto err_format;

	switch (format->u.s.subtype) {
	case AVTP_SUBTYPE_61883_IIDC:
		rc = talker_stream_61883_iidc_check(stream, format, ipc);
		break;

	case AVTP_SUBTYPE_TSCF:
		if (!(stream->common.flags & STREAM_FLAG_CUSTOM_TSPEC))
			goto err_format;

		rc = talker_stream_acf_tscf_check(stream, format, ipc);
		break;

	case AVTP_SUBTYPE_AAF:
		rc = talker_stream_aaf_check(stream, format, ipc);
		break;

	case AVTP_SUBTYPE_CVF:
		rc = talker_stream_cvf_check(stream, format, ipc);
		break;
	case AVTP_SUBTYPE_MMA_STREAM:
	case AVTP_SUBTYPE_SVF:
	case AVTP_SUBTYPE_RVF:
	default:
		os_log(LOG_ERR, "stream_id(%016"PRIx64") subtype(%u) not supported\n", ntohll(stream->id), format->u.s.subtype);
		goto err_format;
	}

	return rc;

err_format:
	return -GENAVB_ERR_STREAM_PARAMS;
}

static int alternative_talker_format_check(struct stream_talker *stream, struct ipc_avtp_connect *ipc, struct avdecc_format const *format)
{
	int rc = GENAVB_SUCCESS;

	if ((stream->common.flags & STREAM_FLAG_CUSTOM_TSPEC) && (ipc->subtype != AVTP_SUBTYPE_NTSCF))
		goto err_format;

	switch (ipc->subtype) {
	case AVTP_SUBTYPE_NTSCF:
		if (!(stream->common.flags & STREAM_FLAG_CUSTOM_TSPEC))
			goto err_format;

		rc = talker_acf_ntscf_check(stream, ipc);
		break;

	case AVTP_SUBTYPE_CRF:
		rc = talker_crf_check(stream, format, ipc);
		break;

	default:
		os_log(LOG_ERR, "stream_id(%016"PRIx64") subtype(%u) not supported\n", ntohll(stream->id), ipc->subtype);
		goto err_format;
	}

	return rc;

err_format:
	return -GENAVB_ERR_STREAM_PARAMS;
}


static int stream_listener_format_check(struct stream_listener *stream, struct ipc_avtp_connect *ipc, struct avdecc_format const *format, u16 flags)
{
	int rc = GENAVB_SUCCESS;

	if (format->u.s.v != AVTP_VERSION_0) { /* version: 0 describes an AVTP stream payload */
		os_log(LOG_ERR, "stream_id(%016"PRIx64") version(%u) not supported (should be 0 for AVTP)\n",
				ntohll(stream->id), format->u.s.v);
		goto err_format;
	}

	/* Likely a misconfiguration since only a talker application is expected to specify the streams parameters */
	if (stream->common.flags & STREAM_FLAG_CUSTOM_TSPEC)
		goto err_format;

	switch (ipc->subtype) {
	case AVTP_SUBTYPE_61883_IIDC:
		rc = listener_stream_61883_iidc_check(stream, format, flags);
		break;

	case AVTP_SUBTYPE_CVF:
		rc = listener_stream_cvf_check(stream, format, flags);
		break;

	case AVTP_SUBTYPE_TSCF:
		rc = listener_stream_acf_tscf_check(stream, format, flags);
		break;

	case AVTP_SUBTYPE_AAF:
		rc = listener_stream_aaf_check(stream, format, flags);
		break;

	case AVTP_SUBTYPE_MMA_STREAM:
	case AVTP_SUBTYPE_SVF:
	case AVTP_SUBTYPE_RVF:
	default:
		os_log(LOG_ERR, "stream_id(%016"PRIx64") subtype(%u) not supported\n", ntohll(stream->id), ipc->subtype);
		goto err_format;
	}

	return rc;

err_format:
	return -GENAVB_ERR_STREAM_PARAMS;
}

static int alternative_listener_format_check(struct stream_listener *stream, struct ipc_avtp_connect *ipc, struct avdecc_format const *format, u16 flags)
{
	int rc = GENAVB_SUCCESS;

	/* Likely a misconfiguration since only a talker application is expected to specify the streams parameters */
	if (stream->common.flags & STREAM_FLAG_CUSTOM_TSPEC)
		goto err_format;

	switch (ipc->subtype) {
	case AVTP_SUBTYPE_NTSCF:
		rc = listener_acf_ntscf_check(stream, flags);
		break;

	case AVTP_SUBTYPE_CRF:
		rc = listener_crf_check(stream, format, flags);
		break;

	default:
		os_log(LOG_ERR, "stream_id(%016"PRIx64") subtype(%u) not supported\n", ntohll(stream->id), ipc->subtype);
		goto err_format;
	}

	return rc;

err_format:
	return -GENAVB_ERR_STREAM_PARAMS;
}


void avtp_latency_stats(struct stream_listener *stream, struct avtp_rx_desc *desc)
{
	stats_update(&stream->stats.avb_delay, stream->gptp_current - desc->desc.ts);

	stats_update(&stream->stats.avtp_delay, desc->avtp_timestamp - stream->gptp_current);
}

void stream_talker_stats_print(struct ipc_avtp_talker_stats *msg)
{
	struct talker_stats *stats = &msg->stats;

	stats_compute(&stats->sched_intvl);

	os_log(LOG_INFO, "stream_id(%016"PRIx64")\n", ntohll(msg->stream_id));

	os_log(LOG_INFO, "rx:     %10u, clock:   %10u, tx: %10u, rx err: %10u, clock err: %10u, gptp err: %10u\n",
	       stats->media_rx, stats->clock_rx, stats->tx, stats->media_err, stats->clock_err, stats->gptp_err);
	os_log(LOG_INFO, "tx err: %10u, partial: %10u, media underrun: %10u  clock invalid: %10u sched intvl: % 10d/% 10d/% 10d (ns)\n",
		stats->tx_err, stats->partial, stats->media_underrun, stats->clock_invalid,
		stats->sched_intvl.min, stats->sched_intvl.mean, stats->sched_intvl.max);
}

static void stream_talker_stats_dump(struct stream_talker *stream, struct ipc_tx *tx)
{
	struct ipc_desc *desc;
	struct ipc_avtp_talker_stats *msg;

	desc = ipc_alloc(tx, sizeof(*msg));
	if (!desc)
		goto err_ipc_alloc;

	desc->type = IPC_AVTP_STREAM_TALKER_STATS;
	desc->len = sizeof(*msg);
	desc->flags = 0;

	msg = (struct ipc_avtp_talker_stats *)&desc->u;

	msg->stream_id = stream->id;
	os_memcpy(&msg->stats, &stream->stats, sizeof(stream->stats));

	if (ipc_tx(tx, desc) < 0)
		goto err_ipc_tx;

	clock_grid_consumer_stats_dump(&stream->consumer, tx);

	stats_reset(&stream->stats.sched_intvl);
	return;

err_ipc_tx:
	ipc_free(tx, desc);

err_ipc_alloc:
	return;
}

void stream_listener_stats_print(struct ipc_avtp_listener_stats *msg)
{
	struct listener_stats *stats = &msg->stats;

	os_log(LOG_INFO, "stream_id(%016"PRIx64")\n", ntohll(msg->stream_id));

	stats_compute(&stats->avb_delay);
	stats_compute(&stats->avtp_delay);
	stats_compute(&stats->batch);

	os_log(LOG_INFO, "rx: %10u, tx: %10u, tx err:  %10u, clock: %10u, lost: %10u, subtype err:   %10u\n",
		stats->rx, stats->media_tx, stats->media_tx_err, stats->clock_tx, stats->pkt_lost, stats->subtype_err);

	os_log(LOG_INFO, "mr: %10u, tu: %10u, dropped: %10u, early: %10u, late: %10u, subformat err: %10u\n",
		stats->mr, stats->tu, stats->media_tx_dropped, stats->early_timestamp, stats->late_timestamp, stats->format_err);

	os_log(LOG_INFO,"now-rx_ts %4d/%4d/%4d     avtp_ts-now %4d/%4d/%4d (us)     batch %2d/%2d/%2d/%2"PRIu64"\n",
		stats->avb_delay.min/1000, stats->avb_delay.mean/1000, stats->avb_delay.max/1000,
		stats->avtp_delay.min/1000, stats->avtp_delay.mean/1000, stats->avtp_delay.max/1000,
		stats->batch.min, stats->batch.mean, stats->batch.max, stats->batch.variance);

	if (msg->clock_rec_enabled)
		media_clock_rec_stats_print(&msg->clock_stats);
}


static void stream_listener_stats_dump(struct stream_listener *stream, struct ipc_tx *tx)
{
	struct ipc_desc *desc;
	struct ipc_avtp_listener_stats *msg;

	desc = ipc_alloc(tx, sizeof(*msg));
	if (!desc)
		goto err_ipc_alloc;

	desc->type = IPC_AVTP_STREAM_LISTENER_STATS;
	desc->len = sizeof(*msg);
	desc->flags = 0;

	msg = (struct ipc_avtp_listener_stats *)&desc->u;

	msg->stream_id = stream->id;
	os_memcpy(&msg->stats, &stream->stats, sizeof(stream->stats));

	if (stream->source) {
		struct clock_grid_producer_stream *producer = &stream->source->grid.producer.u.stream;

		if (producer->rec) {
			msg->clock_rec_enabled = 1;
			media_clock_rec_stats_dump(producer->rec, &msg->clock_stats);
		}
	} else
		msg->clock_rec_enabled = 0;

	stats_reset(&stream->stats.avb_delay);
	stats_reset(&stream->stats.avtp_delay);
	stats_reset(&stream->stats.batch);

	if (ipc_tx(tx, desc) < 0)
		goto err_ipc_tx;

	return;

err_ipc_tx:
	ipc_free(tx, desc);

err_ipc_alloc:
	return;
}

void stream_stats_dump(struct avtp_port *port, struct ipc_tx *tx)
{
	struct stream_talker *stream_talker;
	struct stream_listener *stream_listener;
	struct list_head *entry;

	for (entry = list_first(&port->talker); entry != &port->talker; entry = list_next(entry)) {
		stream_talker = container_of(entry, struct stream_talker, common.list);

		stream_talker_stats_dump(stream_talker, tx);
	}

	for (entry = list_first(&port->listener); entry != &port->listener; entry = list_next(entry)) {
		stream_listener = container_of(entry, struct stream_listener, common.list);

		stream_listener_stats_dump(stream_listener, tx);
	}
}

/** Adds a stream to the port talker stream list
 *
 * \return		none
 * \param port		pointer to port context
 * \param stream	pointer to talker stream
 */
static void stream_talker_add(struct avtp_port *port, struct stream_talker *stream)
{
	list_add_tail(&port->talker, &stream->common.list);

	stream->common.avtp->stream_talker_count++;
}

/** Searches for a stream in the port talker stream list (based on stream id)
 *
 * \return		pointer to the matching stream, NULL if the stream was not found
 * \param port		pointer to port context
 * \param stream_id	stream id to match
 */
struct stream_talker *stream_talker_find(struct avtp_port *port, void *stream_id)
{
	struct stream_talker *stream;
	struct list_head *entry;

	for (entry = list_first(&port->talker); entry != &port->talker; entry = list_next(entry)) {
		stream = container_of(entry, struct stream_talker, common.list);

		if (cmp_64(&stream->id, stream_id))
			return stream;
	}

	return NULL;
}

/** Calculates transmit batch for the stream
 *
 */
static unsigned int stream_tx_batch(const struct stream_talker *stream)
{
	unsigned int tx_batch = 0;
	unsigned int align_batch, min_batch, avtp_min_batch, max_batch, avtp_max_batch;
	unsigned int samples_per_packet;
	unsigned int packet_rate_p, packet_rate_q;

	switch (stream->subtype) {
	case AVTP_SUBTYPE_AAF:
	case AVTP_SUBTYPE_61883_IIDC:
	case AVTP_SUBTYPE_CVF:
	case AVTP_SUBTYPE_TSCF:
	case AVTP_SUBTYPE_NTSCF:
		samples_per_packet = stream->frames_per_packet;

		/* align wakeup period to whole packets */
		align_batch = stream->frames_per_interval;

		/* Set wakeup period so that generated packet number, per period, is less than half the maximum supported transmit burst */
		max_batch = ((NET_TX_BATCH / 2) / align_batch) * align_batch;

		packet_rate_p = stream->sample_rate;
		packet_rate_q = samples_per_packet;

		min_batch = align_batch;

		avtp_min_batch = ((u64)CFG_AVTP_MIN_LATENCY * packet_rate_p + (u64)packet_rate_q * NSECS_PER_SEC - 1) / ((u64)packet_rate_q * NSECS_PER_SEC);

		avtp_max_batch = ((u64)CFG_AVTP_MAX_LATENCY * packet_rate_p) / ((u64)packet_rate_q * NSECS_PER_SEC);
		avtp_max_batch = (avtp_max_batch / align_batch) * align_batch;

		while (min_batch < avtp_min_batch)
			min_batch += align_batch;

		if (max_batch > avtp_max_batch)
			max_batch = avtp_max_batch;

		/* For high sample rates/low samples per packet or low sample rates/high samples per packet */
		if (min_batch > max_batch) {
			os_log(LOG_ERR, "invalid latency/batch target min_batch: %d, max_batch: %d, latency_min_batch: %d, latency_max_batch: %d\n", min_batch, max_batch, avtp_min_batch, avtp_max_batch);
			goto err;
		}

		tx_batch = ((u64)stream->latency * packet_rate_p) / ((u64)packet_rate_q * NSECS_PER_SEC);
		tx_batch = (tx_batch / align_batch) * align_batch;

		if (tx_batch < min_batch)
			tx_batch = min_batch;
		else if (tx_batch > max_batch)
			tx_batch = max_batch;

		break;

	case AVTP_SUBTYPE_CRF:
		tx_batch = CRF_TX_BATCH;
		break;

	default:
		goto err;
		break;
	}

	os_assert(tx_batch <= NET_TX_BATCH);

	return tx_batch;
err:
	return 0;
}

int stream_clock_consumer_enable(struct stream_talker *stream)
{
	unsigned int ts_freq_p, ts_freq_q, packet_freq_p, packet_freq_q;
	unsigned int wake_freq_p, wake_freq_q;
	unsigned int ps;

	if (!(stream->common.flags & STREAM_FLAG_CLOCK_GENERATION))
		return 0;

	ts_freq_p = stream->sample_rate;

	if (stream->subtype == AVTP_SUBTYPE_CRF)
		ts_freq_p *= 2;

	ts_freq_q = stream->samples_per_timestamp;

	if (!ts_freq_p || !ts_freq_q) {
		os_log(LOG_ERR, "talker(%p) invalid ts_freq: %u/%u\n", stream, ts_freq_p, ts_freq_q);
		return -1;
	}

	packet_freq_p = stream->sample_rate;
	packet_freq_q = stream->frames_per_packet;

	stream->time_per_packet = ((u64)NSECS_PER_SEC * packet_freq_q) / packet_freq_p;

	stream->latency = stream->tx_batch * stream->time_per_packet;

	wake_freq_p = packet_freq_p;
	wake_freq_q = packet_freq_q * stream->tx_batch;

	if (!wake_freq_p || !wake_freq_q) {
		os_log(LOG_ERR, "talker(%p) invalid wake_freq: %u/%u\n", stream, wake_freq_p, wake_freq_q);
		return -1;
	}

	os_log(LOG_INFO, "talker(%p) media_rx(%p) frames per packet %u, payload size %u, syt interval %u, syt freq %u, latency %u, batch %u\n",
		stream, &stream->media, stream->frames_per_packet, stream->payload_size,
		stream->samples_per_timestamp, ts_freq_p / ts_freq_q, stream->latency, stream->tx_batch);

	/* Align on media clock grid */
	/* IEEE 1722-2016 4.3.5 and 10.8 */
	/* n x Ps - Ps / 20 < Toffset < n x Ps + Ps / 20. n > 0 */
	/* In practice we just round up to a multiple of Ps = (NSECS_PER_SEC / sample_rate) */
	ps = (NSECS_PER_SEC + stream->sample_rate / 2) / stream->sample_rate;

	if (clock_domain_init_consumer(stream->domain, &stream->consumer, avtp_stream_presentation_offset(stream),
		ts_freq_p, ts_freq_q, ps, sr_class_prio(stream->class)) < 0)
		goto err_init;

	if (stream->subtype == AVTP_SUBTYPE_CRF) {
		if (os_timer_create(&stream->subtype_data.crf.t, stream->domain->source->clock_id, 0, crf_os_timer_handler, stream->priv) < 0)
			goto err_timer_create;

		if (os_timer_start(&stream->subtype_data.crf.t, 0, wake_freq_p, wake_freq_q, 0) < 0)
			goto err_timer_start;
	} else {
		if (clock_domain_init_consumer_wakeup(stream->domain, &stream->consumer, wake_freq_p, wake_freq_q) < 0)
			goto err_init_wakeup;
	}

	stream->consumer_enabled = true;

	return 0;

err_timer_start:
	os_timer_destroy(&stream->subtype_data.crf.t);

err_timer_create:
err_init_wakeup:
	clock_domain_exit_consumer(&stream->consumer);

err_init:
	return -1;
}

void stream_clock_consumer_disable(struct stream_talker *stream)
{
	if ((stream->common.flags & STREAM_FLAG_CLOCK_GENERATION) && (stream->consumer_enabled)) {
		if (stream->subtype == AVTP_SUBTYPE_CRF)
			os_timer_destroy(&stream->subtype_data.crf.t);
		else
			clock_domain_exit_consumer_wakeup(&stream->consumer);

		clock_domain_exit_consumer(&stream->consumer);

		stream->consumer_enabled = false;
	}
}

/** Creates a talker stream context
 *
 * Allocates memory for the stream context and initializes handles to media stack, media clock capture and network layers
 *
 * \return	pointer to created stream or NULL if the stream couldn't be created/already exists.
 * \param avtp	pointer to avtp global context
 * \param ipc	ipc connect message (with all the stream parameters)
 */
struct stream_talker *stream_talker_create(struct avtp_ctx *avtp, struct avtp_port *port, struct ipc_avtp_connect *ipc)
{
	struct stream_talker *stream;
	struct net_address addr;
	unsigned int hdr_len = 0;
	unsigned int flags;
	int rc;

	stream = stream_talker_find(port, &ipc->stream_id);
	if (stream) {
		os_log(LOG_ERR, "Destroying previously existing stream with stream_id(%016"PRIx64")\n", get_ntohll(ipc->stream_id));
		stream_talker_destroy(stream, NULL);
	}

	stream = os_malloc(sizeof(*stream));
	if (!stream)
		goto err_allocation_failed;

	os_memset(stream, 0, sizeof(*stream));

	if (!sr_class_enabled(ipc->stream_class))
		goto err_format;

	stream->common.flags |= STREAM_FLAG_SR | STREAM_FLAG_VLAN;

	stream->consumer_enabled = false;

	stream->domain = clock_domain_get(avtp, ipc->clock_domain);
	if (!stream->domain) {
		os_log(LOG_ERR, "stream_id(%016"PRIx64"), clock_domain_get(%d) failed\n", ntohll(stream->id), ipc->clock_domain);
		goto err_clock_domain;
	}

	stream->locked_count = stream->domain->locked_count;

	/* Legacy support, set the domain source internally */
	if (ipc->clock_domain < GENAVB_CLOCK_DOMAIN_0) {
		if (clock_domain_set_source_legacy(stream->domain, avtp, ipc) < 0)
			goto err_clock_domain;
	}

	/* Stream params inherited from the application (i.e. not retrieved from AVDDECC format) */
	if (ipc->flags & GENAVB_STREAM_FLAGS_CUSTOM_TSPEC)
		stream->common.flags |= STREAM_FLAG_CUSTOM_TSPEC;

	stream->class = ipc->stream_class;
	stream->direction = AVTP_DIRECTION_TALKER;
	stream->subtype = ipc->subtype;
	stream->port = ipc->port;
	stream->latency = ipc->talker.latency;
	stream->common.avtp = avtp;

	copy_64(&stream->id, &ipc->stream_id);
	stream->format = ipc->format;
	stream->media_count = 0;

	stream->clock_gptp = port->clock_gptp;

	stream->priv = avtp->priv;

	os_memset(stream->header_template, 0, HEADER_TEMPLATE_SIZE);

	stream->header_len = net_add_eth_header(stream->header_template, ipc->dst_mac, ETHERTYPE_VLAN);

	/* Vlan id and priority are overriden by the network layer */
	stream->header_len += net_add_vlan_header(stream->header_template + stream->header_len, ETHERTYPE_AVTP, 0, 0, 0);

	stream->avtp_hdr = (struct avtp_data_hdr *)(stream->header_template + stream->header_len);

	/*
	* Initialize stream parameters and check format - hdr_len is an output
	*/
	if (is_avtp_stream(ipc->subtype))
		rc = stream_talker_format_check(stream, ipc, &stream->format);
	else if (is_avtp_alternative(ipc->subtype))
		rc = alternative_talker_format_check(stream, ipc, &stream->format);
	else
		rc = -1;

	if (rc < 0) {
		os_log(LOG_ERR, "stream_id(%016"PRIx64"), stream format check failed: rc = %d\n", ntohll(stream->id), rc);
		goto err_format;
	}

	if (stream->common.flags & STREAM_FLAG_CUSTOM_TSPEC) {
		if (avtp_fmt_sample_size(ipc->subtype, &stream->format))
			stream->frames_per_packet = ipc->talker.max_frame_size / avtp_fmt_sample_size(ipc->subtype, &stream->format);
		else
			stream->frames_per_packet = ipc->talker.max_frame_size;

		stream->payload_size = ipc->talker.max_frame_size;

		stream->frames_per_interval = ipc->talker.max_interval_frames;

		stream->sample_rate = ((u64)stream->payload_size * NSECS_PER_SEC * sr_class_interval_q(stream->class)) / sr_class_interval_p(stream->class);

		stream->samples_per_timestamp = samples_per_interval(stream->sample_rate, stream->class);
	} else {
		stream->frames_per_packet = __avdecc_fmt_samples_per_packet(&stream->format, stream->class, &stream->frames_per_interval);

		stream->payload_size = stream->frames_per_packet * avdecc_fmt_sample_stride(&ipc->format);

		stream->sample_rate = avdecc_fmt_sample_rate(&stream->format);

		stream->samples_per_timestamp = avdecc_fmt_samples_per_timestamp(&stream->format, stream->class);
	}

	stream->init(stream, &hdr_len);

	stream->header_len += hdr_len;

	if (ipc->flags & IPC_AVTP_FLAGS_MAX_TRANSIT_TIME_VALID) {
		if ((ipc->talker.max_transit_time < STREAM_TALKER_MIN_PRESENTATION_TIME_OFFSET_NS) ||
		    (ipc->talker.max_transit_time > STREAM_TALKER_MAX_PRESENTATION_TIME_OFFSET_NS)) {
			os_log(LOG_ERR, "stream_id(%016"PRIx64"), stream max_transit_time(%u) out of range [%u, %u]\n",
				ntohll(stream->id), ipc->talker.max_transit_time, STREAM_TALKER_MIN_PRESENTATION_TIME_OFFSET_NS, STREAM_TALKER_MAX_PRESENTATION_TIME_OFFSET_NS);
			goto err_transit_time;
		}

		stream->max_transit_time = ipc->talker.max_transit_time;
	} else {
		stream->max_transit_time = sr_class_max_transit_time(stream->class);
	}

	addr.ptype = PTYPE_AVTP;
	addr.port = ipc->port;
	addr.vlan_id = ipc->talker.vlan_id;
	addr.priority = ipc->talker.priority;
	addr.u.avtp.subtype = ipc->subtype;
	addr.u.avtp.sr_class = stream->class;
	copy_64(addr.u.avtp.stream_id, &stream->id);

	if (net_tx_init(&stream->tx, &addr) < 0)
		goto err_tx_init;

	flags = 0;

	if (stream->common.flags & STREAM_FLAG_MEDIA_WAKEUP)
		flags |= MEDIA_FLAG_WAKEUP;

	stats_init(&stream->stats.sched_intvl, 31, NULL, NULL);
	if (os_clock_gettime32(stream->clock_gptp, &stream->gptp_current) < 0)
		stream->stats.gptp_err++;

	stream->tx_batch = stream_tx_batch(stream);
	if (!stream->tx_batch)
		goto err_tx_batch;

	if (stream_clock_consumer_enable(stream) < 0)
		goto err_clock_enable;

	if (!(stream->common.flags & STREAM_FLAG_NO_MEDIA))
		if (media_rx_init(&stream->media, &stream->id, avtp->priv, flags, stream->header_len, stream_presentation_offset(stream->max_transit_time, stream->latency)) < 0)
			goto err_rx_init;

	stream_talker_add(port, stream);

	os_log(LOG_INFO, "talker_stream_id(%016"PRIx64") class(%d) format(%016"PRIx64") domain(%p): %d\n",
		ntohll(stream->id), stream->class, get_ntohll(ipc->format.u.raw), stream->domain, stream->domain->id);

	return stream;

err_rx_init:
	stream_clock_consumer_disable(stream);

err_clock_enable:
err_tx_batch:
	net_tx_exit(&stream->tx);

err_tx_init:
err_transit_time:
err_format:
err_clock_domain:
	os_free(stream);

err_allocation_failed:
	return NULL;
}

/** Destroys a talker stream context
 *
 * \return		none
 * \param stream	pointer to talker stream to destroy
 */
void stream_talker_destroy(struct stream_talker *stream, struct ipc_tx *tx)
{
	os_log(LOG_INFO, "talker_stream_id(%016"PRIx64")\n", ntohll(stream->id));

	if (tx)
		stream_talker_stats_dump(stream, tx);

	stream_clock_consumer_disable(stream);

	if (!(stream->common.flags & STREAM_FLAG_NO_MEDIA))
		media_rx_exit(&stream->media);

	net_tx_exit(&stream->tx);

	list_del(&stream->common.list);
	list_add_tail(&stream->common.avtp->stream_destroyed, &stream->common.list);

	stream->common.avtp->stream_talker_count--;
}

/** Adds a stream to the port listener stream list
 *
 * \return		none
 * \param port		pointer to port context
 * \param stream	pointer to listener stream
 */
static void stream_listener_add(struct avtp_port *port, struct stream_listener *stream)
{
	list_add_tail(&port->listener, &stream->common.list);

	stream->common.avtp->stream_listener_count++;
}

/** Searches for a stream in the port listener stream list (based on stream id and class)
 *
 * \return		pointer to the matching stream, NULL if the stream was not found
 * \param port		pointer to port context
 * \param stream_id	stream id to match
 * \param class		stream class to match
 */
struct stream_listener *stream_listener_find(struct avtp_port *port, void *stream_id)
{
	struct stream_listener *stream;
	struct list_head *entry;

	for (entry = list_first(&port->listener); entry != &port->listener; entry = list_next(entry)) {
		stream = container_of(entry, struct stream_listener, common.list);

		if (cmp_64(&stream->id, stream_id))
			return stream;
	}

	return NULL;
}


static unsigned int stream_rx_batch(const struct stream_listener *stream, unsigned int *latency)
{
	unsigned int rx_batch;

	/* FIXME, take into account media queue batch size */

	switch (stream->subtype) {
	case AVTP_SUBTYPE_AAF:
	case AVTP_SUBTYPE_61883_IIDC:
	case AVTP_SUBTYPE_CVF:
		rx_batch = sr_class_max_pending_packets[stream->class];
		*latency = (rx_batch * sr_class_interval_p(stream->class)) / sr_class_interval_q(stream->class);
		break;

	case AVTP_SUBTYPE_TSCF:
	case AVTP_SUBTYPE_NTSCF:
	case AVTP_SUBTYPE_CRF:
		rx_batch = 1;
		*latency = 0;
		break;

	default:
		rx_batch = 0;
		break;
	}

	return rx_batch;
}

/** Creates a listener stream context
 *
 * Allocates memory for the stream context and initializes handles to media stack, media clock recovery and network layers
 *
 * \return	pointer to created stream or NULL if the stream couldn't be created/already exists.
 * \param avtp	pointer to avtp global context
 * \param ipc	ipc connect message (with all the stream parameters)
 */
struct stream_listener *stream_listener_create(struct avtp_ctx *avtp, struct avtp_port *port, struct ipc_avtp_connect *ipc)
{
	struct stream_listener *stream;
	struct net_address addr;
	unsigned int rx_batch, rx_latency;
	int rc;

	if (!sr_class_enabled(ipc->stream_class)) {
		os_log(LOG_ERR, "listener_stream_id(%016"PRIx64") class invalid: %d\n", get_ntohll(ipc->stream_id), ipc->stream_class);
		goto err_class_invalid;
	}

	stream = stream_listener_find(port, ipc->stream_id);
	if (stream) {
		os_log(LOG_ERR, "Destroying previously existing stream with stream_id(%016"PRIx64")\n", get_ntohll(ipc->stream_id));
		stream_listener_destroy(stream, NULL);
	}

	stream = os_malloc(sizeof(*stream));
	if (!stream)
		goto err_allocation_failed;

	os_memset(stream, 0, sizeof(*stream));

	stream->class = ipc->stream_class;
	stream->direction = AVTP_DIRECTION_LISTENER;
	stream->common.avtp = avtp;

	copy_64(&stream->id, ipc->stream_id);

	stream->subtype = ipc->subtype;

	stream->format = ipc->format;

	stream->clock_gptp = port->clock_gptp;

	/*
	* Initialize stream parameters and check format
	*/
	if (is_avtp_stream(ipc->subtype))
		rc = stream_listener_format_check(stream, ipc, &stream->format, ipc->flags);
	else if (is_avtp_alternative(ipc->subtype))
		rc = alternative_listener_format_check(stream, ipc, &stream->format, ipc->flags);
	else
		rc = -1;

	if (rc < 0) {
		os_log(LOG_ERR, "stream_id(%016"PRIx64"), stream format check failed: rc = %d\n", ntohll(stream->id), rc);
		goto err_format;
	}

	if ((rc = stream->init(stream)) < 0)
		goto err_init;

	stream->max_transit_time = sr_class_max_transit_time(stream->class);
	stream->max_timing_uncertainty = sr_class_max_timing_uncertainty(stream->class);

	addr.ptype = PTYPE_AVTP;
	addr.port = ipc->port;
	addr.u.avtp.subtype = ipc->subtype;
	addr.u.avtp.sr_class = stream->class;
	copy_64(addr.u.avtp.stream_id, &stream->id);

	rx_batch = stream_rx_batch(stream, &rx_latency);
	if (!rx_batch)
		goto err_rx_batch;

	if (is_avtp_stream(ipc->subtype))
		rc = net_rx_init_multi(&stream->rx, &addr, avtp_stream_net_rx, rx_batch, rx_latency, avtp->priv);
	else
		rc = net_rx_init_multi(&stream->rx, &addr, avtp_alternative_net_rx, rx_batch, rx_latency, avtp->priv);

	if (rc < 0)
		goto err_net;

	if (!(stream->common.flags & STREAM_FLAG_NO_MEDIA))
		if (media_tx_init(&stream->media, &stream->id) < 0)
			goto err_media;

	stream->domain = clock_domain_get(avtp, ipc->clock_domain);
	if (!stream->domain) {
		os_log(LOG_ERR, "stream_id(%016"PRIx64"), clock_domain_get(%d) failed\n", ntohll(stream->id), ipc->clock_domain);
		goto err_clock_domain;
	}

	/* Legacy support, set the domain source internally */
	if (ipc->clock_domain < GENAVB_CLOCK_DOMAIN_0) {
		if (clock_domain_set_source_legacy(stream->domain, avtp, ipc) < 0)
			goto err_clock_domain;
	}

	/* Setup stream source (new and legacy clock API) */
	if ((ipc->flags & IPC_AVTP_FLAGS_MCR)
	&& (clock_domain_is_source_stream(stream->domain, &stream->id)))
		if (__clock_domain_update_source(stream->domain, stream->domain->source, stream) < 0) {
			os_log(LOG_ERR, "stream_id(%016"PRIx64"), __clock_domain_update_source failed\n", ntohll(stream->id));
			goto err_clock_source;
		}

	/*
	 * FIXME, we should really fail at this stage.
	 * Need to fix AVDECC/entities
	 */
	if (stream->source) {
		os_log(LOG_INFO, "stream is a clock source\n");

		if (stream->source->grid.producer.u.stream.rec)
			os_log(LOG_INFO, "MCR enabled\n");
		else
			os_log(LOG_INFO, "MCR disabled\n");
	}

	os_memcpy(stream->dst_mac, ipc->dst_mac, 6);
	stream->port = ipc->port;

	if (net_add_multi(&stream->rx, stream->port, stream->dst_mac) < 0) {
		os_log(LOG_ERR, "listener_stream_id(%016"PRIx64") cannot register MC address\n", ntohll(stream->id));
		goto err_multi;
	}

	stats_init(&stream->stats.avb_delay, 31, NULL, NULL);
	stats_init(&stream->stats.avtp_delay, 31, NULL, NULL);
	stats_init(&stream->stats.batch, 31, NULL, NULL);

	stream_listener_add(port, stream);

	os_log(LOG_INFO, "listener_stream_id(%016"PRIx64") class(%d) format(%016"PRIx64") domain(%p): %d\n",
		ntohll(stream->id), stream->class, get_ntohll(&stream->format), stream->domain, stream->domain->id);

	return stream;

err_multi:
	if (stream->source)
		clock_source_close(stream->source);

err_clock_source:
err_clock_domain:
	if (!(stream->common.flags & STREAM_FLAG_NO_MEDIA))
		media_tx_exit(&stream->media);

err_media:
	net_rx_exit(&stream->rx);

err_net:
	if (stream->exit)
		stream->exit(stream);

err_rx_batch:
err_init:
err_format:
	os_free(stream);

err_allocation_failed:
err_class_invalid:
	return NULL;
}

/** Destroys a listener stream context
 *
 * \return		none
 * \param stream	pointer to listener stream to destroy
 */
void stream_listener_destroy(struct stream_listener *stream, struct ipc_tx *tx)
{
	os_log(LOG_INFO, "listener_stream_id(%016"PRIx64")\n", ntohll(stream->id));

	if (stream->exit)
		stream->exit(stream);

	net_del_multi(&stream->rx, stream->port, stream->dst_mac);

	net_rx_exit(&stream->rx);

	if (!(stream->common.flags & STREAM_FLAG_NO_MEDIA))
		media_tx_exit(&stream->media);

	if (stream->source)
		clock_source_close(stream->source);

	list_del(&stream->common.list);
	list_add_tail(&stream->common.avtp->stream_destroyed, &stream->common.list);

	stream->common.avtp->stream_listener_count--;

	if (tx)
		stream_listener_stats_dump(stream, tx);
}

void stream_free_all(struct avtp_ctx *avtp)
{
	struct stream_common *stream;
	struct list_head *entry;

	entry = list_first(&avtp->stream_destroyed);
	while (entry != &avtp->stream_destroyed) {
		stream = container_of(entry, struct stream_common, list);
		entry = list_next(entry);

		list_del(&stream->list);
		os_free(stream);
	}
}

void avtp_stream_free(void *avtp_ctx, u64 current_time)
{
	struct avtp_ctx *avtp = (struct avtp_ctx*)avtp_ctx;
	struct stream_common *stream;
	struct list_head *entry;

	entry = list_first(&avtp->stream_destroyed);
	while (entry != &avtp->stream_destroyed) {
		stream = container_of(entry, struct stream_common, list);
		entry = list_next(entry);

		if (!(stream->flags & STREAM_FLAG_DESTROYED)) {
			stream->flags |= STREAM_FLAG_DESTROYED;
			stream->destroy_time = current_time;
		} else if ((current_time - stream->destroy_time) >= STREAM_DESTROYED_FREE_DELAY_NS) {
			list_del(&stream->list);

			os_log(LOG_INFO, "stream(%p)\n", stream);

			os_free(stream);
		}
	}
}

int stream_media_rx(struct stream_talker *stream, struct media_rx_desc **media_desc_array, u32 *ts, unsigned int *flags, unsigned int * alignment_ts)
{
	u32 i;
	int rc;
	struct media_rx_desc *media_desc;
	unsigned int do_align = 0, underrun = 0, tx_batch;

	/* Get samples from media stack. If running behind (late > 0), try to read one extra packet to progressively recover. */
	if (stream->late)
		tx_batch = stream->tx_batch + 1;
	else
		tx_batch = stream->tx_batch;

	if (unlikely((rc = media_rx(&stream->media, media_desc_array, tx_batch)) < 0)) {
		stream->stats.media_err++;
//		os_log(LOG_ERR, "talker(%p) media.rx failed\n", stream);
		goto media_rx_fail;
	}

	/* Keep track of late packets to trigger underrun condition
	 * Only trigger underrun if we have some received some data.
	 * "Late" is relative to number of "number of wakeups" x "tx_batch",
	 * we increment it each time we read less than stream->tx_batch packets from the stack,
	 * we decrement it each time we read more */
	if ((stream->media_count != 0) || (rc != 0))
		stream->late += stream->tx_batch - (unsigned int)rc;

	stream->stats.media_rx += rc;

	for (i = 0; i < rc; i++) {
		media_desc = media_desc_array[i];
		if (media_desc->ts_n) {
			do_align = 1;
			stream->late = 0;
			*alignment_ts = media_desc->avtp_ts[0].val; // Take the first timestamp of the first packet with timestamps
			//FIXME if the timestamp is not for the first byte of the first packet, this can add an error of up to batch size
			break;
		}
	}

	if ((stream->late > stream->tx_batch) && rc) {
		stream->stats.media_underrun++;
		stream->late = 0;
		underrun = 1;
	}

	if (unlikely((((stream->media_count == 0) && rc) || underrun || stream_domain_phase_change(stream)) && !do_align)) {
		do_align = 1;
		*alignment_ts = stream->gptp_current + avtp_stream_presentation_offset(stream);
	}

	if (unlikely(do_align))
		*flags |= MCG_FLAGS_DO_ALIGN;

	return rc;

media_rx_fail:
	stream->late = 0;

	return rc;
}


int stream_tx_flow_control(struct stream_talker *stream, unsigned int *tx_batch)
{
	unsigned int tx_avail;
	int rc = 0;

	/* check the amount of free space in the network transmit queue */
	tx_avail = net_tx_available(&stream->tx);

	if (tx_avail < *tx_batch) {
		/* transmit is congested, enable flow control if not yet done */
		if(!stream->tx_event_enabled) {
			/*disable media queue events so we stop dequeuing data from the media interface */
			media_rx_event_disable(&stream->media);

			/* register event notification from the network interface to signal
			the avtp thread when there is free space again in the transmit queue */
			net_tx_event_enable(&stream->tx, stream->priv);

			stream->tx_event_enabled = 1;

			/* if the transmit queue is full, we do not transmit any data at this round,
			else we just transmit what can be transmitted */
			if (!tx_avail)
				rc = -1;
			else
				*tx_batch = tx_avail;
		} else
			/* we are already in flow controlled state, nothing should be transmitted */
			rc = -1;
	} else {
		/* network transmit queue is not congested (at least one batch of free slots in the queue), make sure flow control is now disabled */
		if (stream->tx_event_enabled) {
			/* disable network queue events, i.e. do not listen anymore to the event from network interface */
			net_tx_event_disable(&stream->tx);

			/* enable media queue events and allow dequeuing from the media interface */
			media_rx_event_enable(&stream->media);

			/* back to normal operation, no restriction on transmit */
			stream->tx_event_enabled = 0;
		}
	}

	os_log(LOG_DEBUG, "stream(%p) tx_batch %u tx_avail %u flow_control_enabled %u rc %d\n", stream, *tx_batch, tx_avail, stream->tx_event_enabled, rc);

	return rc;
}
