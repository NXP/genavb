/*
* Copyright 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief RTOS specific Network service implementation
 @details
*/

#ifndef _RTOS_NET_PORT_NETC_SW_FRER_H_
#define _RTOS_NET_PORT_NETC_SW_FRER_H_

#include "net_bridge.h"

void netc_sw_frer_init(struct net_bridge *bridge);
int netc_sw_frer_get_eid(void *drv, uint32_t handle, uint32_t *isqg_eid, uint32_t *et_eid, uint32_t *et_port_map);

#endif /* _RTOS_NET_PORT_NETC_SW_FRER_H_ */
