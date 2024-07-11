/*
 * Copyright 2018, 2021-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVB logical port functions
 @details

 Copyright 2018 NXP
 All Rights Reserved.
*/

#ifndef _NET_LOGICAL_PORT_H_
#define _NET_LOGICAL_PORT_H_

#include "config.h"
#include <stdbool.h>

struct logical_port {
	bool is_bridge;

	union {
		struct {
			unsigned int port;
		} bridge;
	};

	struct net_port *phys; /* pointer to physical interface */

	struct net_socket *ptp_sock;

	unsigned int id;
};

bool logical_port_valid(unsigned int port);
struct logical_port *__logical_port_get(unsigned int port);
struct logical_port *logical_port_get(unsigned int port);
bool logical_port_is_bridge(unsigned int port);
bool logical_port_is_endpoint(unsigned int port);
unsigned int logical_port_endpoint_id(unsigned int port);
unsigned int logical_port_bridge_id(unsigned int port);
unsigned int logical_port_max(void);
unsigned int logical_port_to_bridge_port(unsigned int port);
void logical_port_init();

#endif /* _NET_LOGICAL_PORT_H_ */
