/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file 61883_iidc.h
 \brief GenAVB public API
 \details 61883 header definitions.

 \copyright Copyright 2014-2016 Freescale Semiconductor, Inc.
 \copyright Copyright 2016-2017, 2023 NXP
*/
#ifndef _GENAVB_PUBLIC_61883_IIDC_H_
#define _GENAVB_PUBLIC_61883_IIDC_H_

#include "types.h"

/**
* \defgroup iec_61883_iidc	IEC 61883/IIDC
* IEC 61883/IIDC format as defined in IEEE 1722-2016, section 5
* \ingroup avtp
*/

/**
 * \ingroup iec_61883_iidc
 */
struct iec_61883_iidc_specific_hdr {
	union {
		avb_u16 raw;
		struct {
#ifdef __BIG_ENDIAN__
			avb_u8 tag:2;
			avb_u8 channel:6;
			avb_u8 tcode:4;
			avb_u8 sy:4;
#else
			avb_u8 channel:6;
			avb_u8 tag:2;
			avb_u8 sy:4;
			avb_u8 tcode:4;
#endif
		} s;
	} u;
};

/**
 * \ingroup iec_61883_iidc
 * @{
 */
#define IEC_61883_PSH_TAG_CIP_INCLUDED		1
#define IEC_61883_PSH_CHANNEL_AVB_NETWORK	31
#define IEC_61883_PSH_TCODE_AVTP		0xa

#define IEC_61883_SF_IIDC 			0	/**< IIDC Stream Format */
#define IEC_61883_SF_61883 			1	/**< IEC 61883 Stream Format */
/** @} */

/**
* \defgroup iec_61883		IEC 61883
* \ingroup iec_61883_iidc
*/

/**
 * \ingroup iec_61883
 */
struct iec_61883_hdr {
#ifdef __BIG_ENDIAN__
	avb_u8 rsvd1:2;
	avb_u8 sid:6;

	avb_u8 dbs;

	avb_u8 fn:2;
	avb_u8 qpc:3;
	avb_u8 sph:1;
	avb_u8 rsvd2:2;

	avb_u8 dbc;

	avb_u8 rsvd3:2;
	avb_u8 fmt:6;

	union {
		avb_u8 raw;
		struct {
			avb_u8 evt:5;
			avb_u8 sfc:3;
		} fdf;
	} fdf_u;

	avb_u16 syt;
#else
	avb_u8 sid:6;
	avb_u8 rsvd1:2;

	avb_u8 dbs;

	avb_u8 rsvd2:2;
	avb_u8 sph:1;
	avb_u8 qpc:3;
	avb_u8 fn:2;

	avb_u8 dbc;

	avb_u8 fmt:6;
	avb_u8 rsvd3:2;

	union {
		avb_u8 raw;
		struct {
			avb_u8 sfc:3;
			avb_u8 evt:5;
		} fdf;
	} fdf_u;

	avb_u16 syt;
#endif
};

/**
 * \ingroup iec_61883
 * @{
 */
#define IEC_61883_CIP_FMT_4	0x20	/**< IEC 61883-4 Stream Format */
#define IEC_61883_CIP_FMT_6	0x10	/**< IEC 61883-6 Stream Format */
#define IEC_61883_CIP_FMT_8	0x01

#define IEC_61883_CIP_SID_AVB_NETWORK		63
#define IEC_61883_CIP_SPH_SOURCE_PACKETS 	1
#define IEC_61883_CIP_2ND_QUAD_ID		2
#define IEC_61883_CIP_SYT_NO_TSTAMP		0xffff
/** @} */

/**
* \defgroup iec_61883_4		IEC 61883-4
* \ingroup iec_61883
*/

/**
 * \ingroup iec_61883_4
 * @{
 */
#define IEC_61883_4_CIP_DBS 			6 /**< Block size in quadlets units, 6*4=24 bytes */
#define IEC_61883_4_CIP_FN 			3 /**< Number of blocks per packet (power of 2), 8*(6*4)=192 bytes */
#define IEC_61883_4_CIP_SPH 			1 /**< Always set for 61883-4 */
#define IEC_61883_4_SP_SIZE			(IEC_61883_4_CIP_DBS << (2 + IEC_61883_4_CIP_FN)) /**< Size of a source packet in bytes */
#define IEC_61883_4_SP_PAYLOAD_SIZE		(IEC_61883_4_SP_SIZE - 4) /**< Size of a source packet payload (i.e. MPEG2-TS packet size) */
/** @} */

/**
* \defgroup iec_61883_6		IEC 61883-6
* \ingroup iec_61883
*/

/**
 * \ingroup iec_61883_6
 * @{
 */
#define IEC_61883_6_FDF_NODATA 			0xff
#define IEC_61883_6_FDF_EVT_AM824 		0x00	/**< IEC 61883-6 AM824 Stream Format */
#define IEC_61883_6_FDF_EVT_PACKED 		0x02
#define IEC_61883_6_FDF_EVT_FLOATING 		0x04	/**< IEC 61883-6 Float Stream Format */
#define IEC_61883_6_FDF_EVT_INT32 		0x06	/**< IEC 61883-6 32-bit Stream Format */
/** @} */

/** IEC 61883-6 sample rates
* \ingroup iec_61883_6
*/
enum iec_61883_6_fdf_sfc {
	IEC_61883_6_FDF_SFC_32000 = 0x00,	/**< 32 KHz */
	IEC_61883_6_FDF_SFC_44100 = 0x01,	/**< 44.1 KHz */
	IEC_61883_6_FDF_SFC_48000 = 0x02,	/**< 48 KHz */
	IEC_61883_6_FDF_SFC_88200 = 0x03,	/**< 88.2 KHz */
	IEC_61883_6_FDF_SFC_96000 = 0x04,	/**< 96 KHz */
	IEC_61883_6_FDF_SFC_176400 = 0x05,	/**< 176.4 KHz */
	IEC_61883_6_FDF_SFC_192000 = 0x06,	/**< 192 KHz */
	IEC_61883_6_FDF_SFC_RSVD = 0x07,
	IEC_61883_6_FDF_SFC_MAX = 0x07
};

#endif /* _GENAVB_PUBLIC_61883_IIDC_H_ */
