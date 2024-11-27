/*
* Copyright 2017, 2020, 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief RTOS specific timer service implementation
 @details
*/

#ifndef _RTOS_OSAL_TIMER_H_
#define _RTOS_OSAL_TIMER_H_

#include "osal/sys_types.h"

struct os_timer {
	int clk_id;

	void *handle;
	void *q_handle;
	unsigned int event_err;

	void (*func)(struct os_timer *t, int count);
	int (*start)(struct os_timer *t, uint64_t value, uint64_t interval_p, uint64_t interval_q, unsigned int flags);
	void (*stop)(struct os_timer *t);
	void (*destroy)(struct os_timer *t);
};

#endif /* _RTOS_OSAL_TIMER_H_ */
