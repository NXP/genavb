/*
* Copyright 2020-2021, 2023-2024 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>
#include <linux/pkt_sched.h>
#include <net/if.h>

#include "common/log.h"
#include "os/net.h"

#include "net_logical_port.h"
#include "rtnetlink.h"

#include "genavb/helpers.h"

#include "net.h"
#include "fqtss.h"

#define CBS_QDISC_HANDLE_BASE	0x9000
#define CBS_QDISC_ID 		"cbs"

#define tcmsg_init(tcm, handle, parent, ifindex) \
	(tcm)->tcm_family = AF_UNSPEC; \
	(tcm)->tcm_handle = handle; \
	(tcm)->tcm_parent = parent; \
	(tcm)->tcm_ifindex = ifindex; \
	(tcm)->tcm_info = 0;

#if defined(TCA_CBS_MAX)

static int fqtss_std_set_oper_idle_slope(unsigned int port_id, uint8_t traffic_class, uint64_t idle_slope)
{
	unsigned int ifindex;
	struct iovec iov;
	bool up, point_to_point;
	uint64_t port_rate;
	struct {
		struct nlmsghdr nh;
		struct tcmsg tcm;
		char buf[1024];
	} req;
	struct rtattr *options_attr;
	struct tc_cbs_qopt cbs_opt = {
		.offload = 1,
		.hicredit = INT32_MAX,
		.locredit = INT32_MIN,
	};

	const char *qdisc_id = CBS_QDISC_ID;
	u32 handle = ((CBS_QDISC_HANDLE_BASE + traffic_class) << 16);

	if (!logical_port_valid(port_id)) {
		os_log(LOG_ERR, "logical_port(%u) invalid\n", port_id);
		goto err_no_log;
	}

	if (!logical_port_is_bridge(port_id)) {
		os_log(LOG_ERR, "logical_port(%u) is not a bridge port\n", port_id);
		goto err_no_log;
	}

	ifindex = if_nametoindex(logical_port_name(port_id));
	if (!ifindex) {
		os_log(LOG_ERR, "if_nametoindex(%s) failed: %s\n", logical_port_name(port_id), strerror(errno));
		goto err_no_log;
	}

	if (net_std_port_status(port_id, &up, &point_to_point, &port_rate) < 0) {
		os_log(LOG_ERR, "net_std_port_status(%u) failed\n", port_id);
		goto err;
	}

	/* idleslope and sendslope in kbits per sec */
	cbs_opt.idleslope = idle_slope / 1000; /* idleslope in kbits per sec */
	cbs_opt.sendslope = cbs_opt.idleslope - port_rate * 1000;

	rtnetlink_nlmsghdr_init(&req.nh, NLMSG_LENGTH(sizeof(struct tcmsg)), RTM_NEWQDISC, NLM_F_REQUEST | NLM_F_REPLACE);

	/* Update the right CBS Qdisc*/
	tcmsg_init(&req.tcm, handle, 0, ifindex);

	if (rtnetlink_attr_add(&req.nh, sizeof(req), TCA_KIND, qdisc_id, strlen(qdisc_id) + 1) < 0)
		goto err;

	options_attr = (struct rtattr *)(((char *) &req) + NLMSG_ALIGN(req.nh.nlmsg_len));

	if (rtnetlink_attr_add(&req.nh, sizeof(req), TCA_OPTIONS, NULL, 0) < 0)
		goto err;

	if (rtnetlink_attr_add(&req.nh, sizeof(req), TCA_CBS_PARMS, &cbs_opt, sizeof(cbs_opt)) < 0)
		goto err;

	/* Update the options nested attribute len */
	options_attr->rta_len = (char *) NLMSG_NEXT_DATA(&req.nh) - (char *)options_attr;

	iov.iov_base = &req;
	iov.iov_len = req.nh.nlmsg_len;

	if (rtnetlink_socket_send_iov(&iov, 1) < 0)
		goto err;

	os_log(LOG_INFO, "logical_port(%u) port (%s, ifindex %u) tc(%u) cbs_qdisc_handle(%x:%x): set idle_slope %" PRIu64 " \n", port_id, logical_port_name(port_id), ifindex, traffic_class, TC_H_MAJ(handle) >> 16, TC_H_MIN(handle), idle_slope);

	return 0;

err:
	os_log(LOG_ERR, "logical_port(%u) port (%s, ifindex %u) tc(%u) cbs_qdisc_handle(%x:%x): failed to set idle_slope %" PRIu64 " \n", port_id, logical_port_name(port_id), ifindex, traffic_class, TC_H_MAJ(handle) >> 16, TC_H_MIN(handle), idle_slope);

err_no_log:
	return -1;
}
#else

#pragma message "Building with old kernel headers: no rtnetlink CBS support"

static int fqtss_std_set_oper_idle_slope(unsigned int port_id, uint8_t traffic_class, uint64_t idle_slope)
{
	return 0;
}
#endif //TCA_CBS_MAX

static int fqtss_std_stream_add(unsigned int port_id, void *stream_id, uint16_t vlan_id, uint8_t priority, uint64_t idle_slope)
{
	return 0;
}

static int fqtss_std_stream_remove(unsigned int port_id, void *stream_id, uint16_t vlan_id, uint8_t priority, uint64_t idle_slope)
{
	return 0;
}

static void fqtss_std_exit(void)
{
	return;
}

const static struct fqtss_ops_cb fqtss_std_ops = {
		.fqtss_set_oper_idle_slope = fqtss_std_set_oper_idle_slope,
		.fqtss_stream_add = fqtss_std_stream_add,
		.fqtss_stream_remove = fqtss_std_stream_remove,
		.fqtss_exit = fqtss_std_exit,
};

int fqtss_std_init(struct fqtss_ops_cb *fqtss_ops)
{
	/* We copy the entire struct rather than just point to it, to reduce the number of
	 * indirections in performance-sensitive code.
	 */
	memcpy(fqtss_ops, &fqtss_std_ops, sizeof(struct fqtss_ops_cb));

	return 0;
}
