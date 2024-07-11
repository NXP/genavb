/*
 * Copyright 2019-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file clock.c
 \brief GenAVB public API
 \details API definition for the GenAVB library

 \copyright Copyright 2019-2023 NXP
*/

#include "clock.h"

#include "genavb/error.h"

static const os_clock_id_t public_clock_to_os_clock[] = {
	[GENAVB_CLOCK_MONOTONIC] = OS_CLOCK_SYSTEM_MONOTONIC,
	[GENAVB_CLOCK_GPTP_0_0] = OS_CLOCK_GPTP_EP_0_0,
	[GENAVB_CLOCK_GPTP_0_1] = OS_CLOCK_GPTP_EP_0_1,
	[GENAVB_CLOCK_GPTP_1_0] = OS_CLOCK_GPTP_EP_1_0,
	[GENAVB_CLOCK_GPTP_1_1] = OS_CLOCK_GPTP_EP_1_1,
	[GENAVB_CLOCK_BR_0_0] = OS_CLOCK_GPTP_BR_0_0,
	[GENAVB_CLOCK_BR_0_1] = OS_CLOCK_GPTP_BR_0_1,
};

os_clock_id_t genavb_clock_to_os_clock(genavb_clock_id_t id)
{
	if (id >= GENAVB_CLOCK_MAX)
		return OS_CLOCK_MAX;

	return public_clock_to_os_clock[id];
}

int genavb_clock_gettime64(genavb_clock_id_t id, uint64_t *ns)
{
	if (id >= GENAVB_CLOCK_MAX || !ns)
		return -GENAVB_ERR_INVALID;

	if (os_clock_gettime64(public_clock_to_os_clock[id], ns) < 0)
		return -GENAVB_ERR_CLOCK;

	return GENAVB_SUCCESS;
}
