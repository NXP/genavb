/*
 * Copyright 2018-2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Packet socket management functions
 @details
*/

#ifndef _RTOS_PACKET_H_
#define _RTOS_PACKET_H_

#include "net_socket.h"
#include "net_rx.h"
#include "slist.h"

#define L2_HASH	16

struct l2_rx_hdlr {
	struct slist_head sock_list_head[L2_HASH];
};

void l2_socket_unbind(struct net_socket *sock);
int l2_socket_bind(struct net_socket *sock, struct net_address *addr);

void l2_socket_disconnect(struct logical_port *port, struct net_socket *sock);
int l2_socket_connect(struct logical_port *port, struct net_socket *sock, struct net_address *addr);

int l2_rx(struct net_rx_ctx *net, struct logical_port *port, struct net_rx_desc *desc);

static inline unsigned int l2_hash(void *dst_mac, uint16_t vlan_id)
{
	u8 *data = dst_mac;

	return (data[0] ^ data[1] ^ data[2] ^ data[3] ^ data[4] ^ data[5]
		^ ((vlan_id >> 8) & 0xFF) ^ (vlan_id & 0xFF))
		& (L2_HASH - 1);
}

#endif /* _RTOS_PACKET_H_ */
