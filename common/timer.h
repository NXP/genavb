/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Timer Service implementation
 @details
*/
#ifndef _COMMON_TIMER_H_
#define _COMMON_TIMER_H_

#include "common/list.h"
#include "os/timer.h"

#include "config.h"

/* software timers flags */
#define TIMER_TYPE_SYS		(1 << 0)
#define TIMER_STATE_CREATED	(1 << 1)
#define TIMER_STATE_STARTED	(1 << 2)

#define NS_PER_MS 	(1000*1000)
#define MS_PER_S	(1000)

struct timer_sys {
	struct list_head head;
	struct list_head *next_to_process;
	unsigned int ms;
	unsigned int flags;
	unsigned int users;
	struct timer_ctx *ctx;
	struct os_timer os_timer; /* OS dependant fields */
};


struct timer {
	struct list_head list;
	int ms;
	unsigned int flags;
	struct timer_sys *timer_sys;
	void (*func)(void *);
	void *data;
};

struct timer_ctx {
	unsigned short max_sys_timers;
	unsigned short num_soft_timers;
	unsigned short num_sys_timers;
	unsigned long priv;

	/* variable size array */
	struct timer_sys timer_sys_table[]; /* contains system timers only , either shared or exclusively used */
};

#define timer_create timer_init
int timer_init(struct timer_ctx *tctx, struct timer *t, unsigned int flags, unsigned int ms);
int timer_start(struct timer *t, unsigned int ms);
int timer_restart(struct timer *t, unsigned int ms);
void timer_stop(struct timer *t);
int timer_is_running(struct timer *t);
int timer_destroy(struct timer *t);
unsigned int timer_pool_size(unsigned int n);
int timer_pool_init(struct timer_ctx *tctx, unsigned int n, unsigned long priv);
void timer_pool_exit(struct timer_ctx *tctx);


#endif /* _COMMON_TIMER_H_ */
