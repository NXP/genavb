/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2018, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Statistics service implementation
 @details
*/

#ifndef _COMMON_STATS_H_
#define _COMMON_STATS_H_

#include "common/types.h"

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

	void *priv;
	void (*func)(struct stats *s);
};

void stats_reset(struct stats *s);
void stats_print(struct stats *s);
void stats_update(struct stats *s, s32 val);
void stats_compute(struct stats *s);


/** Initialize a stats structure.
 * @s: 			Pointer to structure to be initialized
 * @log2_size:	Set size to be reached before statistics are computed, expressed as a power of 2
 * @priv:		private field for use by func
 * @func:		pointer to the function to be called when stats are computed
 *
 */
static inline void stats_init(struct stats *s, unsigned int log2_size, void *priv, void (*func)(struct stats *s))
{
	s->log2_size = log2_size;
	s->priv = priv;
	s->func = func;

	s->abs_min = 0x7fffffff;
	s->abs_max = -0x7fffffff;

	stats_reset(s);
}


#endif /* _COMMON_STATS_H_ */
