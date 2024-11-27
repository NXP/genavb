/*
 * Copyright 2022-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief VLAN registration service OS abstraction
 @details
*/

#ifndef _OS_VLAN_H_
#define _OS_VLAN_H_

#include "os/sys_types.h"
#include "genavb/vlan.h"

#define NETC_INTERNAL_VID_BASE 3000

int vlan_update(uint16_t vid, bool dynamic, struct genavb_vlan_port_map *map);
int vlan_delete(uint16_t vid, bool dynamic);
int vlan_read(uint16_t vid, bool *dynamic, struct genavb_vlan_port_map *map);
int vlan_dump(uint32_t *token, uint16_t *vid, bool *dynamic, struct genavb_vlan_port_map *map);
int vlan_port_set_default(unsigned int port_id, uint16_t vid);
int vlan_port_get_default(unsigned int port_id, uint16_t *vid);

#endif /* _OS_VLAN_H_ */
