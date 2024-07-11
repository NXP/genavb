/*
 * Copyright 2019-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVB media clock timer driver
*/

#include <string.h>

#include "mtimer.h"
#include "media_clock.h"

void mtimer_set_callback(struct mtimer_dev *dev, void (*cb)(void *), void *data)
{
	dev->cb = cb;
	dev->cb_data = data;
}

static void __mtimer_wake_up_init(struct mtimer_dev *dev, unsigned int now)
{
	rational_init(&dev->wake_up_next, now, 1);
	rational_add(&dev->wake_up_next, &dev->wake_up_next, &dev->wake_up_per);
}

int mtimer_start(struct mtimer_dev *dev, struct mtimer_start *start, unsigned int time_now,
			bool internal, bool is_media_clock_running)
{
	int rc = 0;

	rtos_spin_lock(&rtos_global_spinlock, &rtos_global_key);

	if (internal) {
		if (!(dev->flags & MTIMER_FLAGS_STARTED_INT)) {
			dev->flags |= MTIMER_FLAGS_STARTED_INT;
			__mtimer_wake_up_init(dev, time_now);
		}
	} else {
		rational_init(&dev->wake_up_per, (uint64_t)NSECS_PER_SEC * start->q, start->p);

		if (!(dev->flags & MTIMER_FLAGS_STARTED_EXT)) {
			rc = 1;
			dev->flags |= MTIMER_FLAGS_STARTED_EXT;

			/* If media clock is already running, set the internal start flag and wake up the timer
			 * Otherwise, wait for the internal media clock start to do that */
			if (is_media_clock_running) {
				dev->flags |= MTIMER_FLAGS_STARTED_INT;
				__mtimer_wake_up_init(dev, time_now);
			}
		}
	}

	rtos_spin_unlock(&rtos_global_spinlock, rtos_global_key);

	return rc;
}

int mtimer_stop(struct mtimer_dev *dev, bool internal)
{
	int rc = 0;

	rtos_spin_lock(&rtos_global_spinlock, &rtos_global_key);

	if (internal) {
		dev->flags &= ~MTIMER_FLAGS_STARTED_INT;
	} else {
		if (dev->flags & MTIMER_FLAGS_STARTED_EXT) {
			rc = 1;

			dev->flags &= ~(MTIMER_FLAGS_STARTED_INT | MTIMER_FLAGS_STARTED_EXT);
		}
	}

	rtos_spin_unlock(&rtos_global_spinlock, rtos_global_key);

	return rc;
}

void mtimer_wake_up_init(struct mtimer_dev *dev, unsigned int now)
{
	rtos_spin_lock(&rtos_global_spinlock, &rtos_global_key);

	__mtimer_wake_up_init(dev, now);

	rtos_spin_unlock(&rtos_global_spinlock, rtos_global_key);
}

void mtimer_wake_up_init_now(struct mtimer_dev *dev, unsigned int now)
{
	rtos_spin_lock(&rtos_global_spinlock, &rtos_global_key);

	rational_init(&dev->wake_up_next, now, 1);

	rtos_spin_unlock(&rtos_global_spinlock, rtos_global_key);
}

void mtimer_wake_up(struct mtimer_dev *dev, unsigned int now)
{
	rtos_spin_lock(&rtos_global_spinlock, &rtos_global_key);

	if (!(dev->flags & MTIMER_FLAGS_STARTED_INT))
		goto out_unlock;

	if (rational_int_cmp(now, &dev->wake_up_next) >= 0) {

		/* Check if there was a change in period, if so, reset the fractional part */
		if (dev->wake_up_next.q != dev->wake_up_per.q) {
			dev->wake_up_next.q = dev->wake_up_per.q;
			dev->wake_up_next.p = 0;
		}

		rational_add(&dev->wake_up_next, &dev->wake_up_next, &dev->wake_up_per);

		rtos_spin_unlock(&rtos_global_spinlock, rtos_global_key);

		if (dev->cb)
			dev->cb(dev->cb_data);

		return;
	}

out_unlock:
	rtos_spin_unlock(&rtos_global_spinlock, rtos_global_key);
}

struct mtimer_dev *mtimer_open(mclock_t type, unsigned int domain)
{
	struct mtimer_dev *dev;

	dev = rtos_malloc(sizeof(*dev));
	if (!dev)
		goto err_alloc;

	memset(dev, 0, sizeof(*dev));

	dev->type = type;
	dev->domain = domain;

	return dev;

err_alloc:
	return NULL;
}

void mtimer_release(struct mtimer_dev *dev)
{
	mclock_timer_stop(dev);

	rtos_free(dev);
}
