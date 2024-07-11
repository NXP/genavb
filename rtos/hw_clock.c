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

#include "hw_clock.h"
#include "common/log.h"
#include "os/stdlib.h"

#include "rtos_abstraction_layer.h"

#define NSECS_PER_SEC		1000000000

static struct hw_clock *hw_clk_array[HW_CLOCK_MAX];

static uint64_t hw_clock_cycles_to_ns(struct hw_clock *clk, uint64_t cycles, int64_t *frac)
{
	uint64_t ns;

	ns = ((cycles * clk->to_ns.mult) + *frac);
	*frac = ns & ((1 << clk->to_ns.shift) - 1);

	return ns >> clk->to_ns.shift;
}

__init static void hw_clock_init(struct hw_clock *clk)
{
	clk->last_cycles = clk->read_counter(clk->priv);
	clk->frac = 0;
	clk->to_ns.mult = (((uint64_t)NSECS_PER_SEC << clk->to_ns.shift) + clk->rate / 2) / clk->rate;
	clk->to_cyc.mult = (((uint64_t)clk->rate << clk->to_cyc.shift) + NSECS_PER_SEC / 2) / NSECS_PER_SEC;
	clk->nsecs = hw_clock_cycles_to_ns(clk, clk->last_cycles, &clk->frac);

	os_log(LOG_INIT, "rate: %u, period: %llx, mult(to ns): %u, shift(to ns): %u, mult(to cycles): %u, shift(to cycles): %u\n",
			clk->rate, clk->period,
			clk->to_ns.mult, clk->to_ns.shift,
			clk->to_cyc.mult, clk->to_cyc.shift);
}

__init int hw_clock_register(hw_clock_id_t id, struct hw_clock *clk)
{
	if (hw_clk_array[id] || !clk || id >= HW_CLOCK_MAX)
		return -1;

	hw_clock_init(clk);

	hw_clk_array[id] = clk;

	os_log(LOG_INIT, "hw_clock(%d) registered\n", id);

	return 0;
}

__exit void hw_clock_unregister(hw_clock_id_t id)
{
	hw_clk_array[id] = NULL;

	os_log(LOG_INIT, "hw_clock(%d) unregistered\n", id);
}

struct hw_clock *hw_clock_get(hw_clock_id_t id)
{
	if (id >= HW_CLOCK_MAX)
		return NULL;

	return hw_clk_array[id];
}


static uint64_t hw_clock_ns_to_cycles(struct hw_clock *clk, uint64_t ns)
{
	return (ns * clk->to_cyc.mult) >> clk->to_cyc.shift;
}

static uint64_t delta_cycles(struct hw_clock *clk, uint64_t cycles)
{
	uint64_t delta;

	if (cycles >= clk->last_cycles)
		delta = cycles - clk->last_cycles;
	else
		delta = clk->period - clk->last_cycles + cycles;

	return delta;
}

/**
 * hw_clock_read() - Read the HW counter and convert the cycles into nanoseconds
 *
 * @clk - pointer hw clock structure
 *
 * This function needs to be called more than twice than the counter overflows.
 *
 */
uint64_t hw_clock_read(struct hw_clock *clk)
{
	uint64_t cycles, delta;

	cycles = clk->read_counter(clk->priv);

	/*
	 * Protect usage of time conversions functions
	 */
	rtos_spin_lock(&rtos_global_spinlock, &rtos_global_key);

	delta = delta_cycles(clk, cycles);
	clk->nsecs += hw_clock_cycles_to_ns(clk, delta, &clk->frac);
	clk->last_cycles = cycles;

	rtos_spin_unlock(&rtos_global_spinlock, rtos_global_key);

	return clk->nsecs;
}

uint64_t hw_clock_read_isr(struct hw_clock *clk)
{
	uint64_t cycles, delta;

	cycles = clk->read_counter(clk->priv);

	/* No locking since we don't update clk->nsecs or clk->last_cycles */
	delta = delta_cycles(clk, cycles);

	return clk->nsecs + hw_clock_cycles_to_ns(clk, delta, &clk->frac);
}

/**
 * hw_clock_cycles_to_time() - Convert a raw counter value into a full 64 bits
 *                             nanoseconds time without reading the HW counter.
 * @clk - pointer hw clock structure
 * @cycles - counter cycles value to convert
 *
 * This function will work if the hw counter is read often enough (at least twice
 * often than the overflow period) and if the cycles to convert is not too old.
 */
uint64_t hw_clock_cycles_to_time(struct hw_clock *clk, uint64_t cycles)
{
	int64_t frac;
	uint64_t delta, hw_ns;
	int64_t delta_ns;

	rtos_mutex_global_lock();

	delta = delta_cycles(clk, cycles);

	/* cycles is before last_cycles */
	if (delta > ((clk->period - 1) / 2)) {
		frac = -clk->frac;
		delta_ns = -hw_clock_cycles_to_ns(clk, clk->period - delta, &frac);
	}
	/* cycles is after last_cycles */
	else {
		frac = clk->frac;
		delta_ns = hw_clock_cycles_to_ns(clk, delta, &frac);
	}

	hw_ns = clk->nsecs + delta_ns;

	rtos_mutex_global_unlock();

	return hw_ns;
}

/**
 * hw_clock_time_to_cycles() - Convert a 64 bits nanoseconds time to raw counter
 *                             value.
 * @clk - pointer to hw clock structure
 * @ns - time in nanoseconds of the hw clock
 * @cycles - pointer to cycles result variable
 *
 */
int hw_clock_time_to_cycles(struct hw_clock *clk, uint64_t ns, uint64_t *cycles)
{
	uint64_t cyc;
	uint64_t delta_cycles;

	delta_cycles = hw_clock_ns_to_cycles(clk, os_llabs(ns - clk->nsecs));

	if (clk->nsecs <= ns) {
		cyc = clk->last_cycles + delta_cycles;
	} else {
		if (delta_cycles > clk->last_cycles)
			cyc = clk->period - clk->last_cycles + delta_cycles;
		else
			cyc = clk->last_cycles - delta_cycles;
	}

	/* Not strictly right but avoids costly division
	 * If delta is over cycle period the event is too
	 * far in the future...
	 */
	if (cyc > (clk->period - 1))
		cyc -= clk->period;

	if (cyc > (clk->period - 1))
		return -1;

	*cycles = cyc;

	return 0;
}

int hw_clock_dtime_to_cycles(struct hw_clock *clk, uint64_t delta_ns, uint64_t *delta_cycles)
{
	*delta_cycles = hw_clock_ns_to_cycles(clk, delta_ns);

	return 0;
}

int hw_clock_setfreq(struct hw_clock *clk, int32_t ppb)
{
	if (clk->adj_freq)
		return clk->adj_freq(clk->priv, ppb);
	else
		return -1;
}

int hw_clock_setoffset(struct hw_clock *clk, int64_t offset)
{
	int64_t cycles_offset;
	int rc = -1;

	if (!clk->set_offset)
		goto exit;

	if (offset > 0)
		cycles_offset = hw_clock_ns_to_cycles(clk, offset);
	else
		cycles_offset = -hw_clock_ns_to_cycles(clk, -offset);

	rtos_spin_lock(&rtos_global_spinlock, &rtos_global_key);

	clk->last_cycles += cycles_offset;
	clk->nsecs += offset;

	rc = clk->set_offset(clk->priv, cycles_offset);

	rtos_spin_unlock(&rtos_global_spinlock, rtos_global_key);

exit:
	return rc;
}

int hw_clock_convert(struct hw_clock *clk_src, uint64_t ns_src, struct hw_clock *clk_dst, uint64_t *ns_dst)
{
	uint64_t cycles;

	if (!clk_src->convert)
		goto err;

	if (hw_clock_time_to_cycles(clk_src, ns_src, &cycles) < 0)
		goto err;

	if (clk_src->convert(clk_src->priv, cycles, clk_dst, &cycles) < 0)
		goto err;

	*ns_dst = hw_clock_cycles_to_time(clk_dst, cycles);

	return 0;

err:
	return -1;
}
