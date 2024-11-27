/*
 * Copyright 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <linux/sched.h>

#include "genavb/ether.h"
#include "genavb/qos.h"
#include "genavb/61883_iidc.h"
#include "genavb/acf.h"
#include "genavb/crf.h"

#include "avtp.h"
#include "net_socket.h"

struct avtp_rx_hdlr avtp_rx_hdlr;
struct avtp_tx_hdlr avtp_tx_hdlr;

unsigned int avtp_dt_bin_width_shift[SR_CLASS_MAX] = {
	[SR_CLASS_A] = 17, /*131072 ns*/
	[SR_CLASS_B] = 19, /*524288 ns*/
	[SR_CLASS_C] = 19,
	[SR_CLASS_D] = 19,
	[SR_CLASS_E] = 19
};

void avtp_tx_wakeup(struct logical_port *port, struct net_socket **sock_array, unsigned int *n)
{
	struct avtp_stream_tx_hdlr *stream_tx_hdlr = &avtp_tx_hdlr.stream[port->id];
	struct net_socket *sock;
	struct hlist_node *node;
	unsigned int hash;

	for (hash = 0; hash < STREAM_HASH; hash++) {
		hlist_for_each(node, &stream_tx_hdlr->sock_head[hash]) {
			sock = hlist_entry(node, struct net_socket, node);

			if (test_bit(SOCKET_ATOMIC_FLAGS_SOCKET_WAITING_EVENT, &sock->qos_queue->atomic_flags)) {
				/* at least 25% of the queue is free */
				if (queue_available(&sock->queue) >= (sock->queue.size >> 2)) {
					socket_schedule_wake_up(sock, sock_array, n);
				}
			}
		}
	}
}

static inline unsigned int stream_has_sph_quick(struct net_socket *sock, struct avtp_data_hdr *avtp)
{
	return sock->flags & SOCKET_FLAGS_WITH_SPH;
}

static inline struct net_socket *stream_hdlr_find(struct hlist_head *sock_head, void *stream_id)
{
	struct hlist_node *node;
	struct net_socket *sock;
	unsigned int hash;

	hash = stream_hash(stream_id);

	hlist_for_each(node, &sock_head[hash]) {
		sock = hlist_entry(node, struct net_socket, node);

		if (stream_id_match(sock->addr.u.avtp.stream_id, stream_id))
			return sock;
	}

	return NULL;
}


#ifdef CFG_NET_STREAM_STATS
static void avtp_stream_init_stats(struct net_socket *sock)
{
	stream_init_stats(sock);

	sock->avtp.dt_min = 0x7fffffff;
	sock->avtp.dt_max = -0x7fffffff;

	memset(sock->avtp.dt_bin, 0, sizeof(sock->avtp.dt_bin));
}

static void avtp_stream_update_stats(struct net_socket *sock, struct net_rx_desc *desc)
{
	struct avtp_data_hdr *avtp = (void *)desc + desc->l3_offset;
	int dt;
	unsigned int bin, avtp_timestamp;

	stream_update_stats(sock, desc, dt_bin_width_shift[sock->addr.u.avtp.sr_class]);

	if ((avtp->tv) || stream_has_sph_quick(sock, avtp)) {
		avtp_timestamp = ntohl(avtp->avtp_timestamp);

		dt = (int)avtp_timestamp - (int)desc->ts;

		if (dt < sock->avtp.dt_min)
			sock->avtp.dt_min = dt;
		else if (dt > sock->avtp.dt_max)
			sock->avtp.dt_max = dt;

		if (dt < 0)
			bin = 0;
		else {
			bin = 1 + (dt >> avtp_dt_bin_width_shift[sock->addr.u.avtp.sr_class]);

			if (bin >= NET_STATS_BIN_MAX)
				bin = NET_STATS_BIN_MAX - 1;
		}

		sock->avtp.dt_bin[bin]++;
	}
}
#else
static void avtp_stream_update_stats(struct net_socket *sock, struct net_rx_desc *desc)
{
}

static void avtp_stream_init_stats(struct net_socket *sock)
{
}
#endif


static int stream_hdlr_add(struct hlist_head *sock_head, struct net_socket *sock, void *stream_id)
{
	unsigned int hash;

	hash = stream_hash(stream_id);

	hlist_add_head(&sock->node, &sock_head[hash]);

	avtp_stream_init_stats(sock);

	return 0;
}

static void stream_hdlr_del(struct hlist_head *sock_head, struct net_socket *sock)
{
	hlist_del(&sock->node);
}

void avtp_socket_unbind(struct net_socket *sock)
{
	struct avtp_address *addr = &sock->addr.u.avtp;

	if (is_avtp_stream(addr->subtype) || is_avtp_alternative(addr->subtype)) {
		stream_hdlr_del(avtp_rx_hdlr.stream[sock->addr.port].sock_head, sock);
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
	if (!logical_port_valid(addr->port))
		goto err;

	if (logical_port_is_bridge(addr->port))
		goto err;

	/* All alternative formats have a 64bit id (even if it's not a proper stream id)*/
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
	struct eth_avb *eth = port->phys;
	struct net_address *addr = &sock->addr;
	struct avtp_stream_tx_hdlr *hdlr;

	sock->tx_func = NULL;

	if (is_avtp_stream(addr->u.avtp.subtype) || is_avtp_alternative(addr->u.avtp.subtype)) {
		hdlr = &avtp_tx_hdlr.stream[port->id];

		if (sock->addr.u.avtp.sr_class == SR_CLASS_NONE)
			qos_queue_disconnect(eth->qos, sock->qos_queue);
		else
			net_qos_stream_disconnect(eth->qos, sock->qos_queue);

		stream_hdlr_del(hdlr->sock_head, sock);
	} else if (is_avtp_avdecc(addr->u.avtp.subtype)) {
		qos_queue_disconnect(eth->qos, sock->qos_queue);
	}

	sock->qos_queue = NULL;
}

static int avtp_tx(struct net_socket *sock, struct logical_port *port, unsigned long *desc_array, unsigned int *n)
{
	struct avb_tx_desc *desc;
	unsigned int flags;
	unsigned int write;
	unsigned int available;
	struct qos_queue *qos_q = sock->qos_queue;
	int rc = 0, i;

	available = queue_available(&sock->queue);
	if (unlikely(*n > available)) {
		qos_q->full += *n - available;
		qos_q->dropped += *n - available;
		*n = available;
		rc = -EIO;
		if (!available)
			goto out;
	}

	queue_enqueue_init(&sock->queue, &write);

	for (i = 0; i < *n; i++) {
		desc = (void *)desc_array[i];

		flags = desc->common.flags;

		desc->common.flags = 0;

		if (flags & NET_TX_FLAGS_HW_CSUM)
			desc->common.flags |= AVB_TX_FLAG_HW_CSUM;

		if (flags & NET_TX_FLAGS_TS)
			desc->common.flags |= AVB_TX_FLAG_TS;

		queue_enqueue_next(&sock->queue, &write, (unsigned long)desc);
	}

	queue_enqueue_done(&sock->queue, write);

	set_bit(qos_q->index, &qos_q->tc->shared_pending_mask);

	*n = i;
out:
	return rc;
}

static int avdecc_tx(struct net_socket *sock, struct logical_port *port, unsigned long *desc_array, unsigned int *n)
{
	struct avb_tx_desc *desc;
	unsigned int flags;
	int rc = 0, i;

	for (i = 0; i < *n; i++) {
		desc = (void *)desc_array[i];

		flags = desc->common.flags;

		desc->common.flags = 0;

		if (flags & NET_TX_FLAGS_TS)
			desc->common.flags |= AVB_TX_FLAG_TS;

		if ((rc = socket_qos_enqueue(sock, sock->qos_queue, desc)) < 0)
			break;
	}

	*n = i;

	return rc;
}

int avtp_socket_connect(struct logical_port *port, struct net_socket *sock, struct net_address *addr)
{
	struct eth_avb *eth = port->phys;
	struct avtp_stream_tx_hdlr *hdlr;
	u8 priority;

	if (logical_port_is_bridge(port->id))
		goto err;

	if (is_avtp_stream(addr->u.avtp.subtype) || is_avtp_alternative(addr->u.avtp.subtype)) {
		if (addr->u.avtp.sr_class == SR_CLASS_NONE) {
			if (addr->vlan_id != htons(VLAN_VID_NONE)) {
				/* FIXME check if it conflicts with SRP domain priorities */
				sock->flags |= SOCKET_FLAGS_VLAN;
				sock->vlan_label = VLAN_LABEL(ntohs(addr->vlan_id), addr->priority, 0);

				priority = addr->priority;
			} else
				priority = QOS_BEST_EFFORT_PRIORITY;

			sock->qos_queue = qos_queue_connect(eth->qos, priority, &sock->queue, 0);
			if (!sock->qos_queue)
				goto err;

		} else {
			if (!sr_class_enabled(addr->u.avtp.sr_class))
				goto err;

			/* Associates a transmit queue with a specific AVTP stream id/port/class */
			sock->qos_queue = net_qos_stream_connect(eth->qos, addr->u.avtp.sr_class, addr->u.avtp.stream_id, &sock->queue);
			if (!sock->qos_queue)
				goto err;

			sock->flags |= SOCKET_FLAGS_VLAN;

			sock->vlan_label = sock->qos_queue->stream->vlan_label;	/* FIXME use SRP domain information? */
		}

		sock->tx_func = avtp_tx;

		hdlr = &avtp_tx_hdlr.stream[port->id];

		if (stream_hdlr_find(hdlr->sock_head, &addr->u.avtp.stream_id))
			goto err;

		stream_hdlr_add(hdlr->sock_head, sock, &addr->u.avtp.stream_id);
	} else if (is_avtp_avdecc(addr->u.avtp.subtype)) {
		sock->qos_queue = qos_queue_connect(eth->qos, addr->priority, &sock->queue, 0);
		if (!sock->qos_queue)
			goto err;

		sock->tx_func = avdecc_tx;
	} else {
		goto err;
	}

	sock->flags |= SOCKET_FLAGS_FLOW_CONTROL;

	return 0;

err:
	return -1;
}

static inline unsigned int stream_has_sph(struct net_socket *sock, struct avtp_data_hdr *avtp)
{
	if (sock->flags & SOCKET_FLAGS_SPH_MASK)
		return stream_has_sph_quick(sock, avtp);
	else {
		struct iec_61883_iidc_specific_hdr spec_hdr;
		struct iec_61883_hdr *iec_hdr = (struct iec_61883_hdr *)(avtp + 1);

		spec_hdr.u.raw = avtp->protocol_specific_header;

		if (unlikely((avtp->subtype == AVTP_SUBTYPE_61883_IIDC) &&
			(spec_hdr.u.s.tag == IEC_61883_PSH_TAG_CIP_INCLUDED) &&
			(iec_hdr->sph == IEC_61883_CIP_SPH_SOURCE_PACKETS)))
			sock->flags |= SOCKET_FLAGS_WITH_SPH;
		else
			sock->flags |= SOCKET_FLAGS_WITHOUT_SPH;

		return sock->flags & SOCKET_FLAGS_WITH_SPH;
	}

}

static void stream_update_ts(struct net_socket *sock, struct net_rx_desc *desc)
{
	struct avtp_data_hdr *avtp = (void *)desc + desc->l3_offset;

	if (stream_has_sph(sock, avtp))
		avtp->avtp_timestamp = *(u32 *)((void *)avtp + sizeof(struct avtp_data_hdr) + sizeof(struct iec_61883_hdr));

	avtp_stream_update_stats(sock, desc);
}

static inline int avtp_alternative_rx(struct eth_avb *eth, struct net_rx_desc *desc, struct avtp_common_hdr *avtp, struct net_rx_stats *stats)
{
	struct net_socket *sock;
	unsigned long flags;
	void *stream_id;
	int rc;

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

	raw_spin_lock_irqsave(&ptype_lock, flags);

	sock = stream_hdlr_find(avtp_rx_hdlr.stream[desc->port].sock_head, stream_id);
	if (!sock)
		goto slow_unlock;

	stream_update_stats(sock, desc, dt_bin_width_shift[sock->addr.u.avtp.sr_class]);

	rc = net_rx(eth, sock, desc, stats);

	raw_spin_unlock_irqrestore(&ptype_lock, flags);

	return rc;

slow_unlock:
	raw_spin_unlock_irqrestore(&ptype_lock, flags);

slow:
	return net_rx_slow(eth, desc, stats);
}

static inline int avtp_control_rx(struct eth_avb *eth, struct net_rx_desc *desc, struct avtp_common_hdr *avtp, struct net_rx_stats *stats)
{
	struct net_socket *sock;

	switch (avtp->subtype) {
	case AVTP_SUBTYPE_MAAP:
	{
		struct maap_rx_hdlr *maap = &avtp_rx_hdlr.maap;

		sock = maap->rx_hdlr[desc->port].sock;
		if (!sock)
			goto slow;

		return net_rx(eth, sock, desc, &maap->stats[desc->port]);
		break;
	}
	case AVTP_SUBTYPE_ADP:
	case AVTP_SUBTYPE_AECP:
	case AVTP_SUBTYPE_ACMP:
	{
		struct avdecc_rx_hdlr *avdecc = &avtp_rx_hdlr.avdecc;

		sock = avdecc->rx_hdlr[desc->port].sock;
		if (!sock)
			goto slow;

		return net_rx(eth, sock, desc, &avdecc->stats[desc->port]);
		break;
	}
	default:
		break;
	}

slow:
	return net_rx_slow(eth, desc, stats);
}

static inline int avtp_stream_rx(struct eth_avb *eth, struct net_rx_desc *desc, struct avtp_common_hdr *avtp, struct net_rx_stats *stats)
{
	struct avtp_stream_hdr *hdr = (struct avtp_stream_hdr *)avtp;
	struct net_socket *sock;
	unsigned long flags;
	int rc;

	if (unlikely(!hdr->sv))
		goto slow;

	raw_spin_lock_irqsave(&ptype_lock, flags);

	sock = stream_hdlr_find(avtp_rx_hdlr.stream[desc->port].sock_head, &hdr->stream_id);
	if (!sock)
		goto slow_unlock;

	stream_update_ts(sock, desc);

	rc = net_rx(eth, sock, desc, stats);

	raw_spin_unlock_irqrestore(&ptype_lock, flags);

	return rc;

slow_unlock:
	raw_spin_unlock_irqrestore(&ptype_lock, flags);

slow:
	return net_rx_slow(eth, desc, stats);
}

int avtp_rx(struct eth_avb *eth, struct net_rx_desc *desc, void *hdr, unsigned int is_vlan)
{
	struct avtp_common_hdr *avtp = hdr;
	struct net_rx_stats *stats = &ptype_hdlr[PTYPE_AVTP].stats[desc->port];

	/* 1722-2011 5.2.4 "shall be ignored", 1722rev1-2016 5.4.2.3 "shall discard" */
	if (unlikely(avtp->version != AVTP_VERSION_0))
		goto drop;

	desc->ethertype = ETHERTYPE_AVTP;

	if (likely(is_avtp_stream(avtp->subtype))) {

#ifndef CFG_CVF_MJPEG_SALSA_WA
		if (unlikely(!is_vlan))
			goto drop;
#endif
		return avtp_stream_rx(eth, desc, avtp, stats);
	} else if (is_avtp_alternative(avtp->subtype))
		return avtp_alternative_rx(eth, desc, avtp, stats);

	return avtp_control_rx(eth, desc, avtp, stats);

drop:
	return net_rx_drop(eth, desc, stats);
}
