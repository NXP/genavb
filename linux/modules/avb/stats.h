/*
 * AVB stats helpers
 * Copyright 2017, 2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _STATS_H_
#define _STATS_H_

#include <linux/kernel.h>

struct stats {
	u32 log2_size;
	u32 current_count;

	s32 current_min;
	s32 current_max;
	s64 current_mean;
	u64 current_ms;

	/* Stats snapshot */
	s32 min;
	s32 max;
	s32 mean;
	u64 ms;
	u64 variance;

	/* absolute min/max (never reset) */
	s32 abs_min;
	s32 abs_max;
};

void stats_reset(struct stats *s);
void stats_update(struct stats *s, int val);
void stats_compute(struct stats *s);


/** stats_init() - Initialize a stats structure.
 * @s: 			Pointer to structure to be initialized
 * @log2_size:	Set size to be reached before statistics are computed, expressed as a power of 2
 *
 */
static inline void stats_init(struct stats *s, unsigned int log2_size)
{
	s->log2_size = log2_size;

	s->abs_min = 0x7fffffff;
	s->abs_max = -0x7fffffff;

	stats_reset(s);
}


#endif /* _STATS_H_ */
