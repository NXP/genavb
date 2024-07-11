/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2019-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Timer Service OS abstraction
 @details
*/

#ifndef _OS_TIMER_H_
#define _OS_TIMER_H_

#include "osal/timer.h"
#include "os/clock.h"

#define OS_TIMER_FLAGS_ABSOLUTE	(1 << 0)
#define OS_TIMER_FLAGS_PPS	(1 << 1)
#define OS_TIMER_FLAGS_RECOVERY	(1 << 2)
#define OS_TIMER_FLAGS_PERIODIC (1 << 3)

/** Allocate/create a new timer
 * \return	0 on success or -1 on error
 * \param t	pointer to the OS-dependent struct os_timer to create
 * \param id	clock id
 * \param flags flags used to request a timer capable of generating a pps signal
 * \param func	function called when timer expires
 * \param priv	OS-dependent private data
 */
int os_timer_create(struct os_timer *t, os_clock_id_t id, unsigned int flags, void (*func)(struct os_timer *t, int count), unsigned long priv);

/** Setup and start a timer created by os_timer_create()
 *  For periodic timers, the first expiration happens at the end of the first
 *  period (value + period).
 * \return	0 on success or -1 on error.
 * \param t	pointer to the OS-dependent struct os_timer to start
 * \param value expiration time in nanoseconds (for a one shot timer). Start of the first period (for a periodic timer), usually 0.
 * \param interval_p periodic timer value in nanoseconds (in the form p/q). If zero, the timer is one shot.
 * \param interval_q periodic timer value in nanoseconds (in the form p/q). Set to 1, for a integer timer period.
 * \param flags	indicates if "value" is a relative or absolute time
 */
int os_timer_start(struct os_timer *t, u64 value, u64 interval_p, u64 interval_q, unsigned int flags);

/** Stop a running timer
 * \return	none
 * \param t	pointer to the OS-dependent struct os_timer to stop
 */
void os_timer_stop(struct os_timer *t);

/** Destroy/free a timer
 * \return	none
 * \param t	pointer to the OS-dependent struct os_timer to destroy
 */
void os_timer_destroy(struct os_timer *t);

/** Process a timer
 * \return	none
 * \param t	pointer to the OS-dependent struct os_timer to process
 */
void os_timer_process(struct os_timer *t);


#endif /* _OS_TIMER_H_ */
