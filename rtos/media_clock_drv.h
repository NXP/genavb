/*
 * Copyright 2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVB media clock driver
*/

#ifndef _MCLOCK_DRV_H_
#define _MCLOCK_DRV_H_

#include "media_clock.h"
#include "media_clock_gen_ptp.h"
#include "mtimer.h"

struct mclock_drv {
	struct mclock_gen_ptp gen_ptp[MCLOCK_DOMAIN_PTP_RANGE];
	struct slist_head mclock_devices;
	struct mclock_timer isr[MCLOCK_TIMER_MAX];
	rtos_mutex_t mutex;
};

#endif /* _MCLOCK_DRV_H_ */
