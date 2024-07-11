/*
* Copyright 2018, 2020-2021, 2023-2024 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 \file socket.c
 \brief control public API
 \details
 \copyright Copyright 2018, 2020-2021, 2023-2024 NXP
*/

#include "api/socket.h"

#if defined(CONFIG_SOCKET)
#include "genavb/error.h"
#include "os/stdlib.h"

int genavb_socket_rx_open(struct genavb_socket_rx **sock, genavb_sock_f_t flags,
			  struct genavb_socket_rx_params *params)
{
	int rc = 0;

	if (!sock) {
		rc = -GENAVB_ERR_INVALID;
		goto out;
	}

	if (flags & GENAVB_SOCKF_ZEROCOPY) {
		rc = -GENAVB_ERR_SOCKET_PARAMS;
		goto out;
	}

	*sock = os_malloc(sizeof(struct genavb_socket_rx));
	if (!*sock) {
		rc = -GENAVB_ERR_NO_MEMORY;
		goto out_set_null;
	}

	os_memset(*sock, 0, sizeof(struct genavb_socket_rx));

	os_memcpy(&(*sock)->params, params, sizeof(struct genavb_socket_rx_params));
	(*sock)->flags = flags;

	if (params->addr.port >= CFG_MAX_LOGICAL_PORTS) {
		rc = -GENAVB_ERR_SOCKET_PARAMS;
		goto out_free_socket;
	}

	if (socket_rx_event_init(*sock) < 0) {
		rc = -GENAVB_ERR_SOCKET_INIT;
		goto out_free_socket;
	}

	if (net_rx_init(&(*sock)->net, &params->addr, NULL, (*sock)->priv) < 0) {
		rc = -GENAVB_ERR_SOCKET_INIT;
		goto out_event_exit;
	}

	if (MAC_IS_MCAST(params->addr.u.l2.dst_mac)) {
		if (net_add_multi(&(*sock)->net, params->addr.port,
				  params->addr.u.l2.dst_mac) < 0)
			goto out_rx_exit;
	}

	return GENAVB_SUCCESS;

out_rx_exit:
	net_rx_exit(&(*sock)->net);

out_event_exit:
	socket_rx_event_exit(*sock);

out_free_socket:
	os_free(*sock);

out_set_null:
	*sock = NULL;

out:
	return rc;
}

int genavb_socket_tx_open(struct genavb_socket_tx **sock, genavb_sock_f_t flags,
			  struct genavb_socket_tx_params *params)
{
	int rc;
	struct net_address *addr;

	if (!sock) {
		rc = -GENAVB_ERR_INVALID;
		goto out;
	}

	if ((flags & GENAVB_SOCKF_ZEROCOPY) || (flags & GENAVB_SOCKF_NONBLOCK)) {
		rc = -GENAVB_ERR_SOCKET_PARAMS;
		goto out;
	}

	*sock = os_malloc(sizeof(struct genavb_socket_tx));
	if (!*sock) {
		rc = -GENAVB_ERR_NO_MEMORY;
		goto out;
	}
	os_memset(*sock, 0, sizeof(struct genavb_socket_tx));

	os_memcpy(&(*sock)->params, params, sizeof(struct genavb_socket_tx_params));
	(*sock)->flags = flags;

	if (params->addr.ptype == PTYPE_L2) {
		if (!(flags & GENAVB_SOCKF_RAW)) {
			/*
			 * Prepare L2 header, src MAC address
			 * is inserted in transmit path.
			 */
			if (params->addr.vlan_id != VLAN_VID_NONE) {
				(*sock)->header_len = net_add_eth_header(
					(*sock)->header_template,
					params->addr.u.l2.dst_mac,
					ETHERTYPE_VLAN);

				(*sock)->header_len += net_add_vlan_header(
					(*sock)->header_template + (*sock)->header_len,
					ntohs(params->addr.u.l2.protocol),
					ntohs(params->addr.vlan_id), params->addr.priority, 0);
			} else {
				(*sock)->header_len = net_add_eth_header(
					(*sock)->header_template,
					params->addr.u.l2.dst_mac,
					ntohs(params->addr.u.l2.protocol));
			}
		}
		addr = &params->addr;
	} else if (params->addr.ptype == PTYPE_PTP) {
		if (!(flags & GENAVB_SOCKF_RAW)) {
			/* use hardcoded PTP settings */
			const u8 ptp_dst_mac[6] = MC_ADDR_PTP;

			(*sock)->header_len = net_add_eth_header(
					(*sock)->header_template,
					ptp_dst_mac,
					ETHERTYPE_PTP);
		}
		addr = &params->addr;
	} else {
		rc = -GENAVB_ERR_INVALID;
		goto out_free_socket;
	}

	if (net_tx_init(&(*sock)->net, addr) < 0) {
		rc = -GENAVB_ERR_SOCKET_INIT;
		goto out_free_socket;
	}

	if (flags & GENAVB_SOCKF_RAW)
		net_tx_enable_raw(&(*sock)->net);

	return GENAVB_SUCCESS;

out_free_socket:
	os_free(*sock);

out:
	if (sock)
		*sock = NULL;

	return rc;
}

int genavb_socket_rx(struct genavb_socket_rx *sock, void *buf, unsigned int len, uint64_t *ts)
{
	struct net_rx_desc *desc;
	int rc;
	unsigned int data_len;

	if (!sock) {
		rc = -GENAVB_ERR_INVALID;
		goto out;
	}

	if (!buf) {
		rc = -GENAVB_ERR_SOCKET_FAULT;
		goto out;
	}

	if (socket_rx_event_check(sock) < 0) {
		rc = -GENAVB_ERR_SOCKET_INTR;
		goto out;
	}

	desc = __net_rx(&sock->net);
	if (!desc) {
		rc = -GENAVB_ERR_SOCKET_AGAIN;
		goto out_rearm;
	}

	if (sock->flags & GENAVB_SOCKF_RAW)
		data_len = desc->len;
	else
		data_len = desc->len - (desc->l3_offset - desc->l2_offset);

	if (len < data_len) {
		rc = -GENAVB_ERR_SOCKET_BUFLEN;
		goto out_free_desc;
	}

	if (sock->flags & GENAVB_SOCKF_RAW)
		os_memcpy(buf, (uint8_t *)desc + desc->l2_offset, data_len);
	else
		os_memcpy(buf, (uint8_t *)desc + desc->l3_offset, data_len);

	if (ts)
		*ts = desc->ts64;

	net_rx_free(desc);

	socket_rx_event_rearm(sock);

	return data_len;

out_free_desc:
	net_rx_free(desc);

out_rearm:
	socket_rx_event_rearm(sock);

out:
	return rc;
}

static int __genavb_socket_tx(struct genavb_socket_tx *sock, void *buf, unsigned int len, uint32_t ts_priv)
{
	struct net_tx_desc *desc;
	int rc;
	unsigned int data_len;

	if (!sock) {
		rc = -GENAVB_ERR_INVALID;
		goto out;
	}

	if (!buf) {
		rc = -GENAVB_ERR_SOCKET_FAULT;
		goto out;
	}

	if (sock->flags & GENAVB_SOCKF_RAW)
		data_len = len;
	else
		data_len = len + sock->header_len;

	desc = net_tx_alloc(&sock->net, data_len);
	if (!desc) {
		rc = -GENAVB_ERR_NO_MEMORY;
		goto out;
	}

	if (ts_priv) {
		desc->priv = ts_priv;
		desc->flags = NET_TX_FLAGS_HW_TS;
	}

	if (sock->flags & GENAVB_SOCKF_RAW)
		os_memcpy((uint8_t *)desc + desc->l2_offset, buf, data_len);
	else {
		os_memcpy((uint8_t *)desc + desc->l2_offset,
			  sock->header_template, sock->header_len);
		os_memcpy((uint8_t *)desc + desc->l2_offset + sock->header_len,
			  buf, len);
	}

	desc->len = data_len;
	desc->port = sock->params.addr.port;

	if (net_tx(&sock->net, desc) < 0) {
		rc = -GENAVB_ERR_SOCKET_TX;
		goto out_free_desc;
	}

	return GENAVB_SUCCESS;

out_free_desc:
	net_tx_free(desc);

out:
	return rc;
}

int genavb_socket_tx(struct genavb_socket_tx *sock, void *buf, unsigned int len)
{
	return __genavb_socket_tx(sock, buf, len, 0);
}

int genavb_socket_tx_ts(struct genavb_socket_tx *sock, void *buf, unsigned int len, unsigned int ts_priv)
{
	return __genavb_socket_tx(sock, buf, len, ts_priv);
}

int genavb_socket_tx_get_ts(struct genavb_socket_tx *sock, uint64_t *ts, unsigned int *ts_priv)
{
	int ret;

	ret = net_tx_ts_get(&sock->net, ts, ts_priv);
	if (ret <= 0)
		return -1;

	return GENAVB_SUCCESS;
}

void genavb_socket_rx_close(struct genavb_socket_rx *sock)
{
	struct net_address *addr;

	if (!sock)
		return;

	addr = &sock->params.addr;

	if (MAC_IS_MCAST(addr->u.l2.dst_mac)) {
		net_del_multi(&sock->net, addr->port,
			      addr->u.l2.dst_mac);
	}

	socket_rx_event_exit(sock);

	net_rx_exit(&sock->net);

	os_free(sock);

	return;
}

void genavb_socket_tx_close(struct genavb_socket_tx *sock)
{
	if (!sock)
		return;

	net_tx_exit(&sock->net);

	os_free(sock);

	return;
}

#else /* CONFIG_SOCKET */

int genavb_socket_rx_open(struct genavb_socket_rx **sock, genavb_sock_f_t flags, struct genavb_socket_rx_params *params)
{
	return -1;
}

int genavb_socket_tx_open(struct genavb_socket_tx **sock, genavb_sock_f_t flags, struct genavb_socket_tx_params *params)
{
	return -1;
}

int genavb_socket_rx(struct genavb_socket_rx *sock, void *buf, unsigned int len, uint64_t *ts)
{
	return -1;
}

int genavb_socket_tx(struct genavb_socket_tx *sock, void *buf, unsigned int len)
{
	return -1;
}

int genavb_socket_tx_ts(struct genavb_socket_tx *sock, void *buf, unsigned int len, unsigned int ts_priv)
{
	return -1;
}

int genavb_socket_tx_get_ts(struct genavb_socket_tx *sock, uint64_t *ts, unsigned int *ts_priv)
{
	return -1;
}

void genavb_socket_rx_close(struct genavb_socket_rx *sock)
{
	return;
}

void genavb_socket_tx_close(struct genavb_socket_tx *sock)
{
	return;
}

#endif /* CONFIG_SOCKET */
