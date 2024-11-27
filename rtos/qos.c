/*
 * Copyright 2020-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief RTOS specific QoS implementation
 @details
*/

#include "common/log.h"
#include "os/clock.h"
#include "clock.h"

#include "genavb/error.h"

#include "net_port.h"
#include "net_logical_port.h"

int __qos_st_set_admin_config(struct logical_port *port, os_clock_id_t clk_id,
			    struct genavb_st_config *config)
{
	u64 cycle_time, now;

	if (!config->enable)
		goto set_config;

	if (clock_id(clk_id) != clock_id(port->phys->clock_st_psfp)) {
		os_log(LOG_ERR, "clock id: %u no supported for port(%u)\n",
		       clk_id, port->id);
		goto err;
	}

	if (!clock_has_hw_adjust_ratio(clk_id)) {
		os_log(LOG_ERR, "clock id: %u hw clock ratio is not adjusted\n",
		       clk_id, port->id);
		goto err;
	}

	if (!config->cycle_time_p || !config->cycle_time_q)
		goto err;

	cycle_time = ((uint64_t)NSECS_PER_SEC * config->cycle_time_p) / config->cycle_time_q;
	if (((uint64_t)cycle_time * config->cycle_time_q) !=
	    ((uint64_t)NSECS_PER_SEC * config->cycle_time_p)) {
		os_log(LOG_ERR, "only integer cycle time is supported\n");
		goto err;
	}

	port->phys->st_base_time = config->base_time;

	/* Check and update base time.
	 * IEEE 802.1Qbv doesn't have any restrictions for base time value.
	 * If it's in the past, the schedule should start at (baseTime + N * cycleTime),
	 * with N being the minimum integer for which the time is in the future.
	 * However, some MAC don't support values in the past and this can be handled
	 * in software.
	 */
	if (os_clock_gettime64(clk_id, &now) < 0) {
		os_log(LOG_ERR, "os_clock_gettime64() error\n");
		goto err;
	}

	if (config->base_time < now) {
		uint64_t n;

		n = (now - config->base_time) / cycle_time;
		config->base_time = config->base_time + (n + 1) * cycle_time;
	}

	if (clock_time_to_cycles(clk_id, config->base_time, &config->base_time) < 0) {
		os_log(LOG_ERR, "clock_time_to_cycles error\n");
		goto err;
	}

set_config:
	port->phys->st_enabled = (bool)config->enable;

	return port_set_st_config(port->phys, config);

err:
	return -1;
}

int qos_st_set_admin_config(unsigned int port_id, os_clock_id_t clk_id,
			    struct genavb_st_config *config)
{
	struct logical_port *port;
	int rc;

	port = logical_port_get(port_id);
	if (!port)
		return -1;

	rtos_mutex_lock(&port->phys->config_mutex, RTOS_WAIT_FOREVER);

	rc = __qos_st_set_admin_config(port, clk_id, config);

	rtos_mutex_unlock(&port->phys->config_mutex);

	return rc;
}

static int __qos_st_get_config(struct logical_port *port, genavb_st_config_type_t type,
		      struct genavb_st_config *config, unsigned int list_length)
{
	if (port_get_st_config(port->phys, type, config, list_length) < 0)
		goto err;

	config->base_time = port->phys->st_base_time;

	return 0;

err:
	return -1;
}

int qos_st_get_config(unsigned int port_id, genavb_st_config_type_t type,
		      struct genavb_st_config *config, unsigned int list_length)
{
	struct logical_port *port;
	int rc;

	port = logical_port_get(port_id);
	if (!port)
		return -1;

	rtos_mutex_lock(&port->phys->config_mutex, RTOS_WAIT_FOREVER);

	rc = __qos_st_get_config(port, type, config, list_length);

	rtos_mutex_unlock(&port->phys->config_mutex);

	return rc;
}

int qos_st_set_max_sdu(unsigned int port_id, struct genavb_st_max_sdu *queue_max_sdu, unsigned int n)
{
	struct logical_port *port;
	int rc;

	port = logical_port_get(port_id);
	if (!port) {
		rc = -GENAVB_ERR_INVALID_PORT;
		goto exit;
	}

	rtos_mutex_lock(&port->phys->config_mutex, RTOS_WAIT_FOREVER);

	rc = port_st_set_max_sdu(port->phys, queue_max_sdu, n);

	rtos_mutex_unlock(&port->phys->config_mutex);

exit:
	return rc;
}

int qos_st_get_max_sdu(unsigned int port_id, struct genavb_st_max_sdu *queue_max_sdu)
{
	struct logical_port *port;
	int rc;

	port = logical_port_get(port_id);
	if (!port) {
		rc = -GENAVB_ERR_INVALID_PORT;
		goto exit;
	}

	rtos_mutex_lock(&port->phys->config_mutex, RTOS_WAIT_FOREVER);

	rc = port_st_get_max_sdu(port->phys, queue_max_sdu);

	rtos_mutex_unlock(&port->phys->config_mutex);

exit:
	return rc;
}

static int qos_st_update(unsigned int port_id, os_clock_id_t clk_id)
{
	struct genavb_st_gate_control_entry *control_list;
	struct genavb_st_config config;
	struct logical_port *port;
	int max_entries;
	int rc = 0;

	port = logical_port_get(port_id);
	if (!port) {
		rc = -1;
		goto err;
	}

	rtos_mutex_lock(&port->phys->config_mutex, RTOS_WAIT_FOREVER);

	if (port->phys->st_enabled && (clock_id(port->phys->clock_st_psfp) == clock_id(clk_id))) {
		max_entries = port_st_max_entries(port->phys);
		if (max_entries < 0) {
			rc = -1;
			goto err_max_entries;
		}

		control_list = rtos_malloc(max_entries * sizeof(struct genavb_st_gate_control_entry));
		if (!control_list) {
			rc = -1;
			goto err_malloc;
		}
		memset(control_list, 0, max_entries * sizeof(struct genavb_st_gate_control_entry));
		config.control_list = control_list;


		rc = __qos_st_get_config(port, GENAVB_ST_OPER, &config, max_entries);
		if (rc < 0)
			goto err_get_st_config;

		if (clock_has_hw_setoffset(clk_id) && config.enable) {
			config.enable = 0;
			__qos_st_set_admin_config(port, clk_id, &config);
			config.enable = 1;
		}

		rc = __qos_st_set_admin_config(port, clk_id, &config);

err_get_st_config:
		rtos_free(control_list);
	}

err_malloc:
err_max_entries:
	rtos_mutex_unlock(&port->phys->config_mutex);

err:
	return rc;
}

int qos_clock_discontinuity(os_clock_id_t clk_id)
{
	struct logical_port *port;
	int i;

	for (i = 0; i < CFG_LOGICAL_NUM_PORT; i++) {
		port = logical_port_get(i);
		if (port)
			qos_st_update(i, clk_id);
	}

	return 0;
}
