/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2019-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Linux specific clock and time service implementation
 @details
*/

#ifndef _LINUX_CLOCK_H_
#define _LINUX_CLOCK_H_

#include <pthread.h>

#include "os_config.h"
#include "os/clock.h"

enum clock_type {
	CLOCK_TYPE_SYSTEM = 1,
	CLOCK_TYPE_PHC,
	CLOCK_TYPE_SW,
};

/* Clock ratio adjustments affect the hardware clock */
#define OS_CLOCK_FLAGS_HW_RATIO  (1 << 0)
/* Clock offset adjustments affect the hardware clock */
#define OS_CLOCK_FLAGS_HW_OFFSET (1 << 1)

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
	int id;
	int fd;

	/* config parameters: */
	const char *clk_device;
	bool enabled;
	unsigned int type;
	unsigned int flags; /* used for hardware clock adjustment capabilities */
	void *parent_id; /* ID of the parent hw clock, used for comparison purpose */

	/* software clock adjustment: */
	struct os_sw_clock sw_clk;
	int32_t ppb; /* current frequency adjustment configuration */
	int32_t ppb_internal;	/* adjustment between sw clock and local clock */

	int (*gettime32)(struct os_clock *c, u32 *ns);
	int (*gettime64)(struct os_clock *c, u64 *ns);
	int (*setfreq)(struct os_clock *c, s32 ppb);
	int (*setoffset)(struct os_clock *c, s64 offset);
};

int clock_time_from_hw(os_clock_id_t id, uint64_t hw_ns, uint64_t *ns);
int os_clock_gettime64_of_parent(os_clock_id_t id, u64 *ns);

int os_clock_init(struct os_clock_config *config);
void os_clock_exit(void);

#endif /* _LINUX_CLOCK_H_ */
