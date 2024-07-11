/*
 * Copyright 2017-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief RTOS specific Network service implementation
 @details
*/

#include "rtos_abstraction_layer.h"

#include <string.h>

#include "common/log.h"
#include "common/net.h"

#include "net_socket.h"
#include "net_port.h"

#include "genavb/error.h"

#define SOCKET_OTHER_RX_DEFAULT_PACKETS 8

/* allocation size lookup table */
const static uint32_t alloc_size[9] = {
	[0] = 0,
	[1] = 128,
	[2] = 256,
	[3] = 384,
	[4] = 512,
	[5] = 640,
	[6] = 768,
	[7] = 896,
	[8] = 1024,
};

#define NET_ALLOC_SIZE(size) (((size) <= 1024) ? alloc_size[((size) + 127) >> 7] : NET_DATA_SIZE)

static struct net_tx_desc *__net_tx_alloc(unsigned int size)
{
	struct net_tx_desc *desc;

	if (size > NET_DATA_SIZE)
		return NULL;

	size += NET_DATA_OFFSET;

	desc = rtos_malloc(NET_ALLOC_SIZE(size));
	if (!desc)
		goto exit;

	desc->flags = 0;
	desc->len = 0;
	desc->l2_offset = NET_DATA_OFFSET;

exit:
	return desc;
}

struct net_tx_desc *net_tx_alloc(struct net_tx *tx, unsigned int size)
{
	return __net_tx_alloc(size);
}

struct net_rx_desc *net_pool_rx_alloc(unsigned int size)
{
	struct net_rx_desc *desc;

	if (size > NET_DATA_SIZE)
		return NULL;

	size += NET_DATA_OFFSET;

	desc = rtos_malloc(NET_ALLOC_SIZE(size));
	if (!desc)
		goto exit;

	desc->flags = 0;
	desc->len = 0;
	desc->l2_offset = NET_DATA_OFFSET;

exit:
	return desc;
}

int net_tx_alloc_multi(struct net_tx *tx, struct net_tx_desc **desc, unsigned int n, unsigned int size)
{
	int i;

	for (i = 0; i < n; i++) {
		desc[i] = __net_tx_alloc(size);
		if (!desc[i])
			break;
	}

	return i;
}

int net_pool_tx_alloc_multi(struct net_tx_desc **desc, unsigned int n, unsigned int size)
{
	int i;

	for (i = 0; i < n; i++) {
		desc[i] = __net_tx_alloc(size);
		if (!desc[i])
			break;
	}

	return i;
}

struct net_tx_desc *net_tx_clone(struct net_tx *tx, struct net_tx_desc *src)
{
	struct net_tx_desc *desc = net_tx_alloc(tx, src->len);
	if (!desc)
		goto exit;

	os_memcpy(desc, src, src->l2_offset + src->len);

exit:
	return desc;
}

void net_tx_free(struct net_tx_desc *buf)
{
	rtos_free(buf);
}

void net_rx_free(struct net_rx_desc *buf)
{
	rtos_free(buf);
}

void net_tx_desc_free(void *data, unsigned long entry)
{
	rtos_free((void *)entry);
}

void net_rx_desc_free(void *data, unsigned long entry)
{
	rtos_free((void *)entry);
}

void net_free_multi(void **buf, unsigned int n)
{
	int i;

	for (i = 0; i < n; i++)
		rtos_free(buf[i]);
}

void net_rx_multi(struct net_rx *rx)
{
	unsigned long addr[NET_RX_BATCH];
	int len;

	if (unlikely(!rx->socket))
		return;

	len = socket_read(rx->socket, addr, rx->batch);
	if (len < 0)
		len = 0;

	rx->func_multi(rx, (struct net_rx_desc **)addr, len);

	socket_rx_enable_callback(rx->socket);
}

struct net_rx_desc * __net_rx(struct net_rx *rx)
{
	unsigned long addr;
	int len;

	len = socket_read(rx->socket, &addr, 1);
	if (len <= 0)
		return NULL;

	return (struct net_rx_desc *)addr;
}

void net_rx(struct net_rx *rx)
{
	unsigned long addr[NET_RX_BATCH];
	int len, i;

	if (unlikely(!rx->socket))
		return;

	len = socket_read(rx->socket, addr, rx->batch);

	for (i = 0; i < len; i++)
		rx->func(rx, (struct net_rx_desc *) addr[i]);

	socket_rx_enable_callback(rx->socket);
}

int net_rx_set_callback(struct net_rx *rx, void (*callback)(void *), void *data)
{
	return socket_set_callback(rx->socket, callback, data);
}

int net_rx_enable_callback(struct net_rx *rx)
{
	return socket_rx_enable_callback(rx->socket);
}

int net_tx_set_callback(struct net_tx *tx, void (*callback)(void *), void *data)
{
	return socket_set_callback(tx->socket, callback, data);
}

int net_tx_enable_callback(struct net_tx *tx)
{
	return socket_tx_enable_callback(tx->socket);
}

static int net_rx_set_option(struct net_rx *rx, unsigned int type, unsigned long val)
{
	return socket_set_option(rx->socket, type, val);
}

static int net_rx_bind(struct net_rx *rx, struct net_address *addr)
{
	return socket_bind(rx->socket, addr);
}

static int __net_rx_init(struct net_rx *rx, struct net_address *addr,
			void (*func)(struct net_rx *, struct net_rx_desc *),
			void (*func_multi)(struct net_rx *, struct net_rx_desc **, unsigned int),
			unsigned int packets, unsigned int latency,
			unsigned long handle)
{
	os_log(LOG_DEBUG, "enter\n");

	rx->socket = socket_open((rtos_mqueue_t *)handle, rx, 1);
	if (!rx->socket)
		goto err_open;

	if (packets && latency) {
		if (packets > NET_RX_BATCH)
			goto err_option;

		if (net_rx_set_option(rx, SOCKET_OPTION_BUFFER_PACKETS, packets) < 0)
			goto err_option;

		if (net_rx_set_option(rx, SOCKET_OPTION_BUFFER_LATENCY, latency) < 0)
			goto err_option;

		rx->batch = packets;
	} else {
		rx->batch = NET_RX_BATCH;
	}

	if (addr) {
		if (net_rx_bind(rx, addr) < 0)
			goto err_bind;

		if (addr->ptype == PTYPE_OTHER)
			socket_set_option(rx->socket, SOCKET_OPTION_PACKETS, SOCKET_OTHER_RX_DEFAULT_PACKETS);
	}

	rx->func = func;
	rx->func_multi = func_multi;

	os_log(LOG_INFO, "socket(%p)\n", rx->socket);

	return 0;

err_bind:
err_option:
	socket_close(rx->socket);

err_open:
	return -1;
}

int net_rx_init(struct net_rx *rx, struct net_address *addr, void (*func)(struct net_rx *, struct net_rx_desc *), unsigned long handle)
{
	return __net_rx_init(rx, addr, func, NULL, 0, 0, handle);
}

int net_rx_init_multi(struct net_rx *rx, struct net_address *addr, void (*func)(struct net_rx *, struct net_rx_desc **, unsigned int), unsigned int packets, unsigned int time, unsigned long handle)
{
	return __net_rx_init(rx, addr, NULL, func, packets, time, handle);
}

void net_rx_exit(struct net_rx *rx)
{
	socket_close(rx->socket);

	rx->socket = NULL;

	os_log(LOG_INFO, "done\n");
}

static int net_tx_connect(struct net_tx *tx, struct net_address *addr)
{
	return socket_connect(tx->socket, addr);
}

int __net_tx_init(struct net_tx *tx, struct net_address *addr, unsigned long handle)
{
	os_log(LOG_DEBUG, "enter\n");

	tx->socket = socket_open((rtos_mqueue_t *)handle, tx, 0);
	if (!tx->socket)
		goto err_open;

	if (addr) {
		if (net_tx_connect(tx, addr) < 0)
			goto err_connect;

		tx->port_id = addr->port;
	}

	os_log(LOG_INFO, "socket(%p) port_id(%d)\n", tx->socket, tx->port_id);

	return 0;

err_connect:
	socket_close(tx->socket);

err_open:
	return -1;
}

int net_tx_init(struct net_tx *tx, struct net_address *addr)
{
	return __net_tx_init(tx, addr, 0);
}

void net_tx_enable_raw(struct net_tx *tx)
{
	struct net_socket *sock = tx->socket;

	sock->flags |= SOCKET_FLAGS_TX_RAW;
}

void net_tx_exit(struct net_tx *tx)
{
	socket_close(tx->socket);

	tx->socket = NULL;

	os_log(LOG_INFO, "done\n");
}


int net_tx_multi(struct net_tx *tx, struct net_tx_desc **desc, unsigned int n)
{
	unsigned int i;
	unsigned int written;
	struct net_socket *sock = tx->socket;

	written = socket_write(sock, (unsigned long *)desc, n);

	if (written < n)
		goto err;

	return written;

err:
	for (i = written; i < n; i++)
		net_tx_free(desc[i]);

	if (written)
		return written;
	else
		return -1;
}

int net_tx(struct net_tx *tx, struct net_tx_desc *desc)
{
	int rc;
	struct net_socket *sock = tx->socket;

	rc = socket_write(sock, (unsigned long *)&desc, 1);

	if (rc < 1)
		return -1;

	return 1;
}

int net_tx_ts_get(struct net_tx *tx, uint64_t *ts, unsigned int *private)
{
	struct net_ts_desc ts_desc;
	int rc;

	rc = socket_read_ts(tx->socket, &ts_desc);
	if (rc < 0)
		goto err;

	if (rc) {
		*ts = ts_desc.ts;
		*private = ts_desc.priv;
		os_log(LOG_DEBUG, "socket(%p) ts %"PRIu64" type %lu\n", tx->socket, *ts, *private);
	}

	return rc;

err:
	return -1;
}

void net_tx_ts_process(struct net_tx *tx)
{
	uint64_t ts;
	unsigned int private;
	int rc;

	while ((rc = net_tx_ts_get(tx, &ts, &private)) > 0)
		tx->func_tx_ts(tx, ts, private);
}

static int net_tx_set_ts_enable(unsigned char enable, int port_id)
{
	if (!logical_port_valid(port_id))
		return -1;

	/* FIXME nothing to do or delay 1588Configure until here ? */

	return 0;
}


int net_tx_ts_init(struct net_tx *tx, struct net_address *addr, void (*func)(struct net_tx *, uint64_t, unsigned int), unsigned long handle)
{
	os_log(LOG_DEBUG, "\n");

	if (addr->ptype != PTYPE_PTP)
		goto err_wrong_ptype;

	if (__net_tx_init(tx, addr, handle) < 0)
		goto err_tx_init;

	if (net_tx_set_ts_enable(1, addr->port) < 0)
		goto err_tx_ts_enable;

	tx->func_tx_ts = func;

	return 0;

err_tx_ts_enable:
	net_tx_exit(tx);

err_tx_init:
err_wrong_ptype:
	return -1;
}


int net_tx_ts_exit(struct net_tx *tx)
{
	os_log(LOG_DEBUG, "\n");

	if (net_tx_set_ts_enable(0, tx->port_id) < 0)
		goto err_tx_ts_enable;

	net_tx_exit(tx);

	return 0;

err_tx_ts_enable:
	return -1;
}

int net_get_local_addr(unsigned int port_id, unsigned char *addr)
{
	if (!logical_port_valid(port_id)) {
		os_log(LOG_ERR," port_id %d out of range", port_id);
		goto err;
	}

	if (socket_get_hwaddr(port_id, addr) < 0)
		goto err;

	os_log(LOG_DEBUG,"port(%u) %02x:%02x:%02x:%02x:%02x:%02x\n", port_id,
		addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

	return 0;

err:
	return -1;
}

int net_port_status(struct net_tx *tx, unsigned int port_id, bool *up, bool *point_to_point, unsigned int *rate)
{
	struct net_port_status status;

	status.port = port_id;

	if (socket_port_status(tx->socket, &status) < 0) {
		os_log(LOG_ERR, "socket_port_status(%u) failed\n", port_id);
		goto err;
	}

	*up = status.up;
	*point_to_point = status.full_duplex;
	*rate = status.rate;

	return 0;

err:
	return -1;
}

static int __net_multi(struct net_rx *rx, unsigned int port, const unsigned char *hw_addr, unsigned int add)
{
	struct net_mc_address addr;
	int rc;

	addr.port = port;
	memcpy(addr.hw_addr, hw_addr, 6);

	if (add)
		rc = socket_add_multi(rx->socket, &addr);
	else
		rc = socket_del_multi(rx->socket, &addr);

	return rc;
}

int net_add_multi(struct net_rx *rx, unsigned int port_id, const unsigned char *hw_addr)
{
	os_log(LOG_INFO, "port(%u) %02x:%02x:%02x:%02x:%02x:%02x\n", port_id,
		hw_addr[0], hw_addr[1], hw_addr[2], hw_addr[3], hw_addr[4], hw_addr[5]);

	return __net_multi(rx, port_id, hw_addr, 1);
}

int net_del_multi(struct net_rx *rx, unsigned int port_id, const unsigned char *hw_addr)
{
	os_log(LOG_INFO,"port(%u) %02x:%02x:%02x:%02x:%02x:%02x\n", port_id,
		hw_addr[0], hw_addr[1], hw_addr[2], hw_addr[3], hw_addr[4], hw_addr[5]);

	return __net_multi(rx, port_id, hw_addr, 0);
}


int net_tx_event_enable(struct net_tx *tx, unsigned long priv)
{
	os_log(LOG_INFO, "tx(%p)\n", tx);

	return 0;
}

int net_tx_event_disable(struct net_tx *tx)
{
	os_log(LOG_INFO, "tx(%p)\n", tx);

	return 0;
}

unsigned int net_tx_available(struct net_tx *tx)
{
	os_log(LOG_INFO, "tx(%p)\n", tx);

	return socket_tx_available(tx->socket);
}

int net_port_set_max_frame_size(unsigned int port_id, uint16_t size)
{
	struct logical_port *port;
	int rc;

	port = logical_port_get(port_id);
	if (!port) {
		rc = -GENAVB_ERR_INVALID_PORT;
		goto err;
	}

	rc = port_set_max_frame_size(port->phys, size);
	if (rc < 0)
		goto err;

err:
	return rc;
}

int net_port_stats_get_number(unsigned int port_id)
{
	struct logical_port *port;

	port = logical_port_get(port_id);
	if (!port)
		return -1;

	return port_stats_get_number(port->phys);
}

int net_port_stats_get_strings(unsigned int port_id, const char **buf, unsigned int buf_len)
{
	struct logical_port *port;

	port = logical_port_get(port_id);
	if (!port)
		return -1;

	return port_stats_get_strings(port->phys, buf, buf_len);
}

int net_port_stats_get(unsigned int port_id, uint64_t *buf, unsigned int buf_len)
{
	struct logical_port *port;

	port = logical_port_get(port_id);
	if (!port)
		return -1;

	return port_stats_get(port->phys, buf, buf_len);
}

unsigned int net_port_priority_to_traffic_class(unsigned int port_id, uint8_t priority)
{
	struct logical_port *port;

	if (priority > QOS_PRIORITY_MAX)
		return 0;

	port = logical_port_get(port_id);
	if (!port)
		return 0;

	return port_priority_to_traffic_class(port->phys, priority);
}

int net_port_status_get(unsigned int port_id, bool *up, bool *duplex, unsigned int *rate)
{
	struct logical_port *port;
	struct net_port_status status;
	int rc;

	port = logical_port_get(port_id);
	if (!port) {
		rc = -GENAVB_ERR_INVALID_PORT;
		goto err;
	}

	rc = port_status(port->phys, &status);
	if (rc < 0)
		goto err;

	*up = status.up;
	*duplex = status.full_duplex;
	*rate = status.rate;

	return GENAVB_SUCCESS;

err:
	return rc;
}
