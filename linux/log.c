/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Linux logging services
 @details Linux logging services implementation
*/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <inttypes.h>

#include "common/log.h"

static int log_monotonic_enabled = 0;
static u64 log_monotonic_time_s;
static u64 log_monotonic_time_ns;

void log_enable_monotonic(void)
{
	log_monotonic_enabled = 1;
}

int log_update_monotonic(void)
{
	struct timespec now;
	int err;

	if (log_monotonic_enabled) {
		err = clock_gettime(CLOCK_MONOTONIC, &now);
		if (err < 0) {
			os_log(LOG_ERR, "clock_gettime() failed: %s\n", strerror(errno));
			return err;
		}

		log_monotonic_time_s = now.tv_sec;
		log_monotonic_time_ns = now.tv_nsec;
	}

	return 0;
}

void _os_log_raw(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);

	vprintf(format, ap);

	va_end(ap);

	fflush(stdout);
}

void _os_log(const char *level, const char *func, const char *component, const char *format, ...)
{
	va_list ap;

	/* customizing log output depending on user's configuration to have either ptp only or monotonic and ptp time reference */
	if (log_monotonic_enabled)
		printf("%-4s %4" PRIu64 ".%09" PRIu64 " %11" PRIu64 ".%09" PRIu64 " %-6s %-32.32s : ", level, log_monotonic_time_s, log_monotonic_time_ns, log_time_s, log_time_ns, component, func);
	else
		printf("%-4s %11" PRIu64 ".%09" PRIu64 " %-6s %-32.32s : ", level, log_time_s, log_time_ns, component, func);

	va_start(ap, format);

	vprintf(format, ap);

	va_end(ap);

	fflush(stdout);
}
