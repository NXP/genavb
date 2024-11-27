/*
 * Copyright 2018-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVTP management functions
 @details
*/

#ifndef _RTOS_AVTP_H_
#define _RTOS_AVTP_H_

#include "genavb/avtp.h"
#include "genavb/sr_class.h"

#include "net_socket.h"
#include "net_rx.h"
#include "net_tx.h"
#include "slist.h"

#define STREAM_HASH	16

struct avtp_stream_rx_hdlr {
	struct slist_head sock_head[STREAM_HASH];
};

/* FIXME should be merge with above definition as a generic avtp_stream_hdlr */
struct avtp_stream_tx_hdlr {
	struct slist_head sock_head[STREAM_HASH];
};

struct avdecc_rx_hdlr {
	struct net_rx_stats stats[CFG_ENDPOINT_NUM];
	struct generic_rx_hdlr rx_hdlr[CFG_ENDPOINT_NUM];
};

struct maap_rx_hdlr {
	struct net_rx_stats stats[CFG_ENDPOINT_NUM];
	struct generic_rx_hdlr rx_hdlr[CFG_ENDPOINT_NUM];
};

struct avtp_rx_hdlr {
	struct avtp_stream_rx_hdlr stream[CFG_ENDPOINT_NUM];
	struct avdecc_rx_hdlr avdecc;
	struct maap_rx_hdlr maap;
};

struct avtp_tx_hdlr {
	struct avtp_stream_tx_hdlr stream[CFG_ENDPOINT_NUM];
};

extern struct avtp_rx_hdlr avtp_rx_hdlr;

void avtp_socket_unbind(struct net_socket *sock);
int avtp_socket_bind(struct net_socket *sock, struct net_address *addr);

void avtp_socket_disconnect(struct logical_port *port, struct net_socket *sock);
int avtp_socket_connect(struct logical_port *port, struct net_socket *sock, struct net_address *addr);

int avtp_rx(struct net_rx_ctx *net, struct logical_port *port, struct net_rx_desc *desc, void *hdr, unsigned int is_vlan);

static inline int stream_id_match(void *id1, void *id2)
{
	return (((u32 *)id1)[0] == ((u32 *)id2)[0]) && (((u32 *)id1)[1] == ((u32 *)id2)[1]);
}

static inline unsigned int stream_hash(void *stream_id)
{
	u8 *data = stream_id;

	return (data[0] ^ data[1] ^ data[2] ^ data[3] ^ data[4] ^ data[5] ^ data[6] ^ data[7]) & (STREAM_HASH - 1);
}

#endif /* _RTOS_AVTP_H_ */
