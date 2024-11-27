/*
* Copyright 2021, 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>

#include "rtnetlink.h"

#include "common/log.h"

static int fd_netlink = -1;

int rtnetlink_attr_add(struct nlmsghdr *nh, unsigned int req_buf_size, int type, const void *data, unsigned int data_len)
{
	struct rtattr *rta;
	int rc = 0;
	unsigned int attr_len = RTA_LENGTH(data_len);

	/* Check if request buffer has enough space for the attribute */
	if (NLMSG_ALIGN(nh->nlmsg_len) + RTA_ALIGN(attr_len) > req_buf_size) {
		 os_log(LOG_ERR, "unsufficient request buffer size %u for attribute payload len %u\n", req_buf_size, attr_len);
		 rc = -1;
		 goto exit;
	}

	rta = (struct rtattr *) NLMSG_NEXT_DATA(nh);
	rta->rta_type = type;
	rta->rta_len = attr_len;
	nh->nlmsg_len = NLMSG_ALIGN(nh->nlmsg_len) + RTA_ALIGN(attr_len);

	if (data && data_len)
		memcpy(RTA_DATA(rta), data, data_len);

exit:
	return rc;
}

int rtnetlink_socket_send_iov(struct iovec *iov, unsigned int iovlen)
{
	struct sockaddr_nl sa;
	struct msghdr msg;
	int rc = 0;

	sa.nl_family = AF_NETLINK;
	sa.nl_pad = 0;
	sa.nl_pid = 0;
	sa.nl_groups = 0;

	msg.msg_name = &sa;
	msg.msg_namelen = sizeof(sa);
	msg.msg_iov = iov;
	msg.msg_iovlen = iovlen;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_flags = 0;

	if (sendmsg(fd_netlink, &msg, 0) < 0) {
		os_log(LOG_ERR, "sendmsg() failed: %s\n", strerror(errno));
		rc = -1;
		goto exit;
	}
exit:
	return rc;

}

int rtnetlink_socket_init(void)
{
	struct sockaddr_nl sa;

	fd_netlink = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (fd_netlink < 0) {
		os_log(LOG_ERR, "socket() failed: %s\n", strerror(errno));
		goto err_socket;
	}

	sa.nl_family = AF_NETLINK;
	sa.nl_pad = 0;
	sa.nl_pid = 0;
	sa.nl_groups = 0;

	if (bind(fd_netlink, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
		os_log(LOG_ERR, "bind() failed: %s\n", strerror(errno));
		goto err_bind;
	}

	return 0;

err_bind:
	close(fd_netlink);
	fd_netlink = -1;

err_socket:
	return -1;
}

void rtnetlink_socket_exit(void)
{
	close(fd_netlink);
	fd_netlink = -1;
}
