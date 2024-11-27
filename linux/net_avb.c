/*
 * Copyright 2014 Freescale Semiconductor, Inc.
 * Copyright 2019-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Linux specific Network service implementation
 @details
*/


#define _GNU_SOURCE

#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <net/if.h>
#include <sys/ioctl.h>

#include "common/log.h"
#include "common/net.h"

#include "genavb/helpers.h"

#include "clock.h"
#include "epoll.h"
#include "net.h"
#include "net_logical_port.h"
#include "shmem.h"

#include "modules/avbdrv.h"

#define NET_RX_DEV	"/dev/net_rx"
#define NET_TX_DEV	"/dev/net_tx"

/* from "common/ptp.h" */
#define PTP_DOMAIN_0 0

extern int net_set_hw_ts(unsigned int port_id, bool enable);


struct net_tx_desc *net_avb_tx_alloc(unsigned int size)
{
	struct net_tx_desc *desc = shmem_alloc();
	if (!desc)
		goto exit;

	desc->flags = 0;
	desc->len = 0;
	desc->l2_offset = NET_DATA_OFFSET;
	desc->pool_type = POOL_TYPE_AVB;
exit:
	return desc;
}

int net_avb_tx_alloc_multi(struct net_tx_desc **desc, unsigned int n, unsigned int size)
{
	int i, rc;

	rc = shmem_alloc_multi((void **)desc, n);
	if (rc < 0)
		return rc;

	for (i = 0; i < rc; i++) {
		desc[i]->flags = 0;
		desc[i]->len = 0;
		desc[i]->l2_offset = NET_DATA_OFFSET;
		desc[i]->pool_type = POOL_TYPE_AVB;
	}

	return rc;
}

struct net_tx_desc *net_avb_tx_clone(struct net_tx_desc *src)
{
	struct net_tx_desc *desc = shmem_alloc();
	if (!desc)
		goto exit;

	memcpy(desc, src, src->l2_offset + src->len);

	desc->pool_type = POOL_TYPE_AVB;

exit:
	return desc;
}

void net_avb_tx_free(struct net_tx_desc *buf)
{
	shmem_free((void *)buf);
}

void net_avb_rx_free(struct net_rx_desc *buf)
{
	shmem_free((void *)buf);
}

void net_avb_free_multi(void **buf, unsigned int n)
{
	shmem_free_multi(buf, n);
}

static uint64_t hwts_to_u64(os_clock_id_t clk_id, uint32_t hwts_ns)
{
	uint64_t hw_time;
	uint64_t nanoseconds = 0;

	if (os_clock_gettime64_of_parent(clk_id, &hw_time) < 0) {
		os_log(LOG_ERR, "os_clock_gettime64_of_parent(%d) failed\n", clk_id);
		goto out;
	}

	nanoseconds = (hw_time & 0xffffffff00000000) | hwts_ns;
	if (hwts_ns > (hw_time & 0xffffffff)) // the 32-bit clock counter wrapped between hwts_ns and now
		nanoseconds -= 0x100000000ULL;

out:
	return nanoseconds;
}

static int net_avb_rx_set_option(struct net_rx *rx, unsigned int opt, unsigned long val)
{
	struct net_set_option option;

	option.type = opt;
	option.val = val;

	if (ioctl(rx->fd, NET_IOC_SET_OPTION, &option) < 0) {
		os_log(LOG_ERR, "ioctl(NET_IOC_SET_OPTION) failed: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

static int net_avb_rx_bind(struct net_rx *rx, struct net_address *addr)
{
	if (ioctl(rx->fd, NET_IOC_BIND, addr) < 0) {
		os_log(LOG_ERR, "ioctl(NET_IOC_BIND) failed: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

static int __net_avb_rx_init(struct net_rx *rx, struct net_address *addr, void (*func)(struct net_rx *, struct net_rx_desc *),
		void (*func_multi)(struct net_rx *, struct net_rx_desc **, unsigned int), unsigned int packets, unsigned int latency, int epoll_fd)
{
	os_log(LOG_DEBUG, "enter\n");

	rx->fd = open(NET_RX_DEV, O_RDONLY);
	if (rx->fd < 0) {
		os_log(LOG_ERR, "open(%s) %s\n", NET_RX_DEV, strerror(errno));
		goto err_open;
	}

	if (packets && latency) {
		if (packets > NET_RX_BATCH)
			goto err_option;

		if (net_avb_rx_set_option(rx, SOCKET_OPTION_BUFFER_PACKETS, packets) < 0)
			goto err_option;

		if (net_avb_rx_set_option(rx, SOCKET_OPTION_BUFFER_LATENCY, latency) < 0)
			goto err_option;

		rx->batch = packets;
	} else {
		rx->batch = NET_RX_BATCH;
	}

	if (addr) {
		if (!logical_port_valid(addr->port)) {
			os_log(LOG_ERR, "logical_port(%u) invalid\n", addr->port);
			goto err_option;
		}

		if (net_avb_rx_bind(rx, addr) < 0)
			goto err_bind;

		rx->port_id = addr->port;

		if (addr->ptype != PTYPE_PTP) {
			rx->clock_domain = logical_port_to_gptp_clock(rx->port_id, PTP_DOMAIN_0);
		} else {
			rx->clock_domain = logical_port_to_local_clock(rx->port_id);
			rx->is_ptp = true;
		}
	}

	if (epoll_fd >= 0) {
		if (epoll_ctl_add(epoll_fd, rx->fd, EPOLL_TYPE_NET_RX, rx, &rx->epoll_data, EPOLLIN) < 0) {
			os_log(LOG_ERR, "net_rx(%p) epoll_ctl_add() failed\n", rx);
			goto err_epoll_ctl;
		}
	}

	rx->func = func;
	rx->func_multi = func_multi;
	rx->pool_type = POOL_TYPE_AVB;

	os_log(LOG_INFO, "fd(%d)\n", rx->fd);

	return 0;

err_epoll_ctl:
err_bind:
err_option:
	close(rx->fd);
	rx->fd = -1;

err_open:
	return -1;
}

int net_avb_rx_init(struct net_rx *rx, struct net_address *addr, void (*func)(struct net_rx *, struct net_rx_desc *), unsigned long epoll_fd)
{
	return __net_avb_rx_init(rx, addr, func, NULL, 1, 0, epoll_fd);
}

int net_avb_rx_init_multi(struct net_rx *rx, struct net_address *addr, void (*func)(struct net_rx *, struct net_rx_desc **, unsigned int), unsigned int packets, unsigned int time, unsigned long epoll_fd)
{
	return __net_avb_rx_init(rx, addr, NULL, func, packets, time, epoll_fd);
}

void net_avb_rx_exit(struct net_rx *rx)
{
	close(rx->fd);
	rx->fd = -1;

	os_log(LOG_INFO, "done\n");
}

void net_avb_rx_multi(struct net_rx *rx)
{
	unsigned long addr[NET_RX_BATCH];
	struct net_rx_desc *desc[NET_RX_BATCH];
	int len, i;

	len = read(rx->fd, addr, rx->batch * sizeof(unsigned long));
	if (len <= 0)
		len = 0;

	len /= sizeof(unsigned long);

	for (i = 0; i < len; i++)
		desc[i] = shmem_to_virt(addr[i]);

	/* 64bit timestamps not required in this path, only used by avtp */
	/*
	 * clock domain conversion is not required in this path either
	 * (assuming gptp and hardware clock domain are the same)
	 */

	rx->func_multi(rx, desc, len);
}

struct net_rx_desc *__net_avb_rx(struct net_rx *rx)
{
	unsigned long addr;
	struct net_rx_desc *desc;
	int len;

	len = read(rx->fd, &addr, sizeof(unsigned long));
	if (len <= 0)
		return NULL;

	desc = shmem_to_virt(addr);

	if (logical_port_is_endpoint(rx->port_id))
		desc->ts64 = hwts_to_u64(rx->clock_domain, desc->ts);

	clock_time_from_hw(rx->clock_domain, desc->ts64, &desc->ts64);

	return desc;
}

void net_avb_rx(struct net_rx *rx)
{
	unsigned long addr[NET_RX_BATCH];
	struct net_rx_desc *desc;
	int len, i;

	len = read(rx->fd, addr, rx->batch * sizeof(unsigned long));
	if (len <= 0)
		return;

	len /= sizeof(unsigned long);

	for (i = 0; i < len; i++) {
		desc = shmem_to_virt(addr[i]);

		if (logical_port_is_endpoint(rx->port_id))
			desc->ts64 = hwts_to_u64(rx->clock_domain, desc->ts);

		clock_time_from_hw(rx->clock_domain, desc->ts64, &desc->ts64);

		rx->func(rx, desc);
	}
}

static int net_avb_tx_connect(struct net_tx *tx, struct net_address *addr)
{
	if (ioctl(tx->fd, NET_IOC_CONNECT, addr) < 0) {
		os_log(LOG_ERR, "ioctl(NET_IOC_CONNECT) failed: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

int net_avb_tx_init(struct net_tx *tx, struct net_address *addr)
{
	os_log(LOG_DEBUG, "enter\n");

	tx->fd = open(NET_TX_DEV, O_RDWR);
	if (tx->fd < 0) {
		os_log(LOG_ERR, "open(%s) %s\n", NET_TX_DEV, strerror(errno));
		goto err_open;
	}

	if (addr) {
		if (net_avb_tx_connect(tx, addr) < 0)
			goto err_connect;

		tx->port_id = addr->port;

		if (addr->ptype != PTYPE_PTP)
			tx->clock_domain = logical_port_to_gptp_clock(tx->port_id, PTP_DOMAIN_0);
		else
			tx->clock_domain = logical_port_to_local_clock(tx->port_id);
	}

	tx->pool_type = POOL_TYPE_AVB;

	os_log(LOG_INFO, "fd(%d) logical_port(%u)\n", tx->fd, tx->port_id);

	return 0;

err_connect:
	close(tx->fd);
	tx->fd = -1;

err_open:
	return -1;
}

void net_avb_tx_exit(struct net_tx *tx)
{
	close(tx->fd);
	tx->fd = -1;

	os_log(LOG_INFO, "done\n");
}

int net_avb_tx(struct net_tx *tx, struct net_tx_desc *desc)
{
	unsigned long addr = virt_to_shmem(desc);

	if (write(tx->fd, &addr, sizeof(unsigned long)) < 0)
		return -1;

	return 1;
}

int net_avb_tx_multi(struct net_tx *tx, struct net_tx_desc **desc, unsigned int n)
{
	unsigned long addr[NET_TX_BATCH];
	unsigned int n_now, i;
	unsigned int written = 0;
	int len;
	int rc;

	while (written < n) {
		n_now = n - written;
		if (n_now > NET_TX_BATCH)
			n_now = NET_TX_BATCH;

		for (i = 0; i < n_now; i++)
			addr[i] = virt_to_shmem(desc[written + i]);

		len = n_now * sizeof(unsigned long);
		rc = write(tx->fd, addr, len);
		if (rc < len) {
			if (rc < 0)
				goto err;

			written += rc / sizeof(unsigned long);
			goto err;
		}

		written += n_now;
	}

	return written;

err:
	for (i = written; i < n; i++)
		net_tx_free(desc[i]);

	if (written)
		return written;
	else
		return -1;
}

int net_avb_tx_ts_get(struct net_tx *tx, uint64_t *ts, unsigned int *private)
{
	struct net_ts_desc ts_desc;
	int rc = 1;

	if (ioctl(tx->fd, NET_IOC_TS_GET, &ts_desc) < 0) {
		if (errno == EAGAIN)
			rc = 0;
		else {
			os_log(LOG_ERR, "ioctl(NET_IOC_TS) %s\n", strerror(errno));
			rc = -1;
		}

		goto err_ioctl;
	}

	if (logical_port_is_endpoint(tx->port_id))
		*ts = hwts_to_u64(tx->clock_domain, (uint32_t)ts_desc.ts);
	else
		*ts = ts_desc.ts;

	clock_time_from_hw(tx->clock_domain, *ts, ts);

	*private = ts_desc.priv;

	os_log(LOG_DEBUG, "fd(%d) ts %"PRIu64" type %u\n", tx->fd, *ts, *private);

err_ioctl:
	return rc;
}


int net_avb_tx_ts_init(struct net_tx *tx, struct net_address *addr, void (*func)(struct net_tx *, uint64_t, unsigned int), unsigned long priv)
{
	int epoll_fd = priv;

	os_log(LOG_DEBUG, "\n");

	if (addr->ptype != PTYPE_PTP)
		goto err_wrong_ptype;

	if (net_avb_tx_init(tx, addr) < 0)
		goto err_tx_init;

	if (logical_port_is_endpoint(addr->port))
		if (net_set_hw_ts(addr->port, true) < 0)
			goto err_tx_ts_enable;

	if (epoll_ctl_add(epoll_fd, tx->fd, EPOLL_TYPE_NET_TX_TS, tx, &tx->epoll_data, EPOLLIN) < 0) {
		os_log(LOG_ERR, "net_tx(%p) epoll_ctl_add() failed\n", tx);
		goto err_epoll_ctl;
	}

	tx->func_tx_ts = func;
	tx->epoll_fd = epoll_fd;

	return 0;

err_epoll_ctl:
	if (logical_port_is_endpoint(addr->port))
		net_set_hw_ts(addr->port, false);

err_tx_ts_enable:
	net_avb_tx_exit(tx);

err_tx_init:
err_wrong_ptype:
	return -1;
}


int net_avb_tx_ts_exit(struct net_tx *tx)
{
	if (epoll_ctl_del(tx->epoll_fd, tx->fd) < 0)
		os_log(LOG_ERR, "net_tx(%p) epoll_ctl_del() failed\n", tx);

	if (net_set_hw_ts(tx->port_id, false) < 0)
		os_log(LOG_ERR, "net_tx(%p) net_set_hw_ts() failed\n", tx);

	net_avb_tx_exit(tx);

	return 0;
}


unsigned int net_avb_tx_available(struct net_tx *tx)
{
	unsigned int tx_available = 0;

	if (ioctl(tx->fd, NET_IOC_GET_TX_AVAILABLE, &tx_available) < 0) {
		os_log(LOG_ERR, "ioctl(NET_IOC_GET_TX_AVAILABLE) %s\n", strerror(errno));
		tx_available = 0;
		goto err_ioctl;
	}

err_ioctl:
	return tx_available;
}

static inline int __net_avb_port_status(struct net_tx *tx, unsigned int port_id, bool *up, bool *point_to_point, uint64_t *rate)
{
	struct net_port_status status;

	status.port = port_id;

	if (ioctl(tx->fd, NET_IOC_PORT_STATUS, &status) < 0) {
		os_log(LOG_ERR, "ioctl(NET_IOC_PORT_STATUS) %s\n", strerror(errno));
		goto err;
	}

	*up = status.up;
	*point_to_point = status.full_duplex;
	*rate = status.rate;

	return 0;

err:
	return -1;
}

int net_avb_port_status(struct net_tx *tx, unsigned int port_id, bool *up, bool *point_to_point, uint64_t *rate)
{
	if (logical_port_is_bridge(port_id))
		return net_dflt_port_status(tx, port_id, up, point_to_point, rate);
	else
		return __net_avb_port_status(tx, port_id, up, point_to_point, rate);
}

static int __net_avb_multi(struct net_rx *rx, unsigned int port, unsigned long request, const unsigned char *hw_addr)
{
	struct net_mc_address addr;

	addr.port = port;
	memcpy(addr.hw_addr, hw_addr, 6);

	if (ioctl(rx->fd, request, &addr) < 0) {
		os_log(LOG_ERR, "ioctl() %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

int net_avb_add_multi(struct net_rx *rx, unsigned int port_id, const unsigned char *hw_addr)
{
	if (!logical_port_valid(port_id)) {
		os_log(LOG_ERR, "logical_port(%u) invalid\n", port_id);
		return -1;
	}

	os_log(LOG_INFO, "%s %02x:%02x:%02x:%02x:%02x:%02x\n", logical_port_name(port_id),
		hw_addr[0], hw_addr[1], hw_addr[2], hw_addr[3], hw_addr[4], hw_addr[5]);

	return __net_avb_multi(rx, port_id, NET_IOC_ADDMULTI, hw_addr);
}

int net_avb_del_multi(struct net_rx *rx, unsigned int port_id, const unsigned char *hw_addr)
{
	if (!logical_port_valid(port_id)) {
		os_log(LOG_ERR, "logical_port(%u) invalid\n", port_id);
		return -1;
	}

	os_log(LOG_INFO,"%s %02x:%02x:%02x:%02x:%02x:%02x\n", logical_port_name(port_id),
		hw_addr[0], hw_addr[1], hw_addr[2], hw_addr[3], hw_addr[4], hw_addr[5]);

	return __net_avb_multi(rx, port_id, NET_IOC_DELMULTI, hw_addr);
}

int net_avb_port_sr_config(unsigned int port_id, uint8_t *sr_class)
{
	struct net_sr_class_cfg sr_class_cfg;
	int i, fd;

	if ((fd = open(NET_TX_DEV, O_RDWR)) < 0) {
		os_log(LOG_ERR, "open(%s, O_RDWR) failed: %s\n", NET_TX_DEV, strerror(errno));
		goto err;
	}

	sr_class_cfg.port = port_id;
	for (i = 0; i < CFG_SR_CLASS_MAX; i++)
		sr_class_cfg.sr_class[i] = sr_class[i];

	if (ioctl(fd, NET_IOC_SR_CLASS_CONFIG, &sr_class_cfg) < 0) {
		os_log(LOG_ERR, "ioctl(NET_IOC_SR_CLASS_CONFIG) failed: %s\n", strerror(errno));
		goto err_ioctl;
	}

	close(fd);

	return 0;

err_ioctl:
	close(fd);
err:
	return -1;
}

void net_avb_exit(void)
{
	return;
}

const static struct net_rx_ops_cb net_rx_avb_ops = {
		.net_rx_init = net_avb_rx_init,
		.net_rx_init_multi = net_avb_rx_init_multi,
		.net_rx_exit = net_avb_rx_exit,
		.__net_rx = __net_avb_rx,
		.net_rx = net_avb_rx,
		.net_rx_multi = net_avb_rx_multi,

		.net_add_multi = net_avb_add_multi,
		.net_del_multi = net_avb_del_multi,
};

const static struct net_tx_ops_cb net_tx_avb_ops = {
		.net_tx_init = net_avb_tx_init,
		.net_tx_exit = net_avb_tx_exit,
		.net_tx = net_avb_tx,
		.net_tx_multi = net_avb_tx_multi,

		.net_tx_ts_get = net_avb_tx_ts_get,
		.net_tx_ts_init = net_avb_tx_ts_init,
		.net_tx_ts_exit = net_avb_tx_ts_exit,

		.net_tx_alloc = net_avb_tx_alloc,
		.net_tx_alloc_multi = net_avb_tx_alloc_multi,
		.net_tx_clone = net_avb_tx_clone,

		.net_tx_available = net_avb_tx_available,
		.net_port_status = net_avb_port_status,
		.net_port_mtu_get = net_dflt_port_mtu_size_get,
};

const static struct net_mem_ops_cb net_avb_mem_ops = {
		.net_tx_free = net_avb_tx_free,

		.net_rx_free = net_avb_rx_free,

		.net_free_multi = net_avb_free_multi,
};

int net_avb_socket_init(void *net_ops, bool is_rx)
{
	/* We copy the entire struct rather than just point to it, to reduce the number of
	 * indirections in performance-sensitive code.
	 */
	if (is_rx)
		memcpy(net_ops, &net_rx_avb_ops, sizeof(struct net_rx_ops_cb));
	else
		memcpy(net_ops, &net_tx_avb_ops, sizeof(struct net_tx_ops_cb));

	return 0;
}

int net_avb_init(struct net_mem_ops_cb *net_mem_ops)
{
	memcpy(net_mem_ops, &net_avb_mem_ops, sizeof(struct net_mem_ops_cb));

	return 0;
}
