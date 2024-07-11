/*
 * Copyright 2019, 2021-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVB media clock generation driver
*/

#include "common/log.h"

#include "media_clock_gen_ptp.h"
#include "avtp.h"

static void mclock_gen_ptp_timer_reset(struct mclock_dev *dev, unsigned int ptp_now)
{
	struct mclock_gen_ptp *clk = container_of(dev, struct mclock_gen_ptp, dev);

	clk->clk_ptp = dev->clk_timer;
	clk->ts_next = ptp_now;
	clk->ts_corr = 0;
	clk->ts_period_n = 0;
	dev->next_drift = clk->clk_ptp + dev->drift_period;
	clk->stats.reset++;
}

/**
 * mclock_gen_ptp_timer_irq() - Clock generation interrupt handler
 * @dev: pointer to clock device
 * @data: data pointer (ptp_now array)
 *
 * This is the main PTP clock generation function,
 * it generates timestamps synchronous to the gPTP clock.
 * It is called by the EPIT interrupt handler at a 125 us period.
 * The 125 us period is used as the base counter to generate the timestamps.
 * This counter is then adapated to follow the gPTP clock.
 *
 * Return : 	1 if driver should wake-up the user on the wait queue,
 *		0 otherwise
 */
static int mclock_gen_ptp_timer_irq(struct mclock_dev *dev, void *data, unsigned int ticks)
{
	struct mclock_gen_ptp *clk = container_of(dev, struct mclock_gen_ptp, dev);
	unsigned int w_idx;
	unsigned int ptp_now, delta;
	int rc = 0;

	ptp_now = ((unsigned int *)data)[dev->eth->index];

	/* IRQ */
	dev->clk_timer += ticks * dev->timer_period;

	if (dev->flags & MCLOCK_FLAGS_INIT) {

		if (dev->flags & MCLOCK_FLAGS_WAKE_UP)
			mclock_wake_up_init(dev, dev->clk_timer);

		mclock_gen_ptp_timer_reset(dev, ptp_now);

		dev->flags &= ~(MCLOCK_FLAGS_INIT);
	} else {
		delta = ptp_now - *dev->ptp;

		/* Basic sanity check which can early catch big PTP discontinuities */
		if (abs((int)(delta - ticks * dev->timer_period)) > (3 * dev->timer_period)) {
			mclock_gen_ptp_timer_reset(dev, ptp_now);
			clk->stats.err_jump++;
		}
		else {
			clk->clk_ptp += delta;
		}
	}

	/* Measure drift with gPTP clock and adapt the ptp counter */
	if (mclock_drift_adapt(dev, clk->clk_ptp) < 0) {
		mclock_gen_ptp_timer_reset(dev, ptp_now);
		clk->stats.err_drift++;
	}

	/* Update shared grid variables */
	*dev->ptp = ptp_now;
	clk->stats.ptp_now = ptp_now;

	/* Generate timestamp */
	if (avtp_after_eq(ptp_now, clk->ts_next)) {
		w_idx = *dev->w_idx;

		*(unsigned int *)((char *)dev->sh_mem + (w_idx * sizeof(unsigned int))) = clk->ts_next;
		clk->ts_next += clk->ts_period;

		clk->ts_corr += clk->ts_period_rem;
		if (clk->ts_corr >= MCLOCK_GEN_TS_FREQ_INIT) {
			clk->ts_next++;
			clk->ts_corr -= MCLOCK_GEN_TS_FREQ_INIT;
		}

		*dev->count = *dev->count + 1;
		*dev->w_idx = (w_idx + 1) & (MCLOCK_GEN_NUM_TS - 1);
	}

	return rc;
}


static int mclock_gen_ptp_start(struct mclock_dev *dev, struct mclock_start *start)
{
	struct mclock_gen_ptp *clk = container_of(dev, struct mclock_gen_ptp, dev);
	int rc = 0;

	dev->flags |= MCLOCK_FLAGS_INIT;
	clk->ts_next = 0;
	clk->ts_corr = 0;
	dev->clk_timer = 0;
	*dev->w_idx = 0;
	*dev->count = 0;
	clk->stats.start++;

	rc = mclock_register_timer(dev, mclock_gen_ptp_timer_irq, dev->timer_period / NET_RX_TX_PERIOD);
	if (rc < 0) {
		os_log(LOG_ERR, "%s : cannot register timer\n", __func__);
		goto exit;
	}

exit:
	return rc;
}

static int mclock_gen_ptp_stop(struct mclock_dev *dev)
{
	struct mclock_gen_ptp *clk = container_of(dev, struct mclock_gen_ptp, dev);

	clk->stats.stop++;

	mclock_unregister_timer(dev);

	return 0;
}

static int mclock_gen_ptp_reset(struct mclock_dev *dev)
{
	return 0;
}

static int mclock_gen_ptp_open(struct mclock_dev *dev, int port)
{
	struct mclock_gen_ptp *clk = container_of(dev, struct mclock_gen_ptp, dev);
	int rc = 0;

	dev->eth = &ports[port];
	if (!dev->eth) {
		rc = -1;
		goto exit;
	}

	dev->w_idx = (unsigned int *)((char *)dev->sh_mem + MCLOCK_GEN_BUF_SIZE);
	dev->ptp = dev->w_idx + 1;
	dev->count = dev->w_idx + 2;
	dev->mmap_size = MCLOCK_GEN_MMAP_SIZE;

	mclock_set_ts_freq(dev, MCLOCK_GEN_TS_FREQ_INIT, 1);

	// FIXME clean this and use common ts period rational
	clk->ts_period = NSECS_PER_SEC / MCLOCK_GEN_TS_FREQ_INIT;
	clk->ts_period_rem = NSECS_PER_SEC - (clk->ts_period * MCLOCK_GEN_TS_FREQ_INIT);
	dev->drift_period = mclock_drift_period(dev);

	/* Stats */
	memset(&clk->stats, 0, sizeof(struct mclock_gen_ptp_stats));
	clk->stats.ts_period = clk->ts_period;

exit:
	return rc;
}

static void mclock_gen_ptp_release(struct mclock_dev *dev)
{
}

int mclock_gen_ptp_init(struct mclock_gen_ptp *ptp)
{
	struct mclock_dev *dev = &ptp->dev;
	int rc = 0;

	dev->type = PTP;
	dev->start = mclock_gen_ptp_start;
	dev->stop = mclock_gen_ptp_stop;
	dev->clean = NULL;
	dev->reset = mclock_gen_ptp_reset;
	dev->open = mclock_gen_ptp_open;
	dev->release = mclock_gen_ptp_release;

	dev->timer_period = NET_RX_TX_PERIOD;
	dev->num_ts = MCLOCK_GEN_NUM_TS;

	dev->sh_mem = rtos_malloc(MCLOCK_GEN_MMAP_SIZE);
	if (!dev->sh_mem) {
		rc = -1;
		os_log(LOG_ERR, "rtos_malloc failed\n");
		goto exit;
	}

	mclock_register_device(dev);

	os_log(LOG_INIT, "registered gen_ptp device %p\n", dev);
exit:
	return rc;
}

void mclock_gen_ptp_exit(struct mclock_gen_ptp *ptp)
{
	struct mclock_dev *dev = &ptp->dev;

	mclock_unregister_device(dev);

	rtos_free(dev->sh_mem);
}
