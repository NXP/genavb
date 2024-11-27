/*
 * Copyright 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVTP compressed video format (CVF) handling functions
 @details
*/

#ifdef CFG_AVTP_1722A

#include "common/log.h"
#include "common/cvf.h"

#include "avtp.h"
#include "cvf.h"

#include "os/string.h"

static unsigned int avtp_cvf_h264_prepare_header(struct avtp_data_hdr *hdr, const struct avdecc_format *format);

static void avtp_cvf_h264_net_rx(struct stream_listener *stream, struct avtp_rx_desc **desc, unsigned int n)
{
	struct avtp_cvf_h264_hdr *h264_avtp_hdr;
	struct media_desc *media_desc[NET_RX_BATCH];
	unsigned int media_n = 0;
	unsigned int stats = 0;
	int i;
	struct cvf_h264_nalu_header *nalu_header;
	static const u32 sync_bytes = 0x00000001;	/* 3 zero bytes sync word */
	struct cvf_h264_nalu_fu_header *nalu_fu_header;
	u8 spec_version = 0;
	u32 h264_timestamp;
	struct cvf_h264_hdr *h264_hdr;
	unsigned int is_single_packet = 0;
	unsigned int is_last_fu;
	unsigned int is_aggregation;
	u16 l2_offset_orig, len_orig;
	unsigned int free_rx_desc = 0;


	spec_version = stream->format.u.s.subtype_u.cvf.format_u.h264.spec_version;

	os_log(LOG_DEBUG, "enter stream(%p) stream_id(%016"PRIx64") spec_version(%u)\n", stream, ntohll(stream->id), (unsigned int)spec_version);

	for (i = 0; i < n; i++) {

		h264_avtp_hdr = (struct avtp_cvf_h264_hdr *)((char *)desc[i] + desc[i]->desc.l3_offset);

		nalu_fu_header = NULL;
		h264_hdr = NULL;
		is_single_packet = 0;
		is_last_fu = 0;
		is_aggregation = 0;
		free_rx_desc = 0;

		/* Save length and offset for error handling */
		l2_offset_orig = desc[i]->desc.l2_offset;
		len_orig = desc[i]->desc.len;

		if (unlikely((h264_avtp_hdr->format != CVF_FORMAT_RFC)
			|| (h264_avtp_hdr->format_subtype != CVF_FORMAT_SUBTYPE_H264))) {
			stream->stats.format_err++;
			os_log(LOG_DEBUG,"stream_id(%016"PRIx64") CVF H264 not recognized: format %u format_subtype %u rsvd %u\n", ntohll(stream->id), h264_avtp_hdr->format, h264_avtp_hdr->format_subtype, h264_avtp_hdr->reserved);

			/* FIXME we should indicate packets were lost */
			free_rx_desc = 1;
			goto next_iteration;
		}

		/* FIXME The code below works but is very sensitive to operation order, because the source and destination structs
		 * actually point to the same memory area. Need to make it safer to avoid overwriting a valid field by mistake.
		 */
		media_desc[media_n] = (struct media_desc *)desc[i];


		if (spec_version == CVF_H264_IEEE_1722_2016) { /*h264 header is defined in the IEEE1722_2016 specification = H264_timestamps */
			media_desc[media_n]->l2_offset = desc[i]->l4_offset + sizeof(struct cvf_h264_hdr);
			media_desc[media_n]->len = desc[i]->l4_len - sizeof(struct cvf_h264_hdr);
		} else { /* No specific H264 header defined */
			media_desc[media_n]->l2_offset = desc[i]->l4_offset;
			media_desc[media_n]->len = desc[i]->l4_len;
		}

		media_desc[media_n]->bytes_lost = 0;

		media_desc[media_n]->flags = desc[i]->flags & (AVTP_MEDIA_CLOCK_RESTART | AVTP_PACKET_LOST | AVTP_TIMESTAMP_UNCERTAIN);
		media_desc[media_n]->ts = desc[i]->desc.ts; // This is actually a copy onto itself. Kept for clarity...

		/*Save the h264_timestamp */
		h264_hdr = (struct cvf_h264_hdr *)((char *)desc[i] + media_desc[media_n]->l2_offset - sizeof(struct cvf_h264_hdr));
		h264_timestamp = ntohl(h264_hdr->h264_timestamp);

		nalu_header = (struct cvf_h264_nalu_header *)((char *)desc[i] + media_desc[media_n]->l2_offset);

		if(nalu_header->f) {
			os_log(LOG_DEBUG, "stream_id(%016"PRIx64") CVF H264 NALU Header Forbidden bit is set\n", ntohll(stream->id));
			stream->stats.format_err++;
			free_rx_desc = 1;
			goto next_iteration;
		}

		/* Parse the NAL Unit header */
		switch (nalu_header->type) {
			case CVF_H264_NALU_TYPE_RESERVED0:
			case CVF_H264_NALU_TYPE_RESERVED17:
			case CVF_H264_NALU_TYPE_RESERVED18:
			case CVF_H264_NALU_TYPE_RESERVED22:
			case CVF_H264_NALU_TYPE_RESERVED23:
			case CVF_H264_NALU_TYPE_RESERVED30:
			case CVF_H264_NALU_TYPE_RESERVED31:
				/* Undefined */
				os_log(LOG_DEBUG, "stream_id(%016"PRIx64") CVF_H264 NALU TYPE %d reserved and not defined\n", ntohll(stream->id), nalu_header->type);
				stream->stats.format_err++;
				free_rx_desc = 1;
				goto next_iteration;
			case CVF_H264_NALU_TYPE_STAP_A:
			{

			/* Support only up to 2 NALU data for now                              */
			/* Simply add sync_bytes between NALU Data replacing STAP-A NAL header */
			/* and NALU size header, keeping only the NALU HDR + NALU Data         */

			/* Create the following packet as example:
			SYNC_BYTES | NALU 1 HDR | NALU 1 DATA | SYNC_BYTES | NALU 2 HDR | NALU 2 DATA */
				unsigned int temp_offset = 0, payload_length, size_offset, num_data = 0;
				u16 nalu_data_size = 0;

				/* Parse the full STAP-A packet to check how many Data payloads are defined */
				payload_length = media_desc[media_n]->len - 1; /* Remove the STAP-A NAL HDR */
				size_offset = 1;

				while (payload_length > 2) {
					nalu_data_size = ntohs(*(u16 *)((char *)desc[i] + media_desc[media_n]->l2_offset + size_offset));
					os_log(LOG_DEBUG,"DEBUG STAP-A DATA%d packet of size %u\n", num_data + 1, nalu_data_size);
					if (nalu_data_size > (payload_length - 2))
						nalu_data_size = payload_length - 2;

					/* Strip NALU size header */
					size_offset += 2;
					payload_length -= 2;

					/* Go to next data packet */
					size_offset += nalu_data_size;
					payload_length -= nalu_data_size;

					num_data++;
				}

				if (num_data > 2) {
					os_log(LOG_DEBUG,"stream_id(%016"PRIx64") CVF H264 STAP-A packet with %u DATA ! more than the 2 maximum supported\n", ntohll(stream->id), num_data);
					stream->stats.format_err++;
					free_rx_desc = 1;
					goto next_iteration;
				}


				/* nalu_size = nalu_hdr size + nalu data payload size */
				nalu_data_size = ntohs(*(u16 *)((char *)desc[i] + media_desc[media_n]->l2_offset + sizeof(struct cvf_h264_nalu_header)));

				/* Add 2 sync bytes and remove the STAP-A NAL HDR and NALU size headers */
				temp_offset = num_data * sizeof(sync_bytes) - num_data * CVF_H264_STAP_A_NALU_SIZE_HDR_SIZE - sizeof(struct cvf_h264_nalu_header);
				media_desc[media_n]->l2_offset -= temp_offset;
				media_desc[media_n]->len += temp_offset;

				/* Add sync bytes for data1 */
				*(u32 *)((char *)desc[i] + media_desc[media_n]->l2_offset) = htonl(sync_bytes);
				if (num_data == 2) {
					/* Keep data2 in place, move data1 and insert sync_bytes between data payloads */
					os_memmove((char *) ((char *)desc[i] + media_desc[media_n]->l2_offset + sizeof(sync_bytes)),
						(char *) ((char *)desc[i] + media_desc[media_n]->l2_offset + sizeof(sync_bytes) + CVF_H264_STAP_A_NALU_SIZE_HDR_SIZE),
						nalu_data_size);
					/* Add sync bytes for data2 */
					*(u32 *)((char *)desc[i] + media_desc[media_n]->l2_offset + sizeof(sync_bytes) + nalu_data_size) = htonl(sync_bytes);
				}

				is_aggregation = 1;
				break;
			}
			case CVF_H264_NALU_TYPE_STAP_B:
			case CVF_H264_NALU_TYPE_MTAP16:
			case CVF_H264_NALU_TYPE_MTAP24:
				/* still not handled by the software */
				os_log(LOG_DEBUG, "stream_id(%016"PRIx64") CVF_H264 NALU TYPE %d still not handled\n", ntohll(stream->id), nalu_header->type);
				stream->stats.format_err++;
				free_rx_desc = 1;
				goto next_iteration;
			case CVF_H264_NALU_TYPE_FU_B:
			case CVF_H264_NALU_TYPE_FU_A:
			{
				nalu_fu_header = (struct cvf_h264_nalu_fu_header *)(nalu_header + 1);
				is_last_fu = 0;
				if (nalu_fu_header->s) { /* NALU starts here */
					struct cvf_h264_nalu_header temp_nalu_header;
					int temp_offset;

					/* Reconstruct NALU header based on the type defined in the FU header */
					temp_nalu_header.f = nalu_header->f;
					temp_nalu_header.nri = nalu_header->nri;
					temp_nalu_header.type = nalu_fu_header->type;

					temp_offset = sizeof(sync_bytes) - sizeof(struct cvf_h264_nalu_header);
					media_desc[media_n]->l2_offset -= temp_offset;
					media_desc[media_n]->len += temp_offset;

					/* Add 4 bytes sync_bytes */
					*(u32 *)((char *)desc[i] + media_desc[media_n]->l2_offset) = htonl(sync_bytes);
					/* Replace NALU header after the sync_bytes */
					*(struct cvf_h264_nalu_header *)((char *)desc[i] + media_desc[media_n]->l2_offset + sizeof(sync_bytes)) = temp_nalu_header;
				} else { /* Not a starting NALU packet, strip the 2 NALU header bytes */
					media_desc[media_n]->l2_offset += 2;
					media_desc[media_n]->len -= 2;
					if(nalu_fu_header->e)
						is_last_fu = 1;
				}
				break;
			}
			default: /* SINGLE NAL Unit packet */
			{
			/* The entire payload including the NAL unit header is the output */
			/* Just add the sync bytes */
				int temp_offset;

				is_single_packet = 1;
				temp_offset = sizeof(sync_bytes);

				media_desc[media_n]->l2_offset -= temp_offset;
				media_desc[media_n]->len += temp_offset;

				*(u32 *)((char *)desc[i] + media_desc[media_n]->l2_offset) = htonl(sync_bytes);
				break;
			}
		}

		if (media_desc[media_n]->flags) {
			/* Make room for packet-level flags by reserving one event.
			* We cannot fit all possible flags in media_desc->avtp_ts[x].flags, so here we
			* only reserve some space in the ts/event array to simplify accounting later on. The
			* packet-level flags (from media_desc[media_n]->flags) will be posted to the media application
			* by the media driver. */

			media_desc[media_n]->avtp_ts[0].offset = 0;
			media_desc[media_n]->avtp_ts[0].flags = AVTP_FLAGS_TO_MEDIA_DESC(AVTP_TIMESTAMP_INVALID);
			media_desc[media_n]->n_ts = 1;
		} else
			media_desc[media_n]->n_ts = 0;

		/* Signal the End-of-Frame (end of NALU):
		* - Marker bit is present : meaning end of Access Unit but not too reliable
		* - The end of a fragmentation unit packets forming a NALU
		* - On every Single NALU packet
		* - FIXME: for the STAP/MTAP, ideally we should send as end-of-frames as number of NALUs
		* - FIXME: We should send avtp timestamp with the h264 timestamp, for now we send
			   only the h264 timestamp as it represents the presentation timestamp*/

		if (h264_avtp_hdr->M || (is_last_fu) || (is_single_packet) || (is_aggregation)) {
			media_desc[media_n]->avtp_ts[media_desc[media_n]->n_ts].offset = media_desc[media_n]->len - 1;  // Signal the End-of-Frame on the last byte of the packet
			media_desc[media_n]->avtp_ts[media_desc[media_n]->n_ts].flags = AVTP_FLAGS_TO_MEDIA_DESC(AVTP_END_OF_FRAME);
			/*Here we send the h264_timestamp using the avtp timestamp flag*/
			if (!h264_avtp_hdr->ptv) {
				media_desc[media_n]->avtp_ts[media_desc[media_n]->n_ts].val = 0;
				media_desc[media_n]->avtp_ts[media_desc[media_n]->n_ts].flags |= AVTP_FLAGS_TO_MEDIA_DESC(AVTP_TIMESTAMP_INVALID);
			} else
				media_desc[media_n]->avtp_ts[media_desc[media_n]->n_ts].val = h264_timestamp;

			/*If AVTP timestamp is valid, use it for stats calculation*/
			if (!(desc[i]->flags & AVTP_TIMESTAMP_INVALID) && !stats) {
				stats = 1;
				avtp_latency_stats(stream, desc[i]);
			}
			media_desc[media_n]->n_ts++;
		}

		media_n++;
next_iteration:
		if(free_rx_desc) {
			desc[i]->desc.l2_offset = l2_offset_orig;
			desc[i]->desc.len = len_orig;
			net_rx_free(&desc[i]->desc);
		}
	}

	stream_media_tx(stream, media_desc, media_n);
}


/** Handles transmission of cvf-h264 avtp packets
 *
 * Reads data from media stack, converts media descriptors to network descriptors, including protocol encapsulation,
 * and transmit packets. AVTP timestamps are read from media clock generation layer.
 *
 * \return none
 * \param stream pointer to talker stream context
 */
static void avtp_cvf_h264_net_tx(struct stream_talker *stream)
{
	u32 i;
	struct media_rx_desc *media_desc_array[NET_TX_BATCH], *media_desc;
	struct net_tx_desc *net_desc;
	u32 ts[TS_TX_BATCH];
	int rc;
	int ts_n = 0,ts_idx;
	unsigned int ts_batch, frames_in_packet = 0;
	unsigned int flags = 0;
	unsigned int partial,end_of_frame,set_single_packet, media_len;
	unsigned int alignment_ts = 0;
	unsigned start_fu = 0;
	u8 * buf = NULL;
	struct avtp_cvf_h264_hdr *cvf_hdr = NULL;
	u8 * hdr_buf = NULL;

	/* Get H264 data from media stack */
	if (unlikely((rc = stream_media_rx(stream, media_desc_array, ts, &flags, &alignment_ts)) < 0)) {
		goto media_rx_fail;
	}

	/*Fetch Maximum Timestamps which will be stream->tx_batch */
	ts_batch = stream->tx_batch;

	stream->consumer.gptp_current = stream->gptp_current;
	ts_n = media_clock_gen_get_ts(&stream->consumer, ts, ts_batch, &flags, alignment_ts);

	if (ts_n != ts_batch) {
		stream->stats.clock_err++;
		goto media_clock_fail;
	}
	if (flags & MCG_FLAGS_RESET) {
		avtp_data_header_toggle_mcr(stream->avtp_hdr);
	}

	stream->stats.clock_rx += ts_n;

	i = 0;

	media_len = 0;
	ts_idx = 0;

	while (i < rc) {
		media_desc = media_desc_array[i];
		net_desc = &media_desc->net;
		media_len = net_desc->len;
		start_fu = 0;
		set_single_packet = 0;
		partial = net_desc->flags & NET_TX_FLAGS_PARTIAL;
		end_of_frame = net_desc->flags & NET_TX_FLAGS_END_FRAME;
		net_desc->flags = 0;

		buf = NET_DATA_START(net_desc);

		/*Check if we expect a new NALU */
		if (!(stream->subtype_data.cvf_h264.prev_incomplete_nal))
		{
			/*Check if it's a start of an FU-A packets or a single packet NALU */
			if (buf[0] == CVF_H264_NALU_TYPE_FU_A)
				start_fu = 1;
			else
			 	set_single_packet = 1;
		}

		 /*Store the new nalu header depending on the received packet */
                if (set_single_packet)
			stream->subtype_data.cvf_h264.nalu_header = buf[0];
		else if (start_fu)
			stream->subtype_data.cvf_h264.nalu_header = buf[1];

		if (!set_single_packet)
		{
			u8 * nalu_fu_header_buf = NULL;
			struct cvf_h264_nalu_fu_header *fu_hdr = NULL;
			struct cvf_h264_nalu_header *nalu_hdr = NULL;

			/*Set the FU indicator*/
			nalu_hdr = (struct cvf_h264_nalu_header *) buf;
			nalu_hdr->f = 0; /*forbidden zero bit*/
			nalu_hdr->nri = ((stream->subtype_data.cvf_h264.nalu_header & 0x60) >> 5);
			nalu_hdr->type = CVF_H264_NALU_TYPE_FU_A;

			/*Set the FU header*/
			nalu_fu_header_buf = buf +  sizeof(struct cvf_h264_nalu_header);

			fu_hdr = (struct cvf_h264_nalu_fu_header *) nalu_fu_header_buf;
			/*Set FU header to 0*/
			os_memset(fu_hdr, 0, sizeof(struct cvf_h264_nalu_fu_header));
			fu_hdr->type = stream->subtype_data.cvf_h264.nalu_header & 0x1f;
			fu_hdr->r = 0;

			if(start_fu)
				fu_hdr->s = 1; /*This is the first FU packet of the NALU*/
			else if (end_of_frame)
				fu_hdr->e = 1; /*This is the last FU packet of the NALU*/
		}

		/*Only last packet of a NALU contains avtp timestamp*/
		if (end_of_frame) {

			avtp_data_header_set_timestamp(stream->avtp_hdr, ts[ts_idx]);

			stream->ts_n++;
			ts_idx++;
		} else
			avtp_data_header_set_timestamp_invalid(stream->avtp_hdr);

		/*Set the right stream_data_length*/
		avtp_data_header_set_len(stream->avtp_hdr, sizeof(struct cvf_h264_hdr) + net_desc->len);

		cvf_hdr = (struct avtp_cvf_h264_hdr *) stream->avtp_hdr;

		/*Check if the NALU time is correctly sent (should be only sent with the first bytes of the NALU)*/
		if (!(media_desc->ts_n) && (set_single_packet || start_fu)) {
			/*This is a start of NALU with no timestamps sent:
			 * Mark as invalid*/
			stream->subtype_data.cvf_h264.is_nalu_ts_valid = 0;
			stream->stats.clock_invalid++;
		} else if ((media_desc->ts_n && !(set_single_packet || start_fu))) {
			/*This is a timestamp in a middle of NALU:
			 * Increment error stat but keep using the last valid timestamp for the rest of the fragments*/
			stream->stats.clock_invalid++;
		} else if (media_desc->ts_n) {
			/*This is a timestamp in at the start of NALU:
			 * Mark as valid and save for the rest of the fragments*/
			stream->subtype_data.cvf_h264.is_nalu_ts_valid = 1;
			stream->subtype_data.cvf_h264.h264_timestamp = media_desc->avtp_ts[0].val;
		}

		if(stream->subtype_data.cvf_h264.is_nalu_ts_valid){
			cvf_hdr->ptv = 1;
			stream->subtype_data.cvf_h264.h264_hdr->h264_timestamp = htonl(stream->subtype_data.cvf_h264.h264_timestamp);
		} else {
			cvf_hdr->ptv = 0;
			stream->subtype_data.cvf_h264.h264_hdr->h264_timestamp = htonl(0);
		}

		net_desc->l2_offset -= stream->header_len;
		net_desc->len += stream->header_len;
		net_desc->flags = 0;
		hdr_buf = NET_DATA_START(net_desc);

		os_memcpy(hdr_buf, stream->header_template, stream->header_len);

		if (end_of_frame)
			stream->subtype_data.cvf_h264.prev_incomplete_nal = 0;
		else
			stream->subtype_data.cvf_h264.prev_incomplete_nal = 1;

		stream->avtp_hdr->sequence_num++;

		if (partial)
			frames_in_packet = media_len; /*sample_stride for h264 is equal to 1 */
		else
			frames_in_packet = stream->frames_per_packet;

		stream->media_count += frames_in_packet;

		i++;
	}

	if (stream_net_tx(stream, media_desc_array, i))
		goto transmit_fail;

	return;
media_clock_fail:
	net_free_multi((void **)media_desc_array, rc);

	return;
media_rx_fail:
	stream->media_count = 0;
	return;
transmit_fail:
	return;

}

static void cvf_mjpeg_net_rx(struct stream_listener *stream, struct avtp_rx_desc **desc, unsigned int n)
{
	struct cvf_mjpeg_hdr *mjpeg_hdr;
	struct media_desc *media_desc[NET_RX_BATCH];
	struct avtp_cvf_mjpeg_hdr *mjpeg_avtp_hdr;
	unsigned int media_n = 0;
	unsigned int stats = 0;
	unsigned int scan_type;
	unsigned int fragment_offset;
	int i;
	struct avdecc_format_cvf_mjpeg_t *mjpeg;

	os_log(LOG_DEBUG, "enter stream(%p) stream_id(%016"PRIx64") \n", stream, ntohll(stream->id));

	for (i = 0; i < n; i++) {

		mjpeg_avtp_hdr = (struct avtp_cvf_mjpeg_hdr *)((char *)desc[i] + desc[i]->desc.l3_offset);
		mjpeg_hdr = (struct cvf_mjpeg_hdr *)((char *)desc[i] + desc[i]->l4_offset);

		if (unlikely((mjpeg_avtp_hdr->format != CVF_FORMAT_RFC)
			|| (mjpeg_avtp_hdr->format_subtype != CVF_FORMAT_SUBTYPE_MJPEG))) {
			stream->stats.format_err++;
			os_log(LOG_DEBUG,"stream_id(%016"PRIx64") CVF MJPEG not recognized: format %u format_subtype %u rsvd %u\n", ntohll(stream->id), mjpeg_avtp_hdr->format, mjpeg_avtp_hdr->format_subtype, mjpeg_avtp_hdr->reserved);
			net_rx_free(&desc[i]->desc);

			/* FIXME we should indicate packets were lost */
			continue; // Should never happen if stream _is_ CVF-MJPEG
		}

		mjpeg = &stream->format.u.s.subtype_u.cvf.format_u.mjpeg;
		if (unlikely(mjpeg->type != mjpeg_hdr->type)) {
			stream->stats.format_err++;
			os_log(LOG_DEBUG,"stream_id(%016"PRIx64") CVF MJPEG type mismatch: configured = %u but in-stream = %u\n", ntohll(stream->id), mjpeg->type, mjpeg_hdr->type);
		}

		scan_type = mjpeg_hdr->type_specific == CVF_MJPEG_TYPE_SPEC_PROGRESSIVE ?
					CVF_MJPEG_P_PROGRESSIVE : CVF_MJPEG_P_INTERLACE;
		if (unlikely(mjpeg->p != scan_type)) {
			stream->stats.format_err++;
			os_log(LOG_DEBUG,"stream_id(%016"PRIx64") CVF MJPEG scan type mistmatch: configured = %s but in-stream = %s\n",
					ntohll(stream->id), SCAN_TYPE_2_STR(mjpeg->p), SCAN_TYPE_2_STR(scan_type));
		}

		if (unlikely((mjpeg->width != mjpeg_hdr->width)
			|| (mjpeg->height != mjpeg_hdr->height))) {
			stream->stats.format_err++;
			os_log(LOG_DEBUG,"stream_id(%016"PRIx64") CVF MJPEG frame size mistmatch: configured = %ux%u but in-stream = %du%u\n",
					ntohll(stream->id), mjpeg->width,  mjpeg->height, mjpeg_hdr->width, mjpeg_hdr->height);
		}


		/* FIXME The code below works but is very sensitive to operation order, because the source and destination structs
		 * actually point to the same memory area. Need to make it safer to avoid overwriting a valid field by mistake.
		 */
		media_desc[media_n] = (struct media_desc *)desc[i];
		media_desc[media_n]->l2_offset = desc[i]->l4_offset + sizeof(struct cvf_mjpeg_hdr);
		media_desc[media_n]->len = desc[i]->l4_len - sizeof(struct cvf_mjpeg_hdr);
		fragment_offset = (mjpeg_hdr->fragment_offset_msb << 16) + ntohs(mjpeg_hdr->fragment_offset_lsb);
		media_desc[media_n]->bytes_lost = 0;
		if (fragment_offset != stream->subtype_data.cvf.current_frame_offset) {
			stream->stats.format_err++;
			if (fragment_offset > stream->subtype_data.cvf.current_frame_offset)
				media_desc[media_n]->bytes_lost = fragment_offset - stream->subtype_data.cvf.current_frame_offset;
			else
				os_log(LOG_DEBUG, "stream_id(%016"PRIx64") CVF MJPEG desc(%p) fragment offset invalid (current packet overlaps %d bytes (%u - %u) of previous frame).\n",
						ntohll(stream->id), desc[i], stream->subtype_data.cvf.current_frame_offset - fragment_offset,
						stream->subtype_data.cvf.current_frame_offset, fragment_offset);
			stream->subtype_data.cvf.current_frame_offset = fragment_offset;

			//Gather both cases under AVTP_PACKET_LOST to notify the media application
			//FIXME Add new report flags?
			desc[i]->flags &= AVTP_PACKET_LOST;
		}
		stream->subtype_data.cvf.current_frame_offset += media_desc[media_n]->len;

		media_desc[media_n]->flags = desc[i]->flags & (AVTP_MEDIA_CLOCK_RESTART | AVTP_PACKET_LOST | AVTP_TIMESTAMP_UNCERTAIN);
		media_desc[media_n]->ts = desc[i]->desc.ts; // This is actually a copy onto itself. Kept for clarity...


		if (media_desc[media_n]->flags) {
			/* Make room for packet-level flags by reserving one event.
			 * We cannot fit all possible flags in media_desc->avtp_ts[x].flags (only 4 bits), so here we
			 * only reserve some space in the ts/event array to simplify accounting later on. The
			 * packet-level flags (from media_desc[media_n]->flags) will be posted to the media application
			 * by the media driver. */
			media_desc[media_n]->avtp_ts[0].offset = 0;
			media_desc[media_n]->avtp_ts[0].flags = AVTP_FLAGS_TO_MEDIA_DESC(AVTP_TIMESTAMP_INVALID);
			media_desc[media_n]->n_ts = 1;
		} else
			media_desc[media_n]->n_ts = 0;


		if (mjpeg_avtp_hdr->M) {
			media_desc[media_n]->avtp_ts[media_desc[media_n]->n_ts].offset = media_desc[media_n]->len - 1;  // Signal the End-of-Frame on the last byte of the packet
			media_desc[media_n]->avtp_ts[media_desc[media_n]->n_ts].flags = AVTP_FLAGS_TO_MEDIA_DESC(AVTP_END_OF_FRAME);
			if (desc[i]->flags & AVTP_TIMESTAMP_INVALID) {
				media_desc[media_n]->avtp_ts[media_desc[media_n]->n_ts].val = 0;
				media_desc[media_n]->avtp_ts[media_desc[media_n]->n_ts].flags |= AVTP_FLAGS_TO_MEDIA_DESC(AVTP_TIMESTAMP_INVALID);
			} else {
				media_desc[media_n]->avtp_ts[media_desc[media_n]->n_ts].val = desc[i]->avtp_timestamp;

				if (!stats) {
					stats = 1;
					avtp_latency_stats(stream, desc[i]);
				}
			}
			media_desc[media_n]->n_ts++;
			stream->subtype_data.cvf.current_frame_offset = 0;
		}

		media_n++;
	}

	stream_media_tx(stream, media_desc, media_n);
}


/* P1722_D14 chapter I.2.1.4.1 */
int stream_cvf_mjpeg_check_format(struct stream_listener const *stream, struct avdecc_format_cvf_mjpeg_t const *mjpeg)
{
	int rc = GENAVB_SUCCESS;

	/* width and height should be > 0 */
	if ((!mjpeg->width || !mjpeg->height)) {
		os_log(LOG_ERR, "stream_id(%016"PRIx64") MJPEG height/width (%u,%u) parameters cannot be 0\n",
				ntohll(stream->id), mjpeg->height, mjpeg->width);
		goto err_format;
	}

	return rc;

err_format:
	return -GENAVB_ERR_STREAM_PARAMS;
}

static int listener_stream_cvf_init(struct stream_listener *stream)
{
	struct avdecc_format const *format = &stream->format;

	switch (format->u.s.subtype_u.cvf.subtype) { /* P1722_D14 table 20 */
	case CVF_FORMAT_SUBTYPE_MJPEG:
		stream->net_rx = cvf_mjpeg_net_rx;
		stream->subtype_data.cvf.current_frame_offset = 0;
		break;
	case CVF_FORMAT_SUBTYPE_H264:
		stream->net_rx = avtp_cvf_h264_net_rx;
		break;

	default:
		break;
	}

	return 0;
}

/* P1722_D14 chapter I.2.1.4 */
int listener_stream_cvf_check(struct stream_listener *stream, struct avdecc_format const *format, u16 flags)
{
	int rc = GENAVB_SUCCESS;

	switch (format->u.s.subtype_u.cvf.format) { /* P1722_D14 table 19 */
	case CVF_FORMAT_RFC:
		break;

	default:
		os_log(LOG_ERR, "stream_id(%016"PRIx64") Compressed Video Format (%u) not supported\n",
				ntohll(stream->id), format->u.s.subtype_u.cvf.format);
		goto err_format;
	}

	if (flags & IPC_AVTP_FLAGS_MCR) {
		os_log(LOG_ERR, "stream_id(%016"PRIx64") Media clock recovery not supported\n", ntohll(stream->id));
		goto err_format;
	}

	switch (format->u.s.subtype_u.cvf.subtype) { /* P1722_D14 table 20 */
	case CVF_FORMAT_SUBTYPE_MJPEG:
		rc = stream_cvf_mjpeg_check_format(stream, &format->u.s.subtype_u.cvf.format_u.mjpeg);
		break;

	case CVF_FORMAT_SUBTYPE_H264:
		/* Nothing to care for now */
		break;

	/* unsupported formats for now */
	case CVF_FORMAT_SUBTYPE_JPEG2000:
	default:
		os_log(LOG_ERR, "stream_id(%016"PRIx64") CVF subtype (%u) not supported\n",
				ntohll(stream->id), format->u.s.subtype_u.cvf.subtype);
		goto err_format;
	}

	stream->init = listener_stream_cvf_init;

	return rc;

err_format:
	return -GENAVB_ERR_STREAM_PARAMS;
}


static unsigned int cvf_prepare_header(struct avtp_cvf_hdr *hdr, const struct avdecc_format *format, void *stream_id)
{
	/* AVTP stream common fields */
	hdr->subtype = AVTP_SUBTYPE_CVF;
	hdr->version = AVTP_VERSION_0;
	hdr->sv = 1;

	copy_64(&hdr->stream_id, stream_id);

	/* Format fields */
	hdr->format = format->u.s.subtype_u.cvf.format;
	hdr->format_subtype = format->u.s.subtype_u.cvf.subtype;

	hdr->evt = 0;

	return sizeof(struct avtp_cvf_hdr);
}


static unsigned int avtp_cvf_h264_prepare_header(struct avtp_data_hdr *hdr, const struct avdecc_format *format)
{
	/*The h264 header contain only h264 timestamp, return the size only for now*/
	return sizeof(struct cvf_h264_hdr);
}


static void talker_stream_cvf_init(struct stream_talker *stream, unsigned int *hdr_len)
{
	struct avdecc_format const *format = &stream->format;

	switch (format->u.s.subtype_u.cvf.subtype) {
	case CVF_FORMAT_SUBTYPE_H264:
		stream->subtype_data.cvf_h264.h264_hdr = (struct cvf_h264_hdr *)(stream->avtp_hdr + 1);
		stream->subtype_data.cvf_h264.prev_incomplete_nal = 0;
		stream->subtype_data.cvf_h264.h264_timestamp = 0;
		stream->subtype_data.cvf_h264.is_nalu_ts_valid = 0;
		stream->net_tx = avtp_cvf_h264_net_tx;
		*hdr_len = avtp_cvf_h264_prepare_header(stream->avtp_hdr, format);
		break;
	default:
		break;
	}

	*hdr_len += cvf_prepare_header((struct avtp_cvf_hdr *)stream->avtp_hdr, format, &stream->id);

	stream->common.flags |= STREAM_FLAG_CLOCK_GENERATION;

}
/* Check stream format and initialize parameters */
int talker_stream_cvf_check(struct stream_talker *stream, struct avdecc_format const *format,
					struct ipc_avtp_connect *ipc)
{
	int rc = GENAVB_SUCCESS;

	switch (format->u.s.subtype_u.cvf.format) {
	case CVF_FORMAT_RFC:
		break;

	default:
		os_log(LOG_ERR, "stream_id(%016"PRIx64") Compressed Video Format (%u) not supported\n",
				ntohll(stream->id), format->u.s.subtype_u.cvf.format);
		goto err_format;
	}

	switch (format->u.s.subtype_u.cvf.subtype) {

	case CVF_FORMAT_SUBTYPE_H264:
		/* Nothing to care for now */
		break;

	/* unsupported formats for now */
	case CVF_FORMAT_SUBTYPE_MJPEG:
	case CVF_FORMAT_SUBTYPE_JPEG2000:
	default:
		os_log(LOG_ERR, "stream_id(%016"PRIx64") CVF subtype (%u) not supported\n",
				ntohll(stream->id), format->u.s.subtype_u.cvf.subtype);
		goto err_format;
	}

	/*FIXME Assume nothing to do for now*/
	stream->init = talker_stream_cvf_init;
	return rc;

err_format:
	return -GENAVB_ERR_STREAM_PARAMS;
}

#endif /* CFG_AVTP_1722A */
