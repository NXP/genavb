/*
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
 * Copyright 2017-2020, 2022-2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _NET_PORT_H_
#define _NET_PORT_H_

#include "genavb/net_types.h"

#include "net_tx.h"
#include "pool_dma.h"

struct net_port_status {
	u16 port;
	bool up;
	bool full_duplex;
	unsigned int rate;
};

#ifdef __KERNEL__

#include "queue.h"
#include "netdrv.h"

#define FEC_ALIGNMENT	16

#define FCS_LEN	4	/* extra data at the end of each packet, based on current MAC configuration settings */

#define AVB_NET_RX_WAKE		1
#define AVB_NET_RX_OK		0
#define AVB_NET_RX_SLOW		-1
#define AVB_NET_RX_DROP		-2

static inline void stream_id_copy(void *dst, void *src)
{
	*(u32 *)dst = *(u32 *)src;
	*(u32 *)((char *)dst + 4) = *(u32 *)((char *)src + 4);
}

#define PORT_FLAGS_ENABLED	(1 << 0)

struct eth_avb {
	struct queue rx_queue;
	unsigned long rx_queue_extra_storage[CFG_RX_EXTRA_ENTRIES]; /* Must follow rx_queue member */

	struct queue tx_queue;
	struct qos_queue *tx_qos_queue;

	struct queue tx_cleanup_queue;
	unsigned long tx_cleanup_queue_extra_storage[CFG_TX_CLEANUP_EXTRA_ENTRIES]; /* Must follow tx_clean_queue member */

	unsigned int count;
	struct pool_dma *buf_pool;
	raw_spinlock_t lock;
	int port;
	unsigned int flags;
	int ifindex;
	void *fec_data;
	struct port_qos *qos;
	unsigned int rate;

	struct logical_port *logical_port;
	struct device *dev;
};

struct avb_drv;

int eth_avb_port_status(struct eth_avb *eth, struct net_port_status *status);
int eth_avb_read_ptp(struct eth_avb *eth, unsigned int *value);
int eth_avb_add_multi(struct eth_avb *eth, struct net_mc_address *addr);
int eth_avb_del_multi(struct eth_avb *eth, struct net_mc_address *addr);
int eth_avb_init(struct eth_avb *eth, struct pool_dma *buf_pool, struct net_qos *qos, struct dentry *avb_dentry);
void eth_avb_exit(struct eth_avb *eth);

static inline struct logical_port *physical_to_logical_port(struct eth_avb *eth)
{
	return eth->logical_port;
}

#endif /* __KERNEL__ */

#endif /* _NET_PORT_H_ */
