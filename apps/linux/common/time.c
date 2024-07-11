/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

#include "time.h"

int gettime_ns(uint64_t *ns)
{
	struct timespec now;
	int rc;

	rc = clock_gettime(CLOCK_REALTIME, &now);
	if (rc < 0) {
		printf("clock_gettime() failed: %s\n", strerror(errno));
		return rc;
	}

	*ns = (uint64_t)now.tv_sec * NSECS_PER_SEC + now.tv_nsec;

	return 0;
}

int gettime_us(uint64_t *us)
{
	int rc;

	rc = gettime_ns(us);
	*us /= 1000;

	return rc;
}

int gettime_ns_monotonic(uint64_t *ns)
{
	struct timespec now;
	int rc;

	rc = clock_gettime(CLOCK_MONOTONIC, &now);
	if (rc < 0) {
		printf("clock_gettime() failed: %s\n", strerror(errno));
		return rc;
	}

	*ns = (uint64_t)now.tv_sec * NSECS_PER_SEC + now.tv_nsec;

	return 0;
}

int gettime_s_monotonic(uint64_t *secs)
{
	struct timespec now;
	int rc;

	rc = clock_gettime(CLOCK_MONOTONIC, &now);
	if (rc < 0) {
		printf("clock_gettime() failed: %s\n", strerror(errno));
		return rc;
	}

	*secs = (uint64_t)now.tv_sec;

	return 0;
}
