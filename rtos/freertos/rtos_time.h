/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Time abstraction.
 @details
*/

#ifndef _RTOS_ABSTRACTION_LAYER_TIME_H_
#define _RTOS_ABSTRACTION_LAYER_TIME_H_

#include "FreeRTOS.h"
#include "task.h"

typedef TickType_t rtos_tick_t;

#define RTOS_WAIT_FOREVER portMAX_DELAY
#define RTOS_NO_WAIT 0

static inline rtos_tick_t RTOS_MS_TO_TICKS(unsigned int msec)
{
    return pdMS_TO_TICKS(msec);
}

static inline rtos_tick_t RTOS_UINT_TO_TICKS(unsigned int ticks)
{
    return (rtos_tick_t)ticks;
}

static inline rtos_tick_t RTOS_MS_TO_TICKS_AT_LEAST(unsigned int uMsec, unsigned int xMinTicks)
{
    rtos_tick_t xMsecTicks;

    xMsecTicks = pdMS_TO_TICKS(uMsec);

    return (xMsecTicks < xMinTicks ? xMinTicks : xMsecTicks);
}

static inline unsigned int RTOS_TICKS_TO_UINT(rtos_tick_t ticks)
{
    return ticks;
}

static inline uint64_t rtos_get_current_time()
{
    return xTaskGetTickCount();
}

#endif /* _RTOS_ABSTRACTION_LAYER_TIME_H_ */
