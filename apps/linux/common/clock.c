/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2018, 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define _GNU_SOURCE

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "log.h"
#include "clock.h"
#include "time.h"

struct clock {
	int id;
	int fd;
};

#define CFG_PORTS 1
#define PTP_DEVICE "/dev/ptp0"
#define CLOCKFD 3
#define FD_TO_CLOCKID(fd)	((~(clockid_t) (fd) << 3) | CLOCKFD)
#define CLOCKID_TO_FD(clk)	((unsigned int) ~((clk) >> 3))

static struct clock c[CFG_PORTS];

int clock_init(int port_id)
{
	if (port_id >= CFG_PORTS)
		goto err;

	c[port_id].fd= open(PTP_DEVICE, O_RDWR);
	if (c[port_id].fd < 0) {
		ERR("clock(%p) Couldn't open PTP char device: %s error: %d", &c, PTP_DEVICE, c[port_id].fd);
		return -1;
	}

	c[port_id].id = FD_TO_CLOCKID(c[port_id].fd);
	DBG("clock(%p) PTP clock ID: 0x%x fd: 0x%x", &c, c[port_id].id, c[port_id].fd);

	return 0;
err:
	return -1;
}

int clock_exit(int port_id)
{
	if (port_id >= CFG_PORTS)
		goto err;

	close(c[port_id].fd);

	return 0;
err:
	return -1;
}


int clock_gettime32(int port_id, unsigned int *ns)
{
	int err = 0;
	struct timespec now;

	if (port_id >= CFG_PORTS)
		goto err;

	err = clock_gettime(c[port_id].id, &now);
	if (err) {
		ERR("clock(%p) clock_gettime failed: %s", &c, strerror(errno));
		return err;
	}

	*ns = (unsigned long long)now.tv_sec*NSECS_PER_SEC + now.tv_nsec;

	return 0;
err:
	return -1;
}

int clock_gettime64(int port_id, unsigned long long *ns)
{
	int err = 0;
	struct timespec now;

	if (port_id >= CFG_PORTS)
		goto err;

	err = clock_gettime(c[port_id].id, &now);
	if (err) {
		ERR("clock(%p) clock_gettime failed: %s", &c, strerror(errno));
		return err;
	}

	*ns = (unsigned long long)now.tv_sec*NSECS_PER_SEC + now.tv_nsec;

	return 0;
err:
	return -1;
}
