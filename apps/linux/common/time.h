/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2018, 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _COMMON_TIME_H_
#define _COMMON_TIME_H_

#include <stdint.h>

#define NSECS_PER_SEC	1000000000
#define NSECS_PER_MSEC	1000000
#define USECS_PER_SEC	1000000
#define MSECS_PER_SEC	1000

int gettime_us(uint64_t *us);
int gettime_ns(uint64_t *ns);
int gettime_ns_monotonic(uint64_t *ns);
int gettime_s_monotonic(uint64_t *secs);

#endif /* _COMMON_TIME_H_ */
