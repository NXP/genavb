/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2020-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief MAAP linux specific code
 @details Setups linux thread for MAAP stack component. Implements MAAP main loop and event handling.
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
#include "common/timer.h"

#include "os/net.h"

#include "linux/avb.h"

#include "maap/maap_entry.h"


#define EPOLL_MAX_EVENTS	8

/* Linux specific MAAP code entry points */

static void maap_thread_cleanup(void *arg)
{
	struct avb_ctx *avb = arg;
	struct maap_ctx *maap = avb->maap;

	maap_exit(maap);

	avb->maap = NULL;

	os_log(LOG_INIT, "done\n");
}

static void maap_status(struct avb_ctx *avb, int status)
{
	pthread_mutex_lock(&avb->status_mutex);

	avb->maap_status = status;

	pthread_cond_signal(&avb->maap_cond);

	pthread_mutex_unlock(&avb->status_mutex);
}

void *maap_thread_main(void *arg)
{
	struct avb_ctx *avb = arg;
	struct maap_ctx *maap;
	int epoll_fd;
	struct epoll_event event[EPOLL_MAX_EVENTS];
	struct sched_param param = {
		.sched_priority = MAAP_CFG_PRIORITY,
	};
	int rc;

	rc = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
	if (rc) {
		os_log(LOG_ERR, "pthread_setschedparam(), %s\n", strerror(rc));
		goto err_setschedparam;
	}

	epoll_fd = epoll_create(1);
	if (epoll_fd < 0) {
		os_log(LOG_CRIT, "epoll_create(): %s\n", strerror(errno));
		goto err_epoll_create;
	}

	maap = maap_init(&avb->maap_cfg, epoll_fd);
	if (!maap)
		goto err_maap_init;

	avb->maap = maap;

	pthread_cleanup_push(maap_thread_cleanup, avb);

	os_log(LOG_INIT, "started\n");

	maap_status(avb, 1);

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

			os_log(LOG_CRIT, "epoll_wait(): %s\n", strerror(errno));
			break;
		}

		for (i = 0; i < ready; i++) {
			if (event[i].events & (EPOLLHUP | EPOLLRDHUP))
				os_log(LOG_ERR, "event error: %x\n", event[i].events);

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


err_maap_init:
	close(epoll_fd);

err_epoll_create:
	avb->maap = NULL;

err_setschedparam:
	maap_status(avb, -1);

	return (void *)-1;
}
