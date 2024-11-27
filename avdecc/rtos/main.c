/*
 * Copyright 2018-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief NXP AVDECC RTOS specific code
 @details Setups RTOS task for NXP AVDECC stack component. Implements main loop and event handling.
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

#include "avdecc/avdecc_entry.h"
#include "avdecc/config.h"

#define AVDECC_TASK_NAME		"AVDECC Stack"
#define AVDECC_TASK_SUCCESS	(1 << 0)
#define AVDECC_TASK_ERROR		(1 << 1)

struct avdecc_ctx {
	rtos_event_group_t event_group;
	struct avdecc_config *avdecc_cfg;
	rtos_mqueue_t event_queue;
	uint8_t queue_buffer[AVDECC_CFG_EVENT_QUEUE_LENGTH * sizeof(struct event)];
	rtos_thread_t task;
	void *avdecc;
};

const struct avdecc_config avdecc_default_config = {
	.log_level = avdecc_CFG_LOG,
	.port_max = CFG_EP_DEFAULT_NUM_PORTS,
	.logical_port_list = CFG_EP_LOGICAL_PORT_LIST,
	.enabled = true,
	.srp_enabled = true,
#ifdef CONFIG_MANAGEMENT
	.management_enabled = true,
#else
	.management_enabled = false,
#endif
	.milan_mode = false,
	.use_gptp_bridge_stack = false,
	.num_entities = 1,
	.max_entities_discovery = CFG_ADP_DEFAULT_NUM_ENTITIES_DISCOVERY,
	.entity_cfg[0 ... CFG_AVDECC_NUM_ENTITIES - 1].valid_time = CFG_ADP_DEFAULT_VALID_TIME,
	.entity_cfg[0 ... CFG_AVDECC_NUM_ENTITIES - 1].max_listener_pairs = CFG_ACMP_DEFAULT_NUM_LISTENER_PAIRS,
	.entity_cfg[0 ... CFG_AVDECC_NUM_ENTITIES - 1].max_listener_streams = 3,
	.entity_cfg[0 ... CFG_AVDECC_NUM_ENTITIES - 1].max_talker_streams = 3,
	.entity_cfg[0 ... CFG_AVDECC_NUM_ENTITIES - 1].max_inflights = CFG_AVDECC_DEFAULT_NUM_INFLIGHTS,
	.entity_cfg[0 ... CFG_AVDECC_NUM_ENTITIES - 1].max_unsolicited_registrations = CFG_AECP_DEFAULT_NUM_UNSOLICITED,
	.entity_cfg[0 ... CFG_AVDECC_NUM_ENTITIES - 1].max_ptlv_entries = CFG_AEM_DEFAULT_NUM_PTLV_ENTRIES,
};


/**
 *
 */
__init static void clip_config_values(unsigned int *val, unsigned int min, unsigned int max)
{
	if (*val < min)
		*val = min;

	if (*val > max)
		*val = max;
}

__init static void process_avdecc_entity(struct avdecc_entity_config *entity_cfg)
{
	entity_cfg->flags &= (AVDECC_FAST_CONNECT_MODE | AVDECC_FAST_CONNECT_BTB);

	clip_config_values(&entity_cfg->talker_entity_id_n, 0, ACMP_CFG_MAX_UNIQUE_ID);
	clip_config_values(&entity_cfg->talker_unique_id_n, 0, ACMP_CFG_MAX_UNIQUE_ID);
	clip_config_values(&entity_cfg->listener_unique_id_n, 0, ACMP_CFG_MAX_UNIQUE_ID);
	clip_config_values(&entity_cfg->valid_time, CFG_ADP_MIN_VALID_TIME, CFG_ADP_MAX_VALID_TIME);
	clip_config_values(&entity_cfg->max_listener_streams, CFG_ACMP_MIN_NUM_LISTENER_STREAMS, CFG_ACMP_MAX_NUM_LISTENER_STREAMS);
	clip_config_values(&entity_cfg->max_talker_streams, CFG_ACMP_MIN_NUM_TALKER_STREAMS, CFG_ACMP_MAX_NUM_TALKER_STREAMS);
	clip_config_values(&entity_cfg->max_listener_pairs, CFG_ACMP_MIN_NUM_LISTENER_PAIRS, CFG_ACMP_MAX_NUM_LISTENER_PAIRS);
	clip_config_values(&entity_cfg->max_inflights, CFG_AVDECC_MIN_NUM_INFLIGHTS, CFG_AVDECC_MAX_NUM_INFLIGHTS);
	clip_config_values(&entity_cfg->max_unsolicited_registrations, CFG_AECP_MIN_NUM_UNSOLICITED, CFG_AECP_MAX_NUM_UNSOLICITED);
}

/**
 *
 */
__init static void process_config(struct avdecc_config *avdecc_cfg)
{
	int entity_index = 0;

	if (avdecc_cfg->num_entities > CFG_AVDECC_NUM_ENTITIES)
		avdecc_cfg->num_entities = CFG_AVDECC_NUM_ENTITIES;

	for (entity_index = 0; entity_index < avdecc_cfg->num_entities; entity_index++) {
		process_avdecc_entity(&avdecc_cfg->entity_cfg[entity_index]);
	}
}

/**
 *
 */
static void avdecc_task(void *pvParameters)
{
	struct avdecc_ctx *ctx = pvParameters;
	struct avdecc_config *avdecc_cfg = ctx->avdecc_cfg;
	void *avdecc = NULL;

	os_log(LOG_INIT, "avdecc task init\n");

	if (rtos_mqueue_init(&ctx->event_queue, AVDECC_CFG_EVENT_QUEUE_LENGTH, sizeof(struct event), ctx->queue_buffer) < 0) {
		os_log(LOG_ERR, "rtos_mqueue_init() failed\n");
		goto err_queue_create;
	}

	/**
	 * AVDECC Config
	 */
	process_config(avdecc_cfg);

	avdecc = avdecc_init(avdecc_cfg, (unsigned long)&ctx->event_queue);
	if (!avdecc)
		goto err_avdecc_init;

	ctx->avdecc = avdecc;

	rtos_event_group_set(&ctx->event_group, AVDECC_TASK_SUCCESS);

	os_log(LOG_INIT, "started\n");

	/**
	 * Main loop
	 */
	while (1) {
		struct event e;

		if (rtos_mqueue_receive(&ctx->event_queue, &e, RTOS_MS_TO_TICKS(10000)) < 0)
			continue;

		switch (e.type) {
		case EVENT_TYPE_NET_RX:
			net_rx((struct net_rx *) e.data);
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

err_avdecc_init:
	rtos_mqueue_destroy(&ctx->event_queue);

err_queue_create:
	os_log(LOG_INIT, "avdecc task exited\n");

	rtos_event_group_set(&ctx->event_group, AVDECC_TASK_ERROR);

	rtos_thread_abort(NULL);
}

/**
 *
 */
__init void *avdecc_task_init(struct avdecc_config *avdecc_cfg)
{
	struct avdecc_ctx *ctx;
	uint32_t bits;

	ctx = rtos_malloc(sizeof(struct avdecc_ctx));
	if (!ctx)
		goto err_ctx_alloc;

	if (rtos_event_group_init(&ctx->event_group) < 0)
		goto err_event_group;

	ctx->avdecc_cfg = avdecc_cfg;

	if (rtos_thread_create(&ctx->task, AVDECC_CFG_PRIORITY, 0, AVDECC_CFG_STACK_DEPTH, AVDECC_TASK_NAME, avdecc_task, ctx) < 0) {
		os_log(LOG_ERR, "rtos_thread_create(%s) failed\n", AVDECC_TASK_NAME);
		goto err_task_create;
	}

	bits = rtos_event_group_wait(&ctx->event_group, AVDECC_TASK_SUCCESS | AVDECC_TASK_ERROR, false, RTOS_WAIT_FOREVER);
	if (bits & AVDECC_TASK_ERROR)
		goto err_event_group;

	os_log(LOG_INIT, "avdecc main completed\n");

	return ctx;

err_event_group:
err_task_create:
	rtos_free(ctx);

err_ctx_alloc:
	return NULL;
}

__exit void avdecc_task_exit(void *handle)
{
	struct avdecc_ctx *ctx = handle;

	rtos_thread_abort(&ctx->task);

	avdecc_exit(ctx->avdecc);

	rtos_mqueue_destroy(&ctx->event_queue);

	rtos_free(ctx);
}
