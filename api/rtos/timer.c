/*
 * Copyright 2019-2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file timer.c
 \brief timer public API for rtos
 \details
 \copyright Copyright 2019-2021, 2023-2024 NXP
*/

#include <string.h>

#include "api/timer.h"
#include "api/clock.h"
#include "genavb/error.h"
#include "common/types.h"

#include "rtos_abstraction_layer.h"

static void genavb_timer_callback(struct os_timer *t, int count)
{
	struct genavb_timer *timer = container_of(t, struct genavb_timer, os_t);

	if (timer->callback)
		timer->callback(timer->data, count);
}

int genavb_timer_create(struct genavb_timer **t, genavb_clock_id_t id, genavb_timer_f_t flags)
{
	os_clock_id_t os_id;
	int rc;
	unsigned int flags_os = 0;

	if (!t) {
		rc = -GENAVB_ERR_INVALID;
		goto out;
	}

	if (flags & GENAVB_TIMERF_PPS)
		flags_os |= OS_TIMER_FLAGS_PPS;

	if (flags & GENAVB_TIMERF_PERIODIC)
		flags_os |= OS_TIMER_FLAGS_PERIODIC;

	if (id >= GENAVB_CLOCK_MAX) {
		rc = -GENAVB_ERR_CLOCK;
		goto out_set_null;
	}

	os_id = genavb_clock_to_os_clock(id);
	if (os_id >= OS_CLOCK_MAX) {
		rc = -GENAVB_ERR_CLOCK;
		goto out_set_null;
	}

	*t = rtos_malloc(sizeof(struct genavb_timer));
	if (!*t) {
		rc = -GENAVB_ERR_NO_MEMORY;
		goto out_set_null;
	}

	memset(*t, 0, sizeof(struct genavb_timer));

	if (os_timer_create(&(*t)->os_t, os_id, flags_os, genavb_timer_callback, 0) < 0) {
		rc = -GENAVB_ERR_TIMER;
		goto out_free_timer;
	}

	return GENAVB_SUCCESS;

out_free_timer:
	rtos_free(*t);

out_set_null:
	*t = NULL;
out:
	return rc;
}

int genavb_timer_set_callback(struct genavb_timer *t, void (*callback)(void *, int),
			      void *data)
{
	if (!t)
		return GENAVB_ERR_INVALID;

	rtos_spin_lock(&rtos_global_spinlock, &rtos_global_key);

	t->data = data;
	t->callback = callback;

	rtos_spin_unlock(&rtos_global_spinlock, rtos_global_key);

	return GENAVB_SUCCESS;
}

int genavb_timer_start(struct genavb_timer *t, uint64_t value, uint64_t interval,
							genavb_timer_f_t flags)
{
	unsigned int flags_os = 0;

	if (!t)
		return -GENAVB_ERR_INVALID;

	if (flags & GENAVB_TIMERF_ABS)
		flags_os |= OS_TIMER_FLAGS_ABSOLUTE;

	if (flags & GENAVB_TIMERF_PPS)
		flags_os |= OS_TIMER_FLAGS_PPS;

	if (os_timer_start(&t->os_t, value, interval, 1, flags_os) < 0)
		return -GENAVB_ERR_TIMER;

	return GENAVB_SUCCESS;
}

void genavb_timer_stop(struct genavb_timer *t)
{
	if (t)
		os_timer_stop(&t->os_t);
}

void genavb_timer_destroy(struct genavb_timer *t)
{
	if (t) {
		os_timer_destroy(&t->os_t);
		rtos_free(t);
	}
}
