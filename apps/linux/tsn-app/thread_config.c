/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "../common/thread_config.h"

/* Slots are allocated by thread array index order */
thr_thread_t g_thread_array[MAX_THREADS] = {
	{
		.poll_fd = -1,
		.cpu_core = 2,
		.exit_flag = 0,
		.priority = 60,
		.max_slots = 1,
		.thread_capabilities = THR_CAP_TSN_LOOP,
		.slots = {{0}},
	},
	{
		.poll_fd = -1,
		.cpu_core = 2,
		.exit_flag = 0,
		.priority = 10,
		.max_slots = 1,
		.thread_capabilities = THR_CAP_TSN_PT,
		.slots = {{0}},
	},
	{
		.poll_fd = -1,
		.cpu_core = 2,
		.exit_flag = 0,
		.priority = 2,
		.max_slots = 1,
		.thread_capabilities = THR_CAP_STATS,
		.slots = {{0}},
	},
};
