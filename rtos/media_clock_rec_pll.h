/*
 * Copyright 2018-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVB media clock recovery driver
*/
#ifndef _MEDIA_CLOCK_REC_PLL_
#define _MEDIA_CLOCK_REC_PLL_

#include "media_clock.h"
#include "pi.h"
#include "common/types.h"
#include "rational.h"
#include "imx-pll.h"
#include "gptp_dev.h"

#define MCLOCK_PLL_REC_TRACE   0

typedef enum {
	RESET,
	START,
	ADJUST,
	LOCKED,
} rec_pll_state_t;

struct mclock_rec_pll_stats {
	unsigned int err_meas;
	unsigned int err_gptp_reload;
	unsigned int err_gptp_start;
	unsigned int err_gptp_gettime;
	unsigned int err_wd;
	unsigned int err_ts;
	unsigned int err_drift;
	unsigned int err_set_pll_rate;
	unsigned int err_pll_prec;
	unsigned int reset;
	unsigned int start;
	unsigned int stop;
	unsigned int adjust;
	int last_app_adjust;
	int err_cum;
	int err_per_sec;
	unsigned int err_time;
	unsigned int irq_count;
	unsigned int irq_count_fec_event;
	unsigned int fec_reloaded;
	uint32_t pll_numerator;
	uint32_t measure;
};

#if MCLOCK_PLL_REC_TRACE

#define MCLOCK_REC_TRACE_SIZE  512

struct mclock_rec_trace {
	unsigned int meas;
	unsigned int ticks;
	unsigned int state;
	int err;
	int adjust_value;
	int pi_control_output;
	int pi_err_input;
	unsigned long new_rate;
	unsigned long previous_rate;
};

#endif

struct mclock_rec_pll {
	struct mclock_dev dev;
	struct imx_pll pll;
	struct gptp_dev *gptp_event_dev;
	rec_pll_state_t state;
	struct rational fec_next_ts;
	struct rational fec_period;
	struct rational pll_clk_target;
	struct rational pll_clk_period; // per FEC sampling interval
	unsigned int pll_clk_measure;
	unsigned int pll_ref_freq; // PLL frequency at timer input clk
	struct rational clk_media;
	unsigned int div;
	unsigned int r_idx;
	unsigned int ts_slot;
	unsigned int ts_offset;
	rtos_atomic_t ts_read;
	rtos_atomic_t status;
	struct pi pi;
	unsigned int factor;
	int max_adjust;
	int measure;
	int ppb_adjust; /* Exact ppb adjust returned from the PLL control layer*/
	int req_ppb_adjust; /* Requested pbb adjust passed to the PLL control layer*/
	unsigned int zero_err_nb; /* Number of measurements with zero error since last detected error */
	unsigned int wd;
#if MCLOCK_PLL_REC_TRACE
	unsigned int trace_count;
	unsigned int trace_freeze;
	struct mclock_rec_trace trace[MCLOCK_REC_TRACE_SIZE];
#endif
	struct mclock_rec_pll_stats stats;
};

int  mclock_rec_pll_init(struct mclock_rec_pll *rec);
void mclock_rec_pll_exit(struct mclock_rec_pll *rec);
int  mclock_rec_pll_timer_irq(struct mclock_rec_pll *rec, int fec_event, unsigned int meas, unsigned int ticks);
void mclock_rec_pll_reset(struct mclock_rec_pll *rec);
int  mclock_rec_pll_stop(struct mclock_rec_pll *rec);
int  mclock_rec_pll_start(struct mclock_rec_pll *rec, struct mclock_start *start);
int  mclock_rec_pll_config(struct mclock_dev *dev, struct mclock_sconfig *cfg);
int  mclock_rec_pll_clean_get(struct mclock_dev *dev, struct mclock_clean *clean);

static inline void mclock_rec_pll_wd_reset(struct mclock_rec_pll *rec)
{
	rec->wd = rec->dev.clk_timer + 10 * rec->fec_period.i;
}

static inline void mclock_rec_pll_inc_r_idx(struct mclock_rec_pll *rec)
{
	rec->r_idx = (rec->r_idx + 1) & (rec->dev.num_ts - 1);
}

#define MCLOCK_REC_NUM_TS		256	//Needs to be power of 2. Cannot be increased without kernel SDMA driver change (max 682 BD)
#define MCLOCK_REC_BUF_SIZE		(MCLOCK_REC_NUM_TS * sizeof(unsigned int))
#define MCLOCK_REC_MMAP_SIZE		(MCLOCK_REC_BUF_SIZE + sizeof(unsigned int)) //w_idx
#define MCLOCK_REC_TS_FREQ_INIT		6000

#define MCLOCK_REC_PLL_NB_MEAS		10
#define MCLOCK_REC_PLL_LOCKED_ERR_NB	3 /*Number of consecutive zero measurement errors before we declare the recovery locked*/

/* Default sampling frequency in internal mode and target for external TS */
#define MCLOCK_PLL_SAMPLING_FREQ 	100
#define MCLOCK_PLL_SAMPLING_PERIOD_MS	(1000 / MCLOCK_PLL_SAMPLING_FREQ)
#define MCLOCK_PLL_SAMPLING_PERIOD_NS	(MCLOCK_PLL_SAMPLING_PERIOD_MS * 1000 * 1000)

#endif /* _MEDIA_CLOCK_REC_PLL_ */
