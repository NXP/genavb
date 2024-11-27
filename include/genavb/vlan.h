/*
 * Copyright 2022-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief GenAVB/TSN public API
 \details 802.1Q FDB Static VLAN Registration definitions.

 \copyright Copyright 2022-2024 NXP
*/
#ifndef _GENAVB_PUBLIC_VLAN_H_
#define _GENAVB_PUBLIC_VLAN_H_

#include "genavb/types.h"

/** \cond RTOS */

/**
 * \defgroup vlan			802.1Q FDB Static VLAN Registration
 * \ingroup library
 *
 * This API is used to manage FDB Static VLAN Registration entries of a given network bridge (as defined in IEEE 802.1Q-2018 Sections 8.8.2 and 12.7.5.1).
 *
 */

/**
 * \ingroup vlan
 */
typedef enum {
	GENAVB_VLAN_ADMIN_CONTROL_FORBIDDEN = 0,
	GENAVB_VLAN_ADMIN_CONTROL_FIXED,
	GENAVB_VLAN_ADMIN_CONTROL_NORMAL,
	GENAVB_VLAN_ADMIN_CONTROL_REGISTERED,		/**< Internal only, reserved for MVRP */
	GENAVB_VLAN_ADMIN_CONTROL_NOT_REGISTERED,	/**< Internal only, reserved for MVRP */
} genavb_vlan_admin_control_t;

/**
 * \ingroup vlan
 * VLAN Registration port map
 * 802.1Q-2018 - 8.8.2
 */
struct genavb_vlan_port_map {
	unsigned int port_id;
	genavb_vlan_admin_control_t control;
	bool untagged;
};

/** \endcond */

/* OS specific headers */
#include "os/vlan.h"

#endif /* _GENAVB_PUBLIC_VLAN_H_ */
