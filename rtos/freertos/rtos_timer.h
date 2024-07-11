/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _RTOS_ABSTRACTION_LAYER_TIMER_H_
#define _RTOS_ABSTRACTION_LAYER_TIMER_H_

#include <stdbool.h>
#include "FreeRTOS.h"
#include "timers.h"
#include "rtos_time.h"

typedef struct _rtos_timer_t {
    TimerHandle_t handle;
    void *data;
    void (*callback)(struct _rtos_timer_t *, void *);
    bool is_static;
    StaticTimer_t storage;
} rtos_timer_t;

typedef void (*rtos_timer_callback_t)(rtos_timer_t *timer, void *);

/** Start a timer.
 *
 * This function starts a timer. It resets the state of the timer.
 *
 * \ingroup rtos_timer
 * \return 	0 if success, -1 otherwise.
 * \param 	timer Pointer to the timer.
 * \param 	period Timer period.
 */
static inline int rtos_timer_start(rtos_timer_t *timer, rtos_tick_t period)
{
    BaseType_t ret;

    ret = xTimerChangePeriod(timer->handle, period, RTOS_NO_WAIT);

    return (ret == pdPASS)? 0 : -1;
}

/** Stop a timer.
 *
 * This function stops a timer.
 *
 * \ingroup rtos_timer
 * \return 	0 if success, -1 otherwise.
 * \param 	timer Pointer to the timer.
 */
static inline int rtos_timer_stop(rtos_timer_t *timer)
{
    BaseType_t ret;

    ret = xTimerStop(timer->handle, 0);

    return (ret == pdPASS)? 0 : -1;
}

/** Initialize a timer.
 *
 * This function initializes a timer before it can be used.
 *
 * \ingroup rtos_timer
 * \return 	0 if success, -1 otherwise.
 * \param 	timer Pointer to the timer.
 * \param 	name Text name, for debugging purpose. Can be called after init until the timer is destroyed.
 * \param 	periodic If true, timer is periodic, else it is a one-shot timer.
 * \param 	callback Callback function called at each expiration of the timer.
 * \param 	data Pointer passed to the callback.
 */
int rtos_timer_init(rtos_timer_t *timer, const char *name, bool periodic, rtos_timer_callback_t callback, void *data);

/** Allocate and initialize a timer.
 *
 * This function allocates and initializes a timer.
 *
 * \ingroup rtos_timer
 * \return 	Pointer to the timer.
 * \param 	name Text name, for debugging purpose. Can be called after init until the timer is destroyed.
 * \param 	periodic If true, timer is periodic, else it is a one-shot timer.
 * \param 	callback Callback function called at each expiration of the timer.
 * \param 	data Pointer passed to the callback.
 */
rtos_timer_t *rtos_timer_alloc_init(const char *name, bool periodic, rtos_timer_callback_t callback, void *data);

/** Delete a timer.
 *
 * This function deletes a timer.
 *
 * \ingroup rtos_timer
 * \return 	0 if success, -1 otherwise.
 * \param 	timer Pointer to the timer.
 * \param 	expiry_time Timeout period.
 */
int rtos_timer_destroy(rtos_timer_t *timer, rtos_tick_t expiry_time);

#endif /* #ifndef _RTOS_ABSTRACTION_LAYER_TIMER_H_ */
