
/*
* Copyright 2021, 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief Linux Standard network rtnetlink helpers
 @details
*/
#ifndef _LINUX_RTNETLINK_H_
#define _LINUX_RTNETLINK_H_

#include <sys/uio.h>
#include <linux/netlink.h>

#define NLMSG_NEXT_DATA(nmsg) \
        ((void *) (((char *) (nmsg)) + NLMSG_ALIGN((nmsg)->nlmsg_len)))

#define rtnetlink_nlmsghdr_init(nh, len, type, flags) \
	(nh)->nlmsg_len = len; \
	(nh)->nlmsg_type = type; \
	(nh)->nlmsg_flags = flags; \
	(nh)->nlmsg_seq = 0; \
	(nh)->nlmsg_pid = 0;

int rtnetlink_socket_init(void);
void rtnetlink_socket_exit(void);
int rtnetlink_socket_send_iov(struct iovec *iov, unsigned int iovlen);
int rtnetlink_attr_add(struct nlmsghdr *nh, unsigned int req_buf_size, int type, const void *data, unsigned int data_len);

#endif /* _LINUX_RTNETLINK_H_ */
