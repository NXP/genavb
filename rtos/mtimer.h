/*
 * Copyright 2019-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVB media clock timer driver
*/
#ifndef _MTIMER_H_
#define _MTIMER_H_

#include "media_clock.h"

struct mtimer_start {
	unsigned int p; /* in Hz */
	unsigned int q; /* in Hz */
};

#define MTIMER_MAX	4

#define MTIMER_FLAGS_STARTED_EXT		(1 << 0)
#define MTIMER_FLAGS_STARTED_INT		(1 << 1)

struct mtimer_dev {
	mclock_t type;
	unsigned int domain;

	void *mclock_dev;

	struct slist_node list;
	unsigned int flags;
	struct rational wake_up_next;
	struct rational wake_up_per;

	void (*cb)(void *);
	void *cb_data;
};

void mtimer_wake_up_init(struct mtimer_dev *dev, unsigned int now);
void mtimer_wake_up_init_now(struct mtimer_dev *dev, unsigned int now);
void mtimer_wake_up(struct mtimer_dev *dev, unsigned int now);

#ifdef CONFIG_AVTP
int mtimer_start(struct mtimer_dev *dev, struct mtimer_start *start, unsigned int time_now,
		bool internal, bool is_media_clock_running);
int mtimer_stop(struct mtimer_dev *dev, bool internal);
struct mtimer_dev *mtimer_open(mclock_t type, unsigned int domain);
void mtimer_release(struct mtimer_dev *dev);
void mtimer_set_callback(struct mtimer_dev *dev, void (*cb)(void *), void *data);
#else
static inline int mtimer_start(struct mtimer_dev *dev, struct mtimer_start *start, unsigned int time_now,
		bool internal, bool is_media_clock_running)
{
	return -1;
}
static inline int mtimer_stop(struct mtimer_dev *dev, bool internal)
{
	return -1;
}
static inline struct mtimer_dev *mtimer_open(mclock_t type, unsigned int domain)
{
	return NULL;
}
static inline void mtimer_release(struct mtimer_dev *dev)
{
	return;
}
static inline void mtimer_set_callback(struct mtimer_dev *dev, void (*cb)(void *), void *data)
{
	return;
}
#endif /* CONFIG_AVTP */


#endif /* _MTIMER_H_ */
