/*
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
 * Copyright 2017-2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <linux/netdevice.h>
#include <linux/dma-mapping.h>

#include "net_port.h"
#include "avbdrv.h"
#include "net_socket.h"

#include "genavb/ether.h"

#include "ptp.h"
#include "mrp.h"
#include "avtp.h"

unsigned int dt_bin_width_shift[SR_CLASS_MAX] = {
	[SR_CLASS_A] = 13, /*8192  ns*/
	[SR_CLASS_B] = 14, /*16384 ns*/
	[SR_CLASS_C] = 16, /*65536 ns*/
	[SR_CLASS_D] = 16,
	[SR_CLASS_E] = 16
};

#define SOCKET_MIN_PACKETS	1
#define SOCKET_MAX_PACKETS	QUEUE_ENTRIES_MAX
#define SOCKET_MAX_LATENCY	1000000000	/* 1 second */

int socket_set_option(struct net_socket *sock, struct net_set_option *option)
{
	unsigned long flags;
	int rc = 0;

	if (!(sock->flags & SOCKET_FLAGS_RX)) {
		rc = -EINVAL;
		goto out;
	}

	raw_spin_lock_irqsave(&ptype_lock, flags);

	switch (option->type) {
	case SOCKET_OPTION_BUFFER_PACKETS:
		if ((option->val < SOCKET_MIN_PACKETS) || (option->val > SOCKET_MAX_PACKETS)) {
			rc = -EINVAL;

			break;
		}

		sock->max_packets = option->val;

		break;

	case SOCKET_OPTION_BUFFER_LATENCY:
		if (option->val > SOCKET_MAX_LATENCY) {
			rc = -EINVAL;

			break;
		}

		sock->max_latency = option->val;

		break;

	default:
		rc = -EINVAL;
		break;
	}

	/* If socket already bound, move the socket to the right list and reset the batching flags (if necessary),
	 * otherwise leave it for the binding */
	if (sock->flags & SOCKET_FLAGS_BOUND) {
		if ((sock->max_packets > 1) && (sock->max_latency > 0) && !(sock->flags & SOCKET_FLAGS_RX_BATCHING)) {
			if (is_avtp_stream(sock->addr.u.avtp.subtype)) {
				list_move_tail(&sock->list, &sock->drv->batching_sync_list);
				sock->flags = (sock->flags & ~SOCKET_FLAGS_RX_BATCH_ANY) | SOCKET_FLAGS_RX_BATCHING_SYNC;
			} else {
				list_move_tail(&sock->list, &sock->drv->batching_async_list);
				sock->flags = (sock->flags & ~SOCKET_FLAGS_RX_BATCH_ANY) | SOCKET_FLAGS_RX_BATCHING_ASYNC;
			}
		} else {
			list_move_tail(&sock->list, &sock->drv->no_batching_list);
			sock->flags = (sock->flags & ~SOCKET_FLAGS_RX_BATCH_ANY) | SOCKET_FLAGS_RX_NO_BATCHING;
		}
	}

	raw_spin_unlock_irqrestore(&ptype_lock, flags);

out:
	return rc;
}

int socket_port_status(struct net_socket *sock, struct net_port_status *status)
{
	struct avb_drv *avb = container_of(sock->drv, struct avb_drv, net_drv);
	struct logical_port *port;
	int rc;

	port = logical_port_get(avb, status->port);
	if (!port) {
		rc = -EINVAL;
		goto err;
	}

	rc = eth_avb_port_status(port->phys, status);

err:
	return rc;
}


int socket_add_multi(struct net_socket *sock, struct net_mc_address *addr)
{
	struct avb_drv *avb = container_of(sock->drv, struct avb_drv, net_drv);
	struct logical_port *port;
	int rc;

	if (!(sock->flags & SOCKET_FLAGS_RX)) {
		rc = -EINVAL;
		goto err;
	}

	port = logical_port_get(avb, addr->port);
	if (!port) {
		rc = -EINVAL;
		goto err;
	}

	/* Always add it to the physical port */
	return eth_avb_add_multi(port->phys, addr);

err:
	return rc;
}

int socket_del_multi(struct net_socket *sock, struct net_mc_address *addr)
{
	struct avb_drv *avb = container_of(sock->drv, struct avb_drv, net_drv);
	struct logical_port *port;
	int rc;

	if (!(sock->flags & SOCKET_FLAGS_RX)) {
		rc = -EINVAL;
		goto err;
	}

	port = logical_port_get(avb, addr->port);
	if (!port) {
		rc = -EINVAL;
		goto err;
	}

	return eth_avb_del_multi(port->phys, addr);

err:
	return rc;
}

int socket_read(struct net_socket *sock, unsigned long *buf, unsigned int n)
{
	struct avb_drv *avb = container_of(sock->drv, struct avb_drv, net_drv);
	unsigned long addr;
	unsigned int read = 0;

	if (!(sock->flags & SOCKET_FLAGS_RX))
		goto err;

	while (n && ((addr = queue_dequeue(&sock->queue)) != (unsigned long)-1)) {

		buf[read] = pool_dma_virt_to_shmem(&avb->buf_pool, (void *)addr);

		n--;
		read++;
	}

	if ((sock->flags & SOCKET_FLAGS_RX_BATCHING) && read) {
		/*Clear LATENCY_VALID before FLUSH to avoid flushing with an old latency value*/
		clear_bit(SOCKET_ATOMIC_FLAGS_LATENCY_VALID, &sock->atomic_flags);
		clear_bit(SOCKET_ATOMIC_FLAGS_FLUSH, &sock->atomic_flags);
	}

	return read;

err:
	return -EINVAL;
}

int socket_read_ts(struct net_socket *sock, struct net_ts_desc *ts_desc)
{
	struct avb_drv *avb = container_of(sock->drv, struct avb_drv, net_drv);
	struct net_tx_desc *tx_desc;
	int rc;

	if (!(sock->flags & SOCKET_FLAGS_TX_TS_ENABLED)) {
		rc = -EINVAL;
		goto err;
	}

	tx_desc = (struct net_tx_desc *)queue_dequeue(&sock->tx_ts_queue);
	if (tx_desc == (struct net_tx_desc *)-1) {
		rc = -EAGAIN;
		goto err;
	}

	ts_desc->ts = tx_desc->ts64;
	ts_desc->priv = tx_desc->priv;

	pool_dma_free(&avb->buf_pool, tx_desc);

	return 0;

err:
	return rc;
}

static int socket_tx_connected(struct net_socket *sock, unsigned long *buf, unsigned int n)
{
	struct avb_drv *avb = container_of(sock->drv, struct avb_drv, net_drv);
	struct logical_port *port = __logical_port_get(avb, sock->addr.port);
	struct eth_avb *eth_phys = port->phys;
	struct avb_tx_desc *desc;
	struct net_device *ndev;
	struct ethhdr *ethhdr;
	struct qos_queue *qos_q = sock->qos_queue;
	unsigned long addr_shmem;
	unsigned long offset;
	unsigned long flags;
	unsigned int i = 0;
	int rc = 0;

	raw_spin_lock_irqsave(&eth_phys->lock, flags);

	if (!(eth_phys->flags & PORT_FLAGS_ENABLED)) {
		rc = -EIO;
		qos_q->dropped++;
		goto out;
	}

	if (!(qos_q->flags & QOS_QUEUE_FLAG_ENABLED)) {
		rc = -EINVAL;
		qos_q->disabled++;
		qos_q->dropped++;
		goto out;
	}

	ndev = dev_get_by_index(&init_net, port->ifindex);
	if (!ndev) {
		rc = -EINVAL;
		goto out;
	}

	for (i = 0; i < n; i++) {
		addr_shmem = buf[i];

		if (addr_shmem >= BUF_POOL_SIZE) {
			pr_err("%s: desc(%lx) outside of pool range\n", __func__, addr_shmem);
			rc = -EFAULT;
			break;
		}

		desc = pool_dma_shmem_to_virt(&avb->buf_pool, addr_shmem);
		buf[i] = (unsigned long)desc;

		if (desc->common.len > NET_PAYLOAD_SIZE_MAX) {
			pr_err("%s: desc(%lx), len(%u) too big\n", __func__, addr_shmem, desc->common.len);
			rc = -EINVAL;
			break;
		}

		if (desc->common.offset > (NET_PAYLOAD_SIZE_MAX - desc->common.len)) {
			pr_err("%s: desc(%lx), offset(%u) too big\n", __func__, addr_shmem, desc->common.offset);
			rc = -EINVAL;
			break;
		}

		offset = ((unsigned long)desc + desc->common.offset) & (FEC_ALIGNMENT - 1);
		if (offset) {
			pr_err("%s: desc(%lx), data not properly aligned (%lx)\n", __func__, addr_shmem, offset);
			rc = -EINVAL;
			break;
		}

		ethhdr = (void *)desc + desc->common.offset;
		memcpy(ethhdr->h_source, ndev->dev_addr, ETH_ALEN);

		if (sock->flags & SOCKET_FLAGS_VLAN) {
			struct vlanhdr *vlan = (struct vlanhdr *)(ethhdr + 1);

			vlan->label = sock->vlan_label;
		}

		desc->dma_addr = pool_dma_virt_to_dma(&avb->buf_pool, (void *)desc + desc->common.offset, eth_phys->index);
		dma_sync_single_for_device(avb->buf_pool.dma_device[eth_phys->index], desc->dma_addr, desc->common.len, DMA_TO_DEVICE);

		desc->esc = 0;
	}

	if (likely(i))
		rc = sock->tx_func(sock, port, buf, &i);

	dev_put(ndev);

out:
	raw_spin_unlock_irqrestore(&eth_phys->lock, flags);

	if (i > 0)
		return i;
	else
		return rc;
}

static int socket_tx(struct net_socket *sock, const unsigned long *buf, unsigned int n)
{
	struct sk_buff *skb;
	struct avb_drv *avb = container_of(sock->drv, struct avb_drv, net_drv);
	struct logical_port *port;
	unsigned long addr_shmem;
	struct net_tx_desc *desc;
	struct eth_avb *eth;
	struct net_device *ndev;
	struct ethhdr *ethhdr;
	unsigned int offset;
	int i;
	int rc = 0;

	for (i = 0; i < n; i++) {

		addr_shmem = buf[i];

		if (addr_shmem >= BUF_POOL_SIZE) {
			pr_err("%s: desc(%lx) outside of pool range\n", __func__, addr_shmem);
			rc = -EFAULT;
			break;
		}

		desc = pool_dma_shmem_to_virt(&avb->buf_pool, addr_shmem);

		port = logical_port_get(avb, desc->port);
		if (!port) {
			pr_err("%s: desc(%lx), logical_port(%u) invalid\n", __func__, addr_shmem, desc->port);
			rc = -EINVAL;
			break;
		}

		eth = port->phys;

		if (desc->len > NET_PAYLOAD_SIZE_MAX) {
			pr_err("%s: desc(%lx), len(%u) too big\n", __func__, addr_shmem, desc->len);
			rc = -EINVAL;
			break;
		}

		if (desc->l2_offset > (NET_PAYLOAD_SIZE_MAX - desc->len)) {
			pr_err("%s: desc(%lx), offset(%u) too big\n", __func__, addr_shmem, desc->l2_offset);
			rc = -EINVAL;
			break;
		}

		skb = alloc_skb(desc->len + FEC_ALIGNMENT, GFP_KERNEL);
		if (!skb) {
			rc = -ENOMEM;
			break;
		}

		/* Make sure data is properly aligned to avoid using a bounce buffer in the fec driver */
		offset = (unsigned long)skb->data & (FEC_ALIGNMENT - 1);
		if (offset)
			skb_reserve(skb, FEC_ALIGNMENT - offset);

		memcpy(skb->data, (void *)desc + desc->l2_offset, desc->len);
		skb_put(skb, desc->len);
		skb_reset_network_header(skb);

		ndev = dev_get_by_index(&init_net, eth->ifindex);
		if (!ndev) {
			kfree_skb(skb);
			rc = -EINVAL;
			break;
		}

		skb->dev = ndev;

		/* Override source mac address */
		skb_reset_mac_header(skb);

		ethhdr = eth_hdr(skb);
		memcpy(ethhdr->h_source, ndev->dev_addr, ETH_ALEN);

		dev_queue_xmit(skb);

		dev_put(ndev);

		pool_dma_free(&avb->buf_pool, desc);
	}

	if (i > 0)
		return i;
	else
		return rc;
}

unsigned int socket_tx_available(struct net_socket *sock)
{
	return queue_available(&sock->queue);
}

int socket_write(struct net_socket *sock, unsigned long *buf, unsigned int n)
{
	int rc;

	if (sock->flags & SOCKET_FLAGS_RX)
		return -EINVAL;

	if (sock->flags & SOCKET_FLAGS_CONNECTED)
		rc = socket_tx_connected(sock, buf, n);
	else
		rc = socket_tx(sock, buf, n);

	return rc;
}

static void __socket_unbind(struct net_socket *sock)
{
	if (!(sock->flags & SOCKET_FLAGS_BOUND))
		return;

	switch (sock->addr.ptype) {
	case PTYPE_AVTP:
		avtp_socket_unbind(sock);
		break;

	case PTYPE_PTP:
		ptp_socket_unbind(sock);
		break;

	case PTYPE_MRP:
		mrp_socket_unbind(sock);
		break;

	default:
		break;
	}

	memset(&sock->addr, 0, sizeof(sock->addr));
	sock->addr.ptype = PTYPE_NONE;

	sock->flags &= ~SOCKET_FLAGS_BOUND;
}

static void socket_unbind(struct net_socket *sock)
{
	unsigned long flags;

//	pr_info("%s: %p\n", __func__, sock);

wait:
	while (test_bit(SOCKET_ATOMIC_FLAGS_BUSY, &sock->atomic_flags))
		usleep_range(25, 50);

	raw_spin_lock_irqsave(&ptype_lock, flags);

	if (test_bit(SOCKET_ATOMIC_FLAGS_BUSY, &sock->atomic_flags)) {
		raw_spin_unlock_irqrestore(&ptype_lock, flags);
		goto wait;
	}

	__socket_unbind(sock);

	if (sock->flags & SOCKET_FLAGS_RX_BATCH_ANY) {
		list_del(&sock->list);
		sock->flags &= ~SOCKET_FLAGS_RX_BATCH_ANY;
	}

	raw_spin_unlock_irqrestore(&ptype_lock, flags);
}


int socket_bind(struct net_socket *sock, struct net_address *addr)
{
	unsigned long flags;
	int rc;

//	pr_info("%s: sock(%p) ptype(%x) logical_port(%u)\n", __func__, sock, addr->ptype, addr->port);

	/* Associates a queue with a specific protocol ("ethertype" <-> queue) */
	/* Only one queue per ethertype is supported */
	/* This is how rx classification code knows where rx packets should be enqueued */

	if (!(sock->flags & SOCKET_FLAGS_RX)) {
		goto err;
	}

	if (addr->ptype >= PTYPE_MAX)
		goto err;

	/* PORT_ANY is valid */
	if (!logical_port_valid(addr->port) && (addr->port != PORT_ANY)) {
		pr_err("%s: logical_port(%u) invalid\n", __func__, addr->port);
		goto err;
	}

wait:
	while (test_bit(SOCKET_ATOMIC_FLAGS_BUSY, &sock->atomic_flags))
		usleep_range(25, 50);

	raw_spin_lock_irqsave(&ptype_lock, flags);

	if (test_bit(SOCKET_ATOMIC_FLAGS_BUSY, &sock->atomic_flags)) {
		raw_spin_unlock_irqrestore(&ptype_lock, flags);
		goto wait;
	}

	__socket_unbind(sock);

	if (sock->flags & SOCKET_FLAGS_BOUND)
		goto err_unlock;

	switch (addr->ptype) {
	case PTYPE_AVTP:
		rc = avtp_socket_bind(sock, addr);
		break;

	case PTYPE_PTP:
		rc = ptp_socket_bind(sock, addr);
		break;

	case PTYPE_MRP:
		rc = mrp_socket_bind(sock, addr);
		break;

	default:
		rc = -1;

		break;
	}

	if (rc < 0)
		goto err_unlock;

//	memset(&ptype_hdlr[addr->ptype].stats, 0, sizeof(ptype_hdlr[addr->ptype].stats));

	memcpy(&sock->addr, addr, sizeof(*addr));

	/* Assign to the right wakeup list*/
	if ((sock->max_packets > 1) && (sock->max_latency > 0)) {
		if (is_avtp_stream(addr->u.avtp.subtype)) {
			list_add(&sock->list, &sock->drv->batching_sync_list);
			sock->flags |= SOCKET_FLAGS_RX_BATCHING_SYNC;
		} else {
			list_add(&sock->list, &sock->drv->batching_async_list);
			sock->flags |= SOCKET_FLAGS_RX_BATCHING_ASYNC;
		}
	} else {
		list_add(&sock->list, &sock->drv->no_batching_list);
		sock->flags |= SOCKET_FLAGS_RX_NO_BATCHING;
	}

	sock->flags |= SOCKET_FLAGS_BOUND;

	raw_spin_unlock_irqrestore(&ptype_lock, flags);

	return 0;

err_unlock:
	raw_spin_unlock_irqrestore(&ptype_lock, flags);

err:
	return -EINVAL;
}

static void __socket_disconnect(struct logical_port *port, struct net_socket *sock)
{
	if (!(sock->flags & SOCKET_FLAGS_CONNECTED))
		return;

	switch (sock->addr.ptype) {
	case PTYPE_AVTP:
		avtp_socket_disconnect(port, sock);
		break;

	case PTYPE_PTP:
		ptp_socket_disconnect(port, sock);
		break;

	case PTYPE_MRP:
		mrp_socket_disconnect(port, sock);
		break;

	default:
		break;
	}

	memset(&sock->addr, 0, sizeof(sock->addr));
	sock->addr.ptype = PTYPE_NONE;

	sock->flags &= ~SOCKET_FLAGS_CONNECTED;
}

static int socket_disconnect(struct net_socket *sock)
{
	struct avb_drv *avb = container_of(sock->drv, struct avb_drv, net_drv);
	struct logical_port *port;
	struct eth_avb *eth;
	unsigned long flags;

	//pr_info("%s: sock(%p) - logical_port(%u)\n", __func__, sock, sock->addr.port);

	if (sock->flags & SOCKET_FLAGS_CONNECTED) {
		port = __logical_port_get(avb, sock->addr.port);
		eth = port->phys;

		raw_spin_lock_irqsave(&eth->lock, flags);

		__socket_disconnect(port, sock);

		raw_spin_unlock_irqrestore(&eth->lock, flags);
	}

	return 0;
}


int socket_connect(struct net_socket *sock, struct net_address *addr)
{
	struct avb_drv *avb = container_of(sock->drv, struct avb_drv, net_drv);
	struct logical_port *port;
	struct eth_avb *eth;
	int rc;
	unsigned long flags;

	if (sock->flags & SOCKET_FLAGS_RX)
		goto err;

	port = logical_port_get(avb, addr->port);
	if (!port) {
		pr_err("%s: logical_port(%u) invalid\n", __func__, addr->port);
		goto err;
	}

	eth = port->phys;

	raw_spin_lock_irqsave(&eth->lock, flags);

	__socket_disconnect(port, sock);

	if (sock->flags & SOCKET_FLAGS_CONNECTED)
		goto err_unlock;

	switch (addr->ptype) {
	case PTYPE_AVTP:
		rc = avtp_socket_connect(port, sock, addr);
		break;

	case PTYPE_PTP:
		rc = ptp_socket_connect(port, sock, addr);
		break;

	case PTYPE_MRP:
		rc = mrp_socket_connect(port, sock, addr);
		break;

	default:
		rc = -1;
		break;
	}

	if (rc < 0)
		goto err_unlock;

	memcpy(&sock->addr, addr, sizeof(*addr));

	sock->flags |= SOCKET_FLAGS_CONNECTED;

	raw_spin_unlock_irqrestore(&eth->lock, flags);

	return 0;

err_unlock:
	raw_spin_unlock_irqrestore(&eth->lock, flags);

err:
	return -EINVAL;
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

void generic_rx_wakeup(struct net_socket *sock, struct net_socket **sock_array, unsigned int *n)
{
	if (sock && queue_pending(&sock->queue))
		socket_schedule_wake_up(sock, sock_array, n);
}

/* Parse the no batching list and wake up sockets with pending buffers*/
static inline void generic_rx_no_batch_wakeup(struct list_head *no_batching_list, struct net_socket **sock_array, unsigned int *n)
{
	struct net_socket *sock;

	list_for_each_entry(sock, no_batching_list, list) {
		if (queue_pending(&sock->queue))
			socket_schedule_wake_up(sock, sock_array, n);
	}
}

/* Parse the async batching list and wake up only the ready sockets*/
static inline void generic_rx_batch_async_wakeup(struct list_head *batching_list, struct net_socket **sock_array, unsigned int *n)
{
	struct net_socket *sock;

	list_for_each_entry(sock, batching_list, list) {
		if (test_bit(SOCKET_ATOMIC_FLAGS_FLUSH, &sock->atomic_flags))
			socket_schedule_wake_up(sock, sock_array, n);
	}

}

/* Parse the sync batching list and wake up, if at least one socket is ready,  all
   sockets with pending buffers. */
static inline void generic_rx_batch_sync_wakeup(struct list_head *batching_list, struct net_socket **sock_array, unsigned int *n)
{
	struct net_socket *sock;
	unsigned int wake_all = 0;

	list_for_each_entry(sock, batching_list, list) {
		if (test_bit(SOCKET_ATOMIC_FLAGS_FLUSH, &sock->atomic_flags)) {
			wake_all = 1;
			break;
		}
	}

	if (wake_all) {
		list_for_each_entry(sock, batching_list, list) {
			if (queue_pending(&sock->queue)) {
				set_bit(SOCKET_ATOMIC_FLAGS_FLUSH, &sock->atomic_flags);
				socket_schedule_wake_up(sock, sock_array, n);
			}
		}
	}
}

/* parse all the driver socket lists: batch_sync, batch_async and single packet list */
void generic_rx_wakeup_all(struct net_drv *drv, struct net_socket **sock_array, unsigned int *n)
{
	generic_rx_batch_sync_wakeup(&drv->batching_sync_list, sock_array, n);
	generic_rx_batch_async_wakeup(&drv->batching_async_list, sock_array, n);
	generic_rx_no_batch_wakeup(&drv->no_batching_list, sock_array, n);
}

static inline int socket_rx_latency_ready(struct net_socket *sock)
{
	struct avb_drv *avb = container_of(sock->drv, struct avb_drv, net_drv);

	if (!test_and_set_bit(SOCKET_ATOMIC_FLAGS_LATENCY_VALID, &sock->atomic_flags)) {
		struct net_rx_desc *desc;

		if (queue_pending(&sock->queue)) {
			desc = (struct net_rx_desc *)queue_peek(&sock->queue);
			sock->time = desc->priv;
		} else {
			clear_bit(SOCKET_ATOMIC_FLAGS_LATENCY_VALID, &sock->atomic_flags);
			return AVB_NET_RX_OK;
		}
	}

	if ((avb->timer.time - sock->time) >= sock->max_latency)
		if (!test_and_set_bit(SOCKET_ATOMIC_FLAGS_FLUSH, &sock->atomic_flags))
			return AVB_NET_RX_WAKE;

	return AVB_NET_RX_OK;
}

static inline int socket_rx_packets_ready(struct net_socket *sock)
{
	unsigned int pending = queue_pending(&sock->queue);
	int rc = AVB_NET_RX_OK;

	if ((pending >= sock->max_packets) || (sock->max_packets == 1))
		if (!(sock->flags & SOCKET_FLAGS_RX_BATCHING) || !test_and_set_bit(SOCKET_ATOMIC_FLAGS_FLUSH, &sock->atomic_flags))
			rc = AVB_NET_RX_WAKE;

	return rc;
}

/* Should be called with ptype_lock held */
static inline int net_rx_list_latency_ready(struct list_head *batching_list)
{
	struct net_socket *sock;

	list_for_each_entry(sock, batching_list, list) {
		if (socket_rx_latency_ready(sock)) {
			return 1;
		}
	}

	return 0;
}

int net_rx_batch_any_ready(struct net_drv *drv)
{
	unsigned long flags;
	int rc = 0;

	raw_spin_lock_irqsave(&ptype_lock, flags);

	if ((rc = net_rx_list_latency_ready(&drv->batching_sync_list)))
		goto unlock_and_exit;

	rc = net_rx_list_latency_ready(&drv->batching_async_list);

unlock_and_exit:
	raw_spin_unlock_irqrestore(&ptype_lock, flags);

	return rc;
}

int net_rx(struct eth_avb *eth, struct net_socket *sock, struct net_rx_desc *desc, struct net_rx_stats *stats)
{
	struct avb_drv *avb = container_of(sock->drv, struct avb_drv, net_drv);

	desc->len -= FCS_LEN; /* Remove FCS at the end of the packet */
	desc->priv = avb->timer.time;

	if (unlikely(queue_enqueue(&sock->queue, (unsigned long)desc) < 0))
		goto drop;

	stats->rx++;

	return socket_rx_packets_ready(sock);

drop:
	desc->len += FCS_LEN;
	return net_rx_drop(eth, desc, stats);
}

static void socket_flush(struct net_socket *sock)
{
	struct avb_drv *avb = container_of(sock->drv, struct avb_drv, net_drv);

	queue_flush(&sock->queue, &avb->buf_pool);
}

void socket_close(struct net_socket *sock)
{
	struct avb_drv *avb = container_of(sock->drv, struct avb_drv, net_drv);

	if (sock->flags & SOCKET_FLAGS_RX) {
		socket_unbind(sock);
		socket_flush(sock);
	} else
		socket_disconnect(sock);

	queue_flush(&sock->tx_ts_queue, &avb->buf_pool);

	kfree(sock);
}

struct net_socket *socket_open(unsigned int rx)
{
	struct net_socket *sock;

	sock = kzalloc(sizeof(struct net_socket), GFP_KERNEL);
	if (!sock)
		goto err_kmalloc;

	if (rx) {
		sock->flags |= SOCKET_FLAGS_RX;
		sock->max_packets = 1;
		sock->max_latency = 0;
	}

	sock->addr.ptype = PTYPE_NONE;

	if (sock->flags & SOCKET_FLAGS_RX)
		queue_init(&sock->queue, pool_dma_free_virt, 0);
	else
		queue_init(&sock->queue, pool_dma_free_virt, CFG_NET_TX_EXTRA_ENTRIES);

	queue_init(&sock->tx_ts_queue, pool_dma_free_virt, 0);

	init_waitqueue_head(&sock->wait);

	return sock;

err_kmalloc:
	return NULL;
}
