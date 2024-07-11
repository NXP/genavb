/*
 * Copyright 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Generic filtering functions
 @details
*/

#include "filter.h"
#include "common/log.h"
#include "os/stdlib.h"
#include "os/string.h"

#define FILTER_EXP_DECAY_M_MAX (1 << 6)

/** Exponential decay coefficient lookup table
 * This avoid extreme values (coef=0 <-> M=+INF or coef=1 <-> M=0)
 * and spare the exponential computing from M to coef.
 * IEEE 802.1AS 11.1.2 "the weight of a past propagation delay
 * measurement is 1/e after M measurements" so the coef of the new value is:
 * Coef = 1-exp(-1/M)
 */
static const double filter_exp_decay_coef[6] = {
	0.6321205588, //1-exp(-1/1)
	0.3934693403, //1-exp(-1/2)
	0.2211992169, //1-exp(-1/4)
	0.1175030974, //1-exp(-1/8)
	0.0605869372, //1-exp(-1/16)
	0.0307667655, //1-exp(-1/32)
};


/** Identity  filter reset function.
 * @params: pointer to filter parameters
 * @val: sampling value to filter
 *
 */
static double filter_identity_filter(union filter_params *params, double val)
{
	return val;
}


/** Identity filter initialization function.
 * @params: pointer to filter
 *
 */
int filter_identity_init(struct filter *f)
{
	f->reset = NULL;
	f->close = NULL;
	f->filter = filter_identity_filter;

	return 0;
}


/** Mean filter reset function.
 * @params: pointer to filter parameters
 *
 */
static void filter_mean_reset(union filter_params *params)
{
	params->mean.count = 0;
	params->mean.mean = 0;
	params->mean.position = 0;
	os_memset(params->mean.array, 0, params->mean.array_size*sizeof(double));
}


/** Mean filter close function.
 * @params: pointer to filter parameters
 *
 */
static void filter_mean_close(union filter_params *params)
{
	os_free(params->mean.array);
}


/** Mean filter processing function.
 * @params: pointer to  filter parameters
 * @val: sampling value to filter
 */
static double filter_mean_filter(union filter_params *params, double val)
{
	params->mean.mean = params->mean.mean*params->mean.count - params->mean.array[params->mean.position] + val;
	if (params->mean.count < params->mean.array_size)
		params->mean.count++;
	params->mean.mean /= params->mean.count;

	params->mean.array[params->mean.position] = val;
	params->mean.position++;
	if (params->mean.position == params->mean.array_size)
		params->mean.position = 0;

	return params->mean.mean;
}

/** Mean filter initialization function.
 * @params: pointer to  filter parameters
 * @size: max numbers of samples the mean value is computed over
 *
 */
int filter_mean_init(struct filter *f, u32 size)
{
	int rc = 0;

	f->params.mean.array_size = size;
	f->params.mean.array = os_malloc(size*sizeof(double));
	if (!f->params.mean.array) {
		rc = -1;
		goto err_malloc;
	}

	f->reset = filter_mean_reset;
	f->close = filter_mean_close;
	f->filter = filter_mean_filter;

	f->reset(&f->params);

err_malloc:
	return rc;
}


/** Exponential decay  filter reset function.
 * @params: pointer to filter parameters
 *
 */
static void filter_exp_decay_reset(union filter_params *params)
{
	params->exp_decay.mean = 0;
	params->exp_decay.count = 1;
}


/** Exponential decay  filter processing function.
 * @params: pointer to filter parameters
 * @val: sampling value to filter
 *
 */
static double filter_exp_decay_filter(union filter_params *params, double val)
{
	if (params->exp_decay.count <= params->exp_decay.m_factor)
		params->exp_decay.mean = ((params->exp_decay.count - 1) * params->exp_decay.mean + val) / params->exp_decay.count;
	else
		params->exp_decay.mean = (params->exp_decay.mean * (1 - params->exp_decay.coef) + params->exp_decay.coef * val);

	params->exp_decay.count++;

	return params->exp_decay.mean;
}


/** Exponential decay  filter initialization function.
 * @params: pointer to filter
 * @coef: weigth of the sampling value added to the filter
 *
 */
int filter_exp_decay_init(struct filter *f, double coef)
{
	int i;

	if ((coef > 1.0) || (coef <= 0.0)) {
		os_log(LOG_ERR, "Invalid coefficient value (%f), defaulting to 1.\n", coef);
		f->params.exp_decay.coef = 1.0;
	} else {
		f->params.exp_decay.coef = coef;
	}

	f->params.exp_decay.m_factor = FILTER_EXP_DECAY_M_MAX;
	for (i = 0; i < 6; i++)
		if (coef > filter_exp_decay_coef[i]) {
			f->params.exp_decay.m_factor = 1 << i;
			break;
		}

	os_log(LOG_INFO, "Filter parameters -> M = %d   Coef = %f\n", f->params.exp_decay.m_factor, coef);

	f->reset = filter_exp_decay_reset;
	f->close = NULL;
	f->filter = filter_exp_decay_filter;

	f->reset(&f->params);

	return 0;
}


static void filter_fir_reset(union filter_params *params)
{
	params->fir.count = 0;
	params->fir.position = 0;
	os_memset(params->fir.array, 0, params->fir.array_size*sizeof(double));
}

static void filter_fir_close(union filter_params *params)
{
	os_free(params->fir.array);
}

static double filter_fir_filter(union filter_params *params, double val)
{
	double result = 0;
	u32 i, pos;

	/* Update the array of samples with the new value */
	params->fir.array[params->fir.position] = val;

	/* Not enough samples yet, return unfiltered value */
	if (params->fir.count < params->fir.array_size) {
		params->fir.count++;
		result = val;
	} else {
		pos = params->fir.position;
		for (i = 0; i < params->fir.array_size; i ++) {
			result += params->fir.taps_array[i] * params->fir.array[pos];
			pos = pos == 0? params->fir.array_size - 1:pos - 1;
		}

		result /= params->fir.taps_sum;
	}


	params->fir.position++;
	if (params->fir.position == params->fir.array_size)
		params->fir.position = 0;

	return result;
}

int filter_fir_init(struct filter *f, u32 size, double *filter_taps)
{
	u32 i;

	f->params.fir.array_size = size;
	f->params.fir.taps_array = filter_taps;
	f->params.fir.taps_sum = 0;
	for (i = 0; i < size; i++) {
		f->params.fir.taps_sum += filter_taps[i];
	}

	f->params.fir.array = os_malloc(size*sizeof(double));

	if (!f->params.fir.array)
		return -1;

	f->reset = filter_fir_reset;
	f->close = filter_fir_close;
	f->filter = filter_fir_filter;

	f->reset(&f->params);

	return 0;
}
