/*
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
 * Copyright 2017-2024 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <linux/netdevice.h>
#include <linux/fec.h>

#include "genavb/qos.h"

#include "net_port.h"
#include "avbdrv.h"
#include "debugfs.h"

#include "ptp.h"

int eth_avb_port_status(struct eth_avb *eth, struct net_port_status *status)
{
	struct net_device *ndev;
	unsigned long flags;
	int rc = 0;

	raw_spin_lock_irqsave(&eth->lock, flags);

	if (eth->flags & PORT_FLAGS_ENABLED) {
		raw_spin_unlock_irqrestore(&eth->lock, flags);

		ndev = dev_get_by_index(&init_net, eth->ifindex);
		if (!ndev) {
			rc = -EINVAL;
			goto out;
		}

		if (netif_running(ndev) && netif_carrier_ok(ndev)) {
			status->up = true;
			status->full_duplex = true;
			status->rate = eth->rate;
		} else {
			status->up = false;
			status->full_duplex = false;
			status->rate = 0;
		}

		dev_put(ndev);
	} else {
		raw_spin_unlock_irqrestore(&eth->lock, flags);

		status->up = false;
		status->full_duplex = false;
		status->rate = 0;
	}

out:
	return rc;
}

static int __eth_avb_multi(struct eth_avb *eth, struct net_mc_address *addr, int add)
{
	struct net_device *ndev;
	int rc;

	ndev = dev_get_by_index(&init_net, eth->ifindex);
	if (!ndev) {
		rc = -EINVAL;
		goto out;
	}

	if (add)
		rc = dev_mc_add(ndev, addr->hw_addr);
	else
		rc = dev_mc_del(ndev, addr->hw_addr);

	dev_put(ndev);

out:
	return rc;
}

int eth_avb_add_multi(struct eth_avb *eth, struct net_mc_address *addr)
{
	return __eth_avb_multi(eth, addr, 1);
}

int eth_avb_del_multi(struct eth_avb *eth, struct net_mc_address *addr)
{
	return __eth_avb_multi(eth, addr, 0);
}

int eth_avb_read_ptp(struct eth_avb *eth, unsigned int *value)
{
	unsigned long flags;
	int rc = 0;

	raw_spin_lock_irqsave(&eth->lock, flags);

	if (!(eth->flags & PORT_FLAGS_ENABLED)) {
		rc = -1;
		goto exit;
	}

	rc = fec_ptp_read_cnt(eth->fec_data, value);

exit:
	raw_spin_unlock_irqrestore(&eth->lock, flags);

	return rc;
}

static int eth_avb_tx(void *data, struct avb_tx_desc *desc)
{
	struct eth_avb *eth = data;
	struct queue *queue;
	unsigned long flags;
	int rc;

//	pr_info("%s() %p %p %p\n", __func__, data, arg, eth->fec_data);

	raw_spin_lock_irqsave(&eth->lock, flags);

	if (!(eth->flags & PORT_FLAGS_ENABLED)) {
		pr_err("%s: %p port closed\n", __func__, eth);
		rc = -2;
		goto out;
	}

	queue = &eth->tx_queue;

	rc = queue_enqueue(queue, (unsigned long)desc);
	if (rc < 0) {
		rc = -2;
		goto out;
	}

	set_bit(eth->tx_qos_queue->index, &eth->tx_qos_queue->tc->shared_pending_mask);

	if (queue_full(queue) || (queue_available(&eth->tx_cleanup_queue) <= TX_CLEANUP_QUEUE_MAX_AVAIL))
		rc = -1;

out:
	raw_spin_unlock_irqrestore(&eth->lock, flags);

	return rc;
}

static int eth_avb_tx_full(void *data)
{
	struct eth_avb *eth = data;
	struct queue *queue = &eth->tx_queue;

	/* At any point the "tx cleanup queue" needs to have enough free space for the entire hardware ring buffer and
	 * the best effort transmit queues */
	return (queue_full(queue) || (queue_available(&eth->tx_cleanup_queue) <= TX_CLEANUP_QUEUE_MIN_AVAIL));
}

static int eth_avb_rx_irq(void *data, struct avb_rx_desc *arg)
{
	struct eth_avb *eth = data;
	struct net_rx_desc *desc = (struct net_rx_desc *)arg;
	int rc;

//	pr_info("%s\n", __func__);

	rc = eth_rx(eth, desc);
	if (likely(rc == AVB_NET_RX_OK))
		return 0;
	else if (rc == AVB_NET_RX_WAKE)
		return AVB_WAKE_THREAD;
	else if (rc == AVB_NET_RX_DROP)
		return 0;

	return AVB_WAKE_NAPI;
}

static int eth_avb_rx(void *data, struct avb_rx_desc *arg)
{
	return eth_avb_rx_irq(data, arg);
}

static void *eth_avb_alloc(void *data)
{
	struct eth_avb *eth = data;
	struct avb_rx_desc *desc;

	void *buf = pool_dma_alloc(eth->buf_pool);

//	pr_info("%s: %p %p\n", __func__, data, buf);
	if (buf) {
		desc = (struct avb_rx_desc *)buf;
		desc->common.offset = NET_DATA_OFFSET;
		desc->common.pool_type = POOL_TYPE_AVB;

		desc->dma_addr = pool_dma_virt_to_dma(eth->buf_pool, (void *)desc + desc->common.offset, eth->port);
	}


	return buf;
}

static int eth_avb_tx_cleanup(void *data, struct avb_tx_desc *desc)
{
	struct eth_avb *eth = data;
	int rc;

//	pr_info("%s: %p %p\n", __func__, data, desc);

	/* We need to stop transmit before this queue is full */
	rc = queue_enqueue(&eth->tx_cleanup_queue, (unsigned long)desc);
	if (rc < 0)
		pr_err("%s: tx cleanup too slow %d\n", __func__, queue_pending(&eth->tx_cleanup_queue));

	return rc;
}

static int eth_avb_tx_cleanup_ready(void *data)
{
	struct eth_avb *eth = data;
	unsigned int pending = queue_pending(&eth->tx_cleanup_queue);

	if (!pending) {
		eth->count = 0;
		return 0;
	}

	eth->count++;

	if ((pending > 8) || (eth->count > 8)) {
		eth->count = 0;
		return 1;
	} else
		return 0;
}

static void *eth_avb_tx_cleanup_dequeue(void *data)
{
	struct eth_avb *eth = data;
	void *buf = (void *)queue_dequeue(&eth->tx_cleanup_queue);

//	pr_info("%s: %p %p\n", __func__, data, buf);

	return buf;
}

static void eth_avb_free(void *data, struct avb_desc *desc)
{
	struct eth_avb *eth = data;

//	pr_info("%s: %p %p\n", __func__, data, buf);

	pool_dma_free(eth->buf_pool, desc);
}

static int eth_avb_enqueue_tx_ts(void *data, struct avb_desc *desc)
{
	struct eth_avb *eth = data;
	struct net_tx_desc *tx_desc = (struct net_tx_desc *)desc;
	struct logical_port *port = physical_to_logical_port(eth);

	tx_desc->ts64 = tx_desc->ts;
	return ptp_tx_ts(port, tx_desc);
}

static void *eth_avb_dequeue(void *data)
{
	struct eth_avb *eth = data;
	void *buf = (void *)queue_dequeue(&eth->rx_queue);

//	pr_info("%s: %p %p\n", __func__, data, buf);

	return buf;
}

static void eth_avb_open(void *data, void *arg, int phy_speed)
{
	struct eth_avb *eth = data;
	unsigned long flags;

//	pr_info("%s: %p %p rate : %d Mbps\n", __func__, data, arg, phy_speed);

	raw_spin_lock_irqsave(&eth->lock, flags);

	if (!(eth->flags & PORT_FLAGS_ENABLED)) {
		eth->fec_data = arg;
		eth->qos->fec_data = arg;
		eth->rate = phy_speed * 1000000 * 8;
		net_qos_port_reset(eth->qos, phy_speed * 1000000);
		eth->flags |= PORT_FLAGS_ENABLED;
	} else
		pr_err("%s: %p already open\n", __func__, eth);

	raw_spin_unlock_irqrestore(&eth->lock, flags);
}


static void eth_avb_close(void *data)
{
	struct eth_avb *eth = data;
	unsigned long flags;

//	pr_info("%s: %p\n", __func__, data);

	raw_spin_lock_irqsave(&eth->lock, flags);

	if (eth->flags & PORT_FLAGS_ENABLED) {
		eth->fec_data = NULL;
		eth->qos->fec_data = NULL;
		eth->flags &= ~PORT_FLAGS_ENABLED;
	} else
		pr_err("%s: %p already closed\n", __func__, eth);

	net_qos_port_flush(eth->qos);

	raw_spin_unlock_irqrestore(&eth->lock, flags);
}

const static struct avb_ops avb_ops = {
	.open = eth_avb_open,
	.close = eth_avb_close,

	.alloc = eth_avb_alloc,
	.free = eth_avb_free,

	.rx = eth_avb_rx,
	.dequeue = eth_avb_dequeue,

	.tx = eth_avb_tx,
	.tx_full = eth_avb_tx_full,

	.tx_cleanup = eth_avb_tx_cleanup,
	.tx_cleanup_ready = eth_avb_tx_cleanup_ready,
	.tx_cleanup_dequeue = eth_avb_tx_cleanup_dequeue,

	.tx_ts = eth_avb_enqueue_tx_ts,

	.owner = THIS_MODULE,
};


void queue_free_tx_skb(void *data, unsigned long entry)
{
	struct avb_tx_desc *desc = (struct avb_tx_desc *)entry;
	struct sk_buff *skb = desc->data;

	kfree_skb(skb);
}


int eth_avb_init(struct eth_avb *eth, struct pool_dma *buf_pool, struct net_qos *qos, struct dentry *avb_dentry)
{
	int i;

//	pr_info("%s: %p\n", __func__, eth);

	raw_spin_lock_init(&ptype_lock);

	for (i = 0; i < CFG_PORTS; i++) {
		queue_init(&eth[i].rx_queue, pool_dma_free_virt, CFG_RX_EXTRA_ENTRIES);

		queue_init(&eth[i].tx_cleanup_queue, pool_dma_free_virt, CFG_TX_CLEANUP_EXTRA_ENTRIES);

		eth[i].buf_pool = buf_pool;
		eth[i].port = i;
		eth[i].qos = &qos->port[i];
		eth[i].ifindex = -1;
		qos->port[i].eth = &eth[i];

		queue_init(&eth[i].tx_queue, queue_free_tx_skb, 0);

		raw_spin_lock_init(&eth[i].lock);

		eth[i].dev = fec_enet_avb_get_device(physical_port_name(i));

		/* If no avb device on this port, skip. */
		if (!eth[i].dev)
			continue;

		eth[i].tx_qos_queue = qos_queue_connect(eth[i].qos, QOS_BEST_EFFORT_PRIORITY, &eth[i].tx_queue, 0);
		if (eth[i].tx_qos_queue == NULL)
			goto err_config;

		if (pool_dma_map(buf_pool, eth[i].dev, i) < 0)
			goto err_mapping;

		eth[i].ifindex = fec_enet_avb_register(physical_port_name(i), &avb_ops, &eth[i]);

		net_qos_map_traffic_class_to_hw_queues(eth[i].qos);
	}

	net_rx_debugfs_init(eth, avb_dentry);

	return 0;

err_mapping:
err_config:
	while (i--)
		pool_dma_unmap(eth[i].buf_pool, i);

	return -1;
}


void eth_avb_exit(struct eth_avb *eth)
{
	int i;

//	pr_info("%s: %p\n", __func__, eth);

	for (i = 0; i < CFG_PORTS; i++) {
		/* If no avb device on this port, skip. */
		if (!eth[i].dev)
			continue;

		fec_enet_avb_unregister(eth[i].ifindex, &avb_ops);
		pool_dma_unmap(eth[i].buf_pool, i);
		qos_queue_disconnect(eth->qos, eth->tx_qos_queue);
	}
}
