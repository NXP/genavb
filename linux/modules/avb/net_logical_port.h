/*
 * Copyright 2018, 2020, 2022-2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _NET_LOGICAL_PORT_H_
#define _NET_LOGICAL_PORT_H_

#include "port_config.h"

#ifdef __KERNEL__

struct logical_port {
	int ifindex;

	struct eth_avb *eth; /* pointer to physical interface */

	struct net_socket *net_sock;

	unsigned int id;
};

struct avb_drv;

struct logical_port *__logical_port_get(struct avb_drv *avb, unsigned int port);
struct logical_port *logical_port_get(struct avb_drv *avb, unsigned int port);
int logical_port_init(struct avb_drv *avb);

char *physical_port_name(unsigned int port);

extern struct logical_port_config port_config[];

static inline bool logical_port_valid(unsigned int port)
{
	return ((port < CFG_LOGICAL_NUM_PORT) && port_config[port].enabled);
}

#endif /* __KERNEL__ */
#endif /* _NET_LOGICAL_PORT_H_ */
