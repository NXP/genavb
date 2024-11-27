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

static int __stream_gate_set_admin_config(struct net_bridge *bridge, os_clock_id_t clk_id, struct genavb_stream_gate_instance *config)
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

	if (bridge->drv_ops.stream_gate_get_max_entries) {
		if (config->stream_gate_instance_id < bridge->drv_ops.stream_gate_get_max_entries(bridge))
			bridge->sg_base_time[config->stream_gate_instance_id] = config->base_time;
	}

	/* Check and update base time.
	 * IEEE 802.1Qci stream gates don't have any restrictions for base time value.
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

static int __stream_gate_update(struct net_bridge *bridge, os_clock_id_t clk_id, uint32_t index, struct genavb_stream_gate_instance *instance, unsigned int option)
{
	int rc = -GENAVB_ERR_SG_NOT_SUPPORTED;

	if (option == SG_UPDATE_OPTION_CONFIG) {
		rc = __stream_gate_set_admin_config(bridge, clk_id, instance);
		if (rc < 0)
			goto err;
	}

	if (bridge->drv_ops.stream_gate_update)
		rc = bridge->drv_ops.stream_gate_update(bridge, index, instance, option);

err:
	return rc;
}

int stream_gate_update(uint32_t index, os_clock_id_t clk_id, struct genavb_stream_gate_instance *instance, unsigned int option)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc;

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	rc = __stream_gate_update(bridge, clk_id, index, instance, option);

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

static int __stream_gate_read(struct net_bridge *bridge, uint32_t index, genavb_sg_config_type_t type, struct genavb_stream_gate_instance *instance)
{
	int rc = -GENAVB_ERR_SG_NOT_SUPPORTED;

	if (bridge->drv_ops.stream_gate_read)
		rc = bridge->drv_ops.stream_gate_read(bridge, index, type, instance);

	return rc;
}

int stream_gate_read(uint32_t index, genavb_sg_config_type_t type, struct genavb_stream_gate_instance *instance)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc;

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	rc = __stream_gate_read(bridge, index, type, instance);

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

static int stream_gate_clock_discontinuity(os_clock_id_t clk_id)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	struct genavb_stream_gate_control_entry *control_list;
	struct genavb_stream_gate_instance sg;
	int rc = 0;
	int index;

	control_list = rtos_malloc(stream_gate_control_get_max_entries() * sizeof(struct genavb_stream_gate_control_entry));
	if (!control_list) {
		os_log(LOG_ERR, "rtos_malloc() failed\n");
		rc = -1;
		goto err;
	}

	for (index = 0; index < stream_gate_get_max_entries(); index++) {
		sg.control_list = control_list;
		sg.list_length = stream_gate_control_get_max_entries();

		rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

		if (__stream_gate_read(bridge, index, GENAVB_SG_OPER, &sg) == GENAVB_SUCCESS) {
			sg.base_time = bridge->sg_base_time[index];

			if (__stream_gate_update(bridge, clk_id, index, &sg, SG_UPDATE_OPTION_CONFIG) < 0)
				rc = -1;
		}

		rtos_mutex_unlock(&bridge->mutex);
	}

	rtos_free(control_list);

err:
	return rc;
}

static int __flow_meter_update(struct net_bridge *bridge, uint32_t index, struct genavb_flow_meter_instance *instance, unsigned int option)
{
	int rc = -GENAVB_ERR_FM_NOT_SUPPORTED;

	if (bridge->drv_ops.flow_meter_update)
		rc = bridge->drv_ops.flow_meter_update(bridge, index, instance, option);

	return rc;
}

int flow_meter_update(uint32_t index, struct genavb_flow_meter_instance *instance, unsigned int option)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc;

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	rc = __flow_meter_update(bridge, index, instance, option);

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

static int __flow_meter_read(struct net_bridge *bridge, uint32_t index, struct genavb_flow_meter_instance *instance)
{
	int rc = -GENAVB_ERR_FM_NOT_SUPPORTED;

	if (bridge->drv_ops.flow_meter_read)
		rc = bridge->drv_ops.flow_meter_read(bridge, index, instance);

	return rc;
}

int flow_meter_read(uint32_t index, struct genavb_flow_meter_instance *instance)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc;

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	rc = __flow_meter_read(bridge, index, instance);

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

static int flow_meter_clock_discontinuity(os_clock_id_t clk_id)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	struct genavb_flow_meter_instance instance;
	int rc = 0;
	int index;

	for (index = 0; index < flow_meter_get_max_entries(); index++) {
		rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

		if (__flow_meter_read(bridge, index, &instance) == GENAVB_SUCCESS) {
			if (__flow_meter_update(bridge, index, &instance, FM_UPDATE_OPTION_CONFIG) < 0)
				rc = -1;
		}

		rtos_mutex_unlock(&bridge->mutex);
	}

	return rc;
}

void psfp_clock_discontinuity(os_clock_id_t clk_id)
{
	stream_gate_clock_discontinuity(clk_id);

	flow_meter_clock_discontinuity(clk_id);
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

void psfp_clock_discontinuity(os_clock_id_t clk_id) { }
#endif
