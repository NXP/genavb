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
#ifndef _CLOCK_GRID_H_
#define _CLOCK_GRID_H_

#include "common/stats.h"
#include "common/list.h"
#include "common/ipc.h"
#include "os/media_clock.h"
#include "os/timer.h"

struct clock_domain;

typedef enum  {
	GEN_INIT,
	GEN_RUNNING,
} clock_grid_producer_state_t;

typedef enum {
	GRID_PRODUCER_MULT = 0,
	GRID_PRODUCER_PTP,
	GRID_PRODUCER_STREAM,
	GRID_PRODUCER_HW,
	GRID_PRODUCER_MAX,
	GRID_PRODUCER_NONE = GRID_PRODUCER_MAX
} clock_grid_producer_type_t;


struct clock_grid_consumer {
	struct clock_grid *grid;
	u32 gptp_current;
	u32 alignment;
	u32 read_index;
	u32 offset;
	u32 prev_ts;		/* updated each time a timestamp is consumed */
	u32 prev_period;	/* initialized with nominal period, updated each time a timestamp is consumed */
	u32 init;		/* flag that indicates if prev_ts is valid */
	u32 count;
	unsigned int prio;
	struct list_head list;		/* FIXME WAKEUP Used for scheduling. Likely to be removed when switching to media/net event wake-up scheme. */
	struct clock_grid_consumer_stats {
		unsigned int ts;
		unsigned int err_offset;
		unsigned int err_reset;
		unsigned int err_starved;

		struct stats ts_err;
		struct stats ts_batch;
	} stats;
};


struct clock_grid_producer_mult {
	struct clock_grid_consumer source;

	clock_grid_producer_state_t state;
	unsigned int interval;			/**< Interval between 2 timestamps, in ns */
	unsigned int interval_rem;		/**< Fractional part of interval, in units of 1/nominal_freq_p ns */
	unsigned int slot;			/**< Current position in the interval between the 2 source timestamps, in units of 1/nominal_freq_p ns  */
	u32 ts_last;				/**< Previous ts stored in grid (in ns) */
	u32 ts_last_frac;			/**< Fractional part of ts_last, in units of 1/nominal_freq_p ns */
	u32 hw_ts_last;				/**< Previous source timestamp (in ns) */
};

struct clock_grid_producer_stream {
	struct stream_listener *stream;
	struct media_clock_rec *rec;
	unsigned int stitch_ts_offset;
};

struct clock_grid_producer {
	clock_grid_producer_type_t type;

	union {
		struct clock_grid_producer_mult mult;

		struct clock_grid_producer_stream stream;

		struct os_media_clock_gen hw;
	} u;

	unsigned int last_ts;
};

#define CLOCK_GRID_FLAGS_STATIC	(1 << 0)

struct clock_grid {
	struct list_head list;
	unsigned int flags;
	u16 ref_count;				/* number of consumers using the grid */
	u32 nominal_freq_p;
	u32 nominal_freq_q;
	u32 nominal_period;			/**< Cached from nominal_freq */
	u32 period_jitter;
	u32 max_valid_count;
	u32 max_start_count;
	struct clock_domain *domain;

	unsigned int ring_size; 		/**< Number of timestamps in the ring buffer */
	u32 *ts; /* switch to u64 in the future */
	u32 valid_count;
	u32 count;
	u32 write_index;

	void (*ts_update)(struct clock_grid *grid, unsigned int requested, unsigned int *reset);
	struct clock_grid_producer producer;  /* no need for a pointer, since a given producer should always produce the same grid. */
	struct clock_grid_stats {
		unsigned int err_period;
		struct stats period;
	} stats;
};

struct ipc_avtp_clock_grid_stats {
	void *grid;
	void *domain;
	clock_grid_producer_type_t type;

	struct clock_grid_stats stats;
};

struct clock_grid *clock_grid_alloc(void);
void clock_grid_free(struct clock_grid *grid);

int clock_grid_init(struct clock_grid *grid, clock_grid_producer_type_t type, u32 *ring_base, unsigned int ring_size, u32 nominal_freq_p, u32 nominal_freq_q, void (*ts_update)(struct clock_grid *, unsigned int , unsigned int *));
void clock_grid_exit(struct clock_grid *grid);

void clock_grid_ts_update(struct clock_grid *grid, unsigned int requested, unsigned int *reset);
unsigned int clock_grid_start_count(struct clock_grid *grid);
void clock_grid_update_valid_count(struct clock_grid *grid);
int clock_grid_ref(struct clock_grid *grid);
void clock_grid_unref(struct clock_grid *grid);

void clock_grid_stats_print(struct ipc_avtp_clock_grid_stats *msg);
void clock_grid_stats_dump(struct clock_grid *grid, struct ipc_tx *tx);

const char *clock_grid_producer_type2string(clock_grid_producer_type_t type);

#endif /* _CLOCK_GRID_H_ */
