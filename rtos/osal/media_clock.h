/*
 * Copyright 2018-2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief
 @details
*/

#ifndef _RTOS_OSAL_MEDIA_CLOCK_H
#define _RTOS_OSAL_MEDIA_CLOCK_H

struct os_media_clock_rec {
	unsigned long priv;
	void *array_addr;
	unsigned int array_size;
	struct mclock_dev *mclock_dev;
};

struct os_media_clock_gen {
	struct mclock_dev *mclock_dev;
	volatile unsigned int *w_idx;		/**< pointer to write index */
	volatile unsigned int *count;		/**< pointer to time of last update in ns, increments in steps of 125us periods. */
	volatile unsigned int *ptp;		/**< pointer to 32-bit PTP time of last update in ns, according to ENET HW counter */

	unsigned long priv;
	void *array_addr;
	unsigned int array_size;
	unsigned int ts_freq_p;
	unsigned int ts_freq_q;
	unsigned int timer_period;
};

#endif /* _RTOS_OSAL_MEDIA_CLOCK_H */
