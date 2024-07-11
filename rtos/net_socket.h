/*
 * Copyright 2017-2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief RTOS specific Network service implementation
 @details
*/

#ifndef _RTOS_NET_SOCKET_H_
#define _RTOS_NET_SOCKET_H_

#include "rtos_abstraction_layer.h"

#include "avb_queue.h"

#include "net_tx.h"
#include "net_rx.h"
#include "slist.h"
#include "net_logical_port.h"

#define SOCKET_FLAGS_RX			(1 << 0)
#define SOCKET_FLAGS_BOUND		(1 << 1)
#define SOCKET_FLAGS_CONNECTED		(1 << 2)
#define SOCKET_FLAGS_VLAN		(1 << 3)
#define SOCKET_FLAGS_TX_TS_ENABLED	(1 << 4)
#define SOCKET_FLAGS_RX_POLLING		(1 << 5)
#define SOCKET_FLAGS_TX_DIRECT		(1 << 6)
#define SOCKET_FLAGS_TX_RAW		(1 << 7)

#define SOCKET_ATOMIC_FLAGS_CALLBACK_ENABLED	0
#define SOCKET_ATOMIC_FLAGS_LATENCY_VALID	1

#define SOCKET_ERROR			(1 << 0)
#define SOCKET_SUCCESS			(1 << 1)

#define SOCKET_OPTION_BUFFER_PACKETS	0
#define SOCKET_OPTION_BUFFER_LATENCY	1
#define SOCKET_OPTION_PACKETS		2

struct net_socket {
	struct net_address addr;
	struct slist_node list_node;

	struct slist_node polling_node;
	unsigned int time;

	unsigned int flags;

	rtos_atomic_t atomic_flags;

	union {
		struct {
			rtos_mqueue_t *ts_queue_handle;
			struct qos_queue *qos_queue;
			struct queue queue;
			int (*func)(struct net_socket *, struct logical_port *, unsigned long *, unsigned int *);
			uint16_t vlan_label;
		} tx;
		struct {
			struct queue queue;
			struct net_rx_ctx *net_rx;
			unsigned int max_packets;
			unsigned int max_latency;
		} rx;
	};

	/* control */
	rtos_event_group_t event_group;

	rtos_mqueue_t *handle;
	void *handle_data;

	void (*callback)(void *);
	void *callback_data;
};

struct socket_callback_event {
	struct net_socket *sock;
	void (*callback)(void *);
	void *callback_data;
};


struct net_set_option {
	struct net_socket *sock;
	unsigned int type;
	unsigned long val;
};

void socket_poll_all(struct net_rx_ctx *net);
int socket_get_hwaddr(unsigned int port_id, uint8_t *addr);
int socket_port_status(struct net_socket *sock, struct net_port_status *status);
int socket_add_multi(struct net_socket *sock, struct net_mc_address *addr);
int socket_del_multi(struct net_socket *sock, struct net_mc_address *addr);
void generic_unbind(struct generic_rx_hdlr *hdlr, struct net_socket *sock, unsigned int port_max);
int generic_bind(struct generic_rx_hdlr *hdlr, struct net_socket *sock, unsigned int port, unsigned int port_max);
void socket_bind_event(struct net_socket *sock);
void socket_unbind_event(struct net_socket *sock);
int socket_bind(struct net_socket *sock, struct net_address *addr);
void socket_unbind(struct net_socket *sock);
void socket_connect_event(struct net_socket *sock);
void socket_disconnect_event(struct net_socket *sock);
int socket_connect(struct net_socket *sock, struct net_address *addr);
void socket_disconnect(struct net_socket *sock);
int socket_rx(struct net_rx_ctx *net, struct net_socket *sock, struct net_rx_desc *desc, struct net_rx_stats *stats);
int socket_read(struct net_socket *sock, unsigned long *addr, unsigned int len);
int socket_read_ts(struct net_socket *sock, struct net_ts_desc *ts_desc);
unsigned int socket_tx_available(struct net_socket *sock);
int socket_write(struct net_socket *sock, unsigned long *addr, unsigned int n);
struct net_socket *socket_open(rtos_mqueue_t *handle, void *handle_data, unsigned int rx);
void socket_close(struct net_socket *sock);
int socket_set_callback(struct net_socket *sock, void (*callback)(void *), void *data);
int socket_rx_enable_callback(struct net_socket *sock);
int socket_tx_enable_callback(struct net_socket *sock);
void socket_set_callback_event(struct socket_callback_event *sock_cb_event);
int socket_set_option(struct net_socket *sock, unsigned int type, unsigned long val);
void socket_set_option_event(struct net_set_option *option);

static inline int socket_qos_enqueue(struct net_socket *sock, struct logical_port *port,
				     struct qos_queue *qos_q, struct net_tx_desc *desc)
{
	/* If a dedicated HW queue is available, send frame immediately */
	if ((qos_q->tc->flags & TC_FLAGS_HW_SP) ||
	    (sock->flags & SOCKET_FLAGS_TX_DIRECT)) {
		return port_qos_tx(port->phys, qos_q, desc);
	} else {
		if (queue_enqueue(&sock->tx.queue, (unsigned long)desc) < 0) {
			qos_q->full++;
			qos_q->dropped++;
			return -1;
		}

		rtos_atomic_set_bit(qos_q->index, &qos_q->tc->shared_pending_mask);
	}

	return 0;
}

#endif /* _RTOS_NET_SOCKET_H_ */
