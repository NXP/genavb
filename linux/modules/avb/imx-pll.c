/*
 * IMX Clock pll common functions

 * Copyright 2018, 2023 NXP.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "imx-pll.h"
#include <linux/io.h>
#include <linux/clk-provider.h>

/**
 * imx_pll_init() - Initializes the imx_pll struct through the driver device node
 * @p - pointer to imx_pll structure
 * @dev - pointer to the timer device struct
 * @has_parent_clk - boolean to check if audio pll clock has a parent in the clock tree or not
 *
 * Returns 0 on success or negative error.
 */

int imx_pll_init(struct imx_pll *pll, struct device *dev, bool has_parent_clk)
{
	int rc = 0;

	pll->timer_dev = dev;
	pll->parent_rate = 0;

	pll->clk_audio_pll = devm_clk_get(dev, "audio_pll"); //audio pll
	if (IS_ERR(pll->clk_audio_pll)) {
		rc = PTR_ERR(pll->clk_audio_pll);
		goto err;
	}

	clk_prepare_enable(pll->clk_audio_pll);

	pll->clk_pll = clk_imx_pll_get_by_name(__clk_get_name(pll->clk_audio_pll));
	if (!pll->clk_pll) {
		rc = -EINVAL;
		pr_err("%s: Can not get the imx pll for %s \n", __func__, __clk_get_name(pll->clk_audio_pll));
		goto err_clk;
	}

	/* Devices with SCU: do not have audio pll clock parent. */
	if (has_parent_clk) {
		pll->parent_rate = clk_get_rate(clk_get_parent(pll->clk_audio_pll));
		if (!pll->parent_rate) {
			rc = -EINVAL;
			goto err_clk;
		}
	}

	pll->current_rate = clk_imx_pll_get_rate(pll->clk_pll, pll->parent_rate);
	if (!pll->current_rate) {
		rc = -EINVAL;
		pr_err("%s: Can not get the audio pll rate \n", __func__);
		goto err_clk;
	}

	/* Reset the ppb adjust value: PLL running at nominal frequency. */
	pll->ppb_adjust = 0;

	return 0;
err_clk:
	clk_disable_unprepare(pll->clk_audio_pll);
err:
	return rc;
}

int imx_pll_adjust(struct imx_pll *pll, int *ppb)
{
	return clk_imx_pll_adjust(pll->clk_pll, ppb);
}

unsigned long imx_pll_get_rate(struct imx_pll *pll)
{
	return clk_imx_pll_get_rate(pll->clk_pll, pll->parent_rate);
}

void imx_pll_deinit(struct imx_pll *pll)
{
	int rc;
	int ppb = 0;

	/* Reset pll adjustment */
	rc = clk_imx_pll_adjust(pll->clk_pll, &ppb);
	if (rc < 0)
		pr_err("%s: Can not reset audio pll adjustment \n", __func__);
	else
		pll->ppb_adjust = 0;

	clk_disable_unprepare(pll->clk_audio_pll);
}
