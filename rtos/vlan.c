/*
 * Copyright 2022-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief RTOS specific VLAN registration service implementation
 @details
*/

#include "genavb/error.h"
#include "genavb/ether.h"

#include "os/vlan.h"
#include "common/log.h"
#include "net_bridge.h"
#include "net_logical_port.h"

#if CFG_BRIDGE_NUM

#define vlan_bridge_port(port_id) (logical_port_valid(port_id) && logical_port_is_bridge(port_id) && (logical_port_bridge_id(port_id) == DEFAULT_BRIDGE_ID))

static inline bool vlan_invalid(uint16_t vid)
{
	return ((vid == VLAN_VID_MIN) || (vid > VLAN_VID_MAX));
}

int vlan_update(uint16_t vid, bool dynamic, struct genavb_vlan_port_map *map)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	struct logical_port *port;
	int rc = -1;

	if (vlan_invalid(vid)) {
		rc = -GENAVB_ERR_VLAN_VID;
		goto err;
	}

	if (dynamic) {
		if (map->control == GENAVB_VLAN_ADMIN_CONTROL_FORBIDDEN ||
		    map->control == GENAVB_VLAN_ADMIN_CONTROL_FIXED ||
		    map->control == GENAVB_VLAN_ADMIN_CONTROL_NORMAL) {
			rc = -GENAVB_ERR_VLAN_CONTROL;
			goto err;
		}
	} else {
		if (map->control == GENAVB_VLAN_ADMIN_CONTROL_REGISTERED ||
		    map->control == GENAVB_VLAN_ADMIN_CONTROL_NOT_REGISTERED) {
			rc = -GENAVB_ERR_VLAN_CONTROL;
			goto err;
		}
	}

	if (!vlan_bridge_port(map->port_id)) {
		rc = -GENAVB_ERR_INVALID_PORT;
		goto err;
	}

	port = __logical_port_get(map->port_id);

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (port->phys->drv_ops.vlan_update)
		rc = port->phys->drv_ops.vlan_update(port->phys, vid, dynamic, map);

	rtos_mutex_unlock(&bridge->mutex);
err:
	return rc;
}

int vlan_delete(uint16_t vid, bool dynamic)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -1;

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (bridge->drv_ops.vlan_delete)
		rc = bridge->drv_ops.vlan_delete(bridge, vid, dynamic);

	rtos_mutex_unlock(&bridge->mutex);

	return rc;
}

int vlan_read(uint16_t vid, bool *dynamic, struct genavb_vlan_port_map *map)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -1;

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (bridge->drv_ops.vlan_read)
		rc = bridge->drv_ops.vlan_read(bridge, vid, dynamic, map);

	rtos_mutex_unlock(&bridge->mutex);

	return rc;
}

int vlan_dump(uint32_t *token, uint16_t *vid, bool *dynamic, struct genavb_vlan_port_map *map)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -1;

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (bridge->drv_ops.vlan_dump)
		rc = bridge->drv_ops.vlan_dump(bridge, token, vid, dynamic, map);

	rtos_mutex_unlock(&bridge->mutex);

	return rc;
}

int vlan_port_set_default(unsigned int port_id, uint16_t vid)
{
	int rc = -GENAVB_ERR_VLAN_DEFAULT_NOT_SUPPORTED;
	struct logical_port *port;

	if (vlan_invalid(vid)) {
		rc = -GENAVB_ERR_VLAN_VID;
		goto err;
	}

	port = logical_port_get(port_id);
	if (!port) {
		rc = -GENAVB_ERR_INVALID_PORT;
		goto err;
	}

	rtos_mutex_lock(&port->phys->config_mutex, RTOS_WAIT_FOREVER);

	if (port->phys->drv_ops.set_pvid)
		rc = port->phys->drv_ops.set_pvid(port->phys, vid);

	rtos_mutex_unlock(&port->phys->config_mutex);

err:
	return rc;
}

int vlan_port_get_default(unsigned int port_id, uint16_t *vid)
{
	int rc = -GENAVB_ERR_VLAN_DEFAULT_NOT_SUPPORTED;
	struct logical_port *port;

	if (vid == NULL) {
		rc = -GENAVB_ERR_INVALID_PARAMS;
		goto err;
	}

	port = logical_port_get(port_id);
	if (!port) {
		rc = -GENAVB_ERR_INVALID_PORT;
		goto err;
	}

	rtos_mutex_lock(&port->phys->config_mutex, RTOS_WAIT_FOREVER);

	if (port->phys->drv_ops.get_pvid)
		rc = port->phys->drv_ops.get_pvid(port->phys, vid);

	rtos_mutex_unlock(&port->phys->config_mutex);

err:
	return rc;
}

#else
int vlan_update(uint16_t vid, bool dynamic, struct genavb_vlan_port_map *map) { return -1; }
int vlan_delete(uint16_t vid, bool dynamic) { return -1; }
int vlan_read(uint16_t vid, bool *dynamic, struct genavb_vlan_port_map *map) { return -1; }
int vlan_dump(uint32_t *token, uint16_t *vid, bool *dynamic, struct genavb_vlan_port_map *map) { return -1; }
int vlan_set_port_default(unsigned int port_id, uint16_t vid) { return -1; }
int vlan_get_port_default(unsigned int port_id, uint16_t *vid) { return -1; }
#endif
