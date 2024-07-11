/*
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
 * Copyright 2016-2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file		avdecc.c
 @brief  	AVDECC protocol common functions implementations
*/

#include "genavb/avdecc.h"
#include "genavb/sr_class.h"
#include "avtp.h"
#include "61883_iidc.h"
#include "aaf.h"
#include "cvf.h"

unsigned int avdecc_fmt_hdr_size(const struct avdecc_format *format)
{
	unsigned int hdr_size = 0;

	switch (format->u.s.subtype) {
	case AVTP_SUBTYPE_61883_IIDC:
		switch (format->u.s.subtype_u.iec61883.fmt) {
		case IEC_61883_CIP_FMT_4:
		case IEC_61883_CIP_FMT_6:
			hdr_size = sizeof(struct iec_61883_hdr);
			break;

		default:
			/* FIXME */
			break;
		}

		break;

#ifdef CFG_AVTP_1722A
	case AVTP_SUBTYPE_CVF:
		if (format->u.s.subtype_u.cvf.format == CVF_FORMAT_RFC) {
			switch (format->u.s.subtype_u.cvf.subtype) {
			case CVF_FORMAT_SUBTYPE_MJPEG:
				hdr_size = sizeof (struct cvf_mjpeg_hdr);
				break;

			case CVF_FORMAT_SUBTYPE_H264:
				hdr_size = sizeof (struct cvf_h264_hdr);
				break;

			case CVF_FORMAT_SUBTYPE_JPEG2000:
			default:
				break;
			}
		}

		break;

	case AVTP_SUBTYPE_AAF:
		break;

	case AVTP_SUBTYPE_CRF:
		break;

	case AVTP_SUBTYPE_TSCF:
		break;
#endif
	default:
		break;
	}

	return hdr_size;
}


unsigned int avdecc_fmt_sample_stride(const struct avdecc_format *format)
{
	unsigned int sample_stride = 0;

	switch (format->u.s.subtype) {
	case AVTP_SUBTYPE_61883_IIDC:
		if (format->u.s.subtype_u.iec61883.sf == IEC_61883_SF_IIDC)
			break;

		switch (format->u.s.subtype_u.iec61883.fmt) {
		case IEC_61883_CIP_FMT_4:
			sample_stride = IEC_61883_4_SP_SIZE;
			break;

		case IEC_61883_CIP_FMT_6:
			sample_stride = (format->u.s.subtype_u.iec61883.format_u.iec61883_6.dbs == 0?256:format->u.s.subtype_u.iec61883.format_u.iec61883_6.dbs) << 2;
			break;

		case IEC_61883_CIP_FMT_8:
		default:
			break;
		}

		break;

#ifdef CFG_AVTP_1722A
	case AVTP_SUBTYPE_CVF:
		if (format->u.s.subtype_u.cvf.format == CVF_FORMAT_RFC)
			sample_stride = 1;

		break;

	case AVTP_SUBTYPE_AAF:
		sample_stride = avdecc_fmt_sample_size(format);
		break;

	case AVTP_SUBTYPE_CRF:
		sample_stride = avdecc_fmt_sample_size(format);
		break;

	case AVTP_SUBTYPE_TSCF:
		sample_stride = avdecc_fmt_sample_size(format);
		break;
#endif
	default:
		break;
	}

	if (!sample_stride)
		sample_stride = AVDECC_FMT_ERROR;

	return sample_stride;
}


unsigned int samples_per_interval(unsigned int rate, sr_class_t sr_class)
{
	/* samples_per_intvl = rate * sr_class_interval = rate * sr_class_interval_p / (sr_class_interval_q * 10^9) */
	return ((rate / 100) * (sr_class_interval_p(sr_class) / 1000) + 10000 * sr_class_interval_q(sr_class) - 1) / (10000 * sr_class_interval_q(sr_class));
}

unsigned int __avdecc_fmt_samples_per_packet(const struct avdecc_format *format, sr_class_t sr_class, unsigned int *max_interval_frames)
{
	unsigned int samples_per_intvl, samples_per_packet;
	unsigned int rate, stride, max_samples_per_packet;
	unsigned int interval_packets = 1;

	switch (format->u.s.subtype) {
	case AVTP_SUBTYPE_AAF:
		if (avdecc_format_is_aaf_pcm(format))
			samples_per_packet = AVDECC_FMT_AAF_PCM_SAMPLES_PER_FRAME(format);
		else if (avdecc_format_is_aaf_aes3(format))
			samples_per_packet = format->u.s.subtype_u.aaf.format_u.aes3.frames_per_frame;
		else
			goto err_fmt;

		if (!samples_per_packet)
			goto err_fmt;

		rate = avdecc_fmt_sample_rate(format);
		if (rate == AVDECC_FMT_ERROR)
			goto err_fmt;

		samples_per_intvl = samples_per_interval(rate, sr_class);

		interval_packets = (samples_per_intvl + samples_per_packet - 1) / samples_per_packet;

		break;

	case AVTP_SUBTYPE_CRF:
		samples_per_packet = format->u.s.subtype_u.crf.timestamps_per_pdu;
		if (!samples_per_packet)
			goto err_fmt;

		break;

	default:
		rate = avdecc_fmt_sample_rate(format);
		stride = avdecc_fmt_sample_stride(format);

		if ((stride == AVDECC_FMT_ERROR) || (rate == AVDECC_FMT_ERROR))
			goto err_fmt;

		max_samples_per_packet = (AVTP_DATA_MTU - avdecc_fmt_hdr_size(format)) / stride;

		if (avdecc_format_is_61883_6(format)) {
			switch (rate) {
			case 32000:
				samples_per_intvl = samples_per_interval(rate, sr_class);
				break;

			case 44100:
			case 88200:
			case 176400:
				samples_per_intvl = samples_per_interval(44100, sr_class);
				samples_per_intvl *= rate / 44100;
				break;

			case 48000:
			case 96000:
			case 192000:
				samples_per_intvl = samples_per_interval(48000, sr_class);
				samples_per_intvl *= rate / 48000;
				break;

			default:
				goto err_fmt;
				break;
			}
		} else {
			samples_per_intvl = samples_per_interval(rate, sr_class);
		}

		if (samples_per_intvl > max_samples_per_packet) {
			/* More than one packet required, try to minimize the number of packets per interval */
			interval_packets = (samples_per_intvl + max_samples_per_packet - 1) / max_samples_per_packet;

			samples_per_packet = ((samples_per_intvl + interval_packets - 1) / interval_packets);
		} else {
			samples_per_packet = samples_per_intvl;
		}

		break;
	}

	if (max_interval_frames)
		*max_interval_frames = interval_packets;

	return samples_per_packet;

err_fmt:
	if (max_interval_frames)
		*max_interval_frames = 0;

	return AVDECC_FMT_ERROR;
}

unsigned int avdecc_fmt_samples_per_packet(const struct avdecc_format *format, sr_class_t sr_class)
{
	return __avdecc_fmt_samples_per_packet(format, sr_class, NULL);
}

unsigned int avdecc_fmt_samples_per_timestamp(const struct avdecc_format *format, sr_class_t sr_class)
{
	unsigned int samples_per_timestamp = 0;

	switch (format->u.s.subtype) {
	case AVTP_SUBTYPE_61883_IIDC:
		if (format->u.s.subtype_u.iec61883.sf == IEC_61883_SF_IIDC)
			break;

		switch (format->u.s.subtype_u.iec61883.fmt) {
		case IEC_61883_CIP_FMT_6:
		{
			/* SYT_INTERVAL */
			unsigned int samples_per_packet = avdecc_fmt_samples_per_packet(format, sr_class);

			samples_per_timestamp = 8;
			while (samples_per_timestamp < samples_per_packet)
				samples_per_timestamp <<= 1;

			break;
		}

		case IEC_61883_CIP_FMT_4:
			samples_per_timestamp = 1;
			break;

		default:
			break;
		}

		break;

	case AVTP_SUBTYPE_AAF:
		/* For sparse mode this is wrong */
		samples_per_timestamp = avdecc_fmt_samples_per_packet(format, sr_class);
		break;

	case AVTP_SUBTYPE_CRF:
		samples_per_timestamp = 1;
		break;

	case AVTP_SUBTYPE_CVF: /*TODO check this ??? for now make it similar to mpeg2-ts*/
		samples_per_timestamp = avdecc_fmt_samples_per_packet(format, sr_class);
		break;

	default:
		break;
	}

	return samples_per_timestamp;
}

unsigned int __avdecc_fmt_payload_size(const struct avdecc_format *format, sr_class_t sr_class, unsigned int *max_frame_size, unsigned int *max_interval_frames)
{
	unsigned int payload_size;

	payload_size = __avdecc_fmt_samples_per_packet(format, sr_class, max_interval_frames) * avdecc_fmt_sample_stride(format);

	if (max_frame_size)
		*max_frame_size = payload_size;

	return payload_size;
}


unsigned int avdecc_fmt_sample_rate(const struct avdecc_format *format)
{
	unsigned int sample_rate = 0;

	switch (format->u.s.subtype) {
	case AVTP_SUBTYPE_61883_IIDC:
		if (format->u.s.subtype_u.iec61883.sf == IEC_61883_SF_IIDC)
			break;

		switch (format->u.s.subtype_u.iec61883.fmt) {
		case IEC_61883_CIP_FMT_6:	// For AVDECC formats, fdf != IEC_61883_6_FDF_NODATA should be a valid assumption
			sample_rate = avtp_61883_6_sampling_freq[format->u.s.subtype_u.iec61883.format_u.iec61883_6.fdf_u.fdf.sfc];
			break;

		case IEC_61883_CIP_FMT_4:
			 /* FIXME this should be deduced from the contents of the MPEG2TS file
			  for now supports up to 16000 * 188 * 8 ~ 24Mbit/s MPEG2TS rate */
			sample_rate = 16000;
			break;

		case IEC_61883_CIP_FMT_8:
		default:
			break;
		}

		break;

#ifdef CFG_AVTP_1722A
	case AVTP_SUBTYPE_CVF:
		if (format->u.s.subtype_u.cvf.format == CVF_FORMAT_RFC) {
			switch (format->u.s.subtype_u.cvf.subtype) {
			case CVF_FORMAT_SUBTYPE_MJPEG:
				sample_rate = 3750000; // FIXME for now, hard-coded to 30 Mbps
				break;

			case CVF_FORMAT_SUBTYPE_H264:
				sample_rate = 3000000; /* FIXME  for now hardcoded for 24 Mbps*/
				break;
			case CVF_FORMAT_SUBTYPE_JPEG2000:
				/* FIXME */
			default:
				break;
			}
		}

		break;

	case AVTP_SUBTYPE_AAF:
		sample_rate = avtp_aaf_sampling_freq[format->u.s.subtype_u.aaf.nsr];
		break;

	case AVTP_SUBTYPE_CRF: {
		unsigned int p, q;

		switch (format->u.s.subtype_u.crf.pull) {
		case CRF_PULL_1_1:
		default:
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
		}

		/* 1722rev1-2016, Table 28 recommends 300Hz */
		sample_rate = ((u64)AVDECC_FMT_CRF_BASE_FREQUENCY(format) * p) / (AVDECC_FMT_CRF_TIMESTAMP_INTERVAL(format) * q);

		break;
	}
#endif
	default:
		break;
	}

	if (!sample_rate)
		sample_rate = AVDECC_FMT_ERROR;

	return sample_rate;
}
