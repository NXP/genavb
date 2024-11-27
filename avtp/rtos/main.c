/*
 * Copyright 2018-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief NXP AVTP RTOS specific code
 @details Setups RTOS task for NXP AVDECC stack component. Implements main loop and event handling.
 */

#include <string.h>
#include <stdlib.h>

#include "rtos_abstraction_layer.h"

#include "common/ipc.h"
#include "common/log.h"
#include "common/types.h"

#include "os/config.h"
#include "os/sys_types.h"
#include "os/log.h"
#include "os/clock.h"
#include "os/sys_types.h"
#include "os/net.h"
#include "os/ipc.h"
#include "os/timer.h"

#include "avtp/avtp_entry.h"
#include "avtp/config.h"

#define AVTP_TASK_NAME		"AVTP Stack"
#define STATS_TASK_NAME		"Stats Task"
#define AVTP_TASK_SUCCESS	(1 << 0)
#define AVTP_TASK_ERROR		(1 << 1)
#define STATS_TASK_SUCCESS	(1 << 2)
#define STATS_TASK_ERROR	(1 << 3)

#define IPC_POOLING_PERIOD_NS	(10ULL * NSECS_PER_MS)
#define STATS_PERIOD_NS		(10ULL * NSECS_PER_SEC)

struct avtp_ctx {
	rtos_event_group_t event_group;
	struct avtp_config *avtp_cfg;
	rtos_mqueue_t event_queue;
	uint8_t queue_buffer[AVTP_CFG_EVENT_QUEUE_LENGTH * sizeof(struct event)];
	rtos_thread_t task;
	rtos_thread_t stats_task;
	rtos_mqueue_t stats_event_queue;
	uint8_t stats_queue_buffer[STATS_CFG_EVENT_QUEUE_LENGTH * sizeof(struct event)];
	struct ipc_rx stats_ipc_rx;
	void *avtp;
};

const struct avtp_config avtp_default_config = {
	.log_level = avtp_CFG_LOG,

	.port_max = CFG_EP_DEFAULT_NUM_PORTS,

	.logical_port_list = CFG_EP_LOGICAL_PORT_LIST,
};

/**
 *
 */
__init static void process_config(struct avtp_config *avtp_cfg)
{
}


/**
 *
 */
static void stats_task(void *pvParameters)
{
	struct avtp_ctx *ctx = pvParameters;

	os_log(LOG_INIT, "stats task init\n");

	if (rtos_mqueue_init(&ctx->stats_event_queue, STATS_CFG_EVENT_QUEUE_LENGTH, sizeof(struct event), ctx->stats_queue_buffer) < 0) {
		os_log(LOG_ERR, "rtos_mqueue_init() failed\n");
		goto err_queue_create;
	}

	if (ipc_rx_init(&ctx->stats_ipc_rx, IPC_AVTP_STATS, stats_ipc_rx, (unsigned long)&ctx->stats_event_queue) < 0)
		goto err_ipc_rx;

	rtos_event_group_set(&ctx->event_group, STATS_TASK_SUCCESS);

	os_log(LOG_INIT, "started\n");

	while (1) {
		struct event e;

		if (rtos_mqueue_receive(&ctx->stats_event_queue, &e, RTOS_MS_TO_TICKS(10000)) < 0)
			continue;

		switch (e.type) {
		case EVENT_TYPE_IPC:
			ipc_rx((struct ipc_rx *)e.data);
			break;
		default:
			os_log(LOG_ERR, "rtos_mqueue_receive(): invalid event type(%u)\n", e.type);
			break;
		}
	}

	/* Not reached */

err_ipc_rx:
	rtos_mqueue_destroy(&ctx->stats_event_queue);

err_queue_create:

	os_log(LOG_INIT, "stats task exited\n");

	rtos_event_group_set(&ctx->event_group, STATS_TASK_ERROR);

	rtos_thread_abort(NULL);
}


/**
 *
 */
static void avtp_task(void *pvParameters)
{
	struct avtp_ctx *ctx = pvParameters;
	struct avtp_config *avtp_cfg = ctx->avtp_cfg;
	struct avtp_ctx *avtp;
	uint64_t current_time = 0, sched_time = 0, ipc_time = 0, stats_time = 0;
	unsigned int i, events_count;
	struct process_stats stats;
	uint32_t bits;

	os_log(LOG_INIT, "avtp task init\n");

	if (rtos_mqueue_init(&ctx->event_queue, AVTP_CFG_EVENT_QUEUE_LENGTH, sizeof(struct event), ctx->queue_buffer) < 0) {
		os_log(LOG_ERR, "rtos_mqueue_init() failed\n");
		goto err_queue_create;
	}

	/**
	 * Create stats task
	 */
	if (rtos_thread_create(&ctx->stats_task, STATS_CFG_PRIORITY, 0, STATS_CFG_STACK_DEPTH, STATS_TASK_NAME, stats_task, ctx) < 0) {
		os_log(LOG_ERR, "rtos_thread_create(%s) failed\n", STATS_TASK_NAME);
		goto err_task_create;
	}

	bits = rtos_event_group_wait(&ctx->event_group, STATS_TASK_SUCCESS | STATS_TASK_ERROR, false, RTOS_WAIT_FOREVER);
	if (bits & STATS_TASK_ERROR)
		goto err_task_create;

	/**
	 * AVTP Config
	 */
	process_config(avtp_cfg);

	for (i = 0; i < CFG_MAX_NUM_PORT; i++)
		avtp_cfg->clock_gptp_list[i] = logical_port_to_gptp_clock(avtp_cfg->logical_port_list[i], CFG_DEFAULT_GPTP_DOMAIN);

	/**
	 * AVTP Init
	 */
	avtp = avtp_init(avtp_cfg, (unsigned long)&ctx->event_queue);
	if (!avtp)
		goto err_avtp_init;

	ctx->avtp = avtp;

	rtos_event_group_set(&ctx->event_group, AVTP_TASK_SUCCESS);

	os_log(LOG_INIT, "started\n");

	stats_init(&stats.events, 31, NULL, NULL);
	stats_init(&stats.sched_intvl, 31, NULL, NULL);
	stats_init(&stats.processing_time, 31, NULL, NULL);

	/**
	 * Main loop
	 */
	while (1) {
		struct event e;
		int rc;

		events_count = 0;

		rc = rtos_mqueue_receive(&ctx->event_queue, &e, RTOS_MS_TO_TICKS(2));

		if (os_clock_gettime64(OS_CLOCK_SYSTEM_MONOTONIC, &current_time) == 0) {
			stats_update(&stats.sched_intvl, current_time - sched_time);
			sched_time = current_time;
		}

		if (rc != 0)
			goto timeout;

		do {
			events_count++;

			switch (e.type) {
			case EVENT_TYPE_NET_RX:
				net_rx_multi((struct net_rx *)e.data);
				break;

			case EVENT_TYPE_TIMER:
				os_timer_process((struct os_timer *)e.data);
				break;

			case EVENT_TYPE_MEDIA:
				avtp_media_event(e.data);
				break;

			default:
				os_log(LOG_ERR, "rtos_mqueue_receive(): invalid event type(%u)\n", e.type);
				break;
			}

		} while (events_count < 8 && !rtos_mqueue_receive(&ctx->event_queue, &e, RTOS_NO_WAIT));

timeout:
		stats_update(&stats.events, events_count);

		if (os_clock_gettime64(OS_CLOCK_SYSTEM_MONOTONIC, &current_time) == 0) {
			if ((current_time - ipc_time) > IPC_POOLING_PERIOD_NS) {
				avtp_ipc_rx(avtp);
				avtp_stream_free(avtp, current_time);
				ipc_time = current_time;

				if ((current_time - stats_time) > STATS_PERIOD_NS) {
					avtp_stats_dump(avtp, &stats);
					stats_time = current_time;
				}
			}

			stats_update(&stats.processing_time, current_time - sched_time);
		}
	}

	/* Not reached */

err_avtp_init:
	rtos_thread_abort(&ctx->stats_task);

	ipc_rx_exit(&ctx->stats_ipc_rx);

	rtos_mqueue_destroy(&ctx->stats_event_queue);

err_task_create:
	rtos_mqueue_destroy(&ctx->event_queue);

err_queue_create:

	os_log(LOG_INIT, "avtp task exited\n");

	rtos_event_group_set(&ctx->event_group, AVTP_TASK_ERROR);

	rtos_thread_abort(NULL);
}

/**
 *
 */
__init void *avtp_task_init(struct avtp_config *avtp_cfg)
{
	struct avtp_ctx *ctx;
	uint32_t bits;

	ctx = rtos_malloc(sizeof(struct avtp_ctx));
	if (!ctx)
		goto err_ctx_alloc;

	if (rtos_event_group_init(&ctx->event_group) < 0)
		goto err_event_group;

	ctx->avtp_cfg = avtp_cfg;

	if (rtos_thread_create(&ctx->task, AVTP_CFG_PRIORITY, 0, AVTP_CFG_STACK_DEPTH, AVTP_TASK_NAME, avtp_task, ctx) < 0) {
		os_log(LOG_ERR, "rtos_thread_create(%s) failed\n", AVTP_TASK_NAME);
		goto err_task_create;
	}

	bits = rtos_event_group_wait(&ctx->event_group, AVTP_TASK_SUCCESS | AVTP_TASK_ERROR, false, RTOS_WAIT_FOREVER);
	if (bits & AVTP_TASK_ERROR)
		goto err_event_group;

	os_log(LOG_INIT, "avtp main completed\n");

	return ctx;

err_event_group:
err_task_create:
	rtos_free(ctx);

err_ctx_alloc:
	return NULL;
}

__exit void avtp_task_exit(void *handle)
{
	struct avtp_ctx *ctx = handle;

	rtos_thread_abort(&ctx->task);

	avtp_exit(ctx->avtp);

	rtos_thread_abort(&ctx->stats_task);

	ipc_rx_exit(&ctx->stats_ipc_rx);

	rtos_mqueue_destroy(&ctx->event_queue);

	rtos_mqueue_destroy(&ctx->stats_event_queue);

	rtos_free(ctx);
}
