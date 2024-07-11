/*
 * Copyright 2022-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief GenAVB public API
 \details 802.1Q VLAN registration definitions.
*/

#ifndef _OS_GENAVB_PUBLIC_VLAN_API_H_
#define _OS_GENAVB_PUBLIC_VLAN_API_H_

/** Updates VLAN filtering entry (or creates an entry if one doesn't exist).
 * \ingroup vlan
 *
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param       vid
 * \param       map
 */
int genavb_vlan_update(uint16_t vid, struct genavb_vlan_port_map *map);

/** Deletes VLAN filtering entry.
 * \ingroup vlan
 *
 * \return	    ::GENAVB_SUCCESS or negative error code.
 * \param       vid
 */
int genavb_vlan_delete(uint16_t vid);

/** Reads an entry from VLAN Filtering Data Base
 * \ingroup vlan
 *
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param       vid
 * \param       dynamic
 * \param       map
 */
int genavb_vlan_read(uint16_t vid, bool *dynamic, struct genavb_vlan_port_map *map);

/** Dumps the VLAN Filtering Data Base one entry at a time.
 * \ingroup vlan
 *
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param       token
 * \param       vid
 * \param       dynamic
 * \param       map
 */
int genavb_vlan_dump(uint32_t *token, uint16_t *vid, bool *dynamic, struct genavb_vlan_port_map *map);

/** Set port default VID (PVID)
 * \ingroup vlan
 *
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param       port_id
 * \param       vid
 */
int genavb_vlan_set_port_default(unsigned int port_id, uint16_t vid);

/** Get port default VID (PVID)
 * \ingroup vlan
 *
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param       port_id
 * \param       vid
 */
int genavb_vlan_get_port_default(unsigned int port_id, uint16_t *vid);

#endif /* _OS_GENAVB_PUBLIC_VLAN_API_H_ */
