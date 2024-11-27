/*
 * Copyright 2018-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Packet socket management functions
 @details
*/

#include "l2.h"
#include "net_port.h"
#include "net_logical_port.h"
#include "net_socket.h"
#include "clock.h"

#include "os/net.h"

#include "genavb/qos.h"

static struct l2_rx_hdlr l2_rx_hdlr[CFG_ENDPOINT_NUM];

static struct net_socket *
l2_hdlr_find(struct l2_rx_hdlr *hdlr, void *dst_mac, uint16_t vlan_id)
{
	struct slist_node *entry;
	struct slist_head *head;
	struct net_socket *sock;
	unsigned int hash;

	hash = l2_hash(dst_mac, vlan_id);
	head = &hdlr->sock_list_head[hash];

	for (entry = slist_first(head); !slist_is_last(entry); entry = slist_next(entry)) {
		sock = container_of(entry, struct net_socket, list_node);

		if (!memcmp(sock->addr.u.l2.dst_mac, dst_mac, 6)
		&& (sock->addr.vlan_id == vlan_id))
			return sock;
	}

	return NULL;
}

static int
l2_hdlr_add(struct l2_rx_hdlr *hdlr, struct net_socket *sock,
					void *dst_mac, uint16_t vlan_id)
{
	unsigned int hash;

	hash = l2_hash(dst_mac, vlan_id);

	slist_add_head(&hdlr->sock_list_head[hash], &sock->list_node);

	return 0;
}

static void
l2_hdlr_del(struct l2_rx_hdlr *hdlr, struct net_socket *sock)
{
	unsigned int hash;
	struct slist_head *head;

	hash = l2_hash(sock->addr.u.l2.dst_mac,	sock->addr.vlan_id);

	head = &hdlr->sock_list_head[hash];

	slist_del(head, &sock->list_node);
}

void l2_socket_unbind(struct net_socket *sock)
{
	l2_hdlr_del(&l2_rx_hdlr[sock->addr.port], sock);
}

int l2_socket_bind(struct net_socket *sock, struct net_address *addr)
{
	struct logical_port *port;

	port = logical_port_get(addr->port);
	if (!port)
		goto err;

	if (port->is_bridge)
		goto err;

	if (l2_hdlr_find(&l2_rx_hdlr[addr->port],
				addr->u.l2.dst_mac,
				addr->vlan_id))
		goto err;

	sock->clock = PORT_CLOCK_GPTP_0;
	sock->flags |= SOCKET_FLAGS_RX_TS_ENABLED;

	return l2_hdlr_add(&l2_rx_hdlr[addr->port], sock,
				addr->u.l2.dst_mac,
				addr->vlan_id);
err:
	return -1;
}

void l2_socket_disconnect(struct logical_port *port, struct net_socket *sock)
{
	struct net_port *net_port = port->phys;

	if (!port->is_bridge) {
		if (sock->tx.qos_queue) {
			qos_queue_disconnect(net_port->qos, sock->tx.qos_queue);
			sock->tx.qos_queue = NULL;
		}
	}

	sock->tx.func = NULL;
}

int l2_socket_connect(struct logical_port *port, struct net_socket *sock, struct net_address *addr)
{
	struct net_port *net_port = port->phys;

	if (addr->vlan_id != htons(VLAN_VID_NONE)) {
		sock->flags |= SOCKET_FLAGS_VLAN;
		sock->tx.vlan_label = VLAN_LABEL(ntohs(addr->vlan_id), addr->priority, 0);
	}

	if (!port->is_bridge) {
		sock->tx.qos_queue = qos_queue_connect(net_port->qos, addr->priority, &sock->tx.queue, 0);
		if (!sock->tx.qos_queue)
			goto err;

		/* Provide a direct transmit path for devices with insufficient
		 * hardware queues
		 */
		if ((sock->tx.qos_queue->tc->flags & TC_FLAGS_HW_SP) ||
		    (addr->priority == ISOCHRONOUS_DEFAULT_PRIORITY))
			sock->tx.func = socket_port_tx;
		else
			sock->tx.func = socket_qos_enqueue;
	} else {
		sock->tx.func = socket_bridge_port_tx;
	}

	sock->tx.flags = NET_TX_FLAGS_TS64;

	return 0;

err:
	return -1;
}

int l2_rx(struct net_rx_ctx *net, struct logical_port *port, struct net_rx_desc *desc)
{
	struct eth_hdr *eth_hdr = (struct eth_hdr *)((uint8_t *)desc + desc->l2_offset);
	struct net_rx_stats *stats = &net->ptype_hdlr[PTYPE_L2].stats[desc->port];
	struct net_rx_stats *slow_stats = &net->ptype_hdlr[PTYPE_OTHER].stats[desc->port];
	struct net_socket *sock;

	if (!port)
		goto drop;

	if (port->is_bridge)
		goto slow;

	sock = l2_hdlr_find(&l2_rx_hdlr[port->id], eth_hdr->dst, htons(desc->vid));
	if (!sock)
		goto slow;

	return socket_rx(net, sock, port, desc, stats);

slow:
	return net_rx_slow(net, port, desc, slow_stats);

drop:
	return net_rx_drop(net, desc, slow_stats);
}
