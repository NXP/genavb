/*
 * Copyright 2022-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief GenAVB/TSN public API
 \details 802.1Q FDB Static Filtering definitions.

 \copyright Copyright 2022-2024 NXP
*/
#ifndef _GENAVB_PUBLIC_FDB_H_
#define _GENAVB_PUBLIC_FDB_H_

#include "genavb/types.h"

/** \cond RTOS */

/**
 * \defgroup fdb		802.1Q FDB Static Filtering
 * \ingroup library
 *
 * This API is used to manage FDB Static Filtering entries of a given network bridge (as defined in IEEE 802.1Q-2018 Section 8.8.1 and 12.7.2).
 *
 */

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

/** \endcond */

/* OS specific headers */
#include "os/fdb.h"

#endif /* _GENAVB_PUBLIC_FDB_H_ */
