/*
 * Copyright 2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file	clock.h
 @brief Generic clock functions
 @details
*/

#ifndef _COMMON_CLOCK_H_
#define _COMMON_CLOCK_H_

#include "os/clock.h"


int clock_set_time64(os_clock_id_t clk_id, u64 ns);

#endif /* _COMMON_CLOCK_H_ */
