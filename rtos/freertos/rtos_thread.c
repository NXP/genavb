/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "rtos_thread.h"

int rtos_thread_create(rtos_thread_t *thread, unsigned int priority, unsigned int affinity,
                       size_t stack_size, const char *name,
                       void (*start_routine)(void *), void *arg)
{
    BaseType_t ret;

    ret = xTaskCreate(start_routine, name,
                      (stack_size < configMINIMAL_STACK_SIZE) ? configMINIMAL_STACK_SIZE: stack_size,
                      arg, (priority >= configMAX_PRIORITIES) ? configMAX_PRIORITIES - 1 : priority,
                      &thread->handle);

    return (ret != pdPASS)? -1 : 0;
}

void rtos_thread_abort(rtos_thread_t *thread)
{
    if (thread)
        vTaskDelete(thread->handle);
    else
        vTaskDelete(NULL);
}
