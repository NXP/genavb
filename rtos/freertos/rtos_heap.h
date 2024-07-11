/*
* Copyright 2024 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/
#ifndef _RTOS_ABSTRACTION_LAYER_HEAP_H_
#define _RTOS_ABSTRACTION_LAYER_HEAP_H_

#include "FreeRTOS.h"

static inline void *rtos_malloc(size_t size)
{
    return pvPortMalloc(size);
}

static inline void rtos_free(void *pointer)
{
    return vPortFree(pointer);
}

#endif /* _RTOS_ABSTRACTION_LAYER_HEAP_H_ */
