/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2020-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief GenAVB/TSN public API
 \details net_types header definitions.

 \copyright Copyright 2014-2016 Freescale Semiconductor, Inc.
 \copyright Copyright 2016-2018, 2020-2024 NXP
*/
#ifndef _GENAVB_PUBLIC_NET_TYPES_H_
#define _GENAVB_PUBLIC_NET_TYPES_H_

#include "config.h"
#include "types.h"
#include "os/net_types.h"

#define NET_RX_BATCH	16
#define NET_TX_BATCH	32

#define NET_TX_FLAGS_HW_TS	(1 << 0)
#define NET_TX_FLAGS_HW_CSUM	(1 << 1)
#define NET_TX_FLAGS_TS		(1 << 2)
#define NET_TX_FLAGS_PARTIAL	(1 << 3)
#define NET_TX_FLAGS_END_FRAME	(1 << 6)
#define NET_TX_FLAGS_TS64		(1 << 7)

#define NULL_MAC {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}

#ifndef ntohs

#if defined(__BIG_ENDIAN__)

#define ntohs(x) (x)
#define ntohl(x) (x)
#define htons(x) (x)
#define htonl(x) (x)

#elif defined(__LITTLE_ENDIAN__)

#define ntohs(x) ((((x) & 0xFF00) >> 8) | (((x) & 0xFF) << 8))
#define ntohl(x) ((((x) & 0xFF000000) >> 24) | (((x) & 0xFF) << 24) | \
		(((x) & 0xFF0000) >> 8) | (((x) & 0xFF00) << 8))
#define htons(x) ((((x) & 0xFF00) >> 8) | (((x) & 0xFF) << 8))
#define htonl(x) ((((x) & 0xFF000000) >> 24) | (((x) & 0xFF) << 24) | \
		(((x) & 0xFF0000) >> 8) | (((x) & 0xFF00) << 8))
#else
	#error Machine endianness is not properly defined, it shall be either BIG or LITTLE
#endif

#endif /* ntohs */


#if defined(__BIG_ENDIAN__)

#define ntohll(x) (x)
#define htonll(x) (x)

#elif defined(__LITTLE_ENDIAN__)

#define htonll(x) (((avb_u64)htonl((avb_u32)(x)))<<32 | htonl((avb_u32)((avb_u64)(x)>>32)))
#define ntohll(x) (((avb_u64)ntohl((avb_u32)(x)))<<32 | ntohl((avb_u32)((avb_u64)(x)>>32)))

#endif /* __X_ENDIAN__ */


struct net_ts_desc {
	avb_u64 ts;
	avb_u32 priv;
};



/*  NOTE: l2_offset should be at least equal to the size of  net_rx_desc  structure
 *  These fields are common cross layers and should not be overriden while transferred */
#define NET_DESC_COMMON \
	avb_u16 l2_offset; \
	avb_u16 len; \
	avb_u32 ts; \
	avb_u32 flags:28; \
	avb_u32 pool_type:4; \
	avb_u32 priv

/** Network receive descriptor
 *
 */
struct net_rx_desc {
	NET_DESC_COMMON;
	avb_u16 port;
	avb_u16 l3_offset; /**< offset of mrp, avtp, ptp header (with vlan skipped) */
	avb_u16 l4_offset; /**< offset of udp header */
	avb_u16 l5_offset; /**< offset of rtp header (with vlan) */
	avb_u16 ethertype;
	avb_u16 vid;
	avb_u64 ts64;
};

/** Network transmit descriptor
 *
 */
struct net_tx_desc {
	NET_DESC_COMMON;
	avb_u16 port;
	avb_u64 ts64;
};

/** Network address
 * \ingroup socket
 * The protocol type determines the format of the network address
 *
 */
struct net_address {
	avb_u8 ptype;	/**< protocol type */
	avb_u16 port;	/**< port */
	avb_u16 vlan_id;	/**< vlan id (network order), one of [VLAN_VID_MIN, VLAN_VID_MAX], VLAN_VID_NONE or VLAND_ID_DEFAULT */
	avb_u8 priority;	/**< traffic priority */

	union {
		struct avtp_address {
			int subtype; /* AECP, ADP, ACMP, MAAP, 611883-IDC, ... */
			avb_u8 sr_class;
			avb_u8 stream_id[8];
		} avtp;

		struct {
			avb_u8 version;
		} ptp;

		struct {
			avb_u16 protocol;
			avb_u8 dst_mac[6];
		} l2;
	} u;
};

struct net_mc_address {
	avb_u16 port;
	avb_u8 hw_addr[6];
};

/**
 * \ingroup socket
 * Protocol types
 */
enum NET_PTYPE {
	PTYPE_NONE = 0,
	PTYPE_MRP,
	PTYPE_PTP,
	PTYPE_HSR,
	PTYPE_AVTP,
	PTYPE_OTHER, /**< Non handled by genAVB stack */
	PTYPE_L2,    /**< Layer 2 */
	PTYPE_MAX,
};

#define PORT_ANY	0xffff

#endif /* _GENAVB_PUBLIC_NET_TYPES_H_ */
