/*
* Copyright 2022-2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief RTOS specific Network service implementation
 @details
*/

#include "common/types.h"
#include "net_port.h"
#include "net_port_enetc_ep.h"
#include "net_port_netc_sw.h"
#include "genavb/stats.h"

#if CFG_NUM_NETC_SW || CFG_NUM_ENETC_EP_MAC

#include "netc_hw/fsl_netc_hw_port.h"
#include "netc_hw/fsl_netc_hw_si.h"

#define NETC_PORT_RX_STATS_MAX		18
#define NETC_PORT_TX_STATS_MAX		18
#define NETC_PORT_FP_MAX			6
#define NETC_PORT_ERR_STATS_MAX		10
#define NETC_PORT_RX_DISCARD_STATS_MAX	4

#define NETC_PORT_STATS_MAX	(NETC_PORT_RX_STATS_MAX + NETC_PORT_TX_STATS_MAX + \
				NETC_PORT_FP_MAX + NETC_PORT_ERR_STATS_MAX )

static const char *netc_port_stats_str[NETC_PORT_STATS_MAX] = {
	"in-bytes",
	"in-valid-bytes",
	"in-pause-frames",
	"in-valid-frames",
	"in-vlan-frames",
	"in-uc-frames",
	"in-mc-frames",
	"in-bc-frames",
	"in-frames",
	"in-min-frames",
	"in-64-frames",
	"in-65-127-frames",
	"in-128-255-frames",
	"in-256-511-frames",
	"in-512-1023-frames",
	"in-1024-1522-frames",
	"in-1523-max-frames",
	"in-control-frames",

	"out-bytes",
	"out-valid-bytes",
	"out-pause-frames",
	"out-valid-frames",
	"out-vlan-frames",
	"out-uc-frames",
	"out-mc-frames",
	"out-bc-frames",
	"out-frames",
	"out-min-frames",
	"out-64-frames",
	"out-65-127-frames",
	"out-128-255-frames",
	"out-256-511-frames",
	"out-512-1023-frames",
	"out-1024-1522-frames",
	"out-1523-max-frames",
	"out-control-frames",

	"in-valid-reassembled-frames",
	"in-additional-mPackets",
	"in-error-frame-reassembly",
	"in-error-frame-smd",
	"out-additional-mPackets",
	"out-hold-transitions",

	"in-error",
	"in-error-undersize",
	"in-error-oversize",
	"in-error-fcs",
	"in-error-fragment",
	"in-error-jabber",
	"in-error-discard",
	"in-error-dicard-no-truncated",
	"out-error-fcs",
	"out-error-undersize",
};

static const char *netc_port_rx_discard_stats_str[NETC_PORT_RX_DISCARD_STATS_MAX] = {
	"in-discard-count",
	"in-discard-reason0",
	"in-discard-table-id",
	"in-discard-entry-id"
};

static void netc_stats_get_discard(void *port_handle, int type, uint64_t *buf)
{
	netc_port_discard_statistic_t discard;

	NETC_PortGetDiscardStatistic(port_handle, type,
	&discard);

	NETC_PortClearDiscardReason(port_handle, type,
	discard.reason0, discard.reason1);

	buf[0] = discard.count;
	buf[1] = discard.reason0;

	if (type == kNETC_BridgeDiscard) {
		buf[2] = (discard.reason1 & NETC_PORT_BPDCRR1_TT_MASK) >> NETC_PORT_BPDCRR1_TT_SHIFT;
		buf[3] = (discard.reason1 & NETC_PORT_BPDCRR1_ENTRYID_MASK) >> NETC_PORT_BPDCRR1_ENTRYID_SHIFT;
	} else {
		/* The definitions for RX/TX are the same */
		buf[2] = (discard.reason1 & NETC_PORT_PRXDCRR1_TT_MASK) >> NETC_PORT_PRXDCRR1_TT_SHIFT;
		buf[3] = (discard.reason1 & NETC_PORT_PRXDCRR1_ENTRYID_MASK) >> NETC_PORT_PRXDCRR1_ENTRYID_SHIFT;
	}
}

static void netc_stats_get_preemption(void *link_handle, uint64_t *buf)
{
	netc_port_phy_mac_preemption_statistic_t preemption;

	NETC_PortGetPhyMacPreemptionStatistic(link_handle,
	&preemption);

	buf[0] = preemption.rxReassembledFrame;
	buf[1] = preemption.rxMPacket;
	buf[2] = preemption.rxReassembledError;
	buf[3] = preemption.rxSMDError;
	buf[4] = preemption.txMPacket;
	buf[5] = preemption.txPreemptionReq;
}

#endif /* CFG_NUM_NETC_SW || CFG_NUM_ENETC_EP_MAC */

#if CFG_NUM_ENETC_EP_MAC

#define NETC_EP_PORT_STATS_MAX (NETC_PORT_STATS_MAX + NETC_PORT_RX_DISCARD_STATS_MAX)

#define NETC_PSEUDO_PORT_RX_STATS_MAX	4
#define NETC_PSEUDO_PORT_TX_STATS_MAX	4
#define NETC_PSEUDO_PORT_STATS_MAX	(NETC_PSEUDO_PORT_RX_STATS_MAX + NETC_PSEUDO_PORT_TX_STATS_MAX)

#define NETC_PSEUDO_PORT_SI_STATS_MAX	8

static const char *netc_pseudo_port_stats_str[NETC_PSEUDO_PORT_STATS_MAX] = {
	"in-bytes",
	"in-uc-frames",
	"in-mc-frames",
	"in-bc-frames",
	"out-bytes",
	"out-uc-frames",
	"out-mc-frames",
	"out-bc-frames"
};

static const char *netc_pseudo_port_si_stats_str[NETC_PSEUDO_PORT_SI_STATS_MAX] = {
    "si-in-bytes",
    "si-in-frames",
    "si-in-uc-frames",
    "si-in-mc-frames",
    "si-out-bytes",
    "si-out-frames",
    "si-out-uc-frames",
    "si-out-mc-frames"
};

static int netc_ep_pseudo_stats_get_number(struct net_port *port)
{
	return NETC_PSEUDO_PORT_STATS_MAX + NETC_PSEUDO_PORT_SI_STATS_MAX;
}

static int netc_ep_pseudo_stats_get_strings(struct net_port *port, const char **buf, unsigned int buf_len)
{
	int i;

	if (buf_len < ((NETC_PSEUDO_PORT_STATS_MAX + NETC_PSEUDO_PORT_SI_STATS_MAX) * sizeof(char *)))
		return -1;

	for (i = 0; i < NETC_PSEUDO_PORT_STATS_MAX; i++) {
		*buf = netc_pseudo_port_stats_str[i];
		buf++;
	}

	for (i = 0; i < NETC_PSEUDO_PORT_SI_STATS_MAX; i++) {
		*buf = netc_pseudo_port_si_stats_str[i];
		buf++;
	}

	return 0;
}

static int netc_ep_pseudo_stats_get(struct net_port *port, uint64_t *buf, unsigned int buf_len)
{
	void *link_handle = enetc_ep_get_link_handle(port);
	void *si_handle = enetc_ep_get_si_handle(port);

	if (buf_len < ((NETC_PSEUDO_PORT_STATS_MAX + NETC_PSEUDO_PORT_SI_STATS_MAX) * sizeof(uint64_t)))
		return -1;

	NETC_PortGetPseudoMacTrafficStatistic(link_handle, false, (netc_port_pseudo_mac_traffic_statistic_t *)buf);
	buf += NETC_PSEUDO_PORT_RX_STATS_MAX;

	NETC_PortGetPseudoMacTrafficStatistic(link_handle, true, (netc_port_pseudo_mac_traffic_statistic_t *)buf);
	buf += NETC_PSEUDO_PORT_TX_STATS_MAX;

	NETC_SIGetTrafficStatistic(si_handle, (netc_si_traffic_statistic_t *)buf);

	return 0;
}

static int netc_ep_pseudo_vsi_stats_get_number(struct net_port *port)
{
	return NETC_PSEUDO_PORT_SI_STATS_MAX;
}

static int netc_ep_pseudo_vsi_stats_get_strings(struct net_port *port, const char **buf, unsigned int buf_len)
{
	int i;

	if (buf_len < (NETC_PSEUDO_PORT_SI_STATS_MAX * sizeof(char *)))
		return -1;

	for (i = 0; i < NETC_PSEUDO_PORT_SI_STATS_MAX; i++) {
		*buf = netc_pseudo_port_si_stats_str[i];
		buf++;
	}

	return 0;
}

static int netc_ep_pseudo_vsi_stats_get(struct net_port *port, uint64_t *buf, unsigned int buf_len)
{
	void *si_handle = enetc_ep_get_si_handle(port);

	if (buf_len < (NETC_PSEUDO_PORT_SI_STATS_MAX * sizeof(uint64_t)))
		return -1;

	NETC_SIGetTrafficStatistic(si_handle, (netc_si_traffic_statistic_t *)buf);

	return 0;
}

static int netc_ep_stats_get_number(struct net_port *port)
{
	return NETC_EP_PORT_STATS_MAX;
}

static int netc_ep_stats_get_strings(struct net_port *port, const char **buf, unsigned int buf_len)
{
	int i;

	if (buf_len < (NETC_EP_PORT_STATS_MAX * sizeof(char *)))
		return -1;

	for (i = 0; i < NETC_PORT_STATS_MAX; i++) {
		*buf = netc_port_stats_str[i];
		buf++;
	}

	for (i = 0; i < NETC_PORT_RX_DISCARD_STATS_MAX; i++) {
		*buf = netc_port_rx_discard_stats_str[i];
		buf++;
	}

	return 0;
}

static int netc_ep_stats_get(struct net_port *port, uint64_t *buf, unsigned int buf_len)
{
	void *link_handle = enetc_ep_get_link_handle(port);
	void *port_handle = enetc_ep_get_port_handle(port);

	if (buf_len < (NETC_EP_PORT_STATS_MAX * sizeof(uint64_t)))
		return -1;

	NETC_PortGetPhyMacRxStatistic(link_handle, kNETC_ExpressMAC, (netc_port_phy_mac_traffic_statistic_t *)buf);
	buf += NETC_PORT_RX_STATS_MAX;

	NETC_PortGetPhyMacTxStatistic(link_handle, kNETC_ExpressMAC, (netc_port_phy_mac_traffic_statistic_t *)buf);
	buf += NETC_PORT_TX_STATS_MAX;

	netc_stats_get_preemption(link_handle, buf);
	buf += NETC_PORT_FP_MAX;

	NETC_PortGetPhyMacDiscardStatistic(link_handle, kNETC_ExpressMAC, (netc_port_phy_mac_discard_statistic_t *)buf);
	buf += NETC_PORT_ERR_STATS_MAX;

	netc_stats_get_discard(port_handle, kNETC_RxDiscard, buf);
	buf += NETC_PORT_RX_DISCARD_STATS_MAX;

	return 0;
}

__init void netc_ep_pseudo_stats_init(struct net_port *port)
{
	if (port->base == kNETC_ENETC1VSI1) {
		port->drv_ops.stats_get_number = netc_ep_pseudo_vsi_stats_get_number;
		port->drv_ops.stats_get_strings = netc_ep_pseudo_vsi_stats_get_strings;
		port->drv_ops.stats_get = netc_ep_pseudo_vsi_stats_get;
	} else {
		port->drv_ops.stats_get_number = netc_ep_pseudo_stats_get_number;
		port->drv_ops.stats_get_strings = netc_ep_pseudo_stats_get_strings;
		port->drv_ops.stats_get = netc_ep_pseudo_stats_get;
	}
}

__init void netc_ep_stats_init(struct net_port *port)
{
	port->drv_ops.stats_get_number = netc_ep_stats_get_number;
	port->drv_ops.stats_get_strings = netc_ep_stats_get_strings;
	port->drv_ops.stats_get = netc_ep_stats_get;
}

#endif /* CFG_NUM_ENETC_EP_MAC */

#if CFG_NUM_NETC_SW

#include "fsl_netc_switch.h"

#define NETC_PORT_TX_DISCARD_STATS_MAX	4
#define NETC_SW_DISCARD_STATS_MAX	4
#define NETC_SW_STATS_PER_QUEUE		7
#define NETC_SW_QUEUE_PER_PORT		8
#define NETC_QUEUE_STATS_MAX	(NETC_SW_QUEUE_PER_PORT * NETC_SW_STATS_PER_QUEUE)
#define NETC_SW_STATS_MAX	(NETC_PORT_STATS_MAX + NETC_PORT_RX_DISCARD_STATS_MAX + \
							NETC_PORT_TX_DISCARD_STATS_MAX + NETC_SW_DISCARD_STATS_MAX + \
							NETC_QUEUE_STATS_MAX)

#define NETC_SW_PSEUDO_PORT_RX_STATS_MAX	4
#define NETC_SW_PSEUDO_PORT_TX_STATS_MAX	4
#define NETC_SW_PSEUDO_PORT_STATS_MAX	(NETC_SW_PSEUDO_PORT_RX_STATS_MAX + NETC_SW_PSEUDO_PORT_TX_STATS_MAX + \
				NETC_PORT_RX_DISCARD_STATS_MAX + NETC_PORT_TX_DISCARD_STATS_MAX + NETC_SW_DISCARD_STATS_MAX)

static const char *netc_port_tx_discard_stats_str[NETC_PORT_TX_DISCARD_STATS_MAX] = {
	"out-discard-count",
	"out-discard-reason0",
	"out-discard-table-id",
	"out-discard-entry-id"
};

static const char *netc_sw_discard_stats_str[NETC_SW_DISCARD_STATS_MAX] = {
	"bridge-discard-count",
	"bridge-discard-reason0",
	"bridge-discard-table-id",
	"bridge-discard-entry-id"
};

static const char *netc_queue_stats_str[NETC_QUEUE_STATS_MAX] = {
	"q0-rejected-bytes",
	"q0-rejected-frames",
	"q0-dequeue-bytes",
	"q0-dequeue-frames",
	"q0-dropped-bytes",
	"q0-dropped-frames",
	"q0-frames",
	"q1-rejected-bytes",
	"q1-rejected-frames",
	"q1-dequeue-bytes",
	"q1-dequeue-frames",
	"q1-dropped-bytes",
	"q1-dropped-frames",
	"q1-frames",
	"q2-rejected-bytes",
	"q2-rejected-frames",
	"q2-dequeue-bytes",
	"q2-dequeue-frames",
	"q2-dropped-bytes",
	"q2-dropped-frames",
	"q2-frames",
	"q3-rejected-bytes",
	"q3-rejected-frames",
	"q3-dequeue-bytes",
	"q3-dequeue-frames",
	"q3-dropped-bytes",
	"q3-dropped-frames",
	"q3-frames",
	"q4-rejected-bytes",
	"q4-rejected-frames",
	"q4-dequeue-bytes",
	"q4-dequeue-frames",
	"q4-dropped-bytes",
	"q4-dropped-frames",
	"q4-frames",
	"q5-rejected-bytes",
	"q5-rejected-frames",
	"q5-dequeue-bytes",
	"q5-dequeue-frames",
	"q5-dropped-bytes",
	"q5-dropped-frames",
	"q5-frames",
	"q6-rejected-bytes",
	"q6-rejected-frames",
	"q6-dequeue-bytes",
	"q6-dequeue-frames",
	"q6-dropped-bytes",
	"q6-dropped-frames",
	"q6-frames",
	"q7-rejected-bytes",
	"q7-rejected-frames",
	"q7-dequeue-bytes",
	"q7-dequeue-frames",
	"q7-dropped-bytes",
	"q7-dropped-frames",
	"q7-frames"
};

static int netc_sw_pseudo_stats_get_number(struct net_port *port)
{
	return NETC_SW_PSEUDO_PORT_STATS_MAX;
}

static int netc_sw_pseudo_stats_get_strings(struct net_port *port, const char **buf, unsigned int buf_len)
{
	int i;

	if (buf_len < (NETC_SW_PSEUDO_PORT_STATS_MAX * sizeof(char *)))
		return -1;

	for (i = 0; i < (NETC_SW_PSEUDO_PORT_RX_STATS_MAX + NETC_SW_PSEUDO_PORT_TX_STATS_MAX); i++) {
		*buf = netc_pseudo_port_stats_str[i];
		buf++;
	}

	for (i = 0; i < NETC_PORT_RX_DISCARD_STATS_MAX; i++) {
		*buf = netc_port_rx_discard_stats_str[i];
		buf++;
	}

	for (i = 0; i < NETC_PORT_TX_DISCARD_STATS_MAX; i++) {
		*buf = netc_port_tx_discard_stats_str[i];
		buf++;
	}

	for (i = 0; i < NETC_SW_DISCARD_STATS_MAX; i++) {
		*buf = netc_sw_discard_stats_str[i];
		buf++;
	}

	return 0;
}

static int netc_sw_pseudo_stats_get(struct net_port *port, uint64_t *buf, unsigned int buf_len)
{
	void *link_handle = netc_sw_get_link_handle(port);
	void *port_handle = netc_sw_get_port_handle(port);

	if (buf_len < (NETC_SW_PSEUDO_PORT_STATS_MAX * sizeof(uint64_t)))
		return -1;

	NETC_PortGetPseudoMacTrafficStatistic(link_handle, false, (netc_port_pseudo_mac_traffic_statistic_t *)buf);
	buf += NETC_SW_PSEUDO_PORT_RX_STATS_MAX;

	NETC_PortGetPseudoMacTrafficStatistic(link_handle, true, (netc_port_pseudo_mac_traffic_statistic_t *)buf);
	buf += NETC_SW_PSEUDO_PORT_TX_STATS_MAX;

	netc_stats_get_discard(port_handle, kNETC_RxDiscard, buf);
	buf += NETC_PORT_RX_DISCARD_STATS_MAX;

	netc_stats_get_discard(port_handle, kNETC_TxDiscard, buf);
	buf += NETC_PORT_TX_DISCARD_STATS_MAX;

	netc_stats_get_discard(port_handle, kNETC_BridgeDiscard, buf);
	buf += NETC_SW_DISCARD_STATS_MAX;

	return 0;
}

static int netc_sw_stats_get_number(struct net_port *port)
{
	return NETC_SW_STATS_MAX;
}

static int netc_sw_stats_get_strings(struct net_port *port, const char **buf, unsigned int buf_len)
{
	int i;

	if (buf_len < (NETC_SW_STATS_MAX * sizeof(char *)))
		return -1;

	for (i = 0; i < NETC_PORT_STATS_MAX; i++) {
		*buf = netc_port_stats_str[i];
		buf++;
	}

	for (i = 0; i < NETC_PORT_RX_DISCARD_STATS_MAX; i++) {
		*buf = netc_port_rx_discard_stats_str[i];
		buf++;
	}

	for (i = 0; i < NETC_PORT_TX_DISCARD_STATS_MAX; i++) {
		*buf = netc_port_tx_discard_stats_str[i];
		buf++;
	}

	for (i = 0; i < NETC_SW_DISCARD_STATS_MAX; i++) {
		*buf = netc_sw_discard_stats_str[i];
		buf++;
	}

	for (i = 0; i < NETC_QUEUE_STATS_MAX; i++) {
		*buf = netc_queue_stats_str[i];
		buf++;
	}

	return 0;
}

static int netc_sw_stats_get(struct net_port *port, uint64_t *buf, unsigned int buf_len)
{
	void *link_handle = netc_sw_get_link_handle(port);
	void *port_handle = netc_sw_get_port_handle(port);
	uint32_t entry_id;
	int i;

	if (buf_len < (NETC_SW_STATS_MAX * sizeof(uint64_t)))
		return -1;

	NETC_PortGetPhyMacRxStatistic(link_handle, kNETC_ExpressMAC, (netc_port_phy_mac_traffic_statistic_t *)buf);
	buf += NETC_PORT_RX_STATS_MAX;

	NETC_PortGetPhyMacTxStatistic(link_handle, kNETC_ExpressMAC, (netc_port_phy_mac_traffic_statistic_t *)buf);
	buf += NETC_PORT_TX_STATS_MAX;

	netc_stats_get_preemption(link_handle, buf);
	buf += NETC_PORT_FP_MAX;

	NETC_PortGetPhyMacDiscardStatistic(link_handle, kNETC_ExpressMAC, (netc_port_phy_mac_discard_statistic_t *)buf);
	buf += NETC_PORT_ERR_STATS_MAX;

	netc_stats_get_discard(port_handle, kNETC_RxDiscard, buf);
	buf += NETC_PORT_RX_DISCARD_STATS_MAX;

	netc_stats_get_discard(port_handle, kNETC_TxDiscard, buf);
	buf += NETC_PORT_TX_DISCARD_STATS_MAX;

	netc_stats_get_discard(port_handle, kNETC_BridgeDiscard, buf);
	buf += NETC_SW_DISCARD_STATS_MAX;

	for (i = 0; i < NETC_SW_QUEUE_PER_PORT; i++) {
		entry_id = (port->base << 4) | i;

		buf[NETC_SW_STATS_PER_QUEUE - 1] = 0;

		SWT_GetETMClassQueueStatistic(netc_sw_get_handle(port), entry_id, (netc_tb_etmcq_stse_t *)buf);

		buf += NETC_SW_STATS_PER_QUEUE;
	}

	return 0;
}

__init void netc_sw_stats_init(struct net_port *port)
{
	if (NETC_PortIsPseudo(netc_sw_get_port_handle(port))) {
		port->drv_ops.stats_get_number = netc_sw_pseudo_stats_get_number;
		port->drv_ops.stats_get_strings = netc_sw_pseudo_stats_get_strings;
		port->drv_ops.stats_get = netc_sw_pseudo_stats_get;
	} else {
		port->drv_ops.stats_get_number = netc_sw_stats_get_number;
		port->drv_ops.stats_get_strings = netc_sw_stats_get_strings;
		port->drv_ops.stats_get = netc_sw_stats_get;
	}
}

#endif /* CFG_NUM_NETC_SW */
