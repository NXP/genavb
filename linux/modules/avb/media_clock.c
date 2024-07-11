/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "avbdrv.h"
#include "avtp.h"
#include "mtimer.h"

/**
 * DOC: Media clock driver
 * This driver is designed to handle any type of media clock which are differenciated
 * using minor ranges (see media_clock.h).
 * A type of media clock is mainly defined by its set of operations (mclock_ops).
 * The media clock features availability depends on the HW capabilities provided in
 * the device tree.
 */

int mclock_drift_adapt(struct mclock_dev *dev, unsigned int clk_media)
{
	int rc = 0;

	if (avtp_after_eq(dev->clk_timer, dev->next_drift)) {
		signed int err = clk_media - dev->clk_timer;

		/* Media clock is faster */
		if (err >= (int)dev->timer_period) {
			if (err > 3 * (int)dev->timer_period)
				rc = -1;

			dev->clk_timer += dev->timer_period;
		} else if (err <= -((int)dev->timer_period)) {
			if (err < -3 * (int)dev->timer_period)
				rc = -1;

			dev->clk_timer -= dev->timer_period;
		}

		dev->next_drift = dev->clk_timer + dev->drift_period;
	}

	return rc;
}

void mclock_set_ts_freq(struct mclock_dev *dev, unsigned int ts_freq_p, unsigned int ts_freq_q)
{
	dev->ts_freq_p = ts_freq_p;
	dev->ts_freq_q = ts_freq_q;
	rational_init(&dev->ts_period, (unsigned long long)ts_freq_q * NSEC_PER_SEC, ts_freq_p);
}

/* Called in irq context under the driver read lock */
void mclock_wake_up_init(struct mclock_dev *dev, unsigned int now)
{
	struct mtimer_dev *mtimer_dev;

	list_for_each_entry(mtimer_dev, &dev->mtimer_devices, list)
		mtimer_wake_up_init(mtimer_dev, now);
}

/* Called in irq context under the driver read lock */
void mclock_wake_up_init_now(struct mclock_dev *dev, unsigned int now)
{
	struct mtimer_dev *mtimer_dev;

	list_for_each_entry(mtimer_dev, &dev->mtimer_devices, list)
		mtimer_wake_up_init_now(mtimer_dev, now);
}

/* Called in thread context under the driver read lock */
void mclock_wake_up_thread(struct mclock_dev *dev, struct mtimer_dev **mtimer_dev_array, unsigned int *n)
{
	struct mtimer_dev *mtimer_dev;

	list_for_each_entry(mtimer_dev, &dev->mtimer_devices, list)
		mtimer_wake_up_thread(mtimer_dev, mtimer_dev_array, n);
}

/* Called in irq context under the driver read lock */
int mclock_wake_up(struct mclock_dev *dev, unsigned int now)
{
	struct mtimer_dev *mtimer_dev;
	int rc = 0;

	if (!(dev->flags & MCLOCK_FLAGS_INIT)) {

		list_for_each_entry(mtimer_dev, &dev->mtimer_devices, list)
			rc |= mtimer_wake_up(mtimer_dev, now);
	}

	return rc;
}

extern struct mclock_drv *mclk_drv;

/* Called with the driver write lock held */
static int __mclock_timer_start(struct mtimer_dev *mtimer_dev, struct mtimer_start *start, bool internal)
{
	struct mclock_dev *dev;
	int rc;
	bool is_media_clock_running;

	/* get reference to media clock */
	if (internal)
		dev = mtimer_dev->mclock_dev;
	else
		dev = __mclock_drv_find_device(mtimer_dev->type, mtimer_dev->domain);

	if (!dev) {
		rc = -ENODEV;
		goto err;
	}

	is_media_clock_running = dev->flags & MCLOCK_FLAGS_RUNNING;

	if (mtimer_start(mtimer_dev, start, dev->clk_timer, internal, is_media_clock_running)) {

		/* If the first timer in the list and media clock is running: add the wake up flag */
		if (list_empty(&dev->mtimer_devices) & is_media_clock_running)
			dev->flags |= MCLOCK_FLAGS_WAKE_UP;

		list_add(&mtimer_dev->list, &dev->mtimer_devices);

		mtimer_dev->mclock_dev = dev;
	} else if (internal){
		/* Add the wake up flag.
		 * No need to check if list is not empty: if internal start it's definetly populated */
		dev->flags |= MCLOCK_FLAGS_WAKE_UP;
	}

	return 0;

err:
	return rc;
}

int mclock_timer_start(struct mtimer_dev *mtimer_dev, struct mtimer_start *start)
{
	unsigned long flags;
	int rc;

	if (!start->p || !start->q) {
		rc = -EINVAL;
		goto err;
	}

	raw_spin_lock_irqsave(&mclk_drv->lock, flags);

	rc = __mclock_timer_start(mtimer_dev, start, false);

	raw_spin_unlock_irqrestore(&mclk_drv->lock, flags);

	return 0;

err:
	return rc;
}

static void __mclock_timer_stop(struct mtimer_dev *mtimer_dev, bool internal)
{
	struct mclock_dev *dev = mtimer_dev->mclock_dev;

	if (mtimer_stop(mtimer_dev, internal)) {

		list_del(&mtimer_dev->list);

		if (list_empty(&dev->mtimer_devices))
			dev->flags &= ~MCLOCK_FLAGS_WAKE_UP;

		mtimer_dev->mclock_dev = NULL;
	} else if (internal) {
		/* Clear the wake up flag.
		 * No need to check if list is not empty: if internal stop it's definetly populated */
		dev->flags &= ~MCLOCK_FLAGS_WAKE_UP;
	}
}

void mclock_timer_stop(struct mtimer_dev *mtimer_dev)
{
	unsigned long flags;

	raw_spin_lock_irqsave(&mclk_drv->lock, flags);

	__mclock_timer_stop(mtimer_dev, false);

	raw_spin_unlock_irqrestore(&mclk_drv->lock, flags);
}

static void mclock_timer_start_all(struct mclock_dev *dev, struct mclock_start *start)
{
	struct mtimer_dev *mtimer_dev;
	struct mtimer_dev *tmp;
	unsigned long flags;

	raw_spin_lock_irqsave(&dev->drv->lock, flags);

	list_for_each_entry_safe(mtimer_dev, tmp, &dev->mtimer_devices, list)
		__mclock_timer_start(mtimer_dev, NULL, true);

	raw_spin_unlock_irqrestore(&dev->drv->lock, flags);
}

void mclock_timer_stop_all(struct mclock_dev *dev)
{
	struct mtimer_dev *mtimer_dev;
	struct mtimer_dev *tmp;
	unsigned long flags;

	raw_spin_lock_irqsave(&dev->drv->lock, flags);

	list_for_each_entry_safe(mtimer_dev, tmp, &dev->mtimer_devices, list)
		__mclock_timer_stop(mtimer_dev, true);

	raw_spin_unlock_irqrestore(&dev->drv->lock, flags);
}

int mclock_start(struct mclock_dev *dev, struct mclock_start *start)
{
	int rc;

	if (dev->start) {
		if (dev->flags & MCLOCK_FLAGS_RUNNING) {
			rc = -EIO;
			goto err;
		}

		rc = dev->start(dev, start);
		if (!rc)
			dev->flags |= MCLOCK_FLAGS_RUNNING;

		mclock_timer_start_all(dev, start);

	} else {
		rc = -EINVAL;
	}

err:
	return rc;
}

int mclock_stop(struct mclock_dev *dev)
{
	int rc;

	mclock_timer_stop_all(dev);

	if (dev->stop) {
		if (!(dev->flags & MCLOCK_FLAGS_RUNNING)) {
			rc = -EIO;
			goto err;
		}

		rc = dev->stop(dev);

		dev->flags &= ~MCLOCK_FLAGS_RUNNING;
	} else {
		rc = -EINVAL;
	}

err:
	return rc;
}

int mclock_clean(struct mclock_dev *dev, struct mclock_clean *clean)
{
	int rc;

	if (dev->clean) {
		rc = dev->clean(dev, clean);
	} else {
		rc = -EINVAL;
	}

	return rc;
}

int mclock_reset(struct mclock_dev *dev)
{
	int rc;

	if (dev->reset) {
		if (dev->flags & MCLOCK_FLAGS_RUNNING) {
			rc = -EIO;
			goto err;
		}

		rc = dev->reset(dev);

	} else {
		rc = -EINVAL;
	}

err:
	return rc;
}

void mclock_gconfig(struct mclock_dev *dev, struct mclock_gconfig *cfg)
{
	cfg->ts_src = dev->ts_src;
	cfg->ts_freq_p = dev->ts_freq_p;
	cfg->ts_freq_q = dev->ts_freq_q;
	cfg->array_size = dev->num_ts;
	cfg->timer_period = dev->timer_period;
	cfg->mmap_size = dev->mmap_size;
}

int mclock_sconfig(struct mclock_dev *dev, struct mclock_sconfig *cfg)
{
	int rc;

	if (dev->config)
		rc = dev->config(dev, cfg);
	else
		rc = -EINVAL;

	return rc;
}

int mclock_open(struct mclock_dev *dev, int port)
{
	int rc;

	if (dev->open)
		rc = dev->open(dev, port);
	else
		rc = -EINVAL;

	return rc;
}

void mclock_release(struct mclock_dev *dev)
{
	mclock_stop(dev);

	if (dev->release)
		dev->release(dev);
}
