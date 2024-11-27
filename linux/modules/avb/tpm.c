/*
 * AVB TPM driver
 * Copyright 2022-2024 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/fec.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/regmap.h>

#include "tpm.h"
#include "imx-pll.h"
#include "net_port.h"
#include "avbdrv.h"
#include "hw_timer.h"
#include "media_clock_rec_pll.h"

/* TPM registers offsets */
#define TPM_PARAM                       0x4
#define TPM_SC                          0x10
#define TPM_CNT                         0x14
#define TPM_MOD                         0x18
#define TPM_STATUS                      0x1c
#define TPM_CnSC(ch)                    (0x20 + (ch) * 8)
#define TPM_CnV(ch)                     (0x24 + (ch) * 8)

/* TPM_PARAM */
#define TPM_PARAM_WIDTH_SHIFT           16
#define TPM_PARAM_WIDTH_MASK            (0xff << TPM_PARAM_WIDTH_SHIFT)

/* TPM_SC */
#define TPM_PRESCALER(val)	        (((val) & 0x7))
#define TPM_CMOD(val)	                (((val) & 0x3) << 3)
#define TPM_CPWMS	                BIT(5)
#define TPM_TOIE	                BIT(6)
#define TPM_TOF	                	BIT(7)
#define TPM_DMA	                	BIT(8)

/* TPM_CnSC*/
#define TPM_CnSC_CHIE                       BIT(6)
#define TPM_CnSC_CHF                        BIT(7)
#define TPM_CnSC_MODE(val)                  (((val) & 0xf) << 2)
#define TPM_CnSC_MODE_SW_COMPARE            0x4
#define TPM_CnSC_MODE_DISABLED              0x0
#define TPM_CnSC_MODE_INPUT_CAPTURE_BOTH    0x3

/* TPM clock source types */
#define TPM_CLKSRC_DISABLED		0x0	/* No clock */
#define TPM_CLKSRC_IPG			0x1	/* Peripheral clock */
#define TPM_CLKSRC_EXT_CLK		0x2	/* External Clock */

/* TPM_STATUS */
#define TPM_STATUS_CHnF(ch)             (1 << (ch))

#define TPM_REC_TIMER_PERIOD_MS 1
#define TPM_REC_TIMER_PERIOD_NS	(TPM_REC_TIMER_PERIOD_MS * NSEC_PER_MSEC)

#define TPM_REC_MAX_ADJUST_PPB	200000 /* Max 200ppm adjustement in a single step. */

#define TPM_HW_TIMER_MODE	(1 << 0)
#define TPM_MCLOCK_REC_MODE	(1 << 1)
#define TPM_MCLOCK_SW_REC_MODE	(1 << 2)

#define TPM_MAX_CH_ID	3
#define TPM_MIN_CH_ID	0

struct tpm_drv {
	void *baseaddr;
	int irq;

	resource_size_t start;
	resource_size_t size;

	struct clk *clk_tpm;
	struct clk *clk_ipg;

	int prescale;

	int func_mode;
	struct hw_timer_dev timer_dev;
	u32 next_cycles;

	struct mclock_rec_pll rec;
	unsigned int cap_channel;
	unsigned int timer_channel;
	unsigned int eth_port;
	u32 counter_width;
	u32 counter_mask;
	u32 cnt_last;
};

/**
 * DOC: TPM driver which handles HW timer and a PLL based recovery.
 *
 */

int tpm_rec_timer_irq(struct mclock_dev *dev, void *data, unsigned int ticks)
{
	struct mclock_rec_pll *rec = container_of(dev, struct mclock_rec_pll, dev);
	struct tpm_drv *drv = container_of(rec, struct tpm_drv, rec);
	unsigned long flags;
	u32 tpm_status, cnt_val;

	raw_spin_lock_irqsave(&dev->eth->lock, flags);

	/* get status  */
	tpm_status = readl_relaxed(drv->baseaddr + TPM_STATUS);
	if (tpm_status & TPM_STATUS_CHnF(drv->cap_channel)) {
		/* get value */
		cnt_val = readl_relaxed(drv->baseaddr + TPM_CnV(drv->cap_channel));

		mclock_rec_pll_timer_irq(rec, 1, ((u32)(cnt_val - drv->cnt_last) & drv->counter_mask), cnt_val, ticks);
		drv->cnt_last = cnt_val;

		/* clear status bit
		 * Do this _after_ mclock_rec_pll_timer_irq because internal resets
		 * can cause FEC ENET compare output signal toggling.
		 */
		writel_relaxed(TPM_STATUS_CHnF(drv->timer_channel), drv->baseaddr + TPM_STATUS);
	} else {
		mclock_rec_pll_timer_irq(rec, 0, 0, 0, ticks);
	}

	raw_spin_unlock_irqrestore(&dev->eth->lock, flags);

	return 0;
}

int tpm_sw_rec_timer_irq(struct mclock_dev *dev, void *data, unsigned int ticks)
{
	struct mclock_rec_pll *rec = container_of(dev, struct mclock_rec_pll, dev);
	struct tpm_drv *drv = container_of(rec, struct tpm_drv, rec);
	struct eth_avb *eth = rec->dev.eth;
	unsigned long flags;
	u32 cnt_val;
	unsigned int ptp_now;
	int rc;

	raw_spin_lock_irqsave(&dev->eth->lock, flags);

	if (!(eth->flags & PORT_FLAGS_ENABLED)) {
		rec->stats.err_port_down++;
		goto unlock_out;
	}

	/* Sample ptp counter and audio pll cycles "simultaneously".
	 * NOTE: Read TPM counter first to avoid jitter from the udelay() in the ptp read counter function.
	 * TODO: Reduce number of  gPTP and TPM counter reads (only on sampling periods) and use ptp_now passed
	 *       from hw timer interrupt handler to compare with timestamp on every timer interrupt.
	 */
	cnt_val = readl_relaxed(drv->baseaddr + TPM_CNT);
	rc = fec_ptp_read_cnt(eth->fec_data, &ptp_now);

	if (rc >= 0)
		mclock_rec_pll_sw_sampling_irq(rec, cnt_val, ptp_now, ticks);

unlock_out:
	raw_spin_unlock_irqrestore(&dev->eth->lock, flags);

	return 0;
}

int tpm_rec_start(struct mclock_dev *dev, struct mclock_start *start)
{
	struct mclock_rec_pll *rec = container_of(dev, struct mclock_rec_pll, dev);
	struct tpm_drv *drv = container_of(rec, struct tpm_drv, rec);
	u32 tpm_cnsc_rec;
	int rc = 0;

	if (rec->is_hw_recovery_mode) {
		/*
		 *  - Input Capture on both edges mode
		 *  - Clear event channel flag (w1c)
		 */
		tpm_cnsc_rec = TPM_CnSC_CHF | TPM_CnSC_MODE(TPM_CnSC_MODE_INPUT_CAPTURE_BOTH);

		writel_relaxed(tpm_cnsc_rec, drv->baseaddr + TPM_CnSC(drv->cap_channel));
	}

	/* Start ENET */
	mclock_rec_pll_start(rec, start);

	if (rec->is_hw_recovery_mode)
		rc = mclock_drv_register_timer(dev, tpm_rec_timer_irq, 1);
	else
		rc = mclock_drv_register_timer(dev, tpm_sw_rec_timer_irq, 1);

	if (rc < 0) {
		mclock_rec_pll_stop(rec);
		pr_err("%s : cannot register timer\n", __func__);
		goto out;
	}

out:
	return rc;
}

int tpm_rec_stop(struct mclock_dev *dev)
{
	struct mclock_rec_pll *rec = container_of(dev, struct mclock_rec_pll, dev);
	struct tpm_drv *drv = container_of(rec, struct tpm_drv, rec);
	u32 tpm_cnsc_rec;
	int rc = 0;

	if (rec->is_hw_recovery_mode) {
		/* Clear CHF (Write-1-clear), Disable channel */
		tpm_cnsc_rec = TPM_CnSC_CHF | TPM_CnSC_MODE(0);

		writel_relaxed(tpm_cnsc_rec, drv->baseaddr + TPM_CnSC(drv->cap_channel));
	}

	mclock_drv_unregister_timer(dev);

	rc = mclock_rec_pll_stop(rec);
	if (rc < 0)
		pr_err("%s : cannot stop enet\n", __func__);

	return rc;
}

int tpm_rec_reset(struct mclock_dev *dev)
{
	return 0;
}

int tpm_rec_open(struct mclock_dev *dev, int port)
{
	return 0;
}

static void tpm_hw_timer_start(struct hw_timer_dev *dev)
{
	struct tpm_drv *tpm = platform_get_drvdata(dev->pdev);
	u32 cycles_now, tpm_cnsc_timer;

	/*
	 *  - Software compare mode
	 *  - Enable channel interrupt
	 *  - Clear event channel flag (w1c)
	 */
	tpm_cnsc_timer = TPM_CnSC_CHF | TPM_CnSC_MODE(TPM_CnSC_MODE_SW_COMPARE) | TPM_CnSC_CHIE;

	writel_relaxed(tpm_cnsc_timer, tpm->baseaddr + TPM_CnSC(tpm->timer_channel));

	cycles_now = readl_relaxed(tpm->baseaddr + TPM_CNT);

	tpm->next_cycles = (cycles_now + (u32) dev->period) & tpm->counter_mask;

	writel_relaxed(tpm->next_cycles, tpm->baseaddr + TPM_CnV(tpm->timer_channel));
}

static void tpm_hw_timer_stop(struct hw_timer_dev *dev)
{
	struct tpm_drv *drv = platform_get_drvdata(dev->pdev);
	u32 tpm_cnsc_timer;

	/* Clear CHF (Write-1-clear), Disable channel */
	tpm_cnsc_timer = TPM_CnSC_CHF | TPM_CnSC_MODE(0);

	writel_relaxed(tpm_cnsc_timer, drv->baseaddr + TPM_CnSC(drv->timer_channel));
}


static void tpm_hw_timer_set_period(struct hw_timer_dev *dev, unsigned long period_us)
{
	struct tpm_drv *tpm = platform_get_drvdata(dev->pdev);

	dev->period = (period_us * (dev->rate / 1000)) / 1000;

	if (((dev->period * 1000) / (dev->rate / 1000)) != period_us)
		pr_warn("%s: dev(%p) warning the HW timer period is incorrect",
			__func__, dev);

	pr_info("tpm_hw (%p) set period %lu(us), %u(cycles)\n", tpm, period_us, dev->period);
}

static unsigned int tpm_hw_timer_elapsed(struct hw_timer_dev *dev, unsigned int cycles)
{
	struct tpm_drv *drv = platform_get_drvdata(dev->pdev);
	u32 cycles_now;

	cycles_now = readl_relaxed(drv->baseaddr + TPM_CNT);

	return (cycles_now - (u32)cycles) & drv->counter_mask;
}

static unsigned int tpm_hw_timer_irq_ack(struct hw_timer_dev *dev, unsigned int *cycles, unsigned int *dcycles)
{
	struct tpm_drv *drv = platform_get_drvdata(dev->pdev);
	unsigned int ticks = 0;
	u32 cycles_now, num_recovery = 0;

	cycles_now = readl_relaxed(drv->baseaddr + TPM_CNT);

	*cycles = cycles_now;

try_again:
	writel_relaxed(TPM_STATUS_CHnF(drv->timer_channel), drv->baseaddr + TPM_STATUS);

	*dcycles = (cycles_now - drv->next_cycles) & drv->counter_mask;

	/* Check how many periods have elapsed since last time
	 * Normally a single period has elapsed, but we may be late */
	do {
		drv->next_cycles = (drv->next_cycles + (u32)dev->period) & drv->counter_mask;
		ticks++;
	} while ((int)((drv->next_cycles - cycles_now) & drv->counter_mask) < 0);

	/* Set next timer value */
	writel_relaxed(drv->next_cycles, drv->baseaddr + TPM_CnV(drv->timer_channel));

	/* Check if the free running counter has already passed
	the newly programmed target value */
	cycles_now = readl_relaxed(drv->baseaddr + TPM_CNT);

	if ((int)((cycles_now - drv->next_cycles) & drv->counter_mask) > 0) {
		num_recovery++;

		/* hard protection against potential dead loop (> 2*125us) */
		if (num_recovery > 2)
			pr_err("drv(%p) - recovery failure\n", drv);
		else
			/* try to set again the CnV above CNT */
			goto try_again;
	}

	dev->recovery_errors += num_recovery;

	return ticks;
}

static void tpm_reg_init(struct tpm_drv *drv)
{
	u32 tpm_sc, tpm_cnsc_timer;
	int chan;

	/* Disable counter and Clear TOF (Write-1-Clear) */
	writel_relaxed(TPM_TOF | TPM_CMOD(TPM_CLKSRC_DISABLED), drv->baseaddr + TPM_SC);

	/* Clear CHF (Write-1-clear) and disable all channels */
	tpm_cnsc_timer = TPM_CnSC_CHF | TPM_CnSC_MODE(TPM_CnSC_MODE_DISABLED);

	for (chan = 0; chan < TPM_MAX_CH_ID; chan++)
		writel_relaxed(tpm_cnsc_timer, drv->baseaddr + TPM_CnSC(chan));

	/* Clear Counter */
	writel_relaxed(0, drv->baseaddr + TPM_CNT);

	/* Read the counter width */
	drv->counter_width = (readl_relaxed(drv->baseaddr + TPM_PARAM) & TPM_PARAM_WIDTH_MASK) >> TPM_PARAM_WIDTH_SHIFT;

	drv->counter_mask = GENMASK(drv->counter_width - 1, 0);

	/* set MOD register to maximum */
	writel_relaxed(drv->counter_mask, drv->baseaddr + TPM_MOD);

	/* Enable counter, peripheral clock source, upcounting mode and set prescale factor */
	tpm_sc = TPM_PRESCALER(drv->prescale - 1) | TPM_CMOD(TPM_CLKSRC_IPG);

	writel_relaxed(tpm_sc, drv->baseaddr + TPM_SC);
}

static int tpm_probe(struct platform_device *pdev)
{
	struct tpm_drv *drv;
	struct resource *iores;
	struct mclock_rec_pll *rec = NULL;
	struct mclock_dev *clock_dev = NULL;
	struct hw_timer_dev *timer_dev = NULL;
	struct imx_pll *pll = NULL;
	int rc = 0;

	drv = devm_kzalloc(&pdev->dev, sizeof(*drv), GFP_KERNEL);
	if (!drv) {
		rc = -ENOMEM;
		goto err;
	}

	if (of_find_property(pdev->dev.of_node, "rec-channel", NULL)) {
		rec = &drv->rec;
		clock_dev = &rec->dev;
		pll = &rec->pll;
		rec->is_hw_recovery_mode = true;
		drv->func_mode |= TPM_MCLOCK_REC_MODE;
	}

	if (!clock_dev && of_find_property(pdev->dev.of_node, "sw-recovery", NULL)) {
		rec = &drv->rec;
		clock_dev = &rec->dev;
		pll = &rec->pll;
		rec->is_hw_recovery_mode = false;
		drv->func_mode |= TPM_MCLOCK_SW_REC_MODE;
	}

	if (of_find_property(pdev->dev.of_node, "timer-channel", NULL)) {
		timer_dev = &drv->timer_dev;
		drv->func_mode |= TPM_HW_TIMER_MODE;
	}

	if (!(timer_dev || clock_dev)) {
		rc = -EINVAL;
		pr_err("%s: missing properties in device tree node\n", __func__);
		goto err;
	}

	iores = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!iores) {
		rc = -ENODEV;
		goto err;
	}

	drv->start = iores->start;
	drv->size = resource_size(iores);

	drv->irq = platform_get_irq(pdev, 0);
	if (drv->irq < 0) {
		rc = -ENODEV;
		goto err;
	}

	if (!devm_request_mem_region(&pdev->dev, drv->start, drv->size, pdev->name)) {
		rc = -EBUSY;
		goto err;
	}

	drv->baseaddr = devm_ioremap(&pdev->dev, drv->start, drv->size);
	if (!drv->baseaddr) {
		rc = -ENOMEM;
		goto err;
	}

	/* By default choose the peripheral clock as counter clock*/
	drv->clk_tpm = devm_clk_get(&pdev->dev, "per"); //counter clock
	if (IS_ERR(drv->clk_tpm)) {
		rc = PTR_ERR(drv->clk_tpm);
		goto err;
	}

	drv->clk_ipg = devm_clk_get(&pdev->dev, "ipg"); //gate
	if (IS_ERR(drv->clk_ipg)) {
		rc = PTR_ERR(drv->clk_ipg);
		goto err;
	}

	platform_set_drvdata(pdev, drv);

	clk_prepare_enable(drv->clk_tpm);
	clk_prepare_enable(drv->clk_ipg);

	if (of_property_read_u32(pdev->dev.of_node, "prescale", &drv->prescale)) {
		rc = -EINVAL;
		pr_err("%s: cannot get prescale \n", __func__);
		goto err_clk;
	}

	if (clock_dev) {

		if (drv->func_mode & TPM_MCLOCK_REC_MODE) {
			/* Get the HW recovery mode config */
			if (of_property_read_u32_index(pdev->dev.of_node, "rec-channel", 0, &drv->cap_channel)) {
				rc = -EINVAL;
				pr_err("%s: cannot get TPM capture channel id\n", __func__);
				goto err_clk;
			}

			if ((drv->cap_channel > TPM_MAX_CH_ID)) {
				rc = -EINVAL;
				pr_err("%s: TPM capture channel id (%u) out-of-range [%d,%d]\n", __func__, drv->cap_channel, TPM_MIN_CH_ID, TPM_MAX_CH_ID);
				goto err_clk;
			}

			if (of_property_read_u32_index(pdev->dev.of_node, "rec-channel", 1, &drv->eth_port)) {
				rc = -EINVAL;
				pr_err("%s: cannot get ethernet port\n", __func__);
				goto err_clk;
			}

			if (drv->eth_port >= CFG_PORTS) {
				rc = -EINVAL;
				pr_err("%s: The Eth port %u is out-of-range ( > %d)\n", __func__, drv->eth_port, CFG_PORTS - 1);
				goto err_clk;
			}

			if (of_property_read_u32_index(pdev->dev.of_node, "rec-channel", 2, &drv->rec.fec_tc_id)) {
				rc = -EINVAL;
				pr_err("%s: cannot get FEC tc id\n", __func__);
				goto err_clk;
			}
		} else {
			/* Get the SW recovery mode config */
			if (of_property_read_u32(pdev->dev.of_node, "sw-recovery", &drv->eth_port)) {
				rc = -EINVAL;
				pr_err("%s: cannot get ethernet port from sw-recovery\n", __func__);
				goto err_clk;
			}
		}

		if (of_property_read_u32(pdev->dev.of_node, "domain", &clock_dev->domain)) {
			rc = -EINVAL;
			pr_err("%s: cannot get domain\n", __func__);
			goto err_clk;
		}

		if (imx_pll_init(&drv->rec.pll, &pdev->dev, true) < 0) {
			rc = -EINVAL;
			pr_err("%s: cannot init the audio PLL\n", __func__);
			goto err_clk;
		}

		rec->pll_ref_freq = clk_get_rate(drv->clk_tpm) / drv->prescale;
		rec->max_adjust = TPM_REC_MAX_ADJUST_PPB;
		rec->pll_timer_clk_div = clk_get_rate(drv->rec.pll.clk_audio_pll) / rec->pll_ref_freq;

		//FIXME register 2 devices, a REC and a GEN
		clock_dev->type = REC;
		clock_dev->ts_src = TS_INTERNAL;
		clock_dev->start = tpm_rec_start;
		clock_dev->stop = tpm_rec_stop;
		clock_dev->reset = tpm_rec_reset;
		clock_dev->clean = mclock_rec_pll_clean_get;
		clock_dev->open = tpm_rec_open;
		clock_dev->release = NULL;
		clock_dev->config = mclock_rec_pll_config;
	}

	if (timer_dev) {

		if (of_property_read_u32(pdev->dev.of_node, "timer-channel", &drv->timer_channel)) {
			rc = -EINVAL;
			pr_err("%s: cannot get TPM output channel for HW timer \n", __func__);
			goto err_clk;
		}

		if ((drv->timer_channel > TPM_MAX_CH_ID)) {
			rc = -EINVAL;
			pr_err("%s: TPM output compare  channel id (%u) out-of-range [%d,%d]\n", __func__, drv->timer_channel, TPM_MIN_CH_ID, TPM_MAX_CH_ID);
			goto err_clk;
		}

		timer_dev->rate = clk_get_rate(drv->clk_tpm) / drv->prescale;
		timer_dev->min_delay_cycles = ((HW_TIMER_MIN_DELAY_US * timer_dev->rate) / USEC_PER_SEC);
		timer_dev->irq = drv->irq;
		timer_dev->start = tpm_hw_timer_start;
		timer_dev->stop = tpm_hw_timer_stop;
		timer_dev->irq_ack = tpm_hw_timer_irq_ack;
		timer_dev->elapsed = tpm_hw_timer_elapsed;
		timer_dev->set_period = tpm_hw_timer_set_period;
		timer_dev->pdev = pdev;

	}

	/* TPM channels can either be output or input */
	if (clock_dev && timer_dev && (drv->cap_channel == drv->timer_channel)) {
		rc = -EINVAL;
		pr_err("%s: TPM do not support same channel (%u) setup for HW timer and recovery\n", __func__, drv->timer_channel);
		goto err_clk;

	}

	/* Do init config before registering/starting any device*/
	tpm_reg_init(drv);

	if (clock_dev) {
		rc = mclock_rec_pll_init(rec, drv->eth_port);
		if (rc < 0) {
			pr_err("%s: mclock_rec_pll_init(%u) error: %d\n", __func__, drv->eth_port, rc);
			goto err_rec_pll;
		}

		pr_info("%s: %s rec device(%p), domain: %d, sampling period: %d us, clk input: %ld Hz, pll frequency %lu pll parent (osc) frequency %lu pll tpm divider %u\n",
				__func__, drv->rec.is_hw_recovery_mode ? "HW" : "SW", clock_dev, clock_dev->domain, drv->rec.fec_period.i / 1000, clk_get_rate(drv->clk_tpm),
				drv->rec.pll.current_rate, drv->rec.pll.parent_rate, drv->rec.pll_timer_clk_div);
	}

	if (timer_dev) {

		hw_timer_register_device(timer_dev);

		pr_info("%s : TPM registered as HW timer (%p) device input clk = (%lu Hz) timer-channel = (%u)  prescale = (%d) \n",
			__func__, timer_dev, clk_get_rate(drv->clk_tpm), drv->timer_channel, drv->prescale);
	}

	return 0;

err_rec_pll:
	imx_pll_deinit(&drv->rec.pll);
err_clk:
	clk_disable_unprepare(drv->clk_tpm);
	clk_disable_unprepare(drv->clk_ipg);
err:
	return rc;
}

static int tpm_remove(struct platform_device *pdev)
{
	struct tpm_drv *drv = platform_get_drvdata(pdev);

	/* Disable counter and Clear TOF */
	writel_relaxed(TPM_TOF | TPM_CMOD(TPM_CLKSRC_DISABLED), drv->baseaddr + TPM_SC);

	if (drv->func_mode & (TPM_MCLOCK_REC_MODE | TPM_MCLOCK_SW_REC_MODE)) {
		mclock_rec_pll_exit(&drv->rec);
	}

	if (drv->func_mode & TPM_HW_TIMER_MODE)
		hw_timer_unregister_device(&drv->timer_dev);

	platform_set_drvdata(pdev, NULL);

	if (drv->func_mode & (TPM_MCLOCK_REC_MODE | TPM_MCLOCK_SW_REC_MODE))
		imx_pll_deinit(&drv->rec.pll);

	clk_disable_unprepare(drv->clk_tpm);
	clk_disable_unprepare(drv->clk_ipg);

	return 0;
}

#define TPM_AVB_DEVICE_COMPATIBLE	"fsl,avb-tpm"

static const struct of_device_id tpm_dt_ids[] = {
	{ .compatible = TPM_AVB_DEVICE_COMPATIBLE, },
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, tpm_dt_ids);

bool is_tpm_hw_timer_available(void)
{
	struct device_node *node;
	bool ret = false;

	node = of_find_compatible_node(NULL, NULL, TPM_AVB_DEVICE_COMPATIBLE);
	if (!node)
		goto exit;

	if (of_find_property(node, "timer-channel", NULL))
		ret = true;

exit:
	return ret;
}

int tpm_init(struct platform_driver *tpm_driver)
{
	tpm_driver->driver.name = "tpm";
	tpm_driver->driver.owner = THIS_MODULE;
	tpm_driver->driver.of_match_table = tpm_dt_ids;
	tpm_driver->probe = tpm_probe;
	tpm_driver->remove = tpm_remove;

	return platform_driver_register(tpm_driver);
}

void tpm_exit(struct platform_driver *tpm_driver)
{
	platform_driver_unregister(tpm_driver);
}
