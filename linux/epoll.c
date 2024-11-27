/*
 * Copyright 2018, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Linux epoll implementation
 @details
*/

#include <string.h>
#include <errno.h>

#include "epoll.h"
#include "common/log.h"

int epoll_ctl_add(int epoll_fd, int fd, epoll_type_t type, void *ptr, struct linux_epoll_data *data, unsigned int event_type)
{
	struct epoll_event event;

	data->type = type;
	data->ptr = ptr;

	event.events = event_type;
	event.data.ptr = data;

	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) < 0) {
		os_log(LOG_ERR, "epoll_ctl() failed with errno(%d): %s\n", errno, strerror(errno));
		return -1;
	}

	return 0;
}

int epoll_ctl_del(int epoll_fd, int fd)
{
	if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL) < 0) {
		os_log(LOG_ERR, "epoll_ctl() failed with errno(%d): %s\n", errno, strerror(errno));
		return -1;
	}

	return 0;
}
