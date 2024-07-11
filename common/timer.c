/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2018-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Timer Service implementation
 @details
*/

#include "common/list.h"
#include "common/timer.h"
#include "common/log.h"

#include "os/string.h"
#include "os/timer.h"
#include "os/clock.h"

static void timer_process(struct os_timer *os_timer, int count)
{
	struct timer_sys *timer_sys = container_of(os_timer, struct timer_sys, os_timer);
	struct list_head *entry;
	struct timer *t;

	/* Go through all the children of this system timer and call corresponding callback. */
	/* If the timer callback removes the next entry from under our nose, timer_stop will make sure next_to_process still contains the correct next entry. */
	for (entry = list_first(&timer_sys->head); timer_sys->next_to_process = list_next(entry), entry != &timer_sys->head; entry = timer_sys->next_to_process) {

		t = container_of(entry, struct timer, list);

		t->ms -= timer_sys->ms * count;

		if (t->ms <= 0) {
			os_log(LOG_DEBUG, "timer(%p) expired\n", t);

			/* By default timers are one shot */
			timer_stop(t);

			if (t->func)
				t->func(t->data);
		}
	}
}


static int timer_match_timeout(int sys_ms, int ms)
{
	int match = 0;

	/* only support full matching timeout value for now */
	if (sys_ms == ms)
		match = 1;

	return match;
}

static struct timer_sys *timer_sys_find(struct timer_ctx *tctx, unsigned int ms)
{
	int i;
	struct timer_sys *timer_sys;

	/* walk through the system timer table and look for one matching the requested timeout */
	for (i = 0; i < tctx->max_sys_timers; i++) {

		timer_sys = &tctx->timer_sys_table[i];

		if ((timer_sys->flags & TIMER_STATE_CREATED) && !(timer_sys->flags & TIMER_TYPE_SYS)) {

			if (timer_match_timeout(timer_sys->ms, ms))
				return timer_sys;
		}
	}

	return NULL;
}

static struct timer_sys *timer_sys_create(struct timer_ctx *tctx, unsigned int flags, unsigned int ms)
{
	struct timer_sys *timer_sys;
	int i;

	if (!(flags & TIMER_TYPE_SYS)) {
		/* Put an upper bound on the timer granularity, to avoid big timer errors */
		if (ms > 100)
			ms = 100;

		timer_sys = timer_sys_find(tctx, ms);
		if (timer_sys)
			goto done;
	}

	/* a dedicated system timer has been requested or a new shared one is needed to
	shelter a soft timer */

	if (tctx->num_sys_timers == tctx->max_sys_timers)
		goto err;

	for (i = 0; i < tctx->max_sys_timers; i++) {
		timer_sys = &tctx->timer_sys_table[i];

		if (!(timer_sys->flags & TIMER_STATE_CREATED))
			goto found;
	}

	return NULL;

found:
	if (os_timer_create(&timer_sys->os_timer, OS_CLOCK_SYSTEM_MONOTONIC_COARSE, 0, timer_process, tctx->priv) < 0)
		goto err;

	timer_sys->flags = TIMER_STATE_CREATED | (flags & TIMER_TYPE_SYS);
	timer_sys->ms = ms;

	/* we have consumed a new system timer... */
	tctx->num_sys_timers++;

done:
	timer_sys->users++;

	return timer_sys;

err:
	return NULL;
}

static void timer_sys_destroy(struct timer_sys *timer_sys)
{
	timer_sys->users--;

	if (!timer_sys->users) {
		/* if this was the last user can destroy the system timer */

		os_timer_destroy(&timer_sys->os_timer);

		timer_sys->flags &= ~TIMER_STATE_CREATED;

		timer_sys->ctx->num_sys_timers--;
	}
}

static int timer_sys_start(struct timer_sys *timer_sys)
{
	int rc;

	if (!(timer_sys->flags & TIMER_STATE_STARTED)) {

		if (timer_sys->flags & TIMER_TYPE_SYS)
			rc = os_timer_start(&timer_sys->os_timer, (u64)timer_sys->ms * NSECS_PER_MS, 0, 0, 0);
		else
			rc = os_timer_start(&timer_sys->os_timer, 0, (u64)timer_sys->ms * NSECS_PER_MS, 1, 0);

		if (rc < 0)
			return -1;

		timer_sys->flags |= TIMER_STATE_STARTED;
	}

	return 0;
}

static void timer_sys_stop(struct timer_sys *timer_sys)
{
	if ((timer_sys->flags & TIMER_STATE_STARTED) && list_empty(&timer_sys->head)) {

		os_timer_stop(&timer_sys->os_timer);

		timer_sys->flags &= ~TIMER_STATE_STARTED;
	}
}


/*
 * ms is the timer base resolution for shared timers
 */
int timer_init(struct timer_ctx *tctx, struct timer *t, unsigned int flags, unsigned int ms)
{
	struct timer_sys *timer_sys;

	/* get a suitable timer system */
	timer_sys = timer_sys_create(tctx, flags, ms);
	if (!timer_sys) {
		os_log(LOG_ERR, "timer(%p) creation failed\n", t);
		goto err;
	}

	/* a timer system has been found, attach it to our timer */
	t->timer_sys = timer_sys;
	t->flags = TIMER_STATE_CREATED;

	tctx->num_soft_timers++;

	os_log(LOG_INFO, "timer(%p) created in timer_ctx(%p) using os_timer(%p), flags: %x, ms: %d\n",
						t, tctx, &t->timer_sys->os_timer, flags, ms);

	return 0;

err:
	return -1;
}


int timer_start(struct timer *t, unsigned int ms)
{
	struct timer_sys *timer_sys = t->timer_sys;

	os_log(LOG_DEBUG, "timer(%p) ms: %d\n", t, ms);

	if (!timer_sys) {
		os_log(LOG_ERR, "timer(%p) not created\n", t);
		goto err;
	}

	if (t->flags & TIMER_STATE_STARTED) {
		os_log(LOG_ERR, "timer(%p) already started\n", t);
		goto err;
	}

	if (!ms) {
		os_log(LOG_ERR, "timer(%p) 0ms period\n", t);
		goto err;
	}

	t->ms = ms;

	if (timer_sys->flags & TIMER_TYPE_SYS)
		timer_sys->ms = ms;
	else
		t->ms += timer_sys->ms; /* Add the system timer granularity to guarantee that we will wait at least ms */

	list_add(&timer_sys->head, &t->list);
	t->flags |= TIMER_STATE_STARTED;

	return timer_sys_start(timer_sys);

err:
	return -1;
}


void timer_stop(struct timer *t)
{
	struct timer_sys *timer_sys = t->timer_sys;

	os_log(LOG_DEBUG, "timer(%p)\n", t);

	if (!(t->flags & TIMER_STATE_STARTED)) {
		os_log(LOG_DEBUG, "timer(%p) not started\n", t);
		goto out;
	}

	if (&t->list == timer_sys->next_to_process)
		timer_sys->next_to_process = list_next(&t->list);
	list_del(&t->list);
	t->flags &= ~TIMER_STATE_STARTED;

	timer_sys_stop(timer_sys);

out:
	return;
}

int timer_is_running(struct timer *t)
{
	struct timer_sys *timer_sys = t->timer_sys;
	int rc = 0;

	os_log(LOG_DEBUG, "timer(%p)\n", t);

	if (!timer_sys) {
		os_log(LOG_ERR, "timer(%p) not created\n", t);
		rc = - 1;
		goto err;
	}

	if (t->flags & TIMER_STATE_STARTED)
		rc = 1;

err:
	return rc;
}

int timer_restart(struct timer *t, unsigned int ms)
{
	int rc = timer_is_running(t);

	if(rc < 0)
		goto err;

	if(rc)
		timer_stop(t);

	timer_start(t, ms);

err:
	return rc;
}

int timer_destroy(struct timer *t)
{
	struct timer_sys *timer_sys = t->timer_sys;

	os_log(LOG_INFO, "timer(%p)\n", t);

	timer_stop(t);

	if (!timer_sys) {
		os_log(LOG_ERR, "timer(%p) not created\n", t);
		goto err;
	}

	t->timer_sys = NULL;
	t->flags &= ~TIMER_STATE_CREATED;

	timer_sys_destroy(timer_sys);

	timer_sys->ctx->num_soft_timers--;

	return 0;

err:
	return -1;
}

unsigned int timer_pool_size(unsigned int n)
{
	return sizeof(struct timer_ctx) + n * sizeof(struct timer_sys);
}

int timer_pool_init(struct timer_ctx *tctx, unsigned int n, unsigned long priv)
{
	int i;

	os_log(LOG_INFO, "timer_ctx(%p)\n", tctx);

	os_memset(tctx, 0, timer_pool_size(n));

	tctx->max_sys_timers = n;
	tctx->priv = priv;

	for (i = 0; i < tctx->max_sys_timers; i++) {
		struct timer_sys *timer_sys = &tctx->timer_sys_table[i];

		list_head_init(&timer_sys->head);
		timer_sys->ctx = tctx;
	}

	return 0;
}

void timer_pool_exit(struct timer_ctx *tctx)
{
	os_log(LOG_INFO, "timer_ctx(%p)\n", tctx);
}
