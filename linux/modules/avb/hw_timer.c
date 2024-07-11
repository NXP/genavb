/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2020, 2022-2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/kthread.h>

#include "net_tx.h"
#include "net_rx.h"
#include "net_socket.h"
#include "media_clock.h"
#include "avbdrv.h"
#include "debugfs.h"
#include "hw_timer.h"


/**
 * DOC: HW timer
 *
 * HW timer generic layer used to register lower-level HW timers.
 *
 * The interrupt drives several processes:
 * - Transmit QoS scheduler
 * - Receive batching flush
 * - GPTP based media clock generation
 */

struct hw_timer *gtimer = NULL;

static irqreturn_t hw_timer_interrupt(int irq, void *dev_id)
{
	struct hw_timer *timer = dev_id;
	struct hw_timer_dev *dev = timer->dev;
	struct avb_drv *avb = container_of(timer, struct avb_drv, timer);
	irqreturn_t rc = IRQ_HANDLED;
	unsigned int ptp_now[CFG_PORTS];
	struct eth_avb *eth;
	int wake = 0;
	int i;
	unsigned int ticks;
	unsigned int cycles, dcycles;
	unsigned int bin;
	unsigned long flags;

	ticks = dev->irq_ack(dev, &cycles, &dcycles);
	stats_update(&timer->delay_stats, dcycles);

	bin = hw_timer_cycles_to_ns(dev, dcycles) >> HW_TIMER_DELAY_BIN_WIDTH_NS_SHIFT;
	if (bin >= HW_TIMER_DELAY_MAX_BINS)
		bin = HW_TIMER_DELAY_MAX_BINS - 1;

	dev->delay_histogram[bin]++;

	if (ticks >= HW_TIMER_MAX_BINS)
		dev->tick_histogram[HW_TIMER_MAX_BINS - 1]++;
	else
		dev->tick_histogram[ticks]++;

	for (i = 0; i < CFG_PORTS; i++) {
		eth = &avb->eth[i];
		if (eth_avb_read_ptp(eth, &ptp_now[i]) < 0)
			ptp_now[i] = 0;
	}

	/*Update the monotonic time counter*/
	timer->time += ticks * HW_TIMER_PERIOD_NS;

	for (i = 0; i < CFG_PORTS; i++) {
		eth = &avb->eth[i];

		raw_spin_lock_irqsave(&eth->lock, flags);

		if (eth->flags & PORT_FLAGS_ENABLED) {
			/* Cleanup tx ring buffer */
			if (fec_enet_tx_avb(eth->fec_data)) {
				set_bit(ETH_AVB_TX_THREAD_BIT, &timer->scheduled_threads);
				wake = 1;
			}

			if (port_scheduler(eth->qos, ptp_now[i])) {
				set_bit(ETH_AVB_TX_AVAILABLE_THREAD_BIT, &timer->scheduled_threads);
				wake = 1;
			}

			/* Handle rx ring buffer */
			if ((fec_enet_rx_poll_avb(eth->fec_data) & AVB_WAKE_THREAD) || net_rx_batch_any_ready(&avb->net_drv)) {
				set_bit(ETH_AVB_RX_THREAD_BIT, &timer->scheduled_threads);
				wake = 1;
			}
		}

		raw_spin_unlock_irqrestore(&eth->lock, flags);
	}

	if (mclock_drv_interrupt(&avb->mclock_drv, ptp_now, ticks)) {
		set_bit(MCLOCK_THREAD_BIT, &timer->scheduled_threads);
		wake = 1;
	}

	if (wake)
		wake_up_process(timer->kthread);

	dcycles = dev->elapsed(dev, cycles);

	stats_update(&timer->runtime_stats, dcycles);

	return rc;
}

static int hw_timer_kthread(void *data)
{
	struct hw_timer *timer = data;
	struct avb_drv *avb = container_of(timer, struct avb_drv, timer);
	int i;

	set_current_state(TASK_INTERRUPTIBLE);

	while (1) {
		schedule();

		if (kthread_should_stop())
			break;

		set_current_state(TASK_INTERRUPTIBLE);

		if (test_and_clear_bit(MCLOCK_THREAD_BIT, &timer->scheduled_threads))
			mclock_drv_thread(&avb->mclock_drv);

		if (test_and_clear_bit(ETH_AVB_TX_THREAD_BIT, &timer->scheduled_threads)) {
			for (i = 0; i < CFG_PORTS; i++)
				net_tx_thread(&avb->eth[i]);
		}

		if (test_and_clear_bit(ETH_AVB_RX_THREAD_BIT, &timer->scheduled_threads))
			net_rx_thread(&avb->net_drv);

		if (test_and_clear_bit(ETH_AVB_TX_AVAILABLE_THREAD_BIT, &timer->scheduled_threads)) {
			for (i = 0; i < CFG_PORTS; i++)
				net_tx_available_thread(&avb->eth[i]);
		}
	}

	return 0;
}

/* Calculate and set mult/shift factors for scaled math when converting cycles to ns*/
static void hw_timer_dev_set_mult_shift(struct hw_timer_dev *dev)
{
	u64 mul;

	dev->cyc2ns_shift = 31;
	mul = (u64)NSEC_PER_SEC << dev->cyc2ns_shift;
	do_div(mul, dev->rate);

	dev->cyc2ns_mul = mul;
}

void hw_timer_start(struct hw_timer *timer)
{
	if (!timer->users) {
		timer->dev->start(timer->dev);
	}
	timer->users++;
}

void hw_timer_stop(struct hw_timer *timer)
{
	if (timer->users) {
		timer->users--;

		if (!timer->users)
			timer->dev->stop(timer->dev);
	}
}

int hw_timer_register_device(struct hw_timer_dev *dev)
{
	int rc = 0;
	struct hw_timer *timer = gtimer;

	mutex_lock(&timer->lock);

	if (timer->dev) {
		pr_err("%s: a HW timer device is already registered\n", __func__);
		rc = -1;
		goto register_err;
	}

	timer->kthread = kthread_run(hw_timer_kthread, timer, "avb timer");
	if (IS_ERR(timer->kthread)) {
		pr_err("%s: kthread_create() failed\n", __func__);
		rc = -1;
		goto kthread_err;
	}

	rc = request_irq(dev->irq, hw_timer_interrupt, IRQF_NO_THREAD, dev->pdev->name, timer);
	if (rc < 0) {
		pr_err("%s: request_threaded_irq() failed\n", __func__);
		goto irq_err;
	}

	timer->dev = dev;

	hw_timer_dev_set_mult_shift(dev);

	pr_info("%s: dev(%p) (%s), counter clock %lu Hz, min delay cycles %u, mul %llu, shift %u\n",
		__func__, dev, dev->pdev->name, dev->rate, dev->min_delay_cycles, dev->cyc2ns_mul, dev->cyc2ns_shift);

	/* Start timer */
	dev->set_period(dev, timer->period);
	hw_timer_start(timer);

	mutex_unlock(&timer->lock);

	return 0;

irq_err:
	kthread_stop(timer->kthread);

kthread_err:
register_err:
	mutex_unlock(&timer->lock);
	return rc;
}

void hw_timer_unregister_device(struct hw_timer_dev *dev)
{
	struct hw_timer *timer = gtimer;

	mutex_lock(&timer->lock);

	if (timer->dev == dev) {
		hw_timer_stop(timer);
		if (timer->users)
			pr_warn("Warning, users are still pending while"
			"timer device (%p) is unregistered\n", dev);

		free_irq(dev->irq, timer);
		kthread_stop(timer->kthread);

		timer->dev = NULL;
	}

	mutex_unlock(&timer->lock);
}

int hw_timer_init(struct hw_timer *timer, unsigned long period_us, struct dentry *avb_dentry)
{
	gtimer = timer;

	mutex_init(&timer->lock);
	timer->period = period_us;
	timer->dev = NULL;

	stats_init(&timer->runtime_stats, 31);
	stats_init(&timer->delay_stats, 31);

	hw_timer_debugfs_init(timer, avb_dentry);

	return 0;
}

void hw_timer_exit(struct hw_timer *timer)
{
	gtimer = NULL;
}
