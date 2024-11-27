/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __THREAD_CONFIG_H__
#define __THREAD_CONFIG_H__

#include <pthread.h>
#include <sys/epoll.h>
#include <stdbool.h>
#include "stats.h"
/**
 * @addtogroup thread
 * @{
 */
#define MAX_THREADS           8     /**< Maximum number of threads can be used for data transfer */
#define MAX_THREAD_SLOTS      10     /**< Maximum number of slots in threads */
/**
 * @brief     Thread capability mask
 */
typedef enum _THREAD_CAPABILITY {
	THR_CAP_STREAM_TALKER   = 0x01, /**< Thread can process AVB talker stream */
	THR_CAP_STREAM_LISTENER = 0x02, /**< Thread can process AVB listener stream */
	THR_CAP_STREAM_AUDIO    = 0x04, /**< Thread can process AVB stream has format AAF */
	THR_CAP_STREAM_CRF      = 0x08, /**< Thread can process AVB stream has format CRF */
	THR_CAP_STREAM_VIDEO	= 0x10,
	THR_CAP_ALSA		= 0x20,
	THR_CAP_GSTREAMER	= 0x40,
	THR_CAP_GSTREAMER_SYNC	= 0x80,
	THR_CAP_STATS		= 0x100,
	THR_CAP_TIMER		= 0x200,
	THR_CAP_CONTROLLED	= 0x400,
	THR_CAP_GST_MULTI	= 0x800,
	THR_CAP_GST_BUS_TIMER	= 0x1000,
	THR_CAP_TSN_LOOP 	= 0x2000,
	THR_CAP_TSN_PT	 	= 0x4000
} thr_capability_t;

/**
 * @brief      Thread slot structure.
 */
typedef struct _THREAD_SLOT {
	char is_used;                    /**< This flag specifies whether the current slot is used or not */
	char is_enabled; 		 /**< This flag specifies whether the current fd is enabled for monitoring or not*/
	int fd;                          /**< File descriptor of slot, this used to mornitoring input events */
	void *data;                      /**< User parameter for slot handler */
	int (*handler)(void *data, unsigned int events); /**< Slot handler function */
	unsigned int max_timeout;	 /**< Maximum time since last poll before executing the timeout handler */
	unsigned int timeout_count;	 /**< Time since last poll */
	int (*timeout_handler) (void *data);     /**< Timeout handler */
	void *sleep_data;                /**< parameters to pass to the sleep handler. */
	pthread_mutex_t slot_lock;       /**< Thread slot mutex lock */
} thr_thread_slot_t;

typedef struct _THREAD_STAT {
	struct stats epoll_event;     /**< Number of events ready when epoll returns */
	struct stats time_2epoll;     /**< Time between two continuous epoll returns */
	struct stats processing_time; /**< Time to process all ready epoll events */
	bool pending;
} thr_thread_stat_t;
/**
 * @brief       Thread data structure, store require information for each thread
 */
typedef struct _THREAD_THREAD_DATA {
	pthread_t id;              /**< ID of the thread */
	int poll_fd;               /**< File descriptor of epoll */
	int thread_capabilities;   /**< Capabilities of this thread. Can combine flags of @thr_capability_t */
	char cpu_core;             /**< CPU core number that thread will run on */
	int priority;              /**< Thread priority */
	char max_slots;            /**< Max number of slots in a thread */
	char exit_flag;            /**< If this flag is set, the thread will try to terminate itself */
	thr_thread_slot_t slots[MAX_THREAD_SLOTS];  /**< Array of data slots */
	int num_slots;             /**< Number of used slots */
	int (*sleep_handler)(struct _THREAD_THREAD_DATA *thread_ptr, struct epoll_event *recv_events); /**< function to call to put thread to sleep */

	char is_first_poll;        /**< Indicate the first poll loop of thread handle */
	unsigned int last_poll_time;   /**< Time of the last poll */
	thr_thread_stat_t stats;
	thr_thread_stat_t stats_snap;
} thr_thread_t;

/** @} */
#endif /* __THREAD_CONFIG_H__ */
