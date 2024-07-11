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

#ifndef _RTOS_NET_PORT_ENETC_EP_H_
#define _RTOS_NET_PORT_ENETC_EP_H_

#include "net_port.h"

int enetc_ep_init(struct net_port *port);
void *enetc_ep_get_handle(struct net_port *port);
void *enetc_ep_get_link_handle(struct net_port *port);
void *enetc_ep_get_port_handle(struct net_port *port);
void *enetc_ep_get_si_handle(struct net_port *port);

#endif /* _RTOS_NET_PORT_ENETC_EP_H_ */
