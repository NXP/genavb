/*
 * Copyright 2019-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief RTOS specific high-resolution timers
 @details
*/
#include <string.h>

#include "common/log.h"
#include "common/stats.h"

#include "os/clock.h"
#include "os/sys_types.h"
#include "os/timer.h"

#include "rtos_abstraction_layer.h"

#include "clock.h"
#include "hr_timer.h"
#include "hw_timer.h"
#include "slist.h"

#define HR_TIMER_TASK_TIMEOUT		"hr_timer"

#define HR_TIMER_STACK_DEPTH		(RTOS_MINIMAL_STACK_SIZE + 64)
#define HR_TIMER_TASK_PRIORITY		(RTOS_MAX_PRIORITY - 6)
#define HR_TIMER_TASK_NAME		"hr timer"

#define HR_TIMER_ERROR		(1 << 0)
#define HR_TIMER_SUCCESS	(1 << 1)

#define HR_TIMER_RATIO_WND_MS	 	200
#define HR_TIMER_RATIO_WND_NS	 	(HR_TIMER_RATIO_WND_MS * NSECS_PER_MS)
#define HR_TIMER_MIN_DELTA_NS	 	10000

#define HR_TIMER_EVENT_QUEUE_LENGTH	16

struct hr_timer_stats {
	unsigned int start;
	unsigned int stop;
	unsigned int events;
	unsigned int clock_discont;
	unsigned int err_start;
	unsigned int err_event;
	unsigned int err_event_isr;
	unsigned int err_clock;
	unsigned int err_late;
};

struct hr_timer {
	os_clock_id_t clk_id;

	struct hw_timer *hw_timer;
	struct slist_node node;
	struct slist_node node_drv;

	void (*func)(void *data, int count);
	void *data;

	unsigned int skip_reload;
	unsigned int irq_reload;
	rtos_atomic_t irq_pending;
	rtos_atomic_t enqueued;

	uint64_t period;
	uint64_t next_event;
	uint64_t cur_event;	/* Expiration time for current event (ideal/theoretical value) */

	rtos_event_group_t event_group;

	struct hr_timer_stats stats;
	struct stats delay_stats;
};

struct hr_timer_task_stats {
	unsigned int enqueue;
	unsigned int cancel;
	unsigned int run;
	unsigned int err_sched;
	unsigned int err_timeout;
};

struct hr_timer_task_ctx {
	rtos_thread_t task;

	rtos_mqueue_t queue;
	uint8_t queue_buffer[HR_TIMER_EVENT_QUEUE_LENGTH * sizeof(struct event)];

	rtos_timer_t timeout;

	struct slist_head pending[OS_CLOCK_MAX];

	struct hr_timer_task_stats stats;
};

struct hr_timer_drv {
	struct hr_timer_task_ctx task_ctx;
	struct slist_head list;
	rtos_mutex_t lock;
};

static struct hr_timer_drv hr_timer_drv;
struct hr_timer_drv *hr_timer_drv_h;

void hr_timer_task_stats(void)
{
	struct hr_timer_drv *drv = hr_timer_drv_h;
	struct hr_timer_task_ctx *ctx;
	struct hr_timer_task_stats *stats;

	if (!drv)
		return;

	ctx = &drv->task_ctx;
	stats = &drv->task_ctx.stats;

	os_log(LOG_INFO, "hr timer task(%p)\n", ctx);
	os_log(LOG_INFO, " enqueue: %u, cancel: %u, run: %u\n",
		stats->enqueue, stats->cancel, stats->run);
	os_log(LOG_INFO, " errors sched: %u, timeout: %u\n",
		stats->err_sched, stats->err_timeout);
}

static void __hr_timer_stats(struct hr_timer *timer)
{
	struct hr_timer_stats *stats = &timer->stats;
	struct stats *s = &timer->delay_stats;

	os_log(LOG_INFO, "timer(%p), hw_timer(%p), clock id: %d\n",
	       timer, timer->hw_timer, timer->clk_id);
	os_log(LOG_INFO, " period: %llu, next_event: %llu\n",
	       timer->period, timer->next_event);
	os_log(LOG_INFO, " start: %u, stop: %u, events: %u, clock discont: %u\n",
	       stats->start, stats->stop, stats->events, stats->clock_discont);
	os_log(LOG_INFO, " errors start: %u, event: %u, event isr: %u, clock: %u, late: %u\n",
	       stats->err_start, stats->err_event, stats->err_event_isr, stats->err_clock,
		stats->err_late);

	stats_compute(s);
	stats_reset(s);
	os_log(LOG_INFO, " %s min %d mean %d max %d rms^2 %llu stddev^2 %llu\n", s->priv, s->min, s->mean, s->max, s->ms, s->variance);
}

void hr_timer_stats(void)
{
	struct hr_timer_drv *drv = hr_timer_drv_h;
	struct slist_node *entry;

	if (!drv)
		return;

	rtos_mutex_lock(&drv->lock, RTOS_WAIT_FOREVER);

	slist_for_each(&drv->list, entry)
		__hr_timer_stats(container_of(entry, struct hr_timer, node_drv));

	rtos_mutex_unlock(&drv->lock);
}

static int __hr_timer_program_next_event(struct hr_timer *t, uint64_t next_event)
{
	uint64_t cycles;

	if (clock_time_to_cycles_isr(t->clk_id, next_event, &cycles) < 0)
		goto err;

	if (hw_timer_set_next_event(t->hw_timer, cycles) < 0)
		goto err;

	return 0;

err:
	t->stats.err_event_isr++;

	return -1;
}

/* The next_event programs the beginning of the first period
 * for timer with "HW_TIMER_F_PERIODIC" flag
 */
static int __hr_timer_program_periodic_event(struct hr_timer *t, uint64_t next_event)
{
	uint64_t cycles;
	uint64_t period;

	if (clock_time_to_cycles_isr(t->clk_id, next_event, &cycles) < 0)
		goto err;

	if (clock_dtime_to_cycles_isr(t->clk_id, t->period, &period) < 0)
		goto err;

	if (hw_timer_set_periodic_event(t->hw_timer, cycles, period) < 0)
		goto err;

	return 0;

err:
	t->stats.err_event_isr++;

	return -1;
}

static inline int hr_timer_program_next_event_isr(struct hr_timer *t, uint64_t next_event)
{
	return __hr_timer_program_next_event(t, next_event);
}

static inline int hr_timer_program_periodic_event_isr(struct hr_timer *t, uint64_t next_event)
{
	return __hr_timer_program_periodic_event(t, next_event);
}

static int hr_timer_task_schedule(struct hr_timer_task_ctx *timer_task, uint64_t wake_time)
{
	rtos_tick_t period;

	period = RTOS_MS_TO_TICKS_AT_LEAST(wake_time / NSECS_PER_MS, 1);

	if (rtos_timer_start(&timer_task->timeout, period) < 0)
		goto err;

	os_log(LOG_DEBUG, "timer task timeout set to %d ms\n", wake_time / NSECS_PER_MS);

	return 0;

err:
	timer_task->stats.err_sched++;
	return -1;
}

static void hr_timer_task_run(struct hr_timer_task_ctx *timer_task)
{
	int i;
	uint64_t expires_next = UINT64_MAX;

	for (i = 0; i < OS_CLOCK_MAX; i++) {
		struct slist_node *entry, *next;
		struct slist_head *head;
		struct hr_timer *t;
		uint64_t now, next_event, expire;

		head = &timer_task->pending[i];

		if (slist_empty(head))
			continue;

		slist_for_each_safe(head, entry, next) {
			t = container_of(entry, struct hr_timer, node);

			rtos_mutex_global_lock();

			/*
			 * rtos_mutex_global_lock() is used to make sure gettime
			 * and program next event are done atomically
			 */

			if (os_clock_gettime64_isr(i, &now) < 0) {
				t->stats.err_clock++;
				rtos_mutex_global_unlock();
				continue;
			}

			next_event = t->next_event;

			if (likely(now < next_event))
				expire = t->next_event - now;
			else
				expire = HR_TIMER_MIN_DELTA_NS;

			if (expire <= HR_TIMER_RATIO_WND_NS) {
				if (expire <= HR_TIMER_MIN_DELTA_NS)
					next_event = now + HR_TIMER_MIN_DELTA_NS;

				rtos_atomic_set(&t->irq_pending, 1);

				if (t->skip_reload) {
					/* the first period of timer with "HW_TIMER_F_PERIODIC" flag will expire at value + period */
					t->next_event += t->period;
					t->cur_event = t->next_event;
					hr_timer_program_periodic_event_isr(t, next_event);
				} else {
					t->cur_event = t->next_event;
					hr_timer_program_next_event_isr(t, next_event);
				}

				if (!t->period || t->irq_reload || t->skip_reload) {
					rtos_atomic_set(&t->enqueued, 0);
					slist_del(head, &t->node);
					rtos_mutex_global_unlock();
					continue;
				} else {
					t->next_event += t->period;
					expire += t->period;
				}
			}

			rtos_mutex_global_unlock();

			if (expire < expires_next)
				expires_next = expire;
		}
	}

	if (expires_next != UINT64_MAX)
		hr_timer_task_schedule(timer_task, expires_next - HR_TIMER_RATIO_WND_NS / 8);

	timer_task->stats.run++;
}

static void hr_timer_task_enqueue(struct hr_timer_task_ctx *timer_task, struct hr_timer *t)
{
	os_log(LOG_DEBUG, "enqueue t(%p) clk_id: %d\n", t, t->clk_id);

	slist_add_head(&timer_task->pending[t->clk_id], &t->node);

	rtos_atomic_set(&t->enqueued, 1);

	hr_timer_task_run(timer_task);

	rtos_event_group_set(&t->event_group, HR_TIMER_SUCCESS);

	timer_task->stats.enqueue++;

	return;
}

static void hr_timer_task_cancel(struct hr_timer_task_ctx *timer_task, struct hr_timer *t)
{
	struct slist_node *entry, *next;
	struct slist_head *head = &timer_task->pending[t->clk_id];
	struct hr_timer *timer_pending;

	os_log(LOG_DEBUG, "cancel t(%p) clk_id: %d\n", t, t->clk_id);

	if (rtos_atomic_read(&t->enqueued)) {
		slist_for_each_safe(head, entry, next) {
			timer_pending = container_of(entry, struct hr_timer, node);
			if (timer_pending == t) {
				slist_del(head, &timer_pending->node);
				break;
			}
		}
	}

	rtos_event_group_set(&t->event_group, HR_TIMER_SUCCESS);

	timer_task->stats.cancel++;
}

static void hr_timer_task_timeout(rtos_timer_t *t, void *data)
{
	struct hr_timer_task_ctx *timer_task = (struct hr_timer_task_ctx *)data;
	struct event e;

	e.type = EVENT_HR_TIMER_TIMEOUT;

	if (rtos_mqueue_send(&timer_task->queue, &e, RTOS_NO_WAIT) < 0)
		timer_task->stats.err_timeout++;
}

static void hr_timer_task(void *pvParameters)
{
	struct hr_timer_task_ctx *timer_task = pvParameters;
	struct event e;

	for (;;) {
		if (rtos_mqueue_receive(&timer_task->queue, &e, RTOS_WAIT_FOREVER) < 0)
			continue;

		switch (e.type) {
		case EVENT_HR_TIMER_ENQUEUE:
			hr_timer_task_enqueue(timer_task, e.data);
			break;

		case EVENT_HR_TIMER_CANCEL:
			hr_timer_task_cancel(timer_task, e.data);
			break;

		case EVENT_HR_TIMER_TIMEOUT:
			hr_timer_task_run(timer_task);
			break;

		default:
			break;
		}
	}
}

__init int hr_timer_init(void)
{
	struct hr_timer_task_ctx *task_ctx;
	int i;

	hr_timer_drv_h = &hr_timer_drv;
	task_ctx = &hr_timer_drv_h->task_ctx;

	slist_head_init(&hr_timer_drv_h->list);

	for (i = 0; i < OS_CLOCK_MAX; i++)
		slist_head_init(&task_ctx->pending[i]);

	if (rtos_mutex_init(&hr_timer_drv_h->lock) < 0) {
		os_log(LOG_ERR, "rtos_mutex_init failed\n");
		goto err;
	}

	if (rtos_mqueue_init(&task_ctx->queue, HR_TIMER_EVENT_QUEUE_LENGTH,
						    sizeof(struct event),
						    task_ctx->queue_buffer
						    ) < 0) {
		os_log(LOG_ERR, "rtos_mqueue_init(hr timer) failed\n");
		goto err;
	}

	if (rtos_timer_init(&task_ctx->timeout,
			    HR_TIMER_TASK_TIMEOUT,
			    false,
			    hr_timer_task_timeout,
			    task_ctx) < 0) {
		os_log(LOG_ERR, "rtos_timer_init(%s) failed\n", HR_TIMER_TASK_TIMEOUT);
		goto err;
	}

	if (rtos_thread_create(&task_ctx->task, HR_TIMER_TASK_PRIORITY, 0, HR_TIMER_STACK_DEPTH, HR_TIMER_TASK_NAME, hr_timer_task, task_ctx) < 0) {
		os_log(LOG_ERR, "rtos_thread_create(%s) failed\n", HR_TIMER_TASK_NAME);
		goto err;
	}

	return 0;

err:
	return -1;
}

__exit void hr_timer_exit(void)
{
	struct hr_timer_task_ctx *task_ctx = &hr_timer_drv_h->task_ctx;

	rtos_thread_abort(&task_ctx->task);

	if (rtos_timer_destroy(&task_ctx->timeout, RTOS_MS_TO_TICKS(10)) < 0) {
		os_log(LOG_ERR, "rtos_timer_destroy failed\n");
		goto exit;
	}

exit:
	hr_timer_drv_h = NULL;
}

static int __hr_timer_send_event(struct hr_timer_task_ctx *timer_task,
					struct hr_timer *t, unsigned int type)
{
	struct event e;
	uint32_t rc;

	e.type = type;
	e.data = t;

	if (rtos_mqueue_send(&timer_task->queue, &e, RTOS_NO_WAIT) < 0)
		goto err;

	rc = rtos_event_group_wait(&t->event_group,
				 HR_TIMER_ERROR | HR_TIMER_SUCCESS,
				 true, RTOS_MS_TO_TICKS(2));

	if (!(rc & HR_TIMER_SUCCESS))
		goto err;

	return 0;

err:
	return -1;
}

static int hr_timer_enqueue(struct hr_timer_task_ctx *timer_task, struct hr_timer *t)
{
	return __hr_timer_send_event(timer_task, t, EVENT_HR_TIMER_ENQUEUE);
}

static int hr_timer_cancel(struct hr_timer_task_ctx *timer_task, struct hr_timer *t)
{
	return __hr_timer_send_event(timer_task, t, EVENT_HR_TIMER_CANCEL);
}

static void __hr_timer_stop(struct hr_timer *t)
{
	/*
	 * Remove timer from programming task.
	 */
	if (rtos_atomic_read(&t->enqueued))
		hr_timer_cancel(&hr_timer_drv_h->task_ctx, t);

	/*
	 * Make sure IRQ handler cannot call its callback
	 * and then disable IRQ in HW timer.
	 */
	if (rtos_atomic_read(&t->irq_pending)) {
		rtos_atomic_set(&t->irq_pending, 0);
		hw_timer_cancel(t->hw_timer);
	}
}

/*
 * Timer delay handling:
 * "t->cur_event" always tracks the ideal/theoretical time for the current event
 * (when the timer should expire).
 * If the difference between "t->cur_event" and "now" is more or equal to one period,
 * then it gets reported to the application through the "count" callback argument
 * (with count > 1). The t->next_event time is shifted by the same amount of periods
 * as reported through "count". This way, all periods are accounted for.
 */
static void hr_timer_callback(void *data)
{
	struct hr_timer *t = data;
	uint64_t now, next_event;
	int rc = 1;

	if (!rtos_atomic_read(&t->irq_pending))
		return;

	if (os_clock_gettime64_isr(t->clk_id, &now) < 0)
		now = t->cur_event;

	stats_update(&t->delay_stats, now - t->cur_event);

	if ((now - t->cur_event) >= t->period) {
		unsigned int n = (now - t->cur_event + t->period / 2) / t->period;

		t->cur_event += n * t->period;
		t->next_event += n * t->period;
		t->stats.err_late += n;
		rc += n;
	}

	/* skip_reload is used for timer with "HW_TIMER_F_PERIODIC" flag
	* and it is used to skip programming of next_event
	*/
	if (t->skip_reload) {
		t->next_event += t->period;
		t->cur_event = t->next_event;
	} else if (t->irq_reload) {
		t->next_event += t->period;
		next_event = t->cur_event = t->next_event;

		if (next_event < (now + HR_TIMER_MIN_DELTA_NS))
			next_event = now + HR_TIMER_MIN_DELTA_NS;

		if (hr_timer_program_next_event_isr(t, next_event) < 0)
			rc = -1;
	} else {
		rtos_atomic_set(&t->irq_pending, 0);
	}

	t->func(t->data, rc);

	t->stats.events++;
}

int hr_timer_start(void *handle, u64 value, u64 interval_p, u64 interval_q, unsigned int flags)
{
	struct hr_timer *t = handle;
	uint64_t now;

	if (!t)
		goto err_null;

	/* No support for rational period */
	if (interval_p && (interval_q != 1))
		goto err;

	if (os_clock_gettime64(t->clk_id, &now) < 0)
		goto err;

	/* For periodic timer, first expiration at the end of the first period */
	if (!t->skip_reload)
		value += interval_p;

	if (flags & OS_TIMER_FLAGS_PPS) {
		if (hw_timer_pps_enable(t->hw_timer) < 0)
			goto err;
	} else {
		hw_timer_pps_disable(t->hw_timer);
	}

	/*
	 * An expire time already in the past is not valid, other cases
	 * which would eventually be in the past at the time of programming the
	 * timer would be set to expire immediately.
	 */
	if ((flags & OS_TIMER_FLAGS_ABSOLUTE) && (value < now))
		goto err;

	__hr_timer_stop(t);

	if (flags & OS_TIMER_FLAGS_ABSOLUTE)
		t->next_event = value;
	else
		t->next_event = value + now;

	if (interval_p) {
		t->period = interval_p;

		/*
		 * If the timer has a period smaller than the valid ratio window
		 * it can be re-armed automatically in the interrupt callback.
		 */
		if (interval_p < HR_TIMER_RATIO_WND_NS)
			t->irq_reload = 1;
		else
			t->irq_reload = 0;
	}
	else
		t->period = 0;

	if (hr_timer_enqueue(&hr_timer_drv_h->task_ctx, t) < 0)
		goto err;

	t->stats.start++;

	return 0;

err:
	t->stats.err_start++;

err_null:
	return -1;
}

void hr_timer_stop(void *handle)
{
	struct hr_timer *t = handle;

	if (!t)
		return;

	__hr_timer_stop(t);

	t->stats.stop++;
}

void *hr_timer_create(os_clock_id_t clk_id, unsigned int flags,
				 void (*func)(void *data, int count), void *data)
{
	struct hr_timer *t;
	hw_clock_id_t hw_id;
	unsigned int hw_timer_flags = 0;

	if (flags & OS_TIMER_FLAGS_PPS)
		hw_timer_flags |= HW_TIMER_F_PPS;

	if (flags & OS_TIMER_FLAGS_RECOVERY)
		hw_timer_flags |= HW_TIMER_F_RECOVERY;

	if (flags & OS_TIMER_FLAGS_PERIODIC)
		hw_timer_flags |= HW_TIMER_F_PERIODIC;

	if (!func)
		goto err;

	hw_id = clock_to_hw_clock(clk_id);

	if (hw_id == HW_CLOCK_NONE)
		goto err;

	t = rtos_malloc(sizeof(struct hr_timer));
	if (!t)
		goto err;

	memset(t, 0, sizeof(*t));

	t->clk_id = clk_id;
	t->func = func;
	t->data = data;

	if (rtos_event_group_init(&t->event_group) < 0)
		goto err_free_timer;

	t->hw_timer = hw_timer_request(hw_id, hw_timer_flags, hr_timer_callback, t);
	if (!t->hw_timer)
		goto err_free_timer;

	/* Verify if the timer with "HW_TIMER_F_PERIODIC" flag is selected to skip timer reload */
	if (t->hw_timer->flags & HW_TIMER_F_PERIODIC) {
		t->skip_reload = 1;
	} else {
		t->skip_reload = 0;
	}

	stats_init(&t->delay_stats, 31, "delay stats", NULL);

	rtos_mutex_lock(&hr_timer_drv_h->lock, RTOS_WAIT_FOREVER);

	slist_add_head(&hr_timer_drv_h->list, &t->node_drv);

	rtos_mutex_unlock(&hr_timer_drv_h->lock);

	return t;

err_free_timer:
	rtos_free(t);

err:
	return NULL;
}

void hr_timer_destroy(void *handle)
{
	struct hr_timer *t = handle;

	if (!t)
		return;

	rtos_mutex_lock(&hr_timer_drv_h->lock, RTOS_WAIT_FOREVER);

	slist_del(&hr_timer_drv_h->list, &t->node_drv);

	rtos_mutex_unlock(&hr_timer_drv_h->lock);

	__hr_timer_stop(t);
	hw_timer_free(t->hw_timer);
	rtos_free(t);
}

void hr_timer_clock_discontinuity(os_clock_id_t clk)
{
	struct slist_node *entry;
	struct hr_timer *t;

	rtos_mutex_lock(&hr_timer_drv_h->lock, RTOS_WAIT_FOREVER);

	slist_for_each(&hr_timer_drv_h->list, entry) {
		t = container_of(entry, struct hr_timer, node_drv);

		if (clk == t->clk_id) {
			__hr_timer_stop(t);
			t->func(t->data, -1);
			t->stats.clock_discont++;
		}
	}

	rtos_mutex_unlock(&hr_timer_drv_h->lock);
}
