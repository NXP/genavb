/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file crf.h
 \brief GenAVB public API
 \details CRF (AVTP Clock Reference Format) header definitions.

 \copyright Copyright 2016 Freescale Semiconductor, Inc.
 \copyright Copyright 2016-2017, 2023 NXP
*/
#ifndef _GENAVB_PUBLIC_CRF_H_
#define _GENAVB_PUBLIC_CRF_H_

#include "types.h"

/**
* \defgroup crf			CRF
* Clock reference format as defined in IEEE 1722-2016, section 10
* \ingroup avtp
**/

/** CRF Type as defined in IEEE 1722-2016, section 10, Table 26
 * \ingroup crf
 */
enum crf_type {
	CRF_TYPE_USER = 0,		/**< User Specified */
	CRF_TYPE_AUDIO_SAMPLE = 1,	/**< Audio Sample Timestamp */
	CRF_TYPE_VIDEO_FRAME = 2,	/**< Video Frame Sync Timestamp */
	CRF_TYPE_VIDEO_LINE = 3,	/**< Video Line Timestamp */
	CRF_TYPE_MACHINE_CYCLE = 4	/**< Machine Cycle Timestamp */
};

/** CRF Pull as defined in IEEE 1722-2016, section 10, Table 27
 * \ingroup crf
 */
enum crf_pull {
	CRF_PULL_1_1 = 0,	/**< 1.0 */
	CRF_PULL_1000_1001 = 1,	/**< 1/1.001 */
	CRF_PULL_1001_1000 = 2,	/**< 1.001 */
	CRF_PULL_24_25 = 3,	/**< 24/25 */
	CRF_PULL_25_24 = 4,	/**< 25/24 */
	CRF_PULL_1_8 = 5	/**< 1/8 */
};

/**
 * \ingroup crf
 * @{
 */
struct __attribute__ ((packed)) avtp_crf_hdr {
	avb_u8 subtype;

#ifdef __BIG_ENDIAN__
	avb_u8 sv:1;
	avb_u8 version:3;
	avb_u8 mr:1;
	avb_u8 r:1;
	avb_u8 fs:1;
	avb_u8 tu:1;
#else
	avb_u8 tu:1;
	avb_u8 fs:1;
	avb_u8 r:1;
	avb_u8 mr:1;
	avb_u8 version:3;
	avb_u8 sv:1;
#endif

	avb_u8 sequence_num;

	avb_u8 type;

	avb_u64 stream_id;

#ifdef __BIG_ENDIAN__
	avb_u32 pull:3;
	avb_u32 base_frequency:29;
#else
	avb_u32 base_frequency_msb:5;
	avb_u32 pull:3;
	avb_u32 base_frequency_lsb:24;
#endif

	avb_u16 crf_data_length;
	avb_u16 timestamp_interval;
};


#ifdef __BIG_ENDIAN__
#define CRF_BASE_FREQUENCY(hdr)			((hdr)->base_frequency)
#define CRF_BASE_FREQUENCY_SET(hdr, val)	((hdr)->base_frequency = (val))
#else
#define CRF_BASE_FREQUENCY(hdr)			ntohl(((hdr)->base_frequency_lsb << 8) | (hdr)->base_frequency_msb)
#define CRF_BASE_FREQUENCY_SET(hdr, val)	do { unsigned int v = htonl(val); (hdr)->base_frequency_msb = v & 0xff; (hdr)->base_frequency_lsb = v >> 8; } while(0)
#endif

#define CRF_TIMESTAMP_INTERVAL_MIN	1
#define CRF_TIMESTAMP_INTERVAL_MAX	640	/* Not standard, matches audio clock at 192KHz and CRF at 300Hz */

#define CRF_TIMESTAMPS_PER_PDU_MIN	1
#define CRF_TIMESTAMPS_PER_PDU_MAX	8	/* Not standard, 512 payload bytes */
/** @} */
#endif /* GENAVB_PUBLIC_CRF_H */
