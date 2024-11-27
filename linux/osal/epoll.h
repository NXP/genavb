/*
 * Copyright 2018-2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Linux specific epoll data stuctures
 @details
*/

#ifndef _LINUX_OSAL_EPOLL_H_
#define _LINUX_OSAL_EPOLL_H_

typedef enum {
	EPOLL_TYPE_TIMER,
	EPOLL_TYPE_DESC,
	EPOLL_TYPE_NET_RX,
	EPOLL_TYPE_IPC,
	EPOLL_TYPE_MEDIA,
	EPOLL_TYPE_NET_TX_TS,
	EPOLL_TYPE_NET_TX_EVENT,
} epoll_type_t;


struct linux_epoll_data {
	epoll_type_t type;
	void *ptr;
};

#endif /* _LINUX_OSAL_EPOLL_H_ */
