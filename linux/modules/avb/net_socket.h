/*
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
 * Copyright 2017-2020, 2022-2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _NET_SOCKET_H_
#define _NET_SOCKET_H_

#include <linux/sched.h>
#include <linux/fec.h>

#include "genavb/config.h"
#include "genavb/sr_class.h"
#include "genavb/net_types.h"

#include "net_logical_port.h"
#include "queue.h"

/* number of slots for the rx histogram */
#define NET_STATS_BIN_MAX		64

#define CFG_NET_STREAM_STATS		1

extern unsigned int dt_bin_width_shift[SR_CLASS_MAX];

/* Queue kernel character device instance */
struct net_socket {
	struct queue queue;
	unsigned long queue_extra_storage[CFG_NET_TX_EXTRA_ENTRIES]; /* Must follow queue member */
	struct queue tx_ts_queue;
	struct hlist_node node;
	struct list_head list;

	struct net_address addr;

	unsigned int max_packets;
	unsigned int max_latency;

	unsigned int time;

	union {
#ifdef CFG_NET_STREAM_STATS
		struct {
			unsigned int dt_bin[NET_STATS_BIN_MAX];
			int dt_min;
			int dt_max;
		} avtp;
#endif
	};

	int (*tx_func)(struct net_socket *, struct logical_port *, unsigned long *, unsigned int *);
	struct qos_queue *qos_queue;

	unsigned int flags;
	unsigned long atomic_flags;

	u16 vlan_label;

#ifdef CFG_NET_STREAM_STATS
	unsigned int rx;
	unsigned int tlast;
	unsigned int dt_bin[NET_STATS_BIN_MAX];
	unsigned int dt_min;
	unsigned int dt_max;
#endif

	wait_queue_head_t wait;

	struct net_drv *drv;
};

#define SOCKET_FLAGS_RX			(1 << 0)
#define SOCKET_FLAGS_BOUND		(1 << 1)
#define SOCKET_FLAGS_CONNECTED		(1 << 2)
#define SOCKET_FLAGS_WITH_SPH		(1 << 3)
#define SOCKET_FLAGS_WITHOUT_SPH	(1 << 4)
#define SOCKET_FLAGS_SPH_MASK		(SOCKET_FLAGS_WITH_SPH | SOCKET_FLAGS_WITHOUT_SPH)
#define SOCKET_FLAGS_TX_TS_ENABLED	(1 << 5)
#define SOCKET_FLAGS_VLAN		(1 << 6)
#define SOCKET_FLAGS_RX_BATCHING_SYNC	(1 << 7)
#define SOCKET_FLAGS_RX_BATCHING_ASYNC	(1 << 8)
#define SOCKET_FLAGS_RX_NO_BATCHING	(1 << 9)
#define SOCKET_FLAGS_RX_BATCHING	(SOCKET_FLAGS_RX_BATCHING_SYNC | SOCKET_FLAGS_RX_BATCHING_ASYNC)
#define SOCKET_FLAGS_RX_BATCH_ANY	(SOCKET_FLAGS_RX_BATCHING_SYNC | SOCKET_FLAGS_RX_BATCHING_ASYNC | SOCKET_FLAGS_RX_NO_BATCHING)
#define SOCKET_FLAGS_FLOW_CONTROL	(1 << 10)

#define SOCKET_ATOMIC_FLAGS_FLUSH			0
#define SOCKET_ATOMIC_FLAGS_BUSY			1
#define SOCKET_ATOMIC_FLAGS_SOCKET_WAITING_EVENT	2
#define SOCKET_ATOMIC_FLAGS_LATENCY_VALID		3

#define SOCKET_MAX_RX			32

#include "net_rx.h"

int socket_set_option(struct net_socket *sock, struct net_set_option *option);
int socket_port_status(struct net_socket *sock, struct net_port_status *status);
int socket_add_multi(struct net_socket *sock, struct net_mc_address *addr);
int socket_del_multi(struct net_socket *sock, struct net_mc_address *addr);
void generic_unbind(struct generic_rx_hdlr *hdlr, struct net_socket *sock, unsigned int);
int generic_bind(struct generic_rx_hdlr *hdlr, struct net_socket *sock, unsigned int port, unsigned int);

int socket_bind(struct net_socket *sock, struct net_address *addr);
int socket_connect(struct net_socket *sock, struct net_address *addr);
int socket_read(struct net_socket *sock, unsigned long *buf, unsigned int n);
int socket_read_ts(struct net_socket *sock, struct net_ts_desc *ts_desc);
int socket_write(struct net_socket *sock, unsigned long *buf, unsigned int n);
unsigned int socket_tx_available(struct net_socket *sock);

void generic_rx_wakeup(struct net_socket *sock, struct net_socket **sock_array, unsigned int *n);
void generic_rx_wakeup_all(struct net_drv *drv, struct net_socket **sock_array, unsigned int *n);
int net_rx(struct eth_avb *eth, struct net_socket *sock, struct net_rx_desc *desc, struct net_rx_stats *stats);
int net_rx_batch_any_ready(struct net_drv *drv);

void socket_close(struct net_socket *sock);
struct net_socket *socket_open(unsigned int rx);

static inline void socket_schedule_wake_up(struct net_socket *sock, struct net_socket **sock_array, unsigned int *n)
{
	if (*n < SOCKET_MAX_RX) {
		set_bit(SOCKET_ATOMIC_FLAGS_BUSY, &sock->atomic_flags);
		sock_array[*n] = sock;
		(*n)++;
	}
}

static inline int socket_qos_enqueue(struct net_socket *sock, struct qos_queue *qos_q, struct avb_tx_desc *desc)
{
	if (queue_enqueue(&sock->queue, (unsigned long)desc) < 0) {
		qos_q->full++;
		qos_q->dropped++;
		return -EIO;
	}

	set_bit(qos_q->index, &qos_q->tc->shared_pending_mask);

	return 0;
}

#ifdef CFG_NET_STREAM_STATS
static inline void stream_init_stats(struct net_socket *sock)
{
	sock->rx = 0;
	sock->tlast = 0;

	sock->dt_min = 0xffffffff;
	sock->dt_max = 0x0;

	memset(sock->dt_bin, 0, sizeof(sock->dt_bin));
}

static inline void stream_update_stats(struct net_socket *sock, struct net_rx_desc *desc, const unsigned int dt_bin_width_shift)
{
	int dt;
	unsigned int bin;

	if (sock->rx) {
		dt = desc->ts - sock->tlast;

		if (dt < sock->dt_min)
			sock->dt_min = dt;
		else if (dt > sock->dt_max)
			sock->dt_max = dt;

		bin = dt >> dt_bin_width_shift;

		if (bin >= NET_STATS_BIN_MAX)
			bin = NET_STATS_BIN_MAX - 1;

		sock->dt_bin[bin]++;
	}

	sock->rx++;
	sock->tlast = desc->ts;
}
#endif

#endif /* _NET_SOCKET_H_ */
