/*
 * Copyright 2017-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief RTOS specific Network service implementation
 @details
*/

#ifndef _RTOS_NET_RX_H_
#define _RTOS_NET_RX_H_

#include "genavb/config.h"
#include "genavb/net_types.h"
#include "net_port.h"
#include "slist.h"

#include "rtos_abstraction_layer.h"

#include "os/sys_types.h"
#include "os/timer.h"

#define NET_RX_TX_PERIOD		125000	/* 125us */

#ifndef NET_RX_PACKETS
#define NET_RX_PACKETS		20	/* 100Mbit/s * 125us ~= 18.6 small packets,
					 1Gbit/s * 125us ~= 10 big packets */
#endif

#define NET_RX_BRIDGE_PACKETS   8

#define AVB_NET_RX_WAKE		1
#define AVB_NET_RX_OK		0
#define AVB_NET_RX_SLOW		-1
#define AVB_NET_RX_DROP		-2

#define NET_RX_EVENT_QUEUE_LENGTH	16

struct generic_rx_hdlr {
	struct net_socket *sock;
};

struct net_rx_stats {
	unsigned int rx;
	unsigned int dropped;
	unsigned int slow;
	unsigned int slow_dropped;
};

struct ptype_handler {
	struct net_rx_stats stats[CFG_PORTS];
};

struct net_rx_ctx {
	struct ptype_handler ptype_hdlr[PTYPE_MAX];

	rtos_mutex_t mutex;

	rtos_thread_t task;

	/* Periodic timer */
	struct net_rx_timer {
		struct os_timer handle;
		unsigned int period;
	} timer;

	struct slist_head polling_list;
	unsigned int time;

	/* Event Queue */
	rtos_mqueue_t queue;
	uint8_t queue_buffer[NET_RX_EVENT_QUEUE_LENGTH * sizeof(struct event)];
};

extern struct net_rx_ctx net_rx_ctx;

struct net_rx_desc *net_pool_rx_alloc(unsigned int size);
int net_rx_slow(struct net_rx_ctx *net, struct logical_port *port, struct net_rx_desc *desc, struct net_rx_stats *stats);
int net_rx_drop(struct net_rx_ctx *net, struct net_rx_desc *desc, struct net_rx_stats *stats);
int eth_rx(struct net_rx_ctx *net, struct net_rx_desc *desc, struct net_port *port);
void net_rx_flush(struct net_rx_ctx *net, struct net_port *port);
int other_socket_bind(struct net_socket *sock, struct net_address *addr);
void other_socket_unbind(struct net_socket *sock);

#endif /* _RTOS_NET_RX_H_ */
