/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Network service implementation through IPC
 @details
*/


#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/log.h"
#include "common/net.h"
#include "common/ipc.h"
#include "net.h"
#include "net_logical_port.h"

#include "modules/ipc.h"

/* ipc meta data offset. */
#define IPC_DATA_OFFSET        (offsetof(struct ipc_desc, u.data))

/* The IPC buffer will contain: ipc metadata + network descriptor (without metadata) */
static inline bool net_ipc_is_valid_size(unsigned int net_desc_size)
{
	/* FIXME: have an early check on allocation (rather than net_tx()) that the net desc size is
	 * lower than available space in ipc buffer (with ipc metadata) : (net_desc_size + IPC_DATA_OFFSET) <= IPC_BUF_SIZE
	 */
	if (net_desc_size > DEFAULT_NET_DATA_SIZE)
		return false;

	return true;
}

static struct net_rx_desc *net_ipc_rx_alloc(unsigned int size)
{
	struct net_rx_desc *desc;

	if (!net_ipc_is_valid_size(size))
		return NULL;

	desc = malloc(NET_DATA_OFFSET + size);
	if (!desc)
		goto exit;

	desc->flags = 0;
	desc->len = 0;
	desc->l2_offset = NET_DATA_OFFSET;
	desc->pool_type = POOL_TYPE_IPC;
exit:
	return desc;
}

static struct net_tx_desc *net_ipc_tx_alloc(unsigned int size)
{
	struct net_tx_desc *desc;

	if (!net_ipc_is_valid_size(size))
		return NULL;

	desc = malloc(NET_DATA_OFFSET + size);
	if (!desc)
		goto exit;

	desc->flags = 0;
	desc->len = 0;
	desc->l2_offset = NET_DATA_OFFSET;
	desc->pool_type = POOL_TYPE_IPC;
exit:
	return desc;
}

static int net_ipc_tx_alloc_multi(struct net_tx_desc **desc, unsigned int n, unsigned int size)
{
	int i;

	for (i = 0; i < n; i++) {
		desc[i] = net_ipc_tx_alloc(size);
		if (!desc[i])
			goto err_malloc;
	}

err_malloc:
	return i;
}

static struct net_tx_desc *net_ipc_tx_clone(struct net_tx_desc *src)
{
	unsigned int net_size = src->l2_offset + src->len;
	struct net_tx_desc *desc;

	/* Make sure the network descriptor len is valid for ipc */
	if (!net_ipc_is_valid_size(src->len))
		goto err;

	desc = malloc(net_size);
	if (!desc)
		goto err;

	memcpy(desc, src, net_size);

	desc->pool_type = POOL_TYPE_IPC;

	return desc;
err:
	return NULL;
}

void net_ipc_tx_free(struct net_tx_desc *buf)
{
	free((void *)buf);
}

void net_ipc_rx_free(struct net_rx_desc *buf)
{
	free((void *)buf);
}

void net_ipc_free_multi(void **buf, unsigned int n)
{
	int i;

	for (i = 0; i < n; i++)
		free(buf[i]);
}

static struct net_rx_desc *__net_ipc_receive(struct ipc_rx const *ipc_rx, struct ipc_desc *ipc_desc)
{
	struct net_rx *rx = container_of(ipc_rx, struct net_rx, ipc_rx);
	struct net_rx_desc *rx_desc = NULL;
	void *rx_buf;

	if (ipc_desc->type != IPC_NETWORK_BUFFER) {
		os_log(LOG_ERR, "wrong descriptor type(%u)\n", ipc_desc->type);
		goto out;
	}

	/* Allocate a network buffer */
	rx_desc = (struct net_rx_desc *) net_ipc_rx_alloc(ipc_desc->len);
	if (!rx_desc) {
		os_log(LOG_ERR, "net_ipc_rx_alloc(%u) failed\n", ipc_desc->len);
		goto out;
	}

	rx_buf = NET_DATA_START(rx_desc);
	/* Copy the ipc data into the network descriptor. */
	memcpy(rx_buf, ipc_desc->u.data, ipc_desc->len);
	rx_desc->len = ipc_desc->len;
	rx_desc->port = rx->port_id;
	rx_desc->l2_offset = NET_DATA_OFFSET;

	net_std_rx_parser(rx, rx_desc);

out:
	ipc_free(ipc_rx, ipc_desc);

	return rx_desc;
}

static void net_ipc_receive(struct ipc_rx const *ipc_rx, struct ipc_desc *ipc_desc)
{
	struct net_rx *rx = container_of(ipc_rx, struct net_rx, ipc_rx);
	struct net_rx_desc *rx_desc;

	rx_desc = __net_ipc_receive(ipc_rx, ipc_desc);
	if (!rx_desc)
		return;

	rx->func(rx, rx_desc);
}

static int net_ipc_rx_init(struct net_rx *rx, struct net_address *addr, void (*func)(struct net_rx *, struct net_rx_desc *), unsigned long epoll_fd)
{
	ipc_id_t ipc_id;

	os_log(LOG_INFO, "enter\n");

	/* Only support IPC mode for SRP protocol. */
	if (!addr || addr->ptype != PTYPE_MRP)
		goto err_addr;

	ipc_id = (logical_port_is_bridge(addr->port)) ? IPC_SRP_ENDPOINT_BRIDGE : IPC_SRP_BRIDGE_ENDPOINT;
	rx->func = func;
	rx->port_id = addr->port;
	rx->pool_type = POOL_TYPE_IPC;
	rx->fd = -1;

	if (ipc_rx_init(&rx->ipc_rx, ipc_id, net_ipc_receive, epoll_fd) < 0) {
		os_log(LOG_ERR, "ipc_rx_init(%u) failed\n", ipc_id);
		goto err_ipc_init;
	}

	os_log(LOG_INFO, "fd(%d)\n", rx->ipc_rx.fd);

	return 0;

err_ipc_init:
err_addr:
	return -1;
}

static int net_ipc_rx_init_multi(struct net_rx *rx, struct net_address *addr,
					void (*func)(struct net_rx *, struct net_rx_desc **, unsigned int),
					unsigned int packets, unsigned int time, unsigned long epoll_fd)
{
	return -1;
}

static void net_ipc_rx_exit(struct net_rx *rx)
{

	ipc_rx_exit(&rx->ipc_rx);

	os_log(LOG_INFO, "done\n");
}

static struct net_rx_desc *__net_ipc_rx(struct net_rx *rx)
{
	struct net_rx_desc *rx_desc;
	struct ipc_desc *ipc_desc;

	ipc_desc = __ipc_rx(&rx->ipc_rx);
	if (!ipc_desc)
		goto err_ipc_rx;

	rx_desc = __net_ipc_receive(&rx->ipc_rx, ipc_desc);
	if (!rx_desc)
		goto net_ipc_err;

	return rx_desc;

net_ipc_err:
	ipc_free(&rx->ipc_rx, ipc_desc);

err_ipc_rx:
	return NULL;
}

static void net_ipc_rx_multi(struct net_rx *rx)
{
	return;
}

static void net_ipc_rx(struct net_rx *rx)
{
	struct net_rx_desc *desc;

	desc = __net_ipc_rx(rx);
	if (desc)
		rx->func(rx, desc);
}

static int net_ipc_tx_init(struct net_tx *tx, struct net_address *addr)
{
	ipc_id_t ipc_id;

	os_log(LOG_INFO, "enter\n");

	/* Only support IPC mode for SRP protocol. */
	if (!addr || addr->ptype != PTYPE_MRP)
		goto err_addr;

	ipc_id = (logical_port_is_bridge(addr->port)) ? IPC_SRP_BRIDGE_ENDPOINT : IPC_SRP_ENDPOINT_BRIDGE;

	tx->port_id = addr->port;

	if (ipc_tx_init(&tx->ipc_tx, ipc_id) < 0) {
		os_log(LOG_ERR, "ipc_tx_init(%u) failed\n", ipc_id);
		goto err_ipc_init;
	}

	tx->pool_type = POOL_TYPE_IPC;
	tx->fd = -1;

	os_log(LOG_INFO, "fd(%d)\n", tx->ipc_tx.fd);

	return 0;

err_addr:
err_ipc_init:
	return -1;
}

static void net_ipc_tx_exit(struct net_tx *tx)
{
	ipc_tx_exit(&tx->ipc_tx);

	os_log(LOG_INFO, "done\n");
}

static int net_ipc_tx_multi(struct net_tx *tx, struct net_tx_desc **desc, unsigned int n)
{
	return -1;
}

static int net_ipc_tx_ts_get(struct net_tx *tx, uint64_t *ts, unsigned int *private)
{
	return -1;
}

static int net_ipc_tx_ts_init(struct net_tx *tx, struct net_address *addr, void (*func)(struct net_tx *, uint64_t, unsigned int), unsigned long priv)
{
	return -1;
}

static int net_ipc_tx_ts_exit(struct net_tx *tx)
{
	return 0;
}

static unsigned int net_ipc_tx_available(struct net_tx *tx)
{
	return 0;
}

static int net_ipc_tx(struct net_tx *tx, struct net_tx_desc *tx_desc)
{
	void *tx_buf = NET_DATA_START(tx_desc);
	unsigned int ipc_alloc_size;
	struct ipc_desc *ipc_desc;
	int rc;

	/* take into account the ipc metadata size. */
	ipc_alloc_size = tx_desc->len + IPC_DATA_OFFSET;

	ipc_desc = ipc_alloc(&tx->ipc_tx, ipc_alloc_size);
	if (ipc_desc) {
		ipc_desc->type = IPC_NETWORK_BUFFER;
		ipc_desc->len = tx_desc->len;
		ipc_desc->flags = 0;

		memcpy(ipc_desc->u.data, tx_buf, tx_desc->len);

		rc = ipc_tx(&tx->ipc_tx, ipc_desc);
		if (rc < 0) {
			if (rc != IPC_TX_ERR_NO_READER)
				os_log(LOG_ERR, "ipc_tx() failed (%d)\n", rc);
			ipc_free(&tx->ipc_tx, ipc_desc);
		}
	} else {
		os_log(LOG_ERR, "ipc_alloc(%u) failed: with net descriptor length (%u)\n",
			ipc_alloc_size, tx_desc->len);
		rc = -1;
	}

	/* On successful transmission, free the network buffer.
	 * Otherwise, caller will free it.
	 */
	if (!rc)
		net_ipc_tx_free(tx_desc);

	return rc;
}

static int net_ipc_add_multi(struct net_rx *rx, unsigned int port_id, const unsigned char *hw_addr)
{
	return 0;
}

static int net_ipc_del_multi(struct net_rx *rx, unsigned int port_id, const unsigned char *hw_addr)
{
	return 0;
}

static int net_ipc_port_status(struct net_tx *tx, unsigned int port_id, bool *up, bool *point_to_point, uint64_t *rate)
{
	/* The MAC service layer in Hybrid mode uses the standard net mode to fetch the FEC port status.
	 * IPC net mode is only for SRP protocol.
	 */
	return -1;
}

static unsigned int net_ipc_port_mtu_size_get(unsigned int port_id)
{
	return (IPC_BUF_SIZE - IPC_DATA_OFFSET);
}

const static struct net_rx_ops_cb net_rx_ipc_ops = {
		.net_rx_init = net_ipc_rx_init,
		.net_rx_init_multi = net_ipc_rx_init_multi,
		.net_rx_exit = net_ipc_rx_exit,
		.__net_rx = __net_ipc_rx,
		.net_rx = net_ipc_rx,
		.net_rx_multi = net_ipc_rx_multi,

		.net_add_multi = net_ipc_add_multi,
		.net_del_multi = net_ipc_del_multi,
};

const static struct net_tx_ops_cb net_tx_ipc_ops = {
		.net_tx_init = net_ipc_tx_init,
		.net_tx_exit = net_ipc_tx_exit,
		.net_tx = net_ipc_tx,
		.net_tx_multi = net_ipc_tx_multi,

		.net_tx_alloc = net_ipc_tx_alloc,
		.net_tx_alloc_multi = net_ipc_tx_alloc_multi,
		.net_tx_clone = net_ipc_tx_clone,

		.net_tx_ts_get = net_ipc_tx_ts_get,
		.net_tx_ts_init = net_ipc_tx_ts_init,
		.net_tx_ts_exit = net_ipc_tx_ts_exit,

		.net_tx_available = net_ipc_tx_available,
		.net_port_status = net_ipc_port_status,
		.net_port_mtu_get = net_ipc_port_mtu_size_get,
};

const static struct net_mem_ops_cb net_ipc_mem_ops = {
		.net_tx_free = net_ipc_tx_free,

		.net_rx_free = net_ipc_rx_free,

		.net_free_multi = net_ipc_free_multi,
};

int net_ipc_socket_init(void *net_ops, bool is_rx)
{
	/* We copy the entire struct rather than just point to it, to reduce the number of
	 * indirections in performance-sensitive code.
	 */
	if (is_rx)
		memcpy(net_ops, &net_rx_ipc_ops, sizeof(struct net_rx_ops_cb));
	else
		memcpy(net_ops, &net_tx_ipc_ops, sizeof(struct net_tx_ops_cb));

	return 0;
}

int net_ipc_init(struct net_mem_ops_cb *net_mem_ops)
{
	memcpy(net_mem_ops, &net_ipc_mem_ops, sizeof(struct net_mem_ops_cb));

	return 0;
}

void net_ipc_exit(void)
{
	return;
}
