/*
 * Copyright 2017-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief RTOS specific clock and time service implementation
 @details
*/

#include "common/log.h"
#include "os/clock.h"
#include "clock.h"
#include "hw_clock.h"
#include "hr_timer.h"

#include "net_port.h"
#include "net_logical_port.h"

struct os_sw_clock {

	/* software clock parameters */
	struct {
		uint64_t t0;
		uint64_t mul;
		unsigned int shift;
	} sw;

	/* hardware clock parameters */
	struct {
		uint64_t t0;
		uint64_t mul;
		unsigned int shift;
	} hw;
};

struct os_clock {
	void *hw_clk;
	unsigned int flags; /* used for hardware clock adjustments capability */

	/* software clock adjustment: */
	struct os_sw_clock sw_clk;
	int32_t ppb;	/* currently configured software frequency adjustment */
	int32_t ppb_internal;	/* adjustment between sw clock and local clock */

	int (*setfreq)(struct os_clock *c, int32_t ppb);
	int (*setoffset)(struct os_clock *c, int64_t offset);
};

static struct os_clock clock[OS_CLOCK_MAX] = {0, };

static hw_clock_id_t clk_id_to_hw_clk[OS_CLOCK_MAX] = {
	[OS_CLOCK_SYSTEM_MONOTONIC] = HW_CLOCK_MONOTONIC,
	[OS_CLOCK_SYSTEM_MONOTONIC_1] = HW_CLOCK_MONOTONIC_1,
};

static os_clock_id_t clk_id_to_clk_id[OS_CLOCK_MAX] = {
	[OS_CLOCK_SYSTEM_MONOTONIC] = OS_CLOCK_SYSTEM_MONOTONIC,
	[OS_CLOCK_SYSTEM_MONOTONIC_COARSE] = OS_CLOCK_SYSTEM_MONOTONIC_COARSE,
	[OS_CLOCK_MEDIA_HW_0] = OS_CLOCK_MEDIA_HW_0,
	[OS_CLOCK_MEDIA_HW_1] = OS_CLOCK_MEDIA_HW_1,
	[OS_CLOCK_MEDIA_REC_0] = OS_CLOCK_MEDIA_REC_0,
	[OS_CLOCK_MEDIA_REC_1] = OS_CLOCK_MEDIA_REC_1,
	[OS_CLOCK_MEDIA_PTP_0] = OS_CLOCK_MEDIA_PTP_0,
	[OS_CLOCK_MEDIA_PTP_1] = OS_CLOCK_MEDIA_PTP_1,
	[OS_CLOCK_GPTP_EP_0_0] = OS_CLOCK_GPTP_EP_0_0,
	[OS_CLOCK_GPTP_EP_0_1] = OS_CLOCK_GPTP_EP_0_1,
	[OS_CLOCK_GPTP_EP_1_0] = OS_CLOCK_GPTP_EP_1_0,
	[OS_CLOCK_GPTP_EP_1_1] = OS_CLOCK_GPTP_EP_1_1,
	[OS_CLOCK_GPTP_BR_0_0] = OS_CLOCK_GPTP_BR_0_0,
	[OS_CLOCK_GPTP_BR_0_1] = OS_CLOCK_GPTP_BR_0_1,
	[OS_CLOCK_LOCAL_EP_0] = OS_CLOCK_LOCAL_EP_0,
	[OS_CLOCK_LOCAL_EP_1] = OS_CLOCK_LOCAL_EP_1,
	[OS_CLOCK_LOCAL_BR_0] = OS_CLOCK_LOCAL_BR_0,
	[OS_CLOCK_SYSTEM_MONOTONIC_1] = OS_CLOCK_SYSTEM_MONOTONIC_1,
};

static unsigned int clk_flags[OS_CLOCK_MAX] = {
	[OS_CLOCK_GPTP_EP_0_0] = OS_CLOCK_FLAGS_HW_RATIO,
	[OS_CLOCK_GPTP_EP_0_1] = 0,
	[OS_CLOCK_GPTP_EP_1_0] = OS_CLOCK_FLAGS_HW_RATIO,
	[OS_CLOCK_GPTP_EP_1_1] = 0,
	[OS_CLOCK_LOCAL_EP_0] = 0,
	[OS_CLOCK_LOCAL_EP_1] = 0,
	[OS_CLOCK_GPTP_BR_0_0] = OS_CLOCK_FLAGS_HW_RATIO,
	[OS_CLOCK_GPTP_BR_0_1] = 0,
	[OS_CLOCK_LOCAL_BR_0] = 0,
};

os_clock_id_t clock_id(os_clock_id_t clk_id)
{
	return clk_id_to_clk_id[clk_id];
}

static struct os_clock *clock_id_to_clock(os_clock_id_t clk_id)
{
	if ((clk_id < 0) || (clk_id >= OS_CLOCK_MAX))
		goto err;

	return &clock[clock_id(clk_id)];

err:
	return NULL;
}

static bool clock_id_remapped(os_clock_id_t clk_id)
{
	return (clock_id(clk_id) != clk_id);
}

os_clock_id_t logical_port_to_local_clock(unsigned int port_id)
{
	if (!logical_port_valid(port_id))
		goto err;

	if (logical_port_is_endpoint(port_id))
		return OS_CLOCK_LOCAL_EP_0 + logical_port_endpoint_id(port_id);
	else
		return OS_CLOCK_LOCAL_BR_0 + logical_port_bridge_id(port_id);

err:
	return OS_CLOCK_MAX;
}

os_clock_id_t logical_port_to_gptp_clock(unsigned int port_id, unsigned int domain)
{
	if (!logical_port_valid(port_id) || domain >= CFG_MAX_GPTP_DOMAINS)
		goto err;

	if (logical_port_is_endpoint(port_id))
		return OS_CLOCK_GPTP_EP_0_0 + logical_port_endpoint_id(port_id) * CFG_MAX_GPTP_DOMAINS + domain;
	else
		return OS_CLOCK_GPTP_BR_0_0 + logical_port_bridge_id(port_id) * CFG_MAX_GPTP_DOMAINS + domain;

err:
	return OS_CLOCK_MAX;
}

/*
 * The timekeeping task should run at half the period of the lowest period of
 * the registered hw clocks.
 */
static rtos_thread_t tk_thread;
#define TK_STACK_DEPTH	 (RTOS_MINIMAL_STACK_SIZE)
#define TK_TASK_PRIORITY (RTOS_MAX_PRIORITY - 8)
#define TK_TASK_NAME	 "time keep"
#define TK_PERIOD_MS	 500 // lowest period is ENET clock with 1s period.

/*
 * Perform regular os_clock_gettime64() on all registered clocks in order to:
 * -update time base to avoid overflow in the time conversions functions
 * -read the low-level clock HW counters
 */
static void clock_task_timekeeping(void *pvParameters)
{
	int i;
	uint64_t now;

	while (1) {
		for (i = 0; i < OS_CLOCK_MAX; i++)
			if (clock[i].hw_clk)
				if (os_clock_gettime64(i, &now) < 0)
					os_log(LOG_ERR, "os_clock_gettime64() error\n");

		rtos_sleep(RTOS_MS_TO_TICKS(TK_PERIOD_MS));
	}
}

__init static int clock_timekeeping_start(void)
{
	if (rtos_thread_create(&tk_thread, TK_TASK_PRIORITY, 0, TK_STACK_DEPTH, TK_TASK_NAME, clock_task_timekeeping, NULL) < 0) {
		os_log(LOG_ERR, "rtos_thread_create(%s) failed\n", TK_TASK_NAME);
		return -1;
	}

	return 0;
}

__exit static void clock_timekeeping_stop(void)
{
	rtos_thread_abort(&tk_thread);
}

#define TIME_BASE_UPDATE_TRESHOLD (2000000000ULL)

static inline void clock_time_base_update(struct os_clock *c, uint64_t ns, uint64_t ns_hw)
{
	uint64_t delta_hw = ns_hw - c->sw_clk.hw.t0;

	/*
	 * The below functions clock_time_from_hw/clock_time_to_hw overflow
	 * if the delta between current time and t0 is greater than ~4 seconds.
	 * In general t0 is updated when frequency is adjusted but if not it's
	 * done here.
	 */
	if (delta_hw > TIME_BASE_UPDATE_TRESHOLD) {
		c->sw_clk.sw.t0 = ns;
		c->sw_clk.hw.t0 = ns_hw;
	}
}

static inline uint64_t __clock_time_from_hw(struct os_clock *c, uint64_t ns_hw)
{
	uint64_t ns;

	if (c->sw_clk.sw.mul) {
		if (ns_hw > c->sw_clk.hw.t0)
			ns = c->sw_clk.sw.t0 + (((ns_hw - c->sw_clk.hw.t0) * c->sw_clk.sw.mul) >> c->sw_clk.sw.shift);
		else
			ns = c->sw_clk.sw.t0 - (((c->sw_clk.hw.t0 - ns_hw) * c->sw_clk.sw.mul) >> c->sw_clk.sw.shift);
	} else {
		ns = c->sw_clk.sw.t0 + (ns_hw - c->sw_clk.hw.t0);
	}

	return ns;
}

static inline uint64_t __clock_time_to_hw(struct os_clock *c, uint64_t ns)
{
	uint64_t ns_hw;

	if (c->sw_clk.sw.mul) {
		if (ns > c->sw_clk.sw.t0)
			ns_hw = c->sw_clk.hw.t0 + (((ns - c->sw_clk.sw.t0) * c->sw_clk.hw.mul) >> c->sw_clk.hw.shift);
		else
			ns_hw = c->sw_clk.hw.t0 - (((c->sw_clk.sw.t0 - ns) * c->sw_clk.hw.mul) >> c->sw_clk.hw.shift);
	} else {
		ns_hw = c->sw_clk.hw.t0 + (ns - c->sw_clk.sw.t0);
	}

	return ns_hw;
}

static int clock_setoffset_sw(struct os_clock *c, int64_t offset)
{
	/* Protect both against tasks and IRQ's */
	rtos_spin_lock(&rtos_global_spinlock, &rtos_global_key);

	/* Track offset in software */
	c->sw_clk.sw.t0 += offset;

	rtos_spin_unlock(&rtos_global_spinlock, rtos_global_key);

	return 0;
}

static int clock_setoffset_hw(struct os_clock *c, int64_t offset)
{
	struct os_clock *_c;
	int i, rc;

	rtos_mutex_global_lock();

	c->sw_clk.sw.t0 += offset;
	c->sw_clk.hw.t0 += offset;

	rc = hw_clock_setoffset(c->hw_clk, offset);
	if (rc < 0)
		goto exit;

	for (i = 0; i < OS_CLOCK_MAX; i++) {
		_c = &clock[i];

		/* Skip ourselfs */
		if (_c == c)
			continue;

		/* Skip clocks with different hardware clock */
		if (_c->hw_clk != c->hw_clk)
			continue;

		_c->sw_clk.hw.t0 += offset;
	}

exit:
	rtos_mutex_global_unlock();

	return rc;
}

static void __clock_setfreq_sw(struct os_clock *c, int32_t ppb, uint64_t t0_hw)
{
	c->sw_clk.sw.t0 = __clock_time_from_hw(c, t0_hw);
	c->sw_clk.hw.t0 = t0_hw;

	if (ppb) {
		c->sw_clk.sw.shift = 32;
		c->sw_clk.sw.mul = ((1000000000ULL + ppb) << c->sw_clk.sw.shift) / 1000000000ULL;

		c->sw_clk.hw.shift = 32;
		c->sw_clk.hw.mul = (1000000000ULL << c->sw_clk.sw.shift) / (1000000000ULL + ppb);
	} else {
		c->sw_clk.hw.mul = 0;
		c->sw_clk.sw.mul = 0;
	}

	c->ppb = ppb;
}

static int clock_setfreq_sw(struct os_clock *c, int32_t ppb)
{
	uint64_t t0_hw;

	rtos_mutex_global_lock();

	t0_hw = hw_clock_read(c->hw_clk);

	/* Protect both against tasks and IRQ's */
	rtos_spin_lock(&rtos_global_spinlock, &rtos_global_key);

	__clock_setfreq_sw(c, ppb + c->ppb_internal, t0_hw);

	rtos_spin_unlock(&rtos_global_spinlock, rtos_global_key);
	rtos_mutex_global_unlock();

	return 0;
}

static int clock_setfreq_hw(struct os_clock *c, int32_t ppb)
{
	struct os_clock *_c;
	uint64_t t0_hw;
	int i, rc;
	int32_t delta_ppb;

	rtos_mutex_global_lock();

	rc = hw_clock_setfreq(c->hw_clk, ppb);
	if (rc < 0)
		goto exit;

	t0_hw = hw_clock_read(c->hw_clk);

	/* for all other clocks, with the same parent/hw_clk, adjust by -delta_ppb */
	delta_ppb = ppb - c->ppb;
	for (i = 0; i < OS_CLOCK_MAX; i++) {
		_c = &clock[i];

		/* Skip ourselfs */
		if (_c == c)
			continue;

		/* Skip clocks with different hardware clock */
		if (_c->hw_clk != c->hw_clk)
			continue;

		rtos_spin_lock(&rtos_global_spinlock, &rtos_global_key);

		_c->ppb_internal -= delta_ppb;
		__clock_setfreq_sw(_c, _c->ppb - delta_ppb, t0_hw);

		rtos_spin_unlock(&rtos_global_spinlock, rtos_global_key);
	}

	c->ppb = ppb;
exit:
	rtos_mutex_global_unlock();

	return rc;
}

int os_clock_gettime32(os_clock_id_t clk_id, u32 *ns)
{
	struct os_clock *c;

	c = clock_id_to_clock(clk_id);
	if (!c || !c->hw_clk)
		goto err;

	rtos_mutex_global_lock();

	*ns = __clock_time_from_hw(c, hw_clock_read(c->hw_clk));

	rtos_mutex_global_unlock();

	return 0;

err:
	return -1;
}

int os_clock_gettime64(os_clock_id_t clk_id, u64 *ns)
{
	struct os_clock *c;
	uint64_t ns_hw;

	c = clock_id_to_clock(clk_id);
	if (!c || !c->hw_clk)
		goto err;

	rtos_mutex_global_lock();

	ns_hw = hw_clock_read(c->hw_clk);
	*ns = __clock_time_from_hw(c, ns_hw);

	if (c->sw_clk.sw.mul)
		clock_time_base_update(c, *ns, ns_hw);

	rtos_mutex_global_unlock();

	return 0;

err:
	return -1;
}

int os_clock_gettime64_isr(os_clock_id_t clk_id, u64 *ns)
{
	struct os_clock *c;
	uint64_t ns_hw;

	c = clock_id_to_clock(clk_id);
	if (!c || !c->hw_clk)
		goto err;

	ns_hw = hw_clock_read_isr(c->hw_clk);
	*ns = __clock_time_from_hw(c, ns_hw);

	return 0;

err:
	return -1;
}

int os_clock_convert(os_clock_id_t clk_id_src, uint64_t ns_src, os_clock_id_t clk_id_dst, uint64_t *ns_dst)
{
	struct os_clock *c_src;
	struct os_clock *c_dst;
	uint64_t ns_hw_clk;

	c_src = clock_id_to_clock(clk_id_src);
	if (!c_src || !c_src->hw_clk) {
		os_log(LOG_ERR, " Unsupported source clk_id %d\n", clk_id_src);
		goto err;
	}

	c_dst = clock_id_to_clock(clk_id_dst);
	if (!c_dst || !c_dst->hw_clk) {
		os_log(LOG_ERR, " Unsupported destination clk_id %d\n", clk_id_dst);
		goto err;
	}

	if (c_src->hw_clk == c_dst->hw_clk) {

		rtos_mutex_global_lock();

		ns_hw_clk = __clock_time_to_hw(c_src, ns_src);
		*ns_dst = __clock_time_from_hw(c_dst, ns_hw_clk);

		rtos_mutex_global_unlock();
	} else {
		rtos_mutex_global_lock();

		ns_hw_clk = __clock_time_to_hw(c_src, ns_src);

		if (hw_clock_convert(c_src->hw_clk, ns_hw_clk, c_dst->hw_clk, &ns_hw_clk) < 0) {
			rtos_mutex_global_unlock();
			os_log(LOG_ERR, " Incompatible hardware clocks source %p destination %p\n", c_src->hw_clk, c_dst->hw_clk);
			goto err;
		}

		*ns_dst = __clock_time_from_hw(c_dst, ns_hw_clk);

		rtos_mutex_global_unlock();
	}

	return 0;

err:
	*ns_dst = 0;
	return -1;
}

int os_clock_setfreq(os_clock_id_t clk_id, s32 ppb)
{
	struct os_clock *c;

	c = clock_id_to_clock(clk_id);
	if (!c || !c->hw_clk)
		goto err;

	if (c->setfreq)
		return c->setfreq(c, ppb);

err:
	return -1;
}

int os_clock_setoffset(os_clock_id_t clk_id, s64 offset)
{
	struct os_clock *c;
	int rc = -1;

	c = clock_id_to_clock(clk_id);
	if (!c || !c->hw_clk)
		goto exit;

	if (c->setoffset) {
		rc = c->setoffset(c, offset);

		if (!rc) {
			qos_clock_discontinuity(clk_id);
			hr_timer_clock_discontinuity(clk_id);
		}
	}

exit:
	return rc;
}

unsigned int os_clock_adjust_mode(os_clock_id_t clk_id)
{
	return 0;
}

int clock_time_from_hw(os_clock_id_t clk_id, uint64_t hw_ns, uint64_t *ns)
{
	struct os_clock *c;

	c = clock_id_to_clock(clk_id);
	if (!c || !c->hw_clk)
		goto err;

	rtos_mutex_global_lock();

	*ns = __clock_time_from_hw(c, hw_ns);

	rtos_mutex_global_unlock();

	return 0;

err:
	return -1;
}

static inline uint64_t __clock_dtime_to_hw(struct os_clock *c, uint64_t delta_ns)
{
	uint64_t delta_ns_hw;

	if (c->sw_clk.sw.mul) {
		delta_ns_hw = (delta_ns * c->sw_clk.hw.mul) >> c->sw_clk.hw.shift;
	} else {
		delta_ns_hw = delta_ns;
	}

	return delta_ns_hw;
}

int __clock_time_to_cycles(os_clock_id_t clk_id, uint64_t ns, uint64_t *cycles)
{
	struct os_clock *c;

	c = clock_id_to_clock(clk_id);
	if (!c || !c->hw_clk)
		goto err;

	return hw_clock_time_to_cycles(c->hw_clk, __clock_time_to_hw(c, ns), cycles);

err:
	return -1;
}

int clock_time_to_cycles(os_clock_id_t clk_id, uint64_t ns, uint64_t *cycles)
{
	int rc;

	rtos_mutex_global_lock();

	rc =  __clock_time_to_cycles(clk_id, ns, cycles);

	rtos_mutex_global_unlock();

	return rc;
}

int __clock_dtime_to_cycles(os_clock_id_t clk_id, uint64_t delta_ns, uint64_t *delta_cycles)
{
	struct os_clock *c;

	c = clock_id_to_clock(clk_id);
	if (!c || !c->hw_clk)
		goto err;

	return hw_clock_dtime_to_cycles(c->hw_clk, __clock_dtime_to_hw(c, delta_ns), delta_cycles);

err:
	return -1;
}

int clock_id_remap(os_clock_id_t clk_id, os_clock_id_t dst_clk_id)
{
	if (clk_id < OS_CLOCK_GPTP_EP_0_0 || clk_id > OS_CLOCK_LOCAL_BR_0)
		goto err;

	if (dst_clk_id < OS_CLOCK_GPTP_EP_0_0 || dst_clk_id > OS_CLOCK_LOCAL_BR_0)
		goto err;

	if (clock_id_remapped(dst_clk_id))
		goto err;

	clk_id_to_clk_id[clk_id] = dst_clk_id;

	return 0;

err:
	return -1;
}

int clock_to_hw_clock_set(os_clock_id_t clk_id, hw_clock_id_t hw_clk_id)
{
	if ((clk_id < 0) || (clk_id >= OS_CLOCK_MAX))
		return -1;

	if ((hw_clk_id < HW_CLOCK_PORT_BASE) || (hw_clk_id >= HW_CLOCK_MAX))
		return -1;

	clk_id_to_hw_clk[clk_id] = hw_clk_id;

	return 0;
}

int clock_set_flags(os_clock_id_t clk_id, unsigned int flags)
{
	if ((clk_id < 0) || (clk_id >= OS_CLOCK_MAX))
		return -1;

	clk_flags[clk_id] = flags & (OS_CLOCK_FLAGS_HW_RATIO | OS_CLOCK_FLAGS_HW_OFFSET);

	return 0;
}

hw_clock_id_t clock_to_hw_clock(os_clock_id_t clk_id)
{
	if ((clk_id < 0) || (clk_id >= OS_CLOCK_MAX))
		return HW_CLOCK_NONE;

	return clk_id_to_hw_clk[clock_id(clk_id)];
}

bool clock_has_hw_adjust_ratio(os_clock_id_t clk_id)
{
	struct os_clock *c;

	c = clock_id_to_clock(clk_id);
	if (!c) {
		os_log(LOG_ERR, "clock ID: %d invalid\n", clk_id);
		goto err;
	}

	return !!(c->flags & OS_CLOCK_FLAGS_HW_RATIO);

err:
	return false;
}

bool clock_has_hw_setoffset(os_clock_id_t clk_id)
{
	struct os_clock *c;

	c = clock_id_to_clock(clk_id);
	if (!c) {
		os_log(LOG_ERR, "clock ID: %d invalid\n", clk_id);
		goto err;
	}

	return !!(c->flags & OS_CLOCK_FLAGS_HW_OFFSET);

err:
	return false;
}

__init static int _os_clock_init(os_clock_id_t clk_id, int flags)
{
	struct os_clock *c;

	if (clock_id_remapped(clk_id)) {
		os_log(LOG_INIT, "clock ID: %d remapped to %d\n", clk_id, clk_id_to_clk_id[clk_id]);
		goto exit;
	}

	c = clock_id_to_clock(clk_id);
	if (!c) {
		os_log(LOG_ERR, "clock ID: %d invalid\n", clk_id);
		goto err;
	}

	c->hw_clk = hw_clock_get(clk_id_to_hw_clk[clk_id]);
	if (!c->hw_clk) {
		os_log(LOG_ERR, "clock ID: %d has no hw clock\n", clk_id);
		goto err;
	}

	switch (clk_id) {
	case OS_CLOCK_GPTP_EP_0_0:
	case OS_CLOCK_GPTP_EP_0_1:
	case OS_CLOCK_GPTP_EP_1_0:
	case OS_CLOCK_GPTP_EP_1_1:
	case OS_CLOCK_GPTP_BR_0_0:
	case OS_CLOCK_GPTP_BR_0_1:
		if (flags & OS_CLOCK_FLAGS_HW_RATIO)
			c->setfreq = clock_setfreq_hw;
		else
			c->setfreq = clock_setfreq_sw;

		if (flags & OS_CLOCK_FLAGS_HW_OFFSET)
			c->setoffset = clock_setoffset_hw;
		else
			c->setoffset = clock_setoffset_sw;

		break;

	case OS_CLOCK_LOCAL_EP_0:
	case OS_CLOCK_LOCAL_EP_1:
	case OS_CLOCK_LOCAL_BR_0:
	default:
		break;
	}

	c->flags = flags;

	os_log(LOG_INIT, "clock ID: %d success, flags: %x\n", clk_id, c->flags);

exit:
	return 0;

err:
	return -1;
}

__exit static void _os_clock_exit(os_clock_id_t clk_id)
{
	struct os_clock *c;

	if (clock_id_remapped(clk_id))
		goto exit;

	c = clock_id_to_clock(clk_id);
	if (!c) {
		os_log(LOG_ERR, "clock ID: %d invalid\n", clk_id);
		goto exit;
	}

	c->hw_clk = NULL;

exit:
	return;
}

__init int os_clock_init(void)
{
	int i;

	for (i = 0; i < OS_CLOCK_MAX; i++)
		_os_clock_init(i, clk_flags[i]);

	if (clock_timekeeping_start() < 0)
		return -1;

	return 0;
}

__exit void os_clock_exit(void)
{
	int i;

	clock_timekeeping_stop();

	for (i = 0; i < OS_CLOCK_MAX; i++)
		_os_clock_exit(i);
}
