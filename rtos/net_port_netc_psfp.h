/*
* Copyright 2023-2024 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief RTOS specific Per Stream Filtering and Policing implementation
 @details
*/

#ifndef _RTOS_NET_PORT_NETC_PSFP_H_
#define _RTOS_NET_PORT_NETC_PSFP_H_

#include "net_bridge.h"

int netc_sw_psfp_init(struct net_bridge *bridge);
void netc_sw_psfp_exit(struct net_bridge *bridge);
int netc_sw_psfp_get_eid(void *drv, uint32_t handle, uint32_t *rp_eid, uint32_t *sg_eid, uint32_t *isc_eid, bool *sf_enable, uint16_t *msdu);

#endif /* _RTOS_NET_PORT_NETC_PSFP_H_ */
