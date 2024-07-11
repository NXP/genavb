/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Media clock interface handling
 @details
*/
#ifndef _LINUX_OSAL_MEDIA_CLOCK_H_
#define _LINUX_OSAL_MEDIA_CLOCK_H_

#include "epoll.h"

struct os_media_clock_rec {
	int fd;
	unsigned int mmap_size;
	void *array_addr;
	unsigned int array_size;
};

struct os_media_clock_gen {
	int fd;
	volatile unsigned int *w_idx;		/**< pointer to write index */
	volatile unsigned int *count;		/**< pointer to time of last update in ns, increments in steps of 125us periods. */
	volatile unsigned int *ptp;		/**< pointer to 32-bit PTP time of last update in ns, according to ENET HW counter */

	unsigned int mmap_size;
	void *array_addr;
	unsigned int array_size;
	unsigned int ts_freq_p;
	unsigned int ts_freq_q;
	unsigned int timer_period;
};

#endif /* _LINUX_OSAL_MEDIA_CLOCK_H_ */
