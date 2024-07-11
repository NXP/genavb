/*
 * Copyright 2020-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief RTOS specific Network service implementation
 @details
*/

#ifndef _RTOS_NET_PORT_STATS_H_
#define _RTOS_NET_PORT_STATS_H_

typedef enum {
	/* IEEE 802.3 counters (Clause 30) */
	IEEE_OUT_FRAMES,		    /* aFramesTransmittedOK */
	IEEE_IN_FRAMES,			    /* aFramesReceivedOK */
	IEEE_IN_ERROR_CRC_FRAMES,	    /* aFrameCheckSequenceErrors */
	IEEE_IN_ERROR_ALIGN_FRAMES,	    /* aAlignmentErrors */
	IEEE_OUT_ERROR_MAC_INTERNAL_FRAMES, /* aFramesLostDueToIntMACXmitError */
	IEEE_IN_ERROR_MAC_INTERNAL_FRAMES,  /* aFramesLostDueToIntMACRcvError */
	IEEE_OUT_MC_FRAMES,		    /* aMulticastFramesXmittedOK */
	IEEE_OUT_BC_FRAMES,		    /* aBroadcastFramesXmittedOK */
	IEEE_IN_MC_FRAMES,		    /* aMulticastFramesReceivedOK */
	IEEE_IN_BC_FRAMES,		    /* aBroadcastFramesReceivedOK */
	IEEE_OUT_PAUSE_FRAMES,		    /* aPAUSEMACCtrlFramesTransmitted */
	IEEE_IN_PAUSE_FRAMES,		    /* aPAUSEMACCtrlFramesReceived */
	IEEE_OUT_LPI_TIME,		    /* aTransmitLPIMicroseconds */
	IEEE_IN_LPI_TIME,		    /* aReceiveLPIMicroseconds */
	IEEE_OUT_LPI_TRANSITIONS,	    /* aTransmitLPITransitions */
	IEEE_IN_LPI_TRANSITIONS,	    /* aReceiveLPITransitions */
	/* Remote monitoring (IETF RFC 2819) */
	RMON_IN_ERROR_UNDERSIZE_FRAMES, /* etherStatsUndersizePkts */
	RMON_IN_ERROR_OVERSIZE_FRAMES,	/* etherStatsOversizePkts */
	RMON_IN_ERROR_FRAGS_FRAMES,	/* etherStatsFragments */
	RMON_IN_64_FRAMES,		/* etherStatsPkts64Octets */
	RMON_IN_65_127_FRAMES,		/* etherStatsPkts65to127Octets */
	RMON_IN_128_255_FRAMES,		/* etherStatsPkts128to255Octets */
	RMON_IN_256_511_FRAMES,		/* etherStatsPkts256to511Octets */
	RMON_IN_512_1023_FRAMES,	/* etherStatsPkts512to1023Octets */
	RMON_IN_1024_MAX_FRAMES,	/* etherStatsPkts1024to1518Octets */
	RMON_OUT_64_FRAMES,		/* Not part of the spec but common */
	RMON_OUT_65_127_FRAMES,		/* Not part of the spec but common */
	RMON_OUT_128_255_FRAMES,	/* Not part of the spec but common */
	RMON_OUT_256_511_FRAMES,	/* Not part of the spec but common */
	RMON_OUT_512_1023_FRAMES,	/* Not part of the spec but common */
	RMON_OUT_1024_MAX_FRAMES,	/* Not part of the spec but common */
	ETH_STD_STATS_MAX
} eth_std_stats_t;

const char *net_port_stats_str(unsigned int id);

#endif /* _RTOS_NET_PORT_STATS_H_ */
