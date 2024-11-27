/*
* Copyright 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief HSR management functions
 @details
*/

#ifndef _RTOS_HSR_H_
#define _RTOS_HSR_H_

#include "net_socket.h"
#include "net_rx.h"
#include "net_tx.h"

void hsr_socket_unbind(struct net_socket *sock);
int hsr_socket_bind(struct net_socket *sock, struct net_address *addr);

void hsr_socket_disconnect(struct logical_port *port, struct net_socket *sock);
int hsr_socket_connect(struct logical_port *port, struct net_socket *sock, struct net_address *addr);

int hsr_rx(struct net_rx_ctx *net, struct logical_port *port, struct net_rx_desc *desc);
#endif /* _RTOS_HSR_H_ */
