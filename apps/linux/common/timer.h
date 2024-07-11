/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _COMMON_TIMER_H_
#define _COMMON_TIMER_H_

#include <stdint.h>

int sleep_ms(uint64_t ms);
int sleep_ns(uint64_t ns);

int create_timerfd_periodic(int clk_id);
int start_timerfd_periodic(int fd, time_t it_secs, long it_nsecs);
int create_timerfd_periodic_abs(int clk_id);
int start_timerfd_periodic_abs(int fd, time_t val_secs, long val_nsecs,
			       time_t it_secs, long it_nsecs);
int stop_timerfd(int fd);

#endif /* _COMMON_TIMER_H_ */
