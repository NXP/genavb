/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2020-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include <sched.h>
#include <errno.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/epoll.h>
#include <time.h>
#include <unistd.h>
#include "log.h"
#include "thread.h"
#include "time.h"

#define THREAD_STATS_PERIOD_NS	  (10llu * NSECS_PER_SEC)
#define THREAD_HANDLER_TIMEOUT_MS (100llu)
#define THREAD_HANDLER_TIMEOUT_NS (THREAD_HANDLER_TIMEOUT_MS * NSECS_PER_MSEC)

static pthread_mutex_t thread_mutex = PTHREAD_MUTEX_INITIALIZER;

/** Thread handle function.
 *
 * After thread starts, this function will check the registered file descriptor for incoming data.
 * If any data came, it will read from input then transfer to output. This function only exit
 * whenever error occurs or exit_flag of thread is set.
 *
 * @param[in]  param  Index of thread
 *
 * @return     handler result
 */
static void *s_data_thread_handle(void *param);

/** Count number of used slot in thread
 *
 * @param[in]  thread  Pointer of thread that want to check
 *
 * @return     number of used slot
 */
static int thread_count_used_slot(thr_thread_t *thread);

/** Check thread matches specified capabilities.
 *
 * @param[in]  thread        Pointer to thread that need to check
 * @param[in]  capabilities  Required capabilities
 *
 * @return     0 if the thread does not match required capabilities
 * @return     1 if the thread matches required capabilities
 */
static int thread_check_capability_match(thr_thread_t *thread, int capabilities);

/** Get first idle slot of thread
 *
 * @param[in]  thread  Pointer to thread
 *
 * @return     pointer to idle slot, NULL if do not have any idle slot.
 */
static thr_thread_slot_t *thread_get_first_idle_slot(thr_thread_t *thread);

/** Check for timed out slots and execute its handlers
 *
 * @param[in]  thread  Pointer to thread
 * @param[in]  timeout_time The timeout value in ns
 *
 * @return     pointer to idle slot, NULL if do not have any idle slot.
 */
static void thread_check_timeout_slots(thr_thread_t *thread, unsigned timeout_time);

/**
 * @brief      Get the thread that contains given slot
 *
 * @param      slot  The pointer of slot
 *
 * @return     Pointer to thread that contains the slot, else NULL
 */
static thr_thread_t *thread_get_from_slot(thr_thread_slot_t *slot);

/**
 * @brief      Initialize data of thread slot
 *
 * @param      thread  The thread pointer
 */
static void thread_slot_init_data(thr_thread_t *thread);

/**
 * @brief      De-initialize data of thread slot
 *
 * @param      thread  The thread pointer
 */
static void thread_slot_deinit_data(thr_thread_t *thread);

/**
 * @brief      Sleep handler based on epoll_wait
 *
 * @param      thread  The thread pointer
 *
 * @return     n >= 0 if n events received.
 *             0 if call interrupted.
 *             -1 on other errors.
 */
int thread_sleep_epoll(thr_thread_t *thread_ptr, struct epoll_event *recv_events)
{
	int n;

	// Wait fd events for each 100ms
	n = epoll_wait(thread_ptr->poll_fd, recv_events, thread_ptr->max_slots, THREAD_HANDLER_TIMEOUT_MS);
	if (n < 0) {
		if (errno == EINTR)
			return 0;
		else {
			// Error occurs
			ERR("epoll_wait(%d) failed: %s", thread_ptr->poll_fd, strerror(errno));
			return -1;
		}
	} else
		return n;
}

int thread_sleep_nanosleep(thr_thread_t *thread_ptr, struct epoll_event *recv_events)
{
	unsigned int ready_slots = 0, total_wakeup_slots = 0;
	unsigned int next_wakeup_slots[MAX_THREAD_SLOTS];
	struct thread_sleep_nanosleep_data *nsleep;
	uint64_t sched_next = UINT64_MAX;
	struct timespec next_time;
	int i, rc;

	for (i = 0; i < thread_ptr->max_slots; i++) {
		if (thread_ptr->slots[i].is_used) {
			nsleep = (struct thread_sleep_nanosleep_data *)thread_ptr->slots[i].sleep_data;
			if (nsleep->is_armed && (nsleep->next <= sched_next)) {

				if (nsleep->next == sched_next)
					next_wakeup_slots[total_wakeup_slots++] = i;
				else {
					sched_next = nsleep->next;
					next_wakeup_slots[0] = i;
					total_wakeup_slots = 1;
				}
			}
		}
	}

	DBG("total_wakeup_slots(%d) sched_next(%" PRIu64 ")", total_wakeup_slots, sched_next);
	if (total_wakeup_slots == 0) { // Wait 100ms if no scheduled wake-up
		next_time.tv_sec = 0;
		next_time.tv_nsec = THREAD_HANDLER_TIMEOUT_NS;
		rc = clock_nanosleep(CLOCK_REALTIME, 0, &next_time, NULL);

	} else {
		next_time.tv_sec = sched_next / (NSECS_PER_SEC);
		next_time.tv_nsec = sched_next % (NSECS_PER_SEC);
		rc = clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next_time, NULL);
	}

	if (rc > 0) {
		if (rc == EINTR)
			return 0;
		else {
			// Error occurs
			ERR("clock_nanosleep for time (%" PRIu64 ") failed.", sched_next);
			return -1;
		}
	} else {
		for (i = 0; i < total_wakeup_slots; i++) {
			//thr_thread_slot_t *slot = recv_events[i].data.ptr;
			unsigned int slot_index = next_wakeup_slots[i];
			thr_thread_slot_t *slot = &thread_ptr->slots[slot_index];

			// check if the slot was not removed while sleeping
			// FIXME this is not completely safe, as the slot can be reused while sleeping.
			if (!thread_ptr->slots[i].is_used)
				continue;

			nsleep = (struct thread_sleep_nanosleep_data *)slot->sleep_data;
			// check if the timer was not stopped while sleeping;
			if (!nsleep->is_armed)
				continue;

			// this slot is now event ready
			recv_events[ready_slots].data.ptr = (void *)slot;
			recv_events[ready_slots].events = EPOLLIN; // needed to make sure the time-out count gets reset
			ready_slots++;

			// make sure we don't get woken up again for previous events
			nsleep->is_armed = 0;
		}

		return ready_slots;
	}
}

int thread_init(void)
{
	int count;
	int result;

	for (count = 0; count < MAX_THREADS; ++count) {
		thr_thread_t *thread_ptr = &g_thread_array[count];

		// Create epoll for the thread
		result = epoll_create1(0);
		if (result < 0) {
			ERR("Create epoll handle failed. Error %s", strerror(errno));
			return -1;
		}
		thread_ptr->poll_fd = result;

		DBG("epoll %d fd %d created", count, thread_ptr->poll_fd);

		// Init statistics
		stats_init(&thread_ptr->stats.epoll_event, 31, thread_ptr, NULL);
		stats_init(&thread_ptr->stats.time_2epoll, 31, thread_ptr, NULL);
		stats_init(&thread_ptr->stats.processing_time, 31, thread_ptr, NULL);

		thread_ptr->is_first_poll = 1;
		thread_ptr->last_poll_time = 0;
		thread_ptr->exit_flag = 0;
		thread_ptr->num_slots = 0;

		// Default sleep handler
		thread_ptr->sleep_handler = thread_sleep_epoll;

		// Slot data initialization
		thread_slot_init_data(thread_ptr);

		result = pthread_create(&thread_ptr->id, NULL, &s_data_thread_handle, (void *)(intptr_t) count);
		if (result != 0) {
			ERR("thread %d create failed, error %s", count, strerror(result));
			return -1;
		}
		DBG("Thread %d created with id %d", count, (int)thread_ptr->id);

	}

	// Initialize mutex
	pthread_mutex_init(&thread_mutex, NULL);

	return 0;
}

int thread_exit(void)
{
	int count;
	int result;
	void *retval;

	for (count = 0; count < MAX_THREADS; count++) {
		thr_thread_t *thread_ptr = &g_thread_array[count];

		// Request thread quit
		thread_ptr->exit_flag = 1;

		// join thread
		pthread_join(thread_ptr->id, &retval);
		DBG("thread %d id %d joint with result %d", count, (int)thread_ptr->id, (int)(intptr_t)retval);

		// Close epoll
		result = close(thread_ptr->poll_fd);
		if (result != 0) {
			ERR("epoll %d failed to close, error %s", thread_ptr->poll_fd, strerror(errno));
		} else {
			DBG("epoll %d closed", thread_ptr->poll_fd);
		}

		// De-init slot data
		thread_slot_deinit_data(thread_ptr);
	}
	pthread_mutex_destroy(&thread_mutex);

	return 0;
}

static void __thread_print_stats(thr_thread_t *thread_ptr)
{
	thr_thread_stat_t *stats = &thread_ptr->stats_snap;

	if (stats->pending) {
		INF_LOG("Thread (%p) epoll count: %d, epoll number: %d/%d/%d, epoll time: %6d/%7d/%7d processing: %6d/%7d/%7d",
			thread_ptr, stats->epoll_event.current_count, stats->epoll_event.min,
			stats->epoll_event.mean, stats->epoll_event.max, stats->time_2epoll.min,
			stats->time_2epoll.mean, stats->time_2epoll.max, stats->processing_time.min,
			stats->processing_time.mean, stats->processing_time.max);
		stats->pending = false;
	}
}

void thread_print_stats()
{
	int i;

	for (i = 0; i < MAX_THREADS; i++) {
		thr_thread_t *thread_ptr = &g_thread_array[i];

		if (thread_ptr->num_slots > 0)
			__thread_print_stats(thread_ptr);
	}
}

static void thread_dump_stats(thr_thread_t *thread_ptr)
{
	thr_thread_stat_t *stats = &thread_ptr->stats;
	thr_thread_stat_t *stats_snap = &thread_ptr->stats_snap;

	// Thread global statistics
	if (thread_ptr->num_slots > 0) {
		stats_compute(&stats->epoll_event);
		stats_compute(&stats->time_2epoll);
		stats_compute(&stats->processing_time);
		memcpy(stats_snap, stats, sizeof(thr_thread_stat_t));
		stats_reset(&stats->time_2epoll);
		stats_reset(&stats->epoll_event);
		stats_reset(&stats->processing_time);
		stats_snap->pending = true;
	}
}

static void *s_data_thread_handle(void *param)
{
	intptr_t final_result = 0;
	cpu_set_t cpu_set;
	int thread_index = (int)(intptr_t)param;
	thr_thread_t *thread_ptr = &g_thread_array[thread_index];
	struct epoll_event recv_events[MAX_THREAD_SLOTS];
	struct sched_param thread_param = {
		.sched_priority = thread_ptr->priority,
	};
	unsigned int poll_time;
	struct timespec ts;
	unsigned long long tlast_timeout = 0, now = 0, tlast_stat = 0;

	DBG("thread %d, cpu %d, poll_fd: %d, max_slots: %d", thread_index, thread_ptr->cpu_core, thread_ptr->poll_fd, thread_ptr->max_slots);
	// If the max_slots is 0, means the thread is not be used. Just quit.
	if (0 >= thread_ptr->max_slots) {
		INF("The thread %d on cpu %d will not be used because max_slots = 0", thread_index, thread_ptr->cpu_core);
		goto lb_exit;
	}

	// Make thread more real time
	if (sched_setscheduler(0, SCHED_FIFO, &thread_param) < 0) {
		ERR("%d - sched_setscheduler failed with error %d - %s", thread_index, errno, strerror(errno));
		final_result = -1;
		goto lb_exit;
	}

	// Specify which core number that the thread will run on
	CPU_ZERO(&cpu_set);
	CPU_SET(thread_ptr->cpu_core, &cpu_set);
	if (sched_setaffinity(0, sizeof(cpu_set), &cpu_set) == -1)
		ERR("%d - sched_setaffinity failed with error %d - %s", thread_index, errno, strerror(errno));

	// Loop until exit flag of the thread is set
	while (!thread_ptr->exit_flag) {
		int n, i, res;

		n = thread_ptr->sleep_handler(thread_ptr, recv_events);

		if (n < 0) {
			// Error occurs
			ERR("%d - sleep_handler failed", thread_index);
			final_result = -1;
			goto lb_exit;
		}

		res = clock_gettime(CLOCK_MONOTONIC, &ts);

		if (res == 0) {
			now = (unsigned long long) ts.tv_sec * NSECS_PER_SEC + ts.tv_nsec;

			if ((now - tlast_stat) >= THREAD_STATS_PERIOD_NS) {
				// Dump statistic each 10s
				thread_dump_stats(thread_ptr);
				tlast_stat = now;
			}

			if (!tlast_timeout)
				tlast_timeout = now;

			if (now - tlast_timeout >= THREAD_HANDLER_TIMEOUT_NS) {
				/*Check if we have timeout handlers to execute, every 100 ms*/
				thread_check_timeout_slots(thread_ptr, now - tlast_timeout);
				tlast_timeout = now;
			}
		} else {
			ERR("clock_gettime() failed: %s", strerror(errno));
		}

		/* Update events stats */
		stats_update(&thread_ptr->stats.epoll_event, n);

		/* If we got interrupted or timedout, loop again */
		if (n == 0)
			continue;


		if (0 != res) {
			ERR("could not get clock real-time, error: %s", strerror(errno));
		} else {
			poll_time = (unsigned long long)ts.tv_sec*NSECS_PER_SEC + ts.tv_nsec;
			if (!thread_ptr->is_first_poll) {
				stats_update(&thread_ptr->stats.time_2epoll, poll_time - thread_ptr->last_poll_time);
				// DBG("%p time_2epoll: %d - %d - %d", thread_ptr, thread_ptr->time_2epoll.current_count, poll_time - thread_ptr->last_poll_time, thread_ptr->time_2epoll.min);
				// DBG("poll: %d, %d", n, poll_time - last_poll_time);
			} else {
				thread_ptr->is_first_poll = 0;
			}
			thread_ptr->last_poll_time = poll_time;
		}

		// Events received, process them
		for (i = 0; i < n; ++i) {
			int events = recv_events[i].events;
			int fd = recv_events[i].data.fd;
			thr_thread_slot_t *slot = (thr_thread_slot_t *)recv_events[i].data.ptr;

			if (events & EPOLLERR) {
				ERR("%d - fd %d with poll error", thread_index, fd);
				continue;
			}

			if (!slot)
				continue;

			/* Reset timeout counter*/
			if (events & (EPOLLIN | EPOLLOUT))
				slot->timeout_count = 0;

			slot->handler(slot->data, events);
		}

		res = clock_gettime(CLOCK_MONOTONIC, &ts);
		if (res == 0) {
			poll_time = (unsigned long long)ts.tv_sec * NSECS_PER_SEC + ts.tv_nsec;
			stats_update(&thread_ptr->stats.processing_time, poll_time - thread_ptr->last_poll_time);
		} else {
			ERR("clock get real-time failed");
		}

		sched_yield();
	}

lb_exit:
	return (void *)final_result;
}

static int thread_count_used_slot(thr_thread_t *thread)
{
	int result = 0;
	int i;

	for (i = 0; i < thread->max_slots; ++i) {
		if (thread->slots[i].is_used != 0) {
			result ++;
		}
	}

	return result;
}

static int thread_check_capability_match(thr_thread_t *thread, int capabilities)
{
	int tmp = thread->thread_capabilities;

	// if not match capacity, return 0
	if ((tmp & capabilities) != capabilities) {
		return 0;
	}

	return 1;
}

static thr_thread_slot_t *thread_get_first_idle_slot(thr_thread_t *thread)
{
	int i;

	for (i = 0; i < thread->max_slots; ++i) {
		if (!thread->slots[i].is_used) {
			return &thread->slots[i];
		}
	}

	return NULL;
}

static void thread_check_timeout_slots(thr_thread_t *thread, unsigned int timeout_time)
{
	int i;

	for (i = 0; i < thread->max_slots; ++i) {
		if (thread->slots[i].is_used && thread->slots[i].timeout_handler) {
			thread->slots[i].timeout_count += timeout_time;
			if(thread->slots[i].timeout_count > thread->slots[i].max_timeout)
				thread->slots[i].timeout_handler(thread->slots[i].data);
		}
	}

}

int __thread_slot_add(int capabilities, int fd, unsigned int events, void *data,
		      int (*handler)(void *data, unsigned int events),
		      int (*timeout_handler)(void *data),
		      int max_timeout, thr_thread_slot_t **slot,
		      int (*sleep_handler)(thr_thread_t *thread_ptr, struct epoll_event *recv_events),
		      void *sleep_data)
{
	thr_thread_t *thread_ptr = NULL;
	thr_thread_slot_t *thread_slot_ptr = NULL;
	int i;
	int tmp;
	struct epoll_event event;

	if ((events != 0) && (sleep_handler != thread_sleep_epoll)) {
		ERR("Cannot use both epoll and custom sleep handler");
		return -1;
	}

	// Lock data
	pthread_mutex_lock(&thread_mutex);

	// Find best idle slot
	for (i = 0; i < MAX_THREADS; ++i) {
		thr_thread_t *cur_thread = &g_thread_array[i];

		if (!thread_check_capability_match(cur_thread, capabilities)) {
			// Thread does not match capabilities
			continue;
		}

		if ((cur_thread->num_slots != 0) && (cur_thread->sleep_handler != sleep_handler)) {
			//Thread already actively using another sleep handler
			continue;
		}

		tmp = thread_count_used_slot(cur_thread);
		if (tmp < cur_thread->max_slots) {
			// This thread has idle slot
			thread_ptr = cur_thread;
			INF("Using thread number %d", i);
			break;
		}
	}

	DBG("thread_ptr (%p)", thread_ptr);
	if (!thread_ptr) {
		// No thread found
		ERR("No thread found for capabilities 0x%x", capabilities);
		pthread_mutex_unlock(&thread_mutex);
		return -1;
	}

	DBG("CPU core: %d", thread_ptr->cpu_core);
	thread_slot_ptr = thread_get_first_idle_slot(thread_ptr);
	DBG("thread_slot_ptr (%p)", thread_slot_ptr);
	if (!thread_slot_ptr) {
		// No slot found
		ERR("No thread slot found");
		pthread_mutex_unlock(&thread_mutex);
		return -1;
	}

	DBG("Update slot (%p) info, fd: %d, data: %p, handler: %p", thread_slot_ptr, fd, data, handler);
	// Update slot information
	thread_slot_ptr->is_used = 1;
	thread_slot_ptr->data = data;
	thread_slot_ptr->handler = handler;
	thread_slot_ptr->timeout_count = 0;
	thread_slot_ptr->max_timeout = max_timeout;
	thread_slot_ptr->timeout_handler = timeout_handler;
	// Unlock the slot lock for first time
	pthread_mutex_unlock(&thread_slot_ptr->slot_lock);

	// Update output value
	DBG("slot (%p) -> %p", slot, thread_slot_ptr);
	thread_ptr->num_slots++;
	*slot = thread_slot_ptr;

	// Always set the sleep fields with the provided ones
	thread_ptr->sleep_handler = sleep_handler;
	thread_slot_ptr->sleep_data = sleep_data;

	if (events != 0) {
		// If events is provided, add slot into epoll monitor
		thread_slot_ptr->fd = fd;
		event.data.fd = fd;
		event.data.ptr = thread_slot_ptr;
		event.events = (events & (EPOLLIN | EPOLLOUT)) | EPOLLERR | EPOLLHUP;

		DBG("Add fd %d into epoll %d", fd, thread_ptr->poll_fd);
		if (-1 == epoll_ctl(thread_ptr->poll_fd, EPOLL_CTL_ADD, fd, &event)) {
			// epoll add fd error. Clear is_used flag and return NULL
			ERR("epoll_ctl failed, %s", strerror(errno));
			thread_slot_ptr->is_used = 0;
			thread_ptr->num_slots--;
			*slot = NULL;
			pthread_mutex_unlock(&thread_mutex);
			return -1;
		}
		INF("Add fd %d into epoll of thread %p", fd, thread_ptr);
	} else {
		// This is a nanosleep handler
		INF("Enabled slot %p of thread %p with nanosleep handler", thread_slot_ptr, thread_ptr);
	}

	thread_slot_ptr->is_enabled = 1;

	pthread_mutex_unlock(&thread_mutex);

	return 0;
}

int thread_slot_add(int capabilities, int fd, unsigned int events, void *data,
		    int (*handler)(void *data, unsigned int events),
		    int (*timeout_handler)(void *data),
		    int max_timeout, thr_thread_slot_t **slot)
{
	return __thread_slot_add(capabilities, fd, events, data,
				 handler, timeout_handler, max_timeout, slot, thread_sleep_epoll, NULL);
}

int thread_slot_set_events(thr_thread_slot_t *slot, int enable, unsigned int req_events)
{
	struct epoll_event event;
	thr_thread_t *thread;

	// Lock data
	pthread_mutex_lock(&thread_mutex);

	thread = thread_get_from_slot(slot);
	if (!thread) {
		ERR("Thread not found for slot %p", slot);
		pthread_mutex_unlock(&thread_mutex);
		return -1;
	}

	if(enable) {

		if (slot->is_enabled) {
			pthread_mutex_unlock(&thread_mutex);
			return 0;
		}

		// add slot into epoll monitor

		event.data.fd = slot->fd;
		event.data.ptr = slot;
		event.events = (req_events & (EPOLLIN | EPOLLOUT)) | EPOLLERR | EPOLLHUP;

		DBG("Add fd %d into epoll %d", slot->fd, thread->poll_fd);
		if (-1 == epoll_ctl(thread->poll_fd, EPOLL_CTL_ADD, slot->fd, &event)) {
			// epoll add fd error. Clear is_used flag and return NULL
			ERR("epoll_ctl failed, %s", strerror(errno));
			pthread_mutex_unlock(&thread_mutex);
			return -1;
		}

		slot->is_enabled = 1;
	} else {

		if (!slot->is_enabled) {
			pthread_mutex_unlock(&thread_mutex);
			return 0;
		}
		// remove slot from epoll monitor
		DBG("remove fd %d from epoll", slot->fd);
		if (-1 == epoll_ctl (thread->poll_fd, EPOLL_CTL_DEL, slot->fd, NULL)) {
			// epoll delete fd error.
			ERR("remove the fd %d failed", slot->fd);
			pthread_mutex_unlock(&thread_mutex);
			return -1;
		}
		slot->is_enabled = 0;
	}

	pthread_mutex_unlock(&thread_mutex);
	return 0;
}

int thread_slot_free(thr_thread_slot_t *slot)
{
	thr_thread_t *thread;
	// Lock data
	pthread_mutex_lock(&thread_mutex);

	thread = thread_get_from_slot(slot);
	if (!thread) {
		ERR("Thread not found for slot %p", slot);
		pthread_mutex_unlock(&thread_mutex);
		return -1;
	}

	if (!slot->is_enabled)
		goto deinit;

	if (thread->sleep_handler == thread_sleep_epoll) {
		// remove slot from epoll monitor
		DBG("remove fd %d from epoll", slot->fd);
		if (-1 == epoll_ctl (thread->poll_fd, EPOLL_CTL_DEL, slot->fd, NULL)) {
			// epoll delete fd error.
			ERR("remove the fd %d failed", slot->fd);
			pthread_mutex_unlock(&thread_mutex);
			return -1;
		}
	} else {
		//sleep_handler is thread_sleep_nanosleep
		struct thread_sleep_nanosleep_data *nsleep;

		nsleep = (struct thread_sleep_nanosleep_data *)slot->sleep_data;

		nsleep->is_armed = 0;

	}

deinit:
	// epoll fd is removed
	slot->is_used = 0;
	thread->num_slots --;
	slot->is_enabled = 0;
	slot->timeout_count = 0;
	slot->max_timeout = 0;
	slot->timeout_handler = NULL;

	pthread_mutex_unlock(&thread_mutex);

	return 0;
}

static thr_thread_t *thread_get_from_slot(thr_thread_slot_t *slot)
{
	int i;
	int j;

	for (i = 0; i < MAX_THREADS; ++i) {
		thr_thread_t *cur_thread = &g_thread_array[i];
		for (j = 0; j < cur_thread->max_slots; ++j) {
			thr_thread_slot_t *cur_slot = &cur_thread->slots[j];

			if (!cur_slot->is_used) {
				continue;
			}

			if (cur_slot == slot) {
				return cur_thread;
			}
		}
	}

	return NULL;
}

static inline void thread_slot_reset_data(thr_thread_slot_t *slot)
{
	slot->is_used = 0;
	slot->fd = 0;
	slot->data = NULL;
	slot->handler = NULL;
	slot->timeout_count = 0;
	slot->max_timeout = 0;
	slot->timeout_handler = NULL;
}

static void thread_slot_init_data(thr_thread_t *thread)
{
	int i;

	for (i = 0; i < MAX_THREAD_SLOTS; ++i) {
		thr_thread_slot_t *slot = &thread->slots[i];

		// Reset data of slot
		thread_slot_reset_data(slot);
		// Initialize mutex lock
		pthread_mutex_init(&slot->slot_lock, NULL);
	}
}

static void thread_slot_deinit_data(thr_thread_t *thread)
{
	int i;

	for (i = 0; i < MAX_THREAD_SLOTS; ++i) {
		thr_thread_slot_t *slot = &thread->slots[i];

		// Reset data of slot
		thread_slot_reset_data(slot);
		// Destroy mutex lock
		pthread_mutex_destroy(&slot->slot_lock);
	}
}
