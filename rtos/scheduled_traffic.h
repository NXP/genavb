/*
 * Copyright 2020-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief RTOS specific Scheduled Traffic implementation
 @details
*/

#ifndef _RTOS_SCHEDULED_TRAFFIC_H_
#define _RTOS_SCHEDULED_TRAFFIC_H_

#include "os/clock.h"

int st_clock_discontinuity(os_clock_id_t clk_id);

#endif /*_RTOS_SCHEDULED_TRAFFIC_H_ */
