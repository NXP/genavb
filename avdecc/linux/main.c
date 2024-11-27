/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVDECC linux specific code
 @details Setups linux thread for AVDECC stack component. Implements AVDECC main loop and event handling.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/epoll.h>

#include "common/types.h"
#include "common/log.h"
#include "common/ipc.h"

#include "os/string.h"
#include "os/net.h"
#include "os/ipc.h"
#include "os/timer.h"

#include "linux/avb.h"

#include "avdecc/avdecc_entry.h"


#define EPOLL_MAX_EVENTS	8

/* Linux specific AVDECC code entry points */

static void avdecc_thread_cleanup(void *arg)
{
	struct avb_ctx *avb = arg;
	struct avdecc_ctx *avdecc = avb->avdecc;

	avdecc_exit(avdecc);

	avb->avdecc = NULL;

	os_log(LOG_INIT, "done\n");
}

static void avdecc_status(struct avb_ctx *avb, int status)
{
	pthread_mutex_lock(&avb->status_mutex);

	avb->avdecc_status = status;

	pthread_cond_signal(&avb->avdecc_cond);

	pthread_mutex_unlock(&avb->status_mutex);
}

void *avdecc_thread_main(void *arg)
{
	struct avb_ctx *avb = arg;
	struct avdecc_ctx *avdecc;
	int epoll_fd;
	struct epoll_event event[EPOLL_MAX_EVENTS];
	struct sched_param param = {
		.sched_priority = AVDECC_CFG_PRIORITY,
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

	avdecc = avdecc_init(&avb->avdecc_cfg, epoll_fd);
	if (!avdecc)
		goto err_avdecc_init;

	avb->avdecc = avdecc;

	pthread_cleanup_push(avdecc_thread_cleanup, avb);

	os_log(LOG_INIT, "started\n");

	avdecc_status(avb, 1);

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

			os_log(LOG_CRIT, "epoll_wait(), %s\n", strerror(errno));
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

err_avdecc_init:
	close(epoll_fd);

err_epoll_create:
	avb->avdecc = NULL;

err_setschedparam:
	avdecc_status(avb, -1);

	return (void *)-1;
}
