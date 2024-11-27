/*
 * Copyright 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2020, 2022-2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "genavb/qos.h"

#include "mrp.h"
#include "net_socket.h"
#include "net_logical_port.h"

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
	struct eth_avb *eth = port->phys;

	sock->tx_func = NULL;

	qos_queue_disconnect(eth->qos, sock->qos_queue);
	sock->qos_queue = NULL;
}

static int mrp_tx(struct net_socket *sock, struct logical_port *port, unsigned long *desc_array, unsigned int *n)
{
	struct avb_tx_desc *desc;
	int rc = 0, i;

	for (i = 0; i < *n; i++) {
		desc = (void *)desc_array[i];

		desc->common.flags = 0;

		if ((rc = socket_qos_enqueue(sock, sock->qos_queue, desc)) < 0)
			break;
	}

	*n = i;

	return rc;
}
int mrp_socket_connect(struct logical_port *port, struct net_socket *sock, struct net_address *addr)
{
	struct eth_avb *eth = port->phys;

	/* Both SRP and PTP packets need to be strictly serialized when transmitted to network
	through the SJA1105 Host interface (for proper SJA1105 HW route management processing)
	So here SRP protocol is defined as 'connected' and assigned to the same high priority queue as PTP
	*/
	sock->qos_queue = qos_queue_connect(eth->qos, addr->priority, &sock->queue, 0);
	if (!sock->qos_queue)
		goto err;

	sock->tx_func = mrp_tx;

	return 0;
err:
	return -1;
}

int mrp_rx(struct eth_avb *eth, struct net_rx_desc *desc, void *hdr)
{
	struct net_rx_stats *stats = &ptype_hdlr[PTYPE_MRP].stats[desc->port];
	struct net_socket *sock;
	unsigned long flags;
	int rc;

	raw_spin_lock_irqsave(&ptype_lock, flags);

	sock = mrp_rx_hdlr[desc->port].sock;
	if (!sock)
		goto slow;

	rc = net_rx(eth, sock, desc, stats);

	raw_spin_unlock_irqrestore(&ptype_lock, flags);

	return rc;

slow:
	raw_spin_unlock_irqrestore(&ptype_lock, flags);

	return net_rx_slow(eth, desc, stats);
}
