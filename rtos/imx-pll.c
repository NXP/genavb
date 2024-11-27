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

#include "imx-pll.h"
#include "config.h"

struct clk_imx_pll clk_pll_devices[PLL_MAX_DEVICES] = {
	[0] = {0}
};

static struct clk_imx_pll *clk_get_device(unsigned int index)
{
	return &clk_pll_devices[index];
}

static int clk_get_rate(struct clk_imx_pll *pll)
{
	return dev_get_pll_ref_freq();
}

/**
 * 1) pllout_new = pllout * (1 + ppb/1e9)
 * 2) pllout_new = parent * (div + new_num/denom)
 * 	1 = 2 => new_num = (div * ppb * denom + num * 1e9 + num * ppb) / (1e9)
 */
static int clk_imx_pll_adjust(struct clk_imx_pll *pll, int *ppb)
{
	uint64_t temp64;
	int32_t applied_ppb;
	int rc = 0;

	int req_ppb = *ppb;

	/*Calcultate the new PLL Numerator*/
	temp64 = (uint64_t)pll->orig_denom * pll->orig_div * req_ppb + (uint64_t)pll->orig_num * 1000000000 + (uint64_t)pll->orig_num * req_ppb;

	temp64 /= 1000000000;

	if (temp64 >= pll->orig_denom) {
		rc = -1;
		goto exit;
	}

	/*Write the new PLL num*/
	dev_write_audio_pll_num((uint32_t)temp64);

	/*Calculate and return the actual applied ppb*/
	applied_ppb = ((int64_t) (temp64 - pll->orig_num) * 1000000000) / (pll->orig_num + (int64_t)pll->orig_denom * pll->orig_div);

	*ppb = (int)applied_ppb;

exit:
	return rc;
}

__init static void clk_imx_pll_init(struct clk_imx_pll *pll)
{
	pll->orig_num   = dev_read_audio_pll_num();
	pll->orig_denom = dev_read_audio_pll_denom();
	pll->orig_div   = dev_read_audio_pll_post_div();
}

/**
 * imx_pll_init() - Initializes the imx_pll struct through the driver device node
 * @p - pointer to imx_pll structure
 * @dev - pointer to the timer device struct
 *
 * Returns 0 on success or negative error.
 */
__init int imx_pll_init(struct imx_pll *pll)
{
	int rc = 0;

	pll->clk_audio_pll = clk_get_device(AUDIO_PLL_DEVICE);
	if (!pll->clk_audio_pll) {
		rc = -1;
		goto out;
	}

	clk_imx_pll_init(pll->clk_audio_pll);

	pll->parent_rate = clk_get_rate(pll->clk_audio_pll);

out:
	return rc;
}

int imx_pll_adjust(struct imx_pll *pll, int *ppb)
{
	return clk_imx_pll_adjust(pll->clk_audio_pll, ppb);
}

unsigned long imx_pll_get_rate(struct imx_pll *pll)
{
	uint32_t mfn = dev_read_audio_pll_num();
	uint32_t mfd = dev_read_audio_pll_denom();
	uint32_t div = dev_read_audio_pll_post_div();
	uint64_t temp64 = (uint64_t)pll->parent_rate;

	temp64 *= mfn;
	temp64 /= mfd;

	return (pll->parent_rate * div) + (uint32_t)temp64;
}

__exit void imx_pll_deinit(struct imx_pll *pll)
{
	int rc;
	int ppb = 0;

	/* Reset pll adjustment*/
	rc = clk_imx_pll_adjust(pll->clk_audio_pll, &ppb);
	if (rc < 0)
		os_log(LOG_ERR, "Cannot reset audio pll adjustment\n");
}
