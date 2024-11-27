/*
 * Copyright 2017-2018, 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <time.h>
#include <sys/epoll.h>

#include "log.h"
#include "stream_stats.h"
#include "stats.h"


void stream_stats_dump(struct stream_stats *stats)
{
	aar_avb_counter_stats_t *avb_counter_stats = &stats->avb_stats.counter_stats;

	INF_LOG("avb_route(%p) statistic", (void *)stats->app_ptr);

	// AVB counter stats
	INF_LOG("   avb(%p) tx_err: %d, rx_err: %d, batch_tx: %d, batch_rx: %d",
			(void *)stats->avb_stream_ptr, avb_counter_stats->tx_err,
			avb_counter_stats->rx_err, avb_counter_stats->batch_tx,
			avb_counter_stats->batch_rx);

	stats_compute(&stats->avb_stats.gptp_2cont_wakeup);
	// Print stats
	INF_LOG("   avb(%p) inter wakeup time (ns): %d/%d/%d",
			(void *)stats->avb_stream_ptr, stats->avb_stats.gptp_2cont_wakeup.min,
			stats->avb_stats.gptp_2cont_wakeup.mean,
			stats->avb_stats.gptp_2cont_wakeup.max);

	// Check if listener side
	if (stats->is_listener) {
		stats_compute(&stats->avb_stats.event_2cont_wakeup);
		stats_compute(&stats->avb_stats.event_gptp);

		INF_LOG("   avb(%p) listener inter event_ts (ns): %d/%d/%d",
				(void *)stats->avb_stream_ptr,
				stats->avb_stats.event_2cont_wakeup.min,
				stats->avb_stats.event_2cont_wakeup.mean,
				stats->avb_stats.event_2cont_wakeup.max);
		INF_LOG("   avb(%p) listener event_ts-now (ns): %d/%d/%d",
				(void *)stats->avb_stream_ptr,
				stats->avb_stats.event_gptp.min,
				stats->avb_stats.event_gptp.mean,
				stats->avb_stats.event_gptp.max);
	}
}


/* Must be called last, after updating the specific stats first. */
int stream_stats_store(struct stream_stats *stats, aar_avb_stats_t *avb_stats)
{
	memcpy(&stats->avb_stats, avb_stats, sizeof(aar_avb_stats_t));

	stats_reset(&avb_stats->gptp_2cont_wakeup);
	stats_reset(&avb_stats->event_2cont_wakeup);
	stats_reset(&avb_stats->event_gptp);

	stream_stats_set_updated(stats);

	// Let data thread knows that stat is snapshoot ok
	stream_stats_set_time(stats);

	return 0;
}

int stream_stats_is_time(struct stream_stats *stats)
{
	struct timespec ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0)
		return 0;

	// Dump statistic after each 10s
	if (stats->flags & APP_FLAG_FIRST_HANDLE) {
		// First handle, just store the time stamp
		stats->last_stat_dump_time = ts.tv_sec;

		stats->flags &= ~APP_FLAG_FIRST_HANDLE;

		return 0;
	} else if ((ts.tv_sec - stats->last_stat_dump_time) < stats->period) {
		return 0;
	}

	return 1;
}

void stream_stats_set_time(struct stream_stats *stats)
{
	struct timespec ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0)
		return;

	stats->last_stat_dump_time = ts.tv_sec;
}
