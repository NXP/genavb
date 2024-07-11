/*
 * Copyright 2020-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file socket.c
 \brief control public API for linux
 \details
 \copyright Copyright 2020-2021, 2023 NXP
*/

#define _POSIX_C_SOURCE 200809L

#include "genavb/error.h"
#include "os/stdlib.h"
#include "api/socket.h"

int socket_rx_event_init(struct genavb_socket_rx *sock)
{
	if (sock->flags & GENAVB_SOCKF_NONBLOCK) {
		sock->priv = -1;
		return 0;
	} else {
		return -1;
	}
}

void socket_rx_event_exit(struct genavb_socket_rx *sock)
{
}

int socket_rx_event_check(struct genavb_socket_rx *sock)
{
	return 0;
}

void socket_rx_event_rearm(struct genavb_socket_rx *sock)
{
}

int genavb_socket_rx_fd(struct genavb_socket_rx *sock)
{
	if (sock->net.fd < 0)
		return -GENAVB_ERR_SOCKET_INVALID;

	return sock->net.fd;
}

int genavb_socket_tx_fd(struct genavb_socket_tx *sock)
{
	if (sock->net.fd < 0)
		return -GENAVB_ERR_SOCKET_INVALID;

	return sock->net.fd;
}
