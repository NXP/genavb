/*
 * Copyright 2021-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief RTOS specific FDB service implementation
 @details
*/

#include "os/fdb.h"
#include "common/log.h"
#include "net_bridge.h"
#include "net_logical_port.h"


#if CFG_BRIDGE_NUM
int fdb_update(unsigned int port_id, uint8_t *address, uint16_t vid, bool dynamic, genavb_fdb_port_control_t control)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	struct logical_port *port;
	int rc = -1;

	if ((vid == 0) || (vid > 4095))
		goto err;

	if (!(logical_port_valid(port_id) && logical_port_is_bridge(port_id) && (logical_port_bridge_id(port_id) == DEFAULT_BRIDGE_ID)))
		goto err;

	port = __logical_port_get(port_id);

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (port->phys->drv_ops.fdb_update)
		rc = port->phys->drv_ops.fdb_update(port->phys, address, vid, dynamic, control);

	rtos_mutex_unlock(&bridge->mutex);
err:
	return rc;
}

int fdb_delete(uint8_t *address, uint16_t vid, bool dynamic)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -1;

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (bridge->drv_ops.fdb_delete)
		rc = bridge->drv_ops.fdb_delete(bridge, address, vid, dynamic);

	rtos_mutex_unlock(&bridge->mutex);

	return rc;
}

int fdb_read(uint8_t *address, uint16_t vid, bool *dynamic, struct genavb_fdb_port_map *map, genavb_fdb_status_t *status)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -1;

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (bridge->drv_ops.fdb_read)
		rc = bridge->drv_ops.fdb_read(bridge, address, vid, dynamic, map, status);

	rtos_mutex_unlock(&bridge->mutex);

	return rc;
}

int fdb_dump(uint32_t *token, uint8_t *address, uint16_t *vid, bool *dynamic, struct genavb_fdb_port_map *map, genavb_fdb_status_t *status)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -1;

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (bridge->drv_ops.fdb_dump)
		rc = bridge->drv_ops.fdb_dump(bridge, token, address, vid, dynamic, map, status);

	rtos_mutex_unlock(&bridge->mutex);

	return rc;
}
#else
int fdb_update(unsigned int port_id, uint8_t *address, uint16_t vid, bool dynamic, genavb_fdb_port_control_t control) { return -1; }
int fdb_delete(uint8_t *address, uint16_t vid, bool dynamic) { return -1; }
int fdb_read(uint8_t *address, uint16_t vid, bool *dynamic, struct genavb_fdb_port_map *map, genavb_fdb_status_t *status) { return -1; }
int fdb_dump(uint32_t *token, uint8_t *address, uint16_t *vid, bool *dynamic, struct genavb_fdb_port_map *map, genavb_fdb_status_t *status) { return -1; }
#endif
