/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief NXP HSR RTOS specific code
 @details Setups RTOS task for NXP HSR stack component. Implements main loop and event handling.
 */

#include <string.h>
#include <stdlib.h>

#include "rtos_abstraction_layer.h"

#include "common/ipc.h"
#include "common/log.h"

#include "os/sys_types.h"
#include "os/config.h"
#include "os/timer.h"
#include "os/log.h"
#include "os/net.h"

#include "hsr/hsr.h"

#define HSR_TASK_NAME		"HSR Stack"
#define HSR_TASK_SUCCESS	(1 << 0)
#define HSR_TASK_ERROR		(1 << 1)

struct hsr_ctx {
	rtos_event_group_t event_group;
	rtos_mqueue_t event_queue;
	uint8_t queue_buffer[HSR_CFG_EVENT_QUEUE_LENGTH * sizeof(struct event)];
	rtos_thread_t task;
	struct hsr_config *hsr_cfg;
	void *hsr;
	bool started;
};

const struct hsr_config hsr_default_config = {
	.hsr_enabled = 0,
	.port_max = 7,
	.hsr_port[0] = {
		.logical_port = 0,
		.type = HSR_UNUSED_PORT,
	},
	.hsr_port[1] = {
		.logical_port = 1,
		.type = HSR_HOST_PORT,
	},
	.hsr_port[2] = {
		.logical_port = 2,
		.type = HSR_RING_PORT,
	},
	.hsr_port[3] = {
		.logical_port = 3,
		.type = HSR_RING_PORT,
	},
	.hsr_port[4] = {
		.logical_port = 4,
		.type = HSR_EXTERNAL_PORT,
	},
	.hsr_port[5] = {
		.logical_port = 5,
		.type = HSR_EXTERNAL_PORT,
	},
	.hsr_port[6] = {
		.logical_port = 6,
		.type = HSR_INTERNAL_PORT,
	},
};

static void hsr_task(void *pvParameters)
{
	struct hsr_ctx *ctx = pvParameters;
	void *hsr;

	os_log(LOG_INIT, "HSR task started\n");

	if (rtos_mqueue_init(&ctx->event_queue, HSR_CFG_EVENT_QUEUE_LENGTH, sizeof(struct event), ctx->queue_buffer) < 0) {
		os_log(LOG_ERR, "rtos_mqueue_init() failed\n");
		goto err_queue_create;
	}

	/*
	* Intialize hsr stack and apply configuration
	*/
	if (ctx->hsr_cfg->hsr_enabled) {
		hsr = hsr_init(ctx->hsr_cfg, (unsigned long)&ctx->event_queue);
		if (!hsr)
			goto err_hsr_init;
	} else {
		hsr = NULL;
	}

	ctx->hsr = hsr;

	rtos_event_group_set(&ctx->event_group, HSR_TASK_SUCCESS);

	os_log(LOG_INIT, "started\n");

	/*
	* Main event loop
	*/
	while (1) {
		struct event e;

		if (rtos_mqueue_receive(&ctx->event_queue, &e, RTOS_MS_TO_TICKS(10000)) < 0) {
			continue;
		}

		switch (e.type) {
		case EVENT_TYPE_NET_RX:
			net_rx((struct net_rx *)e.data);
			break;

		case EVENT_TYPE_TIMER:
			os_timer_process((struct os_timer *)e.data);
			break;

		case EVENT_TYPE_IPC:
			ipc_rx((struct ipc_rx *)e.data);
			break;

		default:
			os_log(LOG_ERR, "rtos_mqueue_receive(): invalid event type(%u)\n", e.type);
			break;
		}
	}

	/* Not reached */

err_hsr_init:
	rtos_mqueue_destroy(&ctx->event_queue);

err_queue_create:
	os_log(LOG_INIT, "hsr task exited\n");

	rtos_event_group_set(&ctx->event_group, HSR_TASK_ERROR);

	rtos_thread_abort(NULL);
}

__init void *hsr_task_init(struct hsr_config *hsr_cfg)
{
	struct hsr_ctx *ctx;
	uint32_t bits;

	ctx = rtos_malloc(sizeof(struct hsr_ctx));
	if (!ctx)
		goto err_ctx_alloc;

	memset(ctx, 0, sizeof(struct hsr_ctx));

	if (rtos_event_group_init(&ctx->event_group) < 0)
		goto err_event_group;

	ctx->hsr_cfg = hsr_cfg;

	if (rtos_thread_create(&ctx->task, HSR_CFG_PRIORITY, 0, HSR_CFG_STACK_DEPTH, HSR_TASK_NAME, hsr_task, ctx) < 0) {
		os_log(LOG_ERR, "rtos_thread_create(%s) failed\n", HSR_TASK_NAME);
		goto err_task_create;
	}

	bits = rtos_event_group_wait(&ctx->event_group, HSR_TASK_SUCCESS | HSR_TASK_ERROR, false, RTOS_WAIT_FOREVER);
	if (bits & HSR_TASK_ERROR)
		goto err_event_group;

	os_log(LOG_INIT, "hsr main completed\n");

	return ctx;

err_event_group:
err_task_create:
	rtos_free(ctx);

err_ctx_alloc:
	return NULL;
}

__exit void hsr_task_exit(void *handle)
{
	struct hsr_ctx *ctx = handle;

	rtos_thread_abort(&ctx->task);

	hsr_exit(ctx->hsr);

	rtos_mqueue_destroy(&ctx->event_queue);

	rtos_free(ctx);
}
