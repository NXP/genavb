/*
* Copyright 2020-2021, 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 * @file	mvrp_map.c
 * @brief	MVRP MAP implementation.
 * @details
 */

#include "mvrp_map.h"

/** Get the MVRP MAP context associated to the VLAN ID
* \return	MVRP MAP context (mvrp_map) on success, NULL on failure
* \param mvrp	MVRP global context structure
* \param vid	VLAN ID value
*/
struct mvrp_map *mvrp_get_map_context(struct mvrp_ctx *mvrp, unsigned short vid)
{
	/* 802.1Q, section 11.2.3.1.1. Single spanning tree and map context. */
	return &mvrp->map[0];
}

/** Init the MVRP MAP context
* \param mvrp	MVRP global context structure
*/
void mvrp_map_init(struct mvrp_ctx *mvrp)
{
	int i;

	for (i = 0; i < MVRP_MAX_MAP_CONTEXT; i++) {
		list_head_init(&mvrp->map[i].vlans);
		mvrp->map[i].num_vlans = 0;
		mvrp->map[i].forwarding_state = 0;
		mvrp->map[i].map_id = i;
	}

	os_log(LOG_INIT, "done\n");
}

/** Erase the MVRP MAP context
* \param mvrp	MVRP global context structure
*/
void mvrp_map_exit(struct mvrp_ctx *mvrp)
{
	struct list_head *entry, *next;
	struct mvrp_vlan *vlan;
	struct mvrp_map *map;
	int i, j;

	for (i = 0; i < MVRP_MAX_MAP_CONTEXT; i++) {
		map = &mvrp->map[i];

		for (entry = list_first(&map->vlans); next = list_next(entry), entry != &map->vlans; entry = next) {
			vlan = container_of(entry, struct mvrp_vlan, list);

			for (j = 0; j < mvrp->port_max; j++) {
				if (is_vlan_declared(vlan, j)) {
					mrp_mad_leave_request(&mvrp->port[j].mrp_app, MVRP_ATTR_TYPE_VID, (u8 *)&vlan->vlan_id);
					vlan_declared_clear(vlan, j);
				}
			}

			mvrp_free_vlan(map, vlan);
		}
	}

	os_log(LOG_INIT, "done\n");
}

void vlan_user_declaration_clear(struct mvrp_vlan *vlan, unsigned int port_id)
{
	vlan->user_declared &= ~(1 << port_id);
}

void vlan_user_declaration_set(struct mvrp_vlan *vlan, unsigned int port_id)
{
	vlan->user_declared |= (1 << port_id);
}

void vlan_registered_clear(struct mvrp_vlan *vlan, unsigned int port_id)
{
	vlan->registered_state &= ~(1 << port_id);
}

void vlan_registered_set(struct mvrp_vlan *vlan, unsigned int port_id)
{
	vlan->registered_state |= (1 << port_id);
}

void vlan_declared_clear(struct mvrp_vlan *vlan, unsigned int port_id)
{
	vlan->declared_state &= ~(1 << port_id);
}

void vlan_declared_set(struct mvrp_vlan *vlan, unsigned int port_id)
{
	vlan->declared_state |= (1 << port_id);
}

/** Update the MVRP VLAN attribute status for the given VLAN instance
 * \param vlan	MVRP VLAN instance pointer
 * \param port	MVRP Port structure
 * \param new	specify if the vlan registration is signaled for the first time
 */
void mvrp_map_update_vlan(struct mvrp_map *map, struct mvrp_vlan *vlan, bool new)
{
	struct mvrp_ctx *mvrp = container_of(map, struct mvrp_ctx, map[map->map_id]);
	int i;

	for (i = 0; i < mvrp->port_max; i++) {
		/* The port is not in the forwarding set, so per 802.1Q 10.3, this port transmits a leave message for every attribute
		that it has declared */
		if (!is_mvrp_port_forwarding(map, i)) {
			if (is_vlan_declared(vlan, i)) {
				mrp_mad_leave_request(&mvrp->port[i].mrp_app, MVRP_ATTR_TYPE_VID, (u8 *)&vlan->vlan_id);
				vlan_declared_clear(vlan, i);
			}

			continue;
		}

		/* If the port is forwarding and other ports in the forwarding set have registered this vlan, the attribute is declared */
		if (is_vlan_registered_other_forwarding(map, vlan, i) || is_vlan_user_declared(vlan, i)) {
			if (!is_vlan_declared(vlan, i)) {
				if (mrp_mad_join_request(&mvrp->port[i].mrp_app, MVRP_ATTR_TYPE_VID, (u8 *)&vlan->vlan_id, new))
					vlan_declared_set(vlan, i);
			}

		} else {
			/* none of the others ports have that have registered this vlan are in the forwarding set a leave message is transmitted */
			if (is_vlan_declared(vlan, i)) {
				mrp_mad_leave_request(&mvrp->port[i].mrp_app, MVRP_ATTR_TYPE_VID, (u8 *)&vlan->vlan_id);
				vlan_declared_clear(vlan, i);
			}
		}
	}

	os_log(LOG_INFO, "vlan_id(%u) map(%p) registered(0x%04x) declared(0x%04x)\n", ntohs(vlan->vlan_id), map, (vlan->registered_state & map->forwarding_state), vlan->declared_state);

	/* if vlan is not registered anywhere or declared by the user */
	if (!is_vlan_registered_any(vlan) && !is_vlan_user_declared_any(vlan))
		mvrp_free_vlan(map, vlan);
}

/** Update all MVRP VLAN attribute for the given MAP context
* \param map	MVRP MAP context structure
* \param port	MVRP Port structure
*/
void mvrp_map_update(struct mvrp_map *map)
{
	struct list_head *entry, *next;
	struct mvrp_vlan *vlan;

	/*For each VLAN instance*/
	for (entry = list_first(&map->vlans); next = list_next(entry), entry != &map->vlans; entry = next) {
		vlan = container_of(entry, struct mvrp_vlan, list);

		mvrp_map_update_vlan(map, vlan, 0);
	}
}
