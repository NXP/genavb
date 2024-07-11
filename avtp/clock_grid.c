/*
 * Copyright 2016, 2018-2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Clock grid implementation
 @details
*/
#include "os/stdlib.h"
#include "os/string.h"
#include "os/clock.h"

#include "common/log.h"

#include "clock_grid.h"
#include "media_clock.h"

const char *clock_grid_producer_type2string(clock_grid_producer_type_t type)
{
	switch (type) {
		case2str(GRID_PRODUCER_MULT);
		case2str(GRID_PRODUCER_PTP);
		case2str(GRID_PRODUCER_STREAM);
		case2str(GRID_PRODUCER_HW);
		case2str(GRID_PRODUCER_NONE);
	default:
		return (char *)"Unknown clock grid producer type";
	}
}

void clock_grid_stats_print(struct ipc_avtp_clock_grid_stats *msg)
{
	struct clock_grid_stats *stats = &msg->stats;

	stats_compute(&stats->period);

	os_log(LOG_INFO, "domain(%p) clock_grid(%p) %s err_period: %10u period: %9d /%9d /%9d\n",
	       msg->domain, msg->grid, clock_grid_producer_type2string(msg->type), stats->err_period,
	       stats->period.min, stats->period.mean, stats->period.max);
}

void clock_grid_stats_dump(struct clock_grid *grid, struct ipc_tx *tx)
{
	struct ipc_desc *desc;
	struct ipc_avtp_clock_grid_stats *msg;

	desc = ipc_alloc(tx, sizeof(*msg));
	if (!desc)
		goto err_ipc_alloc;

	desc->type = IPC_AVTP_CLOCK_GRID_STATS;
	desc->len = sizeof(*msg);
	desc->flags = 0;

	msg = (struct ipc_avtp_clock_grid_stats *)&desc->u;

	msg->grid = grid;
	msg->domain = grid->domain;
	msg->type = grid->producer.type;

	os_memcpy(&msg->stats, &grid->stats, sizeof(grid->stats));

	stats_reset(&grid->stats.period);

	if (ipc_tx(tx, desc) < 0)
		goto err_ipc_tx;

	switch (grid->producer.type) {
	case GRID_PRODUCER_MULT:
		clock_grid_consumer_stats_dump(&grid->producer.u.mult.source, tx);
		break;

	case GRID_PRODUCER_HW:
	case GRID_PRODUCER_PTP:
	case GRID_PRODUCER_STREAM:
		break;

	default:
		os_log(LOG_ERR, "clock_grid(%p) unsupported producer type %d\n", grid, grid->producer.type);
		break;
	}
	return;

err_ipc_tx:
	ipc_free(tx, desc);

err_ipc_alloc:
	return;
}

void clock_grid_update_valid_count(struct clock_grid *grid)
{
	grid->valid_count = min(grid->count, grid->max_valid_count);
}

unsigned int clock_grid_start_count(struct clock_grid *grid)
{
	return min(grid->count, grid->max_start_count);
}

void clock_grid_ts_update(struct clock_grid *grid, unsigned int requested, unsigned int *reset)
{
	if (grid->ts_update) {
		grid->ts_update(grid, requested, reset);
		clock_grid_update_valid_count(grid);
	}
}

struct clock_grid *clock_grid_alloc(void)
{
	struct clock_grid *grid = os_malloc(sizeof(struct clock_grid));

	if (grid)
		os_memset(grid, 0, sizeof(struct clock_grid));

	return grid;
}

void clock_grid_free(struct clock_grid *grid)
{
        os_free(grid);
}

static void clock_grid_release(struct clock_grid *grid)
{
	clock_domain_remove_grid(grid);

	clock_grid_free(grid);
}

int clock_grid_ref(struct clock_grid *grid)
{
	int rc = 0;

	if (grid->ref_count == 0) { //First consumer of the grid, take any required action.
		switch (grid->producer.type) {
		case GRID_PRODUCER_HW:
		case GRID_PRODUCER_PTP:
		case GRID_PRODUCER_MULT:
		case GRID_PRODUCER_STREAM:
			break;

		default:
			os_log(LOG_ERR, "clock_grid(%p) unsupported producer type %d\n", grid, grid->producer.type);
			break;
		}
	}

	if (rc == 0)
		grid->ref_count++;

	if (grid->ref_count == 0) {
		grid->ref_count--;
		os_log(LOG_ERR, "clock_grid(%p) ref_count overflow, cannot use the grid.\n", grid);
		rc = -1;
	}

	return rc;
}


void clock_grid_unref(struct clock_grid *grid)
{
	if (!grid->ref_count) {
		os_log(LOG_ERR, "grid(%p) WARNING: un-referencing a clock grid with ref count 0\n", grid);
		return;
	}

	grid->ref_count--;

	if (grid->ref_count == 0) {
		switch (grid->producer.type) {
		case GRID_PRODUCER_HW:
		case GRID_PRODUCER_PTP:
		case GRID_PRODUCER_MULT:
		case GRID_PRODUCER_STREAM:
			break;

		default:
			os_log(LOG_ERR, "clock_grid(%p) unsupported producer type %d\n", grid, grid->producer.type);
			break;
		}

		/* Clock grid was allocated dynamically and linked to a domain, so get rid of it. */
		if (!(grid->flags & CLOCK_GRID_FLAGS_STATIC)) {
			clock_grid_exit(grid);
			clock_grid_release(grid);
		}
	}
}


int clock_grid_init(struct clock_grid *grid, clock_grid_producer_type_t type, u32 *ring_base, unsigned int ring_size, u32 nominal_freq_p, u32 nominal_freq_q, void (*ts_update)(struct clock_grid *, unsigned int , unsigned int *))
{
	int rc = 0;

	if (!nominal_freq_p) {
		os_log(LOG_ERR, "(%p) null nominal timestamp frequency\n", grid);
		rc = -1;
		goto err;
	}

	grid->ref_count = 0;
	grid->nominal_freq_p = nominal_freq_p;
	grid->nominal_freq_q = nominal_freq_q;
	grid->nominal_period = ((u64)nominal_freq_q * NSECS_PER_SEC) / nominal_freq_p;
	grid->period_jitter = (grid->nominal_period * CLOCK_GRID_VALID_PPM) / 1000000;
	grid->ts = ring_base;
	grid->ring_size = ring_size;
	grid->ts_update = ts_update;

	// FIXME Arbitrary value
	grid->max_valid_count = min(grid->ring_size / 2, grid->nominal_freq_p / (grid->nominal_freq_q * (USECS_PER_SEC / CLOCK_GRID_VALID_TIME_US)));

	// Place the initial consumer read index behind write index by 1/4 of the grid size or CLOCK_GRID_VALID_TIME_US usecs (whichever is smaller).
	grid->max_start_count = min(grid->ring_size / 4, grid->nominal_freq_p / (2 * grid->nominal_freq_q * (USECS_PER_SEC / CLOCK_GRID_VALID_TIME_US)));

	grid->write_index = 0;
	grid->valid_count = 0;
	grid->count = 0;

	grid->stats.err_period = 0;
	stats_init(&grid->stats.period, 31, NULL, NULL);

	os_log(LOG_INFO, "grid(%p) nominal_freq %u/%u nominal_period %u jitter %u\n",
			grid, nominal_freq_p, nominal_freq_q, grid->nominal_period, grid->period_jitter);
err:
	return rc;
}

void clock_grid_exit(struct clock_grid *grid)
{
	switch (grid->producer.type) {
	case GRID_PRODUCER_HW:
	case GRID_PRODUCER_PTP:
		break;

	case GRID_PRODUCER_MULT:
		clock_grid_mult_exit(grid);
		break;

	default:
		os_log(LOG_ERR, "clock_grid(%p) unsupported producer type %d\n", grid, grid->producer.type);
		break;
	}

	grid->producer.type = GRID_PRODUCER_NONE;
}
