/*
 * Copyright 2018-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVTP management functions
 @details
*/

#include "avtp.h"
#include "net_port.h"
#include "net_logical_port.h"
#include "clock.h"

#include "genavb/acf.h"
#include "genavb/crf.h"
#include "genavb/qos.h"

#include "os/net.h"
#include "slist.h"

#if defined(CONFIG_AVTP)
struct avtp_rx_hdlr avtp_rx_hdlr;
static struct avtp_tx_hdlr avtp_tx_hdlr;


static struct net_socket *stream_hdlr_find(struct slist_head *sock_head, void *stream_id)
{
	struct slist_node *entry;
	struct net_socket *sock;
	unsigned int hash;

	hash = stream_hash(stream_id);

	slist_for_each (&sock_head[hash], entry) {
		sock = container_of(entry, struct net_socket, list_node);

		if (cmp_64(sock->addr.u.avtp.stream_id, stream_id))
			return sock;
	}

	return NULL;
}

static int stream_hdlr_add(struct slist_head *sock_head, struct net_socket *sock, void *stream_id)
{
	unsigned int hash;

	hash = stream_hash(stream_id);

	slist_add_head(&sock_head[hash], &sock->list_node);

	//avtp_stream_init_stats(dev);

	return 0;
}

static void stream_hdlr_del(struct slist_head *sock_head, struct net_socket *sock)
{
	unsigned int hash;
	struct slist_head *head;

	hash = stream_hash(sock->addr.u.avtp.stream_id);

	head = &sock_head[hash];

	slist_del(head, &sock->list_node);
}

void avtp_socket_unbind(struct net_socket *sock)
{
	struct avtp_address *addr = &sock->addr.u.avtp;

	if (is_avtp_stream(addr->subtype) || is_avtp_alternative(addr->subtype)) {
		struct avtp_stream_rx_hdlr *hdlr;

		hdlr = &avtp_rx_hdlr.stream[sock->addr.port];

		stream_hdlr_del(hdlr->sock_head, sock);

	} else {
		switch (addr->subtype) {
		case AVTP_SUBTYPE_MAAP:
			generic_unbind(avtp_rx_hdlr.maap.rx_hdlr, sock, CFG_ENDPOINT_NUM);
			break;
		case AVTP_SUBTYPE_AVDECC:
			generic_unbind(avtp_rx_hdlr.avdecc.rx_hdlr, sock, CFG_ENDPOINT_NUM);
			break;

		default:
			break;
		}
	}
}

int avtp_socket_bind(struct net_socket *sock, struct net_address *addr)
{
	struct logical_port *port;

	port = logical_port_get(addr->port);
	if (!port)
		goto err;

	if (port->is_bridge)
		goto err;

	/* All alternative formats have a 64bit id (even if it's not a proper stream id) */
	if (is_avtp_stream(addr->u.avtp.subtype) || is_avtp_alternative(addr->u.avtp.subtype)) {
		struct avtp_stream_rx_hdlr *hdlr;

		if (!sr_class_enabled(addr->u.avtp.sr_class))
			goto err;

		hdlr = &avtp_rx_hdlr.stream[addr->port];

		if (stream_hdlr_find(hdlr->sock_head, &addr->u.avtp.stream_id))
			goto err;

		stream_hdlr_add(hdlr->sock_head, sock, &addr->u.avtp.stream_id);
	} else {
		switch (addr->u.avtp.subtype) {
		case AVTP_SUBTYPE_MAAP:
			generic_bind(avtp_rx_hdlr.maap.rx_hdlr, sock, addr->port, CFG_ENDPOINT_NUM);
			break;
		case AVTP_SUBTYPE_AVDECC:
			generic_bind(avtp_rx_hdlr.avdecc.rx_hdlr, sock, addr->port, CFG_ENDPOINT_NUM);
			break;

		default:
			goto err;
			break;
		}
	}

	return 0;

err:
	return -1;
}

void avtp_socket_disconnect(struct logical_port *port, struct net_socket *sock)
{
	struct avtp_stream_tx_hdlr *hdlr = &avtp_tx_hdlr.stream[port->id];

	sock->tx.func = NULL;

	if (is_avtp_stream(sock->addr.u.avtp.subtype) || is_avtp_alternative(sock->addr.u.avtp.subtype)) {
		if (sock->addr.u.avtp.sr_class == SR_CLASS_NONE)
			qos_queue_disconnect(port->phys->qos, sock->tx.qos_queue);
		else
			net_qos_stream_disconnect(port->phys->qos, sock->tx.qos_queue);

		stream_hdlr_del(hdlr->sock_head, sock);
	} else if (is_avtp_avdecc(sock->addr.u.avtp.subtype)) {
		qos_queue_disconnect(port->phys->qos, sock->tx.qos_queue);
	}

	sock->tx.qos_queue = NULL;
}

static int avtp_tx(struct net_socket *sock, struct logical_port *port, unsigned long *addr, unsigned int *n)
{
	struct net_tx_desc *desc;
	unsigned int flags;
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

		flags = desc->flags;

		desc->flags = 0;

		if (flags & NET_TX_FLAGS_TS)
			desc->flags |= NET_TX_FLAGS_TS;

		queue_enqueue_next(&sock->tx.queue, &write, (unsigned long)desc);
	}

	queue_enqueue_done(&sock->tx.queue, write);

	rtos_atomic_set_bit(qos_q->index, &qos_q->tc->shared_pending_mask);

	*n = i;
out:
	return rc;
}

static int avdecc_tx(struct net_socket *sock, struct logical_port *port, unsigned long *addr, unsigned int *n)
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

		if ((rc = socket_qos_enqueue(sock, port, sock->tx.qos_queue, desc)) < 0)
			break;
	}

	*n = i;

	return rc;
}

int avtp_socket_connect(struct logical_port *port, struct net_socket *sock, struct net_address *addr)
{
	struct net_port *net_port = port->phys;
	struct avtp_stream_tx_hdlr *hdlr;

	if (port->is_bridge)
		goto err;

	if (is_avtp_stream(addr->u.avtp.subtype) || is_avtp_alternative(addr->u.avtp.subtype)) {
		if (!sr_class_enabled(addr->u.avtp.sr_class))
			goto err;

		/* Associates a transmit queue with a specific AVTP stream id/port/class */
		sock->tx.qos_queue = net_qos_stream_connect(net_port->qos, addr->u.avtp.sr_class, addr->u.avtp.stream_id, &sock->tx.queue);
		if (!sock->tx.qos_queue) {
			goto err;
		}

		sock->flags |= SOCKET_FLAGS_VLAN;

		sock->tx.vlan_label = sock->tx.qos_queue->stream->vlan_label;	/* FIXME use SRP domain information? */

		sock->tx.func = avtp_tx;

		hdlr = &avtp_tx_hdlr.stream[addr->port];

		if (stream_hdlr_find(hdlr->sock_head, &addr->u.avtp.stream_id))
			goto err;

		stream_hdlr_add(hdlr->sock_head, sock, &addr->u.avtp.stream_id);

	} else if (is_avtp_avdecc(addr->u.avtp.subtype)) {

		sock->tx.qos_queue = qos_queue_connect(net_port->qos, addr->priority, &sock->tx.queue, 0);
		if (!sock->tx.qos_queue)
			goto err;

		sock->tx.func = avdecc_tx;
	} else
		goto err;

	return 0;

err:
	return -1;
}

static int avtp_stream_rx(struct net_rx_ctx *net, struct logical_port *port, struct net_rx_desc *desc, struct avtp_common_hdr *avtp, struct net_rx_stats *stats)
{
	struct avtp_stream_hdr *hdr = (struct avtp_stream_hdr *)avtp;
	struct net_socket *sock;

	sock = stream_hdlr_find(avtp_rx_hdlr.stream[port->id].sock_head, &hdr->stream_id);
	if (!sock)
		goto slow;

	return socket_rx(net, sock, desc, stats);

slow:
	return net_rx_slow(net, port, desc, stats);
}

static int avtp_alternative_rx(struct net_rx_ctx *net, struct logical_port *port, struct net_rx_desc *desc, struct avtp_common_hdr *avtp, struct net_rx_stats *stats)
{
	void *stream_id;
	struct net_socket *sock;

	switch (avtp->subtype) {
	case AVTP_SUBTYPE_NTSCF: {
		struct avtp_ntscf_hdr *ntscf_hdr = (struct avtp_ntscf_hdr *)avtp;

		stream_id = &ntscf_hdr->stream_id;

		break;
	}

	case AVTP_SUBTYPE_CRF: {
		struct avtp_crf_hdr *crf_hdr = (struct avtp_crf_hdr *)avtp;

		stream_id = &crf_hdr->stream_id;

		break;
	}

	default:
		goto slow;
		break;
	}

	sock = stream_hdlr_find(avtp_rx_hdlr.stream[port->id].sock_head, stream_id);
	if (!sock)
		goto slow;

	return socket_rx(net, sock, desc, stats);

slow:
	return net_rx_slow(net, port, desc, stats);
}

static int avtp_control_rx(struct net_rx_ctx *net, struct logical_port *port, struct net_rx_desc *desc, struct avtp_common_hdr *avtp, struct net_rx_stats *stats)
{
	struct net_socket *sock;

	switch (avtp->subtype) {
	case AVTP_SUBTYPE_MAAP:
	{
		struct maap_rx_hdlr *maap = &avtp_rx_hdlr.maap;

		sock = maap->rx_hdlr[port->id].sock;
		if (!sock)
			goto slow;

		return socket_rx(net, sock, desc, &maap->stats[desc->port]);
		break;
	}
	case AVTP_SUBTYPE_ADP:
	case AVTP_SUBTYPE_AECP:
	case AVTP_SUBTYPE_ACMP:
	{
		struct avdecc_rx_hdlr *avdecc = &avtp_rx_hdlr.avdecc;

		sock = avdecc->rx_hdlr[port->id].sock;
		if (!sock)
			goto slow;

		return socket_rx(net, sock, desc, &avdecc->stats[desc->port]);
		break;
	}
	default:
		break;
	}

slow:
	return net_rx_slow(net, port, desc, stats);
}

int avtp_rx(struct net_rx_ctx *net, struct logical_port *port, struct net_rx_desc *desc, void *hdr, unsigned int is_vlan)
{
	struct avtp_common_hdr *avtp = hdr;
	struct net_rx_stats *stats = &net->ptype_hdlr[PTYPE_AVTP].stats[desc->port];
	uint64_t gptp_time = 0;

	if (!port)
		goto drop;

	if (port->is_bridge)
		goto drop;

	// 1722-2011 5.2.4 "shall be ignored", 1722rev1-2016 5.4.2.3 "shall discard"
	if (unlikely(avtp->version != AVTP_VERSION_0))
		goto drop;

	clock_time_from_hw(port->phys->clock_gptp[0], desc->ts64, &gptp_time);
	desc->ts = gptp_time;
	desc->ts64 = gptp_time;

	if (likely(is_avtp_stream(avtp->subtype))) {

#ifndef CFG_CVF_MJPEG_SALSA_WA
		if (unlikely(!is_vlan))
			goto drop;
#endif
		return avtp_stream_rx(net, port, desc, avtp, stats);
	} else if (is_avtp_alternative(avtp->subtype))
		return avtp_alternative_rx(net, port, desc, avtp, stats);

	return avtp_control_rx(net, port, desc, avtp, stats);

drop:
	return net_rx_drop(net, desc, stats);
}
#elif defined(CONFIG_AVDECC) || defined(CONFIG_MAAP)
#error AVDECC/MAAP enabled without AVTP
#else
void avtp_socket_unbind(struct net_socket *sock)
{
}

int avtp_socket_bind(struct net_socket *sock, struct net_address *addr)
{
	return -1;
}

void avtp_socket_disconnect(struct logical_port *port, struct net_socket *sock)
{
}

int avtp_socket_connect(struct logical_port *port, struct net_socket *sock, struct net_address *addr)
{
	return -1;
}

int avtp_rx(struct net_rx_ctx *net, struct logical_port *port, struct net_rx_desc *desc, void *hdr, unsigned int is_vlan)
{
	struct net_rx_stats *stats = &net->ptype_hdlr[PTYPE_AVTP].stats[desc->port];

	return net_rx_slow(net, port, desc, stats);
}
#endif
