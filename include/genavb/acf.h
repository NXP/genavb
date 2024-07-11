/*
 * Copyright 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2018, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file acf.h
 \brief GenAVB public API
 \details ACF (AVTP Control Format) header definitions.

 \copyright Copyright 2015-2016 Freescale Semiconductor, Inc.
 \copyright Copyright 2016, 2018, 2023 NXP
*/
#ifndef _GENAVB_PUBLIC_ACF_H_
#define _GENAVB_PUBLIC_ACF_H_

/**
* \defgroup acf			ACF
* AVTP Control message format as defined in IEEE 1722-2016, section 9
* \ingroup avtp
*/

/** ACF message types - IEEE Std 1722-2016
* \ingroup acf
*/
enum acf_type {
	ACF_MSG_TYPE_FLEXRAY = 0x00,		/**<  FlexRay message */
	ACF_MSG_TYPE_CAN = 0x01,		/**<  Controller Area Network (CAN) / CAN Flexible Data-Rate (CAN FD) message */
	ACF_MSG_TYPE_CAN_BRIEF = 0x02,	/**<  Abbreviated CAN/CAN FD message */
	ACF_MSG_TYPE_LIN = 0x03,		/**<  LIN message */
	ACF_MSG_TYPE_MOST = 0x04,		/**<  MOST message */
	ACF_MSG_TYPE_GPC = 0x05,		/**< General Purpose Control message */
	ACF_MSG_TYPE_SERIAL = 0x06,		/**<  Serial port message */
	ACF_MSG_TYPE_PARALLEL = 0x07,	/**<  Parallel port message */
	ACF_MSG_TYPE_SENSOR = 0x08,		/**<  Analog sensor message */
	ACF_MSG_TYPE_SENSOR_BRIEF = 0x09,	/**<  Abbreviated sensor message */
	ACF_MSG_TYPE_AECP = 0x0A,		/**<  IEEE Std 1722.2 AECP message */
	ACF_MSG_TYPE_ANCILLARY = 0x0B,	/**<  Video ancillary data message */
	ACF_MSG_TYPE_USER = 0x78,		/**<  up to 0x7F  User-defined ACF message */
};

/**
 * \ingroup acf
 */
struct __attribute__ ((packed)) acf_can_hdr {
#ifdef __BIG_ENDIAN__
	avb_u8 pad:2;
	avb_u8 mtv:1;
	avb_u8 rtr:1;
	avb_u8 eff:1;
	avb_u8 brs:1;
	avb_u8 fdf:1;
	avb_u8 esi:1;

	avb_u8 rsv1:3;
	avb_u8 can_bus_id:5;

	avb_u64 message_timestamp;

	avb_u32 rsv2:3;
	avb_u32 can_identifier:29;
#else
	avb_u8 esi:1;
	avb_u8 fdf:1;
	avb_u8 brs:1;
	avb_u8 eff:1;
	avb_u8 rtr:1;
	avb_u8 mtv:1;
	avb_u8 pad:2;

	avb_u8 can_bus_id:5;
	avb_u8 rsv1:3;

	avb_u64 message_timestamp;

	avb_u32 can_identifier:29;
	avb_u32 rsv2:3;
#endif
};


/**
 * \ingroup acf
 */
struct __attribute__ ((packed)) avtp_ntscf_hdr {
#ifdef __BIG_ENDIAN__
	avb_u32 subtype:8;
	avb_u32 sv:1;
	avb_u32 version:3;
	avb_u32 r:1;
	avb_u32 ntscf_data_length_msb:11;
	avb_u32 sequence_num:8;
#else
	avb_u8 subtype;

	avb_u8 ntscf_data_length_msb:3;
	avb_u8 r:1;
	avb_u8 version:3;
	avb_u8 sv:1;

	avb_u8 ntscf_data_length_lsb;

	avb_u8 sequence_num;
#endif

	avb_u64 stream_id;
};

#ifdef __BIG_ENDIAN__
#define NTSCF_DATA_LENGTH(hdr) (hdr)->ntscf_data_length
#define NTSCF_DATA_LENGTH_SET(hdr, val) (hdr)->ntscf_data_length = (val)
#else
#define NTSCF_DATA_LENGTH(hdr) (((hdr)->ntscf_data_length_msb << 8) | (hdr)->ntscf_data_length_lsb)
#define NTSCF_DATA_LENGTH_SET(hdr, val) do { (hdr)->ntscf_data_length_msb = ((val) >> 8); (hdr)->ntscf_data_length_lsb = ((val) & 0xff); } while(0)
#endif

/**
 * \ingroup acf
 */
struct __attribute__ ((packed)) acf_msg {
#ifdef __BIG_ENDIAN__
	avb_u16 acf_msg_type:7;
	avb_u16 acf_msg_length:9;
#else
	avb_u8 acf_msg_length_msb:1;
	avb_u8 acf_msg_type:7;

	avb_u8 acf_msg_length_lsb;
#endif

};

#ifdef __BIG_ENDIAN__
#define ACF_MSG_LENGTH(acf) (acf)->acf_msg_length
#define ACF_MSG_LENGTH_SET(acf, length) (acf)->acf_msg_length = (length)

#else
#define ACF_MSG_LENGTH(acf) ((acf)->acf_msg_length_msb << 8 | (acf)->acf_msg_length_lsb)
#define ACF_MSG_LENGTH_SET(acf, length) { (acf)->acf_msg_length_msb = ((length) >> 8) & 0x1; (acf)->acf_msg_length_lsb = (length) & 0xff; } while(0)
#endif

#endif /* GENAVB_PUBLIC_ACF_H */
