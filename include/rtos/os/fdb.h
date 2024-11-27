/*
 * Copyright 2022-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief GenAVB/TSN public API
 \details 802.1Q FDB definitions.
*/

#ifndef _OS_GENAVB_PUBLIC_FDB_API_H_
#define _OS_GENAVB_PUBLIC_FDB_API_H_

/** Adds port to existing Filtering Data Base entry (or creates an entry if one doesn't exist)
 * \ingroup fdb
 *
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param       address
 * \param       vid
 * \param       map
 */
int genavb_fdb_update(uint8_t *address, uint16_t vid, struct genavb_fdb_port_map *map);

/** Deletes port from existing entry. If no ports left, entry is removed.
 * \ingroup fdb
 *
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param       address
 * \param       vid
 */
int genavb_fdb_delete(uint8_t *address, uint16_t vid);


/** Reads an entry from the Filtering Data Base
 * \ingroup fdb
 *
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param       address
 * \param       vid
 * \param       dynamic
 * \param       map
 * \param       status
 */
int genavb_fdb_read(uint8_t *address, uint16_t vid, bool *dynamic, struct genavb_fdb_port_map *map, genavb_fdb_status_t *status);

/** Dumps the Filtering Data Base one entry at a time.
 * \ingroup fdb
 *
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param       token
 * \param       address
 * \param       vid
 * \param       dynamic
 * \param       map
 * \param       status
 */
int genavb_fdb_dump(uint32_t *token, uint8_t *address, uint16_t *vid, bool *dynamic, struct genavb_fdb_port_map *map, genavb_fdb_status_t *status);

#endif /* _OS_GENAVB_PUBLIC_FDB_API_H_ */
