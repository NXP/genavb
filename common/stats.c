/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Statistics service implementation
 @details
*/

#include "stats.h"
#include "common/log.h"

void stats_reset(struct stats *s)
{
	s->current_count = 0;
	s->current_min = 0x7fffffff;
	s->current_mean = 0;
	s->current_max = -0x7fffffff;
	s->current_ms = 0;
}

/** Example function to be passed to stats_init
 * This function expects the priv field to be a character string.
 * Usage:
 * stats_init(s, log2_size, "your string", print_stats);
 */
void stats_print(struct stats *s)
{
	os_log(LOG_INFO, "stats(%p) %s min %d mean %d max %d rms^2 %"PRIu64" stddev^2 %"PRIu64"\n", s, (char *)s->priv, s->min, s->mean, s->max, s->ms, s->variance);
}


/** Update stats with a given sample.
 * @s: handler for the stats being monitored
 * @val: sample to be added
 *
 * This function adds a sample to the set.
 * If the number of accumulated samples is equal to the requested size of the set, the following indicators will be computed:
 *    . minimum observed value,
 *    . maximum observed value,
 *    . mean value,
 *    . square of the RMS (i.e. mean of the squares)
 *    . square of the standard deviation (i.e. variance)
 */
void stats_update(struct stats *s, s32 val)
{
	s->current_count++;

	s->current_mean += val;
	s->current_ms += (s64)val * val;

	if (val < s->current_min) {
		s->current_min = val;
		if (val < s->abs_min)
			s->abs_min = val;
	}

	if (val > s->current_max) {
		s->current_max = val;
		if (val > s->abs_max)
			s->abs_max = val;
	}


	if (s->current_count == (1U << s->log2_size)) {
		s->ms = s->current_ms >> s->log2_size;
		s->variance = s->ms - ((s->current_mean * s->current_mean) >> (2*s->log2_size));
		s->mean = s->current_mean >> s->log2_size;

		s->min = s->current_min;
		s->max = s->current_max;

		if (s->func)
			s->func(s);

		stats_reset(s);
	}
}


/** Compute current stats event if set size hasn't been reached yet.
 * @s: handler for the stats being monitored
 *
 * This function computes current statistics for the stats:
 *    . minimum observed value,
 *    . maximum observed value,
 *    . mean value,
 *    . square of the RMS (i.e. mean of the squares)
 *    . square of the standard deviation (i.e. variance)
 */
void stats_compute(struct stats *s)
{
	if (s->current_count) {
		s->ms = s->current_ms / s->current_count;
		s->variance = s->ms - (s->current_mean * s->current_mean) / ((s64)s->current_count * s->current_count);
		s->mean = s->current_mean / s->current_count;
	} else {
		s->mean = 0;
		s->ms = 0;
		s->variance = 0;
	}

	s->min = s->current_min;
	s->max = s->current_max;
}
