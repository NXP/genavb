/*
* Copyright 2021, 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#include <string.h>
#include <linux/filter.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>

#include "common/log.h"
#include "common/ptp.h"

#include "genavb/ether.h"

#include "net_std_socket_filters.h"

#define BPF_FILTER_OUT_PACKET_OUTGOING_INSTR(jump_false) \
	{ BPF_LD | BPF_B | BPF_ABS, 0, 0, (uint32_t) (SKF_AD_OFF + SKF_AD_PKTTYPE) }, \
	{ BPF_JMP | BPF_JEQ | BPF_K, jump_false, 0, PACKET_OUTGOING }

#define BPF_FILTER_ARRAY_SIZE(array_name) \
	sizeof(array_name) / sizeof(array_name[0]);

static const struct sock_filter bpf_ptp_filter[] = {
	BPF_FILTER_OUT_PACKET_OUTGOING_INSTR(7), // filter out PACKET_OUTGOING
	[2] = { BPF_LD | BPF_H | BPF_ABS, 0, 0, ETHER_ETYPE_OFFSET }, // load etype value
	[3] = { BPF_JMP | BPF_JEQ | BPF_K, 0, 5, ETHERTYPE_PTP },
	[4] = { BPF_LD | BPF_H | BPF_ABS, 0, 0, ETH_HLEN }, // load and check the transportSpecific/majorSdoId and versionPTP fields
	[5] = { BPF_ALU | BPF_AND | BPF_K, 0, 0, 0xF00F },
	[6] = { BPF_JMP | BPF_JEQ | BPF_K, 1, 0, 0x1002 }, // accept gPTP domain 802.1AS packets
	[7] = { BPF_JMP | BPF_JEQ | BPF_K, 0, 1, 0x2002 }, // accept CMLDS 802.1AS packets
	[8] = { BPF_RET | BPF_K, 0, 0, 0x00040000 }, // accept
	[9] = { BPF_RET | BPF_K, 0, 0, 0x00000000 }, // reject
};

static const struct sock_filter bpf_mrp_filter[] = {
	BPF_FILTER_OUT_PACKET_OUTGOING_INSTR(5), // filter out PACKET_OUTGOING
	[2] = { BPF_LD | BPF_H | BPF_ABS, 0, 0, ETHER_ETYPE_OFFSET }, // load etype value
	[3] = { BPF_JMP | BPF_JEQ | BPF_K, 2, 0, ETHERTYPE_MMRP },
	[4] = { BPF_JMP | BPF_JEQ | BPF_K, 1, 0, ETHERTYPE_MVRP },
	[5] = { BPF_JMP | BPF_JEQ | BPF_K, 0, 1, ETHERTYPE_MSRP },
	[6] = { BPF_RET | BPF_K, 0, 0, 0x00040000 }, // accept
	[7] = { BPF_RET | BPF_K, 0, 0, 0x00000000 }, // reject
};

static const struct sock_filter bpf_l2_filter[] = {
	BPF_FILTER_OUT_PACKET_OUTGOING_INSTR(12), // filter out PACKET_OUTGOING
	[2] = { BPF_LD | BPF_W | BPF_ABS, 0, 0, 2 }, // Read the 4 Least Significant Bytes from destination MAC
 #define BPF_FILTER_L2_DST_MAC_LSB_CHECK_INSTR_NUM       3
	[3] = { BPF_JMP | BPF_JEQ | BPF_K, 0, 10, 0x0 }, // Updated on runtime with the right dst MAC (4 LSB), BPF_FILTER_L2_DST_MAC_LSB_CHECK_INSTR_NUM should match the array index here
	[4] = { BPF_LD | BPF_H | BPF_ABS, 0, 0, 0 }, // Read the 2 Most Significant Bytes from destination MAC
#define BPF_FILTER_L2_DST_MAC_MSB_CHECK_INSTR_NUM       5
	[5] = { BPF_JMP | BPF_JEQ | BPF_K, 0, 8, 0x0 }, // Updated on runtime with the right dst MAC (2 MSB), BPF_FILTER_L2_DST_MAC_MSB_CHECK_INSTR_NUM should match the array index here
	[6] = { BPF_LD | BPF_H | BPF_ABS, 0, 0, ETHER_ETYPE_OFFSET }, // load etype value
	[7] = { BPF_JMP | BPF_JEQ | BPF_K, 0, 4, ETHERTYPE_VLAN }, // L2 Frames can be vlan tagged: jump to non-vlan block if not
	[8] = { BPF_LD | BPF_H | BPF_ABS, 0, 0, ETHER_VLAN_LABEL_OFFSET }, // load vlan label value
	[9] = { BPF_ALU | BPF_AND | BPF_K, 0, 0, 0x0FFF }, // check only the vlan id
#define BPF_FILTER_L2_VLAN_ID_CHECK_INSTR_NUM           10
	[10] = { BPF_JMP | BPF_JEQ | BPF_K, 0, 3, 0 }, // Updated on runtime with the right vlan_id, BPF_FILTER_L2_VLAN_ID_CHECK_INSTR_NUM should match the array index here
	[11] = { BPF_LD | BPF_H | BPF_ABS, 0, 0, ETHER_ETYPE_OFFSET + 4 },
#define BPF_FILTER_L2_ETYPE_CHECK_INSTR_NUM             12
	[12] = { BPF_JMP | BPF_JEQ | BPF_K, 0, 1, 0}, // Updated on runtime with the right etype, BPF_FILTER_L2_ETYPE_CHECK_INSTR_NUM should match the array index here
	[13] = { BPF_RET | BPF_K, 0, 0, 0x00040000 }, // accept
	[14] = { BPF_RET | BPF_K, 0, 0, 0x00000000 }, // reject
};

/* Copy the right BPF filter code depending on the ethernet type:
 * inst_count is a pointer to the BPF filter array size in number of filter blocks: instructions count */
int sock_filter_get_bpf_code(struct net_address *addr, void *buf, unsigned int *inst_count)
{
	int rc = 0;
	const void *src_bpf;
	unsigned int bpf_filter_inst_count;
	struct sock_filter *filter = (struct sock_filter *) buf;
	u32 dst_mac_msb, dst_mac_lsb;

	switch (addr->ptype) {
	case PTYPE_PTP:
		src_bpf = bpf_ptp_filter;
		bpf_filter_inst_count = BPF_FILTER_ARRAY_SIZE(bpf_ptp_filter);
		break;

	case PTYPE_MRP:
		src_bpf = bpf_mrp_filter;
		bpf_filter_inst_count = BPF_FILTER_ARRAY_SIZE(bpf_mrp_filter);
		break;

	case PTYPE_L2:
		src_bpf = bpf_l2_filter;
		bpf_filter_inst_count = BPF_FILTER_ARRAY_SIZE(bpf_l2_filter);
		break;

	default:
		rc = -1;
		goto exit;
	}

	if (*inst_count < bpf_filter_inst_count) {
		rc = -1;
		os_log(LOG_ERR, "Provided buffer size is smaller than needed for ptype %d (%u < %u)\n",
				addr->ptype, *inst_count, bpf_filter_inst_count);
		goto exit;
	}

	memcpy(buf, src_bpf, bpf_filter_inst_count * sizeof(struct sock_filter));
	*inst_count = bpf_filter_inst_count;

	if (addr->ptype == PTYPE_L2) {
		/* Update the etype check instruction value */
		filter[BPF_FILTER_L2_ETYPE_CHECK_INSTR_NUM].k = ntohs(addr->u.l2.protocol);

		/* Update the vlan id check instruction value */
		filter[BPF_FILTER_L2_VLAN_ID_CHECK_INSTR_NUM].k = ntohs(addr->vlan_id);

		/* Update the dst mac address check instructions values */
		dst_mac_msb = addr->u.l2.dst_mac[1] | (addr->u.l2.dst_mac[0] << 8);
		dst_mac_lsb = addr->u.l2.dst_mac[5] | (addr->u.l2.dst_mac[4] << 8) | (addr->u.l2.dst_mac[3] << 16) | (addr->u.l2.dst_mac[2] << 24);

		filter[BPF_FILTER_L2_DST_MAC_MSB_CHECK_INSTR_NUM].k = dst_mac_msb;
		filter[BPF_FILTER_L2_DST_MAC_LSB_CHECK_INSTR_NUM].k = dst_mac_lsb;
	}
exit:
	return rc;
}
