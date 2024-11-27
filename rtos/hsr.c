/*
* Copyright 2023-2024 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief HSR management functions
 @details
*/

#include "net_logical_port.h"
#include "net_bridge.h"
#include "net_socket.h"
#include "net_port.h"
#include "net_tx.h"
#include "net_rx.h"

#include "os/net.h"

#include "genavb/ether.h"

#if defined(CONFIG_HSR)
static struct generic_rx_hdlr hsr_rx_hdlr[CFG_LOGICAL_NUM_PORT];

void hsr_socket_unbind(struct net_socket *sock)
{
	generic_unbind(hsr_rx_hdlr, sock, CFG_LOGICAL_NUM_PORT);
}

int hsr_socket_bind(struct net_socket *sock, struct net_address *addr)
{
	return generic_bind(hsr_rx_hdlr, sock, addr->port, CFG_LOGICAL_NUM_PORT);
}

void hsr_socket_disconnect(struct logical_port *port, struct net_socket *sock)
{
	struct net_port *net_port = port->phys;

	sock->tx.func = NULL;

	if (!port->is_bridge)
		qos_queue_disconnect(net_port->qos, sock->tx.qos_queue);
}

int hsr_socket_connect(struct logical_port *port, struct net_socket *sock, struct net_address *addr)
{
	struct net_port *net_port = port->phys;

	sock->tx.flags = 0;

	if (!port->is_bridge) {
		sock->tx.qos_queue = qos_queue_connect(net_port->qos, addr->priority, &sock->tx.queue, 0);
		if (!sock->tx.qos_queue)
			goto err;

		if (sock->tx.qos_queue->tc->flags & TC_FLAGS_HW_SP)
			sock->tx.func = socket_port_tx;
		else
			sock->tx.func = socket_qos_enqueue;
	} else {
		sock->tx.func = socket_bridge_port_tx;
	}

	return 0;

err:
	return -1;
}

int hsr_rx(struct net_rx_ctx *net, struct logical_port *port, struct net_rx_desc *desc)
{
	struct net_rx_stats *stats = &net->ptype_hdlr[PTYPE_HSR].stats[desc->port];
	struct net_socket *sock;
	int rc;

	if (!port)
		goto slow;

	sock = hsr_rx_hdlr[port->id].sock;
	if (!sock)
		goto slow;

	rc = socket_rx(net, sock, port, desc, stats);

	return rc;

slow:
	return net_rx_slow(net, port, desc, stats);
}

#else
void hsr_socket_unbind(struct net_socket *sock)
{
}

int hsr_socket_bind(struct net_socket *sock, struct net_address *addr)
{
	return -1;
}

void hsr_socket_disconnect(struct logical_port *port, struct net_socket *sock)
{
}

int hsr_socket_connect(struct logical_port *port, struct net_socket *sock, struct net_address *addr)
{
	return -1;
}

int hsr_rx(struct net_rx_ctx *net, struct logical_port *port, struct net_rx_desc *desc)
{
	struct net_rx_stats *stats = &net->ptype_hdlr[PTYPE_HSR].stats[desc->port];

	return net_rx_slow(net, port, desc, stats);
}
#endif
