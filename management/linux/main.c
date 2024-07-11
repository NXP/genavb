/*
 * Copyright 2019-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Management linux specific code
 @details Setups linux thread for Management stack component. Implements Management main loop and event handling.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/epoll.h>

#include "common/log.h"
#include "common/net.h"
#include "common/timer.h"
#include "common/ipc.h"

#include "linux/tsn.h"

#include "management/management_entry.h"

#define EPOLL_MAX_EVENTS	8

/* Linux specific Management code entry point */

static void management_thread_cleanup(void *arg)
{
	struct management_ctx *management = arg;

	management_exit(management);

	os_log(LOG_INIT, "done\n");
}

static void management_status(struct tsn_ctx *tsn, int status)
{
	pthread_mutex_lock(&tsn->status_mutex);

	tsn->management_status = status;

	pthread_cond_signal(&tsn->management_cond);

	pthread_mutex_unlock(&tsn->status_mutex);
}

void *management_thread_main(void *arg)
{
	struct tsn_ctx *tsn = arg;
	struct management_ctx *management;
	int epoll_fd;
	struct epoll_event event[EPOLL_MAX_EVENTS];
	struct sched_param param = {
		.sched_priority = MANAGEMENT_CFG_PRIORITY,
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

	management = management_init(&tsn->management_cfg, epoll_fd);
	if (!management)
		goto err_management_init;

	pthread_cleanup_push(management_thread_cleanup, management);

	os_log(LOG_INIT, "started\n");

	management_status(tsn, 1);

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
			if (event[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
				os_log(LOG_ERR, "event error, %x\n", event[i].events);

			if (event[i].events & EPOLLIN) {
				epoll_data = (struct linux_epoll_data *)event[i].data.ptr;

				switch (epoll_data->type) {
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

err_management_init:
	close(epoll_fd);

err_epoll_create:
err_setschedparam:
	management_status(tsn, -1);

	return (void *)-1;
}
