/*
 * Copyright 2022-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief GenAVB public API
 \details 802.1Q VLAN registration definitions.

 \copyright Copyright 2022-2023 NXP
*/
#ifndef _GENAVB_PUBLIC_VLAN_H_
#define _GENAVB_PUBLIC_VLAN_H_

#include "genavb/types.h"

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

/* OS specific headers */
#include "os/vlan.h"

#endif /* _GENAVB_PUBLIC_VLAN_H_ */
