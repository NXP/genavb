/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVTP linux specific code
 @details Setups linux thread for AVTP stack component. Implements AVTP main loop and event handling.
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <time.h>

#include "common/net.h"
#include "common/ipc.h"
#include "common/list.h"
#include "common/avtp.h"
#include "common/timer.h"
#include "common/log.h"

#include "os/sys_types.h"
#include "os/clock.h"

#include "avtp/avtp_entry.h"

#include "linux/avb.h"

#define EPOLL_MAX_EVENTS	8
#define EPOLL_TIMEOUT_MS	10
#define IPC_POOLING_PERIOD_NS	(10ULL * NSECS_PER_MS)
#define STATS_PERIOD_NS		(10ULL * NSECS_PER_SEC)

/* Linux specific AVTP code entry points */

static void stats_thread_cleanup(void *arg)
{
	struct ipc_rx *ipc_rx_stats = arg;

	ipc_rx_exit(ipc_rx_stats);

	os_log(LOG_INIT, "done\n");
}


static void *stats_thread_main(void *arg)
{
	struct ipc_rx ipc_rx_stats;
	int epoll_fd;
	struct epoll_event event[EPOLL_MAX_EVENTS];
	struct sched_param param = {
		.sched_priority = STATS_CFG_PRIORITY,
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

	if (ipc_rx_init(&ipc_rx_stats, IPC_AVTP_STATS, stats_ipc_rx, epoll_fd) < 0)
		goto err_ipc_rx;

	pthread_cleanup_push(stats_thread_cleanup, &ipc_rx_stats);

	os_log(LOG_INIT, "started\n");

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
			if (event[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
				os_log(LOG_ERR, "event error, 0x%x, data = 0x%llx\n", event[i].events, event[i].data.u64);

			if (event[i].events & EPOLLIN) {
				epoll_data = (struct linux_epoll_data *)event[i].data.ptr;
				if (epoll_data->type == EPOLL_TYPE_IPC)
					ipc_rx((struct ipc_rx *)epoll_data->ptr);
			}
		}
	}

	pthread_cleanup_pop(1);

	close(epoll_fd);

	return (void *)0;

err_ipc_rx:
	close(epoll_fd);

err_epoll_create:
err_setschedparam:
	return (void *)-1;
}

static void avtp_thread_cleanup(void *arg)
{
	struct avb_ctx *avb = arg;
	struct avtp_ctx *avtp = avb->avtp;

	avtp_exit(avtp);

	avb->avtp = NULL;

	os_log(LOG_INIT, "done\n");
}

static void avtp_status(struct avb_ctx *avb, int status)
{
	pthread_mutex_lock(&avb->status_mutex);

	avb->avtp_status = status;

	pthread_cond_signal(&avb->avtp_cond);

	pthread_mutex_unlock(&avb->status_mutex);
}

void *avtp_thread_main(void *arg)
{
	struct avb_ctx *avb = arg;
	struct avtp_ctx *avtp;
	int epoll_fd;
	pthread_t stats_thread;
	struct epoll_event event[EPOLL_MAX_EVENTS];
	struct sched_param param = {
		.sched_priority = AVTP_CFG_PRIORITY,
	};
	struct timespec tp;
	u64 current_time = 0, previous_time = 0, ipc_time = 0, stats_time = 0;
	struct process_stats stats;
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

	if (clock_gettime(CLOCK_MONOTONIC_RAW, &tp) == 0) {
		current_time = tp.tv_sec * (u64)NSECS_PER_SEC + tp.tv_nsec;

		previous_time = current_time;
	}

	rc = pthread_create(&stats_thread, NULL, stats_thread_main, NULL);
	if (rc) {
		os_log(LOG_CRIT, "pthread_create(): %s\n", strerror(rc));
		goto err_pthread_create;
	}

	avtp = avtp_init(&avb->avtp_cfg, epoll_fd);
	if (!avtp)
		goto err_avtp_init;

	avb->avtp = avtp;

	pthread_cleanup_push(avtp_thread_cleanup, avb);

	os_log(LOG_INIT, "started\n");

	avtp_status(avb, 1);

	stats_init(&stats.events, 31, NULL, NULL);
	stats_init(&stats.sched_intvl, 31, NULL, NULL);
	stats_init(&stats.processing_time, 31, NULL, NULL);

	while (1) {
		int ready, i;
		struct linux_epoll_data *epoll_data;

		/* thread main loop */
		/* use epoll to wait for events from all open file descriptors */

		pthread_testcancel();

		ready = epoll_wait(epoll_fd, event, EPOLL_MAX_EVENTS, EPOLL_TIMEOUT_MS);
		if (ready < 0) {
			if (errno == EINTR)
				continue;

			os_log(LOG_CRIT, "epoll_wait(), %s\n", strerror(errno));
			break;
		}

		stats_update(&stats.events, ready);

		if (clock_gettime(CLOCK_MONOTONIC_RAW, &tp) == 0) {
			current_time = tp.tv_sec * (u64)NSECS_PER_SEC + tp.tv_nsec;

			stats_update(&stats.sched_intvl, current_time - previous_time);
			previous_time = current_time;
		}

		for (i = 0; i < ready; i++) {
			if (event[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
				os_log(LOG_ERR, "event error, 0x%x, data = 0x%llx\n", event[i].events, event[i].data.u64);

			if (event[i].events & EPOLLIN) {
				epoll_data = (struct linux_epoll_data *)event[i].data.ptr;

				switch (epoll_data->type) {
				case EPOLL_TYPE_NET_RX:
					net_rx_multi((struct net_rx *)epoll_data->ptr);
					break;

				case EPOLL_TYPE_TIMER:
					os_timer_process((struct os_timer *)epoll_data->ptr);
					break;

				case EPOLL_TYPE_MEDIA:
					avtp_media_event(epoll_data->ptr);
					break;

				default:
					break;
				}
			}

			if (event[i].events & EPOLLOUT) {
				epoll_data = (struct linux_epoll_data *)event[i].data.ptr;

				switch (epoll_data->type) {
				case EPOLL_TYPE_NET_TX_EVENT:
					avtp_net_tx_event(epoll_data->ptr);
					break;
				default:
					break;
				}
			}
		}

		if (clock_gettime(CLOCK_MONOTONIC_RAW, &tp) == 0) {
			current_time = tp.tv_sec * (u64)NSECS_PER_SEC + tp.tv_nsec;

			if ((current_time - ipc_time) > IPC_POOLING_PERIOD_NS) {
				avtp_ipc_rx(avtp);
				avtp_stream_free(avtp, current_time);
				ipc_time = current_time;
			}

			if ((current_time - stats_time) > STATS_PERIOD_NS) {
				avtp_stats_dump(avtp, &stats);
				stats_time = current_time;
			}

			stats_update(&stats.processing_time, current_time - previous_time);
		}
	}

	pthread_cleanup_pop(1);

	pthread_cancel(stats_thread);
	pthread_join(stats_thread, NULL);

	close(epoll_fd);

	return (void *)0;

err_avtp_init:
	pthread_cancel(stats_thread);
	pthread_join(stats_thread, NULL);

err_pthread_create:
	close(epoll_fd);

err_epoll_create:
err_setschedparam:
	avtp_status(avb, -1);

	return (void *)-1;
}
