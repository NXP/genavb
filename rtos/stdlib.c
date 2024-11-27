/*
 * Copyright 2018, 2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief RTOS specific standard library implementation
 @details
*/

#include "rtos_abstraction_layer.h"

#include "os/stdlib.h"
#include "os/clock.h"

#include "prng_32.h"
#include "common/log.h"

void *os_malloc(size_t size)
{
	return rtos_malloc(size);
}

void os_free(void *p)
{
	rtos_free(p);
}

int os_abs(int j)
{
	if (j < 0)
		return -j;
	else
		return j;
}

double os_fabs(double j)
{
	if (j < 0.0)
		return -j;
	else
		return j;
}

long long int os_llabs(long long int j)
{
	if (j < 0)
		return -j;
	else
		return j;
}

void os_random_init(void)
{
	uint64_t seed;

	/* Use monotonic time as initial seed. Make sure it's not zero in all cases. */
	if (os_clock_gettime64(OS_CLOCK_SYSTEM_MONOTONIC, &seed) < 0)
		os_log(LOG_ERR, "os_clock_gettime64() failed, seed with hardcoded value\n");

	seed ^= 84569752368ULL;

	xoroshiro64starstar_init_state(seed);
}

long int os_random(void)
{
	/* Limit the range to max of (2^31 - 1) */
	return (long int) (xoroshiro64starstar_next() >> 1);
}
