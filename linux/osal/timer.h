/*
 * Copyright 2014 Freescale Semiconductor, Inc.
 * Copyright 2016-2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Linux specific timer service implementation
 @details
*/

#ifndef _LINUX_OSAL_TIMER_H_
#define _LINUX_OSAL_TIMER_H_

#include "osal/sys_types.h"

#include "epoll.h"

struct os_timer {
	int epoll_fd;
	int fd;
	struct linux_epoll_data epoll_data;

	void (*func)(struct os_timer *t, int count);
	void (*process)(struct os_timer *t);
	int (*start)(struct os_timer *t, u64 value, u64 interval_p, u64 interval_q, unsigned int flags);
	void (*stop)(struct os_timer *t);
	void (*destroy)(struct os_timer *t);
};


#endif /* _LINUX_OSAL_TIMER_H_ */
