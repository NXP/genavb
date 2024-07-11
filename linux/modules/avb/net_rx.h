/*
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
 * Copyright 2017-2018, 2020, 2022-2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _NET_RX_H_
#define _NET_RX_H_

#include "net_port.h"

#include "genavb/net_types.h"

struct net_rx_stats {
	unsigned int rx;
	unsigned int dropped;
	unsigned int slow;
	unsigned int slow_dropped;
};

struct generic_rx_hdlr {
	struct net_socket *sock;
};

struct ptype_handler {
	struct net_rx_stats stats[CFG_PORTS];
};


extern struct ptype_handler ptype_hdlr[PTYPE_MAX];
extern raw_spinlock_t ptype_lock;

int net_rx_slow(struct eth_avb *eth, struct net_rx_desc *desc, struct net_rx_stats *stats);
int net_rx_drop(struct eth_avb *eth, struct net_rx_desc *desc, struct net_rx_stats *stats);

int net_rx_thread(struct net_drv *drv);
int net_tx_thread(struct eth_avb *eth);
int net_tx_available_thread(struct eth_avb *eth);

int eth_rx(struct eth_avb *eth, struct net_rx_desc *desc);

#endif /* _NET_RX_H_ */
