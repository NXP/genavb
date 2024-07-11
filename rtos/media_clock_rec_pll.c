/*
 * Copyright 2018-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVB media clock recovery driver
*/

#include "rtos_abstraction_layer.h"

#include "media_clock_rec_pll.h"
#include "net_port.h"
#include "avtp.h"

#include "os/sys_types.h"
#include "hw_timer.h"
#include "gptp_dev.h"
#include "common/log.h"

int mclock_rec_pll_clean_get(struct mclock_dev *dev, struct mclock_clean *clean)
{
	int rc = 0;
	struct mclock_rec_pll *rec = container_of(dev, struct mclock_rec_pll, dev);

	clean->nb_clean = rtos_atomic_xchg(&rec->ts_read, 0);
	clean->status = rtos_atomic_read(&rec->status);

	return rc;
}

/**
 * mclock_rec_pll_ts_get() - get next ts to program in ENET time compare
 * @rec: pointer to the PLL media clock recovery context
 * @next_ts: pointer to the timestamp
 * Return : 0 on success, negative otherwise
 *
 */
int mclock_rec_pll_ts_get(struct mclock_rec_pll *rec, unsigned int *next_ts)
{
	unsigned int _next_ts = 0;

	if (rec->dev.ts_src == TS_INTERNAL) {
		rational_add(&rec->fec_next_ts, &rec->fec_next_ts, &rec->fec_period);
		*next_ts = rec->fec_next_ts.i;
	}
	else {
		unsigned int w_idx = *rec->dev.w_idx;
		unsigned int n_read = 0;

		while (rec->r_idx != w_idx)
		{
			if (++rec->ts_slot == rec->div) {
				_next_ts = mclock_mem_read(&rec->dev, rec->r_idx);
				rec->ts_slot = 0;
			}

			mclock_rec_pll_inc_r_idx(rec);
			n_read++;

			if (_next_ts) {
				*next_ts = _next_ts + rec->ts_offset;
				break;
			}
		}

		rtos_atomic_add(n_read, &rec->ts_read);

		if (!_next_ts) {
			return -1;
		}

	}

	return 0;
}

int mclock_rec_pll_ts_reset(struct mclock_rec_pll *rec, unsigned int *next_ts)
{
	unsigned int _next_ts = 0;
	unsigned int n_read = 0;
	unsigned int w_idx = *rec->dev.w_idx;

	/* Clean and get last ts */
	while (rec->r_idx != w_idx) {
		mclock_rec_pll_inc_r_idx(rec);
		n_read++;
	}

	rtos_atomic_add(n_read, &rec->ts_read);

	rec->ts_slot = 0;
	_next_ts = mclock_mem_read(&rec->dev, rec->r_idx - 1);

	if (!_next_ts) {
		return -1;
	} else {
		*next_ts = _next_ts + rec->ts_offset;
		return 0;
	}
}

static int mclock_rec_pll_check_ts_freq(struct mclock_rec_pll *rec, unsigned int ts_freq_p,
			unsigned int ts_freq_q)
{
	u64 ratio = (u64)rec->pll_ref_freq * ts_freq_q;

	/* ts_freq must be a divider of pll frequency
	 * This limitation comes from the codec (generally configured to operate with 48Khz family)
	 * and not from the internal recovery mechanism ready to work with any frequency as it is using
	 * rationals*/

	if (ratio % ts_freq_q) {
		os_log(LOG_ERR, "ts frequency %u / %u is not a divider of PLL freq %u \n",
			 ts_freq_p, ts_freq_q, rec->pll_ref_freq);
		return -1;
	}

	return 0;
}

/* This function will set the sampling frequency and check if the chosen ts frequency is acceptable */
static int mclock_rec_pll_configure_freqs(struct mclock_rec_pll *rec, mclock_ts_src_t ts_src,
						unsigned int ts_freq_p, unsigned int ts_freq_q)
{
	unsigned int div = 1;

	if (ts_src == TS_INTERNAL) {
		rational_init(&rec->fec_period, MCLOCK_PLL_SAMPLING_PERIOD_NS, 1);
		ts_freq_p = MCLOCK_PLL_SAMPLING_FREQ;
		ts_freq_q = 1;
	}
	else {
		struct rational ts_period;

		if (!ts_freq_p || !ts_freq_q) {
			os_log(LOG_ERR, "invalid ts frequency: %u / %u Hz\n", ts_freq_p, ts_freq_q);
			return -1;
		}

		rational_init(&ts_period, (unsigned long long) NSECS_PER_SEC * ts_freq_q, ts_freq_p);
		rec->fec_period = ts_period;

		if ((ts_period.i > MCLOCK_PLL_SAMPLING_PERIOD_NS)
		|| ((2 * MCLOCK_PLL_SAMPLING_PERIOD_NS / ts_period.i) >= rec->dev.num_ts)) {
			os_log(LOG_ERR, "ts_period.i = %d\n", ts_period.i);
			os_log(LOG_ERR, "MCLOCK_PLL_SAMPLING_PERIOD_NS = %d\n", MCLOCK_PLL_SAMPLING_PERIOD_NS);
			os_log(LOG_ERR, "num_ts = %d\n", rec->dev.num_ts);
			os_log(LOG_ERR, "invalid ts frequency: %u / %u Hz\n", ts_freq_p, ts_freq_q);
			return -1;
		}

		/* Divide ts input frequency until periode is greater than samplinq period (10 ms) */
		while (rec->fec_period.i < MCLOCK_PLL_SAMPLING_PERIOD_NS) {
			rational_add(&rec->fec_period, &rec->fec_period, &ts_period);
			div++;
		}

		rec->div = div;
	}

	if (mclock_rec_pll_check_ts_freq(rec, ts_freq_p, ts_freq_q) < 0)
		return -1;

	rational_init(&rec->pll_clk_period, (unsigned long long)rec->pll_ref_freq * ts_freq_q * div, ts_freq_p);
	rec->dev.drift_period = rec->fec_period.i;

	//os_log(LOG_INFO, "%s : fec sampling period: %u + %u/%u, pll period / fec period: %u + %u/%u, div: %d\n",
	//	__func__, rec->fec_period.i, rec->fec_period.p, rec->fec_period.q,
	//	rec->pll_clk_period.i, rec->pll_clk_period.p, rec->pll_clk_period.q, div);

	return 0;
}

int mclock_rec_pll_start(struct mclock_rec_pll *rec, struct mclock_start *start)
{
	struct gptp_dev *dev = rec->gptp_event_dev;
	struct mclock_dev *clock_dev = &rec->dev;
	uint32_t now, ts_0 = 0, ts_1 = 0;
	int rc = 0;

	rec->stats.start++;

#if MCLOCK_PLL_REC_TRACE
	rec->trace_freeze = 0;
	rec->trace_count = 0;
#endif

	if (os_clock_gettime32(dev->port->clock_gptp[0], &now) < 0) {
		rc = -1;
		rec->stats.err_gptp_gettime++;
		goto out;
	}

	if (rec->dev.ts_src == TS_INTERNAL) {
		ts_0 = ((now / rec->fec_period.i) + 2) * rec->fec_period.i;
		ts_1 = ts_0 + rec->fec_period.i;
	} else {
		/* user-space start, use provided timestamps */
		if (start) {
			rec->r_idx = 0;
			rec->ts_slot = 0;
			rec->ts_offset = 2 * rec->fec_period.i;
			rtos_atomic_set(&rec->ts_read, 0);

			/* Simple case, we can use ts_0 and ts_1, the next ts
			 * in array will be continuous
			 */
			if (rec->div == 1) {
				ts_0 = start->ts_0 + rec->ts_offset;
				ts_1 = start->ts_1 + rec->ts_offset;
			}
			/* If frequency is divided, it's not possible to use ts_1 as-is,
			 * generate it with theoric sampling period and adjust offset
			 * so that next read ts in array will be continuous.
			 */
			else {
				ts_0 = start->ts_0 + rec->ts_offset / 2;
				ts_1 = ts_0 + rec->fec_period.i;
				rec->ts_slot++;
			}
		}
		/* internal reset */
		else {
			/* Get last ts in array for ts_1 and generate ts_0 */
			if (mclock_rec_pll_ts_reset(rec, (unsigned int *)&ts_1) < 0) {
				rec->stats.err_ts++;
				rc = -1;
				goto out;
			}

			ts_0 = ts_1 - rec->fec_period.i;
		}
	}

	rational_init(&rec->fec_next_ts, ts_1, 1);

	/* Start sampling ENET clock */
	rc = gptp_event_start(rec->gptp_event_dev, ts_0, ts_1);
	if (rc < 0) {
		rec->stats.err_gptp_start++;
		goto out;
	}

	rec->state = START;
	rec->pll.current_rate = imx_pll_get_rate(&rec->pll);
	/* Initial control output is 0 (e.g 0 ppb variation) */
	pi_reset(&rec->pi, 0);
	rational_init(&rec->pll_clk_target, 0, 1);
	rec->pll_clk_measure = 0;
	rec->measure = 0;
	rec->zero_err_nb = 0;
	mclock_rec_pll_wd_reset(rec);
	/*Set the status to running*/
	rtos_atomic_set(&rec->status, MCLOCK_RUNNING);
	clock_dev->flags &= ~MCLOCK_FLAGS_RUNNING_LOCKED;

	rec->stats.err_per_sec = 0;
	rec->stats.err_cum = 0;
	rec->stats.err_time = 0;

out:
	return rc;
}

int mclock_rec_pll_stop(struct mclock_rec_pll *rec)
{
	struct mclock_dev *dev = &rec->dev;

	rec->stats.stop++;

	gptp_stop(rec->gptp_event_dev);
	rtos_atomic_set(&rec->status, MCLOCK_STOPPED);
	dev->flags &= ~MCLOCK_FLAGS_RUNNING_LOCKED;

	return 0;
}

void mclock_rec_pll_reset(struct mclock_rec_pll *rec)
{
	mclock_rec_pll_stop(rec);
	mclock_rec_pll_start(rec, NULL);
	rec->stats.reset++;
}

static void mclock_rec_pll_adjust(struct mclock_rec_pll *rec, int err)
{
	int adjust_val;
	int last_req_ppb, new_ppb_adjust;
	int rc = 0;
	struct imx_pll *pll = &rec->pll;


	pi_update(&rec->pi, err);

	/*Save the last ppb adjustement*/
	last_req_ppb = rec->req_ppb_adjust;

	/* Anti wind-up */
	adjust_val = (rec->pi.u) - rec->ppb_adjust;

#if MCLOCK_PLL_REC_TRACE
	if (!rec->trace_freeze) {
		rec->trace[rec->trace_count].pi_err_input = err;
		rec->trace[rec->trace_count].pi_control_output = rec->pi.u;
	}
#endif

	if (adjust_val > rec->max_adjust)
		adjust_val = rec->max_adjust;
	else if (adjust_val < (-rec->max_adjust))
		adjust_val = (-rec->max_adjust);

	new_ppb_adjust = rec->ppb_adjust + adjust_val;

#if MCLOCK_PLL_REC_TRACE
	if (!rec->trace_freeze)
		rec->trace[rec->trace_count].previous_rate = pll->current_rate;
#endif

	/*Check if we really need to update the PLL settings*/
	if (last_req_ppb == new_ppb_adjust)
		goto no_adjust;

	/* Save the requested ppb adjust*/
	rec->req_ppb_adjust = new_ppb_adjust;

	rc = imx_pll_adjust(pll, &new_ppb_adjust);

	if (rc == -IMX_CLK_PLL_INVALID_PARAM)
		rec->stats.err_set_pll_rate++;
	else if (rc == -IMX_CLK_PLL_PREC_ERR)
		rec->stats.err_pll_prec++;
	else {
		/*Save the returned (exact) pbb adjust*/
		rec->ppb_adjust = new_ppb_adjust;
#if MCLOCK_PLL_REC_TRACE
		pll->current_rate = imx_pll_get_rate(pll);
#endif
	}

no_adjust:

#if MCLOCK_PLL_REC_TRACE
	if (!rec->trace_freeze) {
		rec->trace[rec->trace_count].adjust_value = rec->ppb_adjust;
		rec->trace[rec->trace_count].new_rate = pll->current_rate;
	}
#endif
	rec->stats.last_app_adjust = rec->ppb_adjust;
	rec->stats.adjust++;
}

int mclock_rec_pll_timer_irq(struct mclock_rec_pll *rec, int gptp_event, unsigned int measure, unsigned int ticks)
{
	struct mclock_dev *dev = &rec->dev;
	unsigned int pll_clk_last, pll_clk_meas_last;
	unsigned int next_ts;
	int err;
	int rc = 0;

	dev->clk_timer += ticks * dev->timer_period;

	if (!gptp_event) {
		if (avtp_after(dev->clk_timer, rec->wd)) {
			rec->state = RESET;
			rec->stats.err_wd++; // watchdog error
		}
		goto out_no_trace;
	}

#if MCLOCK_PLL_REC_TRACE
	if (!rec->trace_freeze) {
		rec->trace[rec->trace_count].meas = measure;
		rec->trace[rec->trace_count].ticks = ticks;
		rec->trace[rec->trace_count].err = 0;
		rec->trace[rec->trace_count].adjust_value = 0;
	}
#endif


	mclock_rec_pll_wd_reset(rec);

	if (mclock_rec_pll_ts_get(rec, &next_ts) < 0) {
		rec->stats.err_ts++;
		goto out;
	}

	/* Reload gptp timer */
	rc = gptp_tc_reload(rec->gptp_event_dev, next_ts);
	if (rc < 0) {
		rec->stats.err_gptp_reload++;
		goto out;
	}

	rec->stats.fec_reloaded++;

	/* Check that measurement is fine */
	if (abs(rec->pll_clk_period.i - measure) >  (rec->pll_clk_period.i / 1000)) {
		/* First value is often wrong, skip it */
		if (rec->measure) {
			rec->state = RESET;
			rc = -1;
		}
		rec->stats.err_meas++;
		rec->measure++;
		goto out;
	}


	pll_clk_last = rec->pll_clk_target.i;
	pll_clk_meas_last = rec->pll_clk_measure;

	rational_add(&rec->pll_clk_target, &rec->pll_clk_target, &rec->pll_clk_period);
	rec->pll_clk_measure += measure;
	rec->stats.measure = measure;

	/* err over a sampling period (frequency) */
	err = (rec->pll_clk_target.i - pll_clk_last) - (rec->pll_clk_measure - pll_clk_meas_last);

	/* err stats */
	rec->stats.err_time += rec->fec_period.i;
	rec->stats.err_cum += err;

	if (rec->stats.err_time >= NSECS_PER_SEC) {
		rec->stats.err_per_sec = rec->stats.err_cum;
		rec->stats.err_cum = 0;
		rec->stats.err_time = 0;
	}

#if MCLOCK_PLL_REC_TRACE
	if (!rec->trace_freeze) {
		rec->trace[rec->trace_count].meas = measure;
		rec->trace[rec->trace_count].ticks = ticks;
		rec->trace[rec->trace_count].err = err;
	}
#endif

	switch (rec->state) {
	case START:
		if (rec->measure++ >= MCLOCK_REC_PLL_NB_MEAS) {
			rec->state = ADJUST;
			rational_init(&rec->clk_media, dev->clk_timer , 1);
		}
		break;
	case ADJUST:
		if (!err) {
			/* After few consecutive 0 measurements, we can go to locked*/
			if (rec->zero_err_nb++ >= MCLOCK_REC_PLL_LOCKED_ERR_NB && !(dev->flags & MCLOCK_FLAGS_RUNNING_LOCKED)) {
				rtos_atomic_set(&rec->status, MCLOCK_RUNNING_LOCKED);
				dev->flags |= MCLOCK_FLAGS_RUNNING_LOCKED;
			}
		} else
			rec->zero_err_nb = 0;

		rational_add(&rec->clk_media, &rec->clk_media, &rec->fec_period);
		mclock_rec_pll_adjust(rec, err * rec->factor);

		/* Not needed to reset here there are enough checks before*/
		if (mclock_drift_adapt(&rec->dev, rec->clk_media.i) < 0)
			rec->stats.err_drift++;
		break;
	default:
		break;
	}

out:
#if MCLOCK_PLL_REC_TRACE
	if (!rec->trace_freeze) {
		rec->trace[rec->trace_count].state = rec->state;
		if (++rec->trace_count > MCLOCK_REC_TRACE_SIZE - 1)
		       rec->trace_freeze = 1;
	}
#endif

out_no_trace:
	if (rec->state == RESET)
		mclock_rec_pll_reset(rec);

	return rc;
}

int mclock_rec_pll_config(struct mclock_dev *dev, struct mclock_sconfig *cfg)
{
	int rc = 0;
	struct mclock_rec_pll *rec = container_of(dev, struct mclock_rec_pll, dev);

	switch (cfg->cmd) {
	case MCLOCK_CFG_WAKE_FREQ:
		rc = mclock_wake_up_configure(dev, cfg->wake_freq.p, cfg->wake_freq.q);
		break;


	case MCLOCK_CFG_TS_SRC:
		if (dev->flags & MCLOCK_FLAGS_RUNNING) {
			rc = -1;
			goto exit;
		}

		if (mclock_rec_pll_configure_freqs(rec, cfg->ts_src, dev->ts_freq_p, dev->ts_freq_q) < 0) {
			rc = -1;
			os_log(LOG_ERR, "mclock_rec_pll_configure_freqs error\n");
			goto exit;
		}

		dev->ts_src = cfg->ts_src;
		break;

	case MCLOCK_CFG_FREQ:
		if (dev->flags & MCLOCK_FLAGS_RUNNING) {
			rc = -1;
			goto exit;
		}

		if (dev->ts_src == TS_INTERNAL) {
			rc = -1;
			os_log(LOG_ERR, "cannot set ts freq in internal mode\n");
			goto exit;
		}
		else if (dev->ts_src == TS_EXTERNAL) {

			if (mclock_rec_pll_configure_freqs(rec, dev->ts_src, cfg->ts_freq.p, cfg->ts_freq.q) < 0) {
				rc = -1;
				os_log(LOG_ERR, "mclock_rec_pll_configure_freqs error\n");
				goto exit;

			}
			mclock_set_ts_freq(dev, cfg->ts_freq.p, cfg->ts_freq.q);
		}
		else
			rc = -1;

		break;

	default:
		rc = -1;
		break;
	}

exit:
	return rc;
}

__init int mclock_rec_pll_init(struct mclock_rec_pll *rec)
{
	struct mclock_dev *dev = &rec->dev;
	int rc = 0;

	mclock_rec_pll_configure_freqs(rec, TS_INTERNAL, 0, 0);

	/* For rec_pll, the ts freq holds value for the external ts
	  Set the default value here */
	mclock_set_ts_freq(&rec->dev, MCLOCK_REC_TS_FREQ_INIT, 1);

	rec->gptp_event_dev = gptp_event_init(GPTP_ENET_DEV_INDEX);
	if (!rec->gptp_event_dev) {
		rc = -1;
		os_log(LOG_ERR, "gptp_event_init failed\n");
		goto exit;
	}

	dev->sh_mem = rtos_malloc(MCLOCK_REC_MMAP_SIZE);
	if (!dev->sh_mem) {
		rc = -1;
		os_log(LOG_ERR, "rtos_malloc failed\n");
		goto err_malloc;
	}

	dev->w_idx = (unsigned int *)((char *)dev->sh_mem + MCLOCK_REC_BUF_SIZE);
	dev->sh_mem_size = MCLOCK_REC_MMAP_SIZE;
	dev->num_ts = MCLOCK_REC_NUM_TS;
	dev->timer_period = NET_RX_TX_PERIOD;
#if MCLOCK_PLL_REC_TRACE
	rec->trace_count = 0;
	rec->trace_freeze = 0;
#endif

	mclock_register_device(dev);

err_malloc:
	gptp_event_exit(rec->gptp_event_dev);
exit:
	return rc;
}

__exit void mclock_rec_pll_exit(struct mclock_rec_pll *rec)
{
	mclock_unregister_device(&rec->dev);

	rtos_free(rec->dev.sh_mem);
}
