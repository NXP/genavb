/*
 * Copyright 2018, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Linux specific epoll implementation
 @details
*/

#ifndef _LINUX_EPOLL_H_
#define _LINUX_EPOLL_H_

#include "osal/epoll.h"
#include "sys/epoll.h"


int epoll_ctl_add(int epoll_fd, int fd, epoll_type_t type, void *ptr, struct linux_epoll_data *data, unsigned int event_type);
int epoll_ctl_del(int epoll_fd, int fd);

#endif /* _LINUX_EPOLL_H_ */
