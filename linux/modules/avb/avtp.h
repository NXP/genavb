/*
 * Copyright 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2020, 2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _AVTP_H_
#define _AVTP_H_

#include "genavb/avtp.h"
#include "genavb/sr_class.h"

#include "net_socket.h"
#include "net_logical_port.h"

extern unsigned int avtp_dt_bin_width_shift[SR_CLASS_MAX];

/* Remaining time to avtp presentation time */
static inline unsigned int sr_class_min_remaining_time(sr_class_t sr_class)
{
	return sr_class_max_transit_time(sr_class) / 2;
}

#define STREAM_HASH	16

void avtp_socket_unbind(struct net_socket *sock);
int avtp_socket_bind(struct net_socket *sock, struct net_address *addr);
void avtp_socket_disconnect(struct logical_port *port, struct net_socket *sock);
int avtp_socket_connect(struct logical_port *port, struct net_socket *sock, struct net_address *addr);
int avtp_stream_rx_any_ready(struct eth_avb *eth, unsigned int now);
void avtp_tx_wakeup(struct eth_avb *eth, struct net_socket **sock_array, unsigned int *n);
int avtp_rx(struct eth_avb *eth, struct net_rx_desc *desc, void *hdr, unsigned int is_vlan);

struct avtp_stream_rx_hdlr {
	struct hlist_head sock_head[STREAM_HASH];
};

/* FIXME should be merge with above definition as a generic avtp_stream_hdlr */
struct avtp_stream_tx_hdlr {
	struct hlist_head sock_head[STREAM_HASH];
};

struct avdecc_rx_hdlr {
	struct net_rx_stats stats[CFG_PORTS];
	struct generic_rx_hdlr rx_hdlr[CFG_PORTS];
};

struct maap_rx_hdlr {
	struct net_rx_stats stats[CFG_PORTS];
	struct generic_rx_hdlr rx_hdlr[CFG_PORTS];
};

struct avtp_rx_hdlr {
	struct avtp_stream_rx_hdlr stream[CFG_PORTS];
	struct avdecc_rx_hdlr avdecc;
	struct maap_rx_hdlr maap;
};

struct avtp_tx_hdlr {
	struct avtp_stream_tx_hdlr stream[CFG_PORTS];
};

extern struct avtp_rx_hdlr avtp_rx_hdlr;
extern struct avtp_tx_hdlr avtp_tx_hdlr;

static inline int stream_id_match(void *id1, void *id2)
{
	return (((u32 *)id1)[0] == ((u32 *)id2)[0]) && (((u32 *)id1)[1] == ((u32 *)id2)[1]);
}

static inline unsigned int stream_hash(void *stream_id)
{
	u8 *data = stream_id;

	return (data[0] ^ data[1] ^ data[2] ^ data[3] ^ data[4] ^ data[5] ^ data[6] ^ data[7]) & (STREAM_HASH - 1);
}

#endif /* _AVTP_H_ */
