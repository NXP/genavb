/*
 * Copyright 2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Generic Clock functions implementation
 @details
*/

#include "common/clock.h"

int clock_set_time64(os_clock_id_t clk_id, u64 ns)
{
	u64 curr;
	s64 off;
	int rc;

	rc = os_clock_gettime64(clk_id, &curr);
	if (rc)
		goto exit;

	off = ns - curr;

	rc = os_clock_setoffset(clk_id, off);
exit:
	return rc;
}
