/*
 * Copyright 2018-2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file socket.c
 \brief control public API for rtos
 \details
 \copyright Copyright 2018-2021, 2023-2024 NXP
*/

#include "rtos_abstraction_layer.h"

#include "common/ipc.h"
#include "rtos/net_port.h"
#include "rtos/net_socket.h"

#include "api/socket.h"

#define SOCKET_EVENT_QUEUE_LENGTH	16

int socket_rx_event_init(struct genavb_socket_rx *sock)
{
	rtos_mqueue_t *event_queue_h = NULL;

	if (sock->flags & GENAVB_SOCKF_NONBLOCK) {
		goto exit;
	} else {
		event_queue_h = rtos_mqueue_alloc_init(SOCKET_EVENT_QUEUE_LENGTH, sizeof(struct event));
		if (!event_queue_h) {
			goto err;
		}
	}

exit:
	sock->priv = (unsigned long)event_queue_h;

	return 0;

err:
	return -1;
}

void socket_rx_event_exit(struct genavb_socket_rx *sock)
{
	if (sock->priv)
		rtos_mqueue_destroy((rtos_mqueue_t *)sock->priv);
}

int socket_rx_event_check(struct genavb_socket_rx *sock)
{
	if (sock->priv) {
		struct event e;

		/* Blocking */
		if (rtos_mqueue_receive((rtos_mqueue_t *)sock->priv, &e, RTOS_WAIT_FOREVER) < 0)
			goto err;

		if (e.type != EVENT_TYPE_NET_RX)
			goto err;
	}

	return 0;

err:
	return -1;
}

void socket_rx_event_rearm(struct genavb_socket_rx *sock)
{
	if (sock->priv)
		net_rx_enable_callback(&sock->net);
}

int genavb_socket_rx_set_callback(struct genavb_socket_rx *sock, void (*callback)(void *), void *data)
{
	int rc;

	if (!sock) {
		rc = -GENAVB_ERR_INVALID;
		goto err;
	}

	if (net_rx_set_callback(&sock->net, callback, data) < 0) {
		rc = -GENAVB_ERR_CTRL_INIT;
		goto err;
	}

	return GENAVB_SUCCESS;

err:
	return rc;
}

int genavb_socket_rx_enable_callback(struct genavb_socket_rx *sock)
{
	int rc;

	if (!sock) {
		rc = -GENAVB_ERR_INVALID;
		goto err;
	}

	if (net_rx_enable_callback(&sock->net) < 0) {
		rc = -GENAVB_ERR_CTRL_INIT;
		goto err;
	}

	return GENAVB_SUCCESS;

err:
	return rc;
}

int genavb_socket_tx_set_callback(struct genavb_socket_tx *sock, void (*callback)(void *), void *data)
{
	int rc;

	if (!sock) {
		rc = -GENAVB_ERR_INVALID;
		goto err;
	}

	if (net_tx_set_callback(&sock->net, callback, data) < 0) {
		rc = -GENAVB_ERR_CTRL_INIT;
		goto err;
	}

	return GENAVB_SUCCESS;

err:
	return rc;
}

int genavb_socket_tx_enable_callback(struct genavb_socket_tx *sock)
{
	int rc;

	if (!sock) {
		rc = -GENAVB_ERR_INVALID;
		goto err;
	}

	if (net_tx_enable_callback(&sock->net) < 0) {
		rc = -GENAVB_ERR_CTRL_INIT;
		goto err;
	}

	return GENAVB_SUCCESS;

err:
	return rc;
}

int genavb_socket_get_hwaddr(unsigned int port_id, unsigned char *addr)
{
	return socket_get_hwaddr(port_id, addr);
}
