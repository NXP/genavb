/*
 * Copyright 2020-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file generic.c
 \brief generic public API for rtos
 \details
 \copyright Copyright 2020-2021, 2023 NXP
*/

#include "genavb/log.h"
#include "genavb/error.h"

#include "common/log.h"
#include "os/net.h"

int genavb_log_level_set(genavb_log_component_id_t id, genavb_log_level_t level)
{
	if (log_level_set(id, level) < 0)
		return -GENAVB_ERR_INVALID;

	return GENAVB_SUCCESS;
}

int genavb_port_stats_get_number(unsigned int port_id)
{
	int size;

	size = net_port_stats_get_number(port_id);
	if (size < 0)
		return -GENAVB_ERR_INVALID;

	return size;
}

int genavb_port_stats_get_strings(unsigned int port_id, const char **buf, unsigned int buf_len)
{
	if (!buf)
		return -GENAVB_ERR_INVALID;

	if (net_port_stats_get_strings(port_id, buf, buf_len) < 0)
		return -GENAVB_ERR_INVALID;

	return GENAVB_SUCCESS;
}

int genavb_port_stats_get(unsigned int port_id, uint64_t *buf, unsigned int buf_len)
{
	if (!buf)
		return -GENAVB_ERR_INVALID;

	if (net_port_stats_get(port_id, buf, buf_len) < 0)
		return -GENAVB_ERR_INVALID;

	return GENAVB_SUCCESS;
}

int genavb_port_status_get(unsigned int port_id, bool *up, bool *duplex, unsigned int *rate)
{
	if (!up || !duplex || !rate)
		return -GENAVB_ERR_INVALID;

	return net_port_status_get(port_id, up, duplex, rate);
}

int genavb_port_set_max_frame_size(unsigned int port_id, uint16_t size)
{
	return net_port_set_max_frame_size(port_id, size);
}
