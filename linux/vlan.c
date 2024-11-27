/*
 * Copyright 2022-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Linux specific VLAN registration service implementation
 @details
*/

#include <stdint.h>
#include <stdbool.h>

#include "os/vlan.h"


int vlan_update(uint16_t vid, bool dynamic, struct genavb_vlan_port_map *map)
{
	return -1;
}

int vlan_delete(uint16_t vid, bool dynamic)
{
	return -1;
}

int vlan_read(uint16_t vid, bool *dynamic, struct genavb_vlan_port_map *map)
{
	return -1;
}

int vlan_dump(uint32_t *token, uint16_t *vid, bool *dynamic, struct genavb_vlan_port_map *map)
{
	return -1;
}

int vlan_port_set_default(unsigned int port_id, uint16_t vid)
{
	return -1;
}

int vlan_port_get_default(unsigned int port_id, uint16_t *vid)
{
	return -1;
}
