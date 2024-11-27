/*
 * Copyright 2020-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>

#include "../common/log.h"
#include "../common/thread.h"

#include "network_only.h"
#include "cyclic_task.h"

void network_only_stats_handler(void *data)
{
	struct network_only_ctx *ctx = data;
	struct cyclic_task *c_task = ctx->c_task;

	cyclic_stats_print(c_task);
}

static void null_loop(void *data, int timer_status)
{
	struct network_only_ctx *ctx = data;
	struct cyclic_task *c_task = ctx->c_task;
	uint64_t sched_time = tsn_task_get_time(c_task->task);
	uint64_t sched_now = tsn_task_get_now(c_task->task);

	if ((sched_now - sched_time) < SCHEDULE_LATENCY_THRESHOLD)
		cyclic_net_transmit(c_task, 0, NULL, 0);
}

struct network_only_ctx *network_only_init(unsigned int role, unsigned int period_ns, unsigned int num_peers, unsigned int timer_type)
{
	struct cyclic_task *c_task = NULL;
	struct network_only_ctx *ctx;

	ctx = malloc(sizeof(struct network_only_ctx));
	if (!ctx) {
		ERR("malloc() failed\n");
		goto err;
	}

	memset(ctx, 0, sizeof(*ctx));

	c_task = tsn_conf_get_cyclic_task(role);
	if (!c_task) {
		ERR("tsn_conf_get_cyclic_task() failed\n");
		goto err_free;
	}

	cyclic_task_set_period(c_task, period_ns);

#ifdef TRACE_SNAPSHOT
	cyclic_task_init_trace_snapshot(c_task);
#endif

	if (c_task->type == CYCLIC_CONTROLLER) {
		if (cyclic_task_set_num_peers(c_task, num_peers) < 0) {
			ERR("cyclic_task_set_period() failed\n");
			goto err_free;
		}
	}

	c_task->params.timer_type = timer_type;

	if (cyclic_task_init(c_task, NULL, null_loop, ctx) < 0) {
		ERR("cyclic_task_init() failed\n");
		goto err_free;
	}

	ctx->c_task = c_task;

	cyclic_task_start(c_task);

	return ctx;

err_free:
	free(ctx);
err:
	return NULL;
}

void network_only_exit(void *data)
{
	struct network_only_ctx *ctx = data;

	cyclic_task_exit(ctx->c_task);
	free(ctx);
}
