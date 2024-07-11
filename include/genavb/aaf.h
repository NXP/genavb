/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file aaf.h
 \brief GenAVB public API
 \details AAF (AVTP Audio Format) header definitions.

 \copyright Copyright 2016 Freescale Semiconductor, Inc.
 \copyright Copyright 2016-2017, 2023 NXP
*/
#ifndef _GENAVB_PUBLIC_AAF_H_
#define _GENAVB_PUBLIC_AAF_H_

#include "types.h"

/**
* \defgroup aaf			AAF
* AVTP Audio format as defined in IEEE 1722-2016, section 7
* \ingroup avtp
*/

/** IEEE 1722-2016, section 7, Table 9
 * \ingroup aaf
 */
enum aaf_format {
	AAF_FORMAT_USER = 0,
	AAF_FORMAT_FLOAT_32BIT = 1,	/**< 32bit Floating Point PCM */
	AAF_FORMAT_INT_32BIT = 2,	/**< 32bit Integer PCM */
	AAF_FORMAT_INT_24BIT = 3,	/**< 24bit Integer PCM */
	AAF_FORMAT_INT_16BIT = 4,	/**< 16bit Integer PCM */
	AAF_FORMAT_AES3_32BIT = 5	/**< 32bit AES3 */
};

/**
 * \ingroup aaf
 * @{
 */
#define AAF_SP_NORMAL		0
#define AAF_SP_SPARSE		1

/** IEEE 1722-2016, section 7, Table 11
 * \ingroup aaf
 */
enum aaf_nsr {
	AAF_NSR_USER_SPECIFIED = 0,
	AAF_NSR_8000 = 1,		/**< 8 KHz */
	AAF_NSR_16000 = 2,		/**< 16 KHz */
	AAF_NSR_32000 = 3,		/**< 32 KHz */
	AAF_NSR_44100 = 4,		/**< 44.1 KHz */
	AAF_NSR_48000 = 5,		/**< 48 KHz */
	AAF_NSR_88200 = 6,		/**< 88.2 KHz */
	AAF_NSR_96000 = 7,		/**< 96 KHz */
	AAF_NSR_176400 = 8,		/**< 176.4 KHz */
	AAF_NSR_192000 = 9,		/**< 192 KHz */
	AAF_NSR_24000 = 10,		/**< 24 KHz */
	AAF_NSR_RESERVED1 = 11,		/**< Reserved */
	AAF_NSR_RESERVED2 = 12,		/**< Reserved */
	AAF_NSR_RESERVED3 = 13,		/**< Reserved */
	AAF_NSR_RESERVED4 = 14,		/**< Reserved */
	AAF_NSR_RESERVED5 = 15,		/**< Reserved */
	AAF_NSR_MAX = 15
};

#define AAF_AES3_DT_UNSPECIFIED	0
#define AAF_AES3_DT_PCM		1
#define AAF_AES3_DT_SMPTE338	2
#define AAF_AES3_DT_IEC61937	3
#define AAF_AES3_DT_VENDOR	4

#define AAF_PACKETS_PER_TIMESTAMP_SPARSE	8
#define AAF_PACKETS_PER_TIMESTAMP_NORMAL	1

/**
 * \ingroup aaf
 */
struct __attribute__ ((packed)) avtp_aaf_hdr {
#ifdef __BIG_ENDIAN__
	avb_u8 subtype;

	avb_u8 sv:1;
	avb_u8 version:3;
	avb_u8 mr:1;
	avb_u8 rsv:2;
	avb_u8 tv:1;

	avb_u8 sequence_num;

	avb_u8 reserved:7;
	avb_u8 tu:1;
#else
	avb_u8 subtype;

	avb_u8 tv:1;
	avb_u8 rsv:2;
	avb_u8 mr:1;
	avb_u8 version:3;
	avb_u8 sv:1;

	avb_u8 sequence_num;

	avb_u8 tu:1;
	avb_u8 reserved:7;
#endif

	avb_u64 stream_id;
	avb_u32 avtp_timestamp;

#ifdef __BIG_ENDIAN__
	avb_u32 format:8;
	avb_u32 aaf_format_specific_data1:24;
#else
	avb_u8 format;
	avb_u8 aaf_format_specific_data1_msb;
	avb_u16 aaf_format_specific_data1_lsb;
#endif

	avb_u16 stream_data_length;

#ifdef __BIG_ENDIAN__
	avb_u8 afsd:3;
	avb_u8 sp:1;
	avb_u8 evt:4;
#else
	avb_u8 evt:4;
	avb_u8 sp:1;
	avb_u8 afsd:3;
#endif

	avb_u8 aaf_format_specific_data_2;
};

/**
 * \ingroup aaf
 */
struct __attribute__ ((packed)) avtp_aaf_pcm_hdr {
#ifdef __BIG_ENDIAN__
	avb_u8 subtype;

	avb_u8 sv:1;
	avb_u8 version:3;
	avb_u8 mr:1;
	avb_u8 rsv:2;
	avb_u8 tv:1;

	avb_u8 sequence_num;

	avb_u8 reserved:7;
	avb_u8 tu:1;
#else
	avb_u8 subtype;

	avb_u8 tv:1;
	avb_u8 rsv:2;
	avb_u8 mr:1;
	avb_u8 version:3;
	avb_u8 sv:1;

	avb_u8 sequence_num;

	avb_u8 tu:1;
	avb_u8 reserved:7;
#endif

	avb_u64 stream_id;
	avb_u32 avtp_timestamp;

#ifdef __BIG_ENDIAN__
	avb_u32 format:8;
	avb_u32 nsr:4;
	avb_u32 rsv1:2;
	avb_u32 channels_per_frame:10;
	avb_u32 bit_depth:8;
#else
	avb_u8 format;

	avb_u8 channels_per_frame_msb:2;
	avb_u8 rsv1:2;
	avb_u8 nsr:4;

	avb_u8 channels_per_frame_lsb;

	avb_u8 bit_depth;
#endif
	avb_u16 stream_data_length;

#ifdef __BIG_ENDIAN__
	avb_u8 rsv2:3;
	avb_u8 sp:1;
	avb_u8 evt:4;
#else
	avb_u8 evt:4;
	avb_u8 sp:1;
	avb_u8 rsv2:3;
#endif

	avb_u8 reserved1;
};

#ifdef __BIG_ENDIAN__
#define AAF_PCM_CHANNELS_PER_FRAME(hdr)			((hdr)->channels_per_frame)
#define AAF_PCM_CHANNELS_PER_FRAME_SET(hdr, val)	((hdr)->channels_per_frame = (val))
#else
#define AAF_PCM_CHANNELS_PER_FRAME(hdr)			(((hdr)->channels_per_frame_msb << 8) | (hdr)->channels_per_frame_lsb)
#define AAF_PCM_CHANNELS_PER_FRAME_SET(hdr, val)	do {(hdr)->channels_per_frame_msb = (val) >> 8; (hdr)->channels_per_frame_lsb = (val) & 0xff; } while(0)
#endif

/**
 * \ingroup aaf
 */
struct __attribute__ ((packed)) avtp_aaf_aes3_hdr {
#ifdef __BIG_ENDIAN__
	avb_u8 subtype;

	avb_u8 sv:1;
	avb_u8 version:3;
	avb_u8 mr:1;
	avb_u8 rsv:2;
	avb_u8 tv:1;

	avb_u8 sequence_num;

	avb_u8 reserved:7;
	avb_u8 tu:1;
#else
	avb_u8 subtype;

	avb_u8 tv:1;
	avb_u8 rsv:2;
	avb_u8 mr:1;
	avb_u8 version:3;
	avb_u8 sv:1;

	avb_u8 sequence_num;

	avb_u8 tu:1;
	avb_u8 reserved:7;
#endif

	avb_u64 stream_id;
	avb_u32 avtp_timestamp;

#ifdef __BIG_ENDIAN__
	avb_u32 format:8;
	avb_u32 nfr:4;
	avb_u32 rsv1:2;
	avb_u32 streams_per_frame:10;
	avb_u32 aes3_data_type_h:8;
#else
	avb_u8 format;

	avb_u8 streams_per_frame_msb:2;
	avb_u8 rsv1:2;
	avb_u8 nfr:4;

	avb_u8 streams_per_frame_lsb;

	avb_u8 aes3_data_type_h;
#endif
	avb_u16 stream_data_length;

#ifdef __BIG_ENDIAN__
	avb_u8 aes3_dt_ref:3;
	avb_u8 sp:1;
	avb_u8 evt:4;
#else
	avb_u8 evt:4;
	avb_u8 sp:1;
	avb_u8 aes3_dt_ref:3;
#endif

	avb_u8 aes3_data_type_l;
};

#ifdef __BIG_ENDIAN__
#define AAF_AES3_STREAMS_PER_FRAME(hdr)			((hdr)->streams_per_frame)
#define AAF_AES3_STREAMS_PER_FRAME_SET(hdr, val)		((hdr)->streams_per_frame = (val))
#else
#define AAF_AES3_STREAMS_PER_FRAME(hdr)			(((hdr)->streams_per_frame_msb << 8) | (hdr)->streams_per_frame_lsb)
#define AAF_AES3_STREAMS_PER_FRAME_SET(hdr, val)	do {(hdr)->streams_per_frame_msb = (val) >> 8; (hdr)->streams_per_frame_lsb = (val) & 0xff; } while(0)
#endif

#define AAF_AES3_DATA_TYPE(hdr)				(((hdr)->aes3_data_type_h << 8) | (hdr)->aes3_data_type_l)
#define AAF_AES3_DATA_TYPE_SET(hdr, val)		do {(hdr)->aes3_data_type_h = (val) >> 8; (hdr)->aes3_data_type_l = (val) & 0xff; } while(0)

/** @} */

#endif /* GENAVB_PUBLIC_AAF_H */
