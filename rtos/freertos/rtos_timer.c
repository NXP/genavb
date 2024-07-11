/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "rtos_timer.h"

/* Internal struct for lighter dynamic allocations */
struct __rtos_timer_t {
    TimerHandle_t handle;
    void *data;
    void (*callback)(struct _rtos_timer_t *, void *);
    bool is_static;
};

static void rtos_timer_callback(TimerHandle_t xTimer)
{
    rtos_timer_t *timer = pvTimerGetTimerID(xTimer);

    timer->callback(timer, timer->data);
}

int rtos_timer_init(rtos_timer_t *timer, const char *name, bool periodic, rtos_timer_callback_t callback, void *data)
{
    timer->data = data;
    timer->callback = callback;

    /* uses a callback as interface to use a TimerCallbackFunction_t function as input and use a rtos_timer_callback_t behind the hood */
    timer->handle = xTimerCreateStatic(name, portMAX_DELAY, periodic, timer, rtos_timer_callback, &timer->storage);
    if (!timer->handle)
        goto err;

    timer->is_static = true;

    return 0;

err:
    return -1;
}

rtos_timer_t *rtos_timer_alloc_init(const char *name, bool periodic, rtos_timer_callback_t callback, void *data)
{
    rtos_timer_t *timer;

    /* Using internal lighter struct to let FreeRTOS do the allocation of the Timer */
    timer = (rtos_timer_t *)pvPortMalloc(sizeof(struct __rtos_timer_t));
    if (!timer)
        goto err_alloc;

    timer->data = data;
    timer->callback = callback;

    /* use a callback as interface to use a TimerCallbackFunction_t function as input and use a rtos_timer_callback_t behind the hood */
    timer->handle = xTimerCreate(name, portMAX_DELAY, periodic, timer, rtos_timer_callback);
    if (!timer->handle)
        goto err_init;

    timer->is_static = false;

    return timer;

err_init:
    vPortFree(timer);

err_alloc:
    return NULL;
}

int rtos_timer_destroy(rtos_timer_t *timer, rtos_tick_t expiry_time)
{
    BaseType_t ret;

    ret = xTimerDelete(timer->handle, expiry_time);
    if (ret == pdFAIL)
        goto err;

    if (timer->is_static) {
        while (xTimerIsTimerActive(timer->handle) == pdTRUE)
            vTaskDelay(pdMS_TO_TICKS(1));
    } else {
        vPortFree(timer);
    }

    return 0;

err:
    return -1;
}

