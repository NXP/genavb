/*
 * Copyright 2018, 2020-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file socket.h
 \brief GenAVB API private includes
 \details private definitions for the GenAVB library

 \copyright Copyright 2018, 2020-2021, 2023 NXP
*/

#ifndef _PRIVATE_SOCKET_H
#define _PRIVATE_SOCKET_H

#include "common/net.h"
#include "include/genavb/socket.h"

#define HEADER_TEMPLATE_SIZE 18

struct genavb_socket_rx {
	genavb_sock_f_t flags;
	struct net_rx net;
	struct genavb_socket_rx_params params;
	unsigned long priv;
};

struct genavb_socket_tx {
	genavb_sock_f_t flags;
	struct net_tx net;
	struct genavb_socket_tx_params params;
	uint8_t header_template[HEADER_TEMPLATE_SIZE];
	int header_len;
};

int socket_rx_event_init(struct genavb_socket_rx *sock);
void socket_rx_event_exit(struct genavb_socket_rx *sock);
int socket_rx_event_check(struct genavb_socket_rx *sock);
void socket_rx_event_rearm(struct genavb_socket_rx *sock);

#endif /* _PRIVATE_SOCKET_H */
