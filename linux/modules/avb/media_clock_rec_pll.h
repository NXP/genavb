/*
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
 * Copyright 2018-2020, 2022-2024 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _MEDIA_CLOCK_REC_PLL_
#define _MEDIA_CLOCK_REC_PLL_

#include "media_clock.h"
#include "pi.h"
#include "imx-pll.h"

#define MCLOCK_PLL_REC_TRACE   0

typedef enum {
	RESET,
	START,
	MEASURE,
	ADJUST,
	ADJUST_LOCKED,
} rec_pll_state_t;

struct mclock_rec_pll_stats {
	unsigned int err_meas;
	unsigned int err_port_down;
	unsigned int err_fec;
	unsigned int err_wd;
	unsigned int err_ts;
	unsigned int err_drift;
	unsigned int err_set_pll_rate;
	unsigned int err_pll_prec;
	unsigned int reset;
	unsigned int start;
	unsigned int stop;
	unsigned int adjust;
	unsigned int locked_state;
	int last_app_adjust;
	int err_per_sec;
	unsigned int err_time;
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
	int meas_diff;
	int pll_shift_ns;
	uint32_t pll_ticks;
	int32_t ticks_shift;
	uint64_t ts_ticks;
	uint64_t audio_pll_ticks;
	int64_t ticks_err;
	uint64_t dt_ts_ticks;
};

#endif


struct mclock_rec_pll {
	struct mclock_dev dev;
	struct imx_pll pll;
	rec_pll_state_t state;
	unsigned int fec_tc_id;
	struct rational fec_next_ts;
	struct rational fec_period;
	unsigned int fec_nb_meas;
	struct rational pll_clk_target;
	struct rational pll_clk_period; //per FEC sampling interval
	unsigned int pll_clk_meas;
	unsigned int pll_ref_freq; //PLL frequency at timer input clk
	unsigned int adjust_locked_ppb_threshold;
	unsigned int pll_timer_clk_div; //divider applied to audio pll clock to root the timer (GPT, TPM) module
	struct rational clk_media;
	unsigned int div;
	unsigned int r_idx;
	unsigned int ts_slot;
	unsigned int ts_offset;
	atomic_t ts_read;
	atomic_t status;
	struct pi pi;
	int req_ki_factor;
	int req_kp_factor;
	s64 start_ppb_err; /* The accumulated starting ppb error measurements used to reset the PI at the end of the START state */
	int max_adjust;
	int meas;
	int req_ppb_adjust; /* Requested pbb adjust passed to the PLL control layer */
	unsigned int accepted_ppb_err_nb; /* Number of consecutive error measurements within accepted range */
	unsigned int wd;
	uint64_t ts_ticks;                /* Absolute TS phase in audio pll ticks (e.g expected audio pll ticks depending on TS frequency) */
	uint64_t audio_pll_ticks;         /* Absolute measured audio pll ticks */
	uint64_t prev_ts_ticks;           /* starting point of phase error measurement period */
	uint64_t initial_ts_ticks;
	uint64_t initial_pll_ticks;
	int total_pll_shift_ns;               /* Total audio PLL phase shift in ns. */
	int req_pll_shift_ns;                 /* Requested audio PLL phase shift in ns. */

	bool pll_scu_controlled; /* true if the audio pll is controlled through SCU (e.g for i.MX8DXL) */
#if MCLOCK_PLL_REC_TRACE
	unsigned int override_trace_idx;
	unsigned int trace_count;
	unsigned int trace_freeze;
	struct mclock_rec_trace trace[MCLOCK_REC_TRACE_SIZE];
#endif
	int locked_ppb_err; /* accumulated error measurement since last adjustement in locked phase. */
	unsigned int locked_meas; /* Number of error measurements since last adjustement in locked phase. */
	bool is_hw_recovery_mode; /* True when using recovery pll with hardware sampling, false otherwise. */
	u32  next_ts; /* next software sampling timestamp: must be a 32 bits value. */
	u32 audio_pll_cnt_last;
	int next_ppb;
	struct task_struct *mcr_kthread;
	struct mclock_rec_pll_stats stats;
};

int  mclock_rec_pll_init(struct mclock_rec_pll *rec, unsigned int eth_port);
void mclock_rec_pll_exit(struct mclock_rec_pll *rec);
int mclock_rec_pll_timer_irq(struct mclock_rec_pll *rec, int fec_event, unsigned int meas, unsigned int pll_ticks_val, unsigned int ticks);
int mclock_rec_pll_sw_sampling_irq(struct mclock_rec_pll *rec, u32 audio_pll_cnt, u32 ptp_now, unsigned int ticks);
void mclock_rec_pll_reset(struct mclock_rec_pll *rec);
int  mclock_rec_pll_ready(struct mclock_rec_pll *rec);
int  mclock_rec_pll_stop(struct mclock_rec_pll *rec);
int  mclock_rec_pll_start(struct mclock_rec_pll *rec, struct mclock_start *start);
int  mclock_rec_pll_config(struct mclock_dev *dev, struct mclock_sconfig *cfg);
int  mclock_rec_pll_clean_get(struct mclock_dev *dev, struct mclock_clean *clean);

static inline void mclock_rec_pll_wd_reset(struct mclock_rec_pll *rec)
{
	rec->wd = rec->dev.clk_timer + 4 * rec->fec_period.i;
}

static inline void mclock_rec_pll_inc_r_idx(struct mclock_rec_pll *rec)
{
	rec->r_idx = (rec->r_idx + 1) & (rec->dev.num_ts - 1);
}

#define MCLOCK_REC_NUM_TS		256	//Needs to be power of 2. Cannot be increased without kernel SDMA driver change (max 682 BD)
#define MCLOCK_REC_BUF_SIZE		(MCLOCK_REC_NUM_TS * sizeof(unsigned int))
#define MCLOCK_REC_MMAP_SIZE		(MCLOCK_REC_BUF_SIZE + sizeof(unsigned int)) //w_idx
#define MCLOCK_REC_DMA_SIZE		(MCLOCK_REC_MMAP_SIZE + sizeof(unsigned int)) //One more to hold tmode value
#define MCLOCK_REC_TS_FREQ_INIT		6000

#define MCLOCK_REC_PLL_NB_MEAS			10
#define MCLOCK_REC_PLL_NB_MEAS_START_SKIP	3 /* Number of first measurement to skip before starting ppb error average calculation */
#define MCLOCK_REC_PLL_IN_LOCKED_PPB_ERR	1000 /* Declare the domain locked (ADJUST -> ADJUST_LOCKED) if audio pll error measurement is under 1 ppm */
#define MCLOCK_REC_PLL_IN_LOCKED_NB_VALID_PPB_ERR	8 /* Number of consecutive pll error measurements < MCLOCK_REC_PLL_LOCKED_PPB_ERR before we declare the recovery locked */
#define MCLOCK_REC_PLL_ADJUST_LOCKED_SAMPLING_NUM	16 /* Number of PLL error sampling to be measured in locked phase before feeding to PI (to reduce sampling jitter) */
/* Max allowed ppb drift threshold in ADJUST_LOCKED_PHASE per timer frequency range. */
#define MCLOCK_REC_PLL_LOW_FREQ_MAX_HZ			12288000
#define MCLOCK_REC_PLL_MEDIUM_FREQ_MAX_HZ		49152000
#define MCLOCK_REC_PLL_LOW_FREQ_PPB_THRESHOLD		1500 /* For low frequency pll input: Go from ADJUST_LOCKED to ADJUST (quick adjustements) if measured error is over 1.5 ppm. */
#define MCLOCK_REC_PLL_MEDIUM_FREQ_PPB_THRESHOLD	750  /* For medium frequency pll input: Go from ADJUST_LOCKED to ADJUST (quick adjustements) if measured error is over 0.75 ppm. */
#define MCLOCK_REC_PLL_HIGH_FREQ_PPB_THRESHOLD		500  /* For high frequency pll input: Go from ADJUST_LOCKED to ADJUST (quick adjustements) if measured error is over 0.5 ppm. */

/* Default sampling frequency in internal mode and target for external TS */
#define MCLOCK_PLL_SAMPLING_FREQ 	100
#define MCLOCK_PLL_SAMPLING_PERIOD_MS	(1000 / MCLOCK_PLL_SAMPLING_FREQ)
#define MCLOCK_PLL_SAMPLING_PERIOD_NS	(MCLOCK_PLL_SAMPLING_PERIOD_MS * 1000 * 1000)

#endif /* _MEDIA_CLOCK_REC_PLL_ */
