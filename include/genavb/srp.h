/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
  \file		srp.h
  \brief		SRP common definitions
  \details	Public interface to the SRP module (MSRP, MVRP, MMRP)

  \copyright Copyright 2014-2016 Freescale Semiconductor, Inc.
  \copyright Copyright 2016, 2021, 2023 NXP
*/

#ifndef _GENAVB_PUBLIC_SRP_H_
#define _GENAVB_PUBLIC_SRP_H_

#define MRP_DEFAULT_VID			htons(0x0002)

/**
* MSRP Failure Information values (802.1Qat 35.2.2.8.7)
*/
typedef enum
{
	INSUFFICIENT_BANDWIDTH = 1,
	INSUFFICIENT_BRIDGE_RESOURCES,
	INSUFFICIENT_BANDWIDTH_FOR_TRAFFIC_CLASS,
	STREAM_ID_ALREADY_IN_USE,
	STREAM_DESTINATION_ADDRESS_ALREADY_IN_USE,
	STREAM_PREEMPTED_BY_HIGHER_RANK,
	REPORTED_LATENCY_HAS_CHANGED,
	EGRESS_PORT_IS_NOT_AVB_CAPABLE,
	USE_DIFFERENT_DESTINATION_ADDRESS,
	OUT_OF_MSRP_RESOURCES,
	OUT_OF_MMRP_RESOURCES,
	CANNOT_STORE_DESTINATION_ADDRESS,
	REQUESTED_PRIORITY_IS_NOT_AN_SR_CLASS_PRIORITY,
	MAX_FRAME_SIZE_TOO_LARGE_FOR_MEDIA,
	FAN_IN_PORT_LIMIT_REACHED,
	CHANGE_IN_FIRST_VALUE_FOR_REGISTED_STREAM_ID,
	VLAN_BLOCKED_ON_EGRESS_PORT,
	VLAN_TAGGING_DISABLED_ON_EGRESS_PORT,
	SR_CLASS_PRIORITY_MISMATCH
} msrp_reservation_failure_code_t;

/**
 * \ingroup stream
 * SRP Stream Rank
 */
typedef enum {
	EMERGENCY = 0,		/**< Emergency, 802.1Q-2018, 35.2.2.8.5 b) */
	NORMAL = 1		/**< Normal, 802.1Q-2018, 35.2.2.8.5 b) */
} msrp_rank_t;

struct msrp_failure_information {
	avb_u8 bridge_id[8];
	avb_u8 failure_code;
};

#endif /* _GENAVB_PUBLIC_SRP_H_ */
