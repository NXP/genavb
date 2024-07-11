/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2018-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Linux specific timer service implementation
 @details
*/

#define _GNU_SOURCE

#include <sys/timerfd.h>

#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "common/log.h"
#include "common/timer.h"

#include "timer_media.h"
#include "epoll.h"

static void u64_to_timespec(struct timespec *ts, u64 timeout)
{
	ts->tv_sec = timeout / NSECS_PER_SEC;
	ts->tv_nsec = timeout - ts->tv_sec * NSECS_PER_SEC;
}

static void timer_system_process(struct os_timer *t)
{
	uint64_t count;
	int rc;

	rc = read(t->fd, &count, sizeof(count));
	if (rc == sizeof(count))
		t->func(t, (int)count);
	else {
		if (rc >= 0)
			os_log(LOG_ERR, "os_timer(%p): Unexpected short read (%d bytes) from timerfd descriptor\n", t, rc);
		else
			os_log(LOG_ERR, "os_timer(%p): error %d(%s) reading from timer\n", t, errno, strerror(errno));
	}
}

static int timer_system_start(struct os_timer *t, u64 value, u64 interval_p, u64 interval_q, unsigned int flags)
{
	struct itimerspec new_value;

	/* No support for rational period */
	if (interval_p && (interval_q != 1))
		goto err;

	/* Use stop instead */
	if (!value && !interval_p)
		goto err;

	/* For periodic timer, first expiration at the end of the first period */
	value += interval_p;

	u64_to_timespec(&new_value.it_value, value);
	u64_to_timespec(&new_value.it_interval, interval_p);

	if (flags & OS_TIMER_FLAGS_ABSOLUTE)
		flags = TFD_TIMER_ABSTIME;
	else
		flags = 0;

	if (timerfd_settime(t->fd, flags, &new_value, NULL) < 0) {
		os_log(LOG_ERR, "os_timer(%p), timerfd_settime() %s\n", t, strerror(errno));
		goto err;
	}

	return 0;

err:
	return -1;
}

static void timer_system_stop(struct os_timer *t)
{
	struct itimerspec new_value = {0, };

	if (timerfd_settime(t->fd, 0, &new_value, NULL) < 0)
		os_log(LOG_ERR, "os_timer(%p), timerfd_settime() %s\n", t, strerror(errno));
}

static int timer_system_create(struct os_timer *t, clockid_t id)
{
	int fd;

	fd = timerfd_create(id, TFD_NONBLOCK);
	if (fd < 0)
		os_log(LOG_ERR, "os_timer(%p), timerfd_create(%u) %s\n", t, id, strerror(errno));

	return fd;
}

static void timer_system_destroy(struct os_timer *t)
{
	close(t->fd);
	t->fd = -1;
}

void os_timer_process(struct os_timer *t)
{
	os_log(LOG_DEBUG, "os_timer(%p)\n", t);

	t->process(t);
}

static int os_timer_epoll_add(struct os_timer *t)
{
	os_log(LOG_DEBUG, "os_timer(%p), epoll_fd: %d, fd: %d\n", t, t->epoll_fd, t->fd);

	if (epoll_ctl_add(t->epoll_fd, t->fd, EPOLL_TYPE_TIMER, t, &t->epoll_data, EPOLLIN) < 0) {
		os_log(LOG_ERR, "os_timer(%p), epoll_ctl_add() failed\n", t);
		goto err_add;
	}

	return 0;

err_add:
	return -1;
}

int os_timer_create(struct os_timer *t, os_clock_id_t id, unsigned int flags, void (*func)(struct os_timer *t, int count), unsigned long priv)
{
	int fd;

	if (flags)
		goto err_flags;

	switch (id) {
	case OS_CLOCK_SYSTEM_MONOTONIC_COARSE:
		fd = timer_system_create(t, CLOCK_MONOTONIC);
		t->start = timer_system_start;
		t->stop = timer_system_stop;
		t->process = timer_system_process;
		t->destroy = timer_system_destroy;
		break;

	case OS_CLOCK_MEDIA_HW_0:
	case OS_CLOCK_MEDIA_HW_1:
	case OS_CLOCK_MEDIA_REC_0:
	case OS_CLOCK_MEDIA_REC_1:
	case OS_CLOCK_MEDIA_PTP_0:
	case OS_CLOCK_MEDIA_PTP_1:
		fd = timer_media_create(t, id);
		t->start = timer_media_start;
		t->stop = timer_media_stop;
		t->process = timer_media_process;
		t->destroy = timer_media_destroy;
		break;

	default:
		goto err_id;
	}

	if (fd < 0)
		goto err_create;

	t->fd = fd;
	t->epoll_fd = priv;
	t->func = func;

	os_log(LOG_INFO, "os_timer(%p), fd: %d, epoll_fd: %d\n", t, t->fd, t->epoll_fd);

	if (os_timer_epoll_add(t) < 0)
		goto err_add;

	return 0;

err_add:
	os_timer_destroy(t);

err_create:
err_id:
err_flags:
	return -1;
}

int os_timer_start(struct os_timer *t, u64 value, u64 interval_p, u64 interval_q, unsigned int flags)
{
	os_log(LOG_DEBUG, "os_timer(%p), value: %" PRIx64 ", interval: %" PRIx64 "/%" PRIx64 ", flags: %x\n", t, value, interval_p, interval_q, flags);

	if (flags & (OS_TIMER_FLAGS_PPS | OS_TIMER_FLAGS_RECOVERY))
		return -1;

	return t->start(t, value, interval_p, interval_q, flags);
}

void os_timer_stop(struct os_timer *t)
{
	os_log(LOG_DEBUG, "os_timer(%p)\n", t);

	t->stop(t);
}


void os_timer_destroy(struct os_timer *t)
{
	os_log(LOG_INFO, "os_timer(%p)\n", t);

	t->destroy(t);
}
