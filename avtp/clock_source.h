/*
 * Copyright 2014 Freescale Semiconductor, Inc.
 * Copyright 2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Clock source implementation
 @details
*/
#ifndef _CLOCK_SOURCE_H_
#define _CLOCK_SOURCE_H_

#include "os/timer.h"

#include "clock_grid.h"

/* The source grid should have enough valid timestamps to cover the needs of all its consumers.
 * Consumers read timestamp in batches, so we need to determine the longest batch. This is
 * the same as the longest wakeup period.
 */
#define CLOCK_SOURCE_MIN_VALID_TIME_US	(2 * (CFG_AVTP_MAX_LATENCY / 1000))


struct clock_scheduling_params {
	unsigned int wake_freq_p;
	unsigned int wake_freq_q;
};

#define CLOCK_SOURCE_FLAGS_READY	(1 << 0)
#define CLOCK_SOURCE_FLAGS_FIRST_UPDATE (1 << 1)	/* tracks the first update after start */

struct clock_source {
	unsigned int flags;
	u32 extra_count;	/* extra timestamps added to source grid on first update after start */
	u32 min_valid_count;	/* minimum amount of timestamps to satisfy all possible consumers */

	struct clock_scheduling_params sched_params[CFG_SR_CLASS_MAX];
	struct os_timer timer[CFG_SR_CLASS_MAX];

	os_clock_id_t clock_id;

	unsigned int tick_period;

	struct clock_grid grid;
};


int clock_source_init(struct clock_source *source, clock_grid_producer_type_t type, int id, unsigned long priv);
int clock_source_ready(struct clock_source *source);
void clock_source_exit(struct clock_source *source);
int clock_source_open(struct clock_source *source, void *data);
void clock_source_close(struct clock_source *source);


#endif /* _CLOCK_SOURCE_H_ */
