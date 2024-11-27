/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2020-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file ether.h
 \brief GenAVB/TSN public API
 \details Ethernet header definitions.

 \copyright Copyright 2014-2016 Freescale Semiconductor, Inc.
 \copyright Copyright 2016-2017, 2020-2024 NXP
*/
#ifndef _GENAVB_PUBLIC_ETHER_H_
#define _GENAVB_PUBLIC_ETHER_H_

#include "types.h"

/**
 * \ingroup protocol
 */
struct eth_hdr
{
	avb_u8 dst[6];
	avb_u8 src[6];
	avb_u16 type;
};

/**
 * \ingroup protocol
 */
struct vlanhdr
{
	avb_u16 label;
	avb_u16 type;
};

/**
 * \ingroup protocol
 */
struct hsr_hdr
{
	avb_u16 label;
	avb_u16 seqnr;
	avb_u16 type;
};

#ifdef __BIG_ENDIAN__
#define VLAN_PCP_SHIFT	13
#define VLAN_CFI_SHIFT	12
#else
#define VLAN_PCP_SHIFT	5
#define VLAN_CFI_SHIFT	4
#endif

#define VLAN_TCI_PCP_MASK	0x7
#define VLAN_TCI_CFI_MASK	0x1
#define VLAN_TCI_VID_MASK	0xfff

#define MAC_IS_MCAST(mac)  ((mac)[0] & 0x1)

#define VLAN_PCP(vlan)  (((vlan)->label >> VLAN_PCP_SHIFT) & VLAN_TCI_PCP_MASK)
#define VLAN_CFI(vlan)  (((vlan)->label >> VLAN_CFI_SHIFT) & VLAN_TCI_CFI_MASK)
#define VLAN_VID(vlan)  (ntohs((vlan)->label) & VLAN_TCI_VID_MASK)
#define VLAN_LABEL(vid, pcp, cfi)  (htons((vid) & VLAN_TCI_VID_MASK) | (((pcp) & VLAN_TCI_PCP_MASK) << VLAN_PCP_SHIFT) | (((cfi) & VLAN_TCI_CFI_MASK) << VLAN_CFI_SHIFT))

/**
 * \ingroup socket
 * @{
 */
/* 802.1Q-2011, 9.6 */
#define VLAN_VID_MIN		0
#define VLAN_VID_MAX		4094
#define VLAN_VID_DEFAULT	0xfffe /**< non standard value, used to indicate default vlan vid */
#define VLAN_VID_NONE		0xffff /**< non standard value, used to indicate no vlan tag */

/* 802.1Q-2018, Table 9-2 */
#define VLAN_PVID_DEFAULT	1

/* 802.1Q-2018, 6.9.3 */
#define VLAN_DEI_DEFAULT	0

/** @} */

#define ETHERTYPE_IPV4	0x0800
#define ETHERTYPE_MSRP	0x22ea
#define ETHERTYPE_AVTP	0x22f0
#define ETHERTYPE_VLAN	0x8100
#define ETHERTYPE_IPV6	0x86DD
#define ETHERTYPE_MVRP	0x88f5
#define ETHERTYPE_MMRP	0x88f6
#define ETHERTYPE_PTP	0x88f7
#define ETHERTYPE_HSR	0x892f
#define ETHERTYPE_HSR_SUPERVISION 0x88fb

#define MC_ADDR_AVDECC_ADP_ACMP		{0x91, 0xe0, 0xf0, 0x01, 0x00, 0x00}
#define MC_ADDR_MAAP_BASE		{0x91, 0xe0, 0xf0, 0x00, 0xfe, 0x00}	/* MAAP locally administered pool, per IEEE1722-2016 Annex B section 4 */
#define MC_ADDR_MSRP			{0x01, 0x80, 0xc2, 0x00, 0x00, 0x0e}
#define MC_ADDR_MMRP			{0x01, 0x80, 0xc2, 0x00, 0x00, 0x20}
#define MC_ADDR_MVRP			{0x01, 0x80, 0xc2, 0x00, 0x00, 0x21}
#define MC_ADDR_PTP			{0x01, 0x80, 0xc2, 0x00, 0x00, 0x0e}
#define MC_ADDR_PTP_MASK		{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}

#define C_VLAN_RESERVED_BASE	{0x01, 0x80, 0xc2, 0x00, 0x00, 0x00}
#define C_VLAN_RESERVED_MASK	{0xff, 0xff, 0xff, 0xff, 0xff, 0xf0}
#define MMRP_MVRP_BASE		{0x01, 0x80, 0xC2, 0x00, 0x00, 0x20}
#define MMRP_MVRP_MASK		{0xff, 0xff, 0xff, 0xff, 0xff, 0xfe}

/* HSR supervision frame, defined in IEC 62439-3, section 5.7.2 */
#define HSR_SUPERVISION_BASE	{0x01, 0x15, 0x4E, 0x00, 0x01, 0x00}
#define HSR_SUPERVISION_MASK	{0xff, 0xff, 0xff, 0xff, 0xff, 0x00}

#define ETHER_MTU		1500
#define ETHER_MIN_FRAME_SIZE	64	/* including fcs */

#define ETHER_FCS              4

#define ETHER_IFG              12
#define ETHER_PREAMBLE         8

#define ETHER_ETYPE_OFFSET	12
#define ETHER_VLAN_LABEL_OFFSET	14

#endif /* _GENAVB_PUBLIC_ETHER_H_ */
