/*
* Copyright 2015 Freescale Semiconductor, Inc.
* Copyright 2019-2021, 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief 802.1AS target clock update functions header file
 @details Definition of target clock synchronization functions and data structures.
*/

#ifndef _TARGET_CLOCK_ADJ_H_
#define _TARGET_CLOCK_ADJ_H_

#include "genavb/init.h"

#include "common/stats.h"
#include "common/ptp.h"
#include "os/clock.h"

#include "config.h"

#define PTP_PHASE_DISCONT_THRESHOLD	4000ULL // in ns

enum {
	TARGET_PLL_UNLOCKED = 0,
	TARGET_PLL_LOCKING,
	TARGET_PLL_LOCKED
};

/**
 * Hardware clock adjustments parameters
 */
struct target_clkadj_params {
	os_clock_id_t clock;				/**< clock to use with clock adjustment functions */
	struct ptp_local_clock_entity *local_clock;
	unsigned int mode;
	int state;

	u8 instance_index;
	u8 domain;

	int freq_change;		/* tracks if there was a frequency adjustment on last sync received */
	int phase_change;		/* tracks if there was a phase adjustment on last sync received */

	u64 previous_receipt_time;
	u64 previous_receipt_local_time;

	s64 integral;
	s64 previous_err;
	s64 last_ppb;

	struct stats freq_stats;
	struct stats diff_stats;
};


void target_clkadj_params_init(struct target_clkadj_params *target_clkadj_params, os_clock_id_t clk_id, struct ptp_local_clock_entity *local_clock, u8 instance_index, u8 domain);
void target_clkadj_params_exit(struct target_clkadj_params *target_clkadj_params);
int target_clock_adjust_on_sync(struct target_clkadj_params *target_clkadj_params, u64 sync_receipt_time, u64 sync_receipt_local_time, ptp_double gm_rate_ratio);
void target_clkadj_dump_stats(struct target_clkadj_params *target_clkadj_params);
void target_clkadj_gm_change(struct target_clkadj_params *target_clkadj_params);
void target_clkadj_system_role_change(struct target_clkadj_params *target_clkadj_params, bool is_grandmaster);

#endif /* _TARGET_CLOCK_ADJ_H_ */
