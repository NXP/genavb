/*
 * Copyright 2019-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file clock.c
 \brief GenAVB public API
 \details API definition for the GenAVB library

 \copyright Copyright 2019-2024 NXP
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
	[GENAVB_CLOCK_LOCAL_BR_0] = OS_CLOCK_LOCAL_BR_0,
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

int genavb_clock_setoffset(genavb_clock_id_t id, int64_t offset)
{
	if (id >= GENAVB_CLOCK_MAX)
		return -GENAVB_ERR_INVALID;

	if (os_clock_setoffset(public_clock_to_os_clock[id], offset) < 0)
		return -GENAVB_ERR_CLOCK;

	return GENAVB_SUCCESS;
}

int genavb_clock_setfreq(genavb_clock_id_t id, int32_t ppb)
{
	if (id >= GENAVB_CLOCK_MAX)
		return -GENAVB_ERR_INVALID;

	if (os_clock_setfreq(public_clock_to_os_clock[id], ppb) < 0)
		return -GENAVB_ERR_CLOCK;

	return GENAVB_SUCCESS;
}

int genavb_clock_convert(genavb_clock_id_t clk_id_src, uint64_t ns_src, genavb_clock_id_t clk_id_dst, uint64_t *ns_dst)
{
	if (clk_id_src >= GENAVB_CLOCK_MAX)
		return -GENAVB_ERR_INVALID;
	if (clk_id_dst >= GENAVB_CLOCK_MAX)
		return -GENAVB_ERR_INVALID;

	if (os_clock_convert(public_clock_to_os_clock[clk_id_src], ns_src, public_clock_to_os_clock[clk_id_dst], ns_dst) < 0)
		return -GENAVB_ERR_CLOCK;

	return GENAVB_SUCCESS;
}
