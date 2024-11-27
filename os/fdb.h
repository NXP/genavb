/*
 * Copyright 2020-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief FDB OS abstraction
 @details
*/

#ifndef _OS_FDB_H_
#define _OS_FDB_H_

#include "os/sys_types.h"
#include "genavb/fdb.h"

/* 802.1Q-2018, section 8.8.7 */
int fdb_update(unsigned int port_id, uint8_t *address, uint16_t vid, bool dynamic, genavb_fdb_port_control_t control);
int fdb_delete(uint8_t *address, uint16_t vid, bool dynamic);
int fdb_read(uint8_t *address, uint16_t vid, bool *dynamic, struct genavb_fdb_port_map *map, genavb_fdb_status_t *status);
int fdb_dump(uint32_t *token, uint8_t *address, uint16_t *vid, bool *dynamic, struct genavb_fdb_port_map *map, genavb_fdb_status_t *status);

#endif /* _OS_FDB_H_ */
