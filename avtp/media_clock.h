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
#ifndef _MEDIA_CLOCK_H_
#define _MEDIA_CLOCK_H_

#include "os/media_clock.h"
#include "clock_domain.h"

typedef enum  {
	INIT,
	MEASUREMENT,
	READY,
	RUNNING,
	RUNNING_LOCKED,
	ERR,
} media_clock_rec_state_t;

struct media_clock_rec {
	unsigned int id;
	struct os_media_clock_rec os;
	int flags;
	media_clock_rec_state_t state;
	u32 *array_addr;
	unsigned int array_size; //number of timestamps
	unsigned int ts_period; //theoric value (ns)
	u32 *write_idx; //shared with driver
	unsigned int clean_idx;
	u32 delay;
	u32 period_mean;
	u32 period_min;
	u32 period_max;
	u32 period_first;
	int offset_min;
	int offset_max;
	int max_offset_val;
	u32 last_ts;
	u32 ts[2];
	unsigned int ready;
	unsigned int nb_meas;
	unsigned int nb_clean_total;
	unsigned int nb_ts_total;
	unsigned int nb_pending;

	struct clock_rec_stats {
		unsigned int running;
		unsigned int running_locked;
		unsigned int err_restart;
		unsigned int err_uncertain;
		unsigned int err_offset;
		unsigned int err_period;
		unsigned int err_rec_driver;
		unsigned int err_crit;
		struct stats period;
	} stats;	/**< pointer to time of last update, in number of 125us periods. */
};


#define TS_TX_BATCH	(4 * NET_TX_BATCH)
#define MCG_TS_SIZE	1024 //TS_TX_BATCH
/* 100ms will result in a drift of at most 0.5µs between 2 clocks with a 5ppm clock ratio.
 * Since the uncertainty on our clock measurements (and/or the stability of a single clock)
 * should be much less than 1ppm (gPTP expects less than 0.1ppm),
 * we should meet the 5% grid alignment required by AVTP (5% of 96kHz ~ 0.5µs).
 */
#define CLOCK_GRID_VALID_TIME_US 100000

#define CLOCK_GRID_VALID_PPM	100


int clock_grid_init_mult(struct clock_grid *grid, struct clock_domain *domain, struct clock_grid *grid_parent, unsigned int freq_p, unsigned int freq_q);

int clock_grid_consumer_get_ts(struct clock_grid_consumer *consumer, u32 *ts, unsigned int ts_n, unsigned int *flags, unsigned int alignment_ts); /* similar to media_clock_gen_get_ts */

int clock_grid_consumer_attach(struct clock_grid_consumer *consumer, struct clock_grid *grid, unsigned int offset, unsigned int alignment);
void clock_grid_consumer_detach(struct clock_grid_consumer *consumer);
void clock_grid_consumer_exit(struct clock_grid_consumer *consumer);

struct timestamp {
	unsigned int ts_nsec;
	unsigned int flags;
};

#define MCG_MAX_FREQUENCY	48000
#define MCG_MIN_FREQUENCY	300

#define MCR_MAX_PERIOD		20000000	// 20 ms : Maximum period/minimum frequency allowed for the intermediate clock
#define MCR_DELAY		5000000		// 5 ms : Added to the presentation time to overcome rx batching + processing time

#define MCR_CLEAN_BATCH	 	32 		// 2^x

#define MCR_NB_MEAS		100

#define MCR_FLAGS_IN_USE	(1 << 0)
#define MCR_FLAGS_RUNNING	(1 << 1)

#define MCG_FLAGS_RESET		(1 << 0)
#define MCG_FLAGS_DO_ALIGN	(1 << 1)
#define MCG_NO_WAKE_UP		0
#define MCG_WAKE_UP_MIN		2

#define SYT_INTERVAL_LN2	3

#include "os/media_clock.h"

struct ipc_avtp_clock_grid_consumer_stats {
	void *consumer;
	void *grid;

	struct clock_grid_consumer_stats stats;
};

struct ipc_avtp_clock_rec_stats {
	void *clock_id;
	struct clock_rec_stats stats;
};

int media_clock_rec_open(struct media_clock_rec *clock, unsigned int ts_freq_p, unsigned int ts_freq_q, void *context);
void media_clock_rec_close(struct media_clock_rec *clock);
struct media_clock_rec *media_clock_rec_init(int domain_id);
void media_clock_rec_exit(struct media_clock_rec *);
media_clock_rec_state_t media_clock_rec(struct media_clock_rec *clock, struct timestamp *ts, int num_ts);
void media_clock_rec_stats_print(struct ipc_avtp_clock_rec_stats *msg);
void media_clock_rec_stats_dump(struct media_clock_rec *clock, struct ipc_avtp_clock_rec_stats *msg);
int media_clock_rec_open_ptp(struct media_clock_rec *rec);
void media_clock_rec_close_ptp(struct media_clock_rec *rec);

void clock_grid_consumer_stats_print(struct ipc_avtp_clock_grid_consumer_stats *msg);
void clock_grid_consumer_stats_dump(struct clock_grid_consumer *consumer, struct ipc_tx *tx);

void clock_grid_mult_exit(struct clock_grid *grid);

void clock_producer_stream_rx(struct clock_grid *grid, struct timestamp *ts, unsigned *ts_n, unsigned int stitch_ts);
int clock_producer_stream_open(struct clock_grid *grid, struct stream_listener *stream, struct media_clock_rec *rec,
					unsigned int ts_freq_p, unsigned int ts_freq_q);
void clock_producer_stream_close(struct clock_grid *grid);

static inline int media_clock_gen_get_ts(struct clock_grid_consumer *consumer, u32 *ts, unsigned int ts_n, unsigned int *flags, unsigned int alignment_ts)
{
	return clock_grid_consumer_get_ts(consumer, ts, ts_n, flags, alignment_ts);
}

#endif /* _MEDIA_CLOCK_H_ */
