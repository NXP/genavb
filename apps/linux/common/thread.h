/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2020-2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __THREAD_MANAGEMENT_H__
#define __THREAD_MANAGEMENT_H__

#include <stdbool.h>
#include "thread_config.h"

extern thr_thread_t g_thread_array[MAX_THREADS];

/**
 * @addtogroup thread
 * @{
 */

/** Create all routing required threads.
 *
 * @details    This function will create four threads (equals to number of CPU cores) to routing
 *             data between AVB stream and ALSA device. Each thread executes on a specified CPU
 *             core.
 *
 * @return     0 if success or negative error code
 */
int thread_init();

/** Terminate all routing threads, free resources.
 *
 * @details    This function will terminate all threads are created by @ref thread_init function.
 *
 * @return     0 if success or negative error code
 */
int thread_exit();

/** Add a route slot into thread (generic version).
 *
 * @details    This function finds a thread with a free slot and matching thread capabilities
 *             parameter then populates this new slot with its information.
 *
 * @param[in]  capabilities    Required capabilities of the slot
 * @param[in]  fd              Input file descriptor of input source. Used by default epoll sleep
 *                             handler, ignored otherwise.
 * @param[in]  events          Events epoll should listen to. If non-zero, sleep_handler must be set
 *                             to thread_sleep_epoll or an error will be returned.
 * @param[in]  data            Private data of the slot, this data will be passed to handler
 *                             function whenever this function is called.
 * @param[in]  handler         Handler function, will be called to process data whenever fd has
 *                             input data.
 * @param[in]  timeout_handler Handler function will be executed in case epoll timed out
 * @param[in]  max_timeout     Max timeout (ns) since last poll before timeout handler is executed
 * @param[out] slot            When thread slot is successful added, this will contains created
 *                             thread slot information
 * @param[in]  sleep_handler   Function to be called to put the thread to sleep. A thread with
 *                             matching capabilities will only be selected if the requested
 *                             sleep_handler matches the one already in place.
 * @param[in]  sleep_data      Optional slot-specific data to be used by the sleep handler.
 *
 * @return     0 if success or negative error code
 */

int __thread_slot_add(int capabilities, int fd, unsigned int events, void *data,
		      int (*handler)(void *data, unsigned int events),
		      int (*timeout_handler)(void *data),
		      int max_timeout, thr_thread_slot_t **slot,
		      int (*sleep_handler)(thr_thread_t *thread_ptr, struct epoll_event *recv_events),
		      void *sleep_data);

/** Add a route slot into thread (default epoll version).
 *
 * @details    This function finds a thread with a free slot and matching thread capabilities
 *             parameter then populates this new slot with its information, using the default
 *             epoll sleep_handler.
 *
 * @param[in]  capabilities    Required capabilities of the slot
 * @param[in]  fd              Input file descriptor of input source
 * * @param[in]  events        Events epoll should listen to.
 * @param[in]  data            Private data of the slot, this data will be passed to handler
 *                             function whenever this function is called.
 * @param[in]  handler         Handler function, will be called to process data whenever fd has
 *                             input data.
 * @param[in]  timeout_handler Handler function will be executed in case epoll timed out
 * @param[in]  max_timeout     Max timeout (ns) since last poll before timeout handler is executed
 * @param[out] slot            When thread slot is successful added, this will contains created
 *                             thread slot information
 *
 * @return     0 if success or negative error code
 */
int thread_slot_add(int capabilities, int fd, unsigned int events, void *data,
		    int (*handler)(void *data, unsigned int events),
		    int (*timeout_handler)(void *data),
		    int max_timeout, thr_thread_slot_t **slot);

/** Free a thread slot and its resources.
 *
 * @details    This function will remove the slot fd from monitor fds.
 *
 * @param[in]  slot  Pointer to slot that you want to remove.
 *
 * @return     0 if success or negative error code
 */
int thread_slot_free(thr_thread_slot_t *slot);

/** Modify the fd monitor of a thread slot.
 *
 * @details    	This function will enable or disable the requested events on the monitored fd
 * 		of the slot.
 *
 * @param[in]  slot  Pointer to slot that you want to remove.
 * @param[in]  enable  flag to enable or disable.
 * @param[in]  req_events  The events to enbale/disable: EPOLLIN and/or EPOLLOUT
 *
 * @return     0 if success or negative error code
 */
int thread_slot_set_events(thr_thread_slot_t *slot, int enable, unsigned int req_events);

/** Print global thread scheduling statistics
 *
 * @details    This function prints all threads statistics when available.
 *	       Should be polled regularly.
 *
 * @return     None
 */
void thread_print_stats();

/** Sleep handler based on clock_nanosleep, to be used with thread framework.
 *
 */
int thread_sleep_nanosleep(thr_thread_t *thread_ptr, struct epoll_event *recv_events);

/** Private data for clock_nanosleep sleep handler
 *
 */
struct thread_sleep_nanosleep_data {
	uint64_t next; /**< Absolute time of next event, in nanoseconds. */
	bool is_armed; /**< Whether the slot is armed (must fire an event when reaching next). */
};

/**
 * @}
 */

#endif /* __THREAD_MANAGEMENT_H__ */
