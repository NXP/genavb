/*
* Copyright 2023-2024 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief RTOS specific PSFP service implementation
 @details
*/

#include "os/psfp.h"
#include "os/clock.h"
#include "common/log.h"
#include "genavb/error.h"
#include "clock.h"
#include "net_bridge.h"


#if CFG_BRIDGE_NUM
int stream_filter_update(uint32_t index, struct genavb_stream_filter_instance *instance)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -1;

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (bridge->drv_ops.stream_filter_update)
		rc = bridge->drv_ops.stream_filter_update(bridge, index, instance);

	rtos_mutex_unlock(&bridge->mutex);

	return rc;
}

int stream_filter_delete(uint32_t index)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -1;

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (bridge->drv_ops.stream_filter_delete)
		rc = bridge->drv_ops.stream_filter_delete(bridge, index);

	rtos_mutex_unlock(&bridge->mutex);

	return rc;
}

int stream_filter_read(uint32_t index, struct genavb_stream_filter_instance *instance)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -1;

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (bridge->drv_ops.stream_filter_read)
		rc = bridge->drv_ops.stream_filter_read(bridge, index, instance);

	rtos_mutex_unlock(&bridge->mutex);

	return rc;
}

unsigned int stream_filter_get_max_entries(void)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	unsigned int max_entries = 0;

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (bridge->drv_ops.stream_filter_get_max_entries)
		max_entries =  bridge->drv_ops.stream_filter_get_max_entries(bridge);

	rtos_mutex_unlock(&bridge->mutex);

	return max_entries;
}

static int __stream_gate_set_admin_config(genavb_clock_id_t clk_id, struct genavb_stream_gate_instance *config)
{
	u64 cycle_time, now;
	int rc = GENAVB_SUCCESS;

	if (!config->cycle_time_p || !config->cycle_time_q) {
		rc = -GENAVB_ERR_SG_INVALID_CYCLE_PARAMS;
		goto err;
	}

	cycle_time = ((uint64_t)NSECS_PER_SEC * config->cycle_time_p) / config->cycle_time_q;
	if (((uint64_t)cycle_time * config->cycle_time_q) !=
	    ((uint64_t)NSECS_PER_SEC * config->cycle_time_p)) {
		rc = -GENAVB_ERR_SG_INVALID_CYCLE_TIME;
		goto err;
	}

	/* Check and update base time.
	 * IEEE 802.1Qbv doesn't have any restrictions for base time value.
	 * If it's in the past, the schedule should start at (baseTime + N * cycleTime),
	 * with N being the minimum integer for which the time is in the future.
	 * However, some MAC don't support values in the past and this can be handled
	 * in software.
	 */
	if (os_clock_gettime64(clk_id, &now) < 0) {
		rc = -GENAVB_ERR_SG_GETTIME;
		goto err;
	}

	if (config->base_time < now) {
		uint64_t n;

		n = (now - config->base_time) / cycle_time;
		config->base_time = config->base_time + (n + 1) * cycle_time;
	}

	if (clock_time_to_cycles(clk_id, config->base_time, &config->base_time) < 0) {
		rc = -GENAVB_ERR_SG_INVALID_BASETIME;
		goto err;
	}

err:
	return rc;
}

int stream_gate_update(uint32_t index, os_clock_id_t clk_id, struct genavb_stream_gate_instance *instance, unsigned int option)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -GENAVB_ERR_SG_NOT_SUPPORTED;

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (bridge->drv_ops.stream_gate_update) {
		if (option == SG_UPDATE_OPTION_CONFIG) {
			rc = __stream_gate_set_admin_config(clk_id, instance);
			if (rc < 0)
				goto err;
		}
		rc = bridge->drv_ops.stream_gate_update(bridge, index, instance, option);
	}

err:
	rtos_mutex_unlock(&bridge->mutex);

	return rc;
}

int stream_gate_delete(uint32_t index)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -GENAVB_ERR_SG_NOT_SUPPORTED;

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (bridge->drv_ops.stream_gate_delete)
		rc = bridge->drv_ops.stream_gate_delete(bridge, index);

	rtos_mutex_unlock(&bridge->mutex);

	return rc;
}

int stream_gate_read(uint32_t index, genavb_sg_config_type_t type, struct genavb_stream_gate_instance *instance)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -GENAVB_ERR_SG_NOT_SUPPORTED;

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (bridge->drv_ops.stream_gate_read)
		rc = bridge->drv_ops.stream_gate_read(bridge, index, type, instance);

	rtos_mutex_unlock(&bridge->mutex);

	return rc;
}

unsigned int stream_gate_get_max_entries(void)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	unsigned int max_entries = 0;

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (bridge->drv_ops.stream_gate_get_max_entries)
		max_entries = bridge->drv_ops.stream_gate_get_max_entries(bridge);

	rtos_mutex_unlock(&bridge->mutex);

	return max_entries;
}

unsigned int stream_gate_control_get_max_entries(void)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	unsigned int max_entries = 0;

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (bridge->drv_ops.stream_gate_control_get_max_entries)
		max_entries = bridge->drv_ops.stream_gate_control_get_max_entries(bridge);

	rtos_mutex_unlock(&bridge->mutex);

	return max_entries;
}

int flow_meter_update(uint32_t index, struct genavb_flow_meter_instance *instance, unsigned int option)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -GENAVB_ERR_FM_NOT_SUPPORTED;

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (bridge->drv_ops.flow_meter_update)
		rc = bridge->drv_ops.flow_meter_update(bridge, index, instance, option);

	rtos_mutex_unlock(&bridge->mutex);

	return rc;
}

int flow_meter_delete(uint32_t index)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -GENAVB_ERR_FM_NOT_SUPPORTED;

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (bridge->drv_ops.flow_meter_delete)
		rc = bridge->drv_ops.flow_meter_delete(bridge, index);

	rtos_mutex_unlock(&bridge->mutex);

	return rc;
}

int flow_meter_read(uint32_t index, struct genavb_flow_meter_instance *instance)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -GENAVB_ERR_FM_NOT_SUPPORTED;

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (bridge->drv_ops.flow_meter_read)
		rc = bridge->drv_ops.flow_meter_read(bridge, index, instance);

	rtos_mutex_unlock(&bridge->mutex);

	return rc;
}

unsigned int flow_meter_get_max_entries(void)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	unsigned int max_entries = 0;

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (bridge->drv_ops.flow_meter_get_max_entries)
		max_entries = bridge->drv_ops.flow_meter_get_max_entries(bridge);

	rtos_mutex_unlock(&bridge->mutex);

	return max_entries;
}
#else
int stream_filter_update(uint32_t index, struct genavb_stream_filter_instance *instance) { return -1; }
int stream_filter_delete(uint32_t index) { return -1; }
int stream_filter_read(uint32_t index, struct genavb_stream_filter_instance *instance) { return -1; }
unsigned int stream_filter_get_max_entries(void) { return 0; }

int stream_gate_update(uint32_t index, os_clock_id_t clk_id, struct genavb_stream_gate_instance *instance, unsigned int option) { return -1; }
int stream_gate_delete(uint32_t index) { return -1; }
int stream_gate_read(uint32_t index, genavb_sg_config_type_t type, struct genavb_stream_gate_instance *instance) { return -1; }
unsigned int stream_gate_get_max_entries(void) { return 0; }
unsigned int stream_gate_control_get_max_entries(void) { return 0; }

int flow_meter_update(uint32_t index, struct genavb_flow_meter_instance *instance, unsigned int option) { return -1; }
int flow_meter_delete(uint32_t index) { return -1; }
int flow_meter_read(uint32_t index, struct genavb_flow_meter_instance *instance) { return -1; }
unsigned int flow_meter_get_max_entries(void) { return 0; }
#endif
