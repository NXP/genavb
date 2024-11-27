/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief SRP linux specific code
 @details Setups linux thread for SRP stack component. Implements SRP main loop and event handling.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/epoll.h>

#include "common/ipc.h"
#include "common/timer.h"
#include "common/types.h"
#include "common/log.h"

#include "linux/tsn.h"

#include "os/config.h"
#include "os/sys_types.h"
#include "os/log.h"
#include "os/sys_types.h"
#include "os/net.h"
#include "os/ipc.h"
#include "os/timer.h"

#include "srp/srp_entry.h"


#define EPOLL_MAX_EVENTS	8

/* Linux specific SRP code entry point */

static void srp_thread_cleanup(void *arg)
{
	struct srp_ctx *srp = arg;

	srp_exit(srp);

	os_log(LOG_INIT, "done\n");
}

static void srp_status(struct tsn_ctx *tsn, int status)
{
	pthread_mutex_lock(&tsn->status_mutex);

	tsn->srp_status = status;

	pthread_cond_signal(&tsn->srp_cond);

	pthread_mutex_unlock(&tsn->status_mutex);
}

void *srp_thread_main(void *arg)
{
	struct tsn_ctx *tsn = arg;
	struct srp_ctx *srp;
	int epoll_fd;
	struct epoll_event event[EPOLL_MAX_EVENTS];
	struct sched_param param = {
		.sched_priority = SRP_CFG_PRIORITY,
	};
	int rc;

	rc = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
	if (rc) {
		os_log(LOG_ERR, "pthread_setschedparam(), %s\n", strerror(rc));
		goto err_setschedparam;
	}

	epoll_fd = epoll_create(1);
	if (epoll_fd < 0) {
		os_log(LOG_CRIT, "epoll_create(), %s\n", strerror(errno));
		goto err_epoll_create;
	}

	srp = srp_init(&tsn->srp_cfg, epoll_fd);
	if (!srp)
		goto err_srp_init;

	pthread_cleanup_push(srp_thread_cleanup, srp);

	os_log(LOG_INIT, "started\n");

	srp_status(tsn, 1);

	while (1) {
		int ready, i;
		struct linux_epoll_data *epoll_data;

		/* thread main loop */
		/* use epoll to wait for events from all open file descriptors */

		pthread_testcancel();

		ready = epoll_wait(epoll_fd, event, EPOLL_MAX_EVENTS, -1);
		if (ready < 0) {
			if (errno == EINTR)
				continue;

			os_log(LOG_ERR, "epoll_wait(), %s\n", strerror(errno));
			break;
		}

		for (i = 0; i < ready; i++) {
			if (event[i].events & (EPOLLHUP | EPOLLRDHUP))
				os_log(LOG_ERR, "event error, %x\n", event[i].events);

			if (event[i].events & (EPOLLIN | EPOLLERR)) {
				epoll_data = (struct linux_epoll_data *)event[i].data.ptr;

				switch (epoll_data->type) {
				case EPOLL_TYPE_NET_RX:
					net_rx((struct net_rx *)epoll_data->ptr);
					break;
				case EPOLL_TYPE_TIMER:
					os_timer_process((struct os_timer *)epoll_data->ptr);
					break;
				case EPOLL_TYPE_IPC:
					ipc_rx((struct ipc_rx *)epoll_data->ptr);
					break;
				default:
					break;
				}
			}
		}
	}

	pthread_cleanup_pop(1);

	close(epoll_fd);

	return (void *)0;

err_srp_init:
	close(epoll_fd);

err_epoll_create:
err_setschedparam:
	srp_status(tsn, -1);

	return (void *)-1;
}
