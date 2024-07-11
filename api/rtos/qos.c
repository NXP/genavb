/*
 * Copyright 2020-2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file qos.c
 \brief qos public API for rtos
 \details
 \copyright Copyright 2020-2021, 2023-2024 NXP
*/

#include "api/clock.h"
#include "genavb/qos.h"
#include "genavb/error.h"

#include "os/net.h"
#include "os/qos.h"

int genavb_st_set_admin_config(unsigned int port_id, genavb_clock_id_t clk_id,
			       struct genavb_st_config *config)
{
	os_clock_id_t os_id;
	int rc = GENAVB_SUCCESS;

	if (!config) {
		rc = -GENAVB_ERR_INVALID;
		goto out;
	}

	if (config->enable &&
	   (config->list_length && !config->control_list)) {
		rc = -GENAVB_ERR_INVALID;
		goto out;
	}

	if (clk_id >= GENAVB_CLOCK_MAX) {
		rc = -GENAVB_ERR_CLOCK;
		goto out;
	}

	os_id = genavb_clock_to_os_clock(clk_id);
	if (os_id >= OS_CLOCK_MAX) {
		rc = -GENAVB_ERR_CLOCK;
		goto out;
	}

	if (qos_st_set_admin_config(port_id, os_id, config) < 0) {
		rc = -GENAVB_ERR_ST;
		goto out;
	}

out:
	return rc;
}

int genavb_st_get_config(unsigned int port_id, genavb_st_config_type_t type,
			 struct genavb_st_config *config, unsigned int list_length)
{
	int rc = GENAVB_SUCCESS;

	if (!config || !config->control_list) {
		rc = -GENAVB_ERR_INVALID;
		goto out;
	}

	if (qos_st_get_config(port_id, type, config, list_length) < 0) {
		rc = -GENAVB_ERR_ST;
		goto out;
	}

out:
	return rc;
}

int genavb_st_set_max_sdu(unsigned int port_id, struct genavb_st_max_sdu *queue_max_sdu, unsigned int n)
{
	int rc;

	if (!queue_max_sdu) {
		rc = -GENAVB_ERR_INVALID;
		goto out;
	}

	rc = qos_st_set_max_sdu(port_id, queue_max_sdu, n);

out:
	return rc;
}

int genavb_st_get_max_sdu(unsigned int port_id, struct genavb_st_max_sdu *queue_max_sdu)
{
	int rc;

	if (!queue_max_sdu) {
		rc = -GENAVB_ERR_INVALID;
		goto out;
	}

	rc = qos_st_get_max_sdu(port_id, queue_max_sdu);

out:
	return rc;
}

int genavb_fp_set(unsigned int port_id, genavb_fp_config_type_t type, struct genavb_fp_config *config)
{
	int rc = GENAVB_SUCCESS;

	if (!config) {
		rc = -GENAVB_ERR_INVALID;
		goto out;
	}

	if (qos_fp_set(port_id, type, config) < 0) {
		rc = -GENAVB_ERR_INVALID;
		goto out;
	}

out:
	return rc;
}

int genavb_fp_get(unsigned int port_id, genavb_fp_config_type_t type, struct genavb_fp_config *config)
{
	int rc = GENAVB_SUCCESS;

	if (!config) {
		rc = -GENAVB_ERR_INVALID;
		goto out;
	}

	if (qos_fp_get(port_id, type, config) < 0) {
		rc = -GENAVB_ERR_INVALID;
		goto out;
	}

out:
	return rc;
}

unsigned int genavb_priority_to_traffic_class(unsigned int port_id, uint8_t priority)
{
	return net_port_priority_to_traffic_class(port_id, priority);
}
