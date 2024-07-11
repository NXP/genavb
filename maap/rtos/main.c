/*
 * Copyright 2022-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief NXP MAAP RTOS specific code
 @details Setups RTOS task for NXP MAAP stack component. Implements main loop and event handling.
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

#include "maap/maap_entry.h"
#include "maap/config.h"

#define MAAP_TASK_NAME		"MAAP Stack"
#define MAAP_TASK_SUCCESS	(1 << 0)
#define MAAP_TASK_ERROR		(1 << 1)

struct maap_ctx {
	rtos_event_group_t event_group;
	struct maap_config *maap_cfg;
	rtos_mqueue_t event_queue;
	uint8_t queue_buffer[MAAP_CFG_EVENT_QUEUE_LENGTH * sizeof(struct event)];
	rtos_thread_t task;
	void *maap;
};

const struct maap_config maap_default_config = {
	.log_level = maap_CFG_LOG,

	.port_max = CFG_EP_DEFAULT_NUM_PORTS,
#ifdef CONFIG_MANAGEMENT
	.management_enabled = true,
#else
	.management_enabled = false,
#endif

	.logical_port_list = CFG_EP_LOGICAL_PORT_LIST,
};

/**
 *
 */
__init static void process_config(struct maap_config *maap_cfg)
{
}

/**
 *
 */
static void maap_task(void *pvParameters)
{
	struct maap_ctx *ctx = pvParameters;
	struct maap_config *maap_cfg = ctx->maap_cfg;
	struct maap_ctx *maap;

	os_log(LOG_INIT, "maap task init\n");

	if (rtos_mqueue_init(&ctx->event_queue, MAAP_CFG_EVENT_QUEUE_LENGTH, sizeof(struct event), ctx->queue_buffer) < 0) {
		os_log(LOG_ERR, "rtos_mqueue_init() failed\n");
		goto err_queue_create;
	}

	/**
	 * MAAP Config
	 */
	process_config(maap_cfg);

	/**
	 * MAAP Init
	 */
	maap = maap_init(maap_cfg, (unsigned long)&ctx->event_queue);
	if (!maap)
		goto err_maap_init;

	ctx->maap = maap;

	rtos_event_group_set(&ctx->event_group, MAAP_TASK_SUCCESS);

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

		case EVENT_TYPE_NET_TX_TS:
			net_tx_ts_process((struct net_tx *)e.data);
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

err_maap_init:
	rtos_mqueue_destroy(&ctx->event_queue);

err_queue_create:
	os_log(LOG_INIT, "maap task exited\n");

	rtos_event_group_set(&ctx->event_group, MAAP_TASK_ERROR);

	rtos_thread_abort(NULL);
}

__init void *maap_task_init(struct maap_config *maap_cfg)
{
	struct maap_ctx *ctx;
	uint32_t bits;

	ctx = rtos_malloc(sizeof(struct maap_ctx));
	if (!ctx)
		goto err_ctx_alloc;

	if (rtos_event_group_init(&ctx->event_group) < 0)
		goto err_event_group;

	ctx->maap_cfg = maap_cfg;

	if (rtos_thread_create(&ctx->task, MAAP_CFG_PRIORITY, 0, MAAP_CFG_STACK_DEPTH, MAAP_TASK_NAME, maap_task, ctx) < 0) {
		os_log(LOG_ERR, "rtos_thread_create(%s) failed\n", MAAP_TASK_NAME);
		goto err_task_create;
	}

	bits = rtos_event_group_wait(&ctx->event_group, MAAP_TASK_SUCCESS | MAAP_TASK_ERROR, false, RTOS_WAIT_FOREVER);
	if (bits & MAAP_TASK_ERROR)
		goto err_event_group;

	os_log(LOG_INIT, "maap main completed\n");

	return ctx;

err_event_group:
err_task_create:
	rtos_free(ctx);

err_ctx_alloc:
	return NULL;
}

__exit void maap_task_exit(void *handle)
{
	struct maap_ctx *ctx = handle;

	rtos_thread_abort(&ctx->task);

	maap_exit(ctx->maap);

	rtos_mqueue_destroy(&ctx->event_queue);

	rtos_free(ctx);
}
