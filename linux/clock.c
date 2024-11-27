/*
 * Copyright 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Linux specific clock and time service implementation
 @details
*/

#define _GNU_SOURCE

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "common/log.h"
#include "os/clock.h"

#include "clock.h"
#include "net_logical_port.h"

#ifndef ADJ_SETOFFSET
#define ADJ_SETOFFSET 0x0100
#endif

#define CLOCKFD 3
#define FD_TO_CLOCKID(fd)	((~(clockid_t) (fd) << 3) | CLOCKFD)
#define CLOCKID_TO_FD(clk)	((unsigned int) ~((clk) >> 3))

#define OS_CLOCK_SHARE_SAME_PARENT(c1, c2) \
			((c1)->parent_id == (c2)->parent_id)

#define for_each_sw_clock_with_same_parent(clock, sibling) \
	for (int __i = 0; __i < OS_CLOCK_MAX; __i++) \
		/* Skip ourself, non-software clocks and sw clock with different parent */ \
		if ((((sibling) = &os_clock[__i]) != (clock)) && \
			((sibling)->type == CLOCK_TYPE_SW) && \
			((sibling)->parent_id == (clock)->parent_id))

static struct os_clock os_clock[OS_CLOCK_MAX] = {
	[OS_CLOCK_SYSTEM_MONOTONIC] = {
		.type = CLOCK_TYPE_SYSTEM,
		.id = CLOCK_MONOTONIC_RAW,
		.enabled = true,
	},

	/*
	 * All other (phc) clocks configured at init time:
	 * see os_clock_config_init()
	 */
};

/*
 * Mutex lock to protect concurrent access to entire clock layer
 *
 * Offset/frequency computation must be done atomically to avoid bad (old) values
 * when calling into ->gettime_sw(), ->setfreq(), ->setoffset() and so on...
 */
static pthread_mutex_t os_clock_mutex;

static int clock_gettime64_hw(struct os_clock *c, u64 *ns);

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

/*
 * Note:
 * - os_clock_mutex must be held before entering this function
 */
static inline uint64_t __clock_time_from_hw(struct os_clock *c, uint64_t ns_hw)
{
	uint64_t ns = ns_hw;

	if (c->type != CLOCK_TYPE_SW)
		goto pass_through;

	if (c->sw_clk.sw.mul) {
		if (ns_hw > c->sw_clk.hw.t0)
			ns = c->sw_clk.sw.t0 + (((ns_hw - c->sw_clk.hw.t0) * c->sw_clk.sw.mul) >> c->sw_clk.sw.shift);
		else
			ns = c->sw_clk.sw.t0 - (((c->sw_clk.hw.t0 - ns_hw) * c->sw_clk.sw.mul) >> c->sw_clk.sw.shift);
	} else {
		ns = c->sw_clk.sw.t0 + (ns_hw - c->sw_clk.hw.t0);
	}

pass_through:
	return ns;
}

/*
 * Note:
 * - os_clock_mutex must be held before entering this function
 */
static uint64_t __clock_time_to_hw(struct os_clock *c, uint64_t ns)
{
	uint64_t ns_hw = ns;

	if (c->type != CLOCK_TYPE_SW)
		goto pass_through;

	if (c->sw_clk.sw.mul) {
		if (ns > c->sw_clk.sw.t0)
			ns_hw = c->sw_clk.hw.t0 + (((ns - c->sw_clk.sw.t0) * c->sw_clk.hw.mul) >> c->sw_clk.hw.shift);
		else
			ns_hw = c->sw_clk.hw.t0 - (((c->sw_clk.sw.t0 - ns) * c->sw_clk.hw.mul) >> c->sw_clk.hw.shift);
	} else {
		ns_hw = c->sw_clk.hw.t0 + (ns - c->sw_clk.sw.t0);
	}

pass_through:
	return ns_hw;
}

static int clock_gettime64_sw(struct os_clock *c, u64 *ns)
{
	int err;
	struct timespec now;
	uint64_t ns_hw;

	pthread_mutex_lock(&os_clock_mutex);

	err = clock_gettime(c->id, &now);
	if (err) {
		os_log(LOG_ERR, "clock(%p) clock_gettime failed: %s\n", c, strerror(errno));
		goto unlock;
	}

	ns_hw = (u64)now.tv_sec*NSECS_PER_SEC + now.tv_nsec;
	*ns = __clock_time_from_hw(c, ns_hw);

	if (c->sw_clk.sw.mul)
		clock_time_base_update(c, *ns, ns_hw);

unlock:
	pthread_mutex_unlock(&os_clock_mutex);

	return err;
}

static int clock_gettime32_sw(struct os_clock *c, u32 *ns)
{
	u64 val;
	int err;

	err = clock_gettime64_sw(c, &val);
	if (!err)
		*ns = (u32)val;

	return err;
}

int clock_setoffset_sw(struct os_clock *c, s64 offset)
{
	pthread_mutex_lock(&os_clock_mutex);

	c->sw_clk.sw.t0 += offset;

	pthread_mutex_unlock(&os_clock_mutex);

	return 0;
}

/*
 * Note:
 * - os_clock_mutex must be held before entering this function
 */
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
	int ret = 0;

	pthread_mutex_lock(&os_clock_mutex);

	ret = clock_gettime64_hw(c, &t0_hw);
	if (ret)
		goto unlock;

	__clock_setfreq_sw(c, ppb + c->ppb_internal, t0_hw);

unlock:
	pthread_mutex_unlock(&os_clock_mutex);

	return ret;
}

static int clock_gettime32_hw(struct os_clock *c, u32 *ns)
{
	int err = 0;
	struct timespec now;

	err = clock_gettime(c->id, &now);
	if (err) {
		os_log(LOG_ERR, "clock(%p) clock_gettime failed: %s\n", c, strerror(errno));
		return err;
	}

	*ns = (u64)now.tv_sec*NSECS_PER_SEC + now.tv_nsec;

	return 0;
}

static int clock_gettime64_hw(struct os_clock *c, u64 *ns)
{
	int err = 0;
	struct timespec now;

	err = clock_gettime(c->id, &now);
	if (err) {
		os_log(LOG_ERR, "clock(%p) clock_gettime failed: %s\n", c, strerror(errno));
		return err;
	}

	*ns = (u64)now.tv_sec*NSECS_PER_SEC + now.tv_nsec;

	return 0;
}

static long ppb_to_scaled_ppm(int ppb)
{
	/*
	* The 'freq' field in the 'struct timex' is in parts per
	* million, but with a 16 bit binary fractional field.
	* Instead of calculating either one of
	*
	* scaled_ppm = (ppb / 1000) << 16 [1]
	* scaled_ppm = (ppb << 16) / 1000 [2]
	*
	* we simply use double precision math, in order to avoid the
	* truncation in [1] and the possible overflow in [2].
	*/
	return (long) (ppb * 65.536);
}

static int clock_setfreq_hw(struct os_clock *c, s32 ppb)
{
	struct timex t;
	struct os_clock *_c;
	uint64_t t0_hw;
	int ret = -1;
	s32 delta_ppb;

	memset(&t, 0, sizeof(t));

	t.modes = ADJ_FREQUENCY;
	t.freq = ppb_to_scaled_ppm(ppb);

	pthread_mutex_lock(&os_clock_mutex);

	if (clock_adjtime(c->id, &t) < 0) {
		os_log(LOG_ERR, "clock_id(0x%x) failed adjusting frequency\n", c->id);
		goto unlock;
	}

	os_log(LOG_DEBUG, "clock_id(0x%x) adjusted frequency by %d ppb\n", c->id, ppb);

	/* for all other clocks, with the same parent/clock device, adjust by -delta_ppb */
	delta_ppb = ppb - c->ppb;

	ret = clock_gettime64_hw(c, &t0_hw);
	if (ret) {
		os_log(LOG_ERR, "clock_id(0x%x) failed to get time\n", c->id);
		ret = -1;
		goto unlock;
	}

	for_each_sw_clock_with_same_parent(c, _c) {

		_c->ppb_internal -= delta_ppb;
		__clock_setfreq_sw(_c, _c->ppb - delta_ppb, t0_hw);

		os_log(LOG_DEBUG, "clock_id(0x%x) adjusted sw frequency by %d ppb\n",
				 _c->id, _c->ppb - delta_ppb);
	}

	c->ppb = ppb;
unlock:
	pthread_mutex_unlock(&os_clock_mutex);

	return ret;
}

static int clock_setoffset_hw(struct os_clock *c, s64 offset)
{
	struct timex t;
	struct os_clock *_c;
	int err = 0;

	memset(&t, 0, sizeof(t));

	t.modes = ADJ_SETOFFSET | ADJ_NANO;
	t.time.tv_sec = offset / (s64)NSECS_PER_SEC;
	t.time.tv_usec = offset % (s64)NSECS_PER_SEC;

	if (offset < 0) {
		t.time.tv_sec -= 1;
		t.time.tv_usec += (s64)NSECS_PER_SEC;
	}

	pthread_mutex_lock(&os_clock_mutex);

	if (clock_adjtime(c->id, &t) < 0) {
		os_log(LOG_ERR, "clock_id(0x%x) failed adjusting offset\n", c->id);
		err = -1;
		goto unlock;
	}

	os_log(LOG_DEBUG, "clock_id(0x%x) offset clock by %"PRId64" ns\n", c->id, offset);

	/* for all other clocks, with the same parent/clock device, adjust by -offset */

	for_each_sw_clock_with_same_parent(c, _c) {

		_c->sw_clk.hw.t0 += offset;

		os_log(LOG_DEBUG, "clock_id(0x%x) adjusted hw.t0 offset by %"PRId64" ns\n",
				 _c->id, offset);
	}

unlock:
	pthread_mutex_unlock(&os_clock_mutex);

	return err;
}

static struct os_clock *clock_id_to_clock(os_clock_id_t clk_id)
{
	if (clk_id >= OS_CLOCK_MAX)
		goto err;

	if (!os_clock[clk_id].enabled)
		goto err;

	return &os_clock[clk_id];

err:
	return NULL;
}

static int _os_clock_init(os_clock_id_t clk_id)
{
	struct os_clock *c;

	c = clock_id_to_clock(clk_id);
	if (!c) {
		os_log(LOG_ERR, "clock ID invalid: %d\n", clk_id);
		goto err;
	}

	switch (c->type) {
	case CLOCK_TYPE_SYSTEM:
		c->gettime32 = clock_gettime32_hw;
		c->gettime64 = clock_gettime64_hw;
		c->setfreq = clock_setfreq_hw;
		c->setoffset = clock_setoffset_hw;
		c->id = CLOCK_MONOTONIC_RAW;
		c->flags = OS_CLOCK_FLAGS_HW_OFFSET;
		break;

	case CLOCK_TYPE_PHC:
		c->gettime32 = clock_gettime32_hw;
		c->gettime64 = clock_gettime64_hw;
		c->setfreq = clock_setfreq_hw;
		c->setoffset = clock_setoffset_hw;
		c->flags = OS_CLOCK_FLAGS_HW_OFFSET;
		break;

	case CLOCK_TYPE_SW:
		c->gettime32 = clock_gettime32_sw;
		c->gettime64 = clock_gettime64_sw;
		c->setfreq = clock_setfreq_sw;
		c->setoffset = clock_setoffset_sw;
		c->flags = 0;
		break;

	default:
		os_log(LOG_ERR, "clock id: %d not supported\n", clk_id);
		goto err;
		break;
	}

	if (c->clk_device) {
		c->fd = open(c->clk_device, O_RDWR);
		if (c->fd < 0) {
			os_log(LOG_ERR, "clock(%p) couldn't open clock char device: %s error: %s\n",
								c, c->clk_device, strerror(errno));

			c->enabled = false;

			goto err;
		}
		c->id = FD_TO_CLOCKID(c->fd);

		os_log(LOG_INIT, "clock(%p)%s id: %d success, linux clockid: 0x%x fd: 0x%x device: %s parent: %p\n",
							c, (c->type == CLOCK_TYPE_SW) ? "(sw)" : "",
							clk_id, c->id, c->fd, c->clk_device, c->parent_id);
	} else {
		os_log(LOG_INIT, "clock(%p) id: %d success, linux clockid: 0x%x\n",
							c, clk_id, c->id);
	}

	return 0;

err:
	return -1;
}

static int _os_clock_exit(os_clock_id_t clk_id)
{
	struct os_clock *c;

	c = clock_id_to_clock(clk_id);
	if (!c)
		goto err;

	if (c->fd >= 0)
		close(c->fd);

	return 0;

err:
	return -1;
}

static void os_clock_get_config(struct os_clock *c, const char *clk_device, bool force_sw)
{
	if (!strlen(clk_device) || !strcmp(clk_device, "off")) {
		c->enabled = false;
	} else if (!strncmp(clk_device, "sw_clock", sizeof("sw_clock"))) {
		c->type = CLOCK_TYPE_SW;
		c->enabled = true;
	} else {
		c->type = force_sw ? CLOCK_TYPE_SW : CLOCK_TYPE_PHC;
		c->clk_device = clk_device;
		c->enabled = true;
	}
}

/*
 * PHC & software clocks configuration
 *
 * Set clocks' config parameters based on the config file
 * - force the local clock to be a software clock
 * - use the local clock name to define the hardware clock for the whole domain
 * - use that hw clock device as a parent ID for all software clocks
 */
static void os_clock_config_init(struct os_clock_config *config)
{
	struct os_clock *hw_clock, *local, *c;
	int i, j;

	for (i = 0; i < CFG_MAX_ENDPOINTS; i++) {
		local = &os_clock[OS_CLOCK_LOCAL_EP_0 + i];
		hw_clock = local;
		os_clock_get_config(local, config->endpoint_local[i], true);
		local->parent_id = hw_clock;

		for (j = 0; j < CFG_MAX_GPTP_DOMAINS; j++) {
			c = &os_clock[OS_CLOCK_GPTP_EP_0_0 + i * CFG_MAX_GPTP_DOMAINS + j];

			os_clock_get_config(c, config->endpoint_gptp[j][i], false);

			if (c->type == CLOCK_TYPE_SW)
				c->clk_device = hw_clock->clk_device;

			c->parent_id = hw_clock;
		}
	}

	for (i = 0; i < CFG_MAX_BRIDGES; i++) {
		local = &os_clock[OS_CLOCK_LOCAL_BR_0 + i];
		hw_clock = local;
		os_clock_get_config(local, config->bridge_local[i], true);
		local->parent_id = hw_clock;

		for (j = 0; j < CFG_MAX_GPTP_DOMAINS; j++) {
			c = &os_clock[OS_CLOCK_GPTP_BR_0_0 + i * CFG_MAX_GPTP_DOMAINS + j];

			os_clock_get_config(c, config->bridge_gptp[j][i], false);

			if (c->type == CLOCK_TYPE_SW)
				c->clk_device = hw_clock->clk_device;

			c->parent_id = hw_clock;
		}
	}
}

int os_clock_init(struct os_clock_config *config)
{
	int i;

	pthread_mutex_init(&os_clock_mutex, NULL);

	os_clock_config_init(config);

	for (i = 0; i < OS_CLOCK_MAX; i++)
		_os_clock_init(i);

	return 0;
}

void os_clock_exit(void)
{
	int i;

	for (i = 0; i < OS_CLOCK_MAX; i++)
		_os_clock_exit(i);

	pthread_mutex_destroy(&os_clock_mutex);
}

int os_clock_gettime32(os_clock_id_t clk_id, u32 *ns)
{
	struct os_clock *c;

	c = clock_id_to_clock(clk_id);
	if (!c)
		goto err;

	if (c->gettime32)
		return c->gettime32(c, ns);

err:
	return -1;
}

int os_clock_gettime64(os_clock_id_t clk_id, u64 *ns)
{
	struct os_clock *c;

	c = clock_id_to_clock(clk_id);
	if (!c)
		goto err;

	if (c->gettime64)
		return c->gettime64(c, ns);

err:
	return -1;
}

/**
 * Get time in nanoseconds of the parent (hardware) clock.
 * \param id	clock id.
 * \param ns	pointer to u64 variable that will hold the result.
 * \return	0 on success, or negative value on error.
 */
int os_clock_gettime64_of_parent(os_clock_id_t clk_id, u64 *ns)
{
	struct timespec now;
	struct os_clock *c;
	struct os_clock *parent;
	int err = 0;

	c = clock_id_to_clock(clk_id);
	if (!c)
		goto err;

	parent = c->parent_id;
	if (!parent)
		goto err;

	err = clock_gettime(parent->id, &now);
	if (err) {
		os_log(LOG_ERR, "clock(%p) clock_gettime failed: %s\n", c, strerror(errno));
		goto err;
	}

	*ns = (u64)now.tv_sec*NSECS_PER_SEC + now.tv_nsec;

	return 0;
err:
	return -1;
}

int os_clock_convert(os_clock_id_t clk_id_src, u64 ns_src, os_clock_id_t clk_id_dst, u64 *ns_dst)
{
	struct os_clock *c_src;
	struct os_clock *c_dst;
	uint64_t ns_hw_clk;

	c_src = clock_id_to_clock(clk_id_src);
	if (!c_src) {
		os_log(LOG_ERR, " Unsupported source clk_id %d\n", clk_id_src);
		goto err;
	}

	c_dst = clock_id_to_clock(clk_id_dst);
	if (!c_dst) {
		os_log(LOG_ERR, " Unsupported destination clk_id %d\n", clk_id_dst);
		goto err;
	}

	if (!OS_CLOCK_SHARE_SAME_PARENT(c_src, c_dst)) {
		os_log(LOG_ERR, " Incompatible hardware clocks source(%p) destination(%p)\n", c_src, c_dst);
		goto err;
	}

	pthread_mutex_lock(&os_clock_mutex);

	ns_hw_clk = __clock_time_to_hw(c_src, ns_src);
	*ns_dst = __clock_time_from_hw(c_dst, ns_hw_clk);

	pthread_mutex_unlock(&os_clock_mutex);

	return 0;

err:
	*ns_dst = 0;
	return -1;
}

int os_clock_setfreq(os_clock_id_t clk_id, s32 ppb)
{
	struct os_clock *c;

	os_log(LOG_DEBUG, "clock %d ppb %d\n", clk_id, ppb);

	c = clock_id_to_clock(clk_id);
	if (!c)
		goto err;

	if (c->setfreq)
		return c->setfreq(c, ppb);

err:
	return -1;
}

int os_clock_setoffset(os_clock_id_t clk_id, s64 offset)
{
	struct os_clock *c;

	os_log(LOG_DEBUG, "clock %d offset %"PRId64"\n", clk_id, offset);

	c = clock_id_to_clock(clk_id);
	if (!c)
		goto err;

	if (c->setoffset)
		return c->setoffset(c, offset);

err:
	return -1;
}

unsigned int os_clock_adjust_mode(os_clock_id_t clk_id)
{
	struct os_clock *c;
	unsigned int mode = 0;

	c = clock_id_to_clock(clk_id);
	if (!c)
		goto exit;

	if (c->flags & OS_CLOCK_FLAGS_HW_OFFSET)
		mode |= OS_CLOCK_ADJUST_MODE_HW_OFFSET;

exit:
	return mode;
}

/**
 * Convert hw clock time to sw clock time.
 * The time must come from the root hw clock of the sw clock
 * given as argument.
 * \param id		clock id.
 * \param hw_ns		hw time to convert
 * \param ns		pointer to u64 variable that will hold the result.
 * \return		0 on success, or negative value on error.
 */
int clock_time_from_hw(os_clock_id_t clk_id, uint64_t hw_ns, uint64_t *ns)
{
	struct os_clock *c;

	c = clock_id_to_clock(clk_id);
	if (!c || !c->parent_id)
		goto err;

	pthread_mutex_lock(&os_clock_mutex);

	*ns = __clock_time_from_hw(c, hw_ns);

	pthread_mutex_unlock(&os_clock_mutex);

	return 0;

err:
	return -1;
}
