/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief IEC 61883/IIDC protocol handling functions
 @details
*/


#include "os/stdlib.h"
#include "os/clock.h"

#include "common/61883_iidc.h"
#include "common/log.h"
#include "common/net.h"
#include "common/ipc.h"
#include "common/avdecc.h"

#include "61883_iidc.h"
#include "stream.h"
#include "avtp.h"
#include "media_clock.h"

static unsigned int avtp_61883_4_prepare_header(struct avtp_data_hdr *hdr, const struct avdecc_format *format);
static unsigned int avtp_61883_6_prepare_header(struct avtp_data_hdr *hdr, const struct avdecc_format *format);

void avtp_61883_2_net_rx(struct stream_listener *stream, struct avtp_rx_desc **desc, unsigned int n)
{

}

void avtp_61883_3_net_rx(struct stream_listener *stream, struct avtp_rx_desc **desc, unsigned int n)
{

}


/* P1722_D14 - chapter I.2.1.1.2.2 */
static int stream_61883_6_check_format(u64 const *stream_id, struct avdecc_format_iec61883_6_t const *iec61883_6)
{
	int rc = GENAVB_SUCCESS;
	unsigned int dbs_check = 0;

	switch (iec61883_6->fdf_u.fdf.evt) {
	case IEC_61883_6_FDF_EVT_AM824:
		/* for now, support only mbla. All others cnt should be 0 */
		if (iec61883_6->label_iec_60958_cnt || iec61883_6->label_midi_cnt || iec61883_6->label_smptecnt)
			goto err_format;

		/* data block size check - P1722_D14 - chapter I.2.1.1.2.2.3 */
		dbs_check = iec61883_6->label_iec_60958_cnt + iec61883_6->label_mbla_cnt
				+ iec61883_6->label_midi_cnt + iec61883_6->label_smptecnt;

		if (iec61883_6->dbs != dbs_check) {
			os_log(LOG_ERR, "stream_id(%016"PRIx64") IEC61883_6 invalid DBS value, %u. Should be %u.\n",
					ntohll(*stream_id), iec61883_6->dbs, dbs_check);
			goto err_format;
		}
		/* intentional fall through */

	case IEC_61883_6_FDF_EVT_FLOATING:
	case IEC_61883_6_FDF_EVT_INT32:
		/* Data block size in quadlets (max 256). Should be > 0, and not exceed the allowed maximum */
		if ((!iec61883_6->dbs) || (iec61883_6->dbs > CFG_AVTP_61883_6_MAX_CHANNELS)) {
			os_log(LOG_ERR, "stream_id(%016"PRIx64") IEC61883_6 invalid DBS value (%u).\n",
					ntohll(*stream_id), iec61883_6->dbs);

			goto err_format;
		}
		break;

	case IEC_61883_6_FDF_EVT_PACKED:
	default:
		os_log(LOG_ERR, "stream_id(%016"PRIx64") IEC61883_6 invalid evt value (%u)\n",
				ntohll(*stream_id), iec61883_6->fdf_u.fdf.evt);
		goto err_format;
	}

	switch (iec61883_6->fdf_u.fdf.sfc) {
	case IEC_61883_6_FDF_SFC_32000:
	case IEC_61883_6_FDF_SFC_44100:
	case IEC_61883_6_FDF_SFC_48000:
	case IEC_61883_6_FDF_SFC_88200:
	case IEC_61883_6_FDF_SFC_96000:
	case IEC_61883_6_FDF_SFC_176400:
	case IEC_61883_6_FDF_SFC_192000:
		break;

	case IEC_61883_6_FDF_SFC_RSVD:
	default:
		os_log(LOG_ERR, "stream_id(%016"PRIx64") IEC61883_6 invalid sfc value (%u)\n",
				ntohll(*stream_id), iec61883_6->fdf_u.fdf.sfc);
		goto err_format;
	}


	/* b = blocking mode support, nb = non-blocking mode support - support only b=0 nb=1 */
	if ((iec61883_6->b == 1) || (iec61883_6->nb == 0)) {
		os_log(LOG_ERR, "stream_id(%016"PRIx64") IEC61883_6 incompatible b/nb (%u/%u) values\n",
				ntohll(*stream_id), iec61883_6->b, iec61883_6->nb);
		goto err_format;
	}

//TODO check new ut and sc fields are equal to 0?
// #ifdef CFG_AVTP_1722A
// 	if ((iec61883_6->ut != 0) || (iec61883_6->sc != 0)) {
//		os_log(LOG_ERR, "stream_id(%016"PRIx64") invalid ut/sc (%u/%u) values. Should be 0/0\n",
//				ntohll(stream->id), iec61883_6->ut, iec61883_6->sc);
// 		goto err_format;
// 	}
// #endif

	return rc;

err_format:
	return -GENAVB_ERR_STREAM_PARAMS;
}

/** Calculate a SYT interval for 61883-6 streams
 *
 * Calculates a SYT interval such that:
 *  syt_interval = 2^(n + SYT_INTERVAL_LN2) >= frames per packet
 *
 * \return Log2 of the calculated syt_interval
 * \param format stream format
 * \param class stream SR class
 */
static unsigned int avtp_61883_6_syt_interval_ln2(const struct avdecc_format *format, sr_class_t class)
{
	unsigned int frames_per_packet = avdecc_fmt_samples_per_packet(format, class);
	unsigned int syt_interval_ln2 = SYT_INTERVAL_LN2;

	while ((1 << syt_interval_ln2) < frames_per_packet)
		syt_interval_ln2++;

	return syt_interval_ln2;
}

static int listener_stream_61883_iidc_init(struct stream_listener *stream)
{
	struct avdecc_format const *format = &stream->format;

	switch (format->u.s.subtype_u.iec61883.fmt) {
	case IEC_61883_CIP_FMT_4:
		stream->net_rx = avtp_61883_4_net_rx;
		break;

	case IEC_61883_CIP_FMT_6:
		stream->net_rx = avtp_61883_6_net_rx;
		stream->subtype_data.iec61883_6.syt_interval_ln2 = avtp_61883_6_syt_interval_ln2(format, stream->class);
		break;

	default:
		break;
	}

	return 0;
}


/* P1722_D14 chapter I.2.1.1 */
int listener_stream_61883_iidc_check(struct stream_listener *stream, struct avdecc_format const *format, u16 flags)
{
	int rc = GENAVB_SUCCESS;

	/* checking stream format IIDC or IEC 61883-X */
	switch (format->u.s.subtype_u.iec61883.sf) {
	case IEC_61883_SF_61883:
		break;

	case IEC_61883_SF_IIDC:
	default:
		os_log(LOG_ERR, "stream_id(%016"PRIx64") Stream format (%u) not supported\n",
				ntohll(stream->id), format->u.s.subtype_u.iec61883.sf);
		rc = -GENAVB_ERR_STREAM_PARAMS;
		goto exit;
	}


	switch (format->u.s.subtype_u.iec61883.fmt) {
	case IEC_61883_CIP_FMT_4:
		if (flags & IPC_AVTP_FLAGS_MCR) {
			os_log(LOG_ERR, "stream_id(%016"PRIx64") Media clock recovery not supported\n", ntohll(stream->id));
			rc = -GENAVB_ERR_STREAM_PARAMS;
			goto exit;
		}

		/* no specific checks on 61883-4 - P1722_D14 chap I.2.1.1.2.1 */
		break;

	case IEC_61883_CIP_FMT_6:
		rc = stream_61883_6_check_format(&stream->id, &format->u.s.subtype_u.iec61883.format_u.iec61883_6);
		if (rc < 0)
			goto exit;

		break;

	default:
		os_log(LOG_ERR, "stream_id(%016"PRIx64") 61883 format (%u) not supported\n",
				ntohll(stream->id), format->u.s.subtype_u.iec61883.fmt);

		rc = -GENAVB_ERR_STREAM_PARAMS;
		break;
	}

	stream->init = listener_stream_61883_iidc_init;

exit:
	return rc;
}

static void talker_stream_61883_iidc_init(struct stream_talker *stream, unsigned int *hdr_len)
{
	struct avdecc_format const *format = &stream->format;

	stream->subtype_data.iec61883.iec_hdr = (struct iec_61883_hdr *)(stream->avtp_hdr + 1);

	switch (format->u.s.subtype_u.iec61883.fmt) {
	case IEC_61883_CIP_FMT_4:
		/* There is no real syt_interval in this case, but the ratio timestamp/sample is still a valid concept */
		stream->subtype_data.iec61883.syt_interval_ln2 = 0;
		stream->net_tx = avtp_61883_4_net_tx;
		*hdr_len = avtp_61883_4_prepare_header(stream->avtp_hdr, format);
		break;

	case IEC_61883_CIP_FMT_6:
		stream->subtype_data.iec61883.syt_interval_ln2 = avtp_61883_6_syt_interval_ln2(format, stream->class);
		stream->net_tx = avtp_61883_6_net_tx;
		*hdr_len = avtp_61883_6_prepare_header(stream->avtp_hdr, format);
                break;

	default:
		break;
	}

	avtp_data_header_set_len(stream->avtp_hdr, stream->payload_size + *hdr_len);

	*hdr_len += avtp_data_header_init(stream->avtp_hdr, AVTP_SUBTYPE_61883_IIDC, &stream->id);

	stream->common.flags |= STREAM_FLAG_CLOCK_GENERATION;
}


/* Check stream format and initialize parameters - P1722_D14 chapter I.2.1.1 */
int talker_stream_61883_iidc_check(struct stream_talker *stream, struct avdecc_format const *format,
					struct ipc_avtp_connect *ipc)
{
	int rc = GENAVB_SUCCESS;

	/* checking stream format IIDC or IEC 61883-X */
	switch (format->u.s.subtype_u.iec61883.sf) {
	case IEC_61883_SF_61883:
		break;

	case IEC_61883_SF_IIDC:
	default:
		os_log(LOG_ERR, "stream_id(%016"PRIx64") Stream format (%u) not supported\n",
				ntohll(stream->id), format->u.s.subtype_u.iec61883.sf);

		goto err_format;
	}


	switch (format->u.s.subtype_u.iec61883.fmt) {
	case IEC_61883_CIP_FMT_4:
		/* no specific checks on 61883-4 - P1722_D14 chap I.2.1.1.2.1 */
		break;

	case IEC_61883_CIP_FMT_6:
		rc = stream_61883_6_check_format(&stream->id, &format->u.s.subtype_u.iec61883.format_u.iec61883_6);
		break;

	default:
		goto err_format;
	}

	stream->init = talker_stream_61883_iidc_init;

	return rc;

err_format:
	return -GENAVB_ERR_STREAM_PARAMS;
}


/** Handles reception of 61883-4 avtp packets
 *
 * Does protocol validation and decapsulation, converts avtp descriptors to media descriptors and transmits
 * data to media stack.
 * The function takes ownership of the received descriptors and is responsible for freeing them.
 *
 * \return none
 * \param stream pointer to listener stream context
 * \param desc array of avtp receive descriptors
 * \param n array length
 */
void avtp_61883_4_net_rx(struct stream_listener *stream, struct avtp_rx_desc **desc, unsigned int n)
{
	struct iec_61883_hdr *iec_hdr;
	struct iec_61883_iidc_specific_hdr spec_hdr;
	struct media_desc *media_desc[NET_RX_BATCH];
	unsigned int payload_size, media_n = 0, stats = 0;
	int i, j, offset, data_offset;
	u32 *current_ts;

	os_log(LOG_DEBUG, "enter stream(%p)\n", stream);

	for (i = 0; i < n; i++) {
		iec_hdr = (struct iec_61883_hdr *)((char *)desc[i] + desc[i]->l4_offset);

		spec_hdr.u.raw = desc[i]->protocol_specific_header;
		if (unlikely((spec_hdr.u.s.tag != IEC_61883_PSH_TAG_CIP_INCLUDED)
				|| (iec_hdr->sph != IEC_61883_CIP_SPH_SOURCE_PACKETS)
				|| (iec_hdr->qpc != 0)
				|| (iec_hdr->fmt != IEC_61883_CIP_FMT_4))) {
			stream->stats.format_err++;

			net_rx_free(&desc[i]->desc);

			/* FIXME we should indicate packets were lost */
			continue; /* Should never happen if stream _is_ 61883-4 */
		}

		if (unlikely(IEC_61883_4_CIP_DBS != iec_hdr->dbs) || unlikely(IEC_61883_4_CIP_FN != iec_hdr->fn)) {
			stream->stats.format_err++;
			os_log(LOG_ERR,"61883-4 frame stride mismatch: expected DBS = %d, FN = %d but in-stream DBS = %d, FN = %d\n", IEC_61883_4_CIP_DBS, IEC_61883_4_CIP_FN, iec_hdr->dbs, iec_hdr->fn);
		}

		if (!(desc[i]->flags & AVTP_TIMESTAMP_INVALID)) {
			stream->stats.format_err++;
			os_log(LOG_ERR,"61883-4 packet format mismatch: AVTP timestamp valid bit set but shouldn't\n");
		}

		payload_size = desc[i]->l4_len - sizeof(struct iec_61883_hdr);
		if ((payload_size % IEC_61883_4_SP_SIZE) != 0) {
			stream->stats.format_err++;
			os_log(LOG_ERR,"61883-4 payload size invalid: %d instead of %d\n", payload_size, IEC_61883_4_SP_SIZE);
		}

		/* FIXME The code below works but is very sensitive to operation order, because the source and destination structs
		 * actually point to the same memory area. Need to make it safer to avoid overwriting a valid field by mistake.
		 */
		media_desc[media_n] = (struct media_desc *)desc[i];
		media_desc[media_n]->l2_offset = desc[i]->l4_offset + sizeof(struct iec_61883_hdr);
		current_ts = (u32 *)((char *)media_desc[i] + media_desc[i]->l2_offset);
		media_desc[media_n]->len = payload_size;
		media_desc[media_n]->bytes_lost = 0; // FIXME compute value based on dbc
		media_desc[media_n]->flags = desc[i]->flags & (AVTP_MEDIA_CLOCK_RESTART | AVTP_PACKET_LOST | AVTP_TIMESTAMP_UNCERTAIN);
		media_desc[media_n]->ts = desc[i]->desc.ts;  // This is actually a copy onto itself. Kept for clarity...

		desc[i]->avtp_timestamp = ntohl(*current_ts);
		if (!stats) {
			stats = 1;
			avtp_latency_stats(stream, desc[i]);
		}

		j = 0;
		data_offset = 0;
		for (offset = 0; offset < payload_size; offset += IEC_61883_4_SP_SIZE) {
			media_desc[media_n]->avtp_ts[j].offset = data_offset;
			media_desc[media_n]->avtp_ts[j].flags = media_desc[media_n]->flags;
			media_desc[media_n]->avtp_ts[j].val = ntohl(*current_ts);
			current_ts += IEC_61883_4_SP_SIZE / sizeof(u32);
			j++;
			data_offset += IEC_61883_4_SP_PAYLOAD_SIZE;
		}
		media_desc[media_n]->n_ts = j;

		media_n++;
	}

	stream_media_tx(stream, media_desc, media_n);
}

void avtp_61883_5_net_rx(struct stream_listener *stream, struct avtp_rx_desc **desc, unsigned int n)
{

}

/** Handles reception of 61883-6 avtp packets
 *
 * Does protocol validation and decapsulation, converts avtp descriptors to media descriptors and transmits
 * data to media stack.
 * The function takes ownership of the received descriptors and is responsible for freeing them.
 *
 * \return none
 * \param stream pointer to listener stream context
 * \param desc array of avtp receive descriptors
 * \param n array length
 */
void avtp_61883_6_net_rx(struct stream_listener *stream, struct avtp_rx_desc **desc, unsigned int n)
{
	struct iec_61883_hdr *iec_hdr;
	struct iec_61883_iidc_specific_hdr spec_hdr;
	struct avdecc_format_iec61883_6_t *iec61883_6;
	struct media_desc *media_desc[NET_RX_BATCH];
	unsigned int media_n = 0;
	unsigned int stats = 0;
	int i;
	unsigned int syt_interval = (1 << stream->subtype_data.iec61883_6.syt_interval_ln2);
	unsigned int syt_interval_mod = syt_interval - 1;

	os_log(LOG_DEBUG, "enter stream(%p)\n", stream);

	for (i = 0; i < n; i++) {

		iec_hdr = (struct iec_61883_hdr *)((char *)desc[i] + desc[i]->l4_offset);

		spec_hdr.u.raw = desc[i]->protocol_specific_header;
		if (unlikely((spec_hdr.u.s.tag != IEC_61883_PSH_TAG_CIP_INCLUDED)
			|| (iec_hdr->sph == IEC_61883_CIP_SPH_SOURCE_PACKETS)
			|| (iec_hdr->qpc != 0)
			|| (iec_hdr->fmt != IEC_61883_CIP_FMT_6))) {
			stream->stats.format_err++;

			net_rx_free(&desc[i]->desc);

			/* FIXME we should indicate packets were lost */
			continue; // Should never happen if stream _is_ 61883-6
		}

		if (unlikely(iec_hdr->fdf_u.raw == IEC_61883_6_FDF_NODATA)) {
			stream->stats.format_err++;

			net_rx_free(&desc[i]->desc);

			/* FIXME we should indicate packets were lost */
			continue;
		}

		iec61883_6 = &stream->format.u.s.subtype_u.iec61883.format_u.iec61883_6;
		if (unlikely(iec61883_6->dbs != iec_hdr->dbs)) {
			stream->stats.format_err++;
			os_log(LOG_ERR,"Frame stride mismatch: configured = %d but in-stream = %d\n", avdecc_fmt_sample_stride(&stream->format), (iec_hdr->dbs == 0?256:iec_hdr->dbs) << 2);
		}

		/* This should only happen if there is a discontinuity in the dbc */
		if (unlikely((u8)(stream->subtype_data.iec61883_6.syt_count - iec_hdr->dbc) >= syt_interval)) {
			stream->stats.format_err++;

			/* offset = mod(SYT_INTERVAL - mod(dbc, SYT_INTERVAL), SYT_INTERVAL) */
			stream->subtype_data.iec61883_6.syt_count = iec_hdr->dbc + ((syt_interval - (iec_hdr->dbc & syt_interval_mod)) & syt_interval_mod);
		}

		if (unlikely(iec61883_6->fdf_u.fdf.sfc != iec_hdr->fdf_u.fdf.sfc)) {
			stream->stats.format_err++;
			os_log(LOG_ERR,"Sampling rate mismatch: configured = %d but in-stream = %d\n", avdecc_fmt_sample_rate(&stream->format), avtp_61883_6_sampling_freq[iec_hdr->fdf_u.fdf.sfc]);
		}

		if (unlikely(iec61883_6->fdf_u.fdf.evt != iec_hdr->fdf_u.fdf.evt)) {
			stream->stats.format_err++;
			os_log(LOG_ERR,"Sample format mismatch: configured fdf_evt = %d but in-stream = %d\n", iec61883_6->fdf_u.fdf.evt, iec_hdr->fdf_u.fdf.evt);
		}

		switch (iec_hdr->fdf_u.fdf.evt) {
		case IEC_61883_6_FDF_EVT_AM824:
		case IEC_61883_6_FDF_EVT_FLOATING:
		case IEC_61883_6_FDF_EVT_INT32:
			//TODO There may be more than one sample format per frame. For now we only accept AM824 frames with mbla samples

			/* FIXME The code below works but is very sensitive to operation order, because the source and destination structs
			 * actually point to the same memory area. Need to make it safer to avoid overwriting a valid field by mistake.
			 */
			media_desc[media_n] = (struct media_desc *)desc[i];
			media_desc[media_n]->l2_offset = desc[i]->l4_offset + sizeof(struct iec_61883_hdr);
			media_desc[media_n]->len = desc[i]->l4_len - sizeof(struct iec_61883_hdr);
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
				media_desc[media_n]->avtp_ts[0].offset = (u8)(stream->subtype_data.iec61883_6.syt_count - iec_hdr->dbc) * iec_hdr->dbs << 2;
				media_desc[media_n]->avtp_ts[0].flags = 0;
				media_desc[media_n]->avtp_ts[0].val = desc[i]->avtp_timestamp;
				media_desc[media_n]->n_ts = 1;

				stream->subtype_data.iec61883_6.syt_count += syt_interval;


				if (!stats) {
					stats = 1;
					avtp_latency_stats(stream, desc[i]);
				}
			}

			media_n++;

			break;

		case IEC_61883_6_FDF_EVT_PACKED:
			//Not supported according to P1722a/D9 section 6.3.5
		default:
			stream->stats.format_err++;
			net_rx_free(&desc[i]->desc);
			break;
		}
	}

	stream_media_tx(stream, media_desc, media_n);
}

void avtp_61883_7_net_rx(struct stream_listener *stream, struct avtp_rx_desc **desc, unsigned int n)
{

}

void avtp_iidc_net_rx(struct stream_listener *stream, struct avtp_rx_desc **desc, unsigned int n)
{

}


void avtp_61883_2_net_tx(struct stream_talker *stream)
{

}

void avtp_61883_3_net_tx(struct stream_talker *stream)
{

}

static unsigned int avtp_61883_4_prepare_header(struct avtp_data_hdr *hdr, const struct avdecc_format *format)
{
	struct iec_61883_hdr *iec_hdr;
	struct iec_61883_iidc_specific_hdr spec_hdr;

	spec_hdr.u.s.tag = IEC_61883_PSH_TAG_CIP_INCLUDED;
	spec_hdr.u.s.channel = IEC_61883_PSH_CHANNEL_AVB_NETWORK;
	spec_hdr.u.s.tcode = IEC_61883_PSH_TCODE_AVTP;
	spec_hdr.u.s.sy = 0;

	avtp_data_header_set_protocol_specific(hdr, spec_hdr.u.raw);
	avtp_data_header_set_timestamp_invalid(hdr);

	iec_hdr = (struct iec_61883_hdr *)(hdr + 1);
	iec_hdr->rsvd1 = 0;
	iec_hdr->sid = IEC_61883_CIP_SID_AVB_NETWORK;
	iec_hdr->dbs = IEC_61883_4_CIP_DBS;
	iec_hdr->fn = IEC_61883_4_CIP_FN;
	iec_hdr->qpc = 0;
	iec_hdr->sph = IEC_61883_4_CIP_SPH;
	iec_hdr->rsvd2 = 0;
	iec_hdr->dbc = 0;
	iec_hdr->rsvd3 = IEC_61883_CIP_2ND_QUAD_ID;
	iec_hdr->fmt = format->u.s.subtype_u.iec61883.fmt;
	iec_hdr->fdf_u.raw = 0;
	iec_hdr->syt = 0;

	return sizeof(struct iec_61883_hdr);
}

/** Handles transmission of 61883-4 avtp packets
 *
 * Reads data from media stack, converts media descriptors to network descriptors, including protocol encapsulation,
 * and transmit packets. AVTP timestamps are read from media clock generation layer.
 *
 * \return none
 * \param stream pointer to talker stream context
 */
void avtp_61883_4_net_tx(struct stream_talker *stream)
{
	u32 i, j;
	int n_now;
	void *buf;
	struct media_rx_desc *media_desc_array[NET_TX_BATCH], *media_desc;
	struct net_tx_desc *net_desc;
	u32 ts[TS_TX_BATCH];
	struct iec_61883_hdr *iec_hdr = stream->subtype_data.iec61883.iec_hdr;
	struct avtp_data_hdr *data_hdr;
	int rc;
	int ts_n;
	int desc_ts_n;
	unsigned int ts_batch, frames_in_packet;
	unsigned int offset, ts_now;
	unsigned int flags = 0;
	unsigned int partial;
	unsigned int alignment_ts = 0;

	/* Get MPEG2-TS data from media stack */
	if (unlikely((rc = stream_media_rx(stream, media_desc_array, ts, &flags, &alignment_ts)) < 0)) {
		goto media_rx_fail;
	}

	/* The media clock outputs timestamps at a constant (max) rate, but less may be required depending on
	 * instantaneous media bandwidth, so make sure to always fetch max timestamps. */
	ts_batch = stream->tx_batch * stream->frames_per_packet;

	if (ts_batch > TS_TX_BATCH) {
		os_log(LOG_ERR, "stream(%p) Unexpected number of timestamps needed(%u), clamping down to %u\n", stream, ts_batch, TS_TX_BATCH);
		ts_batch = TS_TX_BATCH;
	}

	stream->consumer.gptp_current = stream->gptp_current;
	ts_n = media_clock_gen_get_ts(&stream->consumer, ts, ts_batch, &flags, alignment_ts);

	if (ts_n != ts_batch) {
		stream->stats.clock_err++;
		goto media_clock_fail;
	}

	if (flags & MCG_FLAGS_RESET) {
		avtp_data_header_toggle_mcr(stream->avtp_hdr);
		stream->ts_media_prev = ts[0];
	}

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
		net_desc->flags = 0;

		buf = NET_DATA_START(net_desc);

		os_memcpy(buf, stream->header_template, stream->header_len);
		if (unlikely(partial))
			frames_in_packet = (net_desc->len - stream->header_len) / avdecc_fmt_sample_stride(&stream->format);
		else
			frames_in_packet = stream->frames_per_packet;


		desc_ts_n = 0;
		for (j = 0; j < frames_in_packet; j++) {
			offset = j * IEC_61883_4_SP_SIZE;

			if ((desc_ts_n < media_desc->ts_n) && (media_desc->avtp_ts[desc_ts_n].offset == offset)) {
				ts_now = media_desc->avtp_ts[desc_ts_n].val;
				stream->ts_media_prev = ts_now;
				desc_ts_n++;
			} else
				ts_now = stream->ts_media_prev;

			*(u32 *)((char *)buf + stream->header_len + offset) = htonl(ts_now);

			ts_n++;
		}

		stream->avtp_hdr->sequence_num++;
		i++;

		if (unlikely(partial)) {
			data_hdr = (struct avtp_data_hdr *)((char *)buf + ((char *)stream->avtp_hdr - (char *)stream->header_template));
			data_hdr->stream_data_length = htons(ntohs(data_hdr->stream_data_length) - (stream->frames_per_packet - frames_in_packet)*avdecc_fmt_sample_stride(&stream->format));

			stream->stats.partial++;
			stream->media_count = 0;
			iec_hdr->dbc = 0;

			net_free_multi((void **)&media_desc_array[i], n_now - i);
			stream->stats.tx_err += n_now - i;
			break;
		} else {
			stream->media_count += frames_in_packet;

			/* Always send an integer number of source packets */
			iec_hdr->dbc += frames_in_packet << IEC_61883_4_CIP_FN;
		}
	}

	if (stream_net_tx(stream, media_desc_array, i))
		goto transmit_fail;

	return;

media_clock_fail:
	net_free_multi((void **)media_desc_array, rc);

media_rx_fail:
	/* Reset the avtp stream since it was reset by the media stack */
	stream->media_count = 0;
	iec_hdr->dbc = 0;

	return;

transmit_fail:
	return;

}

void avtp_61883_5_net_tx(struct stream_talker *stream)
{

}

static unsigned int avtp_61883_6_prepare_header(struct avtp_data_hdr *hdr, const struct avdecc_format *format)
{
	struct iec_61883_hdr *iec_hdr;
	struct iec_61883_iidc_specific_hdr spec_hdr;
	const struct avdecc_format_iec61883_6_t *iec61883_6 = &format->u.s.subtype_u.iec61883.format_u.iec61883_6;

	spec_hdr.u.s.tag = IEC_61883_PSH_TAG_CIP_INCLUDED;
	spec_hdr.u.s.channel = IEC_61883_PSH_CHANNEL_AVB_NETWORK;
	spec_hdr.u.s.tcode = IEC_61883_PSH_TCODE_AVTP;
	spec_hdr.u.s.sy = 0;

	avtp_data_header_set_protocol_specific(hdr, spec_hdr.u.raw);

	iec_hdr = (struct iec_61883_hdr *)(hdr + 1);
	iec_hdr->rsvd1 = 0;
	iec_hdr->sid = IEC_61883_CIP_SID_AVB_NETWORK;
	iec_hdr->dbs = iec61883_6->dbs;
	iec_hdr->fn = 0;
	iec_hdr->qpc = 0;
	iec_hdr->sph = 0;
	iec_hdr->rsvd2 = 0;
	iec_hdr->dbc = 0;
	iec_hdr->rsvd3 = IEC_61883_CIP_2ND_QUAD_ID;
	iec_hdr->fmt = format->u.s.subtype_u.iec61883.fmt;
	iec_hdr->fdf_u.raw = iec61883_6->fdf_u.raw;
	iec_hdr->syt = IEC_61883_CIP_SYT_NO_TSTAMP;

	return sizeof(struct iec_61883_hdr);
}


/** Resets AVTP stream timestamp counters.
 *
 * Resets the AVTP 61883-6 timestamp counters so the placement of timestamps inside the packets can start again from a clean state.
 * \return none
 * \param stream	pointer to talker stream context
 * \param iec_hdr	pointer to IEC header context
 */
static inline void avtp_61883_6_stream_ts_reset(struct stream_talker *stream, struct iec_61883_hdr *iec_hdr)
{
	stream->media_count = 0;
	stream->frame_with_ts = 0;
	stream->ts_n = 0;
	iec_hdr->dbc = 0;
}

/** Handles transmission of 61883-6 avtp packets
 *
 * Reads data from media stack, converts media descriptors to network descriptors, including protocol encapsulation,
 * and transmit packets. AVTP timestamps are read from media clock generation layer.
 *
 * \return none
 * \param stream pointer to talker stream context
 */
void avtp_61883_6_net_tx(struct stream_talker *stream)
{
	u32 get_ts, i;
	int n_now;
	void *buf;
	struct media_rx_desc *media_desc_array[NET_TX_BATCH], *media_desc;
	struct net_tx_desc *net_desc;
	u32 ts[NET_TX_BATCH];
	struct iec_61883_hdr *iec_hdr = stream->subtype_data.iec61883.iec_hdr;
	struct avtp_data_hdr *data_hdr;
	unsigned int syt_interval_ln2;
	int rc;
	int ts_n;
	unsigned int ts_batch, frames_in_packet;
	unsigned int flags = 0;
	unsigned int partial;
	unsigned int alignment_ts = 0;

	// Get Audio media from media stack
	if (unlikely((rc = stream_media_rx(stream, media_desc_array, ts, &flags, &alignment_ts)) < 0)) {
		goto media_rx_fail;
	}

	syt_interval_ln2 = stream->subtype_data.iec61883.syt_interval_ln2;
	/* The last frame with a timestamp was (stream->media_count - stream->frame_with_ts) frames ago.
	 * We just received rc * stream->frames_per_packet new frames.
	 * So the number of necessary timestamps is the sum of these 2 values, divided by the number of frames per timestamps, rounded up to the nearest integer.
	 * IOW: ts_batch = ceil(total_frames_since_ts / syt_interval), total_frames_since_ts includes this batch.
	 */
	ts_batch = ((stream->media_count - stream->frame_with_ts) + rc * stream->frames_per_packet + (1 << syt_interval_ln2) - 1) >> syt_interval_ln2;

	if (ts_batch > NET_TX_BATCH) {
		os_log(LOG_ERR, "stream(%p) Unexpected number of timestamps needed(%u), clamping down to %u\n", stream, ts_batch, NET_TX_BATCH);
		ts_batch = NET_TX_BATCH;
	}

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
		if (unlikely(partial))
			frames_in_packet = (net_desc->len - stream->header_len) / avdecc_fmt_sample_stride(&stream->format);
		else
			frames_in_packet = stream->frames_per_packet;

		if (stream->media_count == stream->frame_with_ts) {
			stream->ts_launch = ts[ts_n] - stream->max_transit_time;

			/* Update time_per_packet based on actual data rate */
			if (stream->ts_n)
				stream->time_per_packet = ((ts[ts_n] - stream->ts_last) * stream->frames_per_packet) >> syt_interval_ln2;
		} else
			stream->ts_launch += stream->time_per_packet;

		net_desc->ts = stream->ts_launch;

		stream->media_count += frames_in_packet;

		get_ts = ((int)stream->media_count - (int)stream->frame_with_ts) > 0 ? 1 : 0;
		if (get_ts) {

			avtp_data_header_set_timestamp(stream->avtp_hdr, ts[ts_n]);

			stream->ts_last = ts[ts_n];
			stream->ts_n++;
			ts_n++;
			stream->frame_with_ts += (1 << syt_interval_ln2);
		} else
			avtp_data_header_set_timestamp_invalid(stream->avtp_hdr);

		os_memcpy(buf, stream->header_template, stream->header_len);

		stream->avtp_hdr->sequence_num++;
		i++;

		if (unlikely(partial)) {
			data_hdr = (struct avtp_data_hdr *)((char *)buf + ((char *)stream->avtp_hdr - (char *)stream->header_template));
			data_hdr->stream_data_length = htons(ntohs(data_hdr->stream_data_length) - (stream->frames_per_packet - frames_in_packet)*avdecc_fmt_sample_stride(&stream->format));

			stream->stats.partial++;
			avtp_61883_6_stream_ts_reset(stream, iec_hdr);

			net_free_multi((void **)&media_desc_array[i], n_now - i);
			stream->stats.tx_err += n_now - i;
			break;
		} else
			iec_hdr->dbc += frames_in_packet;
	}

	if (stream_net_tx(stream, media_desc_array, i))
		goto transmit_fail;

	return;

media_clock_fail:
	net_free_multi((void **)media_desc_array, rc);

media_rx_fail:
	/* Reset the avtp stream since it was reset by the media stack */
	avtp_61883_6_stream_ts_reset(stream, iec_hdr);

	return;

transmit_fail:
	return;
}

void avtp_61883_7_net_tx(struct stream_talker *stream)
{

}

void avtp_iidc_net_tx(struct stream_talker *stream)
{

}
