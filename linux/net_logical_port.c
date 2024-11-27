/*
 * Copyright 2020-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdbool.h>

#include "genavb/helpers.h"

#include "common/log.h"

#include "modules/port_config.h"

#include "net_logical_port.h"

static struct logical_port_config port_config[CFG_MAX_LOGICAL_PORTS] = {0, };
static struct {
	char name[IFNAMSIZ];
} bridge_config[CFG_MAX_BRIDGES];

bool logical_port_valid(unsigned int port_id)
{
	return ((port_id < CFG_MAX_LOGICAL_PORTS) && port_config[port_id].enabled);
}

const char *logical_port_name(unsigned int port_id)
{
	return port_config[port_id].name;
}

const char *logical_port_bridge_name(unsigned int port_id)
{
	return bridge_config[port_config[port_id].bridge.id].name;
}

bool logical_port_is_bridge(unsigned int port_id)
{
	return (port_config[port_id].type == LOGICAL_PORT_TYPE_BRIDGE);
}

bool logical_port_is_endpoint(unsigned int port_id)
{
	return (port_config[port_id].type == LOGICAL_PORT_TYPE_PHYSICAL);
}

bool logical_port_is_hybrid(unsigned int port_id)
{
	return (port_config[port_id].is_hybrid);
}

unsigned int logical_port_endpoint_id(unsigned int port_id)
{
	return port_config[port_id].physical.id;
}

unsigned int logical_port_bridge_id(unsigned int port_id)
{
	return port_config[port_id].bridge.id;
}

unsigned int logical_port_max(void)
{
	return CFG_MAX_LOGICAL_PORTS;
}

unsigned int logical_port_to_bridge_port(unsigned int port_id)
{
	return port_config[port_id].bridge.port;
}

static void logical_port_get_config(struct logical_port_config *port, const char *name, const char *hybrid_port_name)
{
	port->is_hybrid = false;

	if (!strlen(name) || !strcmp(name, "off")) {
		port->enabled = false;
	} else {
		h_strncpy(port->name, name, IF_NAMESIZE);
		port->enabled = true;

		if (strlen(hybrid_port_name) && !strncmp(port->name, hybrid_port_name, strlen(port->name)))
			port->is_hybrid = true;
	}
}

void logical_port_init(struct os_logical_port_config *config)
{
	int i, j;

	for (i = 0; i < CFG_MAX_ENDPOINTS; i++) {
		port_config[i].type = LOGICAL_PORT_TYPE_PHYSICAL;
		port_config[i].physical.id = i;

		logical_port_get_config(&port_config[i], config->endpoint[i], config->endpoint_hybrid_port);

		if (port_config[i].enabled)
			os_log(LOG_INIT, "logical_port(%d) enabled, endpoint(%u, %s) %s\n",
				i, logical_port_endpoint_id(i), logical_port_name(i), logical_port_is_hybrid(i) ? "hybrid" : "");
		else
			os_log(LOG_INIT, "logical_port(%d) disabled\n", i);
	}

	for (i = 0; i < CFG_MAX_BRIDGES; i++) {
		/* Get the bridge interface name */
		if (strlen(config->bridge[i]) && strcmp(config->bridge[i], "off"))
			h_strncpy(bridge_config[i].name, config->bridge[i], IF_NAMESIZE);

		for (j = 0; j < CFG_MAX_NUM_PORT; j++) {
			unsigned int port_id = CFG_MAX_ENDPOINTS + i * CFG_MAX_NUM_PORT + j;

			port_config[port_id].type = LOGICAL_PORT_TYPE_BRIDGE;
			port_config[port_id].bridge.id = i;
			port_config[port_id].bridge.port = j;

			logical_port_get_config(&port_config[port_id], config->bridge_ports[i][j], config->bridge_hybrid_port);

			if (port_config[port_id].enabled)
				os_log(LOG_INIT, "logical_port(%d) enabled, bridge(%s, %u, %u, %s) %s\n", port_id, bridge_config[i].name, logical_port_bridge_id(port_id),
				       logical_port_to_bridge_port(port_id), logical_port_name(port_id), logical_port_is_hybrid(port_id) ? "hybrid" : "");
			else
				os_log(LOG_INIT, "logical_port(%d) disabled\n", port_id);
		}
	}
}
