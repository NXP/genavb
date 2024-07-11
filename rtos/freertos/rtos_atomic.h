/*
* Copyright 2018, 2020, 2023-2024 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#ifndef _RTOS_ABSTRACTION_LAYER_ATOMIC_H_
#define _RTOS_ABSTRACTION_LAYER_ATOMIC_H_

#include "FreeRTOS.h"
#include "task.h"

typedef uint32_t rtos_atomic_t;

/* if rtos_atomic_t is encoded in 32 bits */
#define ATOMIC_MASK 0x1f
#define ATOMIC_SHIFT 5

static inline void rtos_atomic_set(rtos_atomic_t *addr, unsigned int val)
{
	*(volatile rtos_atomic_t *)addr = val;
}

static inline unsigned int rtos_atomic_read(rtos_atomic_t *addr)
{
	return *(volatile rtos_atomic_t *)addr;
}

void rtos_atomic_add(unsigned int val, rtos_atomic_t *addr);
void rtos_atomic_sub(unsigned int val, rtos_atomic_t *addr);
unsigned int rtos_atomic_xchg(rtos_atomic_t *addr, unsigned int new);

static inline void rtos_atomic_inc(rtos_atomic_t *addr)
{
	rtos_atomic_add(1, addr);
}

static inline void rtos_atomic_dec(rtos_atomic_t *addr)
{
	rtos_atomic_sub(1, addr);
}

static inline void rtos_atomic_clear_bit(unsigned int nr, rtos_atomic_t *addr)
{
	rtos_atomic_t *word = ((rtos_atomic_t *)addr + (nr >> ATOMIC_SHIFT));

	taskENTER_CRITICAL();

	*word &= ~(1 << (nr & ATOMIC_MASK));

	taskEXIT_CRITICAL();
}

static inline void rtos_atomic_set_bit(unsigned int nr, rtos_atomic_t *addr)
{
	rtos_atomic_t *word = ((rtos_atomic_t *)addr + (nr >> ATOMIC_SHIFT));

	taskENTER_CRITICAL();

	*word |= (1 << (nr & ATOMIC_MASK));

	taskEXIT_CRITICAL();
}

static inline int rtos_atomic_test_bit(unsigned int nr, rtos_atomic_t *addr)
{
	rtos_atomic_t *word = ((rtos_atomic_t *)addr + (nr >> ATOMIC_SHIFT));
	int ret;

	taskENTER_CRITICAL();

	ret = (*word & (1 << (nr & ATOMIC_MASK)));

	taskEXIT_CRITICAL();

	return ret;
}

static inline int rtos_atomic_test_and_clear_bit(unsigned int nr, rtos_atomic_t *addr)
{
	rtos_atomic_t *word = ((rtos_atomic_t *)addr + (nr >> ATOMIC_SHIFT));
	int ret;

	taskENTER_CRITICAL();

	ret = (*word & (1 << (nr & ATOMIC_MASK)));
	*word &= ~(1 << (nr & ATOMIC_MASK));

	taskEXIT_CRITICAL();

	return ret;
}

static inline int rtos_atomic_test_and_set_bit(unsigned int nr, rtos_atomic_t *addr)
{
	rtos_atomic_t *word = ((rtos_atomic_t *)addr + (nr >> ATOMIC_SHIFT));
	int ret;

	taskENTER_CRITICAL();

	ret = (*word & (1 << (nr & ATOMIC_MASK)));
	*word |= (1 << (nr & ATOMIC_MASK));

	taskEXIT_CRITICAL();

	return ret;
}

#endif /* _RTOS_ABSTRACTION_LAYER_ATOMIC_H_ */
