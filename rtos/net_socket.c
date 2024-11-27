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

#include "rtos_abstraction_layer.h"
#include "string.h"

#include "net.h"
#include "net_bridge.h"
#include "net_socket.h"
#include "net_tx.h"
#include "net_port.h"
#include "net_logical_port.h"

#include "genavb/ether.h"
#include "common/log.h"
#include "os/net.h"
#include "os/string.h"

#include "ptp.h"
#include "mrp.h"
#include "hsr.h"
#include "avtp.h"
#include "l2.h"

#define SOCKET_MIN_PACKETS	1
#define SOCKET_MAX_PACKETS	QUEUE_ENTRIES_MAX
#define SOCKET_MAX_LATENCY	1000000000	/* 1 second */

int socket_get_hwaddr(unsigned int port_id, unsigned char *addr)
{
	struct logical_port *port;

	port = logical_port_get(port_id);
	if (!port)
		return -1;

	os_memcpy(addr, port_get_hwaddr(port->phys), 6);

	return 0;
}

void socket_set_option_event(struct net_set_option *option)
{
	struct net_socket *sock = option->sock;
	struct net_rx_ctx *net_rx = sock->rx.net_rx;
	unsigned int packets;

	switch (option->type) {
	case SOCKET_OPTION_BUFFER_PACKETS:
		packets = option->val;

		if ((packets < SOCKET_MIN_PACKETS) || (packets > SOCKET_MAX_PACKETS))
			goto err;

		sock->rx.max_packets = packets;

		break;

	case SOCKET_OPTION_BUFFER_LATENCY:
		if (option->val > SOCKET_MAX_LATENCY)
			goto err;

		sock->rx.max_latency = option->val;

		break;

	case SOCKET_OPTION_PACKETS:
		if (option->val < sock->rx.max_packets)
			goto err;

		if (queue_set_size(&sock->rx.queue, option->val) < 0)
			goto err;

		break;

	default:
		goto err;
	}

	if ((sock->rx.max_packets > 1) && (sock->rx.max_latency > 0)) {
		if (!(sock->flags & SOCKET_FLAGS_RX_POLLING)) {
			slist_add_head(&net_rx->polling_list, &sock->polling_node);
			sock->flags |= SOCKET_FLAGS_RX_POLLING;
		}
	} else {
		if (sock->flags & SOCKET_FLAGS_RX_POLLING) {
			slist_del(&net_rx->polling_list, &sock->polling_node);
			sock->flags &= ~SOCKET_FLAGS_RX_POLLING;
		}
	}

	rtos_event_group_set(&sock->event_group, SOCKET_SUCCESS);

	return;

err:
	rtos_event_group_set(&sock->event_group, SOCKET_ERROR);
}

int socket_set_option(struct net_socket *sock, unsigned int type, unsigned long val)
{
	struct event e;
	struct net_set_option option;
	int rc;

	option.sock = sock;
	option.type = type;
	option.val = val;

	e.type = EVENT_SOCKET_OPT;
	e.data = &option;

	if (rtos_mqueue_send(&net_rx_ctx.queue, &e, RTOS_NO_WAIT) < 0)
		goto err;

	rc = rtos_event_group_wait(&sock->event_group, SOCKET_ERROR | SOCKET_SUCCESS, true, RTOS_MS_TO_TICKS(10));

	if (!(rc & SOCKET_SUCCESS))
		goto err;

	return 0;

err:
	return -1;
}

int socket_port_status(struct net_socket *sock, struct net_port_status *status)
{
	struct logical_port *port;

	port = logical_port_get(status->port);
	if (!port)
		goto err;

	return port_status(port->phys, status);

err:
	return -1;
}

int socket_add_multi(struct net_socket *sock, struct net_mc_address *addr)
{
	struct logical_port *port;

	port = logical_port_get(addr->port);
	if (!port)
		return -1;

	return port_add_multi(port->phys, addr->hw_addr);
}

int socket_del_multi(struct net_socket *sock, struct net_mc_address *addr)
{
	struct logical_port *port;

	port = logical_port_get(addr->port);
	if (!port)
		return -1;

	return port_del_multi(port->phys, addr->hw_addr);
}

void generic_unbind(struct generic_rx_hdlr *hdlr, struct net_socket *sock, unsigned int port_max)
{
	int i;

	if (sock->addr.port == PORT_ANY) {
		for (i = 0; i < port_max; i++)
			hdlr[i].sock = NULL;
	} else {
		hdlr[sock->addr.port].sock = NULL;
	}
}

int generic_bind(struct generic_rx_hdlr *hdlr, struct net_socket *sock, unsigned int port, unsigned int port_max)
{
	int i;

	if ((port >= port_max) && (port != PORT_ANY))
		return -1;

	if (port == PORT_ANY) {
		for (i = 0; i < port_max; i++)
			if (hdlr[i].sock)
				return -1;

		for (i = 0; i < port_max; i++)
			hdlr[i].sock = sock;
	} else {
		if (hdlr[port].sock)
			return -1;

		hdlr[port].sock = sock;
	}

	return 0;
}

void socket_bind_event(struct net_socket *sock)
{
	struct net_address *addr = &sock->addr;
	int rc;

	switch (addr->ptype) {
	case PTYPE_AVTP:
		rc = avtp_socket_bind(sock, addr);
		break;

	case PTYPE_MRP:
		rc = mrp_socket_bind(sock, addr);
		break;

	case PTYPE_PTP:
		rc = ptp_socket_bind(sock, addr);
		break;

	case PTYPE_HSR:
		rc = hsr_socket_bind(sock, addr);
		break;

	case PTYPE_L2:
		rc = l2_socket_bind(sock, addr);
		break;

	case PTYPE_OTHER:
		rc = other_socket_bind(sock, addr);
		break;

	default:
		rc = -1;
		break;
	}

	if (rc < 0)
		goto err;

	rtos_event_group_set(&sock->event_group, SOCKET_SUCCESS);

	return;

err:
	rtos_event_group_set(&sock->event_group, SOCKET_ERROR);
}

int socket_bind(struct net_socket *sock, struct net_address *addr)
{
	struct event e;
	uint32_t rc;

	if (!(sock->flags & SOCKET_FLAGS_RX))
		goto err;

	if (addr->ptype >= PTYPE_MAX)
		goto err;

	/* PORT_ANY is valid */
	if ((!logical_port_valid(addr->port)) && (addr->port != PORT_ANY)) {
		os_log(LOG_ERR, "port(%d) outside of range\n", addr->port);
		goto err;
	}

	socket_unbind(sock);

	if (sock->flags & SOCKET_FLAGS_BOUND)
		goto err;

	e.type = EVENT_SOCKET_BIND;
	e.data = sock;

	memcpy(&sock->addr, addr, sizeof(*addr));

	if (rtos_mqueue_send(&net_rx_ctx.queue, &e, RTOS_NO_WAIT) < 0)
		goto err;

	rc = rtos_event_group_wait(&sock->event_group, SOCKET_ERROR | SOCKET_SUCCESS, true, RTOS_MS_TO_TICKS(1));

	if (!(rc & SOCKET_SUCCESS))
		goto err;

	sock->flags |= SOCKET_FLAGS_BOUND;

	return 0;

err:
	return -1;
}

void socket_unbind_event(struct net_socket *sock)
{
	switch (sock->addr.ptype) {
	case PTYPE_AVTP:
		avtp_socket_unbind(sock);
		break;

	case PTYPE_MRP:
		mrp_socket_unbind(sock);
		break;

	case PTYPE_PTP:
		ptp_socket_unbind(sock);
		break;

	case PTYPE_HSR:
		hsr_socket_unbind(sock);
		break;

	case PTYPE_L2:
		l2_socket_unbind(sock);
		break;

	case PTYPE_OTHER:
		other_socket_unbind(sock);
		break;

	default:
		break;
	}

	rtos_event_group_set(&sock->event_group, SOCKET_SUCCESS);
}


void socket_unbind(struct net_socket *sock)
{
	struct event e;

	if (!(sock->flags & SOCKET_FLAGS_RX))
		return;

	if (!(sock->flags & SOCKET_FLAGS_BOUND))
		return;

	e.type = EVENT_SOCKET_UNBIND;
	e.data = sock;

	if (rtos_mqueue_send(&net_rx_ctx.queue, &e, RTOS_NO_WAIT) < 0)
		return;

	rtos_event_group_wait(&sock->event_group, SOCKET_SUCCESS, true, RTOS_MS_TO_TICKS(1));

	memset(&sock->addr, 0, sizeof(sock->addr));
	sock->addr.ptype = PTYPE_NONE;

	sock->flags &= ~SOCKET_FLAGS_BOUND;
}

void socket_connect_event(struct net_socket *sock)
{
	struct net_address *addr = &sock->addr;
	struct logical_port *port;
	int rc;

	port = logical_port_get(addr->port);
	if (!port) {
		os_log(LOG_ERR, "port(%d) invalid\n", addr->port);
		goto err;
	}

	switch (addr->ptype) {
	case PTYPE_PTP:
		rc = ptp_socket_connect(port, sock, addr);
		break;
	case PTYPE_AVTP:
		rc = avtp_socket_connect(port, sock, addr);
		break;

	case PTYPE_MRP:
		rc = mrp_socket_connect(port, sock, addr);
		break;

	case PTYPE_HSR:
		rc = hsr_socket_connect(port, sock, addr);
		break;

	case PTYPE_L2:
		rc = l2_socket_connect(port, sock, addr);
		break;

	default:
		rc = -1;
		break;
	}

	if (rc < 0)
		goto err;

	rtos_event_group_set(&sock->event_group, SOCKET_SUCCESS);

	return;

err:
	rtos_event_group_set(&sock->event_group, SOCKET_ERROR);
}

int socket_connect(struct net_socket *sock, struct net_address *addr)
{
//	struct net_rx_ctx *net = sock->net_rx;
	struct event e;
	int rc;

	if (sock->flags & SOCKET_FLAGS_RX)
		goto err;

	socket_disconnect(sock);

	if (sock->flags & SOCKET_FLAGS_CONNECTED)
		goto err;

	memcpy(&sock->addr, addr, sizeof(*addr));

	e.type = EVENT_SOCKET_CONNECT;
	e.data = sock;

	if (rtos_mqueue_send(&net_tx_ctx.queue, &e, RTOS_NO_WAIT) < 0)
		goto err;

	rc = rtos_event_group_wait(&sock->event_group, SOCKET_ERROR | SOCKET_SUCCESS, true, RTOS_MS_TO_TICKS(1));

	if (!(rc & SOCKET_SUCCESS))
		goto err;

	sock->flags |= SOCKET_FLAGS_CONNECTED;

	return 0;

err:
	return -1;
}

void socket_disconnect_event(struct net_socket *sock)
{
	struct logical_port *port = __logical_port_get(sock->addr.port);

	switch (sock->addr.ptype) {
	case PTYPE_PTP:
		ptp_socket_disconnect(port, sock);
		break;
	case PTYPE_AVTP:
		avtp_socket_disconnect(port, sock);
		break;

	case PTYPE_MRP:
		mrp_socket_disconnect(port, sock);
		break;

	case PTYPE_HSR:
		hsr_socket_disconnect(port, sock);
		break;

	case PTYPE_L2:
		l2_socket_disconnect(port, sock);
		break;

	default:
		break;
	}

	rtos_event_group_set(&sock->event_group, SOCKET_SUCCESS);
}

void socket_disconnect(struct net_socket *sock)
{
//	struct net_rx_ctx *net = sock->net_rx;
	struct event e;

	if (sock->flags & SOCKET_FLAGS_RX)
		return;

	if (!(sock->flags & SOCKET_FLAGS_CONNECTED))
		return;

	e.type = EVENT_SOCKET_DISCONNECT;
	e.data = sock;

	if (rtos_mqueue_send(&net_tx_ctx.queue, &e, RTOS_NO_WAIT) < 0)
		return;

	rtos_event_group_wait(&sock->event_group, SOCKET_SUCCESS, true, RTOS_MS_TO_TICKS(1));

	memset(&sock->addr, 0, sizeof(sock->addr));
	sock->addr.ptype = PTYPE_NONE;

	sock->flags &= ~SOCKET_FLAGS_CONNECTED;
}

static inline unsigned int socket_rx_pending(struct net_socket *sock)
{
	return queue_pending(&sock->rx.queue);
}

/* This function can't be called from ISR */
static inline unsigned int socket_tx_ts_pending(struct net_socket *sock)
{
	return rtos_mqueue_pending(sock->tx.ts_queue_handle);
}

static inline int socket_rx_ready(struct net_rx_ctx *net, struct net_socket *sock)
{
	if (!rtos_atomic_test_and_set_bit(SOCKET_ATOMIC_FLAGS_LATENCY_VALID, &sock->atomic_flags)) {
		struct net_rx_desc *desc;

		if (queue_pending(&sock->rx.queue)) {
			desc = (struct net_rx_desc *)queue_peek(&sock->rx.queue);
			sock->time = desc->priv;
		} else {
			rtos_atomic_clear_bit(SOCKET_ATOMIC_FLAGS_LATENCY_VALID, &sock->atomic_flags);
			return 0;
		}
	}

	if ((net->time - sock->time) >= sock->rx.max_latency)
		return 1;

	return 0;
}

void socket_poll_all(struct net_rx_ctx *net)
{
	struct slist_node *entry;
	struct net_socket *sock;

	/* go through list of sockets with polling enabled */
	slist_for_each (&net->polling_list, entry) {
		sock = container_of(entry, struct net_socket, polling_node);

		if (socket_rx_ready(net, sock)) {
			if (rtos_atomic_test_and_clear_bit(SOCKET_ATOMIC_FLAGS_CALLBACK_ENABLED, &sock->atomic_flags))
				sock->callback(sock->callback_data);
		}
	}
}

int socket_rx(struct net_rx_ctx *net, struct net_socket *sock, struct logical_port *port, struct net_rx_desc *desc, struct net_rx_stats *stats)
{
	if (sock->flags & SOCKET_FLAGS_RX_TS_ENABLED) {
		desc->ts64 = socket_time_from_hw(sock, port, desc->ts64);
		desc->ts = desc->ts64;
	}

	desc->priv = net->time;

	if (queue_enqueue(&sock->rx.queue, (unsigned long)desc) < 0)
		goto drop;

	stats->rx++;

	if (sock->callback) {
		/* Optimize the case where the buffer is 1, since we know there is at least that much in the queue */
		if ((sock->rx.max_packets == 1) || (socket_rx_pending(sock) >= sock->rx.max_packets)) {
			if (rtos_atomic_test_and_clear_bit(SOCKET_ATOMIC_FLAGS_CALLBACK_ENABLED, &sock->atomic_flags))
				sock->callback(sock->callback_data);
		}
	}

	return AVB_NET_RX_WAKE;

drop:
	return net_rx_drop(net, desc, stats);
}

void socket_flush(struct net_socket *sock)
{

	if (sock->flags & SOCKET_FLAGS_RX) {
		queue_flush(&sock->rx.queue, NULL);
	} else {
		queue_flush(&sock->tx.queue, NULL);
	}
}

static inline int socket_rx_sync(struct net_socket *sock, unsigned long *addr, int n, unsigned int len)
{
	struct net_rx_ctx *net = sock->rx.net_rx;
	struct logical_port *port = __logical_port_get(sock->addr.port);
	unsigned int rx_now = NET_RX_PACKETS;
	int queue;
	uint32_t qp = 0, read;
	int i = n;

	rtos_mutex_lock(&net->mutex, RTOS_WAIT_FOREVER);

	for (queue = port->phys->num_rx_q - 1; queue >= 0 && rx_now; queue--) {
		rx_now -= port_rx(net, port->phys, rx_now, queue);

		qp = queue_pending(&sock->rx.queue);
		if (qp >= (len - n))
			break;
	}

	rtos_mutex_unlock(&net->mutex);

	if (len > i + qp)
		len = i + qp;

	queue_dequeue_init(&sock->rx.queue, &read);

	for (; i < len; i++)
		addr[i] = queue_dequeue_next(&sock->rx.queue, &read);

	queue_dequeue_done(&sock->rx.queue, read);

	return i;
}

int socket_read(struct net_socket *sock, unsigned long *addr, unsigned int len)
{
	int i = 0;
	uint32_t read;
	unsigned int len_now, pending;

	if (!(sock->flags & SOCKET_FLAGS_RX))
		return -1;

	pending = queue_pending(&sock->rx.queue);
	len_now = min(pending, len);

	queue_dequeue_init(&sock->rx.queue, &read);

	for (i = 0; i < len_now; i++)
		addr[i] = queue_dequeue_next(&sock->rx.queue, &read);

	queue_dequeue_done(&sock->rx.queue, read);

	/* If socket is of type polling, and data was not available, trigger net rx explicitely and try again */
	/* Type polling: no callback defined */
	if (!sock->callback && (i < len))
		i = socket_rx_sync(sock, addr, i, len);

	return i;
}

int socket_read_ts(struct net_socket *sock, struct net_ts_desc *ts_desc)
{
	struct socket_tx_ts_data socket_data;

	if (sock->flags & SOCKET_FLAGS_RX)
		return -1;

	if (!(sock->flags & SOCKET_FLAGS_TX_TS_ENABLED))
		return -1;

	if (rtos_mqueue_receive_from_isr(sock->tx.ts_queue_handle, &socket_data, RTOS_NO_WAIT, NULL) < 0)
		return 0;

	ts_desc->ts = socket_data.ts;
	ts_desc->priv = socket_data.priv;

	return 1;
}

int socket_qos_enqueue(struct net_socket *sock, struct logical_port *port, unsigned long *addr, unsigned int *n)
{
	struct net_tx_desc *desc;
	u32 write;
	unsigned int available;
	struct qos_queue *qos_q = sock->tx.qos_queue;
	int rc = 0, i;

	available = queue_available(&sock->tx.queue);
	if (unlikely(*n > available)) {
		qos_q->full += *n - available;
		qos_q->dropped += *n - available;
		*n = available;
		rc = -1;
		if (!available)
			goto out;
	}

	queue_enqueue_init(&sock->tx.queue, &write);

	for (i = 0; i < *n; i++) {
		desc = (struct net_tx_desc *)addr[i];

		queue_enqueue_next(&sock->tx.queue, &write, (unsigned long)desc);
	}

	queue_enqueue_done(&sock->tx.queue, write);

	rtos_atomic_set_bit(qos_q->index, &qos_q->tc->shared_pending_mask);

	*n = i;

out:
	return rc;
}

int socket_port_tx(struct net_socket *sock, struct logical_port *port, unsigned long *addr, unsigned int *n)
{
	struct net_tx_desc *desc;
	int rc = 0, i;

	for (i = 0; i < *n; i++) {
		desc = (struct net_tx_desc *)addr[i];

		if (desc->flags & NET_TX_FLAGS_TS64)
			desc->ts64 = socket_time_to_cycles(sock, port, desc->ts64);

		if ((rc = port_tx(port->phys, desc, sock->tx.qos_queue->tc->hw_queue_id)) < 0)
			break;
	}

	*n = i;

	return rc;
}

int socket_bridge_port_tx(struct net_socket *sock, struct logical_port *port, unsigned long *addr, unsigned int *n)
{
	struct net_tx_desc *desc;
	int rc = 0, i;

	for (i = 0; i < *n; i++) {
		desc = (struct net_tx_desc *)addr[i];

		if ((rc = bridge_port_tx(port->phys, sock->addr.priority, desc)) < 0)
			break;
	}

	*n = i;

	return rc;
}

static int socket_tx_connected(struct net_socket *sock, unsigned long *addr, unsigned int n)
{
	struct eth_hdr *eth;
	struct logical_port *port = __logical_port_get(sock->addr.port);
	unsigned int i;

	if (!port->phys->up)
		return 0;

	for (i = 0; i < n; i++) {
		struct net_tx_desc *desc = (struct net_tx_desc *)addr[i];

		eth = (struct eth_hdr *)((uint8_t *)desc + desc->l2_offset);

		/* Add src mac address */
		if (!(sock->flags & SOCKET_FLAGS_TX_RAW))
			socket_get_hwaddr(sock->addr.port, eth->src);

		/* Add vlan tag */
		if (sock->flags & SOCKET_FLAGS_VLAN) {
			struct vlanhdr *vlan = (struct vlanhdr *)(eth + 1);

			vlan->label = sock->tx.vlan_label;
		}

		desc->flags &= sock->tx.flags;
	}

	sock->tx.func(sock, port, addr, &i);

	return i;
}

unsigned int socket_tx_available(struct net_socket *sock)
{
	return queue_available(&sock->rx.queue);
}

int socket_write(struct net_socket *sock, unsigned long *addr, unsigned int n)
{
	if (sock->flags & SOCKET_FLAGS_RX)
		return -1;

	if (sock->flags & SOCKET_FLAGS_CONNECTED)
		return socket_tx_connected(sock, addr, n);
	else
		return -1;
}

static void socket_event_callback(void *arg)
{
	struct event e;
	struct net_socket *sock = (struct net_socket *)arg;

	e.type = EVENT_TYPE_NET_RX;
	e.data = sock->handle_data;
	rtos_mqueue_send(sock->handle, &e, RTOS_NO_WAIT);
}

void socket_set_callback_event(struct socket_callback_event *sock_cb_event)
{
	struct net_socket *sock = sock_cb_event->sock;

	sock->callback = sock_cb_event->callback;
	sock->callback_data = sock_cb_event->callback_data;

	rtos_atomic_set_bit(SOCKET_ATOMIC_FLAGS_CALLBACK_ENABLED, &sock->atomic_flags);

	rtos_event_group_set(&sock->event_group, SOCKET_SUCCESS);
}

int socket_set_callback(struct net_socket *sock, void (*callback)(void *), void *data)
{
	struct event e;
	struct socket_callback_event sock_cb_event;
	uint32_t rc;
	rtos_mqueue_t *mq;

	if (sock->handle)
		return -1;

	sock_cb_event.sock = sock;
	sock_cb_event.callback = callback;
	sock_cb_event.callback_data = data;

	e.type = EVENT_SOCKET_CALLBACK;
	e.data = &sock_cb_event;

	if (sock->flags & SOCKET_FLAGS_RX)
		mq = &net_rx_ctx.queue;
	else
		mq = &net_tx_ctx.queue;

	if (rtos_mqueue_send(mq, &e, RTOS_NO_WAIT) < 0)
		return -1;

	rc = rtos_event_group_wait(&sock->event_group, SOCKET_SUCCESS, true, RTOS_MS_TO_TICKS(1));

	if (!(rc & SOCKET_SUCCESS))
		return -1;

	return 0;
}

int socket_rx_enable_callback(struct net_socket *sock)
{
	if (!sock->callback)
		goto err;

	/* Force update of socket latency */
	rtos_atomic_clear_bit(SOCKET_ATOMIC_FLAGS_LATENCY_VALID, &sock->atomic_flags);

	rtos_atomic_set_bit(SOCKET_ATOMIC_FLAGS_CALLBACK_ENABLED, &sock->atomic_flags);

	if (socket_rx_pending(sock) >= sock->rx.max_packets) {
		if (rtos_atomic_test_and_clear_bit(SOCKET_ATOMIC_FLAGS_CALLBACK_ENABLED, &sock->atomic_flags))
			sock->callback(sock->callback_data);
	}

	return 0;

err:
	return -1;
}

int socket_tx_enable_callback(struct net_socket *sock)
{
	if (!sock->callback)
		goto err;

	rtos_atomic_set_bit(SOCKET_ATOMIC_FLAGS_CALLBACK_ENABLED, &sock->atomic_flags);

	if (socket_tx_ts_pending(sock)) {
		if (rtos_atomic_test_and_clear_bit(SOCKET_ATOMIC_FLAGS_CALLBACK_ENABLED, &sock->atomic_flags))
			sock->callback(sock->callback_data);
	}

	return 0;

err:
	return -1;
}

struct net_socket *socket_open(rtos_mqueue_t *handle, void *handle_data, unsigned int rx)
{
	struct net_socket *sock;

	sock = rtos_malloc(sizeof(struct net_socket));
	if (!sock)
		goto err_malloc;

	memset(sock, 0, sizeof(*sock));

	if (rtos_event_group_init(&sock->event_group) < 0)
		goto err_group_create;

	sock->handle = handle;
	sock->handle_data = handle_data;

	if (rx) {
		queue_init(&sock->rx.queue, net_rx_desc_free, 0);

		if (handle) {
			sock->callback = socket_event_callback;
			sock->callback_data = sock;

			rtos_atomic_set_bit(SOCKET_ATOMIC_FLAGS_CALLBACK_ENABLED, &sock->atomic_flags);
		}

		sock->rx.max_packets = 1;
		sock->rx.max_latency = 0;
		sock->flags = SOCKET_FLAGS_RX;
		sock->rx.net_rx = &net_rx_ctx;
	} else {
		queue_init(&sock->tx.queue, net_tx_desc_free, 0);
	}

	return sock;

err_group_create:
	rtos_free(sock);

err_malloc:
	return NULL;
}

void socket_close(struct net_socket *sock)
{
	if (sock->flags & SOCKET_FLAGS_RX) {
		socket_unbind(sock);

		if (sock->flags & SOCKET_FLAGS_RX_POLLING) {
			slist_del(&sock->rx.net_rx->polling_list, &sock->polling_node);
			sock->flags &= ~SOCKET_FLAGS_RX_POLLING;
		}
	} else {
		socket_disconnect(sock);
	}

	socket_flush(sock);

	rtos_free(sock);
}
