/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _RTOS_ABSTRACTION_LAYER_MUTEX_H_
#define _RTOS_ABSTRACTION_LAYER_MUTEX_H_

#include "FreeRTOS.h"
#include "semphr.h"
#include "rtos_time.h"

typedef struct {
    SemaphoreHandle_t handle;
    StaticSemaphore_t storage;
} rtos_mutex_t;

/** Initialize a mutex.
 *
 * This function initializes a mutex before it can be used.
 *
 * \ingroup rtos_mutex
 * \return 	0 if success, -1 otherwise.
 * \param 	mutex Pointer to the mutex.
 */
static inline int rtos_mutex_init(rtos_mutex_t *mutex)
{
    mutex->handle = xSemaphoreCreateMutexStatic(&mutex->storage);

    if (!mutex->handle)
        return -1;

    return 0;
}

/** Unlock a mutex.
 *
 * This function unlocks a mutex. The calling thread must already be its owner.
 *
 * \ingroup rtos_mutex
 * \return 	0 if success, -1 otherwise.
 * \param 	mutex Pointer to the mutex.
 */
static inline int rtos_mutex_unlock(rtos_mutex_t *mutex)
{
    BaseType_t ret;

    ret = xSemaphoreGive(mutex->handle);

    return (ret == pdPASS) ? 0 : -1;
}

/** Lock a mutex.
 *
 * This function locks a mutex. If the mutex is already locked, the current
 * thread is blocked until the mutex becomes available or the timeout
 * period expires.
 *
 * \ingroup rtos_mutex
 * \return 	0 if success, -1 otherwise.
 * \param 	mutex Pointer to the mutex.
 * \param 	ticks Timeout period.
 */
static inline int rtos_mutex_lock(rtos_mutex_t *mutex, rtos_tick_t ticks)
{
    BaseType_t ret;

    ret = xSemaphoreTake(mutex->handle, ticks);

    return (ret == pdPASS) ? 0 : -1;
}

#endif /* #ifndef _RTOS_ABSTRACTION_LAYER_MUTEX_H_ */
