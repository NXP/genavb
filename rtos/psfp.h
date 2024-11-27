/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief RTOS PSFP functions
 @details
*/

#ifndef _RTOS_PSFP_H_
#define _RTOS_PSFP_H_

#include "os/clock.h"

void psfp_clock_discontinuity(os_clock_id_t clk_id);

#endif /* _RTOS_PSFP_H_ */
