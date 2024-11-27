/*
 * Copyright 2018-2019, 2021-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief SRP RTOS specific code
 @details Setups RTOS thread for SRP stack component. Implements SRP main loop and event handling.
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
#include "os/net.h"
#include "os/ipc.h"
#include "os/timer.h"

#include "srp/srp_entry.h"
#include "srp/config.h"

#define SRP_TASK_NAME		"SRP Stack"
#define SRP_TASK_SUCCESS	(1 << 0)
#define SRP_TASK_ERROR		(1 << 1)

struct srp_ctx {
	rtos_event_group_t event_group;
	struct srp_config *srp_cfg;
	rtos_mqueue_t event_queue;
	uint8_t queue_buffer[SRP_CFG_EVENT_QUEUE_LENGTH * sizeof(struct event)];
	rtos_thread_t task;
	void *srp;
	bool started;
};

const struct srp_config srp_default_config = {
	.log_level = srp_CFG_LOG,

	.is_bridge = 0,

	.port_max = CFG_EP_DEFAULT_NUM_PORTS,

	.logical_port_list = CFG_EP_LOGICAL_PORT_LIST,

#ifdef CONFIG_MANAGEMENT
	.management_enabled = 1,
#else
	.management_enabled = 0,
#endif
	.mrp_cfg = {
		.leave_timeout = CFG_MRP_LVTIMER_VAL_802_1Q_DEFAULT,
	},
	.msrp_cfg = {
		.is_bridge = 0,
		.flags = 0,
		.port_max = CFG_EP_DEFAULT_NUM_PORTS,
		.logical_port_list = CFG_EP_LOGICAL_PORT_LIST,
		.enabled = 1,
	},
	.mvrp_cfg = {
		.is_bridge = 0,
		.port_max = CFG_EP_DEFAULT_NUM_PORTS,
		.logical_port_list = CFG_EP_LOGICAL_PORT_LIST,
	},
};

__init static void process_section_msrp(struct msrp_config *cfg)
{
	cfg->flags = 0;
}

__init static void process_config(struct srp_config *cfg)
{
	process_section_msrp(&cfg->msrp_cfg);
}

static void srp_task(void *pvParameters)
{
	struct srp_ctx *ctx = pvParameters;
	struct srp_config *srp_cfg = ctx->srp_cfg;
	void *srp;

	os_log(LOG_INIT, "srp task started\n");

	/*
	 * Main event queue creation
	 */
	if (rtos_mqueue_init(&ctx->event_queue, SRP_CFG_EVENT_QUEUE_LENGTH, sizeof(struct event), ctx->queue_buffer) < 0) {
		os_log(LOG_ERR, "rtos_mqueue_init() failed\n");
		goto err_queue_create;
	}

	process_config(srp_cfg);

	srp = srp_init(srp_cfg, (unsigned long)&ctx->event_queue);
	if (!srp)
		goto err_srp_init;

	ctx->srp = srp;

	rtos_event_group_set(&ctx->event_group, SRP_TASK_SUCCESS);

	os_log(LOG_INIT, "started\n");

	/*
	 * Main event loop
	 */
	while (1) {
		struct event e;

		if (rtos_mqueue_receive(&ctx->event_queue, &e, RTOS_MS_TO_TICKS(10000)) < 0)
			continue;

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

err_srp_init:
	rtos_mqueue_destroy(&ctx->event_queue);

err_queue_create:
	os_log(LOG_INIT, "srp task exited\n");

	rtos_event_group_set(&ctx->event_group, SRP_TASK_ERROR);

	rtos_thread_abort(NULL);
}

__init void *srp_task_init(struct srp_config *srp_cfg)
{
	struct srp_ctx *ctx;
	uint32_t bits;

	ctx = rtos_malloc(sizeof(struct srp_ctx));
	if (!ctx)
		goto err_ctx_alloc;

	if (rtos_event_group_init(&ctx->event_group) < 0)
		goto err_event_group;

	ctx->srp_cfg = srp_cfg;

	if (rtos_thread_create(&ctx->task, SRP_CFG_PRIORITY, 0, SRP_CFG_STACK_DEPTH, SRP_TASK_NAME, srp_task, ctx) < 0) {
		os_log(LOG_ERR, "rtos_thread_create(%s) failed\n", SRP_TASK_NAME);
		goto err_task_create;
	}

	bits = rtos_event_group_wait(&ctx->event_group, SRP_TASK_SUCCESS | SRP_TASK_ERROR, false, RTOS_WAIT_FOREVER);
	if (bits & SRP_TASK_ERROR)
		goto err_event_group;

	os_log(LOG_INIT, "srp main completed\n");

	return ctx;

err_event_group:
err_task_create:
	rtos_free(ctx);

err_ctx_alloc:
	return NULL;
}

__exit void srp_task_exit(void *handle)
{
	struct srp_ctx *ctx = handle;

	rtos_thread_abort(&ctx->task);

	srp_exit(ctx->srp);

	rtos_mqueue_destroy(&ctx->event_queue);

	rtos_free(ctx);
}
