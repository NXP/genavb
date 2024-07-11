/*
 * Copyright 2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _MTIMER_H_
#define _MTIMER_H_

#include "genavb/types.h"

struct mtimer_start {
	unsigned int p; /* in Hz */
	unsigned int q; /* in Hz */
};

#ifdef __KERNEL__

#include <linux/wait.h>
#include "rational.h"

#define MTIMER_MAX	16

#define MTIMER_FLAGS_STARTED_EXT	(1 << 0)
#define MTIMER_FLAGS_STARTED_INT	(1 << 1)

#define MTIMER_ATOMIC_FLAGS_BUSY	(1 << 0)

struct mtimer_dev {
	mclock_t type;
	unsigned int domain;

	void *mclock_dev;

	struct list_head list;
	raw_spinlock_t lock;
	unsigned int flags;
	unsigned long atomic_flags;
	wait_queue_head_t wait;
	atomic_t wake_up;
	struct rational wake_up_next;
	struct rational wake_up_per;
};

void mtimer_wake_up_init(struct mtimer_dev *dev, unsigned int now);
void mtimer_wake_up_init_now(struct mtimer_dev *dev, unsigned int now);
void mtimer_wake_up_thread(struct mtimer_dev *dev, struct mtimer_dev **mtimer_dev_array, unsigned int *n);
int mtimer_wake_up(struct mtimer_dev *dev, unsigned int now);

int mtimer_start(struct mtimer_dev *dev, struct mtimer_start *start, unsigned int time_now,
		bool internal, bool is_media_clock_running);
int mtimer_stop(struct mtimer_dev *dev, bool internal);

struct mtimer_dev *mtimer_open(mclock_t type, unsigned int domain);
void mtimer_release(struct mtimer_dev *dev);

#endif /* __KERNEL__ */


#endif /* _MTIMER_H_ */
