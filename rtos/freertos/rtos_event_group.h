/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _RTOS_ABSTRACTION_LAYER_EVENT_GROUP_H_
#define _RTOS_ABSTRACTION_LAYER_EVENT_GROUP_H_

#include <stdbool.h>
#include "FreeRTOS.h"
#include "event_groups.h"
#include "rtos_time.h"

typedef struct {
    EventGroupHandle_t handle;
    StaticEventGroup_t storage;
} rtos_event_group_t;

/** Initialize an event group.
 *
 * This function initializes an event group before it can be used.
 *
 * \ingroup rtos_event_group
 * \return 	0 if success, -1 otherwise.
 * \param 	event_group Pointer to the event group.
 */
static inline int rtos_event_group_init(rtos_event_group_t *event_group)
{
    event_group->handle = xEventGroupCreateStatic(&event_group->storage);
    if (!event_group->handle)
        return -1;

    return 0;
}

/** Send one or more events to an event group.
 *
 * This function sends one or more events to an event group.
 *
 * \ingroup rtos_event_group
 * \return 	Value of the events set in the event group.
 * \param 	event_group Pointer to the event group.
 * \param 	events Set of events to set in the event group.
 */
static inline uint32_t rtos_event_group_set(rtos_event_group_t *event_group, uint32_t events)
{
    return (uint32_t)xEventGroupSetBits(event_group->handle, events);
}

/** Wait for any of the specified events.
 *
 * This function waits on an event group until it receives any of the specified events.
 * The current thread is blocked until an event is received or it reaches a timeout.
 *
 * \ingroup rtos_event_group
 * \return 	Value of the events set in the event group.
 * \param 	event_group Pointer to the event group.
 * \param 	events Set of events on which to wait.
 * \param 	reset If true, clear the set of events from event_group tracked by the event group on exit of this function (after wait). If false, do not clear the events.
 * \param 	ticks Timeout period.
 */
static inline uint32_t rtos_event_group_wait(rtos_event_group_t *event_group, uint32_t events, bool reset, rtos_tick_t ticks)
{
    return (uint32_t)xEventGroupWaitBits(event_group->handle, events, reset, pdFALSE, ticks);
}

#endif /* #ifndef _RTOS_ABSTRACTION_LAYER_EVENT_GROUP_H_ */
