/*
 * Copyright 2020-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LINUX_NET_LOGICAL_PORT_H_
#define _LINUX_NET_LOGICAL_PORT_H_

#include "os_config.h"

bool logical_port_valid(unsigned int port_id);
const char *logical_port_name(unsigned int port_id);
const char *logical_port_bridge_name(unsigned int port_id);
bool logical_port_is_endpoint(unsigned int port_id);
bool logical_port_is_bridge(unsigned int port_id);
bool logical_port_is_hybrid(unsigned int port_id);
unsigned int logical_port_endpoint_id(unsigned int port_id);
unsigned int logical_port_bridge_id(unsigned int port_id);
unsigned int logical_port_max(void);
unsigned int logical_port_to_bridge_port(unsigned int port_id);
void logical_port_init(struct os_logical_port_config *config);

#endif /* _LINUX_NET_LOGICAL_PORT_H_ */
