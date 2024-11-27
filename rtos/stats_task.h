/*
 * Copyright 2019-2020, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief RTOS stats prinitng task
 @details
*/

#ifndef _RTOS_STATS_TASK_H_
#define _RTOS_STATS_TASK_H_

#include "rtos_abstraction_layer.h"

#define STATS_TASK_NAME			"genavb_stats"
#define STATS_TASK_STACK_DEPTH		(RTOS_MINIMAL_STACK_SIZE + 166)
#define STATS_TASK_PRIORITY		2
#define STATS_TASK_DELAY_MS		10000

struct stats_ctx {
	rtos_thread_t task;
};

extern struct stats_ctx stats_ctx;

int stats_task_init(void);
void stats_task_exit(void);

#endif /* _RTOS_STATS_TASK_H_ */
