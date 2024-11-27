/*
 * Copyright 2018-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVB HW timer generic layer
 @details
*/

#include "hw_timer.h"
#include "net_port.h"
#include "net_tx.h"
#include "common/log.h"


static struct hw_timer_drv hw_timer_drv;
static struct hw_timer_drv *hw_timer_drv_h = NULL;

__init int hw_timer_init(void)
{
	int i;

	if (hw_timer_drv_h != NULL) {
		os_log(LOG_ERR, "a HW timer driver is already initialized\n");
		goto err;
	}

	hw_timer_drv_h  = &hw_timer_drv;

	if (rtos_mutex_init(&hw_timer_drv_h->lock) < 0) {
		os_log(LOG_ERR, "rtos_mutex_init failed\n");
		goto err;
	}

	for (i = 0; i < HW_CLOCK_MAX; i++)
		slist_head_init(&hw_timer_drv_h->list_head[i]);

	return 0;

err:
	return -1;
}

__exit void hw_timer_exit(void)
{
	hw_timer_drv_h  = NULL;
}

int hw_timer_register(hw_clock_id_t id, struct hw_timer *timer, unsigned int flags)
{
	if (!timer || id >= HW_CLOCK_MAX)
		return -1;

	rtos_mutex_lock(&hw_timer_drv_h->lock, RTOS_WAIT_FOREVER);

	slist_add_head(&hw_timer_drv_h->list_head[id], &timer->node);

	timer->flags = HW_TIMER_F_FREE;

	timer->flags |= hw_timer_user_flags(flags);

	rtos_mutex_unlock(&hw_timer_drv_h->lock);

	os_log(LOG_INIT, "hw_timer(%p) of hw_clock(%d) registered%s\n", timer, id, hw_timer_is_pps(timer) ? ", pps support" : "");

	return 0;
}

static struct hw_timer *__hw_timer_request(hw_clock_id_t id, unsigned int requested_flags, unsigned int rejected_flags)
{
	unsigned int optional_flags = hw_timer_optional_flags(requested_flags);
	unsigned int mandatory_flags = hw_timer_mandatory_flags(requested_flags);
	struct hw_timer *best_timer = NULL; /* set a suitable hw_timer when optional flag is enabled */
	struct hw_timer *timer;
	struct slist_node *entry;
	struct slist_head *head;

	head = &hw_timer_drv_h->list_head[id];

	slist_for_each(head, entry) {
		timer = container_of(entry, struct hw_timer, node);

		if (!(timer->flags & HW_TIMER_F_FREE))
			continue;

		if (!((timer->flags & mandatory_flags) == mandatory_flags))
			continue;

		if (timer->flags & rejected_flags)
			continue;

		if (((timer->flags & optional_flags) != optional_flags)) {
			/* FIXME determine if timer is better than previous one, in case we have
				several optional flags */
			best_timer = timer;
			continue;
		}

		goto exit;
	}

	if (best_timer) {
		timer = best_timer;
		goto exit;
	}

	return NULL;

exit:
	return timer;
}

struct hw_timer *hw_timer_request(hw_clock_id_t id, unsigned int flags, void (*func)(void *), void *data)
{
	struct hw_timer *timer;

	/* Sanitize flags */
	flags = hw_timer_user_flags(flags);

	rtos_mutex_lock(&hw_timer_drv_h->lock, RTOS_WAIT_FOREVER);

	timer = __hw_timer_request(id, flags, hw_timer_user_flags(~flags));

	/* If no more regular timer, try to get one with extra features */
	if (!timer && !flags)
		timer = __hw_timer_request(id, flags, 0);

	if (timer) {
		timer->flags &= ~HW_TIMER_F_FREE;
		timer->func = func;
		timer->data = data;
		os_log(LOG_INFO, "hw_timer(%p)%s\n", timer, hw_timer_is_pps(timer) ? " pps" : "");
	}

	rtos_mutex_unlock(&hw_timer_drv_h->lock);

	return timer;
}

void hw_timer_free(struct hw_timer *timer)
{
	rtos_mutex_lock(&hw_timer_drv_h->lock, RTOS_WAIT_FOREVER);

	timer->flags |= HW_TIMER_F_FREE;
	timer->func = NULL;
	timer->data = NULL;

	rtos_mutex_unlock(&hw_timer_drv_h->lock);
}

int hw_timer_cancel(struct hw_timer *timer)
{
	return timer->cancel_event(timer);
}
