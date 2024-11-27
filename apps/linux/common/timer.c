/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "time.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/timerfd.h>

int sleep_ns(uint64_t ns)
{
	struct timespec req, rem;

	req.tv_sec = ns / NSECS_PER_SEC;
	req.tv_nsec = ns - req.tv_sec * NSECS_PER_SEC;

retry:
	if (nanosleep(&req, &rem) < 0) {
		if (errno == EINTR) {
			req = rem;
			goto retry;
		} else {
			return -1;
		}
	}

	return 0;
}

int sleep_ms(uint64_t ms)
{
	return sleep_ns(ms * NSECS_PER_MSEC);
}

static int __create_timerfd_periodic(int clkid, int flags)
{
	int timer_fd;

	timer_fd = timerfd_create(clkid, flags);
	if (timer_fd < 0) {
		printf("%s timerfd_create() failed %s\n", __func__, strerror(errno));
		return -1;
	}

	return timer_fd;
}

int create_timerfd_periodic(int clk_id)
{
	return __create_timerfd_periodic(clk_id, 0);
}

int create_timerfd_periodic_abs(int clk_id)
{
	return __create_timerfd_periodic(clk_id, 0);
}

static int __start_timerfd_periodic(int fd, int flags, time_t val_secs, long val_nsecs,
				    time_t it_secs, long it_nsecs)
{
	struct itimerspec timer;

	timer.it_interval.tv_sec = it_secs;
	timer.it_interval.tv_nsec = it_nsecs;
	timer.it_value.tv_sec = val_secs;
	timer.it_value.tv_nsec = val_nsecs;

	if (timerfd_settime(fd, flags, &timer, NULL) < 0) {
		printf("%s timerfd_settime() failed %s\n", __func__, strerror(errno));
		return -1;
	}

	return 0;
}

int start_timerfd_periodic(int fd, time_t it_secs, long it_nsecs)
{
	return __start_timerfd_periodic(fd, 0, it_secs, it_nsecs, it_secs, it_nsecs);
}

int start_timerfd_periodic_abs(int fd, time_t val_secs, long val_nsecs,
			       time_t it_secs, long it_nsecs)
{
	int flags = TFD_TIMER_ABSTIME;

#ifdef TFD_TIMER_CANCEL_ON_SET
	flags |= TFD_TIMER_CANCEL_ON_SET;
#endif

	return __start_timerfd_periodic(fd, flags, val_secs, val_nsecs, it_secs, it_nsecs);
}

int stop_timerfd(int fd)
{
	return __start_timerfd_periodic(fd, 0, 0, 0, 0, 0);
}
