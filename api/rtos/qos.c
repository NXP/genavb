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

#include "genavb/qos.h"

#include "os/net.h"
#include "os/qos.h"

unsigned int genavb_priority_to_traffic_class(unsigned int port_id, uint8_t priority)
{
	return net_port_priority_to_traffic_class(port_id, priority);
}

int genavb_traffic_class_set(unsigned int port_id, struct genavb_traffic_class_config *config)
{
	return net_port_traffic_class_set(port_id, config);
}

