/*
* Copyright 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief AVB TPM media clock recovery driver
 @details
*/

#include "tpm_rec.h"
#include "imx-pll.h"
#include "media_clock_rec_pll.h"

#if defined(CONFIG_AVTP) && (CFG_NUM_TPM > 0) && defined(BOARD_TPM_REC_BASE)

#include "fsl_tpm.h"

#define TPM_REC_FACTOR		16
#define TPM_REC_P_FACTOR	1
#define TPM_REC_I_FACTOR	3
#define TPM_REC_MAX_ADJUST	100
#define TPM_REC_NB_START	10

#define DOMAIN_0		0

struct tpm_input_capture {
	tpm_chnl_t channel;
	tpm_interrupt_enable_t interrupt_mask;
	tpm_status_flag_t status_flag_mask;
	tpm_input_capture_edge_t operation_mode;
};

struct tpm_rec {
	void *base;
	struct mclock_rec_pll rec;
	struct tpm_input_capture capture;

	uint32_t cnt_last;
	unsigned int eth_port;
};

static struct tpm_rec tpm_rec = {
	.base = BOARD_TPM_REC_BASE,
	.capture = {
#if (BOARD_TPM_REC_CHANNEL == 0)
		.channel = kTPM_Chnl_0,
		.interrupt_mask = kTPM_Chnl0InterruptEnable,
		.status_flag_mask = kTPM_Chnl0Flag,
#elif (BOARD_TPM_REC_CHANNEL == 1)
		.channel = kTPM_Chnl_1,
		.interrupt_mask = kTPM_Chnl1InterruptEnable,
		.status_flag_mask = kTPM_Chnl1Flag,
#elif (BOARD_TPM_REC_CHANNEL == 2)
		.channel = kTPM_Chnl_2,
		.interrupt_mask = kTPM_Chnl2InterruptEnable,
		.status_flag_mask = kTPM_Chnl2Flag,
#else
#error Invalid BOARD_TPM_REC_CHANNEL
#endif
		.operation_mode = kTPM_RiseAndFallEdge,
	},
};

static int tpm_rec_timer_irq(struct mclock_dev *dev, void *data, unsigned int ticks)
{
	struct mclock_rec_pll *rec = container_of(dev, struct mclock_rec_pll, dev);
	struct tpm_rec *tpm_rec = container_of(rec, struct tpm_rec, rec);
	uint32_t cnt_val;

	/* get status */
	if (TPM_GetStatusFlags(tpm_rec->base) & tpm_rec->capture.status_flag_mask) {
		/* get value */
		cnt_val = TPM_GetChannelValue(tpm_rec->base, tpm_rec->capture.channel);

		mclock_rec_pll_timer_irq(rec, 1, (uint32_t)(cnt_val - tpm_rec->cnt_last), ticks);
		tpm_rec->cnt_last = cnt_val;

		/* clear status bit
		 * Do this _after_ mclock_rec_pll_timer_irq because internal resets
		 * can cause FEC ENET compare output signal toggling.
		 */
		TPM_ClearStatusFlags(tpm_rec->base, tpm_rec->capture.status_flag_mask);

		rec->stats.irq_count_fec_event++;
	} else {
		mclock_rec_pll_timer_irq(rec, 0, 0, ticks);
	}

	rec->stats.irq_count++;

	return 0;
}

static int tpm_rec_start(struct mclock_dev *dev, struct mclock_start *start)
{
	struct mclock_rec_pll *rec = container_of(dev, struct mclock_rec_pll, dev);
	int rc = 0;

	/* Start ENET */
	mclock_rec_pll_start(rec, start);

	rc = mclock_register_timer(dev, tpm_rec_timer_irq, 1);
	if (rc < 0) {
		mclock_rec_pll_stop(rec);
		os_log(LOG_ERR, "cannot register timer\n");
		goto out;
	}

out:
	return rc;
}

static int tpm_rec_stop(struct mclock_dev *dev)
{
	struct mclock_rec_pll *rec = container_of(dev, struct mclock_rec_pll, dev);
	int rc = 0;

	mclock_unregister_timer(dev);

	rc = mclock_rec_pll_stop(rec);
	if (rc < 0)
		os_log(LOG_ERR, "cannot stop enet\n");

	return rc;
}

static int tpm_rec_reset(struct mclock_dev *dev)
{
	return 0;
}

static int tpm_rec_open(struct mclock_dev *dev, int port)
{
	struct mclock_rec_pll *rec = container_of(dev, struct mclock_rec_pll, dev);
	struct tpm_rec *tpm_rec = container_of(rec, struct tpm_rec, rec);

	dev->eth = &ports[tpm_rec->eth_port];
	if (!dev->eth)
		return -1;

	return 0;
}

__init int tpm_rec_init(void)
{
	struct mclock_rec_pll *rec = &tpm_rec.rec;
	struct mclock_dev *clock_dev = &tpm_rec.rec.dev;
	unsigned int input_clk_rate, divider;

	input_clk_rate = BOARD_TPM_clk_freq(tpm_rec.base);
	divider = (1 << ((tpm_rec.base->SC & TPM_SC_PS_MASK) >> TPM_SC_PS_SHIFT));

	TPM_SetupInputCapture(tpm_rec.base, tpm_rec.capture.channel, tpm_rec.capture.operation_mode);

	if (imx_pll_init(&rec->pll) < 0) {
		os_log(LOG_ERR, "cannot init the audio PLL\n");
		goto err;
	}

	clock_dev->domain = DOMAIN_0;

	rec->pll_ref_freq = input_clk_rate / divider;
	rec->max_adjust = TPM_REC_MAX_ADJUST * IMX_PLL_ADJUST_FACTOR;
	pi_init(&rec->pi, TPM_REC_I_FACTOR, TPM_REC_P_FACTOR);
	rec->factor = TPM_REC_FACTOR * IMX_PLL_ADJUST_FACTOR;

	// FIXME register 2 devices, a REC and a GEN
	clock_dev->type = REC;
	clock_dev->ts_src = TS_INTERNAL;
	clock_dev->start = tpm_rec_start;
	clock_dev->stop = tpm_rec_stop;
	clock_dev->reset = tpm_rec_reset;
	clock_dev->clean = mclock_rec_pll_clean_get;
	clock_dev->open = tpm_rec_open;
	clock_dev->release = NULL;
	clock_dev->config = mclock_rec_pll_config;

	if (mclock_rec_pll_init(rec) < 0) {
		os_log(LOG_ERR, "mclock_rec_pll_init error\n");
		goto err_rec_pll;
	}

	os_log(LOG_INIT, "TPM rec device(%p), clk input: %ld Hz, pll frequency %lu, pll parent (osc) frequency %lu\n",
		clock_dev, input_clk_rate,
		imx_pll_get_rate(&rec->pll), rec->pll.parent_rate);

	return 0;

err_rec_pll:
	imx_pll_deinit(&rec->pll);

err:
	return -1;
}

__exit void tpm_rec_exit(void)
{
	mclock_rec_pll_exit(&tpm_rec.rec);
	imx_pll_deinit(&tpm_rec.rec.pll);
}
#else
__init int tpm_rec_init(void) { return -1; }
__exit void tpm_rec_exit(void) { }
#endif
