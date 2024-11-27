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
#include "../common/time.h"
#include "../common/timer.h"

#include "serial_controller.h"
#include "cyclic_task.h"

#define STAT_PERIOD_SEC 5

void serial_controller_stats_print(struct serial_controller_ctx *ctx)
{
	struct serial_controller_stats *stats_snap = &ctx->stats_snap;

	if (stats_snap->pending) {
		INF("net:");
		INF("    rx cmds          : %u", stats_snap->net_cmds);
		INF("    rx null cmds     : %u", stats_snap->net_null_cmds);
		INF("pt:");
		INF("    read bytes       : %u", stats_snap->pt_read_bytes);
		INF("    read cmds        : %u", stats_snap->pt_read_cmds);
		INF("    read aborts      : %u", stats_snap->pt_read_abort_cmds);
		INF("    write errors     : %u", stats_snap->pt_write_errors);
		INF("    write incomplete : %u", stats_snap->pt_write_incomplete);

		stats_snap->pending = false;
	}
}

static void serial_controller_stats_dump(struct serial_controller_ctx *ctx)
{
	if (ctx->stats_snap.pending)
		return;

	memcpy(&ctx->stats_snap, &ctx->stats, sizeof(struct serial_controller_stats));
	ctx->stats_snap.pending = true;
}

void serial_controller_stats_handler(void *data)
{
	struct serial_controller_ctx *ctx = data;
	struct cyclic_task *c_task = ctx->c_task;

	cyclic_stats_print(c_task);
	serial_controller_stats_print(ctx);
}

static int serial_controller_pt_handler(void *data, unsigned int events)
{
	struct serial_controller_ctx *ctx = data;
	struct pt_cmd *cmd = &ctx->pt_cmd;
	ssize_t s;

	/* If the other end hanged-up relax handling
	 * as epoll will trigger continuously until it resumes.
	 */
	if (events & EPOLLHUP) {
		sleep_ms(10);
		return 0;
	}

	/* Shift possible remaining data after the previous command */
	if (cmd->rem) {
		memcpy(cmd->buf, cmd->buf + cmd->len, cmd->rem);
		cmd->len = cmd->rem;
		cmd->rem = 0;
	}

	s = read(ctx->pt_fd, cmd->buf + cmd->len, cmd->buf_size - cmd->len);
	if (s == -1) {
		ERR("read() failed: %s\n", strerror(errno));
		return -1;
	}

	if (s) {
		int i, rc;

		for (i = 0; i < s; i++) {
			char c = cmd->buf[cmd->len++];

			if (c == '\n' || c == '\r') {
				/* Aborted command or empty */
				if (cmd->abort || (cmd->len == 1)) {
					cmd->len = 0;
					cmd->abort = 0;
					continue;
				}

				/* Received more data than the current command */
				if (s > cmd->len)
					cmd->rem = s - cmd->len;

				ctx->stats.pt_read_cmds++;

				DBG("cmd: %.*s", cmd->len, cmd->buf);

				/* Notify network loop and wait that it processes it */
				cmd->avail = 1;
				rc = sem_wait(&ctx->cmd_sem);
				if (rc < 0)
					ERR("sem_wait() failed : %s\n", strerror(errno));

				if (!cmd->rem)
					cmd->len = 0;
			}
		}

		if (cmd->len == cmd->buf_size) {
			cmd->abort = 1;
			cmd->len = 0;
			ctx->stats.pt_read_abort_cmds++;
		}

		ctx->stats.pt_read_bytes += s;
	} else {
		/* pt slave hanged-up */
		cmd->len = 0;
		cmd->rem = 0;
		cmd->abort = 0;
	}

	return 0;
}

//FIXME do the write in the pseudo-tty thread
static void serial_controller_net_recv(void *data, int msg_id, int src_id, void *buf, int len)
{
	struct serial_controller_ctx *ctx = data;
	struct msg_serial *msg_recv = buf;

	if (msg_id != MSG_SERIAL) {
		ctx->stats.net_invalid_msg_id++;
		return;
	}

	if (msg_recv->cmd_len) {
		ssize_t written;

		written = write(ctx->pt_fd, msg_recv->cmd, msg_recv->cmd_len);
		if (written < 0)
			ctx->stats.pt_write_errors++;

		if (written < msg_recv->cmd_len)
			ctx->stats.pt_write_incomplete++;

		ctx->stats.net_cmds++;
	} else {
		ctx->stats.net_null_cmds++;
	}
}

static void serial_controller_loop(void *data, int timer_status)
{
	struct serial_controller_ctx *ctx = data;
	struct cyclic_task *c_task = ctx->c_task;
	struct pt_cmd *cmd = &ctx->pt_cmd;
	struct msg_serial msg_to_send;
	unsigned int num_sched_stats = STAT_PERIOD_SEC * (NSECS_PER_SEC / ctx->c_task->task->params->task_period_ns);

	msg_to_send.cmd_len = 0;

	if (cmd->avail) {
		int rc;

		memcpy(msg_to_send.cmd, cmd->buf, cmd->len);
		msg_to_send.cmd_len = cmd->len;
		cmd->avail = 0;

		/* Unblock pt side */
		rc = sem_post(&ctx->cmd_sem);
		if (rc < 0)
			ERR("sem_wait() failed : %s\n", strerror(errno));
	} else {
		memset(msg_to_send.cmd, 0, MAX_SERIAL_COMMAND_LEN);
	}

	cyclic_net_transmit(c_task, MSG_SERIAL, &msg_to_send, sizeof(msg_to_send));

	if (ctx->c_task->task->stats.sched % num_sched_stats == 0)
		serial_controller_stats_dump(ctx);
}

struct serial_controller_ctx *serial_controller_init(unsigned int period_ns, unsigned int num_peers, int pt_fd, unsigned int timer_type)
{
	struct cyclic_task *c_task = NULL;
	struct serial_controller_ctx *ctx;

	ctx = malloc(sizeof(struct serial_controller_ctx));
	if (!ctx) {
		ERR("malloc() failed\n");
		goto err;
	}

	memset(ctx, 0, sizeof(*ctx));

	c_task = tsn_conf_get_cyclic_task(CONTROLLER_0);
	if (!c_task) {
		ERR("tsn_conf_get_cyclic_task() failed\n");
		goto err_free;
	}

	cyclic_task_set_period(c_task, period_ns);

#ifdef TRACE_SNAPSHOT
	cyclic_task_init_trace_snapshot(c_task);
#endif

	if (cyclic_task_set_num_peers(c_task, num_peers) < 0) {
		ERR("cyclic_task_set_period() failed\n");
		goto err_free;
	}

	c_task->params.timer_type = timer_type;

	if (cyclic_task_init(c_task, serial_controller_net_recv, serial_controller_loop, ctx) < 0) {
		ERR("cyclic_task_init() failed\n");
		goto err_free;
	}

	ctx->c_task = c_task;
	ctx->pt_cmd.buf_size = MAX_SERIAL_COMMAND_LEN;
	ctx->pt_fd = pt_fd;

	if (sem_init(&ctx->cmd_sem, 0, 0) < 0) {
		ERR("sem_init() failed %s\n", strerror(errno));
		goto err_cyclic_exit;
	}

	if (thread_slot_add(THR_CAP_TSN_PT, pt_fd, EPOLLIN, ctx, serial_controller_pt_handler,
			    NULL, 0, (thr_thread_slot_t **)&ctx->pt_thread) < 0) {
		ERR("thread_slot_add() failed\n");
		goto err_cyclic_exit;
	}

	cyclic_task_start(c_task);

	return ctx;

err_cyclic_exit:
	cyclic_task_exit(c_task);

err_free:
	free(ctx);
err:
	return NULL;
}

void serial_controller_exit(void *data)
{
	struct serial_controller_ctx *ctx = data;

	cyclic_task_exit(ctx->c_task);
	sem_destroy(&ctx->cmd_sem);
	free(ctx);
}
