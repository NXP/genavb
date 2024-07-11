/*
 * Copyright 2018-2019, 2021-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "rtos_abstraction_layer.h"

void rtos_atomic_add(unsigned int val, rtos_atomic_t *addr)
{
	rtos_spin_lock(&rtos_global_spinlock, &rtos_global_key);

	*addr += val;

	rtos_spin_unlock(&rtos_global_spinlock, rtos_global_key);
}

void rtos_atomic_sub(unsigned int val, rtos_atomic_t *addr)
{
	rtos_spin_lock(&rtos_global_spinlock, &rtos_global_key);

	*addr -= val;

	rtos_spin_unlock(&rtos_global_spinlock, rtos_global_key);
}

unsigned int rtos_atomic_xchg(rtos_atomic_t *addr, unsigned int new)
{
	unsigned int ret;

	rtos_spin_lock(&rtos_global_spinlock, &rtos_global_key);

	ret = *addr;
	*addr = new;

	rtos_spin_unlock(&rtos_global_spinlock, rtos_global_key);

	return ret;
}
