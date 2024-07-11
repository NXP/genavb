/*
* Copyright 2015 Freescale Semiconductor, Inc.
* Copyright 2019-2021, 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief 802.1AS Target clock update functions
 @details Synchronize a target clock against a gPTP grandmaster
*/

#include "config.h"
#include "target_clock_adj.h"
#include "common/ptp.h"
#include "common/ptp_time_ops.h"
#include "common/stats.h"
#include "common/log.h"
#include "os/stdlib.h"


/** Unlock target clock pll
 * \return	none
 * \param target_clkadj_params	pointer to the adjustments parameters structure
 */
static void target_clkadj_params_unlock(struct target_clkadj_params *target_clkadj_params)
{
	target_clkadj_params->state = TARGET_PLL_UNLOCKED;
}

static void target_clkadj_params_reset(struct target_clkadj_params *target_clkadj_params)
{
	target_clkadj_params_unlock(target_clkadj_params);

	os_clock_setfreq(target_clkadj_params->clock, 0);
	target_clkadj_params->freq_change = 1;
	target_clkadj_params->last_ppb = 0;

	target_clkadj_params->local_clock->rate_ratio_adjustment = 1.0 + target_clkadj_params->last_ppb / 1.0e9;

	if (target_clkadj_params->mode & OS_CLOCK_ADJUST_MODE_HW_OFFSET)
		target_clkadj_params->local_clock->phase_discont++;
}

/** Dumps target clock adjustment stats
 * \return	none
 * \param target_clkadj_params	pointer to the adjustments parameters structure
 */
void target_clkadj_dump_stats(struct target_clkadj_params *target_clkadj_params)
{
	struct stats *freq_stats;
	struct stats *diff_stats;

	freq_stats = &target_clkadj_params->freq_stats;
	diff_stats = &target_clkadj_params->diff_stats;

	stats_compute(freq_stats);
	stats_compute(diff_stats);

	os_log(LOG_INFO_RAW, "domain(%u, %u) Correction applied to target clock (ppb): min %6d avg %6d max %6d variance %5"PRIu64"\n",
		target_clkadj_params->instance_index, target_clkadj_params->domain, freq_stats->min, freq_stats->mean, freq_stats->max, freq_stats->variance);

	os_log(LOG_INFO_RAW, "domain(%u, %u) Offset between GM and target clock (ns):  min %6d avg %6d max %6d variance %5"PRIu64"\n",
		target_clkadj_params->instance_index, target_clkadj_params->domain, diff_stats->min, diff_stats->mean, diff_stats->max, diff_stats->variance);

	stats_reset(freq_stats);
	stats_reset(diff_stats);
}


/** Call to signal a domain grandmaster change
 * \return	none
 * \param target_clkadj_params	pointer to the adjustments parameters structure
 */
void target_clkadj_gm_change(struct target_clkadj_params *target_clkadj_params)
{
	os_log(LOG_INFO, "domain(%u, %u) GM change\n", target_clkadj_params->instance_index, target_clkadj_params->domain);

	target_clkadj_params_unlock(target_clkadj_params);
}

/** Call to signal a time aware system transition from/to grandmaster role
 * \return	none
 * \param target_clkadj_params	pointer to the adjustments parameters structure
 * \param is_grandmaster	true if we are the grandmaster, false otherwise
 */
void target_clkadj_system_role_change(struct target_clkadj_params *target_clkadj_params, bool is_grandmaster)
{
	os_log(LOG_INFO, "domain(%u, %u) grandmaster(%d)\n", target_clkadj_params->instance_index, target_clkadj_params->domain, is_grandmaster);

	if (is_grandmaster)
		target_clkadj_params_reset(target_clkadj_params);
}


/** Initialize the target clock adjustment parameters.
 * \return	none
 * \param target_clkadj_params	pointer to the adjustments parameters structure
 * \param clk_id 		clock identifier of the adjusted clock
 * \param local_clock		Local clock entity. Used to track target clock adjustment side effects on the local clock.
 */
void target_clkadj_params_init(struct target_clkadj_params *target_clkadj_params, os_clock_id_t clk_id, struct ptp_local_clock_entity *local_clock, u8 instance_index, u8 domain)
{
	target_clkadj_params->clock = clk_id;
	target_clkadj_params->local_clock = local_clock;
	target_clkadj_params->mode = os_clock_adjust_mode(clk_id);

	target_clkadj_params->local_clock->phase_discont = 0;

	target_clkadj_params->instance_index = instance_index;
	target_clkadj_params->domain = domain;

	target_clkadj_params_reset(target_clkadj_params);
}

/** Free resources allocated for clock adjustement.
 * \return	none
 * \param target_clkadj_params	pointer to the adjustments parameters structure
 */
void target_clkadj_params_exit(struct target_clkadj_params *target_clkadj_params)
{

}

/**
 *  One slaving algorithm for the target clock, so it tracks grandmaster gptp clock phase:
 *  2nd order PLL, type 2, which basically contains a PI controller:
 *  u(t) = e(t) * kp + integral (e(t)) * ki
 *  e(t) is the measured phase error between the grandmaster clock and our target clock (i.e, gptp time received in sync messages and the target time at which the message arrived)
 *  u(t) is the control variable, the ratio applied to the target clock frequency (and close to the frequency ratio between the target and grandmaster clocks)
 *
 * This is based on the information present in 802.1AS-2011, Annex B.4 and ITU-T Recommendation G.810
 * The performance of the PI controller is very sensitive to an initial estimate of the frequency ratio. We make sure that no local clock adjustments are made while estimating
 * the ratio.
 * To counter problems due to discrete sampling of the phase error, e(t) is actually the phase error per unit of time (which should be approximately constant and independent
 * of the sampling period).
 *
 * The initial conditions for the PI controller are:
 *  e(0) = 0, obtained by doing an offset adjustment of the target clock
 *  u(0) = ratio ==> integral(0) = ratio / ki, with ratio estimated from rateRatio or two consecutive sync messages
 *
 * The actual rate control is done in ppb:
 * e(t) = err(t) / dt * 10^9, err(t) is the phase error in seconds, dt is the elapsed time in seconds
 * u(t) = ratio * 10^9
 * ppb(t) = u(t) - 10^9
 *
 * In normal/locked operation, only frequency adjustments are done to the target clock.
 * If a step in the phase error is observed (above a certain threshold) the PI controller is reinitialized.
 * If the initial phase error is low (below a certain threshold), we avoid doing an offset adjustment (and let the PI controller bring the error down).
 *
 *  Three possible operation modes, depending on how target clock adjustments affect the local clock:
 *  - Offset + ratio adjustments affect the local clock
 *  - ratio adjustment affects the local clock
 *  - no adjustment affects the local clock
 *
 *  Which one is used depends on software/hardware configuration and capabilities.
 *
 * OS_CLOCK_ADJUST_MODE_HW_OFFSET:
 *  Local clock offset adjustments lead to big errors in pdelay measurements both locally and remotely. They should be done rarely, only when
 *  it's not possible to synchronize target clock with ratio adjustments alone (in a reasonable time, <~1s). Usually they are done
 *  once when the first sync messages are received from the grandmaster.
 *  These adjustments must be tracked and affected pdelay measurements rejected (the standard pdelay filtering is not enough to reject these measurements).
 *  It's is very important to avoid setting neighborRateRatioValid = False (which results in !asCapable for 802.1AS-2011), due to these adjustments.
 *
 * No hardware adjustments:
 *  When no adjustements are made to the local clock, pdelay neighborRateRatio measurements are very stable (and rateRatio). No additions are required on top of the standard state
 *  machines.
 *
 *  Handling of local clock adjustments is outside the scope of 802.1AS, additional logic must be added to the standard state machines in order to achieve a
 *  stable synchronization, reliably.
 *
 *
 * \return 1 if a phase discontinuity happened on the local clock, 0 if not.
 * \param target_clkadj_params	pointer to the clock adjustments parameters structure
 * \param sync_receipt_time	64-bit gptp timestamp within the sync message itself set by the sender
 * \param sync_receipt_local_time	64-bit local timestamp when the sync message is received
 * \param gm_rate_ratio		rate ratio between the local clock and the grandmaster conveyed within the sync message
 */
int target_clock_adjust_on_sync(struct target_clkadj_params *target_clkadj_params, u64 sync_receipt_time, u64 sync_receipt_local_time, ptp_double gm_rate_ratio)
{
	s64 err_ns, dt_ns, err;
	s64 ppb;
	u64 abs_err_ns;
	int freq_change = 0;
	int phase_change = 0;
	s64 dt_local, ratio;

#define _kp	2	/* kp = 1 / _kp */
#define _ki	16	/* ki = 1 / _ki */

	err_ns = sync_receipt_time - sync_receipt_local_time;
	dt_ns = sync_receipt_time - target_clkadj_params->previous_receipt_time;
	abs_err_ns = os_llabs(err_ns);

start:
	switch (target_clkadj_params->state) {
	case TARGET_PLL_UNLOCKED:
	default:
		os_log(LOG_INFO, "domain(%u, %u) PLL unlocked\n", target_clkadj_params->instance_index, target_clkadj_params->domain);

		stats_init(&target_clkadj_params->freq_stats, 31, NULL, NULL);
		stats_init(&target_clkadj_params->diff_stats, 31, NULL, NULL);

		target_clkadj_params->state = TARGET_PLL_LOCKING;

		/* fallthrough */

	case TARGET_PLL_LOCKING:
		/* Remove offset and estimate ratio */

		if (!target_clkadj_params->mode) {

			/* Lock immediately, gm_rate_ratio can be used as is (not dependent on local rate adjustment) */
			ppb = (s64)(1.0e9 * (gm_rate_ratio - 1.0));

		} else {
			/* Wait for two syncs, without an offset or rate adjustment, to get a good ratio estimate and trying to lock */
			/* measured rate is dependent on applied rate adjustment */
			if (target_clkadj_params->freq_change) {
				/* wait for next sync */
				goto exit;
			}

			if ((target_clkadj_params->mode & OS_CLOCK_ADJUST_MODE_HW_OFFSET) && target_clkadj_params->phase_change) {
				/* wait for next sync */
				goto exit;
			}

			if (!(sync_receipt_local_time > target_clkadj_params->previous_receipt_local_time)) {
				os_log(LOG_ERR, "domain(%u, %u) Invalid values for local sync receipt, check local clock\n", target_clkadj_params->instance_index, target_clkadj_params->domain);
				goto exit;
			}

			dt_local = sync_receipt_local_time - target_clkadj_params->previous_receipt_local_time;

			ratio = (dt_ns * 1000000000LL) / dt_local;
			ppb = ratio - 1000000000LL;

			ppb += target_clkadj_params->last_ppb;
		}

		/* Ratio sanity check */
		if (os_abs(ppb) > PTP_MAXFREQ_PPB) {
			os_log(LOG_ERR, "domain(%u, %u) Invalid ratio: %"PRId64"\n", target_clkadj_params->instance_index, target_clkadj_params->domain, ppb);
			goto exit;
		}

		if (abs_err_ns > PTP_PHASE_DISCONT_THRESHOLD) {
			os_log(LOG_INFO, "domain(%u, %u) Initial adjustment, offset: %"PRId64" ns, freq_adjust: %"PRId64"\n\n", target_clkadj_params->instance_index, target_clkadj_params->domain, err_ns, ppb);

			os_clock_setoffset(target_clkadj_params->clock, err_ns);
			err_ns = 0;
			phase_change = 1;
		}

		target_clkadj_params->integral = (ppb + 1000000000LL) * _ki;
		err = 0;

		target_clkadj_params->state = TARGET_PLL_LOCKED;

		break;

	case TARGET_PLL_LOCKED:
		if (abs_err_ns > PTP_PHASE_DISCONT_THRESHOLD) {
			target_clkadj_params->state = TARGET_PLL_UNLOCKED;
			goto start;
		}

		/*
		 * PLL with PI filter
		 *
		 * integral(n) = integral(n-1) + (e(n - 1) + e(n)) / 2
		 * u(n) = e(n) * kp + integral(n) * ki
		 *
		 * if e(n) is in nanoseconds, ppb(n) = u(n) - 10^9
		 */
		err = (err_ns * 1000000000LL) / dt_ns;
		target_clkadj_params->integral += (err + target_clkadj_params->previous_err) / 2;

		ppb = ((err / _kp) + (target_clkadj_params->integral / _ki)) - 1000000000LL;

		break;
	}

	if (ppb != target_clkadj_params->last_ppb) {
		os_clock_setfreq(target_clkadj_params->clock, ppb);
		freq_change = 1;
		target_clkadj_params->last_ppb = ppb;
	}

	target_clkadj_params->previous_err = err;

	os_log(LOG_DEBUG, "domain(%u, %u) err: %"PRId64", dt: %"PRId64", err_ppb: %"PRId64", integral_ppb: %lld, ppb: %"PRId64"\n",
	target_clkadj_params->instance_index, target_clkadj_params->domain, err_ns, dt_ns, err / _kp, target_clkadj_params->integral / _ki - 1000000000LL, ppb);

	stats_update(&target_clkadj_params->freq_stats, ppb);
	stats_update(&target_clkadj_params->diff_stats, err_ns);

exit:
	target_clkadj_params->freq_change = freq_change;
	target_clkadj_params->phase_change = phase_change;

	target_clkadj_params->previous_receipt_time = sync_receipt_time;
	target_clkadj_params->previous_receipt_local_time = sync_receipt_local_time;

	if (freq_change && (target_clkadj_params->mode & OS_CLOCK_ADJUST_MODE_HW_RATIO)) {
		target_clkadj_params->local_clock->rate_ratio_adjustment = 1.0 + target_clkadj_params->last_ppb / 1.0e9;
		os_log(LOG_ERR, "domain(%u, %u) should no longer be executed\n", target_clkadj_params->instance_index, target_clkadj_params->domain);
	}

	if (phase_change && (target_clkadj_params->mode & OS_CLOCK_ADJUST_MODE_HW_OFFSET)) {
		target_clkadj_params->local_clock->phase_discont++;
		return phase_change;
	}

	return 0;
}
