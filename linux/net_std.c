/*
 * Copyright 2019-2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Linux specific Network service implementation
 @details
*/


#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <linux/net_tstamp.h>
#include <linux/in.h>
#include <linux/ethtool.h>
#include <linux/if_packet.h>
#include <linux/errqueue.h>
#include <linux/filter.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <time.h>

#include "common/log.h"
#include "common/net.h"
#include "common/ptp.h"
#include "clock.h"
#include "epoll.h"
#include "net.h"
#include "net_logical_port.h"
#include "net_std_socket_filters.h"

extern int net_set_hw_ts(unsigned int port_id, bool enable);


struct net_rx_desc *net_std_rx_alloc(unsigned int size)
{
	struct net_rx_desc *desc;

	if (size > DEFAULT_NET_DATA_SIZE)
		return NULL;

	desc = malloc(NET_DATA_OFFSET + size);
	if (!desc)
		goto exit;

	desc->flags = 0;
	desc->len = 0;
	desc->l2_offset = NET_DATA_OFFSET;
	desc->pool_type = POOL_TYPE_STD;
exit:
	return desc;
}

struct net_tx_desc *net_std_tx_alloc(unsigned int size)
{
	struct net_tx_desc *desc;

	if (size > DEFAULT_NET_DATA_SIZE)
		return NULL;

	desc = malloc(NET_DATA_OFFSET + size);
	if (!desc)
		goto exit;

	desc->flags = 0;
	desc->len = 0;
	desc->l2_offset = NET_DATA_OFFSET;
	desc->pool_type = POOL_TYPE_STD;
exit:
	return desc;
}

int net_std_tx_alloc_multi(struct net_tx_desc **desc, unsigned int n, unsigned int size)
{
	int i;

	for (i = 0; i < n; i++) {
		desc[i] = net_std_tx_alloc(size);
		if (!desc[i])
			goto err_malloc;
	}

err_malloc:
	return i;
}

struct net_tx_desc *net_std_tx_clone(struct net_tx_desc *src)
{
	struct net_tx_desc *desc = malloc(DEFAULT_NET_DATA_SIZE);
	if (!desc)
		goto exit;

	memcpy(desc, src, src->l2_offset + src->len);

	desc->pool_type = POOL_TYPE_STD;

exit:
	return desc;
}

void net_std_tx_free(struct net_tx_desc *buf)
{
	free((void *)buf);
}

void net_std_rx_free(struct net_rx_desc *buf)
{
	free((void *)buf);
}

void net_std_free_multi(void **buf, unsigned int n)
{
	int i;

	for (i = 0; i < n; i++)
		free(buf[i]);
}

/*
 * returns 1 if the ptype in the network address is supported, 0 otherwise.
 */
static bool net_address_is_supported(struct net_address *addr)
{
	bool rc;

	switch (addr->ptype) {
	case PTYPE_PTP:
	case PTYPE_MRP:
	case PTYPE_L2:
		rc = true;
		break;
	default:
		rc = false;
		break;
	}

	return rc;
}

static int net_std_set_socket_ts(unsigned int port_id, int fd, int tx, bool enable)
{
	int flags = 0;

	if (!logical_port_valid(port_id)) {
		os_log(LOG_ERR, "logical_port(%u) invalid\n", port_id);
		goto err_sockopt;
	}

	/* enable timestamping on the socket */
	if (enable) {
		if (tx) {
			/* Transmit timestamp are requested on per message basis, so
			we are not enabling the SOF_TIMESTAMPING_TX_HARDWARE here */
		#if 0
			/ * FIXME OPT_ID and TSONLY feature below are not yet
			functional, so disabling for now */

			/* This option associates each packet at send() with a unique
			identifier and returns that along with the timestamp */
			flags = SOF_TIMESTAMPING_OPT_ID;

			/* Applies to transmit timestamps only. Makes the kernel return the
			timestamp as a cmsg alongside an empty packet, as opposed to
			alongside the original packet */
			flags |= SOF_TIMESTAMPING_OPT_TSONLY;
		#endif
		} else {
			/* Request rx timestamps generated by the network adapter */
			flags = SOF_TIMESTAMPING_RX_HARDWARE;
		}

		/* Support recvmsg cmsg for all timestamped packets on transmit and receive */
		flags |= SOF_TIMESTAMPING_OPT_CMSG;

		/* Report hardware timestamps */
		flags |= SOF_TIMESTAMPING_RAW_HARDWARE;
	}

	if (setsockopt(fd, SOL_SOCKET, SO_TIMESTAMPING, &flags, sizeof(flags)) < 0) {
		os_log(LOG_ERR, "setsockopt(SO_TIMESTAMPING, 0x%x) failed: %s", flags, strerror(errno));
		goto err_sockopt;
	}

	if (tx) {
		flags = 1;
		if (setsockopt(fd, SOL_SOCKET, SO_SELECT_ERR_QUEUE, &flags, sizeof(flags)) < 0) {
			os_log(LOG_ERR, "setsockopt(SO_SELECT_ERR_QUEUE, 0x%x) failed: %s", flags, strerror(errno));
			goto err_sockopt;
		}
	}

	return 0;

err_sockopt:
	return -1;
}

static int net_std_get_cmsg_timestamp(struct msghdr *msg, uint64_t *ts)
{
	struct cmsghdr *cm;
	struct scm_timestamping *scm_ts = NULL;
	struct sock_extended_err *sock_exterr;

	for (cm = CMSG_FIRSTHDR(msg); cm != NULL; cm = CMSG_NXTHDR(msg, cm)) {
		if (cm->cmsg_level == SOL_SOCKET) {
			switch (cm->cmsg_type) {
			case SCM_TIMESTAMPING:
				if (cm->cmsg_len < sizeof(struct scm_timestamping)) {
					os_log(LOG_ERR, "short SCM_TIMESTAMPING message\n");
					return -1;
				}

				if (scm_ts != NULL)
					os_log(LOG_ERR, "SCM_TIMESTAMPING already parsed\n");

				scm_ts = (struct scm_timestamping *)CMSG_DATA(cm);
				break;

			case PACKET_TX_TIMESTAMP:
				sock_exterr = (struct sock_extended_err *)CMSG_DATA(cm);
				if (sock_exterr)
					os_log(LOG_INFO, "PACKET_TX_TIMESTAMP: errno %s origin %d info %d data %d\n", strerror(sock_exterr->ee_errno), sock_exterr->ee_origin, sock_exterr->ee_info, sock_exterr->ee_data);
				break;

			default:
				os_log(LOG_ERR, "cmsg_type unknown (%d)\n", cm->cmsg_type);
				break;
			}
		}
	}

	if (scm_ts) {
		*ts = (uint64_t)scm_ts->ts[2].tv_sec * NSECS_PER_SEC + scm_ts->ts[2].tv_nsec;
		return 1;
	} else {
		*ts = 0;
		return 0;
	}
}

static int net_std_rx_bind(struct net_rx *rx, struct net_address *addr)
{
	unsigned int index;
	struct sockaddr_ll sock_addr;
	struct sock_fprog prg;
	struct sock_filter bpf_filter[BPF_FILTER_MAX_ARRAY_SIZE];
	unsigned int filter_array_size = BPF_FILTER_MAX_ARRAY_SIZE;

	index = if_nametoindex(logical_port_name(addr->port));
	if (!index){
		os_log(LOG_ERR, "if_nametoindex error failed: %s\n", strerror(errno));
		goto err_itf;
	}

	if (sock_filter_get_bpf_code(addr, bpf_filter, &filter_array_size) < 0)
		goto err_bpf;

	prg.filter = bpf_filter;
	prg.len = filter_array_size;

	memset(&sock_addr, 0, sizeof(sock_addr));
	sock_addr.sll_ifindex = index;
	sock_addr.sll_family = AF_PACKET;
	sock_addr.sll_protocol = ntohs(ETH_P_ALL);
	if (bind(rx->fd, (struct sockaddr *) &sock_addr, sizeof(sock_addr)) < 0) {
		os_log(LOG_ERR, "bind failed: %s (%u)\n", logical_port_name(addr->port), index);
		goto err_bind;
	}

	if (setsockopt(rx->fd, SOL_SOCKET, SO_ATTACH_FILTER, &prg, sizeof(prg))) {
		os_log(LOG_ERR, "setsockopt(ATTACH_FILTER) failed: %s\n", strerror(errno));
		goto err_bind;
	}

	rx->port_id = addr->port;

	if (addr->ptype != PTYPE_PTP) {
		rx->clock_domain = logical_port_to_gptp_clock(rx->port_id, PTP_DOMAIN_0);
	} else {
		rx->clock_domain = logical_port_to_local_clock(rx->port_id);
		rx->is_ptp = true;
	}

	return 0;

err_bind:
err_bpf:
err_itf:
	return -1;
}

static int __net_std_rx_init(struct net_rx *rx, struct net_address *addr, void (*func)(struct net_rx *, struct net_rx_desc *),
		void (*func_multi)(struct net_rx *, struct net_rx_desc **, unsigned int), unsigned int packets, unsigned int latency, int epoll_fd)
{
	os_log(LOG_INFO, "enter\n");

	if (!addr || !net_address_is_supported(addr))
		goto err_addr;

	rx->fd = socket(PF_PACKET, SOCK_RAW | SOCK_NONBLOCK, htons(ETH_P_ALL));
	if (rx->fd < 0) {
		os_log(LOG_ERR, "socket failed: %s\n", strerror(errno));
		goto err_open_fd;
	}

	if (net_std_set_socket_ts(addr->port, rx->fd, 0, 1) < 0) {
		os_log(LOG_ERR, "net_set_socket_ts error\n");
		goto err_set_ts_enable;
	}

	if (net_std_rx_bind(rx, addr) < 0)
		goto err_bind;

	if (epoll_fd >= 0) {
		if (epoll_ctl_add(epoll_fd, rx->fd, EPOLL_TYPE_NET_RX, rx, &rx->epoll_data, EPOLLIN) < 0) {
			os_log(LOG_ERR, "net_rx(%p) epoll_ctl_add() failed\n", rx);
			goto err_epoll_ctl;
		}
	}

	rx->func = func;
	rx->func_multi = func_multi;

	rx->pool_type = POOL_TYPE_STD;

	os_log(LOG_INIT, "fd(%d)\n", rx->fd);

	return 0;

err_epoll_ctl:
err_bind:
err_set_ts_enable:
	close(rx->fd);
	rx->fd = -1;

err_open_fd:
err_addr:
	return -1;
}

int net_std_rx_init(struct net_rx *rx, struct net_address *addr, void (*func)(struct net_rx *, struct net_rx_desc *), unsigned long epoll_fd)
{
	return __net_std_rx_init(rx, addr, func, NULL, 1, 0, epoll_fd);
}

int net_std_rx_init_multi(struct net_rx *rx, struct net_address *addr, void (*func)(struct net_rx *, struct net_rx_desc **, unsigned int), unsigned int packets, unsigned int time, unsigned long epoll_fd)
{
	return __net_std_rx_init(rx, addr, NULL, func, packets, time, epoll_fd);
}

void net_std_rx_exit(struct net_rx *rx)
{
	close(rx->fd);
	rx->fd = -1;

	os_log(LOG_INFO, "done\n");
}

struct net_rx_desc *__net_std_rx(struct net_rx *rx)
{
	int cnt;
	struct iovec iov;
	struct msghdr msg;
	char control[128];
	struct sockaddr_ll sock_addr;
	struct net_rx_desc *desc;
	uint64_t ts;

	desc = net_std_rx_alloc(DEFAULT_NET_DATA_SIZE);
	if (desc) {
		memset(&msg, 0, sizeof(msg));

		iov.iov_base = NET_DATA_START(desc);
		iov.iov_len = DEFAULT_NET_DATA_SIZE;

		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;
		msg.msg_control = control;
		msg.msg_controllen = sizeof(control);
		msg.msg_name = &sock_addr;
		msg.msg_namelen = sizeof(sock_addr);

		cnt = recvmsg(rx->fd, &msg, 0);
		if (cnt < 0) {
			if (errno != EAGAIN)
				os_log(LOG_ERR, "recvmsg failed: %s\n", strerror(errno));

			net_std_rx_free(desc);
			return NULL;
		}

		desc->len = cnt;
		desc->port = rx->port_id;

		os_log(LOG_DEBUG, "recvmsg len %d on port %u\n", cnt, sock_addr.sll_ifindex);

		net_std_get_cmsg_timestamp(&msg, &ts);

		clock_time_from_hw(rx->clock_domain, ts, &ts);
		desc->ts = (uint32_t)ts;
		desc->ts64 = ts;

		net_std_rx_parser(rx, desc);
	}

	return desc;
}

void net_std_rx_multi(struct net_rx *rx)
{
	struct net_rx_desc *desc[NET_RX_BATCH];
	int i = 0;

	do {
		desc[i] = __net_std_rx(rx);
		if (!desc[i])
			break;
		i++;
	} while ( i < NET_RX_BATCH);

	rx->func_multi(rx, desc, i);
}

void net_std_rx(struct net_rx *rx)
{
	struct net_rx_desc *desc;

	desc = __net_std_rx(rx);
	if (desc)
		rx->func(rx, desc);
}

static int net_std_tx_bind(struct net_tx *tx, struct net_address *addr)
{
	unsigned int index;
	struct sockaddr_ll sock_addr;

	index = if_nametoindex(logical_port_name(addr->port));
	if (!index){
		os_log(LOG_ERR, "if_nametoindex failed: %s\n", strerror(errno));
		goto err_itf;
	}

	memset(&sock_addr, 0, sizeof(sock_addr));
	sock_addr.sll_ifindex = index;
	sock_addr.sll_family = AF_PACKET;
	sock_addr.sll_protocol = 0; /* socket for transmission only, no receive */

	if (bind(tx->fd, (struct sockaddr *) &sock_addr, sizeof(sock_addr)) < 0) {
		os_log(LOG_ERR, "bind failed: %s\n", strerror(errno));
		goto err_bind;
	}

	os_log(LOG_INIT, "bind %s (%u)\n", logical_port_name(addr->port), index);

	return 0;

err_bind:
err_itf:
	return -1;
}

static int net_std_tx_connect(struct net_tx *tx, struct net_address *addr)
{
	int opt_val = addr->priority;
	socklen_t opt_len = sizeof(opt_val);

	if (setsockopt(tx->fd, SOL_SOCKET, SO_PRIORITY, &opt_val, opt_len) < 0) {
		os_log(LOG_ERR, "setsockopt(SO_PRIORITY, %d) failed: %s\n", opt_val, strerror(errno));
		return -1;
	}

	return 0;
}

int net_std_tx_init(struct net_tx *tx, struct net_address *addr)
{
	if (addr && !net_address_is_supported(addr))
		goto err_addr;

	/* protocol 0 for AF_PACKET means socket for transmission only (the sll_protocol
	 * should also be 0 in bind) */
	tx->fd = socket(AF_PACKET, SOCK_RAW | SOCK_NONBLOCK, 0);
	if (tx->fd < 0) {
		os_log(LOG_ERR, "socket() failed: %s\n", strerror(errno));
		goto err_open_fd;
	}

	if (addr) {
		if (net_std_tx_bind(tx, addr) < 0)
			os_log(LOG_ERR, "net_std_tx_bind() failed: %s\n", strerror(errno));

		if (net_std_tx_connect(tx, addr) < 0)
			goto err_connect;

		tx->port_id = addr->port;

		if (addr->ptype != PTYPE_PTP)
			tx->clock_domain = logical_port_to_gptp_clock(tx->port_id, PTP_DOMAIN_0);
		else
			tx->clock_domain = logical_port_to_local_clock(tx->port_id);

		if (net_get_local_addr(tx->port_id, tx->eth_src) < 0)
			goto err_get_local;

		os_log(LOG_INIT, "fd(%d) logical_port(%u)\n", tx->fd, tx->port_id);
	} else {
		os_log(LOG_INIT, "fd(%d)\n", tx->fd);
	}

	tx->pool_type = POOL_TYPE_STD;

	return 0;

err_get_local:
err_connect:
	close(tx->fd);
	tx->fd = -1;

err_open_fd:
err_addr:
	return -1;
}

void net_std_tx_exit(struct net_tx *tx)
{
	close(tx->fd);
	tx->fd = -1;

	os_log(LOG_INFO, "done\n");
}

int net_std_tx(struct net_tx *tx, struct net_tx_desc *desc)
{
	struct eth_hdr *ethhdr = (struct eth_hdr *)NET_DATA_START(desc);
	struct msghdr msg;
	struct cmsghdr *cmsg;
	u32 *cmsg_data;
	struct iovec iov[1];
	char control[CMSG_SPACE(sizeof(__u32))];
	int rc = -1;

	memcpy(ethhdr->src, tx->eth_src, ETH_ALEN);

	iov[0].iov_base = NET_DATA_START(desc);
	iov[0].iov_len = desc->len;

	memset(&msg, 0, sizeof(struct msghdr));
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	msg.msg_name = NULL;
	msg.msg_namelen = 0;

	if (desc->flags & NET_TX_FLAGS_HW_TS) {
		msg.msg_control = control;
		msg.msg_controllen = sizeof(control);
		cmsg = CMSG_FIRSTHDR(&msg);
		cmsg->cmsg_level  = SOL_SOCKET;
		cmsg->cmsg_type = SO_TIMESTAMPING;
		cmsg->cmsg_len = CMSG_LEN(sizeof(__u32));
		cmsg_data = (u32 *)CMSG_DATA(cmsg);
		*cmsg_data = SOF_TIMESTAMPING_TX_HARDWARE;
		msg.msg_controllen = cmsg->cmsg_len;
	} else {
		msg.msg_control = NULL;
		msg.msg_controllen = 0;
	}

	if (sendmsg(tx->fd, &msg, 0) < 0) {
		os_log(LOG_ERR, "sendmsg() failed: %s (%d)\n", strerror(errno), tx->fd);
		goto err_send;
	}

	rc = 0;

	net_std_tx_free(desc);

err_send:
	return rc;
}

int net_std_tx_multi(struct net_tx *tx, struct net_tx_desc **desc, unsigned int n)
{
	unsigned int written = 0;
	int i, rc;

	while (written < n) {
		rc = net_std_tx(tx, desc[written]);
		if (rc < 0)
			goto err;

		written++;
	}

err:
	for (i = written; i < n; i++)
		net_std_tx_free(desc[i]);

	if (written)
		return written;
	else
		return -1;
}

int net_std_tx_ts_get(struct net_tx *tx, uint64_t *ts, unsigned int *private)
{
	char iobuf[256];
	struct iovec iov = {iobuf, 256};
	struct msghdr msg;
	char control[256];
	int rc;
	struct eth_hdr *ethhdr;
	unsigned int ether_type;

	memset(&msg, 0, sizeof(msg));

	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = control;
	msg.msg_controllen = sizeof(control);

	if (recvmsg(tx->fd, &msg, MSG_ERRQUEUE) < 0) {
		os_log(LOG_DEBUG, "recvmsg MSG_ERRQUEUE failed: %s\n", strerror(errno));
		return -1;
	}

	rc = net_std_get_cmsg_timestamp(&msg, ts);
	if (rc > 0) {
		ethhdr = (struct eth_hdr *)iobuf;
		ether_type = ethhdr->type;

		if (ether_type == htons(ETHERTYPE_PTP)) {
			struct ptp_hdr *ptp = (struct ptp_hdr *)(ethhdr + 1);

			/* retreiving message type from the timestamped packet itself */
			*private = (ptp->transport_specific << 24) | (ptp->domain_number << 16) | ptp->msg_type;
		} else {
			os_log(LOG_ERR, "logical_port(%u) TS on non-PTP type (%d)\n", tx->port_id, ether_type);
			rc = -1;
		}

		clock_time_from_hw(tx->clock_domain, *ts, ts);
	}

	return rc;
}

int net_std_tx_ts_init(struct net_tx *tx, struct net_address *addr, void (*func)(struct net_tx *, uint64_t, unsigned int), unsigned long priv)
{
	int epoll_fd = priv;

	os_log(LOG_DEBUG, "\n");

	if (addr->ptype != PTYPE_PTP)
		goto err_wrong_ptype;

	if (net_std_tx_init(tx, addr) < 0)
		goto err_tx_init;

	if (net_set_hw_ts(addr->port, true) < 0) {
		os_log(LOG_ERR, "net_set_hw_ts error\n");
		goto err_set_ts;
	}

	if (net_std_set_socket_ts(addr->port, tx->fd, 1, true) < 0) {
		os_log(LOG_ERR, "net_set_socket_ts error\n");
		goto err_set_ts;
	}

	if (epoll_ctl_add(epoll_fd, tx->fd, EPOLL_TYPE_NET_TX_TS, tx, &tx->epoll_data, 0) < 0) {
		os_log(LOG_ERR, "net_tx(%p) epoll_ctl_add() failed\n", tx);
		goto err_epoll_ctl;
	}

	tx->func_tx_ts = func;
	tx->epoll_fd = epoll_fd;

	return 0;

err_epoll_ctl:
err_set_ts:
	net_std_tx_exit(tx);

err_tx_init:
err_wrong_ptype:
	return -1;
}

int net_std_tx_ts_exit(struct net_tx *tx)
{
	if (epoll_ctl_del(tx->epoll_fd, tx->fd) < 0)
		os_log(LOG_ERR, "net_tx(%p) epoll_ctl_del() failed\n", tx);

	if (net_set_hw_ts(tx->port_id, false) < 0)
		os_log(LOG_ERR, "net_tx(%p) net_set_hw_ts() failed\n", tx);

	net_std_tx_exit(tx);

	return 0;
}

unsigned int net_std_tx_available(struct net_tx *tx)
{
	return 0;
}

void net_std_exit(void)
{
	return;
}

const static struct net_rx_ops_cb net_rx_std_ops = {
		.net_rx_init = net_std_rx_init,
		.net_rx_init_multi = net_std_rx_init_multi,
		.net_rx_exit = net_std_rx_exit,
		.__net_rx = __net_std_rx,
		.net_rx = net_std_rx,
		.net_rx_multi = net_std_rx_multi,

		.net_add_multi = net_std_add_multi,
		.net_del_multi = net_std_del_multi,
};

const static struct net_tx_ops_cb net_tx_std_ops = {
		.net_tx_init = net_std_tx_init,
		.net_tx_exit = net_std_tx_exit,
		.net_tx = net_std_tx,
		.net_tx_multi = net_std_tx_multi,

		.net_tx_alloc = net_std_tx_alloc,
		.net_tx_alloc_multi = net_std_tx_alloc_multi,
		.net_tx_clone = net_std_tx_clone,

		.net_tx_ts_get = net_std_tx_ts_get,
		.net_tx_ts_init = net_std_tx_ts_init,
		.net_tx_ts_exit = net_std_tx_ts_exit,

		.net_tx_available = net_std_tx_available,
		.net_port_status = net_dflt_port_status,
		.net_port_mtu_get = net_dflt_port_mtu_size_get,
};

const static struct net_mem_ops_cb net_std_mem_ops = {
		.net_tx_free = net_std_tx_free,

		.net_rx_free = net_std_rx_free,

		.net_free_multi = net_std_free_multi,
};

int net_std_socket_init(void *net_ops, bool is_rx)
{
	/* We copy the entire struct rather than just point to it, to reduce the number of
	 * indirections in performance-sensitive code.
	 */
	if (is_rx)
		memcpy(net_ops, &net_rx_std_ops, sizeof(struct net_rx_ops_cb));
	else
		memcpy(net_ops, &net_tx_std_ops, sizeof(struct net_tx_ops_cb));

	return 0;
}

int net_std_init(struct net_mem_ops_cb *net_mem_ops)
{
	memcpy(net_mem_ops, &net_std_mem_ops, sizeof(struct net_mem_ops_cb));

	return 0;
}
