/*
 * Copyright 2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Linux specific timer media service implementation
 @details
*/

#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "common/log.h"
#include "common/timer.h"

#include "modules/avbdrv.h"

static const char *media_clock_to_device[] = {
	[OS_CLOCK_MEDIA_HW_0] = "/dev/mclk_gen_timer_0",
	[OS_CLOCK_MEDIA_HW_1] = "/dev/mclk_gen_timer_1",
	[OS_CLOCK_MEDIA_REC_0] = "/dev/mclk_rec_timer_0",
	[OS_CLOCK_MEDIA_REC_1] = "/dev/mclk_rec_timer_1",
	[OS_CLOCK_MEDIA_PTP_0]= "/dev/mclk_ptp_timer_0",
	[OS_CLOCK_MEDIA_PTP_1]= "/dev/mclk_ptp_timer_1"
};

void timer_media_process(struct os_timer *t)
{
	t->func(t, 1);
}

int timer_media_start(struct os_timer *t, u64 value, u64 interval_p, u64 interval_q, unsigned int flags)
{
	struct mtimer_start start;

	if (value || flags)
		goto err;

	start.p = interval_p;
	start.q = interval_q;

	if (ioctl(t->fd, MTIMER_IOC_START, &start) < 0) {
		os_log(LOG_ERR, "os_timer(%p), ioctl(MTIMER_IOC_START) %s\n", t, strerror(errno));
		goto err;
	}

	return 0;

err:
	return -1;
}

void timer_media_stop(struct os_timer *t)
{
	if (ioctl(t->fd, MTIMER_IOC_STOP) < 0)
		os_log(LOG_ERR, "os_timer(%p), ioctl(MTIMER_IOC_STOP) %s\n", t, strerror(errno));
}

int timer_media_create(struct os_timer *t, os_clock_id_t id)
{
	const char *device = media_clock_to_device[id];
	int fd;

	fd = open(device, O_RDWR);
	if (fd < 0)
		os_log(LOG_ERR, "os_timer(%p), open(%s) %s\n", t, device, strerror(errno));

	return fd;
}

void timer_media_destroy(struct os_timer *t)
{
	close(t->fd);
	t->fd = -1;
}
