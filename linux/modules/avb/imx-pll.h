/*
 * IMX Clock PLL

 * Copyright 2018, 2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _IMX_CLK_PLL_H_
#define _IMX_CLK_PLL_H_

#include <linux/clk.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/clk/imx-pll.h>

struct imx_pll {
	int ppb_adjust; /* Exact ppb adjustment returned from the PLL control layer */
	unsigned long parent_rate; /* OSC Reference clock rate */
	unsigned long current_rate;
	struct clk *clk_audio_pll;
	struct device *timer_dev;
	struct clk_imx_pll *clk_pll;
};

int imx_pll_init(struct imx_pll *pll, struct device *dev, bool has_parent_clk);
int imx_pll_adjust(struct imx_pll *pll, int *ppb);
unsigned long imx_pll_get_rate(struct imx_pll *pll);
void imx_pll_deinit(struct imx_pll *pll);

#endif /* _IMX_CLK_PLL_H_ */
