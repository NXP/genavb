/*
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
 * Copyright 2017-2018, 2020-2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "net_rx.h"

#include "genavb/ether.h"

#include "ptp.h"
#include "mrp.h"
#include "avtp.h"

struct ptype_handler ptype_hdlr[PTYPE_MAX];

raw_spinlock_t ptype_lock;

int net_rx_thread(struct net_drv *drv)
{
	struct net_socket *sock[SOCKET_MAX_RX];
	unsigned long flags;
	unsigned int i, n = 0;

//	pr_info("%s\n", __func__);

	raw_spin_lock_irqsave(&ptype_lock, flags);

	generic_rx_wakeup_all(drv, sock, &n);

	raw_spin_unlock_irqrestore(&ptype_lock, flags);

	for (i = 0; i < n; i++) {
		wake_up(&sock[i]->wait);
		clear_bit(SOCKET_ATOMIC_FLAGS_BUSY, &sock[i]->atomic_flags);
	}

	return 0;
}

int net_tx_thread(struct eth_avb *eth)
{
	int i;
//	pr_info("%s\n", __func__);

	for (i = 0; i < eth->num_logical_ports; i++) {
		struct logical_port *port = eth->logical_port[i];

		ptp_tx_ts_wakeup(port);
	}

	return 0;
}

int net_tx_available_thread(struct eth_avb *eth)
{
	struct net_socket *sock[SOCKET_MAX_RX];
	unsigned int i, n = 0;
	unsigned long flags;

	for (i = 0; i < eth->num_logical_ports; i++) {
		struct logical_port *port = eth->logical_port[i];

		raw_spin_lock_irqsave(&eth->lock, flags);
		avtp_tx_wakeup(port, sock, &n);
		raw_spin_unlock_irqrestore(&eth->lock, flags);
	}

	for (i = 0; i < n; i++)
		wake_up(&sock[i]->wait);

	return 0;
}

int net_rx_slow(struct eth_avb *eth, struct net_rx_desc *desc, struct net_rx_stats *stats)
{
	if (queue_enqueue(&eth->rx_queue, (unsigned long)desc) < 0) {
		stats->slow_dropped++;
		pool_dma_free(eth->buf_pool, desc);
	} else
		stats->slow++;

	return AVB_NET_RX_SLOW;
}

int net_rx_drop(struct eth_avb *eth, struct net_rx_desc *desc, struct net_rx_stats *stats)
{
	stats->dropped++;
	pool_dma_free(eth->buf_pool, desc);

	return AVB_NET_RX_DROP;
}

static inline int vlan_rx(struct eth_avb *eth, struct net_rx_desc *desc, void *hdr)
{
	struct vlanhdr *vlan = hdr;
	unsigned int ether_type = vlan->type;

	desc->vid = VLAN_VID(vlan);

	desc->l3_offset = desc->l2_offset + sizeof(struct eth_hdr) + sizeof(struct vlanhdr);

	if (likely(ether_type == htons(ETHERTYPE_AVTP)))
		return avtp_rx(eth, desc, vlan + 1, 1);

	return net_rx_slow(eth, desc, &ptype_hdlr[PTYPE_OTHER].stats[desc->port]);
}

static inline unsigned int eth_rx_get_logical_port(struct eth_avb *eth, struct net_rx_desc *desc)
{
	/* If descriptor embeds port information (like DSA tag) to identify logical port,
	 * use that. Otherwise, use the default/first mapped logical port of the
	 * physical interface.
	 */
	return eth->logical_port[CFG_DEFAULT_LOGICAL_PORT_MAP_INDEX]->id;
}

int eth_rx(struct eth_avb *eth, struct net_rx_desc *desc)
{
	struct eth_hdr *ethhdr = (void *)desc + desc->l2_offset;
	unsigned int ether_type = ethhdr->type;

	/* The port id in the descriptor should contain the logical port id */
	desc->port = eth_rx_get_logical_port(eth, desc);

	if (likely(ether_type == htons(ETHERTYPE_VLAN)))
		return vlan_rx(eth, desc, ethhdr + 1);

	desc->ethertype = ntohs(ether_type);
	desc->l3_offset = desc->l2_offset + sizeof(struct eth_hdr);

	switch (desc->ethertype) {
	case ETHERTYPE_AVTP:
		return avtp_rx(eth, desc, ethhdr + 1, 0);
		break;

	case ETHERTYPE_PTP:
		return ptp_rx(eth, desc, ethhdr + 1);
		break;

	case ETHERTYPE_MSRP:
	case ETHERTYPE_MVRP:
	case ETHERTYPE_MMRP:
		return mrp_rx(eth, desc, ethhdr + 1);
		break;

	default:
		break;
	}

	return net_rx_slow(eth, desc, &ptype_hdlr[PTYPE_OTHER].stats[desc->port]);
}
