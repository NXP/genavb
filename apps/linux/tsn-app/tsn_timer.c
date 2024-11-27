/*
 * Copyright 2021-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <unistd.h>
#include "tsn_timer.h"
#include "../common/log.h"
#include "../common/time.h"
#include "../common/timer.h"

struct thread_sleep_epoll_timer_ctx {
	thr_thread_slot_t *slot; // pointer to the slot used for the timer
	int timer_fd;
};

static void *tsn_timer_epoll_init(int capabilities, void *ctx,
				      int (*handler)(void *data, unsigned int events),
				      thr_thread_slot_t **slot)
{
	struct thread_sleep_epoll_timer_ctx *timer_ctx;

	timer_ctx = malloc(sizeof(struct thread_sleep_epoll_timer_ctx));
	if (!timer_ctx) {
		ERR("malloc() failed\n");
		goto err;
	}

	timer_ctx->timer_fd = create_timerfd_periodic_abs(CLOCK_REALTIME);
	if (timer_ctx->timer_fd < 0) {
		ERR("create_timerfd_periodic_abs() failed\n");
		goto err_free;
	}

	if (thread_slot_add(capabilities, timer_ctx->timer_fd, EPOLLIN, ctx, handler, NULL, 0, slot) < 0) {
		ERR("thread_slot_add() failed\n");
		goto close_timerfd;
	}

	timer_ctx->slot = *slot;

	return timer_ctx;

close_timerfd:
	close(timer_ctx->timer_fd);

err_free:
	free(timer_ctx);

err:
	return NULL;
}

static void tsn_timer_epoll_exit(void *ctx)
{
	if (ctx) {
		struct thread_sleep_epoll_timer_ctx *timer_ctx = (struct thread_sleep_epoll_timer_ctx *)ctx;
		thread_slot_free(timer_ctx->slot);

		close(timer_ctx->timer_fd);
		free(timer_ctx);
	}
}

static int tsn_timer_epoll_start(void *ctx, uint64_t start_time, unsigned int period_ns)
{
	struct thread_sleep_epoll_timer_ctx *timer_ctx = (struct thread_sleep_epoll_timer_ctx *)ctx;
	time_t start_secs = start_time / (NSECS_PER_SEC);
	unsigned int start_nsecs = start_time - ((uint64_t)start_secs * NSECS_PER_SEC);

	if (start_timerfd_periodic_abs(timer_ctx->timer_fd, start_secs, start_nsecs, 0, period_ns) < 0) {
		ERR("start_timerfd_periodic_abs() error\n");
		return -1;
	}

	return 0;
}

static void tsn_timer_epoll_stop(void *ctx)
{
	if (ctx) {
		struct thread_sleep_epoll_timer_ctx *timer_ctx = (struct thread_sleep_epoll_timer_ctx *)ctx;

		stop_timerfd(timer_ctx->timer_fd);
	}
}

static int tsn_timer_epoll_check(void *ctx, uint64_t now, uint64_t *n_time)
{
	struct thread_sleep_epoll_timer_ctx *timer_ctx = (struct thread_sleep_epoll_timer_ctx *)ctx;

	return read(timer_ctx->timer_fd, &n_time, sizeof(n_time));
}

struct thread_sleep_nanosleep_timer_ctx {
	struct thread_sleep_nanosleep_data nsleep;
	thr_thread_slot_t *slot; // pointer to the slot used for the timer
	unsigned int period_ns;
};

static void *tsn_timer_nanosleep_init(int capabilities, void *ctx,
					  int (*handler)(void *data, unsigned int events),
					  thr_thread_slot_t **slot)
{
	struct thread_sleep_nanosleep_timer_ctx *timer_ctx;

	timer_ctx = malloc(sizeof(struct thread_sleep_nanosleep_timer_ctx));
	if (!timer_ctx) {
		ERR("malloc() failed\n");
		goto err;
	}

	timer_ctx->nsleep.is_armed = 0;
	if (__thread_slot_add(capabilities, 0, 0, ctx, handler, NULL, 0, slot, thread_sleep_nanosleep, &timer_ctx->nsleep) < 0) {
		ERR("__thread_slot_add() failed\n");
		goto err_free;
	}

	timer_ctx->slot = *slot;

	return timer_ctx;

err_free:
	free(timer_ctx);

err:
	return NULL;
}

static int tsn_timer_nanosleep_start(void *ctx, uint64_t start_time, unsigned int period_ns)
{
	struct thread_sleep_nanosleep_timer_ctx *timer_ctx = (struct thread_sleep_nanosleep_timer_ctx *)ctx;

	timer_ctx->nsleep.next = start_time;
	timer_ctx->nsleep.is_armed = 1;
	timer_ctx->period_ns = period_ns;

	return 0;
}

static void tsn_timer_nanosleep_stop(void *ctx)
{
	if (ctx) {
		struct thread_sleep_nanosleep_timer_ctx *timer_ctx = (struct thread_sleep_nanosleep_timer_ctx *)ctx;

		timer_ctx->nsleep.is_armed = 0;
	}
}

static void tsn_timer_nanosleep_exit(void *ctx)
{
	if (ctx) {
		struct thread_sleep_nanosleep_timer_ctx *timer_ctx = (struct thread_sleep_nanosleep_timer_ctx *)ctx;

		thread_slot_free(timer_ctx->slot);
		free(timer_ctx);
	}
}

static int tsn_timer_nanosleep_check(void *ctx, uint64_t now, uint64_t *n_time)
{
	struct thread_sleep_nanosleep_timer_ctx *timer_ctx = (struct thread_sleep_nanosleep_timer_ctx *)ctx;

	if ((now < timer_ctx->nsleep.next) || ((now - timer_ctx->nsleep.next) > 10000000000UL)) {
		return -1;
	}

	*n_time = ((now - timer_ctx->nsleep.next) / timer_ctx->period_ns) + 1;

	timer_ctx->nsleep.next += *n_time * timer_ctx->period_ns;
	timer_ctx->nsleep.is_armed = 1;
	DBG("now(%" PRIu64 ") sched_next(%" PRIu64 ") n_time(%" PRIu64 ") sched_time(%" PRIu64 ")", now, task->nsleep.next, *n_time, task->sched_time);

	return 0;
}

static struct tsn_timer_ops tsn_timer_ops[TSN_TIMER_MAX] = {
	[0] = {
		.init = tsn_timer_epoll_init,
		.exit = tsn_timer_epoll_exit,
		.start = tsn_timer_epoll_start,
		.stop = tsn_timer_epoll_stop,
		.check = tsn_timer_epoll_check,
		.type = "epoll",
	},
	[1] = {
		.init = tsn_timer_nanosleep_init,
		.exit = tsn_timer_nanosleep_exit,
		.start = tsn_timer_nanosleep_start,
		.stop = tsn_timer_nanosleep_stop,
		.check = tsn_timer_nanosleep_check,
		.type = "nanosleep",
	}};

struct tsn_timer *tsn_timer_init(unsigned int timer_type, int capabilities, void *ctx,
				 int (*handler)(void *data, unsigned int events))
{
	struct tsn_timer *timer;

	if (timer_type >= TSN_TIMER_MAX) {
		ERR("Unknown timer type %d", timer_type);
		goto err;
	}

	timer = malloc(sizeof(struct tsn_timer));
	if (!timer) {
		ERR("malloc() failed\n");
		goto err;
	}

	timer->ops = &tsn_timer_ops[timer_type];

	timer->ctx = timer->ops->init(capabilities, ctx, handler, &timer->slot);

	if (!timer->ctx) {
		ERR("timer registration failed");
		goto err_register;
	}

	INF("sleep handler         : %s", timer->ops->type);

	return timer;

err_register:
	free(timer);

err:
	return NULL;
}

void tsn_timer_exit(struct tsn_timer *timer)
{
	timer->ops->exit(timer->ctx);

	free(timer);
}
