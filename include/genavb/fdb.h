/*
 * Copyright 2022-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief GenAVB public API
 \details 802.1Q FDB definitions.

 \copyright Copyright 2022-2023 NXP
*/
#ifndef _GENAVB_PUBLIC_FDB_H_
#define _GENAVB_PUBLIC_FDB_H_

#include "genavb/types.h"

/**
 * \ingroup fdb
 */
typedef enum {
	GENAVB_FDB_PORT_CONTROL_FILTERING = 0,
	GENAVB_FDB_PORT_CONTROL_FORWARDING,
} genavb_fdb_port_control_t;

/**
 * \ingroup fdb
 */
typedef enum {
	GENAVB_FDB_STATUS_INVALID = 0,
	GENAVB_FDB_STATUS_OTHER,
	GENAVB_FDB_STATUS_LEARNED,
	GENAVB_FDB_STATUS_SELF,
	GENAVB_FDB_STATUS_MGMT
} genavb_fdb_status_t;

/**
 * \ingroup fdb
 * FDB port map
 * 802.1Q-2018 - 8.8.1
 */
struct genavb_fdb_port_map {
	unsigned int port_id;
	genavb_fdb_port_control_t control;
};

/* OS specific headers */
#include "os/fdb.h"

#endif /* _GENAVB_PUBLIC_FDB_H_ */
