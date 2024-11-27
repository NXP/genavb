/*
 * Copyright 2019-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief RTOS generic hw clock service implementation
 @details
*/

#ifndef _RTOS_CLOCK_H_
#define _RTOS_CLOCK_H_

#include "os/clock.h"
#include "common/types.h"

#include "hw_clock.h"

/* Clock ratio adjustments affect the hardware clock */
#define OS_CLOCK_FLAGS_HW_RATIO  (1 << 0)
/* Clock offset adjustments affect the hardware clock */
#define OS_CLOCK_FLAGS_HW_OFFSET (1 << 1)

os_clock_id_t clock_id(os_clock_id_t clk_id);

int os_clock_gettime64_isr(os_clock_id_t clk_id, u64 *ns);

/**
 * Convert hw clock time to sw clock time.
 * The time must come from the root hw clock of the sw clock
 * given as argument.
 * \param id		clock id.
 * \param hw_ns		hw time to convert
 * \param ns		pointer to u64 variable that will hold the result.
 * \return		0 on success, or negative value on error.
 */
int clock_time_from_hw(os_clock_id_t id, uint64_t hw_ns, uint64_t *ns);

/**
 * Convert sw clock ns time to hw clock cycles.
 * The provided cycles is the raw counter value of the base hw clock
 * of sw clock time given as argument.
 * The _isr variant needs to be used when called in IRQ context.
 * \param id		clock id.
 * \param ns		time to convert in ns.
 * \param cycles	pointer to u64 variable that will hold the result.
 * \return		0 on success, or negative value on error.
 */
int clock_time_to_cycles(os_clock_id_t id, uint64_t ns, uint64_t *cycles);
int __clock_time_to_cycles(os_clock_id_t clk_id, uint64_t ns, uint64_t *cycles);
int __clock_dtime_to_cycles(os_clock_id_t clk_id, uint64_t delta_ns, uint64_t *delta_cycles);

static inline int clock_time_to_cycles_isr(os_clock_id_t clk_id, uint64_t ns, uint64_t *cycles)
{
	return __clock_time_to_cycles(clk_id, ns, cycles);
}

static inline int clock_dtime_to_cycles_isr(os_clock_id_t clk_id, uint64_t delta_ns, uint64_t *delta_cycles)
{
	return __clock_dtime_to_cycles(clk_id, delta_ns, delta_cycles);
}

hw_clock_id_t clock_to_hw_clock(os_clock_id_t clk_id);

int clock_id_remap(os_clock_id_t clk_id, os_clock_id_t dst_clk_id);
int clock_to_hw_clock_set(os_clock_id_t clk_id, hw_clock_id_t hw_clk_id);
int clock_set_flags(os_clock_id_t clk_id, unsigned int flags);

bool clock_has_hw_adjust_ratio(os_clock_id_t clk_id);
bool clock_has_hw_setoffset(os_clock_id_t clk_id);

/**
 * Initialize clock layer.
 * \return     0 on success or -1 on error.
 */
int os_clock_init(void);

/**
 * Close clock layer.
 * \return     none
 */
void os_clock_exit(void);

#endif /* _RTOS_CLOCK_H_ */
