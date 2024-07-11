/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2019, 2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVTP Audio Format (AAF) handling functions
 @details
*/

#ifdef CFG_AVTP_1722A

#include "common/log.h"

#include "avtp.h"
#include "aaf.h"

static void aaf_net_tx(struct stream_talker *stream)
{
	u32 get_ts, i;
	int n_now;
	void *buf;
	struct media_rx_desc *media_desc_array[NET_TX_BATCH], *media_desc;
	struct net_tx_desc *net_desc;
	u32 ts[NET_TX_BATCH];
	int rc;
	int ts_n;
	unsigned int ts_batch;
	unsigned int flags = 0;
	unsigned int partial;
	unsigned int sparse = stream->subtype_data.aaf.sparse;
	unsigned int alignment_ts = 0;

	// Get Audio media from media stack
	if (unlikely((rc = stream_media_rx(stream, media_desc_array, ts, &flags, &alignment_ts)) < 0)) {
		goto media_rx_fail;
	}

	if (sparse) {
		/* The last packet with a timestamp was (stream->subtype_data.aaf.tx_count - stream->frame_with_ts) packets ago.
		 * We just received rc new packets.
		 * So the number of necessary timestamps is the sum of these 2 values, divided by the number of packets per timestamps, rounded up to the nearest integer.
		 * IOW: ts_batch = ceil(total_packets_since_ts / pkts_per_ts), total_packets_since_ts includes this batch.
		 */
		ts_batch = ((stream->subtype_data.aaf.tx_count - stream->frame_with_ts) + rc + AAF_PACKETS_PER_TIMESTAMP_SPARSE - 1) / AAF_PACKETS_PER_TIMESTAMP_SPARSE;
		if (ts_batch > NET_TX_BATCH) {
			os_log(LOG_ERR, "stream(%p) Unexpected number of timestamps needed(%u), clamping down to %u\n", stream, ts_batch, NET_TX_BATCH);
			ts_batch = NET_TX_BATCH;
		}
	} else
		ts_batch = rc;

	stream->consumer.gptp_current = stream->gptp_current;
	ts_n = media_clock_gen_get_ts(&stream->consumer, ts, ts_batch, &flags, alignment_ts);
	if (unlikely(flags & MCG_FLAGS_DO_ALIGN)) {
		/* skip time per packet update, if there is a timestamp discontinuity */
		stream->ts_n = 0;
//		os_log(LOG_ERR, "talker(%p) offset %u write %u read %u ts_batch %d ts_n %d\n", stream, stream->consumer.offset, stream->consumer.grid->write_index, stream->consumer.read_index, ts_batch, ts_n);
	}

	if (ts_n != ts_batch) {
		stream->stats.clock_err++;
		goto media_clock_fail;
	}

	if (ts_n) {
		u32 delay = ts[0] - stream->gptp_current;

		if ( (delay < stream->max_transit_time) ||
			(delay > (stream_presentation_offset(stream->max_transit_time, stream->latency) + stream->latency))) {
			stream->stats.clock_invalid++;
//			goto media_clock_fail;
		}
	}

	if (flags & MCG_FLAGS_RESET)
		avtp_data_header_toggle_mcr(stream->avtp_hdr);

	stream->stats.clock_rx += ts_n;

	n_now = rc;

	ts_n = 0;
	i = 0;
	while (i < n_now) {
		media_desc = media_desc_array[i];
		net_desc = &media_desc->net;

		partial = net_desc->flags & NET_TX_FLAGS_PARTIAL;
		net_desc->l2_offset -= stream->header_len;
		net_desc->len += stream->header_len;
		net_desc->flags = NET_TX_FLAGS_TS;

		buf = NET_DATA_START(net_desc);

		if (unlikely(sparse)) {
			if (stream->subtype_data.aaf.tx_count == stream->frame_with_ts) {
				stream->ts_launch = ts[ts_n] - stream->max_transit_time;

				/* Update time_per_packet based on actual data rate */
				if (stream->ts_n)
					stream->time_per_packet = (ts[ts_n] - stream->ts_last) / AAF_PACKETS_PER_TIMESTAMP_SPARSE;
			} else
				stream->ts_launch += stream->time_per_packet;

			stream->media_count += stream->frames_per_packet;
			stream->subtype_data.aaf.tx_count++;

			get_ts = ((int)stream->subtype_data.aaf.tx_count - (int)stream->frame_with_ts) > 0 ? 1 : 0;

			if (get_ts) {

				avtp_data_header_set_timestamp(stream->avtp_hdr, ts[ts_n]);

				stream->ts_last = ts[ts_n];
				stream->ts_n++;
				ts_n++;
				stream->frame_with_ts += AAF_PACKETS_PER_TIMESTAMP_SPARSE;
			} else
				avtp_data_header_set_timestamp_invalid(stream->avtp_hdr);
		} else {
			stream->ts_launch = ts[ts_n] - stream->max_transit_time;

			avtp_data_header_set_timestamp(stream->avtp_hdr, ts[ts_n]);

			stream->ts_n++;
			ts_n++;

			stream->media_count += stream->frames_per_packet;
		}

		net_desc->ts = stream->ts_launch;

		os_memcpy(buf, stream->header_template, stream->header_len);

		stream->avtp_hdr->sequence_num++;

		if (unlikely(partial)) {
			net_free_multi((void **)&media_desc_array[i], n_now - i);
			stream->stats.tx_err += n_now - i;
			stream->stats.partial++;
			break;
		}

		i++;
	}

	if (stream_net_tx(stream, media_desc_array, i))
		goto transmit_fail;

	return;

media_clock_fail:
	net_free_multi((void **)media_desc_array, rc);

media_rx_fail:
	/* Reset the avtp stream since it was reset by the media stack */
	stream->media_count = 0;
	stream->ts_n = 0;
	stream->subtype_data.aaf.tx_count = 0;
	stream->frame_with_ts = 0;

	return;

transmit_fail:
	return;

}

static void aaf_pcm_prepare_header(struct avtp_aaf_pcm_hdr *hdr, const struct avdecc_format *format)
{
	const struct avdecc_format_aaf_t *aaf_fmt = &format->u.s.subtype_u.aaf;
	const struct avdecc_format_aaf_pcm_t *pcm_fmt = &aaf_fmt->format_u.pcm;

	hdr->nsr = aaf_fmt->nsr;
	AAF_PCM_CHANNELS_PER_FRAME_SET(hdr, AVDECC_FMT_AAF_PCM_CHANNELS_PER_FRAME(format));
	hdr->bit_depth = pcm_fmt->bit_depth;
}

static void aaf_aes3_prepare_header(struct avtp_aaf_aes3_hdr *hdr, const struct avdecc_format *format)
{
	const struct avdecc_format_aaf_t *aaf_fmt = &format->u.s.subtype_u.aaf;
	const struct avdecc_format_aaf_aes3_t *aes3_fmt = &aaf_fmt->format_u.aes3;

	hdr->nfr = aaf_fmt->nsr;
	AAF_AES3_STREAMS_PER_FRAME_SET(hdr, AVDECC_FMT_AAF_AES3_STREAMS_PER_FRAME(format));
	hdr->aes3_dt_ref = aes3_fmt->aes3_dt_ref;
	AAF_AES3_DATA_TYPE_SET(hdr, aes3_fmt->aes3_data_type);
}

static unsigned int aaf_prepare_header(struct avtp_aaf_hdr *hdr, const struct avdecc_format *format, void *stream_id, unsigned int sparse)
{
	const struct avdecc_format_aaf_t *aaf_fmt = &format->u.s.subtype_u.aaf;

	/* AVTP stream common fields */
	hdr->subtype = AVTP_SUBTYPE_AAF;
	hdr->version = AVTP_VERSION_0;
	hdr->sv = 1;

	copy_64(&hdr->stream_id, stream_id);

	/* AAF fields */
	hdr->format = aaf_fmt->format;

	if (sparse)
		hdr->sp = AAF_SP_SPARSE;
	else
		hdr->sp = AAF_SP_NORMAL;

	hdr->evt = 0;

	if (avdecc_format_is_aaf_pcm(format))
		aaf_pcm_prepare_header((struct avtp_aaf_pcm_hdr *)hdr, format);
	else
		aaf_aes3_prepare_header((struct avtp_aaf_aes3_hdr *)hdr, format);

	return sizeof(struct avtp_aaf_hdr);
}

static void aaf_net_rx(struct stream_listener *stream, struct avtp_rx_desc **desc, unsigned int n)
{
	struct avtp_aaf_hdr *aaf_hdr;
	struct media_desc *media_desc[NET_RX_BATCH];
	unsigned int media_n = 0;
	unsigned int stats = 0;
	u32 *hdr;
	int i;

	os_log(LOG_DEBUG, "enter stream(%p)\n", stream);

	for (i = 0; i < n; i++) {

		aaf_hdr = (struct avtp_aaf_hdr *)((char *)desc[i] + desc[i]->desc.l3_offset);

		hdr = (u32 *)&aaf_hdr->format;

		if (unlikely(((hdr[0] & stream->subtype_data.aaf.hdr_mask[0]) != stream->subtype_data.aaf.hdr[0]) ||
			     ((hdr[1] & stream->subtype_data.aaf.hdr_mask[1]) != stream->subtype_data.aaf.hdr[1]))) {
			stream->stats.format_err++;

			net_rx_free(&desc[i]->desc);

			/* FIXME we should indicate packets were lost */
			continue;
		}

		/* FIXME The code below works but is very sensitive to operation order, because the source and destination structs
		 * actually point to the same memory area. Need to make it safer to avoid overwriting a valid field by mistake.
		 */
		media_desc[media_n] = (struct media_desc *)desc[i];
		media_desc[media_n]->l2_offset = desc[i]->l4_offset;
		media_desc[media_n]->len = desc[i]->l4_len;
		media_desc[media_n]->bytes_lost = 0; // FIXME compute value based on dbc
		media_desc[media_n]->flags = desc[i]->flags & (AVTP_MEDIA_CLOCK_RESTART | AVTP_PACKET_LOST | AVTP_TIMESTAMP_UNCERTAIN);
		media_desc[media_n]->ts = desc[i]->desc.ts; // This is actually a copy onto itself. Kept for clarity...

		if (desc[i]->flags & AVTP_TIMESTAMP_INVALID) {
			if (media_desc[media_n]->flags) {
				media_desc[media_n]->avtp_ts[0].offset = 0;
				media_desc[media_n]->avtp_ts[0].flags = AVTP_FLAGS_TO_MEDIA_DESC(AVTP_TIMESTAMP_INVALID);
				media_desc[media_n]->n_ts = 1;
			} else
				media_desc[media_n]->n_ts = 0;
		} else {
			media_desc[media_n]->avtp_ts[0].offset = 0;
			media_desc[media_n]->avtp_ts[0].flags = 0;
			media_desc[media_n]->avtp_ts[0].val = desc[i]->avtp_timestamp;
			media_desc[media_n]->n_ts = 1;

			if (!stats) {
				stats = 1;
				avtp_latency_stats(stream, desc[i]);
			}
		}

		media_n++;
	}

	stream_media_tx(stream, media_desc, media_n);
}

static int aaf_pcm_check_format(const struct avdecc_format *format)
{
	const struct avdecc_format_aaf_pcm_t *pcm_fmt = &format->u.s.subtype_u.aaf.format_u.pcm;
	int rc = GENAVB_SUCCESS;

	/* IEEE1722-2016 7.3.4 */
	switch (format->u.s.subtype_u.aaf.format) {
	case AAF_FORMAT_USER:
		break;

	case AAF_FORMAT_FLOAT_32BIT:
		if (pcm_fmt->bit_depth != 32) {
			rc = -GENAVB_ERR_STREAM_PARAMS;
			goto err_format;
		}
		break;

	case AAF_FORMAT_INT_32BIT:
	case AAF_FORMAT_INT_24BIT:
	case AAF_FORMAT_INT_16BIT:
		if ((pcm_fmt->bit_depth <= 0) || (pcm_fmt->bit_depth > avdecc_fmt_bits_per_sample(format))) {
			rc = -GENAVB_ERR_STREAM_PARAMS;
			goto err_format;
		}
		break;

	default:
		rc = -GENAVB_ERR_STREAM_PARAMS;
		goto err_format;
		break;
	}


	/* 1722rev1-2016 8.3.2 */
	if ((AVDECC_FMT_AAF_PCM_CHANNELS_PER_FRAME(format) <= 0) || (AVDECC_FMT_AAF_PCM_CHANNELS_PER_FRAME(format) > CFG_AVTP_AAF_PCM_MAX_CHANNELS)) {
		rc = -GENAVB_ERR_STREAM_PARAMS;
		goto err_format;
	}

	if ((AVDECC_FMT_AAF_PCM_SAMPLES_PER_FRAME(format) <= 0) || (AVDECC_FMT_AAF_PCM_SAMPLES_PER_FRAME(format) > CFG_AVTP_AAF_PCM_MAX_SAMPLES)) {
		rc = -GENAVB_ERR_STREAM_PARAMS;
		goto err_format;
	}

	if (AVDECC_FMT_AAF_PCM_SAMPLES_PER_FRAME(format) * avdecc_fmt_sample_size(format) > AVTP_DATA_MTU - avdecc_fmt_hdr_size(format)) {
		rc = -GENAVB_ERR_STREAM_PARAMS;
		goto err_format;
	}

err_format:
	return rc;
}

static int aaf_aes3_check_format(const struct avdecc_format *format,  unsigned int is_talker)
{
	const struct avdecc_format_aaf_aes3_t *aes3_fmt = &format->u.s.subtype_u.aaf.format_u.aes3;
	int rc = GENAVB_SUCCESS;

	/* IEEE1722-2016 7.4.4 */
	switch (aes3_fmt->aes3_dt_ref) {
	case AAF_AES3_DT_UNSPECIFIED:
	case AAF_AES3_DT_PCM:
		if (is_talker && aes3_fmt->aes3_data_type) { /* IEEE1722-2016 7.4.5 Table 16 */
			rc = -GENAVB_ERR_STREAM_PARAMS;
			goto err_format;
		}
		break;

	case AAF_AES3_DT_SMPTE338:
		if (aes3_fmt->aes3_data_type > 0x1f) { /* IEEE1722-2016 7.4.4.2 */
			rc = -GENAVB_ERR_STREAM_PARAMS;
			goto err_format;
		}
		break;

	case AAF_AES3_DT_IEC61937:
		if (aes3_fmt->aes3_data_type > 0x7f) { /* IEEE1722-2016 7.4.4.2 */
			rc = -GENAVB_ERR_STREAM_PARAMS;
			goto err_format;
		}
		break;

	case AAF_AES3_DT_VENDOR:
		break;

	default:
		rc = -GENAVB_ERR_STREAM_PARAMS;
		goto err_format;
		break;
	}

	/* 1722rev1-2016 8.4.2 */
	if ((AVDECC_FMT_AAF_AES3_STREAMS_PER_FRAME(format) <= 0) || (AVDECC_FMT_AAF_AES3_STREAMS_PER_FRAME(format) > CFG_AVTP_AAF_AES3_MAX_STREAMS)) {
		rc = -GENAVB_ERR_STREAM_PARAMS;
		goto err_format;
	}

	/* 1722rev1-2016 I.2.1.3.2 */
	/* frames_per_frame is an uint8_t < CFG_AVTP_AAF_AES3_MAX_FRAMES (256) */
	if ((aes3_fmt->frames_per_frame <= 0)/* || (aes3_fmt->frames_per_frame > CFG_AVTP_AAF_AES3_MAX_FRAMES)*/) {
		rc = -GENAVB_ERR_STREAM_PARAMS;
		goto err_format;
	}

	if (aes3_fmt->frames_per_frame * avdecc_fmt_sample_size(format) > AVTP_DATA_MTU - avdecc_fmt_hdr_size(format)) {
		rc = -GENAVB_ERR_STREAM_PARAMS;
		goto err_format;
	}

err_format:
	return rc;
}

static int aaf_check_format(const struct avdecc_format *format, unsigned int is_talker)
{
	const struct avdecc_format_aaf_t *aaf_fmt = &format->u.s.subtype_u.aaf;
	int rc;

	/* 1722rev1-2016 Table 11 and 15 */
	switch (aaf_fmt->nsr) {
	default:
	case AAF_NSR_USER_SPECIFIED:
		rc = -GENAVB_ERR_STREAM_PARAMS;
		goto err_format;
		break;

	case AAF_NSR_8000:
	case AAF_NSR_16000:
	case AAF_NSR_32000:
	case AAF_NSR_44100:
	case AAF_NSR_48000:
	case AAF_NSR_88200:
	case AAF_NSR_96000:
	case AAF_NSR_176400:
	case AAF_NSR_192000:
	case AAF_NSR_24000:
		break;
	}

	if (avdecc_format_is_aaf_pcm(format))
		rc = aaf_pcm_check_format(format);
	else if (avdecc_format_is_aaf_aes3(format))
		rc = aaf_aes3_check_format(format, is_talker);
	else
		rc = -GENAVB_ERR_STREAM_PARAMS;

err_format:
	return rc;
}

static int listener_stream_aaf_init(struct stream_listener *stream)
{
	struct avdecc_format const *format = &stream->format;
	const struct avdecc_format_aaf_t *aaf_fmt = &format->u.s.subtype_u.aaf;
	struct avtp_aaf_hdr aaf_hdr, aaf_hdr_mask;
	u32 *hdr, *hdr_mask;

	os_memset(&aaf_hdr, 0, sizeof(aaf_hdr));
	os_memset(&aaf_hdr_mask, 0, sizeof(aaf_hdr_mask));

	/* Setup receive header pattern match */
	aaf_hdr.format = aaf_fmt->format;
	aaf_hdr_mask.format = 0xff;

	if (avdecc_format_is_aaf_pcm(format)) {
		const struct avdecc_format_aaf_pcm_t *pcm_fmt = &aaf_fmt->format_u.pcm;
		struct avtp_aaf_pcm_hdr *pcm_hdr = (struct avtp_aaf_pcm_hdr *)&aaf_hdr;
		struct avtp_aaf_pcm_hdr *pcm_hdr_mask = (struct avtp_aaf_pcm_hdr *)&aaf_hdr_mask;

		pcm_hdr->nsr = aaf_fmt->nsr;
		pcm_hdr_mask->nsr = 0xf;

		AAF_PCM_CHANNELS_PER_FRAME_SET(pcm_hdr, AVDECC_FMT_AAF_PCM_CHANNELS_PER_FRAME(format));
		AAF_PCM_CHANNELS_PER_FRAME_SET(pcm_hdr_mask, 0x3ff);

		pcm_hdr->bit_depth = pcm_fmt->bit_depth;
		pcm_hdr_mask->bit_depth = 0xff;
	} else {
		const struct avdecc_format_aaf_aes3_t *aes3_fmt = &aaf_fmt->format_u.aes3;
		struct avtp_aaf_aes3_hdr *aes3_hdr = (struct avtp_aaf_aes3_hdr *)&aaf_hdr;
		struct avtp_aaf_aes3_hdr *aes3_hdr_mask = (struct avtp_aaf_aes3_hdr *)&aaf_hdr_mask;

		aes3_hdr->nfr = aaf_fmt->nsr;
		aes3_hdr_mask->nfr = 0xf;

		AAF_AES3_STREAMS_PER_FRAME_SET(aes3_hdr, AVDECC_FMT_AAF_AES3_STREAMS_PER_FRAME(format));
		AAF_AES3_STREAMS_PER_FRAME_SET(aes3_hdr_mask, 0x3ff);

		aes3_hdr->aes3_dt_ref = aes3_fmt->aes3_dt_ref;
		aes3_hdr_mask->aes3_dt_ref = 0x7;

		AAF_AES3_DATA_TYPE_SET(aes3_hdr, aes3_fmt->aes3_data_type);
		switch (aes3_fmt->aes3_dt_ref) {
			case AAF_AES3_DT_UNSPECIFIED:
			case AAF_AES3_DT_PCM:
				AAF_AES3_DATA_TYPE_SET(aes3_hdr_mask, 0x0);
				break;
			default:
				AAF_AES3_DATA_TYPE_SET(aes3_hdr_mask, 0xffff);
				break;
		}

	}

	hdr = (u32 *)&aaf_hdr.format;
	hdr_mask = (u32 *)&aaf_hdr_mask.format;

	stream->subtype_data.aaf.hdr[0] = hdr[0];
	stream->subtype_data.aaf.hdr[1] = hdr[1];

	stream->subtype_data.aaf.hdr_mask[0] = hdr_mask[0];
	stream->subtype_data.aaf.hdr_mask[1] = hdr_mask[1];

	stream->net_rx = aaf_net_rx;

	return 0;
}

int listener_stream_aaf_check(struct stream_listener *stream, struct avdecc_format const *format, u16 flags)
{
	int rc;

	rc = aaf_check_format(format, 0);
	if (rc < 0)
		goto err;

	stream->init = listener_stream_aaf_init;

err:
	return rc;
}

static void talker_stream_aaf_init(struct stream_talker *stream, unsigned int *hdr_len)
{
	struct avdecc_format const *format = &stream->format;

	stream->subtype_data.aaf.sparse = 0;

	*hdr_len = aaf_prepare_header((struct avtp_aaf_hdr *)stream->avtp_hdr, format, &stream->id, stream->subtype_data.aaf.sparse);

	avtp_data_header_set_len(stream->avtp_hdr, stream->payload_size);

	if (stream->subtype_data.aaf.sparse)
		stream->subtype_data.aaf.frames_per_timestamp = stream->frames_per_packet * AAF_PACKETS_PER_TIMESTAMP_SPARSE;
	else
		stream->subtype_data.aaf.frames_per_timestamp = stream->frames_per_packet * AAF_PACKETS_PER_TIMESTAMP_NORMAL;

	stream->common.flags |= STREAM_FLAG_CLOCK_GENERATION;

	stream->net_tx = aaf_net_tx;
}

int talker_stream_aaf_check(struct stream_talker *stream, struct avdecc_format const *format, struct ipc_avtp_connect *ipc)
{
	int rc;

	rc = aaf_check_format(format, 1);
	if (rc < 0)
		goto err;

	stream->init = talker_stream_aaf_init;

err:
	return rc;
}

#endif /* CFG_AVTP_1722A */
