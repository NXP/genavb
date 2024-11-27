/*
 * Copyright 2018-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief IMX Clock pll common functions
 @details
*/

#ifndef _IMX_CLK_PLL_H_
#define _IMX_CLK_PLL_H_

#include "common/log.h"

enum clk_imx_pll_error {
	IMX_CLK_PLL_SUCCESS = 0,
	IMX_CLK_PLL_INVALID_PARAM,
	IMX_CLK_PLL_PREC_ERR,
};

#define PLL_MAX_DEVICES		1
#define AUDIO_PLL_DEVICE	0

struct clk_imx_pll {
	uint32_t orig_num;
	uint32_t orig_denom;
	uint32_t orig_div;
};

/* The PI controller was tuned (proportional gain and integral gain) to work for a control
 * output as a numerator variation in the PLL output equation.
 * Moving to a control output of a ppb varitaion we need to adjust these coefficients:
 * 1 numerator variation was giving a 24Hz variation in the PLL output frequency (786432000 Hz) which
 * is around 30 ppb*/
#define IMX_PLL_ADJUST_FACTOR	30

struct imx_pll {
	unsigned long parent_rate; /*OSC Reference clock rate*/
	unsigned long current_rate;
	struct clk_imx_pll *clk_audio_pll;
};

int imx_pll_init(struct imx_pll *pll/*, struct device *dev*/);
int imx_pll_adjust(struct imx_pll *pll, int *ppb);
unsigned long imx_pll_get_rate(struct imx_pll *pll);
void imx_pll_deinit(struct imx_pll *pll);

#endif /* _IMX_CLK_PLL_H_ */
