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
#include "net_bridge.h"
#include "net_port.h"
#include "net_logical_port.h"
#include "clock.h"
#include "net_tx.h"

#include "os/net.h"

static struct generic_rx_hdlr mrp_rx_hdlr[CFG_LOGICAL_NUM_PORT];

static int mrp_tx(struct net_socket *sock, struct logical_port *port, unsigned long *addr, unsigned int *n);

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
	}

	sock->tx.func = mrp_tx;

	return 0;

err:
	return -1;
}

int mrp_rx(struct net_rx_ctx *net, struct logical_port *port, struct net_rx_desc *desc, void *hdr)
{
	struct net_rx_stats *stats = &net->ptype_hdlr[PTYPE_MRP].stats[desc->port];
	struct net_socket *sock;
	uint64_t gptp_time = 0;
	int rc;

	if (!port)
		goto slow;

	sock = mrp_rx_hdlr[port->id].sock;
	if (!sock)
		goto slow;

	clock_time_from_hw(port->phys->clock_gptp[0], desc->ts64, &gptp_time);
	desc->ts = gptp_time;
	desc->ts64 = gptp_time;

	rc = socket_rx(net, sock, desc, stats);

	return rc;

slow:
	return net_rx_slow(net, port, desc, stats);
}

static int mrp_tx(struct net_socket *sock, struct logical_port *port, unsigned long *addr, unsigned int *n)
{
	struct net_tx_desc *desc;
	unsigned int flags;
	int rc = 0, i;

	for (i = 0; i < *n; i++) {
		desc = (struct net_tx_desc *)addr[i];

		flags = desc->flags;

		desc->flags = 0;

		if (flags & NET_TX_FLAGS_TS)
			desc->flags |= NET_TX_FLAGS_TS;

		if (!port->is_bridge) {
			if ((rc = socket_qos_enqueue(sock, port, sock->tx.qos_queue, desc)) < 0)
				break;
		} else {
			if ((rc = bridge_port_tx(port->phys, sock->addr.priority, desc)) < 0)
				break;
		}
	}

	*n = i;

	return rc;
}
