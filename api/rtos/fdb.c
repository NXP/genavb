/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "genavb/fdb.h"


int genavb_fdb_update(uint8_t *address, uint16_t vid, struct genavb_fdb_port_map *map)
{
	int rc;

	rc = fdb_update(map->port_id, address, vid, false, map->control);

	return rc;
}

int genavb_fdb_delete(uint8_t *address, uint16_t vid)
{
	int rc;

	rc = fdb_delete(address, vid, false);

	return rc;
}

int genavb_fdb_read(uint8_t *address, uint16_t vid, bool *dynamic, struct genavb_fdb_port_map *map, genavb_fdb_status_t *status)
{
	int rc;

	rc = fdb_read(address, vid, dynamic, map, status);

	return rc;
}

int genavb_fdb_dump(uint32_t *token, uint8_t *address, uint16_t *vid, bool *dynamic, struct genavb_fdb_port_map *map, genavb_fdb_status_t *status)
{
	int rc;

	rc = fdb_dump(token, address, vid, dynamic, map, status);

	return rc;
}
