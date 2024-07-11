/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2021-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file avdecc.h
 \brief GenAVB public API
 \details AVDECC format definition and helper functions.

 \copyright Copyright 2014-2016 Freescale Semiconductor, Inc.
 \copyright Copyright 2016-2018, 2021-2023 NXP
*/
#ifndef _GENAVB_PUBLIC_AVDECC_H_
#define _GENAVB_PUBLIC_AVDECC_H_


#include "sr_class.h"
#include "avtp.h"
#include "crf.h"
#include "61883_iidc.h"
#include "cvf.h"
#include "aaf.h"
#include "types.h"
#include "aecp.h"
#include "acmp.h"
#include "adp.h"
#include "net_types.h"

/** \defgroup avdecc_format AVDECC FORMAT
 * \ingroup stream
 */

/**
 * \ingroup avdecc_format
 * This structure maps the AVDECC format definition specified by IEEE1722.1-2013.
 */
struct __attribute__ ((packed)) avdecc_format {
	union {
		struct __attribute__ ((packed)) {
#ifdef __BIG_ENDIAN__
			avb_u8 v:1;
			avb_u8 subtype:7;

			union {
				struct __attribute__ ((packed)) {		// IEC 61883/IIDC SUBTYPE
					avb_u8 sf:1;
					avb_u8 fmt:6;
					avb_u8 r:1;

					union {
						struct __attribute__ ((packed)) {	// IIDC
							avb_u16 rsvd1;

							avb_u8 rsvd2;

							avb_u8 format;  // format and mode merged as u16 in 1722a D9

							avb_u8 mode;

							avb_u8 rate;
						} iidc;
						struct __attribute__ ((packed)) { // 61883-4
							avb_u16 rsvd1;
							avb_u32 rsvd2;
						} iec61883_4;
						struct __attribute__ ((packed)) avdecc_format_iec61883_6_t {	// 61883-6
							union {
								avb_u8 raw;
								struct __attribute__ ((packed)) {
									avb_u8 evt:5;
									avb_u8 sfc:3;
								} fdf;
							} fdf_u;

							avb_u8 dbs;

#ifdef CFG_AVTP_1722A
							avb_u8 b:1;
							avb_u8 nb:1;
							avb_u8 ut:1;
							avb_u8 sc:1;
							avb_u8 rsvd:4;
#else
							avb_u8 b:1;
							avb_u8 nb:1;
							avb_u8 rsvd:6;
#endif
							avb_u8 label_iec_60958_cnt;

							avb_u8 label_mbla_cnt;

							avb_u8 label_midi_cnt:4;
							avb_u8 label_smptecnt:4;
						} iec61883_6;
						struct __attribute__ ((packed)) avdecc_format_iec61883_8_t {	// 61883-8
							avb_u16 rsvd1;

							avb_u8 rsvd2;

							avb_u8 video_mode;

							avb_u8 compress_mode;

							avb_u8 color_space;
						} iec61883_8;
					} format_u;
				} iec61883;

				struct __attribute__ ((packed)) avdecc_format_aaf_t {		// AVTP AUDIO SUBTYPE
					avb_u8 rsvd:3;
					avb_u8 ut:1;
					avb_u8 nsr:4;

					avb_u8 format;

					union {
						struct __attribute__ ((packed)) avdecc_format_aaf_pcm_t {
							avb_u8 bit_depth;
							avb_u32 channels_per_frame:10;
							avb_u32 samples_per_frame:10;
							avb_u32 reserved:12;
						} pcm;

						struct __attribute__ ((packed)) avdecc_format_aaf_aes3_t {
							avb_u8 frames_per_frame;

							avb_u16 streams_per_frame:10;
							avb_u16 reserved:3;
							avb_u16 aes3_dt_ref:3;

							avb_u16 aes3_data_type;
						} aes3;
					} format_u;
				} aaf;

				struct __attribute__ ((packed)) { 		// AVTP COMPRESSED VIDEO SUBTYPE (per P1722-rev1/D13)
					avb_u8 format;
					avb_u8 subtype;
					union {
						struct __attribute__ ((packed)) avdecc_format_cvf_mjpeg_t {		// MJPEG
							avb_u8 p:1;
							avb_u8 rsvd1:7;
							avb_u8 rsvd2;
							avb_u8 type;
							avb_u8 width;
							avb_u8 height;
						} mjpeg;

						struct __attribute__ ((packed)) avdecc_format_cvf_h264_t {		// H.264
							avb_u8 spec_version; /* normally defined as rsvd1. Used to differentiate the IEEE1722 specification version for now */
							avb_u32 rsvd2;
						} h264;

						struct __attribute__ ((packed)) avdecc_format_cvf_jpeg2000_t {		// JPEG2000
							avb_u8 p:1;
							avb_u8 rsvd1:7;
							avb_u32 rsvd2;
						} jpeg2000;
					} format_u;
				} cvf;

				struct __attribute__ ((packed)) avdecc_format_crf_t {		// AVTP CRF SUBTYPE
					avb_u8 type:4;
					avb_u8 timestamp_interval_msb:4;
					avb_u8 timestamp_interval_lsb;
					avb_u8 timestamps_per_pdu;

					avb_u32 pull:3;
					avb_u32 base_frequency:29;
				} crf;

				struct __attribute__ ((packed)) {
					avb_u8 m:1;
					avb_u8 rsvd1:7;

					avb_u16 rsvd2;

					avb_u8 t3v:1;
					avb_u8 type_3:7;

					avb_u8 t2v:1;
					avb_u8 type_2:7;

					avb_u8 t1v:1;
					avb_u8 type_1:7;

					avb_u8 t0v:1;
					avb_u8 type_0:7;
				} tscf;
			} subtype_u;
#else
			avb_u8 subtype:7;
			avb_u8 v:1;

			union {
				struct __attribute__ ((packed)) {		// IEC 61883/IIDC SUBTYPE
					avb_u8 r:1;
					avb_u8 fmt:6;
					avb_u8 sf:1;

					union {
						struct __attribute__ ((packed)) {	// IIDC
							avb_u16 rsvd1;

							avb_u8 rsvd2;

							avb_u8 format;  // format and mode merged as u16 in 1722a D9

							avb_u8 mode;

							avb_u8 rate;
						} iidc;
						struct __attribute__ ((packed)) { // 61883-4
							avb_u16 rsvd1;
							avb_u32 rsvd2;
						} iec61883_4;
						struct __attribute__ ((packed)) avdecc_format_iec61883_6_t {	// 61883-6
							union {
								avb_u8 raw;
								struct {
									avb_u8 sfc:3;
									avb_u8 evt:5;
								} fdf;
							} fdf_u;


							avb_u8 dbs;

#ifdef CFG_AVTP_1722A
							avb_u8 rsvd:4;
							avb_u8 sc:1;
							avb_u8 ut:1;
							avb_u8 nb:1;
							avb_u8 b:1;
#else
							avb_u8 rsvd:6;
							avb_u8 nb:1;
							avb_u8 b:1;
#endif

							avb_u8 label_iec_60958_cnt;

							avb_u8 label_mbla_cnt;

							avb_u8 label_smptecnt:4;
							avb_u8 label_midi_cnt:4;
						} iec61883_6;
						struct __attribute__ ((packed)) avdecc_format_iec61883_8_t {	// 61883-8
							avb_u16 rsvd1;

							avb_u8 rsvd2;

							avb_u8 video_mode;

							avb_u8 compress_mode;

							avb_u8 color_space;
						} iec61883_8;
					} format_u;
				} iec61883;

				struct __attribute__ ((packed)) avdecc_format_aaf_t {		// AVTP AUDIO SUBTYPE
					avb_u8 nsr:4;
					avb_u8 ut:1;
					avb_u8 rsvd:3;

					avb_u8 format;

					union {
						struct __attribute__ ((packed)) avdecc_format_aaf_pcm_t {
							avb_u8 bit_depth;

							avb_u8 channels_per_frame_msb;

							avb_u8 samples_per_frame_msb:6;
							avb_u8 channels_per_frame_lsb:2;

							avb_u8 reserved_msb:4;
							avb_u8 samples_per_frame_lsb:4;

							avb_u8 reserved_lsb;
						} pcm;

						struct __attribute__ ((packed)) avdecc_format_aaf_aes3_t {
							avb_u8 frames_per_frame;

							avb_u8 streams_per_frame_msb;

							avb_u8 aes3_dt_ref:3;
							avb_u8 reserved:3;
							avb_u8 streams_per_frame_lsb:2;

							avb_u16 aes3_data_type;
						} aes3;
					} format_u;
				} aaf;

				struct __attribute__ ((packed)) { 		// AVTP COMPRESSED VIDEO SUBTYPE (per P1722-rev1/D13)
					avb_u8 format;
					avb_u8 subtype;
					union {
						struct __attribute__ ((packed)) avdecc_format_cvf_mjpeg_t {		// MJPEG
							avb_u8 rsvd1:7;
							avb_u8 p:1;
							avb_u8 rsvd2;
							avb_u8 type;
							avb_u8 width;
							avb_u8 height;
						} mjpeg;

						struct __attribute__ ((packed)) avdecc_format_cvf_h264_t {		// H.264
							avb_u8 spec_version; /* normally defined as rsvd1. Used to differentiate the IEEE1722 specification version for now */
							avb_u32 rsvd2;
						} h264;

						struct __attribute__ ((packed)) avdecc_format_cvf_jpeg2000_t {		// JPEG2000
							avb_u8 rsvd1:7;
							avb_u8 p:1;
							avb_u32 rsvd2;
						} jpeg2000;
					} format_u;
				} cvf;

				struct __attribute__ ((packed)) avdecc_format_crf_t {		// AVTP CRF SUBTYPE
					avb_u8 timestamp_interval_msb:4;
					avb_u8 type:4;

					avb_u8 timestamp_interval_lsb;

					avb_u8 timestamps_per_pdu;

					avb_u32 base_frequency_msb:5;
					avb_u32 pull:3;
					avb_u32 base_frequency_lsb:24;
				} crf;

				struct __attribute__ ((packed)) {
					avb_u8 rsvd1:7;
					avb_u8 m:1;

					avb_u16 rsvd2;

					avb_u8 type_3:7;
					avb_u8 t3v:1;

					avb_u8 type_2:7;
					avb_u8 t2v:1;

					avb_u8 type_1:7;
					avb_u8 t1v:1;

					avb_u8 type_0:7;
					avb_u8 t0v:1;
				} tscf;
			} subtype_u;
#endif
		} s;
		avb_u8 raw[8];
	} u;
};

#define AVDECC_FMT_ERROR	((unsigned int)-1)

unsigned int avdecc_fmt_sample_rate(const struct avdecc_format *format);
unsigned int avdecc_fmt_hdr_size(const struct avdecc_format *format);
unsigned int __avdecc_fmt_payload_size(const struct avdecc_format *format, sr_class_t sr_class, unsigned int *max_frame_size, unsigned int *max_interval_frames);
unsigned int __avdecc_fmt_samples_per_packet(const struct avdecc_format *format, sr_class_t sr_class, unsigned int *max_interval_frames);
unsigned int avdecc_fmt_samples_per_packet(const struct avdecc_format *format, sr_class_t sr_class);
unsigned int avdecc_fmt_samples_per_timestamp(const struct avdecc_format *format, sr_class_t sr_class);
unsigned int samples_per_interval(unsigned int rate, sr_class_t sr_class);

#ifdef __BIG_ENDIAN__

#define AAF_PCM_CHANNELS_PER_FRAME_INIT(val)			.channels_per_frame = (val)

#define AVDECC_FMT_AAF_PCM_CHANNELS_PER_FRAME(fmt)		((fmt)->u.s.subtype_u.aaf.format_u.pcm.channels_per_frame)
#define AVDECC_FMT_AAF_PCM_CHANNELS_PER_FRAME_SET(fmt, val)	((fmt)->u.s.subtype_u.aaf.format_u.pcm.channels_per_frame = (val))

#define AAF_PCM_SAMPLES_PER_FRAME_INIT(val)			.samples_per_frame = (val)

#define AVDECC_FMT_AAF_PCM_SAMPLES_PER_FRAME(fmt)		((fmt)->u.s.subtype_u.aaf.format_u.pcm.samples_per_frame)
#define AVDECC_FMT_AAF_PCM_SAMPLES_PER_FRAME_SET(fmt, val)	((fmt)->u.s.subtype_u.aaf.format_u.pcm.samples_per_frame = (val))

#define AAF_AES3_STREAMS_PER_FRAME_INIT(val)			.streams_per_frame = (val)

#define AVDECC_FMT_AAF_AES3_STREAMS_PER_FRAME(fmt)		((fmt)->u.s.subtype_u.aaf.format_u.aes3.streams_per_frame)
#define AVDECC_FMT_AAF_AES3_STREAMS_PER_FRAME_SET(fmt, val)	((fmt)->u.s.subtype_u.aaf.format_u.aes3.streams_per_frame = (val))


#define AVDECC_FMT_CRF_BASE_FREQUENCY(fmt)			((fmt)->u.s.subtype_u.crf.base_frequency)
#define AVDECC_FMT_CRF_BASE_FREQUENCY_SET(fmt, val)		((fmt)->u.s.subtype_u.crf.base_frequency = (val))
#define CRF_BASE_FREQUENCY_INIT(val)				.base_frequency = htonl(val)

#else

#define AAF_PCM_CHANNELS_PER_FRAME_INIT(val)			.channels_per_frame_msb = (val) >> 2, .channels_per_frame_lsb = (val) & 0x3

#define AVDECC_FMT_AAF_PCM_CHANNELS_PER_FRAME(fmt)		(((fmt)->u.s.subtype_u.aaf.format_u.pcm.channels_per_frame_msb << 2) | (fmt)->u.s.subtype_u.aaf.format_u.pcm.channels_per_frame_lsb)
#define AVDECC_FMT_AAF_PCM_CHANNELS_PER_FRAME_SET(fmt, val)	do {(fmt)->u.s.subtype_u.aaf.format_u.pcm.channels_per_frame_msb = (val) >> 2; (fmt)->u.s.subtype_u.aaf.format_u.pcm.channels_per_frame_lsb = (val) & 0x3; } while(0)

#define AAF_PCM_SAMPLES_PER_FRAME_INIT(val)			.samples_per_frame_msb = (val) >> 4, .samples_per_frame_lsb = (val) & 0xf

#define AVDECC_FMT_AAF_PCM_SAMPLES_PER_FRAME(fmt)		(((fmt)->u.s.subtype_u.aaf.format_u.pcm.samples_per_frame_msb << 4) |(fmt)->u.s.subtype_u.aaf.format_u.pcm.samples_per_frame_lsb)
#define AVDECC_FMT_AAF_PCM_SAMPLES_PER_FRAME_SET(fmt, val)	do {(fmt)->u.s.subtype_u.aaf.format_u.pcm.samples_per_frame_msb = (val) >> 4; (fmt)->u.s.subtype_u.aaf.format_u.pcm.samples_per_frame_lsb = (val) & 0xf; } while(0)

#define AAF_AES3_STREAMS_PER_FRAME_INIT(val)			.streams_per_frame_msb = (val) >> 2, .streams_per_frame_lsb = (val) & 0x3

#define AVDECC_FMT_AAF_AES3_STREAMS_PER_FRAME(fmt)		(((fmt)->u.s.subtype_u.aaf.format_u.aes3.streams_per_frame_msb << 2) |(fmt)->u.s.subtype_u.aaf.format_u.aes3.streams_per_frame_lsb)
#define AVDECC_FMT_AAF_AES3_STREAMS_PER_FRAME_SET(fmt, val)	do {(fmt)->u.s.subtype_u.aaf.format_u.aes3.streams_per_frame_msb = (val) >> 2; (fmt)->u.s.subtype_u.aaf.format_u.aes3.streams_per_frame_lsb = (val) & 0x3; } while(0)

#define AVDECC_FMT_CRF_BASE_FREQUENCY(fmt)			ntohl(((fmt)->u.s.subtype_u.crf.base_frequency_lsb << 8) | (fmt)->u.s.subtype_u.crf.base_frequency_msb)
#define AVDECC_FMT_CRF_BASE_FREQUENCY_SET(fmt, val)		do { unsigned int v = htonl(val); (fmt)->u.s.subtype_u.crf.base_frequency_msb = v & 0xff; (fmt)->u.s.subtype_u.crf.base_frequency_lsb = v >> 8; } while(0)

#define CRF_BASE_FREQUENCY_INIT(val)				.base_frequency_lsb = htonl(val) >> 8, .base_frequency_msb = htonl(val) & 0xff

#endif

#define AVDECC_FMT_CRF_TIMESTAMP_INTERVAL(fmt)			(((fmt)->u.s.subtype_u.crf.timestamp_interval_msb << 8) | (fmt)->u.s.subtype_u.crf.timestamp_interval_lsb)
#define AVDECC_FMT_CRF_TIMESTAMP_INTERVAL_SET(fmt, val)		do { (fmt)->u.s.subtype_u.crf.timestamp_interval_msb = (val) >> 8;  (fmt)->u.s.subtype_u.crf.timestamp_interval_lsb = (val) & 0xff; } while(0)
#define CRF_TIMESTAMP_INTERVAL_INIT(val)			.timestamp_interval_msb = (val) >> 8, .timestamp_interval_lsb = (val) & 0xff

#define AVDECC_FMT_61883_6_FDF_EVT(fmt)				((fmt)->u.s.subtype_u.iec61883.format_u.iec61883_6.fdf_u.fdf.evt)

static inline unsigned int avdecc_fmt_is_raw_audio(const struct avdecc_format *format)
{
	switch (format->u.s.subtype) {
	case AVTP_SUBTYPE_61883_IIDC:
		if (format->u.s.subtype_u.iec61883.sf == IEC_61883_SF_IIDC)
			return 0;
		else
			switch (format->u.s.subtype_u.iec61883.fmt) {
			case IEC_61883_CIP_FMT_6:	// For AVDECC formats, fdf != IEC_61883_6_FDF_NODATA should be a valid assumption
				return 1;
			case IEC_61883_CIP_FMT_4:
				return 0;
			case IEC_61883_CIP_FMT_8:
				return 0;
			default:
				return 0;
			}
		break;
#ifdef CFG_AVTP_1722A
	case AVTP_SUBTYPE_AAF:
		switch (format->u.s.subtype_u.aaf.format) {
		case AAF_FORMAT_FLOAT_32BIT:
		case AAF_FORMAT_INT_32BIT:
		case AAF_FORMAT_INT_24BIT:
		case AAF_FORMAT_INT_16BIT:
			return 1;
		default:
			return 0;
		}
#endif
	default:
		return 0;
	}
}

static inline unsigned int avdecc_format_is_61883_4(const struct avdecc_format *format)
{
	return (format->u.s.subtype == AVTP_SUBTYPE_61883_IIDC) && (format->u.s.subtype_u.iec61883.sf != IEC_61883_SF_IIDC) && (format->u.s.subtype_u.iec61883.fmt == IEC_61883_CIP_FMT_4);
}

static inline unsigned int avdecc_format_is_61883_6(const struct avdecc_format *format)
{
	return (format->u.s.subtype == AVTP_SUBTYPE_61883_IIDC) && (format->u.s.subtype_u.iec61883.sf != IEC_61883_SF_IIDC) && (format->u.s.subtype_u.iec61883.fmt == IEC_61883_CIP_FMT_6);
}

static inline unsigned int avdecc_format_is_aaf_pcm(const struct avdecc_format *format)
{
	return (format->u.s.subtype == AVTP_SUBTYPE_AAF) &&
		((format->u.s.subtype_u.aaf.format == AAF_FORMAT_FLOAT_32BIT) ||
		 (format->u.s.subtype_u.aaf.format == AAF_FORMAT_INT_32BIT) ||
		 (format->u.s.subtype_u.aaf.format == AAF_FORMAT_INT_24BIT) ||
		 (format->u.s.subtype_u.aaf.format == AAF_FORMAT_INT_16BIT));
}

static inline unsigned int avdecc_format_is_crf(const struct avdecc_format *format)
{
	return (format->u.s.subtype == AVTP_SUBTYPE_CRF);
}

static inline unsigned int avdecc_format_is_aaf_aes3(const struct avdecc_format *format)
{
	return (format->u.s.subtype == AVTP_SUBTYPE_AAF) && (format->u.s.subtype_u.aaf.format == AAF_FORMAT_AES3_32BIT);
}

static inline unsigned int avdecc_format_is_cvf_h264(const struct avdecc_format *format)
{
	return (format->u.s.subtype == AVTP_SUBTYPE_CVF) && (format->u.s.subtype_u.cvf.subtype == CVF_FORMAT_SUBTYPE_H264);
}

static inline unsigned int avdecc_fmt_bits_per_sample(const struct avdecc_format *format)
{
	switch (format->u.s.subtype) {
	case AVTP_SUBTYPE_61883_IIDC:
		if (format->u.s.subtype_u.iec61883.sf == IEC_61883_SF_IIDC)
			return 0;
		switch (format->u.s.subtype_u.iec61883.fmt) {
		case IEC_61883_CIP_FMT_6:
			return 32;
		case IEC_61883_CIP_FMT_4:
		case IEC_61883_CIP_FMT_8:
		default:
			return 0;
		}
#ifdef CFG_AVTP_1722A
	case AVTP_SUBTYPE_AAF:
		switch(format->u.s.subtype_u.aaf.format) {
		case AAF_FORMAT_AES3_32BIT:
		case AAF_FORMAT_FLOAT_32BIT:
		case AAF_FORMAT_INT_32BIT:
			return 32;

		case AAF_FORMAT_INT_24BIT:
			return 24;

		case AAF_FORMAT_INT_16BIT:
			return 16;

		default:
			return 0;
		}
#endif
	default:
		return 0;
	}
}

static inline unsigned int avdecc_fmt_unused_bits(const struct avdecc_format *format)
{
	switch (format->u.s.subtype) {
	case AVTP_SUBTYPE_61883_IIDC:
		if (format->u.s.subtype_u.iec61883.sf == IEC_61883_SF_IIDC)
			return 0;
		switch (format->u.s.subtype_u.iec61883.fmt) {
		case IEC_61883_CIP_FMT_6:
			switch (format->u.s.subtype_u.iec61883.format_u.iec61883_6.fdf_u.fdf.evt) {
			case IEC_61883_6_FDF_EVT_AM824:
				return 8;
			case IEC_61883_6_FDF_EVT_FLOATING:
			case IEC_61883_6_FDF_EVT_INT32:
			default:
				return 0;
			}
		case IEC_61883_CIP_FMT_4:
		case IEC_61883_CIP_FMT_8:
		default:
			return 0;
		}
#ifdef CFG_AVTP_1722A
	case AVTP_SUBTYPE_AAF:
		switch (format->u.s.subtype_u.aaf.format) {
		case AAF_FORMAT_INT_32BIT:
		case AAF_FORMAT_INT_24BIT:
		case AAF_FORMAT_INT_16BIT:
			return avdecc_fmt_bits_per_sample(format) - format->u.s.subtype_u.aaf.format_u.pcm.bit_depth;

		default:
		case AAF_FORMAT_FLOAT_32BIT:
		case AAF_FORMAT_AES3_32BIT:
			return 0;
		}
#endif
	default:
		return 0;
	}
}

static inline unsigned int avdecc_fmt_audio_is_float(const struct avdecc_format *format)
{
	switch (format->u.s.subtype) {
	case AVTP_SUBTYPE_61883_IIDC:
		if (format->u.s.subtype_u.iec61883.sf == IEC_61883_SF_IIDC)
			return 0;
		else
			switch (format->u.s.subtype_u.iec61883.fmt) {
			case IEC_61883_CIP_FMT_6:	// For AVDECC formats, fdf != IEC_61883_6_FDF_NODATA should be a valid assumption
				switch (format->u.s.subtype_u.iec61883.format_u.iec61883_6.fdf_u.fdf.evt) {
				case IEC_61883_6_FDF_EVT_FLOATING:
					return 1;
				case IEC_61883_6_FDF_EVT_AM824:
				case IEC_61883_6_FDF_EVT_INT32:
				default:
					return 0;
				}
			case IEC_61883_CIP_FMT_4:
			case IEC_61883_CIP_FMT_8:
			default:
				return 0;
			}
		break;
#ifdef CFG_AVTP_1722A
	case AVTP_SUBTYPE_AAF:
		if (format->u.s.subtype_u.aaf.format == AAF_FORMAT_FLOAT_32BIT)
			return 1;

		return 0;
#endif
	default:
		return 0;
	}
}

static inline unsigned int avdecc_fmt_channels_per_sample(const struct avdecc_format *format)
{
	switch (format->u.s.subtype) {
	case AVTP_SUBTYPE_61883_IIDC:
		if (format->u.s.subtype_u.iec61883.sf == IEC_61883_SF_IIDC)
			return 0;

		switch (format->u.s.subtype_u.iec61883.fmt) {
		case IEC_61883_CIP_FMT_6:
			switch (format->u.s.subtype_u.iec61883.format_u.iec61883_6.fdf_u.fdf.evt) {
			case IEC_61883_6_FDF_EVT_AM824:
				return format->u.s.subtype_u.iec61883.format_u.iec61883_6.label_mbla_cnt;  // For now only assume audio is MBLA only
			case IEC_61883_6_FDF_EVT_FLOATING:
			case IEC_61883_6_FDF_EVT_INT32:
				return format->u.s.subtype_u.iec61883.format_u.iec61883_6.dbs == 0?256:format->u.s.subtype_u.iec61883.format_u.iec61883_6.dbs;
			default:
				return 0;
			}
		case IEC_61883_CIP_FMT_4:
		case IEC_61883_CIP_FMT_8:
		default:
			return 0;
		}

		break;
#ifdef CFG_AVTP_1722A
	case AVTP_SUBTYPE_AAF:
		if (avdecc_format_is_aaf_pcm(format))
			return AVDECC_FMT_AAF_PCM_CHANNELS_PER_FRAME(format);
		else if (avdecc_format_is_aaf_aes3(format))
			return AVDECC_FMT_AAF_AES3_STREAMS_PER_FRAME(format);
		else
			return 0;

	case AVTP_SUBTYPE_CRF:
		return 1;

#endif
	default:
		return 0;
	}
}

static inline unsigned int avdecc_fmt_sample_size(const struct avdecc_format *format)
{
	switch (format->u.s.subtype) {
	case AVTP_SUBTYPE_61883_IIDC:
		if (format->u.s.subtype_u.iec61883.sf == IEC_61883_SF_IIDC)
			return 0;
		switch (format->u.s.subtype_u.iec61883.fmt) {
		case IEC_61883_CIP_FMT_4:
			return IEC_61883_4_SP_PAYLOAD_SIZE;

		case IEC_61883_CIP_FMT_6:
			return (avdecc_fmt_bits_per_sample(format) * avdecc_fmt_channels_per_sample(format)) >> 3;

		case IEC_61883_CIP_FMT_8:
		default:
			return 0;
		}
#ifdef CFG_AVTP_1722A
	case AVTP_SUBTYPE_CVF:
		if (format->u.s.subtype_u.cvf.format == CVF_FORMAT_RFC)
			return 1;
		else
			return 0;

	case AVTP_SUBTYPE_AAF:
		return avdecc_fmt_bits_per_sample(format) * avdecc_fmt_channels_per_sample(format) >> 3;

	case AVTP_SUBTYPE_CRF:
		return 8; /* sample == 64bit timestamp */

	case AVTP_SUBTYPE_TSCF:
		return 1;
#endif
	default:
		return 0;
	}
}

/* Computation of tspec
 * As per 802.1Q Table 35-5 + 1 byte from Milan Baseline Interoperability 6.3.2
 */
static inline void avdecc_fmt_tspec(const struct avdecc_format *format, sr_class_t sr_class, unsigned int *max_frame_size, unsigned int *max_interval_frames)
{
	__avdecc_fmt_payload_size(format, sr_class, max_frame_size, max_interval_frames);

	*max_frame_size += avdecc_fmt_hdr_size(format) + 1;

	if (is_avtp_stream(format->u.s.subtype))
		*max_frame_size += sizeof(struct avtp_data_hdr);
	else if (format->u.s.subtype == AVTP_SUBTYPE_CRF)
		*max_frame_size += sizeof(struct avtp_crf_hdr);
	else
		; /* FIXME handle other formats */
}

#endif /* _GENAVB_PUBLIC_AVDECC_H_ */
