/*
 * Copyright 2019-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief RTOS stats prinitng task
 @details
*/

#include "stats_task.h"

#include "common/log.h"
#include "debug_print.h"

struct stats_ctx stats_ctx;

static void stats_task(void *pvParameters)
{
	while (1) {
		rtos_sleep(RTOS_MS_TO_TICKS(STATS_TASK_DELAY_MS));
		hr_timer_show_stats();
		mclock_show_stats();
		net_port_qos_show_stats();
		net_port_show_stats();
    }
}

__init int stats_task_init(void)
{
	if (rtos_thread_create(&stats_ctx.task, STATS_TASK_PRIORITY, 0, STATS_TASK_STACK_DEPTH, STATS_TASK_NAME, stats_task, &stats_ctx) < 0) {
		os_log(LOG_ERR, "rtos_thread_create(%s) failed\n", STATS_TASK_NAME);
		goto err;
	}

	return 0;

err:
	return -1;
}

__exit void stats_task_exit(void)
{
	rtos_thread_abort(&stats_ctx.task);
}
