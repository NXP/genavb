/*
 * Copyright 2018-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVB logical port functions
 @details
*/

#include "net_logical_port.h"
#include "net_port.h"
#include "log.h"

struct logical_port_config {
	unsigned int type;
	unsigned int net_port_id;
	bool enabled;

	union {
		struct {
			unsigned int id;
			unsigned int port;
		} bridge;

		struct {
			unsigned int id;
		} endpoint;
	};
};

enum logical_port_type {
	LOGICAL_PORT_TYPE_ENDPOINT,
	LOGICAL_PORT_TYPE_BRIDGE
};

static struct logical_port_config port_config[CFG_LOGICAL_NUM_PORT];

static struct logical_port logical_port[CFG_LOGICAL_NUM_PORT];

bool logical_port_valid(unsigned int port)
{
	return ((port < CFG_LOGICAL_NUM_PORT) && port_config[port].enabled);
}

struct logical_port *__logical_port_get(unsigned int port)
{
	return &logical_port[port];
}

struct logical_port *logical_port_get(unsigned int port)
{
	if (!logical_port_valid(port))
		return NULL;

	return __logical_port_get(port);
}

bool logical_port_is_bridge(unsigned int port)
{
	return (port_config[port].type == LOGICAL_PORT_TYPE_BRIDGE);
}

bool logical_port_is_endpoint(unsigned int port)
{
	return (port_config[port].type == LOGICAL_PORT_TYPE_ENDPOINT);
}

unsigned int logical_port_endpoint_id(unsigned int port)
{
	return port_config[port].endpoint.id;
}

unsigned int logical_port_bridge_id(unsigned int port)
{
	return port_config[port].bridge.id;
}

unsigned int logical_port_max(void)
{
	return CFG_LOGICAL_NUM_PORT;
}

unsigned int logical_port_to_bridge_port(unsigned int port)
{
	return port_config[port].bridge.port;
}

/*
* Note: The logical port array has a specific mapping. The first two slots are reserved for endpoints ports and
* remaining slots are used for bridges ports e.g:
* 0 => 1st endpoint port
* 1 => 2nd endpoint port
* 2 => 1st bridge port (for the 1st bridge)
* 3 => 2nd bridge port (for the 1st bridge)
* 4 => 3rd bridge port (for the 1st bridge)
* etc...
*/
__init void logical_port_init(void)
{
	struct logical_port *port;
	int num_endpoints = 0, num_bridge_ports = 0;
	int i;

	for (i = 0; i < CFG_PORTS; i++) {
		switch (ports[i].drv_type) {
		case ENET_t:
		case ENET_1G_t:
		case ENET_QOS_t:
		case ENETC_1G_t:
		case ENETC_PSEUDO_1G_t:
			if ((num_endpoints < CFG_MAX_ENDPOINTS) && (num_endpoints < CFG_LOGICAL_NUM_PORT)) {
				port_config[num_endpoints].net_port_id = i;
				port_config[num_endpoints].enabled = true;
				port_config[num_endpoints].type = LOGICAL_PORT_TYPE_ENDPOINT;
				port_config[num_endpoints].endpoint.id = num_endpoints;

				os_log(LOG_INIT, "logical_port(%d) enabled, endpoint(%u)\n",
					num_endpoints, num_endpoints);

				num_endpoints++;
			}
			break;

#if CFG_NUM_NETC_SW
		case NETC_SW_t:
			if ((num_bridge_ports + CFG_MAX_ENDPOINTS) < CFG_LOGICAL_NUM_PORT) {
				port_config[num_bridge_ports + CFG_MAX_ENDPOINTS].net_port_id = i;
				port_config[num_bridge_ports + CFG_MAX_ENDPOINTS].enabled = true;
				port_config[num_bridge_ports + CFG_MAX_ENDPOINTS].type = LOGICAL_PORT_TYPE_BRIDGE;
				port_config[num_bridge_ports + CFG_MAX_ENDPOINTS].bridge.id = ports[i].drv_index;
				port_config[num_bridge_ports + CFG_MAX_ENDPOINTS].bridge.port = ports[i].base;

				os_log(LOG_INIT, "logical_port(%d) enabled, bridge(%u, %u)\n",
					num_bridge_ports + CFG_MAX_ENDPOINTS,
					logical_port_bridge_id(num_bridge_ports + CFG_MAX_ENDPOINTS),
					logical_port_to_bridge_port(num_bridge_ports + CFG_MAX_ENDPOINTS));

				num_bridge_ports++;
			}
			break;
#endif
		default:
			os_log(LOG_INIT, "logical_port(%d) disabled\n", i);
			break;
		}
	}

	os_log(LOG_INIT, "logical ports configuration: %u endpoints, %u bridge ports\n", num_endpoints, num_bridge_ports);

	for (i = 0; i < CFG_LOGICAL_NUM_PORT; i++) {
		port = __logical_port_get(i);

		port->id = i;

		if (!port_config[i].enabled)
			continue;

		if (port_config[i].type == LOGICAL_PORT_TYPE_ENDPOINT) {
			port->is_bridge = false;
		} else {
			port->is_bridge = true;
			port->bridge.port = port_config[i].bridge.port;
		}

		port->phys = &ports[port_config[i].net_port_id];
		port->phys->logical_port = port;
	}
}
