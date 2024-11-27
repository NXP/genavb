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

#ifndef _HW_CLOCK_H_
#define _HW_CLOCK_H_

#include "common/types.h"

typedef enum {
	HW_CLOCK_NONE = 0,
	HW_CLOCK_MONOTONIC = 1,
	HW_CLOCK_MONOTONIC_1,
	HW_CLOCK_PORT_BASE,
	HW_CLOCK_MAX = 6
} hw_clock_id_t;

/*
 * Clock locking model.
 *
 * Two global locks are used. One protects from task preemption using
 * rtos_mutex_global_lock()/rtos_mutex_global_unlock(), the other from interrupts preemption
 * using rtos_spin_lock()/rtos_spin_unlock().
 *
 * The task preemption lock is the responsibility of the hw_clock layer user.
 * The interrupt preemption lock allows calling readers in interrupt context and
 * is performed here to do it in the most efficient way.
 *
 * The hw_clock object (nsecs + last_cycles members) needs to be updated atomically
 * so that readers don't get corrupted time.
 * Readers: hw_clock_time_to_cycles(), hw_clock_cycles_to_time(), hw_clock_read_isr()
 * Writers: hw_clock_read()
 *
 * Readers in task context are protected against task preemption
 * Readers in interrupt context are protected by an IRQ disable done by the writer.
 * We can't have any writer in interrupt context (otherwise all readers/writers would
 * need protection agains interrupt preemption).
 *
 */

/**
 * struct hw_clock - abstraction for a cycle counter
 *  	Provides time in ns since start of the HW counter and conversion functions
 *  	from time in nanoseconds to counter cycles.
 * 	This implementation has no restrictions concerning the period value (no
 * 	need to be a power of 2) but on the other hand does not scale to a full
 * 	64 bits register support.
 *
 * @read_counter:	returns raw counter value
 * @adj_freq:		adjust counter frequency in ppb
 * @priv:		private data used by the underlying clock driver
 * @period:		period in counter cycles
 * @rate: 		rate of the counter
 * @frac:		cycles remainder of time conversion
 * @nsecs:		ns count
 * @last_cycles:	last cycles read by read_counter
 */
struct hw_clock {
	uint64_t (*read_counter)(void *priv);
	int (*set_offset)(void *priv, int64_t offset);
	int (*adj_freq)(void *priv, int32_t ppb);
	int (*convert)(void *priv, uint64_t ns_src, struct hw_clock *clk_dst, uint64_t *ns_dst);
	void *priv;
	uint64_t period;
	uint32_t rate;
	struct {
		uint32_t shift;
		uint32_t mult;
	} to_ns;
	struct {
		uint32_t shift;
		uint32_t mult;
	} to_cyc;

	int64_t frac;
	uint64_t nsecs;
	uint64_t last_cycles;
};

int hw_clock_register(hw_clock_id_t id, struct hw_clock *clk);
void hw_clock_unregister(hw_clock_id_t id);
struct hw_clock *hw_clock_get(hw_clock_id_t id);

uint64_t hw_clock_read(struct hw_clock *clk);
uint64_t hw_clock_read_isr(struct hw_clock *clk);
int hw_clock_setfreq(struct hw_clock *clk, int32_t ppb);
int hw_clock_setoffset(struct hw_clock *clk, int64_t offset);

uint64_t hw_clock_cycles_to_time(struct hw_clock *clk, uint64_t cycles);
int hw_clock_time_to_cycles(struct hw_clock *clk, uint64_t ns, uint64_t *cycles);
int hw_clock_dtime_to_cycles(struct hw_clock *clk, uint64_t delta_ns, uint64_t *delta_cycles);

int hw_clock_convert(struct hw_clock *clk_src, uint64_t ns_src, struct hw_clock *clk_dst, uint64_t *ns_dst);
#endif /* _HW_CLOCK_H_ */
