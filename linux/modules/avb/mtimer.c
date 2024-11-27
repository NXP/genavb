/*
 * Copyright 2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <linux/slab.h>
#include <linux/delay.h>

#include "avbdrv.h"
#include "mtimer.h"
#include "media_clock_drv.h"

static void __mtimer_wake_up_init(struct mtimer_dev *dev, unsigned int now)
{
	atomic_set(&dev->wake_up, 0);
	rational_init(&dev->wake_up_next, now, 1);
	rational_add(&dev->wake_up_next, &dev->wake_up_next, &dev->wake_up_per);
}

int mtimer_start(struct mtimer_dev *dev, struct mtimer_start *start, unsigned int time_now,
			bool internal, bool is_media_clock_running)
{
	unsigned long flags;
	int rc = 0;

	raw_spin_lock_irqsave(&dev->lock, flags);

	if (internal) {
		if (!(dev->flags & MTIMER_FLAGS_STARTED_INT)) {
			dev->flags |= MTIMER_FLAGS_STARTED_INT;
			__mtimer_wake_up_init(dev, time_now);
		}
	} else {
		rational_init(&dev->wake_up_per, (u64)NSEC_PER_SEC * start->q, start->p);

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

	raw_spin_unlock_irqrestore(&dev->lock, flags);

	return rc;
}

int mtimer_stop(struct mtimer_dev *dev, bool internal)
{
	unsigned long flags;
	int rc = 0;

	raw_spin_lock_irqsave(&dev->lock, flags);

	if (internal) {
		dev->flags &= ~MTIMER_FLAGS_STARTED_INT;
	} else {
		if (dev->flags & MTIMER_FLAGS_STARTED_EXT) {
			rc = 1;

			dev->flags &= ~(MTIMER_FLAGS_STARTED_INT | MTIMER_FLAGS_STARTED_EXT);
		}
	}

	raw_spin_unlock_irqrestore(&dev->lock, flags);

	return rc;
}

void mtimer_wake_up_init(struct mtimer_dev *dev, unsigned int now)
{
	unsigned long flags;

	raw_spin_lock_irqsave(&dev->lock, flags);

	__mtimer_wake_up_init(dev, now);

	raw_spin_unlock_irqrestore(&dev->lock, flags);
}

void mtimer_wake_up_init_now(struct mtimer_dev *dev, unsigned int now)
{
	unsigned long flags;

	raw_spin_lock_irqsave(&dev->lock, flags);

	atomic_set(&dev->wake_up, 0);
	rational_init(&dev->wake_up_next, now, 1);

	raw_spin_unlock_irqrestore(&dev->lock, flags);
}

void mtimer_wake_up_thread(struct mtimer_dev *dev, struct mtimer_dev **mtimer_dev_array, unsigned int *n)
{
	if (atomic_read(&dev->wake_up)) {
		if ((*n) < MTIMER_MAX) {
			set_bit(MTIMER_ATOMIC_FLAGS_BUSY, &dev->atomic_flags);

			mtimer_dev_array[*n] = dev;
			(*n)++;
		}
	}
}

int mtimer_wake_up(struct mtimer_dev *dev, unsigned int now)
{
	unsigned long flags;

	raw_spin_lock_irqsave(&dev->lock, flags);

	if (!(dev->flags & MTIMER_FLAGS_STARTED_INT))
		goto out_unlock;

	if (rational_int_cmp(now, &dev->wake_up_next) >= 0) {

		atomic_inc(&dev->wake_up);

		/* Check if there was a change in period, if so, reset the fractional part */
		if (dev->wake_up_next.q != dev->wake_up_per.q) {
			dev->wake_up_next.q = dev->wake_up_per.q;
			dev->wake_up_next.p = 0;
		}

		rational_add(&dev->wake_up_next, &dev->wake_up_next, &dev->wake_up_per);

		raw_spin_unlock_irqrestore(&dev->lock, flags);

		return 1;
	}

out_unlock:
	raw_spin_unlock_irqrestore(&dev->lock, flags);

	return 0;
}

struct mtimer_dev *mtimer_open(mclock_t type, unsigned int domain)
{
	struct mtimer_dev *dev;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		goto err_alloc;

	raw_spin_lock_init(&dev->lock);

	init_waitqueue_head(&dev->wait);
	atomic_set(&dev->wake_up, 0);

	dev->type = type;
	dev->domain = domain;

	return dev;

err_alloc:
	return NULL;
}

void mtimer_release(struct mtimer_dev *dev)
{
	mclock_timer_stop(dev);

	while (test_bit(MTIMER_ATOMIC_FLAGS_BUSY, &dev->atomic_flags))
		usleep_range(25, 50);

	kfree(dev);
}
