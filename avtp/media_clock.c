/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Media clock interface handling
 @details
*/
#include "os/stdlib.h"
#include "os/clock.h"

#include "common/log.h"

#include "media_clock.h"
#include "clock_domain.h"
#include "avtp.h"
#include "stream.h"

/*
 * ENET compare value restriction
 * increment  <=   compare value  <=  (ENET_ATPER[PERIOD] - increment)
 */
static inline u32 ts_wa(u32 ts)
{
	if (ts < 0x8)
		return  0x8;
	else if (ts > 0xfffffff7)
		return 0xfffffff7;
	else
		return ts;
}


/** Writes a timestamp into the array shared with the lower-level recovery layer.
 * \return	 none
 * \param rec	pointer to media_clock_rec context
 * \param ts	timestamp
 */
static void media_clock_rec_write_ts(struct media_clock_rec *rec, u32 ts)
{
	unsigned int w_idx = *rec->write_idx;

	*(rec->array_addr + w_idx) = ts_wa(ts);

	w_idx = (w_idx + 1) & (rec->array_size - 1);

	/* This allows to stop ENET generation in case no more timestamps are received */
	*(rec->array_addr + w_idx) = 0;

	*rec->write_idx = w_idx;

	rec->nb_ts_total++;
	rec->nb_pending++;
}


/** Gets the recovery status and the number of timestamps processed by the lower-level recovery layer.
 * When using DMA based recovery, it cleans the DMA descriptors.
 * This function updates the media_clock_rec nb_clean_total, nb_pending and clean_idx
 * fields.
 * \return	os_media_clock_rec_state_t media clock recovery driver state
 * \param rec	pointer to media_clock_rec context
 */
static os_media_clock_rec_state_t media_clock_rec_clean(struct media_clock_rec *rec)
{
	unsigned int nb_clean;
	os_media_clock_rec_state_t rc;

	rc = os_media_clock_rec_clean(&rec->os, &nb_clean);
	if (rc < 0) {
		os_log(LOG_ERR, "clock (%p): os_media_clock_rec_clean failed\n", rec);
		goto exit;
	}

	if (nb_clean) {

		rec->nb_clean_total += nb_clean;
		rec->nb_pending -= nb_clean;
		rec->clean_idx = (rec->clean_idx + nb_clean) & (rec->array_size - 1);
	}

	if (rec->nb_pending >= rec->array_size) {
		os_log(LOG_CRIT, "clock(%p) REC Driver overflow, idx %d nb_clean %u nb_write %u\n", rec, *rec->write_idx, rec->nb_clean_total, rec->nb_ts_total);
		rc = OS_MCR_ERROR;
	} else if (!rec->nb_pending) {
		os_log(LOG_CRIT, "clock(%p) REC Driver underflow, idx %d nb_clean %u nb_write %u\n", rec, *rec->write_idx, rec->nb_clean_total, rec->nb_ts_total);
		rc = OS_MCR_ERROR;
	}

exit:
	return rc;
}

static inline int period_error(unsigned int p1, unsigned int p2)
{
	/* Check if the periods differ by more than (100/1024) % */
	/* Make the check here less strict than the one in the CRF code, so that
	 * a timestamp that was accepted by the CRF will always be accepted here
	 * as well (to avoid quick LOCKED/UNLOCKED sequences).
	 */
	if (os_abs(p1 - p2) > (p1 >> 5))
		return 1;
	else
		return 0;
}

void media_clock_rec_stats_print(struct ipc_avtp_clock_rec_stats *msg)
{
	struct clock_rec_stats *stats = &msg->stats;

	os_log(LOG_INFO, "clock(%p)\n", msg->clock_id);

	os_log(LOG_INFO, "running %10u running_locked: %10u, restart: %10u, uncertain: %10u, offset: %10u, period: %10u, rec driver: %10u, crit: %10u\n",
		stats->running, stats->running_locked, stats->err_restart, stats->err_uncertain, stats->err_offset,
		stats->err_period, stats->err_rec_driver, stats->err_crit);

	stats_compute(&stats->period);

	os_log(LOG_INFO, "period: %d/%d/%d (ns)\n", stats->period.min, stats->period.mean, stats->period.max);
}


void media_clock_rec_stats_dump(struct media_clock_rec *rec, struct ipc_avtp_clock_rec_stats *msg)
{
	struct clock_rec_stats *stats = &msg->stats;

	msg->clock_id = rec;

	os_memcpy(stats, &rec->stats, sizeof(rec->stats));

	stats_reset(&rec->stats.period);
}

static unsigned int ts_offset(unsigned int period)
{
	/* Media clock timestamps should be at least 2 clock periods in the future (since media clock recovery */
	/* hardware mechanism pre-loads 2 timestamps in advance) plus some margin to account for software processing */
	/* Everything must be aligned to the clock period */
	return 2 * period + ((MCR_DELAY + period - 1) / period) * period;
}

/** Main media clock recovery function.
 * Takes in argument an array of timestamps and performs measurements
 * and sanity checks before providing the timestamps
 * to the lower-level recovery mechanism.
 * \return	 media clock state
 * \param rec	pointer to media_clock_rec context
 * \param ts	pointer to an array of timestamp struct
 * \param num_ts	number of elements in the array
 */
media_clock_rec_state_t media_clock_rec(struct media_clock_rec *rec, struct timestamp *ts, int num_ts)
{
	int i = 0;
	u32 local_time = 0, curr_ts, period;
	int offset, rc;

	/* Un-recoverable error state, exit */
	if ((rec->state == ERR) || (!rec->ts_period)) {
		rec->stats.err_crit++;
		goto exit;
	}

restart:
	if (os_clock_gettime32(avtp_to_clock(CFG_DEFAULT_PORT_ID), &local_time) < 0)
		goto exit;

	for (; i < num_ts; i++, ts++) {

		if (unlikely(ts->flags & AVTP_MEDIA_CLOCK_RESTART)) {
//			os_log(LOG_ERR, "clock(%p) media clock restart\n", rec);

			rec->stats.err_restart++;

			rec->state = INIT;

			/* clear the flag so it won't trigger again */
			ts->flags &= ~AVTP_MEDIA_CLOCK_RESTART;

			goto restart;
		}

		if (unlikely(ts->flags & AVTP_TIMESTAMP_UNCERTAIN)) {
//			os_log(LOG_ERR, "clock(%p) timestamp uncertain\n", rec);

			rec->stats.err_uncertain++;

			rec->state = INIT;

			/* skip this timestamp */
			i++, ts++;

			goto restart;
		}

		curr_ts = ts->ts_nsec;

		period = curr_ts - rec->last_ts;

		switch (rec->state) {
		case INIT:
			offset = curr_ts - local_time;
			/* maximal accepted offset value */
			rec->max_offset_val = min((3 * rec->array_size / 4) * period, 100 * NS_PER_MS);

			/* Initialize with default period */
			rec->delay = ts_offset(rec->ts_period);

			if ((offset + (int)rec->delay) < 0) {
//				os_log(LOG_ERR, "clock(%p) invalid timestamp, offset %u, ts %u, local %u\n", rec, offset, curr_ts, local_time);

				rec->stats.err_offset++;

				break;
			}

			rec->state = MEASUREMENT;
			rec->period_mean = 0;
			rec->offset_max = offset;
			rec->offset_min = offset;
			*rec->write_idx = 0;
			rec->clean_idx = 0;
			rec->nb_clean_total = 0;
			rec->nb_meas = 0;
			rec->nb_ts_total = 0;
			rec->nb_pending = 0;

			stats_reset(&rec->stats.period);

			if (rec->flags & MCR_FLAGS_RUNNING) {
				if (os_media_clock_rec_stop(&rec->os) < 0) {
					/* Un-recoverable error */
					rec->state = ERR;
					break;
				}
				if (os_media_clock_rec_reset(&rec->os) < 0) {
					/* Un-recoverable error */
					rec->state = ERR;
					break;
				}
				rec->flags &= ~MCR_FLAGS_RUNNING;
			}

			break;

		case MEASUREMENT:
			offset = curr_ts - local_time;

			if (((offset + (int)rec->delay) < 0) || ((offset + (int)rec->delay) > rec->max_offset_val)) {

				rec->stats.err_offset++;
				rec->state = INIT;

				continue;
			}

			if (period > MCR_MAX_PERIOD) {

//				os_log(LOG_ERR, "clock(%p) invalid timestamp frequency, ts %u, period %d\n", rec, curr_ts, period);

				rec->stats.err_period++;
				rec->state = INIT;

				continue;
			}

			/* For now we don't do much with the measurements but they will be needed for
			 * supporting various clock stream input
			 */
			if (rec->nb_meas != 0)
				rec->period_mean = (rec->period_mean + period) / 2;
			else {
				rec->period_min = period;
				rec->period_max = period;
				rec->period_mean = period;
			}

			if (period < rec->period_min)
				rec->period_min = period;

			if (period > rec->period_max)
				rec->period_max = period;

			if (offset < rec->offset_min)
				rec->offset_min = offset;

			if (offset > rec->offset_max)
				rec->offset_max = offset;

			if (++rec->nb_meas >= MCR_NB_MEAS) {

				if (period_error(rec->period_min, rec->period_max)) {
//					os_log(LOG_ERR, "clock(%p) period range too big %d %d\n", rec, rec->period_min, rec->period_max);

					rec->stats.err_period++;
					rec->state = INIT;

					continue;
				}

				/* Adjust based on actual measured period */
				rec->delay = ts_offset(rec->period_mean);

				rec->offset_max += rec->delay;
				rec->offset_min += rec->delay;

				if ((rec->offset_max < 0) || (rec->offset_min < 0)) {
					rec->stats.err_offset++;
					rec->state = INIT;

					continue;
				}

				/* Check if DDR array buffer is sufficient */
				if ((rec->offset_max / rec->period_mean) > (3 * rec->array_size / 4)) {
//					os_log(LOG_ERR, "clock(%p) offset to local time too large offset max %u, period %u\n", rec, rec->offset_max, rec->period_mean);

					rec->stats.err_offset++;
					rec->state = INIT;

					continue;
				}

				if (period_error(rec->ts_period, rec->period_mean)) {
					rec->stats.err_period++;
					rec->state = INIT;
					continue;
				}

				rec->state = READY;
				rec->ready = 0;
			}
			break;

		case READY:
			if (period_error(rec->ts_period, period)) {
//				os_log(LOG_ERR, "clock(%p) invalid timestamp frequency, ts %u, period %d (%d)\n", rec, curr_ts, period, rec->period_mean);
				rec->stats.err_period++;
				rec->state = INIT;
				continue;
			}

			if (rec->ready < 2)
				rec->ts[rec->ready] = curr_ts + rec->delay;
			else if (rec->ready < 4) {
				media_clock_rec_write_ts(rec, curr_ts + rec->delay);

				if (rec->ready == 3) {
					if (os_media_clock_rec_start(&rec->os, ts_wa(rec->ts[0]), ts_wa(rec->ts[1])) < 0) {
						/* Un-recoverable error */
						rec->state = ERR;
						break;
					}
					rec->state = RUNNING;
					rec->flags |= MCR_FLAGS_RUNNING;
				}
			}

			rec->ready++;

			break;

		case RUNNING:
		case RUNNING_LOCKED:
			if (period_error(rec->ts_period, period)) {

//				os_log(LOG_ERR, "clock(%p) invalid timestamp frequency, ts %u, period %d (%d)\n", rec, curr_ts, period, rec->period_mean);

				rec->stats.err_period++;
				rec->state = INIT;

				continue;
			}

			offset = curr_ts + rec->delay - local_time;
			if ((offset < 0) || (offset > rec->max_offset_val)) {

				rec->stats.err_offset++;
				rec->state = INIT;

				continue;
			}

			rec->stats.running++;

			if (rec->state == RUNNING_LOCKED)
				rec->stats.running_locked++;

			stats_update(&rec->stats.period, period);

			media_clock_rec_write_ts(rec, curr_ts + rec->delay);

			if (!(*rec->write_idx & (MCR_CLEAN_BATCH - 1))) {
				rc = media_clock_rec_clean(rec);
				/* Check if the media clock driver has passed to locked state */
				if (rc == OS_MCR_RUNNING_LOCKED && rec->state == RUNNING) {

					rec->state = RUNNING_LOCKED;

					os_log(LOG_INFO, "clock(%p) locked: meas period %u/%u/%u, offset %u/%u, added delay %u\n",
						rec, rec->period_min, rec->period_mean, rec->period_max,
						rec->offset_min, rec->offset_max, rec->delay);
				} else if (rc == OS_MCR_ERROR) {
					rec->state = INIT;
					rec->stats.err_rec_driver++;
				}
			}

			break;

		default:
			os_log(LOG_ERR, "Invalid media clock rec state %d\n", rec->state);
			break;
		}

		rec->last_ts = curr_ts;
	}
exit:
	return rec->state;
}

/** Opens a media clock recovery context to sync it to a gPTP based
 *  media clock.
 * A media clock recovery context is associated to a HW recovery system.
 * \return	 	0 if succes or negative in case of error
 * \param rec	pointer to media_clock_rec structure
 */

int media_clock_rec_open_ptp(struct media_clock_rec *rec)
{
	if ((rec->flags & MCR_FLAGS_IN_USE)) {
		os_log(LOG_ERR, "clock(%p) device already opened\n", rec);
		return -1;
	}

	if (os_media_clock_rec_set_ptp_sync(&rec->os) < 0) {
		os_log(LOG_ERR, "clock(%p) ptp sync error\n", rec);
		return -1;
	}

	if (os_media_clock_rec_start(&rec->os, 0, 0) < 0) {
		os_log(LOG_ERR, "clock(%p) start error\n", rec);
		return -1;
	}

	rec->flags |= MCR_FLAGS_IN_USE;

	return 0;
}

/** Closes a media clock recovery context.
 * \return	 none
 * \param rec	pointer to media_clock_rec  context
 */
void media_clock_rec_close_ptp(struct media_clock_rec *rec)
{
	media_clock_rec_close(rec);
}

/** Opens a media clock recovery context.
 * A media clock recovery context is associated to a HW recovery system.
 * \return	 	0 if succes or negative in case of error
 * \param clk_array	pointer to media_clock_rec  array
 * \param id		id in the media_clock_rec array
 * \param freq		frequency
 */
int media_clock_rec_open(struct media_clock_rec *rec,
							unsigned int ts_freq_p, unsigned int ts_freq_q,
							void *context)
{
	if (!rec) {
		os_log(LOG_ERR, "clock(%p)\n", rec);
		goto err;
	}

	if ((rec->flags & MCR_FLAGS_IN_USE)) {
		os_log(LOG_ERR, "clock(%p) device already opened\n", rec);
		goto err;
	}

	if (os_media_clock_rec_set_ext_ts(&rec->os) < 0) {
		os_log(LOG_ERR, "clock(%p) set ext ts error\n", rec);
		goto err;
	}

	if (os_media_clock_rec_reset(&rec->os) < 0) {
		os_log(LOG_ERR, "clock(%p) reset error\n", rec);
		goto err;
	}

	if (os_media_clock_rec_set_ts_freq(&rec->os, ts_freq_p, ts_freq_q) < 0) {
		os_log(LOG_ERR, "clock(%p) ts freq config error\n", rec);
		goto err_set_ts_freq;
	}

	rec->state = INIT;
	rec->flags |= MCR_FLAGS_IN_USE;
	rec->ts_period = ((u64)NSECS_PER_SEC * ts_freq_q) / ts_freq_p;

	os_memset(&rec->stats, 0, sizeof(rec->stats));
	stats_init(&rec->stats.period, 31, NULL, NULL);

	os_log(LOG_INFO, "clock(%p) ts freq: %u/%u = %u Hz, period: %u ns\n",
		rec, ts_freq_p, ts_freq_q, ts_freq_p / ts_freq_q, rec->ts_period);

	return 0;

err_set_ts_freq:
err:
	return -1;
}

/** Closes a media clock recovery context.
 * \return	 none
 * \param rec	pointer to media_clock_rec  context
 */
void media_clock_rec_close(struct media_clock_rec *rec)
{
	if (rec->flags & MCR_FLAGS_IN_USE) {
		os_media_clock_rec_stop(&rec->os);
		rec->flags &= ~(MCR_FLAGS_RUNNING | MCR_FLAGS_IN_USE);
	}
}

__init struct media_clock_rec *media_clock_rec_init(int domain_id)
{
	struct media_clock_rec *rec;

	rec = os_malloc(sizeof(*rec));
	if (!rec)
		goto err_malloc;

	os_memset(rec, 0, sizeof(struct media_clock_rec));

	if (os_media_clock_rec_init(&rec->os, domain_id) < 0)
		goto err_init;

	rec->id = domain_id;
	rec->array_addr = rec->os.array_addr;
	rec->array_size = rec->os.array_size;
	rec->write_idx = rec->array_addr + rec->array_size;

	/* Success */
	os_log(LOG_INIT, "clock id %d, init done\n", domain_id);

	return rec;

err_init:
	os_free(rec);

err_malloc:
	return NULL;
}

__exit void media_clock_rec_exit(struct media_clock_rec *rec)
{
	os_media_clock_rec_exit(&rec->os);

	os_free(rec);

	os_log(LOG_INIT, "done\n");
}

static void clock_grid_mult_ts_update(struct clock_grid *grid, unsigned int requested, unsigned int *reset);

/** This function opens a HW user clock generation (media_clock_gen_usr_hw) based on the
 * HW clock generator (media_clock_gen_hw). The frequency of the user clock cannot be lower
 * than the HW clock frequency and needs to be a mutiple of it.
 * \return	 0 if success, -1 otherwise
 * \param clock		pointer to media_clock_gen_hw context
 * \param clk_usr	pointer to media_clock_gen_usr_hw context
 * \param freq_p	timestamp frequency for the user clock generation (in the form p/q Hz)
 * \param freq_q	timestamp frequency for the user clock generation
 * \param wake_freq_p	wake-up frequency for the user clock generation (in the form p/q Hz)
 * \param wake_freq_q	wake-up frequency for the user clock generation
 */
int clock_grid_init_mult(struct clock_grid *grid, struct clock_domain *domain, struct clock_grid *grid_parent, unsigned int freq_p, unsigned int freq_q)
{
	u32 hw_freq = grid_parent->nominal_freq_p / grid_parent->nominal_freq_q;
	struct clock_grid_producer_mult *mult = &grid->producer.u.mult;
	u32 *ring_base;

	if (freq_p < hw_freq * freq_q) {
		os_log(LOG_ERR, "clock grid(%p) frequency(%u Hz) cannot be under %u Hz\n", grid, freq_p / freq_q, hw_freq);
		goto err;
	}

	grid->flags = 0;

	ring_base = os_malloc(MCG_TS_SIZE * sizeof(u32));
	if (!ring_base)   {
		os_log(LOG_ERR, "clock grid(%p) could not allocate %zu bytes for clock grid\n",
				grid, MCG_TS_SIZE * sizeof(u32));
		goto err;
	}

	if (clock_grid_init(grid, GRID_PRODUCER_MULT, ring_base, MCG_TS_SIZE, freq_p, freq_q, clock_grid_mult_ts_update) < 0)
		goto err_grid_init;

	mult->state = GEN_INIT;
	mult->interval = grid->nominal_period;
	mult->interval_rem = (u64)NSECS_PER_SEC * grid->nominal_freq_q - (u64)mult->interval * grid->nominal_freq_p;

	clock_grid_consumer_attach(&mult->source, grid_parent, 0, 0);

	clock_domain_add_grid(domain, grid);

	os_log(LOG_INFO, "clock grid(%p) successfully opened on parent grid(%p)\n", grid, grid_parent);

	return 0;

err_grid_init:
	os_free(ring_base);

err:
	return -1;
}

void clock_grid_mult_exit(struct clock_grid *grid)
{
	clock_grid_consumer_exit(&grid->producer.u.mult.source);
	os_free(grid->ts);

	os_log(LOG_INFO, "clock_grid MULT(%p) closed\n", grid);
}

static unsigned int ts_available(struct clock_grid_consumer *consumer)
{
	unsigned int avail;

	if (consumer->grid->write_index >= consumer->read_index)
		avail = consumer->grid->write_index - consumer->read_index;
	else
		avail = (consumer->grid->write_index + consumer->grid->ring_size) - consumer->read_index;

	return avail;
}

static void ts_put(struct clock_grid *grid, unsigned int ts)
{
	if (grid->count >= 1) {
		u32 prev_ts = grid->ts[(grid->write_index - 1) & (grid->ring_size - 1)];
		unsigned int period = ts - prev_ts;

		if (os_abs((int)period - (int)grid->nominal_period) > grid->period_jitter)
			grid->stats.err_period++;

		stats_update(&grid->stats.period, ts - prev_ts);
	}

	grid->ts[grid->write_index] = ts;

	grid->write_index = (grid->write_index + 1) & (grid->ring_size - 1);
	grid->count++;

}

static unsigned int ts_peek_n(struct clock_grid_consumer *consumer, unsigned int n)
{
	return consumer->grid->ts[(consumer->read_index + n) & (consumer->grid->ring_size - 1)];
}

static unsigned int ts_peek(struct clock_grid_consumer *consumer)
{
	return consumer->grid->ts[consumer->read_index];
}

static void _clock_grid_consumer_reset(struct clock_grid_consumer *consumer)
{
	struct clock_grid *grid = consumer->grid;
	u32 start_count = clock_grid_start_count(grid);

	os_log(LOG_DEBUG, "Resetting consumer(%p): count %u grid count %u read %u write %u start_count %u (grid size %u, valid_count %u)\n",
			consumer, consumer->count, consumer->grid->count, consumer->read_index, grid->write_index, start_count,
			consumer->grid->ring_size, consumer->grid->valid_count);

	consumer->read_index = (grid->write_index - start_count) % grid->ring_size;
	consumer->count = grid->count - start_count;
}

static unsigned int ts_get(struct clock_grid_consumer *consumer)
{
	struct clock_grid *grid = consumer->grid;
	unsigned int ts = consumer->grid->ts[consumer->read_index];
	unsigned int period, next_ts, prev_offset;

	/* Detect discontinuities caused by the producer */
	if (consumer->init) {
		period = ts - consumer->prev_ts;

		if (os_abs((int)period - (int)grid->nominal_period) > grid->period_jitter) {
			/* Adjust consumer offset to hide discontinuity */

			next_ts = consumer->prev_ts + consumer->prev_period;
			prev_offset = consumer->offset;
			consumer->offset += next_ts - ts;

			os_log(LOG_DEBUG, "consumer(%p) discontinuity %u, next: %u, %u %u %u %d %d\n",
				consumer, consumer->count, next_ts, ts + consumer->offset, ts, consumer->prev_ts, consumer->offset, prev_offset);
		} else {
			consumer->prev_period = period;
		}
	}

	consumer->prev_ts = ts;
	ts += consumer->offset;

	consumer->read_index = (consumer->read_index + 1) & (consumer->grid->ring_size - 1);
	consumer->count++;
	consumer->init = 1;

	return ts;
}

static void clock_grid_consumer_reset(struct clock_grid_consumer *consumer)
{
	u32 new_ts;

	_clock_grid_consumer_reset(consumer);

	if (consumer->init) {
		new_ts = ts_peek(consumer);

		consumer->offset += (consumer->prev_ts + consumer->prev_period - new_ts);
		consumer->prev_ts = new_ts - consumer->prev_period;
	}

	stats_reset(&consumer->stats.ts_err);
	stats_reset(&consumer->stats.ts_batch);
}

static void clock_grid_consumer_overflow_check(struct clock_grid_consumer *consumer)
{
	/* Check for overflow of the clock grid */
	if (((int)consumer->grid->count -(int)consumer->count) > (int)consumer->grid->valid_count)  {
		consumer->stats.err_reset++;
		clock_grid_consumer_reset(consumer);
	}
}


static void ts_generate_new(struct clock_grid *grid)
{
	struct clock_grid_producer_mult *mult = &grid->producer.u.mult;

	ts_put(grid, mult->ts_last);

	mult->ts_last += mult->interval;

	mult->ts_last_frac += mult->interval_rem;
	if (mult->ts_last_frac >= grid->nominal_freq_p) {
		mult->ts_last++;
		mult->ts_last_frac -= grid->nominal_freq_p;
	}
}

static void clock_grid_consumer_ts_update(struct clock_grid_consumer *consumer, unsigned int requested, unsigned int *reset)
{
	struct clock_grid *grid = consumer->grid;
	unsigned int ts_avail;

	if (grid->ts_update) {

		if (consumer->init) {
			clock_grid_consumer_overflow_check(consumer);

			ts_avail = ts_available(consumer);
			if (requested > ts_avail)
				requested -= ts_avail;
			else
				goto exit;	/* Amount requested is less than available, exit early */

		}

		grid->ts_update(grid, requested, reset);
		clock_grid_update_valid_count(grid);

		if (consumer->init)
			clock_grid_consumer_overflow_check(consumer);
		else
			_clock_grid_consumer_reset(consumer);
	}

exit:
	return;
}

/** Updates the HW user clock grid based on the evolution of the HW clock.
 * \return	 none
 * \param clk_usr	pointer to media_clock_gen_usr_hw context
 * \param reset 	pointer to unsigned int
 */
static void clock_grid_mult_ts_update(struct clock_grid *grid, unsigned int requested, unsigned int *reset)
{
	struct clock_grid_producer_mult *mult = &grid->producer.u.mult;
	struct clock_grid *source_grid = mult->source.grid;
	unsigned int flags = 0;
	unsigned int ts_n;

	*reset = 0;

	/* First TS, lock to HW generation */
init:
	if (mult->state == GEN_INIT) {
		u32 alignment_ts;

		/* Align the multiplier grid to "now", so that timestamp offset grid stats can be easily
		 * checked
		 */
		if (os_clock_gettime32(avtp_to_clock(CFG_DEFAULT_PORT_ID), &alignment_ts) < 0)
			return;

		flags |= MCG_FLAGS_DO_ALIGN;

		if (clock_grid_consumer_get_ts(&mult->source, &mult->hw_ts_last, 1, &flags, alignment_ts) < 1)
			return;

		mult->interval = ((u64)NSECS_PER_SEC * grid->nominal_freq_q) / grid->nominal_freq_p;
		mult->interval_rem = (u64)NSECS_PER_SEC * grid->nominal_freq_q - (u64)mult->interval * grid->nominal_freq_p;

		mult->slot = 0;
		mult->state = GEN_RUNNING;

		mult->ts_last = mult->hw_ts_last;
		mult->ts_last_frac = 0;

		os_log(LOG_DEBUG, "clk_grid(%p) start, read_index: %u, hw ts: %u, sw interval: %u\n",
			grid, mult->source.read_index, mult->hw_ts_last, mult->interval);
		os_log(LOG_DEBUG, "clk_grid(%p) start, write_index: %u, grid_count: %u, consumer_count: %u, offset %u, write_val %u, read_val %u\n",
			grid, mult->source.grid->write_index, mult->source.grid->count, mult->source.count, mult->source.offset,
			mult->source.grid->ts[mult->source.grid->write_index], mult->source.grid->ts[mult->source.read_index]);

		*reset = 1;
	}

	/* Compute ts */
	if (requested)
		ts_n = requested;
	else
		ts_n = 1;

	while (ts_n) {
		u32 hw_freq = source_grid->nominal_freq_p / source_grid->nominal_freq_q;

		/* New interval */
		if (mult->slot >= grid->nominal_freq_p) {
			u32 hw_ts, period;

			flags = 0;

			if (clock_grid_consumer_get_ts(&mult->source, &hw_ts, 1, &flags, 0) < 1)
				break;

			period = hw_ts - mult->hw_ts_last;
			if (os_abs((int)period - (int)mult->source.grid->nominal_period) > mult->source.grid->period_jitter) {
//				os_log(LOG_ERR, "grid(%p) invalid hw ts %u %u %u\n", grid, hw_ts, mult->hw_ts_last, period);

				mult->state = GEN_INIT;

				if (!(*reset)) {
					requested = ts_n;

					goto init;
				}

				break;
			}

			mult->slot -= grid->nominal_freq_p;

			if (!mult->slot) {
				/* Integer multiple of reference period, reset timestamp offset */
				mult->interval = ((u64)(hw_ts - mult->hw_ts_last + hw_ts - mult->ts_last) * hw_freq * grid->nominal_freq_q) / grid->nominal_freq_p;
				mult->interval_rem = ((u64)(hw_ts - mult->hw_ts_last + hw_ts - mult->ts_last) * hw_freq * grid->nominal_freq_q) - (u64)grid->nominal_freq_p * mult->interval;
			} else {
				mult->interval = ((u64)(hw_ts - mult->hw_ts_last) * hw_freq * grid->nominal_freq_q) / grid->nominal_freq_p;
				mult->interval_rem = ((u64)(hw_ts - mult->hw_ts_last) * hw_freq * grid->nominal_freq_q) - (u64)grid->nominal_freq_p * mult->interval;
			}

			mult->hw_ts_last = hw_ts;
		}

		ts_generate_new(grid);

		mult->slot += hw_freq * grid->nominal_freq_q;

		if (requested)
			ts_n--;
	}

}
static unsigned int clock_grid_consumer_mean_period(struct clock_grid_consumer *consumer)
{
	int n_ts = min(10, ts_available(consumer));
	unsigned int mean = 0;

	if (n_ts >= 2) {
		int i;
		unsigned int period;

		/* Look for any discontinuity */
		for (i = 0; i < (n_ts - 1); i++) {
			period = ts_peek_n(consumer, i + 1) - ts_peek_n(consumer, i);
			if (os_abs((int)period - (int)consumer->grid->nominal_period) > consumer->grid->period_jitter)
				break;
		}

		if (i >= 1)
			mean = (ts_peek_n(consumer, i) - ts_peek_n(consumer, 0)) / i;
	}

	/*
	 * Not enough timestamps or early discontinuity
	 */
	if (!mean) {
		mean = consumer->grid->nominal_period;
		os_log(LOG_ERR, "consumer(%p) failed to measure mean period, fallback to nominal period\n", consumer);
	}

	return mean;
}

static void clock_grid_consumer_compute_offset(struct clock_grid_consumer *consumer, unsigned int alignment_ts)
{
	if (ts_available(consumer)) {
		unsigned int mean_period = clock_grid_consumer_mean_period(consumer);
		u32 new_ts = ts_peek(consumer);

		consumer->offset = alignment_ts - new_ts;

		if (consumer->alignment) {
			consumer->offset = (consumer->offset + consumer->alignment - 1) / consumer->alignment;
			consumer->offset = consumer->offset * consumer->alignment;
		}

		consumer->prev_ts = new_ts - mean_period;

		os_log(LOG_DEBUG, "consumer(%p) measured period %u, %u %u offset %d %u %u(ns)\n",
		       consumer, mean_period, alignment_ts, new_ts, consumer->offset, new_ts + consumer->offset, consumer->prev_ts);
	} else {
		consumer->offset = 0;
	}

	if (consumer->offset > (CLOCK_GRID_VALID_TIME_US * 1000)) {
		os_log(LOG_DEBUG, "consumer(%p) offset %u above threshold %u (ns)\n", consumer, consumer->offset, (CLOCK_GRID_VALID_TIME_US * 1000));
		consumer->stats.err_offset++;
	}
}


int clock_grid_consumer_get_ts(struct clock_grid_consumer *consumer, u32 *ts, unsigned int ts_n, unsigned int *flags, unsigned int alignment_ts)
{
	unsigned int written = 0;
	unsigned int reset = 0;
	unsigned int ts_n_actual;
	unsigned int ts_avail;

	if (ts_n)
		clock_grid_consumer_ts_update(consumer, ts_n, &reset);

	if (*flags & MCG_FLAGS_DO_ALIGN)
		clock_grid_consumer_compute_offset(consumer, alignment_ts);

	if (reset) {
		*flags |= MCG_FLAGS_RESET;
		consumer->stats.err_reset++;
	}

	ts_avail = ts_available(consumer);
	ts_n_actual = min(ts_n, ts_avail);
	if (ts_n_actual < ts_n) {
		consumer->stats.err_starved++;
		os_log(LOG_DEBUG, "consumer(%p): Not enough timestamps: ts_n %u avail %u read %u write %u grid_count %u consumer_count %u\n",
				consumer, ts_n, ts_n_actual, consumer->read_index, consumer->grid->write_index, consumer->grid->count, consumer->count);
	}

	while (written < ts_n_actual) {
		*ts = ts_get(consumer);

		if (!written) {
			stats_update(&consumer->stats.ts_err, (int)*ts - (int)consumer->gptp_current);
			//stats_update(&consumer->stats.ts_err, (consumer->grid->write_index - consumer->read_index) & (consumer->grid->ring_size - 1));
		}

		consumer->stats.ts++;
		ts++;
		written++;
	}

	stats_update(&consumer->stats.ts_batch, written);

	return written;
}


void clock_grid_consumer_stats_print(struct ipc_avtp_clock_grid_consumer_stats *msg)
{
	struct clock_grid_consumer_stats *stats = &msg->stats;

	stats_compute(&stats->ts_err);
	stats_compute(&stats->ts_batch);

	os_log(LOG_INFO, "	clock_consumer(%p) grid(%p)\n", msg->consumer, msg->grid);
	os_log(LOG_INFO, "	ts: %10u, offset err: %10u, reset err: %10u, starved err: %10u ts_err: %10d /%10d /%10d (ns) ts batch: %3d/%3d/%3d\n",
		stats->ts, stats->err_offset, stats->err_reset, stats->err_starved,
		stats->ts_err.min, stats->ts_err.mean, stats->ts_err.max,
		stats->ts_batch.min, stats->ts_batch.mean, stats->ts_batch.max);
}

void clock_grid_consumer_stats_dump(struct clock_grid_consumer *consumer, struct ipc_tx *tx)
{
	struct ipc_desc *desc;
	struct ipc_avtp_clock_grid_consumer_stats *msg;

	desc = ipc_alloc(tx, sizeof(*msg));
	if (!desc)
		goto err_ipc_alloc;

	desc->type = IPC_AVTP_CLOCK_GRID_CONSUMER_STATS;
	desc->len = sizeof(*msg);
	desc->flags = 0;

	msg = (struct ipc_avtp_clock_grid_consumer_stats *)&desc->u;

	msg->consumer = consumer;
	msg->grid = consumer->grid;

	os_memcpy(&msg->stats, &consumer->stats, sizeof(consumer->stats));

	stats_reset(&consumer->stats.ts_err);
	stats_reset(&consumer->stats.ts_batch);

	if (ipc_tx(tx, desc) < 0)
		goto err_ipc_tx;

	return;

err_ipc_tx:
	ipc_free(tx, desc);

err_ipc_alloc:
	return;
}

int clock_grid_consumer_attach(struct clock_grid_consumer *consumer, struct clock_grid *grid, unsigned int offset, unsigned int alignment)
{
	int rc = clock_grid_ref(grid);

	if (!rc) {
		consumer->grid = grid;
		consumer->offset = offset;
		consumer->alignment = alignment;
		consumer->init = 0;
		consumer->prev_period = grid->nominal_period;

		stats_init(&consumer->stats.ts_err, 31, NULL, NULL);
		stats_init(&consumer->stats.ts_batch, 31, NULL, NULL);
	}

	return rc;
}

void clock_grid_consumer_detach(struct clock_grid_consumer *consumer)
{
	if (consumer->grid) {
		clock_grid_unref(consumer->grid);

		consumer->grid = NULL;
		consumer->count = 0;
		consumer->read_index = 0;
	}
}

void clock_grid_consumer_exit(struct clock_grid_consumer *consumer)
{
	clock_grid_consumer_detach(consumer);
}

void clock_producer_stream_rx(struct clock_grid *grid, struct timestamp *ts, unsigned *ts_n, unsigned int do_stitch)
{
	struct clock_grid_producer_stream *producer = &grid->producer.u.stream;
	struct media_clock_rec *rec = producer->rec;
	int i;

	if (rec) {
		/* If upper layer notifies that a timestamp discontinuity is happening but could be worthwhile
		 * to mask (going into a stable locked state) and if the discontinuity is enough to unlock
		 * the media clock recovery, offset the timestamps to "hide" the discontinuity.
		 *
		 * FIXME align on media clock sample rate
		 */
		if (rec->state == RUNNING_LOCKED) {
			if (do_stitch && period_error(grid->nominal_period, ts[0].ts_nsec + producer->stitch_ts_offset - rec->last_ts)) {
				producer->stitch_ts_offset = grid->nominal_period - (ts[0].ts_nsec - rec->last_ts);
				clock_domain_clear_state(grid->domain, CLOCK_DOMAIN_STATE_LOCKED);
				os_log(LOG_DEBUG, "stitch offset %d\n", producer->stitch_ts_offset);
			}

			if (producer->stitch_ts_offset) {
				for (i = 0; i < *ts_n; i++)
					ts[i].ts_nsec += producer->stitch_ts_offset;
			}
		}

		media_clock_rec(rec, ts, *ts_n);

		grid->write_index = *rec->write_idx;
		grid->count = rec->nb_ts_total;

		if (rec->state == RUNNING_LOCKED)
			clock_domain_set_state(grid->domain, CLOCK_DOMAIN_STATE_LOCKED);
		else {
			clock_domain_clear_state(grid->domain, CLOCK_DOMAIN_STATE_LOCKED);
			producer->stitch_ts_offset = 0;
		}
	}
	else {
		int i = 0;

		while (i < *ts_n)
			ts_put(grid, ts[i++].ts_nsec);
	}

	clock_grid_update_valid_count(grid);
}

int clock_producer_stream_open(struct clock_grid *grid, struct stream_listener *stream, struct media_clock_rec *rec,
					unsigned int ts_freq_p, unsigned int ts_freq_q)
{
	struct clock_grid_producer_stream *producer = &grid->producer.u.stream;

	if (media_clock_rec_open(rec, ts_freq_p, ts_freq_q, grid) < 0)
		goto err_rec_open;

	if (clock_grid_init(grid, GRID_PRODUCER_STREAM, rec->array_addr, rec->array_size, ts_freq_p, ts_freq_q, NULL) < 0)
		goto err_grid_init;

	producer->rec = rec;
	producer->stream = stream;
	producer->stitch_ts_offset = 0;

	os_log(LOG_INFO, "stream(%p) rec(%p)\n", stream, rec);

	return 0;

err_grid_init:
	media_clock_rec_close(rec);

err_rec_open:
	return -1;
}

void clock_producer_stream_close(struct clock_grid *grid)
{
	struct clock_grid_producer_stream *producer = &grid->producer.u.stream;

	if (producer->stream) {
		if (producer->rec)
			media_clock_rec_close(producer->rec);
		else
			os_free(grid->ts);

		producer->stream = NULL;
		producer->rec = NULL;
	}
}
