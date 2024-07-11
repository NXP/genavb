/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Linux specific IPC service implementation
 @details
*/

#ifndef _LINUX_OSAL_IPC_H_
#define _LINUX_OSAL_IPC_H_

#include "epoll.h"

#define DEFAULT_IPC_DATA_SIZE	1024

struct ipc_rx {
	int fd;
	void *mmap_baseaddr;
	unsigned long pool_size;
	int epoll_fd;
	void (*func)(struct ipc_rx const *, struct ipc_desc *);
	struct linux_epoll_data epoll_data;
};

struct ipc_tx {
	int fd;
	void *mmap_baseaddr;
	unsigned long pool_size;
};

#endif /* _LINUX_OSAL_IPC_H_ */
