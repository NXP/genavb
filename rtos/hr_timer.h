/*
 * Copyright 2019-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief RTOS specific high-resolution timers
 @details
*/

#ifndef _RTOS_HR_TIMER_H_
#define _RTOS_HR_TIMER_H_

#include "os/clock.h"

void hr_timer_task_stats(void);
void hr_timer_stats(void);

int hr_timer_start(void *handle, u64 value, u64 interval_p, u64 interval_q,
		   unsigned int flags);
void hr_timer_stop(void *handle);
void *hr_timer_create(os_clock_id_t clk_id, unsigned int flags,
				 void (*func)(void *data, int count), void *data);
void hr_timer_destroy(void *handle);

void hr_timer_clock_discontinuity(os_clock_id_t clk);

int hr_timer_init(void);
void hr_timer_exit(void);

#endif /* _RTOS_HR_TIMER_H_ */
