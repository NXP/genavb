/*
* Copyright 2023-2024 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#include "os/sys_types.h"

#if defined(CONFIG_DSA)

#include "genavb/error.h"
#include "net_bridge.h"
#include "net_logical_port.h"

int net_port_dsa_add(unsigned int cpu_port, uint8_t *mac_addr, unsigned int slave_port)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -GENAVB_ERR_DSA_NOT_SUPPORTED;

	if (!mac_addr) {
		rc = -GENAVB_ERR_INVALID_PARAMS;
		goto err;
	}

	if (!(logical_port_valid(cpu_port) && logical_port_is_bridge(cpu_port) && (logical_port_bridge_id(cpu_port) == DEFAULT_BRIDGE_ID))) {
		rc = -GENAVB_ERR_INVALID_PORT;
		goto err;
	}

	if (!(logical_port_valid(slave_port) && logical_port_is_bridge(slave_port) && (logical_port_bridge_id(slave_port) == DEFAULT_BRIDGE_ID))) {
		rc = -GENAVB_ERR_INVALID_PORT;
		goto err;
	}

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (bridge->drv_ops.dsa_add)
		rc = bridge->drv_ops.dsa_add(bridge, cpu_port, mac_addr, slave_port);

	rtos_mutex_unlock(&bridge->mutex);

err:
	return rc;
}

int net_port_dsa_delete(unsigned int slave_port)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -GENAVB_ERR_DSA_NOT_SUPPORTED;

	if (!(logical_port_valid(slave_port) && logical_port_is_bridge(slave_port) && (logical_port_bridge_id(slave_port) == DEFAULT_BRIDGE_ID))) {
		rc = -GENAVB_ERR_INVALID_PORT;
		goto err;
	}

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (bridge->drv_ops.dsa_delete)
		rc = bridge->drv_ops.dsa_delete(bridge, slave_port);

	rtos_mutex_unlock(&bridge->mutex);

err:
	return rc;
}

#else
int net_port_dsa_add(unsigned int cpu_port, uint8_t *mac_addr, unsigned int slave_port) { return -1; }
int net_port_dsa_delete(unsigned int slave_port) { return -1; }
#endif
