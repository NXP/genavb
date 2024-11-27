/*
 * Copyright 2020, 2022-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file		qos.c
 \brief	802.1Q QoS functions
 \details

 \copyright Copyright 2020, 2022-2023 NXP.
*/
#include "genavb/qos.h"

static const uint8_t default_priority_to_traffic_class[QOS_SR_CLASS_MAX + 1][QOS_TRAFFIC_CLASS_MAX][QOS_PRIORITY_MAX] = {

	/* 802.1Q Table 8-5, no SR Class */
	[0] = {
		[0] = {0, 0, 0, 0, 0, 0, 0, 0},
		[1] = {0, 0, 0, 0, 1, 1, 1, 1},
		[2] = {0, 0, 0, 0, 1, 1, 2, 2},
		[3] = {0, 0, 1, 1, 2, 2, 3, 3},
		[4] = {0, 0, 1, 1, 2, 2, 3, 4},
		[5] = {1, 0, 2, 2, 3, 3, 4, 5},
		[6] = {1, 0, 2, 3, 4, 4, 5, 6},
		[7] = {1, 0, 2, 3, 4, 5, 6, 7}
	},

	/* 802.1Q Table 34-2, one SR Class.
	 * Except that priorities 2 and 3 are swapped. If there is a single SR class enabled it is always mapped to SR Class A priority.
	 * This matches the Automotive Ethernet AVB Functional and Interoperability
	 * Specification 1.4 table 17 (and our sr_class.h definitions)
	 */
	[1] = {
		[0] = {0, 0, 0, 0, 0, 0, 0, 0}, /* Not supported, at least two traffic classes required */
		[1] = {0, 0, 0, 1, 0, 0, 0, 0},
		[2] = {0, 0, 0, 2, 1, 1, 1, 1},
		[3] = {0, 0, 0, 3, 1, 1, 2, 2},
		[4] = {0, 0, 1, 4, 2, 2, 3, 3},
		[5] = {0, 0, 1, 5, 2, 2, 3, 4},
		[6] = {1, 0, 2, 6, 3, 3, 4, 5},
		[7] = {1, 0, 2, 7, 3, 4, 5, 6}
	},

	/* 802.1Q Table 34-1, two SR Classes */
	[2] = {
		[0] = {0, 0, 0, 0, 0, 0, 0, 0}, /* Not supported, at least two traffic classes required */
		[1] = {0, 0, 1, 1, 0, 0, 0, 0},
		[2] = {0, 0, 1, 2, 0, 0, 0, 0},
		[3] = {0, 0, 2, 3, 1, 1, 1, 1},
		[4] = {0, 0, 3, 4, 1, 1, 2, 2},
		[5] = {0, 0, 4, 5, 1, 1, 2, 3},
		[6] = {0, 0, 5, 6, 1, 2, 3, 4},
		[7] = {1, 0, 6, 7, 2, 3, 4, 5}
	}
};

const uint8_t *priority_to_traffic_class_map(unsigned int enabled_tc, unsigned int enabled_sr_class)
{
	if (enabled_sr_class > QOS_SR_CLASS_MAX)
		return NULL;

	if ((enabled_tc < (QOS_TRAFFIC_CLASS_MIN + (enabled_sr_class ? 1 : 0))) || (enabled_tc > QOS_TRAFFIC_CLASS_MAX))
		return NULL;

	return default_priority_to_traffic_class[enabled_sr_class][enabled_tc - 1];
}
