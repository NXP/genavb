/*
 * Copyright 2018-2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief MRP management functions
 @details
*/

#ifndef _RTOS_MRP_H_
#define _RTOS_MRP_H_

#include "net_socket.h"
#include "net_rx.h"

void mrp_socket_unbind(struct net_socket *sock);
int mrp_socket_bind(struct net_socket *sock, struct net_address *addr);

void mrp_socket_disconnect(struct logical_port *port, struct net_socket *sock);
int mrp_socket_connect(struct logical_port *port, struct net_socket *sock, struct net_address *addr);

int mrp_rx(struct net_rx_ctx *net, struct logical_port *port, struct net_rx_desc *desc, void *hdr);

#endif /* _RTOS_MRP_H_ */
