/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2018-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file	mvrp_map.c
 * @brief	MVRP MAP interface.
 * @details
 */

#ifndef _MVRP_MAP_H_
#define _MVRP_MAP_H_

#include "srp.h"

static inline int is_vlan_declared(struct mvrp_vlan *vlan, unsigned int port_id)
{
	return (vlan->declared_state & (1 << port_id));
}

static inline int is_vlan_registered(struct mvrp_vlan *vlan, unsigned int port_id)
{
	return (vlan->registered_state & (1 << port_id));
}

static inline int is_vlan_registered_other(struct mvrp_vlan *vlan, unsigned int port_id)
{
	return (vlan->registered_state & ~(1 << port_id));
}

static inline int is_vlan_registered_any(struct mvrp_vlan *vlan)
{
	return (vlan->registered_state != 0);
}

static inline int is_vlan_user_declared(struct mvrp_vlan *vlan, unsigned int port_id)
{
	return (vlan->user_declared & (1 << port_id));
}

static inline int is_vlan_user_declared_any(struct mvrp_vlan *vlan)
{
	return (vlan->user_declared != 0);
}

static inline int is_vlan_registered_other_forwarding(struct mvrp_map *map, struct mvrp_vlan *vlan, unsigned int port_id)
{
	return ((vlan->registered_state & map->forwarding_state) & ~(1 << port_id));
}

static inline unsigned int is_mvrp_port_forwarding(struct mvrp_map *map, unsigned int port_id)
{
	return (map->forwarding_state & (1 << port_id));
}

struct mvrp_map *mvrp_get_map_context(struct mvrp_ctx *mvrp, unsigned short vid);
void mvrp_map_init(struct mvrp_ctx *mvrp);
void mvrp_map_exit(struct mvrp_ctx *mvrp);
void vlan_user_declaration_clear(struct mvrp_vlan *vlan, unsigned int port_id);
void vlan_user_declaration_set(struct mvrp_vlan *vlan, unsigned int port_id);
void vlan_registered_clear(struct mvrp_vlan *vlan, unsigned int port_id);
void vlan_registered_set(struct mvrp_vlan *vlan, unsigned int port_id);
void vlan_declared_clear(struct mvrp_vlan *vlan, unsigned int port_id);
void vlan_declared_set(struct mvrp_vlan *vlan, unsigned int port_id);
void mvrp_map_update_vlan(struct mvrp_map *map, struct mvrp_vlan *vlan, bool new);
void mvrp_map_update(struct mvrp_map *map);

#endif /* _MVRP_MAP_H_ */
