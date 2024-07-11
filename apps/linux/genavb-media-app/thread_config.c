/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "../common/thread_config.h"
#include "../common/gstreamer.h"

/* Slots are allocated by thread array index order */
thr_thread_t g_thread_array[MAX_THREADS] = {
	{
		.poll_fd = -1,
		.cpu_core = 2,
		.exit_flag = 0,
		.priority = 60,
		.max_slots = 1,
		.thread_capabilities = THR_CAP_STREAM_TALKER | THR_CAP_STREAM_LISTENER | THR_CAP_ALSA | THR_CAP_STREAM_AUDIO,
		.slots = {{0}},
	},
	{
		.poll_fd = -1,
		.cpu_core = 2,
		.exit_flag = 0,
		.priority = 60,
		.max_slots = 1,
		.thread_capabilities = THR_CAP_STREAM_TALKER | THR_CAP_STREAM_LISTENER | THR_CAP_ALSA | THR_CAP_STREAM_AUDIO,
		.slots = {{0}},
	},
	{
		.poll_fd = -1,
		.cpu_core = 2,
		.exit_flag = 0,
		.priority = 60,
		.max_slots = 1,
		.thread_capabilities = THR_CAP_STREAM_TALKER | THR_CAP_STREAM_LISTENER | THR_CAP_ALSA | THR_CAP_STREAM_AUDIO,
		.slots = {{0}},
	},
	{
		.poll_fd = -1,
		.cpu_core = 2,
		.exit_flag = 0,
		.priority = 60,
		.max_slots = 1,
		.thread_capabilities = THR_CAP_STREAM_TALKER | THR_CAP_STREAM_LISTENER | THR_CAP_ALSA | THR_CAP_STREAM_AUDIO,
		.slots = {{0}},
	},
	{
		.poll_fd = -1,
		.cpu_core = 2,
		.exit_flag = 0,
		.priority = 60,
		.max_slots = 4,
		.thread_capabilities = THR_CAP_GSTREAMER_SYNC | THR_CAP_STREAM_TALKER | THR_CAP_CONTROLLED | THR_CAP_TIMER,
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
	{
		.poll_fd = -1,
		.cpu_core = 2,
		.exit_flag = 0,
		.priority = 60,
		.max_slots = 4,
		.thread_capabilities = THR_CAP_GST_MULTI | THR_CAP_STREAM_TALKER | THR_CAP_TIMER,
		.slots = {{0}},
	},
	{
		.poll_fd = -1,
		.cpu_core = 2,
		.exit_flag = 0,
		.priority = GST_THREADS_PRIORITY,
		.max_slots = 1,
		.thread_capabilities = THR_CAP_GST_BUS_TIMER,
		.slots = {{0}},
	},
};
