/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2020, 2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _HW_TIMER_H_
#define _HW_TIMER_H_

#ifdef __KERNEL__

#include "stats.h"

#define MCLOCK_THREAD_BIT		(1 << 0)
#define ETH_AVB_RX_THREAD_BIT		(1 << 1)
#define ETH_AVB_TX_THREAD_BIT		(1 << 2)
#define ETH_AVB_TX_AVAILABLE_THREAD_BIT	(1 << 3)

struct hw_timer_dev;

struct hw_timer {
	unsigned long period; //in us
	struct task_struct *kthread;
	struct hw_timer_dev *dev;
	struct mutex lock;
	unsigned long scheduled_threads;
	unsigned users;

	unsigned int time; /*Should be accessed only from hw timer interrupt context*/

	struct stats runtime_stats;
	struct stats delay_stats;
};

#define HW_TIMER_MAX_BINS	5
#define HW_TIMER_DELAY_MAX_BINS	16
#define HW_TIMER_DELAY_BIN_WIDTH_NS_SHIFT	13 /* Determines the delay bin width in ns : 2^13 = 8192 ns */
#define HW_TIMER_MIN_DELAY_US	10 /* Minimum delay between compare value and now (in microseconds) */

struct hw_timer_dev {
	struct platform_device *pdev;
	unsigned int (*irq_ack)(struct hw_timer_dev *, unsigned int *, unsigned int *);
	unsigned int (*elapsed)(struct hw_timer_dev *, unsigned int);
	void (*start)(struct hw_timer_dev *);
	void (*stop)(struct hw_timer_dev *);
	void (*set_period)(struct hw_timer_dev *, unsigned long period_us);
	int irq;
	unsigned int period; /* in cycles */
	unsigned long rate; /* in cycles per second */
	unsigned int cyc2ns_shift;
	u64 cyc2ns_mul;

	unsigned int min_delay_cycles;

	unsigned int tick_histogram[HW_TIMER_MAX_BINS];
	unsigned int delay_histogram[HW_TIMER_DELAY_MAX_BINS];
	unsigned int recovery_errors;
};

int hw_timer_register_device(struct hw_timer_dev *);
void hw_timer_unregister_device(struct hw_timer_dev *);

int hw_timer_init(struct hw_timer *timer, unsigned long period_us, struct dentry *avb_dentry);
void hw_timer_exit(struct hw_timer *timer);

static inline unsigned int hw_timer_cycles_to_ns(struct hw_timer_dev *dev, unsigned int cycles)
{
	return (((u64)cycles) * dev->cyc2ns_mul) >> dev->cyc2ns_shift;
}

#endif /* __KERNEL__ */

#define HW_TIMER_PERIOD_US	125
#define HW_TIMER_PERIOD_NS	(HW_TIMER_PERIOD_US * 1000)

#endif /* _HW_TIMER_H_ */
