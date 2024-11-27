/*
 * Copyright 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2020, 2022-2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _PTP_H_
#define _PTP_H_

#include "genavb/ptp.h"

#include "net_rx.h"
#include "netdrv.h"

void ptp_socket_unbind(struct net_socket *sock);
int ptp_socket_bind(struct net_socket *sock, struct net_address *addr);
void ptp_socket_disconnect(struct logical_port *port, struct net_socket *sock);
int ptp_socket_connect(struct logical_port *port, struct net_socket *sock, struct net_address *addr);
void ptp_tx_ts_wakeup(struct logical_port *port);
int ptp_tx_ts(struct logical_port *port, struct net_tx_desc *desc);
int ptp_tx_ts_lock(struct logical_port *port, struct net_tx_desc *desc);
int ptp_rx(struct eth_avb *eth, struct net_rx_desc *desc, void *hdr);
int __ptp_rx(struct logical_port *port, struct net_rx_desc *desc);

#endif /* _PTP_H_ */
