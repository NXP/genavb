/*
 * Copyright 2017-2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief PTP management functions
 @details
*/

#ifndef _RTOS_PTP_H_
#define _RTOS_PTP_H_

#include "genavb/ptp.h"

#include "net_socket.h"
#include "net_rx.h"
#include "net_tx.h"

void ptp_socket_unbind(struct net_socket *sock);
int ptp_socket_bind(struct net_socket *sock, struct net_address *addr);

void ptp_socket_disconnect(struct logical_port *port, struct net_socket *sock);
int ptp_socket_connect(struct logical_port *port, struct net_socket *sock, struct net_address *addr);

void ptp_tx_ts(struct net_port *phys_port, u64 ts, u32 priv);
int ptp_rx(struct net_rx_ctx *net, struct logical_port *port, struct net_rx_desc *desc, void *hdr);

#endif /* _RTOS_PTP_H_ */
