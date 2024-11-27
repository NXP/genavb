/*
* Copyright 2021, 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief Linux specific FDB service implementation
 @details
*/

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/if_bridge.h>
#include <sys/socket.h>
#include <net/if.h>
#include "rtnetlink.h"

#include "os/fdb.h"

#include "common/log.h"

#include "net_logical_port.h"
#include "fdb.h"

#define br_port_msg_mdb_init(bpm, ifidx) \
	(bpm)->family = PF_BRIDGE; \
	(bpm)->ifindex = ifidx;

static int std_bridge_rtnetlink(u8 *mac_addr, u16 vid, unsigned int port_id, bool add)
{
	unsigned int ifindex, br_ifindex;
	struct iovec iov;
	u16 nlmsg_type, nlmsg_flags;
	struct {
		struct nlmsghdr nh;
		struct br_port_msg bpm;
		char buf[512];
	} req;
	struct br_mdb_entry entry = {
		.state = MDB_PERMANENT,
		.addr.proto = 0,
		.vid = vid,
	};

	if (!logical_port_valid(port_id)) {
		os_log(LOG_ERR, "logical_port(%u) invalid\n", port_id);
		goto err_no_log;
	}

	if (!logical_port_is_bridge(port_id)) {
		os_log(LOG_ERR, "logical_port(%u) is not a valid bridge port\n", port_id);
		goto err_no_log;
	}

	ifindex = if_nametoindex(logical_port_name(port_id));
	if (!ifindex) {
		os_log(LOG_ERR, "if_nametoindex(%s) failed: %s\n", logical_port_name(port_id), strerror(errno));
		goto err_no_log;
	}

	br_ifindex = if_nametoindex(logical_port_bridge_name(port_id));
	if (!br_ifindex) {
		os_log(LOG_ERR, "if_nametoindex(%s) failed: %s\n", logical_port_bridge_name(port_id), strerror(errno));
		goto err_no_log;
	}

	if (add) {
		nlmsg_type = RTM_NEWMDB;	/* add port to mdb entry */
		nlmsg_flags = NLM_F_REQUEST | NLM_F_REPLACE | NLM_F_CREATE;
	} else {
		nlmsg_type = RTM_DELMDB;	/* delete port from mdb entry */
		nlmsg_flags = NLM_F_REQUEST;
	}

	rtnetlink_nlmsghdr_init(&req.nh, NLMSG_LENGTH(sizeof(struct br_port_msg)), nlmsg_type, nlmsg_flags);

	br_port_msg_mdb_init(&req.bpm, br_ifindex);

	entry.ifindex = ifindex; //port field
	entry.addr.proto = 0;
	memcpy(&entry.addr.u, mac_addr, 6);

	if (rtnetlink_attr_add(&req.nh, sizeof(req), MDBA_SET_ENTRY, &entry, sizeof(entry)) < 0)
		goto err;

	iov.iov_base = &req;
	iov.iov_len = req.nh.nlmsg_len;
	if (rtnetlink_socket_send_iov(&iov, 1) < 0)
		goto err;

	os_log(LOG_INFO, "%s MDB: bridge (%s, ifindex %u) logical_port(%u) port (%s, ifindex %u) mac_addr(%02x:%02x:%02x:%02x:%02x:%02x) vlan_id(%u)\n",
			add ? "add" : "remove", logical_port_bridge_name(port_id), br_ifindex, port_id, logical_port_name(port_id), ifindex, mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5], vid);

	return 0;

err:
	os_log(LOG_ERR, "%s MDB failed: bridge (%s, ifindex %u) logical_port(%u) port (%s, ifindex %u) mac_addr(%02x:%02x:%02x:%02x:%02x:%02x) vlan_id(%u)\n",
			add ? "add" : "remove", logical_port_bridge_name(port_id), br_ifindex, port_id, logical_port_name(port_id), ifindex, mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5], vid);

err_no_log:
	return -1;
}

const static struct fdb_ops_cb fdb_std_ops = {
		.bridge_rtnetlink = std_bridge_rtnetlink,
};

int fdb_std_init(struct fdb_ops_cb *fdb_ops)
{
	/* We copy the entire struct rather than just point to it, to reduce the number of
	 * indirections in performance-sensitive code.
	 */
	memcpy(fdb_ops, &fdb_std_ops, sizeof(struct fdb_ops_cb));

	return 0;
}
