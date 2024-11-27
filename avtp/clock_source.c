/*
 * Copyright 2014 Freescale Semiconductor, Inc.
 * Copyright 2019-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Clock source implementation
 @details
*/
#include "os/stdlib.h"
#include "common/log.h"

#include "clock_source.h"
#include "stream.h"


static void clock_source_grid_expand(struct clock_grid *grid, unsigned int start, unsigned int count, unsigned int avail)
{
	unsigned int ts = grid->ts[start];
	unsigned int period;
	int i;

	if (avail > 1) {
		/* Extrapolate timestamps based on actual period */
		period = grid->ts[(start + 1) & (grid->ring_size - 1)] - ts;
		if (os_abs((int)period - (int)grid->nominal_period) > grid->period_jitter)
				period = grid->nominal_period;
	} else {
		/* Extrapolate timestamps based on nominal period */
		period = grid->nominal_period;
	}

	for (i = 0; i < count; i++) {
		start = (start - 1) & (grid->ring_size - 1);
		ts -= period;

		grid->ts[start] = ts;
	}
}

static void clock_source_hw_update(struct clock_grid *grid, unsigned int requested, unsigned int *reset)
{
	struct clock_source *source = container_of(grid, struct clock_source, grid);
	unsigned int w_idx, count, i;
	u32 ts, period, avail;

	os_media_clock_gen_ts_update(&grid->producer.u.hw, &w_idx, &count);

	avail = (w_idx - grid->write_index) & (grid->ring_size - 1);

	if (source->flags & CLOCK_SOURCE_FLAGS_FIRST_UPDATE) {
		if (avail > 0) {

			if (avail < source->min_valid_count) {
				unsigned int extra_count = source->min_valid_count - avail;

				clock_source_grid_expand(grid, grid->write_index, extra_count, avail);

				grid->write_index = (grid->write_index - extra_count) & (grid->ring_size - 1);
				source->extra_count = extra_count;
				avail += extra_count;
			}

			ts = grid->ts[grid->write_index];
			period = grid->ts[(grid->write_index + 1) & (grid->ring_size - 1)] - ts;
			grid->producer.last_ts = ts - period;
			source->flags &= ~CLOCK_SOURCE_FLAGS_FIRST_UPDATE;
		}
	}

	if (requested)
		avail = min(avail, requested);

	/*
	 * Verify there is no discontinuity in hardware generated timestamps.
	 * If one is found, reset the hardware grid.
	 */
	if (avail) {
		i = grid->write_index;
		while (i != w_idx) {
			ts = grid->ts[i];
			period = ts - grid->producer.last_ts;

			/* Signal a clock domain unlock and increase stats if a single timestamp is wrong */
			if (os_abs((int)period - (int)grid->nominal_period) > grid->period_jitter) {
				if (&grid->domain->source->grid == grid)
					clock_domain_clear_state(grid->domain, CLOCK_DOMAIN_STATE_LOCKED);

				grid->stats.err_period++;
			} else {
				stats_update(&grid->stats.period, period);
			}

			i = (i + 1) & (grid->ring_size - 1);
			grid->producer.last_ts = ts;
		}
	}

	grid->write_index = w_idx;
	grid->count = count + source->extra_count;

	if (&grid->domain->source->grid == grid)
		clock_domain_set_state(grid->domain, CLOCK_DOMAIN_STATE_LOCKED);
}

static void clock_source_sched(struct os_timer *t, int count, unsigned int prio)
{
	struct clock_source *source = container_of(t, struct clock_source, timer[prio]);
	struct clock_domain *domain;

	domain = source->grid.domain;
	if (!domain) {
		os_log(LOG_ERR, "invalid domain for source(%p)\n", source);
		return;
	}

	clock_domain_sched(domain, prio);
}

static void clock_source_sched_low(struct os_timer *t, int count)
{
	clock_source_sched(t, count, SR_PRIO_LOW);
}

static void clock_source_sched_high(struct os_timer *t, int count)
{
	clock_source_sched(t, count, SR_PRIO_HIGH);
}

static int clock_source_hw_start(struct clock_source *source)
{
	struct clock_grid *grid = &source->grid;

	if (os_media_clock_gen_start(&grid->producer.u.hw, &grid->write_index) < 0)
		goto err;

	source->flags |= CLOCK_SOURCE_FLAGS_FIRST_UPDATE;
	source->extra_count = 0;

	grid->count = 0;
	grid->valid_count = 0;

	return 0;

err:
	return -1;
}

static void clock_source_hw_stop(struct clock_source *source)
{
	struct clock_grid *grid = &source->grid;

	os_media_clock_gen_stop(&grid->producer.u.hw);
	os_media_clock_gen_reset(&grid->producer.u.hw);
}

static int clock_source_ptp_open(struct clock_source *source)
{
	struct clock_grid *grid = &source->grid;

	if (!(source->flags & CLOCK_SOURCE_FLAGS_READY)) {
		os_log(LOG_ERR, "os_clock_producer not ready for source(%p)\n", source);
		return -1;
	}

	if (clock_source_hw_start(source) < 0) {
		os_log(LOG_ERR, "clock_source_hw_start for source(%p) failed\n", source);
		return -1;
	}

	if (grid->domain->hw_sync)
		media_clock_rec_open_ptp(grid->domain->hw_sync);

	return 0;
}

static void clock_source_ptp_close(struct clock_source *source)
{
	struct clock_grid *grid = &source->grid;

	clock_source_hw_stop(source);

	if (grid->domain->hw_sync)
		media_clock_rec_close_ptp(grid->domain->hw_sync);
}

static int clock_source_hw_open(struct clock_source *source)
{
	if (!(source->flags & CLOCK_SOURCE_FLAGS_READY)) {
		os_log(LOG_ERR, "os_clock_producer not ready for source(%p)\n", source);
		return -1;
	}

	if (clock_source_hw_start(source) < 0) {
		os_log(LOG_ERR, "clock_source_hw_start for source(%p) failed\n", source);
		return -1;
	}

	return 0;
}

static void clock_source_hw_close(struct clock_source *source)
{
	clock_source_hw_stop(source);
}

static int clock_source_stream_open(struct clock_source *source, struct stream_listener *stream)
{
	struct clock_grid *grid = &source->grid;
	unsigned int ts_freq_p;
	unsigned int ts_freq_q;

	if (!grid->domain->hw_sync) {
		os_log(LOG_ERR, "source(%p), domain(%p) has no HW recovery support\n",
			source, source->grid.domain);
		return -1;
	}

	if (!stream)
		return 0;

	ts_freq_p = avdecc_fmt_sample_rate(&stream->format);
	ts_freq_q = avdecc_fmt_samples_per_timestamp(&stream->format, stream->class);

	if (!ts_freq_p || !ts_freq_q) {
		os_log(LOG_ERR, "source(%p) invalid ts_freq: %u/%u\n",
			source, ts_freq_p, ts_freq_q);
		return -1;
	}

	/*
	 * Source is not yet bound to a stream.
	 */
	if (!grid->producer.u.stream.stream) {
		if (clock_producer_stream_open(grid, stream, stream->domain->hw_sync, ts_freq_p, ts_freq_q) < 0) {
			os_log(LOG_ERR, "listener_stream_id(%016"PRIx64") clock_source_stream_open error\n", ntohll(stream->id));
			goto err;
		}
		stream->source = source;
	}
#if 0
	else
		/* Legacy, do not fail is source is already setup on another stream */
		goto err;
#endif
	return 0;
err:
	return -1;
}

static void clock_source_stream_close(struct clock_source *source)
{
	struct clock_grid *grid = &source->grid;

	if (grid->producer.u.stream.stream) {
		grid->producer.u.stream.stream->source = NULL;
		clock_producer_stream_close(grid);
	}
}

int clock_source_open(struct clock_source *source, void *data)
{
	int rc = -1;

	switch (source->grid.producer.type) {
	case GRID_PRODUCER_HW:
		rc = clock_source_hw_open(source);
		break;

	case GRID_PRODUCER_PTP:
		rc = clock_source_ptp_open(source);
		break;

	case GRID_PRODUCER_STREAM:
		rc = clock_source_stream_open(source, data);
		break;

	default:
		break;
	}

	return rc;
}

void clock_source_close(struct clock_source *source)
{
	struct clock_grid *grid = &source->grid;

	switch (grid->producer.type) {
	case GRID_PRODUCER_STREAM:
		clock_source_stream_close(source);

		clock_domain_clear_state(grid->domain, CLOCK_DOMAIN_STATE_LOCKED);
		clock_domain_clear_state(grid->domain, CLOCK_DOMAIN_STATE_FREE_WHEELING);
		break;

	case GRID_PRODUCER_HW:
		clock_source_hw_close(source);

		clock_domain_clear_state(grid->domain, CLOCK_DOMAIN_STATE_LOCKED);
		break;

	case GRID_PRODUCER_PTP:
		clock_source_ptp_close(source);

		clock_domain_clear_state(grid->domain, CLOCK_DOMAIN_STATE_LOCKED);
		break;

	default:
		break;
	}
}

int clock_source_ready(struct clock_source *source)
{
	int rc = -1;

	if (!(source->flags & CLOCK_SOURCE_FLAGS_READY))
		goto exit;

	switch (source->grid.producer.type) {
	case GRID_PRODUCER_STREAM:
	case GRID_PRODUCER_HW:
	case GRID_PRODUCER_PTP:
		rc = 0;
		break;

	default:
		break;
	}

exit:
	return rc;
}

__init static int clock_source_hw_init(struct clock_source *source, clock_grid_producer_type_t type, int id)
{
	struct clock_grid *grid = &source->grid;

	if (os_media_clock_gen_init(&grid->producer.u.hw, id, type == GRID_PRODUCER_HW))
		goto err_hw_init;

	if (clock_grid_init(grid, type, grid->producer.u.hw.array_addr, grid->producer.u.hw.array_size, grid->producer.u.hw.ts_freq_p, grid->producer.u.hw.ts_freq_q, clock_source_hw_update) < 0) {
		os_log(LOG_ERR, "clock_source(%p)\n", source);
		goto err_grid_init;
	}

	source->min_valid_count = min(grid->ring_size / 4, grid->nominal_freq_p / (grid->nominal_freq_q * (USECS_PER_SEC / CLOCK_SOURCE_MIN_VALID_TIME_US)));

	source->tick_period = grid->producer.u.hw.timer_period;

	return 0;

err_grid_init:
	os_media_clock_gen_exit(&grid->producer.u.hw);

err_hw_init:
	return -1;
}

__exit static void clock_source_hw_exit(struct clock_source *source)
{
	clock_grid_exit(&source->grid);

	os_media_clock_gen_exit(&source->grid.producer.u.hw);
}

static const struct {
	clock_grid_producer_type_t type;
	int domain_id;
} clock_id[OS_CLOCK_MAX] = {
	[OS_CLOCK_MEDIA_HW_0] = {
		.type = GRID_PRODUCER_HW,
		.domain_id = 0
	},
	[OS_CLOCK_MEDIA_HW_1] = {
		.type = GRID_PRODUCER_HW,
		.domain_id = 1
	},
	[OS_CLOCK_MEDIA_PTP_0] = {
		.type = GRID_PRODUCER_PTP,
		.domain_id = 0
	},
	[OS_CLOCK_MEDIA_PTP_1] = {
		.type = GRID_PRODUCER_PTP,
		.domain_id = 1
	},
	[OS_CLOCK_MEDIA_REC_0] = {
		.type = GRID_PRODUCER_STREAM,
		.domain_id = 0
	},
	[OS_CLOCK_MEDIA_REC_1] = {
		.type = GRID_PRODUCER_STREAM,
		.domain_id = 1
	},
};

static int clock_source_clock_id(clock_grid_producer_type_t type, int id)
{
	int i;

	for (i = 0; i < OS_CLOCK_MAX; i++)
		if ((clock_id[i].type == type) &&
		    (clock_id[i].domain_id == id))
			return i;

	return -1;
}

__init int clock_source_init(struct clock_source *source, clock_grid_producer_type_t type, int id, unsigned long priv)
{
	int clock_id = clock_source_clock_id(type, id);

	os_memset(source, 0, sizeof(*source));

	source->grid.flags = CLOCK_GRID_FLAGS_STATIC;
	source->grid.producer.type = type;
	source->clock_id = clock_id;

	if (clock_id < 0)
		goto err;

	switch (type) {
	case GRID_PRODUCER_HW:
	case GRID_PRODUCER_PTP:
		if (os_timer_create(&source->timer[SR_PRIO_HIGH], clock_id, 0, clock_source_sched_high, priv) < 0)
			goto err_create_high;

		if (os_timer_create(&source->timer[SR_PRIO_LOW], clock_id, 0, clock_source_sched_low, priv) < 0)
			goto err_create_low;

		if (clock_source_hw_init(source, type, id) < 0)
			goto err_init;

		break;

	case GRID_PRODUCER_STREAM:
		if (os_timer_create(&source->timer[SR_PRIO_HIGH], clock_id, 0, clock_source_sched_high, priv) < 0)
			goto err_create_high;

		if (os_timer_create(&source->timer[SR_PRIO_LOW], clock_id, 0, clock_source_sched_low, priv) < 0)
			goto err_create_low;

		break;

	default:
		os_log(LOG_ERR, "clock_source(%p) type(%d) invalid\n", source, type);
		goto err;
		break;
	}

	source->flags |= CLOCK_SOURCE_FLAGS_READY;

	os_log(LOG_INIT, "clock_source(%p) type: %s, id: %d\n", source, clock_grid_producer_type2string(type), id);

	return 0;

err_init:
	os_timer_destroy(&source->timer[SR_PRIO_LOW]);

err_create_low:
	os_timer_destroy(&source->timer[SR_PRIO_HIGH]);

err_create_high:
err:
	return -1;
}

__exit void clock_source_exit(struct clock_source *source)
{
	if (source->flags & CLOCK_SOURCE_FLAGS_READY) {

		source->flags &= ~CLOCK_SOURCE_FLAGS_READY;

		switch (source->grid.producer.type) {
		case GRID_PRODUCER_HW:
		case GRID_PRODUCER_PTP:
			os_timer_destroy(&source->timer[SR_PRIO_HIGH]);
			os_timer_destroy(&source->timer[SR_PRIO_LOW]);

			clock_source_hw_exit(source);
			break;

		case GRID_PRODUCER_STREAM:
			os_timer_destroy(&source->timer[SR_PRIO_HIGH]);
			os_timer_destroy(&source->timer[SR_PRIO_LOW]);
			break;

		default:
			break;
		}
	}
}
