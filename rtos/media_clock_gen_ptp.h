/*
 * Copyright 2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVB media clock generation driver
*/

#ifndef _MEDIA_CLOCK_GEN_PTP_
#define _MEDIA_CLOCK_GEN_PTP_

#include "media_clock.h"

#define MCLOCK_DOMAIN_PTP_RANGE		1

struct mclock_gen_ptp_stats {
	unsigned int reset;
	unsigned int start;
	unsigned int stop;
	unsigned int ts_period;
	unsigned int ptp_now;
	unsigned int err_drift;
	unsigned int err_jump;
};

struct mclock_gen_ptp {
	struct mclock_dev dev;
	unsigned int clk_ptp;
	uint32_t ts_next;
	uint32_t ts_period;
	uint32_t ts_period_rem;
	uint32_t ts_corr;
	uint32_t ts_period_n;
	struct mclock_gen_ptp_stats stats;
};

int mclock_gen_ptp_init(struct mclock_gen_ptp *ptp);
void mclock_gen_ptp_exit(struct mclock_gen_ptp *ptp);


#define MCLOCK_GEN_NUM_TS		256	// Needs to be power of 2.
#define MCLOCK_GEN_BUF_SIZE		(MCLOCK_GEN_NUM_TS * sizeof(unsigned int))
#define MCLOCK_GEN_MMAP_SIZE		(MCLOCK_GEN_BUF_SIZE + 3 * sizeof(unsigned int)) // w_idx, ptp and count
#define MCLOCK_GEN_TS_FREQ_INIT		300

#endif /* _MEDIA_CLOCK_GEN_PTP_ */
