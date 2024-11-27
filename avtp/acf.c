/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2018-2019, 2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVTP Control Format (ACF) handling functions
 @details
*/

#ifdef CFG_AVTP_1722A
#include "common/log.h"

#include "avtp.h"
#include "acf.h"

static unsigned int avtp_tscf_prepare_header(struct stream_talker *stream)
{
	avtp_data_header_init(stream->avtp_hdr, AVTP_SUBTYPE_TSCF, &stream->id);
	avtp_data_header_set_protocol_specific(stream->avtp_hdr, 0);
	avtp_data_header_set_timestamp_invalid(stream->avtp_hdr);

	return sizeof(struct avtp_stream_hdr);
}

static void acf_tscf_net_tx(struct stream_talker *stream)
{
	struct media_rx_desc *media_desc_array[NET_TX_BATCH], *media_desc;
	struct net_tx_desc *net_desc;
	struct avtp_stream_hdr *tscf_hdr = stream->avtp_hdr;
	unsigned int tscf_data_length;
	void *buf;
	unsigned int tx_batch;
	int n_now, i;
	int rc;

	tx_batch = stream->tx_batch;

	/* manage flow control between the network transmit queue and the media interface */
	if (stream_tx_flow_control(stream, &tx_batch) < 0)
		goto transmit_disabled;

	if ((rc = media_rx(&stream->media, media_desc_array, tx_batch)) < 0) {
		stream->stats.media_err++;
//		os_log(LOG_ERR, "talker(%p) media.rx failed %d %d\n", stream, rc, stream->tx_batch);
		goto media_rx_fail;
	}

	stream->stats.media_rx += rc;

	n_now = rc;

	if (rc && media_desc_array[0]->ts_n) {
		u32 delay = media_desc_array[0]->avtp_ts[0].val - stream->gptp_current;

		if ((delay < stream->max_transit_time) ||
			(delay > (stream_presentation_offset(stream->max_transit_time, stream->latency) + stream->latency))) {
			stream->stats.clock_invalid++;
		}
	}

	i = 0;
	while (i < n_now) {
		media_desc = media_desc_array[i];

		net_desc = &media_desc->net;

		tscf_data_length = net_desc->len;

		net_desc->l2_offset -= stream->header_len;
		net_desc->len += stream->header_len;
		net_desc->flags = 0;

		buf = NET_DATA_START(net_desc);

		tscf_hdr->stream_data_length = htons(tscf_data_length);

		if (media_desc->ts_n) {
			net_desc->flags = NET_TX_FLAGS_TS;
			avtp_data_header_set_timestamp(tscf_hdr, media_desc->avtp_ts[0].val);
			stream->ts_launch = media_desc->avtp_ts[0].val - stream->max_transit_time;
			net_desc->ts = stream->ts_launch;
		} else
			avtp_data_header_set_timestamp_invalid(tscf_hdr);

		os_memcpy(buf, stream->header_template, stream->header_len);

		tscf_hdr->sequence_num++;

		i++;

		stream->media_count += tscf_data_length;
	}

	if (stream_net_tx(stream, media_desc_array, i) < 0)
		goto transmit_fail;

	return;

media_rx_fail:
	/* Reset the avtp stream since it was reset by the media stack */
	stream->media_count = 0;

transmit_disabled:
transmit_fail:
	return;
}

static unsigned int avtp_ntscf_prepare_header(struct stream_talker *stream)
{
	struct avtp_ntscf_hdr *hdr = (struct avtp_ntscf_hdr *)stream->avtp_hdr;

	hdr->subtype = AVTP_SUBTYPE_NTSCF;
	hdr->version = AVTP_VERSION_0;

	if (stream->common.flags & STREAM_FLAG_SR)
		hdr->sv = 1;
	else
		hdr->sv = 0;

	hdr->r = 0;

	copy_64(&hdr->stream_id, &stream->id);

	return sizeof(struct avtp_ntscf_hdr);
}

static void acf_ntscf_net_tx(struct stream_talker *stream)
{
	struct media_rx_desc *media_desc_array[NET_TX_BATCH], *media_desc;
	struct net_tx_desc *net_desc;
	struct avtp_ntscf_hdr *ntscf_hdr = (struct avtp_ntscf_hdr *)stream->avtp_hdr;
	unsigned int ntscf_data_length;
	void *buf;
	unsigned int tx_batch;
	int n_now, i;
	int rc;

	tx_batch = stream->tx_batch;

	/* manage flow control between the network transmit queue and the media interface */
	if (stream_tx_flow_control(stream, &tx_batch) < 0)
		goto transmit_disabled;

	if ((rc = media_rx(&stream->media, media_desc_array, tx_batch)) < 0) {
		stream->stats.media_err++;
//		os_log(LOG_ERR, "talker(%p) media.rx failed %d %d\n", stream, rc, stream->tx_batch);
		goto media_rx_fail;
	}

	stream->stats.media_rx += rc;

	n_now = rc;

	i = 0;
	while (i < n_now) {
		media_desc = media_desc_array[i];
		net_desc = &media_desc->net;

		ntscf_data_length = net_desc->len;

		net_desc->l2_offset -= stream->header_len;
		net_desc->len += stream->header_len;
		net_desc->flags = 0;

		buf = NET_DATA_START(net_desc);

		NTSCF_DATA_LENGTH_SET(ntscf_hdr, ntscf_data_length);

		os_memcpy(buf, stream->header_template, stream->header_len);

		ntscf_hdr->sequence_num++;

		i++;

		stream->media_count += ntscf_data_length;
	}

	if (stream_net_tx(stream, media_desc_array, i) < 0)
		goto transmit_fail;

	return;

media_rx_fail:
	/* Reset the avtp stream since it was reset by the media stack */
	stream->media_count = 0;

transmit_disabled:
transmit_fail:
	return;
}

static void acf_tscf_net_rx(struct stream_listener *stream, struct avtp_rx_desc **desc, unsigned int n)
{
	struct media_desc *media_desc[NET_RX_BATCH];
	unsigned int media_n = 0;
	unsigned int payload_size;
	unsigned int stats = 0;
	int i;

	os_log(LOG_DEBUG, "enter stream(%p)\n", stream);

	for (i = 0; i < n; i++) {

		payload_size = desc[i]->l4_len;

		media_desc[media_n] = (struct media_desc *)desc[i];
		media_desc[media_n]->l2_offset = desc[i]->l4_offset;
		media_desc[media_n]->len = payload_size;
		media_desc[media_n]->bytes_lost = 0;
		media_desc[media_n]->flags = desc[i]->flags & (AVTP_MEDIA_CLOCK_RESTART | AVTP_PACKET_LOST | AVTP_TIMESTAMP_UNCERTAIN);
		media_desc[media_n]->ts = desc[i]->desc.ts;  // This is actually a copy onto itself. Kept for clarity...

		if (!(desc[i]->flags & AVTP_TIMESTAMP_INVALID)) {
			media_desc[media_n]->avtp_ts[0].offset = 0;
			media_desc[media_n]->avtp_ts[0].flags = 0;
			media_desc[media_n]->avtp_ts[0].val = desc[i]->avtp_timestamp;
			media_desc[media_n]->n_ts = 1;

			if (!stats) {
				stats = 1;
				avtp_latency_stats(stream, desc[i]);
			}
		} else {
			if (media_desc[media_n]->flags) {
				media_desc[media_n]->avtp_ts[0].offset = 0;
				media_desc[media_n]->avtp_ts[0].flags = AVTP_FLAGS_TO_MEDIA_DESC(AVTP_TIMESTAMP_INVALID);
				media_desc[media_n]->n_ts = 1;
			} else
				media_desc[media_n]->n_ts = 0;
		}

		media_n++;
	}

	stream_media_tx(stream, media_desc, media_n);
}

static void acf_ntscf_net_rx(struct stream_listener *stream, struct avtp_rx_desc **desc, unsigned int n)
{
	struct media_desc *media_desc[NET_RX_BATCH];
	unsigned int media_n = 0;
	unsigned int payload_size, l4_max_len;
	unsigned int stats = 0;
	int i;

	os_log(LOG_DEBUG, "enter stream(%p)\n", stream);

	for (i = 0; i < n; i++) {
		struct avtp_ntscf_hdr *hdr = (struct avtp_ntscf_hdr *)((char *)desc[i] + desc[i]->desc.l3_offset);

		/* drop truncated packets */
		desc[i]->l4_len = NTSCF_DATA_LENGTH(hdr);
		desc[i]->l4_offset = desc[i]->desc.l3_offset + sizeof(struct avtp_ntscf_hdr);

		l4_max_len = desc[i]->desc.len - (desc[i]->l4_offset - desc[i]->desc.l2_offset);

		if (desc[i]->l4_len > l4_max_len) {
			stream->stats.format_err++;
			continue;
		}

		/* Check for lost packets using sequence_num */
		if (likely(stream->stats.media_tx)) {
			if (unlikely(hdr->sequence_num != ((stream->sequence_num + 1) & 0xff))) {

				desc[i]->flags |= AVTP_PACKET_LOST;
				stream->stats.pkt_lost++;
			}
		}

		stream->sequence_num = hdr->sequence_num;

		stream->pkt_received++;

		payload_size = desc[i]->l4_len;

		media_desc[media_n] = (struct media_desc *)desc[i];
		media_desc[media_n]->l2_offset = desc[i]->l4_offset;
		media_desc[media_n]->len = payload_size;
		media_desc[media_n]->bytes_lost = 0;
		media_desc[media_n]->flags = desc[i]->flags & AVTP_PACKET_LOST;
		media_desc[media_n]->ts = desc[i]->desc.ts;  // This is actually a copy onto itself. Kept for clarity...

		if (media_desc[media_n]->flags) {
			media_desc[media_n]->avtp_ts[0].offset = 0;
			media_desc[media_n]->avtp_ts[0].flags = AVTP_FLAGS_TO_MEDIA_DESC(AVTP_TIMESTAMP_INVALID);
			media_desc[media_n]->n_ts = 1;
		} else
			media_desc[media_n]->n_ts = 0;

		if (!stats) {
			stats = 1;
			avtp_latency_stats(stream, desc[i]);
		}

		media_n++;
	}

	stream_media_tx(stream, media_desc, media_n);
}


static int listener_stream_acf_tscf_init(struct stream_listener *stream)
{
	stream->net_rx = acf_tscf_net_rx;

	return 0;
}

int listener_stream_acf_tscf_check(struct stream_listener *stream, struct avdecc_format const *format, u16 flags)
{
	int rc = GENAVB_SUCCESS;

	if (flags & IPC_AVTP_FLAGS_MCR) {
		os_log(LOG_ERR, "stream_id(%016"PRIx64") Media clock recovery not supported\n", ntohll(stream->id));
		rc = -GENAVB_ERR_STREAM_PARAMS;
		goto exit;
	}

	stream->init = listener_stream_acf_tscf_init;

exit:
	return rc;
}

static int listener_acf_ntscf_init(struct stream_listener *stream)
{
	stream->net_rx = acf_ntscf_net_rx;

	return 0;
}

int listener_acf_ntscf_check(struct stream_listener *stream, u16 flags)
{
	int rc = GENAVB_SUCCESS;

	if (flags & IPC_AVTP_FLAGS_MCR) {
		os_log(LOG_ERR, "stream_id(%016"PRIx64") Media clock recovery not supported\n", ntohll(stream->id));
		rc = -GENAVB_ERR_STREAM_PARAMS;
		goto exit;
	}

	stream->init = listener_acf_ntscf_init;
exit:
	return rc;
}

static void talker_stream_acf_tscf_init(struct stream_talker *stream, unsigned int *hdr_len)
{
	*hdr_len = avtp_tscf_prepare_header(stream);

	stream->common.flags |= STREAM_FLAG_MEDIA_WAKEUP;

	stream->net_tx = acf_tscf_net_tx;
}

int talker_stream_acf_tscf_check(struct stream_talker *stream, struct avdecc_format const *format, struct ipc_avtp_connect *ipc)
{
	int rc = GENAVB_SUCCESS;

	if ((!ipc->talker.max_frame_size) || (ipc->talker.max_frame_size > avtp_mtu(AVTP_SUBTYPE_TSCF))) {
		rc = -GENAVB_ERR_INVALID_PARAMS;
		goto err;
	}

	if ((!ipc->talker.max_interval_frames) || (ipc->talker.max_interval_frames > sr_class_max_interval_frames(stream->class))) {
		rc = -GENAVB_ERR_INVALID_PARAMS;
		goto err;
	}

	stream->init = talker_stream_acf_tscf_init;
err:
	return rc;
}

void talker_acf_ntscf_init(struct stream_talker *stream, unsigned int *hdr_len)
{
	*hdr_len = avtp_ntscf_prepare_header(stream);

	stream->common.flags |= STREAM_FLAG_MEDIA_WAKEUP;

	stream->net_tx = acf_ntscf_net_tx;
}

int talker_acf_ntscf_check(struct stream_talker *stream, struct ipc_avtp_connect *ipc)
{
	int rc = GENAVB_SUCCESS;

	if ((!ipc->talker.max_frame_size) || (ipc->talker.max_frame_size > avtp_mtu(AVTP_SUBTYPE_NTSCF))) {
		rc = -GENAVB_ERR_INVALID_PARAMS;
		goto err;
	}

	if ((!ipc->talker.max_interval_frames) || (ipc->talker.max_interval_frames> sr_class_max_interval_frames(stream->class))) {
		rc = -GENAVB_ERR_INVALID_PARAMS;
		goto err;
	}

	stream->init = talker_acf_ntscf_init;
err:
	return rc;
}

#endif /* CFG_AVTP_1722A */
