/*
 * Copyright 2018-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief MRP management functions
 @details
*/

#include "genavb/qos.h"

#include "mrp.h"
#include "net_port.h"
#include "net_logical_port.h"
#include "clock.h"
#include "net_tx.h"

#include "os/net.h"

static struct generic_rx_hdlr mrp_rx_hdlr[CFG_LOGICAL_NUM_PORT];

void mrp_socket_unbind(struct net_socket *sock)
{
	generic_unbind(mrp_rx_hdlr, sock, CFG_LOGICAL_NUM_PORT);
}

int mrp_socket_bind(struct net_socket *sock, struct net_address *addr)
{
	return generic_bind(mrp_rx_hdlr, sock, addr->port, CFG_LOGICAL_NUM_PORT);
}

void mrp_socket_disconnect(struct logical_port *port, struct net_socket *sock)
{
	struct net_port *net_port = port->phys;

	if (!port->is_bridge)
		qos_queue_disconnect(net_port->qos, sock->tx.qos_queue);

	sock->tx.func = NULL;
}

int mrp_socket_connect(struct logical_port *port, struct net_socket *sock, struct net_address *addr)
{
	struct net_port *net_port = port->phys;

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

	sock->tx.flags = 0;

	return 0;

err:
	return -1;
}

int mrp_rx(struct net_rx_ctx *net, struct logical_port *port, struct net_rx_desc *desc, void *hdr)
{
	struct net_rx_stats *stats = &net->ptype_hdlr[PTYPE_MRP].stats[desc->port];
	struct net_socket *sock;
	int rc;

	if (!port)
		goto slow;

	sock = mrp_rx_hdlr[port->id].sock;
	if (!sock)
		goto slow;

	rc = socket_rx(net, sock, port, desc, stats);

	return rc;

slow:
	return net_rx_slow(net, port, desc, stats);
}
