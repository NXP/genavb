/*
 * Copyright 2017, 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __STREAM_STATS_H__
#define __STREAM_STATS_H__

#include "avb_stream_config.h"
#include "stats.h"

#define APP_FLAG_FIRST_HANDLE	(1 << 0)

struct stream_stats {
	unsigned long app_ptr;          /**< AVB route data pointer value, just for printout */
	unsigned long avb_stream_ptr;     /**< AVB stream pointer value, just for printout */
	int is_listener;  /**< This is statistic for listener route or not */

	unsigned int period;
	unsigned int flags;
	unsigned int last_stat_dump_time;

	char is_updated_flag;   /**< Flag indicate statistic is updated by data thread */
	aar_avb_stats_t avb_stats;   /**< AVB statistic structure */
};


void stream_stats_dump(struct stream_stats *stats);
int stream_stats_store(struct stream_stats *stats, aar_avb_stats_t *avb_stats);
int stream_stats_is_time(struct stream_stats *stats);
void stream_stats_set_time(struct stream_stats *stats);


static inline int stream_stats_is_updated(struct stream_stats *stats)
{
	return stats->is_updated_flag;
}

static inline void stream_stats_set_updated(struct stream_stats *stats)
{
	stats->is_updated_flag = 1;
}


#endif /* __STREAM_STATS_H__ */
