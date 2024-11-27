/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2020-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Linux specific standard library implementation
 @details
*/


#define _GNU_SOURCE

#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "os/stdlib.h"

void *os_malloc(size_t size)
{
	return malloc(size);
}

void os_free(void *p)
{
	free(p);
}

int os_abs(int j)
{
	return abs(j);
}

double os_fabs(double j)
{
	return fabs(j);
}

long long int os_llabs(long long int j)
{
	return llabs(j);
}

void os_random_init(void)
{
	struct timespec ts;

	if (clock_gettime(CLOCK_REALTIME, &ts) < 0) {
		/* Handle error */
		srandom(time(NULL));
	} else {
		srandom(ts.tv_nsec ^ ts.tv_sec);
	}
}

long int os_random(void)
{
	return random();
}
