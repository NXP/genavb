/*
* Copyright 2022-2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief RTOS specific Network service implementation
 @details
*/

#ifndef _RTOS_NET_PORT_NETC_SW_H_
#define _RTOS_NET_PORT_NETC_SW_H_

#include "net_port.h"
#include "net_bridge.h"

int netc_sw_init(struct net_bridge *sw);
int netc_sw_port_init(struct net_port *port);
void *netc_sw_get_handle(struct net_port *port);
void *netc_sw_get_link_handle(struct net_port *port);
void *netc_sw_get_port_handle(struct net_port *port);

#endif /* _RTOS_NET_PORT_NETC_SW_H_ */
