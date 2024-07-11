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

#ifndef _RTOS_NET_PORT_NETC_STREAM_IDENTIFICATION_H_
#define _RTOS_NET_PORT_NETC_STREAM_IDENTIFICATION_H_

#include "net_bridge.h"

int netc_si_init(struct net_bridge *bridge);
int netc_si_update_sf_ref(void *drv, uint32_t handle);
int netc_si_update_frer_ref(void *drv, uint32_t handle);

#endif /* _RTOS_NET_PORT_NETC_STREAM_IDENTIFICATION_H_ */

