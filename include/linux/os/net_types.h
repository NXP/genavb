/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2018, 2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief OS specific GenAVB/TSN public API
 \details OS specific net types

 \copyright Copyright 2016 Freescale Semiconductor, Inc.
 \copyright Copyright 2016, 2018, 2021, 2023-2024 NXP
*/

#ifndef _OS_GENAVB_PUBLIC_NET_TYPES_H_
#define _OS_GENAVB_PUBLIC_NET_TYPES_H_

#define NET_DATA_OFFSET 128 /* Hardcode the value to 128 as a common L1_CACHE alignment of
                               the max avb descriptor size for both i.MX6 and i.MX8*/

#define MAX_SOCKETS	256

struct genavb_xdp_key {
	uint16_t protocol;	/**< protocol type */
	uint16_t vlan_id;	/**< vlan id (network order), one of [VLAN_VID_MIN, VLAN_VID_MAX], VLAN_VID_NONE or VLAND_ID_DEFAULT */
	uint8_t dst_mac[6];	/**< destination MAC */
};

enum NET_POOL_TYPE {
	POOL_TYPE_STD     = 0, /**< Memory Pool for linux standard network interface */
	POOL_TYPE_AVB     = 1, /**< Memory Pool for linux avb network interface */
	POOL_TYPE_XDP     = 2, /**< Memory Pool for linux xdp network interface */
	POOL_TYPE_IPC     = 3, /**< Memory Pool for linux ipc network interface */
	POOL_TYPE_MAX,
};

#endif /* _OS_GENAVB_PUBLIC_NET_TYPES_H_ */
