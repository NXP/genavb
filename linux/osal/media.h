/*
 * Copyright 2016, 2018, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Linux specific media abstraction implementation
 @details
*/

#ifndef _LINUX_OSAL_MEDIA_H_
#define _LINUX_OSAL_MEDIA_H_

#include "epoll.h"

struct media_rx {
	int fd;
	int epoll_fd;
	struct linux_epoll_data epoll_data;
};

struct media_tx {
	int fd;
};

#endif /* _LINUX_OSAL_MEDIA_H_ */
