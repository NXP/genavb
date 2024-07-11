/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2024 NXP
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
#include <errno.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/ethtool.h>
#include <linux/if_ether.h>
#include <linux/net_tstamp.h>
#include <linux/sockios.h>

#include "genavb/qos.h"
#include "genavb/sr_class.h"
#include "genavb/helpers.h"
#include "genavb/avtp.h"
#include "os/string.h"
#include "os/assert.h"
#include "common/log.h"
#include "common/net.h"
#include "common/ipc.h"
#include "epoll.h"

#include "net.h"

#include "net_logical_port.h"

__attribute__((weak)) int net_avb_socket_init(void *net_ops, bool is_rx) { return -1; };
__attribute__((weak)) int net_std_socket_init(void *net_ops, bool is_rx) { return -1; };
__attribute__((weak)) int net_xdp_socket_init(void *net_ops, bool is_rx) { return -1; };
__attribute__((weak)) int net_ipc_socket_init(void *net_ops, bool is_rx) { return -1; };

__attribute__((weak)) int net_avb_init(struct net_mem_ops_cb *net_mem_ops) { return -1; };
__attribute__((weak)) int net_std_init(struct net_mem_ops_cb *net_mem_ops) { return -1; };
__attribute__((weak)) int net_ipc_init(struct net_mem_ops_cb *net_mem_ops) { return -1; };
__attribute__((weak)) int net_xdp_init(struct os_xdp_config *xdp_config, struct net_mem_ops_cb *net_mem_ops) { return -1; };

__attribute__((weak)) void net_avb_exit(void) { return; };
__attribute__((weak)) void net_std_exit(void) { return; };
__attribute__((weak)) void net_ipc_exit(void) { return; };
__attribute__((weak)) void net_xdp_exit(void) { return; };

__attribute__((weak)) int net_avb_port_sr_config(unsigned int port_id, uint8_t *sr_class) { return 0; };

static struct os_net_config net_config;

static struct net_mem_ops_cb net_mem_handler[POOL_TYPE_MAX] = {{ 0 }};

static int socket_fd = -1;

struct net_rx_desc *__net_rx(struct net_rx *rx)
{
	return rx->net_ops.__net_rx(rx);
}

void net_rx(struct net_rx *rx)
{
	return rx->net_ops.net_rx(rx);
}

void net_rx_multi(struct net_rx *rx)
{
	return rx->net_ops.net_rx_multi(rx);
}

static network_mode_t net_get_mode(struct net_address *addr)
{
	network_mode_t mode = CONFIG_DEFAULT_NET_MODE;

	/* Use the standard net mode for sockets with null addresses (e.g management) */
	if (!addr) {
		mode = NET_STD;
		goto out;
	}

	switch (addr->ptype) {
	case PTYPE_MRP:
		/* SRP packets on hybrid ports use ipc. */
		if (logical_port_is_hybrid(addr->port))
			mode = NET_IPC;
		else
			mode = net_config.srp_net_mode;

		break;
	case PTYPE_PTP:
		mode = net_config.gptp_net_mode;

		break;
	case PTYPE_L2:
		mode = net_config.api_sockets_net_mode;

		break;
	case PTYPE_AVTP:
		if (is_avtp_avdecc(addr->u.avtp.subtype))
			mode = net_config.avdecc_net_mode;
		else
			mode = net_config.avtp_net_mode;

		break;
	}

out:
	return mode;
}

static int net_ops_init(struct net_address *addr, void *net_ops, bool is_rx)
{
	int rc = 0;

	switch (net_get_mode(addr)) {
	case NET_AVB:
		net_avb_socket_init(net_ops, is_rx);
		break;
	case NET_STD:
		net_std_socket_init(net_ops, is_rx);
		break;
	case NET_XDP:
		net_xdp_socket_init(net_ops, is_rx);
		break;
	case NET_IPC:
		net_ipc_socket_init(net_ops, is_rx);
		break;
	default:
		rc = -1;
		break;
	}

	return rc;
}

int net_rx_init(struct net_rx *rx, struct net_address *addr, void (*func)(struct net_rx *, struct net_rx_desc *), unsigned long epoll_fd)
{
	if (net_ops_init(addr, &rx->net_ops, true))
		return -1;

	return rx->net_ops.net_rx_init(rx, addr, func, epoll_fd);
}

int net_rx_init_multi(struct net_rx *rx, struct net_address *addr, void (*func)(struct net_rx *, struct net_rx_desc **, unsigned int), unsigned int packets, unsigned int time, unsigned long epoll_fd)
{
	if (net_ops_init(addr, &rx->net_ops, true))
		return -1;

	return rx->net_ops.net_rx_init_multi(rx, addr, func, packets, time, epoll_fd);
}

void net_rx_exit(struct net_rx *rx)
{
	return rx->net_ops.net_rx_exit(rx);
}

int net_tx_init(struct net_tx *tx, struct net_address *addr)
{
	if (net_ops_init(addr, &tx->net_ops, false))
		return -1;

	return tx->net_ops.net_tx_init(tx, addr);
}

void net_tx_enable_raw(struct net_tx *tx)
{
	os_log(LOG_INFO, "net_tx_enable_raw() is still not supported");
}

void net_tx_exit(struct net_tx *tx)
{
	tx->net_ops.net_tx_exit(tx);
}

int net_tx(struct net_tx *tx, struct net_tx_desc *desc)
{
	return tx->net_ops.net_tx(tx, desc);
}

int net_tx_multi(struct net_tx *tx, struct net_tx_desc **desc, unsigned int n)
{
	return tx->net_ops.net_tx_multi(tx, desc, n);
}

void net_tx_ts_process(struct net_tx *tx)
{
	uint64_t ts;
	unsigned int private;
	int rc;

	while ((rc = tx->net_ops.net_tx_ts_get(tx, &ts, &private)) > 0)
		tx->func_tx_ts(tx, ts, private);
}

int net_tx_ts_get(struct net_tx *tx, uint64_t *ts, unsigned int *private)
{
	return tx->net_ops.net_tx_ts_get(tx, ts, private);
}

struct net_tx_desc *net_tx_alloc(struct net_tx *tx, unsigned int size)
{
	return tx->net_ops.net_tx_alloc(size);
}

int net_tx_alloc_multi(struct net_tx *tx, struct net_tx_desc **desc, unsigned int n, unsigned int size)
{
	return tx->net_ops.net_tx_alloc_multi(desc, n, size);
}

struct net_tx_desc *net_tx_clone(struct net_tx *tx, struct net_tx_desc *src)
{
	return tx->net_ops.net_tx_clone(src);
}

void net_tx_free(struct net_tx_desc *buf)
{
	unsigned int pool_type = buf->pool_type;

	if (pool_type >= POOL_TYPE_MAX || !net_mem_handler[pool_type].net_tx_free) {
		os_log(LOG_ERR, "desc(%p): Unsupported pool_type(%u)\n", buf, pool_type);
		return;
	}

	return net_mem_handler[pool_type].net_tx_free(buf);
}

void net_rx_free(struct net_rx_desc *buf)
{
	unsigned int pool_type = buf->pool_type;

	if (buf->pool_type >= POOL_TYPE_MAX || !net_mem_handler[pool_type].net_rx_free) {
		os_log(LOG_ERR, "desc(%p): Unsupported pool_type(%u)\n", buf, pool_type);
		return;
	}

	return net_mem_handler[pool_type].net_rx_free(buf);
}

void net_free_multi(void **buf, unsigned int n)
{
	unsigned int pool_type;

	if (n) {
		/* - Assume all buffers are from the same pool
		 * - Buffers can be from RX path, but cast to network tx
		 *   descriptor just to access the common part containing the pool
		 */
		pool_type = ((struct net_tx_desc *)buf[0])->pool_type;

		if (pool_type >= POOL_TYPE_MAX || !net_mem_handler[pool_type].net_free_multi) {
			os_log(LOG_ERR, "Unsupported pool_type(%u)\n", pool_type);
			return;
		}

		return net_mem_handler[pool_type].net_free_multi(buf, n);
	}
}

int net_set_hw_ts(unsigned int port_id, bool enable)
{
	struct ifreq hwtstamp;
	struct hwtstamp_config config;

	if (!logical_port_valid(port_id))
		return -1;

	/* set hw timestamping on the interface */
	memset(&hwtstamp, 0, sizeof(hwtstamp));
	strncpy(hwtstamp.ifr_name, logical_port_name(port_id), sizeof(hwtstamp.ifr_name) - 1);
	memset(&config, 0, sizeof(config));
	hwtstamp.ifr_data = (void *)&config;


	config.tx_type = (enable) ? HWTSTAMP_TX_ON : HWTSTAMP_TX_OFF;

	if (enable) {
		config.rx_filter = logical_port_is_bridge(port_id) ? HWTSTAMP_FILTER_PTP_V2_EVENT: HWTSTAMP_FILTER_ALL;
	} else {
		config.rx_filter = HWTSTAMP_FILTER_NONE;
	}

	if (ioctl(socket_fd, SIOCSHWTSTAMP, &hwtstamp) < 0) {
		os_log(LOG_ERR, "ioctl(SIOCSHWTSTAMP) %s, failed %s timestamping on %s\n", strerror(errno), (enable)?"enabling":"disabling", logical_port_name(port_id));
		goto err_ioctl;
	}

	os_log(LOG_INFO, "Configured HW timestamping to tx_type(%d) rx_filter(%d) on %s\n", config.tx_type, config.rx_filter, logical_port_name(port_id));

	return 0;

err_ioctl:
	return -1;
}

int net_tx_ts_init(struct net_tx *tx, struct net_address *addr, void (*func)(struct net_tx *, uint64_t, unsigned int), unsigned long priv)
{
	if (net_ops_init(addr, &tx->net_ops, false))
		return -1;

	return tx->net_ops.net_tx_ts_init(tx, addr, func, priv);
}

int net_tx_ts_exit(struct net_tx *tx)
{
	return tx->net_ops.net_tx_ts_exit(tx);
}

int net_get_local_addr(unsigned int port_id, unsigned char *addr)
{
	struct ifreq buf;

	if (!logical_port_valid(port_id)) {
		os_log(LOG_ERR, "logical_port(%u) invalid\n", port_id);
		goto err_portid;
	}

	memset(&buf, 0, sizeof(buf));

	h_strncpy(buf.ifr_name, logical_port_name(port_id), sizeof(buf.ifr_name));

	if (ioctl(socket_fd, SIOCGIFHWADDR, &buf) < 0) {
		os_log(LOG_ERR, "ioctl(SIOCGIFHWADDR) failed: %s\n", strerror(errno));
		goto err_ioctl;
	}

	os_memcpy(addr, buf.ifr_hwaddr.sa_data, ETH_ALEN);

	os_log(LOG_DEBUG,"%s %02x:%02x:%02x:%02x:%02x:%02x\n", logical_port_name(port_id),
		addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

	return 0;

err_ioctl:
err_portid:
	return -1;
}

int net_std_port_status(unsigned int port_id, bool *up, bool *point_to_point, unsigned int *rate)
{
	int rc = -1;
	struct ifreq if_req;
	struct ethtool_cmd eth_cmd;

	os_assert((up != NULL) && (point_to_point != NULL) && (rate != NULL));

	if_req.ifr_ifindex = if_nametoindex(logical_port_name(port_id));
	if (!if_req.ifr_ifindex) {
		os_log(LOG_ERR, "if_nametoindex error failed: %s %s\n", strerror(errno), logical_port_name(port_id));
		goto err_itf;
	}

	h_strncpy(if_req.ifr_name, logical_port_name(port_id), IF_NAMESIZE);

	if (ioctl(socket_fd, SIOCGIFFLAGS, &if_req) < 0) {
		os_log(LOG_ERR, "ioctl(SIOCGIFFLAGS) failed: %s %s\n", strerror(errno), logical_port_name(port_id));
		goto err_ioctl;
	}

	/* up: true if port is up, false otherwise */
	*up = (if_req.ifr_flags & IFF_RUNNING) ? 1 : 0;

	eth_cmd.cmd = ETHTOOL_GSET;
	if_req.ifr_data = (void *)&eth_cmd;

	if (ioctl(socket_fd, SIOCETHTOOL, &if_req) < 0) {
		os_log(LOG_ERR, "ioctl(SIOCETHTOOL) failed: %s\n", strerror(errno));
		goto err_ioctl;
	}

	/* rate:  port rate in bits per second (valid only if up is true) */
	*rate = ethtool_cmd_speed(&eth_cmd);

	/* point_to_point: true if port is full duplex (valid only if up is true) */
	*point_to_point = eth_cmd.duplex;

	rc = 0;

err_ioctl:
err_itf:
	return rc;
}

int net_dflt_port_status(struct net_tx *tx, unsigned int port_id, bool *up, bool *point_to_point, unsigned int *rate)
{
	return net_std_port_status(port_id, up, point_to_point, rate);
}

int net_port_status(struct net_tx *tx, unsigned int port_id, bool *up, bool *point_to_point, unsigned int *rate)
{
	if (!logical_port_valid(port_id))
		return -1;

	return tx->net_ops.net_port_status(tx, port_id, up, point_to_point, rate);
}

static int __net_std_multi(unsigned int port, unsigned long request, const unsigned char *hw_addr)
{
	struct ifreq ifr = {0};

	memcpy(ifr.ifr_name, logical_port_name(port), IFNAMSIZ);
	memcpy(ifr.ifr_hwaddr.sa_data, hw_addr, 6);

	if (ioctl(socket_fd, request, (char *)&ifr) < 0) {
		os_log(LOG_ERR, "ioctl failed: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

int net_std_add_multi(struct net_rx *rx, unsigned int port_id, const unsigned char *hw_addr)
{
	if (!logical_port_valid(port_id)) {
		os_log(LOG_ERR, "logical_port(%u) invalid\n", port_id);
		return -1;
	}

	os_log(LOG_INFO, "%s %02x:%02x:%02x:%02x:%02x:%02x\n", logical_port_name(port_id),
		hw_addr[0], hw_addr[1], hw_addr[2], hw_addr[3], hw_addr[4], hw_addr[5]);

	return __net_std_multi(port_id, SIOCADDMULTI, hw_addr);
}

int net_std_del_multi(struct net_rx *rx, unsigned int port_id, const unsigned char *hw_addr)
{
	if (!logical_port_valid(port_id)) {
		os_log(LOG_ERR, "logical_port(%u) invalid\n", port_id);
		return -1;
	}

	os_log(LOG_INFO,"%s %02x:%02x:%02x:%02x:%02x:%02x\n", logical_port_name(port_id),
		hw_addr[0], hw_addr[1], hw_addr[2], hw_addr[3], hw_addr[4], hw_addr[5]);

	return __net_std_multi(port_id, SIOCDELMULTI, hw_addr);
}


int net_add_multi(struct net_rx *rx, unsigned int port_id, const unsigned char *hw_addr)
{
	return rx->net_ops.net_add_multi(rx, port_id, hw_addr);
}

int net_del_multi(struct net_rx *rx, unsigned int port_id, const unsigned char *hw_addr)
{
	return rx->net_ops.net_del_multi(rx, port_id, hw_addr);
}

void net_std_rx_parser(struct net_rx *rx, struct net_rx_desc *desc)
{
	struct eth_hdr *ethhdr = (struct eth_hdr *)NET_DATA_START(desc);
	uint16_t ether_type = ethhdr->type;

	if (ether_type == htons(ETHERTYPE_VLAN)) {
		struct vlanhdr *vlan = (void *)(ethhdr + 1);
		desc->ethertype = ntohs(vlan->type);
		desc->l3_offset = desc->l2_offset + sizeof(struct eth_hdr) + sizeof(struct vlanhdr);
		desc->vid = VLAN_VID(vlan);
	} else {
		desc->ethertype = ntohs(ether_type);
		desc->l3_offset = desc->l2_offset + sizeof(struct eth_hdr);
		desc->vid = 0;
	}

	desc->l4_offset = 0;
	desc->l5_offset = 0;
	desc->flags = 0;
	desc->priv = 0;
}

int net_tx_event_enable(struct net_tx *tx, unsigned long priv)
{
	os_log(LOG_DEBUG, "tx(%p) priv %lu\n", tx, priv);

	tx->epoll_fd = priv;

	if (epoll_ctl_add(tx->epoll_fd, tx->fd, EPOLL_TYPE_NET_TX_EVENT, tx, &tx->epoll_data, EPOLLOUT) < 0) {
		os_log(LOG_ERR, "tx(%p) epoll_ctl_add() failed\n", tx);
		goto err_epoll_ctl;
	}

	return 0;

err_epoll_ctl:
	return -1;
}

int net_tx_event_disable(struct net_tx *tx)
{
	os_log(LOG_DEBUG, "tx(%p) epoll_fd %d\n", tx, tx->epoll_fd);

	if (epoll_ctl_del(tx->epoll_fd, tx->fd) < 0) {
		os_log(LOG_ERR, "tx(%p) epoll_ctl_del() failed\n", tx);
		goto err_epoll_ctl;
	}

	return 0;

err_epoll_ctl:
	return -1;
}

unsigned int net_tx_available(struct net_tx *tx)
{
	return tx->net_ops.net_tx_available(tx);
}

int net_port_sr_config(unsigned int port_id, uint8_t *sr_class)
{
	int rc = 0;

	/* Only the avb net mode needs this configureation. */
	if (HAS_NET_MODE_ENABLED(net_config.enabled_modes_flag, NET_AVB))
		rc = net_avb_port_sr_config(port_id, sr_class);

	return rc;
}

unsigned int net_port_priority_to_traffic_class(unsigned int port_id, uint8_t priority)
{
	const u8 *map;

	if (!logical_port_valid(port_id))
		return 0;

	if (priority > QOS_PRIORITY_MAX)
		return 0;

	/* FIXME retrieve more detailed information from the port */
	if (logical_port_is_bridge(port_id))
		map = priority_to_traffic_class_map(QOS_TRAFFIC_CLASS_MAX, QOS_SR_CLASS_MAX);
	else
		map = priority_to_traffic_class_map(CFG_TRAFFIC_CLASS_MAX, CFG_SR_CLASS_MAX);

	if (!map)
		return 0;

	return map[priority];
}

int bridge_software_maclearn(bool enable)
{
	return -1;
}

int net_init(struct os_net_config *config, struct os_xdp_config *xdp_config)
{
	socket_fd = socket(PF_INET, SOCK_DGRAM, 0);
	if (socket_fd < 0) {
		os_log(LOG_ERR, "socket() failed: %s\n", strerror(errno));
		goto err_sock;
	}

	/* Save the network config */
	os_memcpy(&net_config, config, sizeof(struct os_net_config));

	if (HAS_NET_MODE_ENABLED(net_config.enabled_modes_flag, NET_AVB)) {
		if (net_avb_init(&net_mem_handler[POOL_TYPE_AVB]) < 0) {
			os_log(LOG_ERR, "Could not initialize AVB network service implementation\n");
			goto err;
		}
	}

	if (HAS_NET_MODE_ENABLED(net_config.enabled_modes_flag, NET_IPC)) {
		if (net_ipc_init(&net_mem_handler[POOL_TYPE_IPC]) < 0) {
			os_log(LOG_ERR, "Could not initialize IPC network service implementation\n");
			goto err;
		}
	}

	if (HAS_NET_MODE_ENABLED(net_config.enabled_modes_flag, NET_STD)) {
		if (net_std_init(&net_mem_handler[POOL_TYPE_STD]) < 0) {
			os_log(LOG_ERR, "Could not initialize STD network service implementation\n");
			goto err;
		}
	}

	if (HAS_NET_MODE_ENABLED(net_config.enabled_modes_flag, NET_XDP)) {
		if (net_xdp_init(xdp_config, &net_mem_handler[POOL_TYPE_XDP]) < 0) {
			os_log(LOG_ERR, "Could not initialize XDP network service implementation\n");
			goto err;
		}
	}

	os_log(LOG_INIT, "done\n");

	return 0;

err:
	close(socket_fd);

	socket_fd = -1;
err_sock:
	return -1;
}

void net_exit(void)
{
	if (HAS_NET_MODE_ENABLED(net_config.enabled_modes_flag, NET_XDP))
		net_xdp_exit();

	if (HAS_NET_MODE_ENABLED(net_config.enabled_modes_flag, NET_STD))
		net_std_exit();

	if (HAS_NET_MODE_ENABLED(net_config.enabled_modes_flag, NET_IPC))
		net_ipc_exit();

	if (HAS_NET_MODE_ENABLED(net_config.enabled_modes_flag, NET_AVB))
		net_avb_exit();

	close(socket_fd);

	socket_fd = -1;
}
