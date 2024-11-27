/*
 * Copyright 2019-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Management RTOS specific code
 @details Setups RTOS thread for Management stack component. Implements Management main loop and event handling.
 */

#include <string.h>
#include <stdlib.h>

#include "rtos_abstraction_layer.h"

#include "common/ipc.h"
#include "common/log.h"

#include "os/config.h"
#include "os/sys_types.h"
#include "os/log.h"
#include "os/sys_types.h"
#include "os/ipc.h"
#include "os/timer.h"

#include "management/management_entry.h"
#include "management/config.h"

#define MANAGEMENT_TASK_NAME		"Management Stack"
#define MANAGEMENT_TASK_SUCCESS		(1 << 0)
#define MANAGEMENT_TASK_ERROR		(1 << 1)

struct management_ctx {
	rtos_event_group_t event_group;
	struct management_config *management_cfg;
	rtos_mqueue_t event_queue;
	uint8_t queue_buffer[MANAGEMENT_CFG_EVENT_QUEUE_LENGTH * sizeof(struct event)];
	rtos_thread_t task;
	void *management;
};

const struct management_config management_default_config = {
	.log_level = management_CFG_LOG,
	.is_bridge = 0,
	.port_max = CFG_EP_DEFAULT_NUM_PORTS,
	.logical_port_list = CFG_EP_LOGICAL_PORT_LIST,
};

static void management_task(void *pvParameters)
{
	struct management_ctx *ctx = pvParameters;
	struct management_config *management_cfg = ctx->management_cfg;
	void *management;

	os_log(LOG_INIT, "management task started\n");

	/*
	 * Main event queue creation
	 */
	if (rtos_mqueue_init(&ctx->event_queue, MANAGEMENT_CFG_EVENT_QUEUE_LENGTH, sizeof(struct event), ctx->queue_buffer) < 0) {
		os_log(LOG_ERR, "rtos_mqueue_init() failed\n");
		goto err_queue_create;
	}

	management = management_init(management_cfg, (unsigned long)&ctx->event_queue);
	if (!management)
		goto err_management_init;

	ctx->management = management;

	rtos_event_group_set(&ctx->event_group, MANAGEMENT_TASK_SUCCESS);

	os_log(LOG_INIT, "started\n");

	/*
	 * Main event loop
	 */
	while (1) {
		struct event e;

		if (rtos_mqueue_receive(&ctx->event_queue, &e, RTOS_MS_TO_TICKS(10000)) < 0)
			continue;

		switch (e.type) {
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

err_management_init:
	rtos_mqueue_destroy(&ctx->event_queue);

err_queue_create:
	os_log(LOG_INIT, "management task exited\n");

	rtos_event_group_set(&ctx->event_group, MANAGEMENT_TASK_ERROR);

	rtos_thread_abort(NULL);
}

__init void *management_task_init(struct management_config *management_cfg)
{
	struct management_ctx *ctx;
	uint32_t bits;

	ctx = rtos_malloc(sizeof(struct management_ctx));
	if (!ctx)
		goto err_ctx_alloc;

	if (rtos_event_group_init(&ctx->event_group) < 0)
		goto err_event_group;

	ctx->management_cfg = management_cfg;

	if (rtos_thread_create(&ctx->task, MANAGEMENT_CFG_PRIORITY, 0, MANAGEMENT_CFG_STACK_DEPTH, MANAGEMENT_TASK_NAME, management_task, ctx) < 0) {
		os_log(LOG_ERR, "rtos_thread_create(%s) failed\n", MANAGEMENT_TASK_NAME);
		goto err_task_create;
	}

	bits = rtos_event_group_wait(&ctx->event_group, MANAGEMENT_TASK_SUCCESS | MANAGEMENT_TASK_ERROR, false, RTOS_WAIT_FOREVER);
	if (bits & MANAGEMENT_TASK_ERROR)
		goto err_event_group;

	os_log(LOG_INIT, "management main completed\n");

	return ctx;

err_event_group:
err_task_create:
	rtos_free(ctx);

err_ctx_alloc:
	return NULL;
}

__exit void management_task_exit(void *handle)
{
	struct management_ctx *ctx = handle;

	rtos_thread_abort(&ctx->task);

	management_exit(ctx->management);

	rtos_mqueue_destroy(&ctx->event_queue);

	rtos_free(ctx);
}
