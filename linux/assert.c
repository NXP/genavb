/*
 * Copyright 2017, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Linux specific standard library implementation
 @details
*/

#include <assert.h>
#include "os/assert.h"

void os_assert(int expr)
{
	assert(expr);
}
