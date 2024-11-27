/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _TSN_TIMER_H_
#define _TSN_TIMER_H_

#include <stdint.h>
#include "../common/thread.h"

#define TSN_TIMER_EPOLL	    0
#define TSN_TIMER_NANOSLEEP 1
#define TSN_TIMER_MAX	    2

struct tsn_timer_ops {
	void *(*init)(int capabilities, void *ctx,
		      int (*handler)(void *data, unsigned int events),
		      thr_thread_slot_t **slot);
	void (*exit)(void *timer_ctx);
	int (*start)(void *timer_ctx, uint64_t start_time, unsigned int period_ns);
	void (*stop)(void *timer_ctx);
	int (*check)(void *timer_ctx, uint64_t now, uint64_t *n_time);

	char *type;
};

struct tsn_timer {
	thr_thread_slot_t *slot;
	struct tsn_timer_ops *ops;
	void *ctx;
};

struct tsn_timer *tsn_timer_init(unsigned int timer_type, int capabilities, void *ctx,
				 int (*handler)(void *data, unsigned int events));

void tsn_timer_exit(struct tsn_timer *timer);

static inline int tsn_timer_start(struct tsn_timer *timer, uint64_t start_time, unsigned int period_ns)
{
	return timer->ops->start(timer->ctx, start_time, period_ns);
}

static inline void tsn_timer_stop(struct tsn_timer *timer)
{
	timer->ops->stop(timer->ctx);
}

static inline int tsn_timer_check(struct tsn_timer *timer, uint64_t now, uint64_t *n_time)
{
	return timer->ops->check(timer->ctx, now, n_time);
}

#endif /* _TSN_TIMER_H_ */
