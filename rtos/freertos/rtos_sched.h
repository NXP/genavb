/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _RTOS_ABSTRACTION_LAYER_SCHEDULING_H_
#define _RTOS_ABSTRACTION_LAYER_SCHEDULING_H_

#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "rtos_time.h"

typedef unsigned int rtos_spinlock_t;
typedef unsigned int rtos_spinlock_key_t;

static inline void rtos_sleep(rtos_tick_t ticks)
{
    vTaskDelay(ticks);
}

static inline void rtos_yield_from_isr(bool yield)
{
    portYIELD_FROM_ISR((yield)? pdTRUE : pdFALSE);
}

static inline void rtos_spin_lock(rtos_spinlock_t *lock, rtos_spinlock_key_t *key)
{
    taskENTER_CRITICAL();
}

static inline void rtos_spin_unlock(rtos_spinlock_t *lock, rtos_spinlock_key_t key)
{
    taskEXIT_CRITICAL();
}

static inline void rtos_yield(void)
{
    taskYIELD();
}

static inline void rtos_mutex_global_lock(void)
{
    vTaskSuspendAll();
}

static inline void rtos_mutex_global_unlock(void)
{
    xTaskResumeAll();
}

extern rtos_spinlock_key_t rtos_global_key;
extern rtos_spinlock_t rtos_global_spinlock;

#endif /* #ifndef _RTOS_ABSTRACTION_LAYER_SCHEDULING_H_ */
