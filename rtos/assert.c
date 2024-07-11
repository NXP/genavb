/*
 * Copyright 2018-2020, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief RTOS specific standard library implementation
 @details
*/

#include "rtos_abstraction_layer.h"

void os_assert(int expr) {
	rtos_assert(expr);
}
