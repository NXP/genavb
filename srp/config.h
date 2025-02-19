/*
 * Copyright 2014 Freescale Semiconductor, Inc.
 * Copyright 2016-2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief SRP static configuration
 @details Contains all compile time configuration options for SRP
*/

#ifndef _SRP_CFG_H_
#define _SRP_CFG_H_

#include "common/config.h"

#define srp_CFG_LOG	CFG_LOG

/* 2 applications: MVRP + MSRP */
/* 3 timers per application: LeaveAll, Join Periodic */
#define CFG_SRP_MAX_TIMERS_PER_PORT	(2 * 3)

#define CFG_PORT_TC_MAX_LATENCY	500	/* ns */

#define CFG_MVRP_PARTICIPANT_TYPE	MRP_PARTICIPANT_TYPE_FULL_POINT_TO_POINT
#define CFG_MSRP_PARTICIPANT_TYPE	MRP_PARTICIPANT_TYPE_FULL_POINT_TO_POINT

/* to be defined if no upper layer connected to SRP */
//#define CFG_MRP_AUTO_LISTENER_REPLY 1

/* disable following define to reduce SRP log messages */
#define CFG_MRP_SM_LOG 1

/* FIXME: below values to be discussed */
#define CFG_MVRP_MAX_VLANS	32
#define CFG_MVRP_VID		MRP_DEFAULT_VID

/*
 * 802.1Q-2011
 * 9.6 and 11.2.3.1.7
 */
#define CFG_MVRP_VID_MIN	1
#define CFG_MVRP_VID_MAX	4094

#define CFG_MSRP_MAX_STREAMS	150 /* Should be exactly 150 streams for Milan testcases: bres-4.2/4.3/5.2 */
#define CFG_MSRP_MAX_DOMAINS	8
#define CFG_MSRP_MAX_CLASSES	8

#define CFG_MSRP_DEFAULT_SEND_LEAVE_ON_DECLARATION_UPDATE	0 /* By default, do not send leave event on listener or talker declaration update. */

/*
 * MRP Leave Timeout in milliseconds
 * As per Milan v1.2 Table 4.3 (default: 5000ms, min: 4500ms, max: 7500ms)
 * and 802.1Q-2018 Table 10-7 (timeout between 600ms and 1000ms)
 */
#define CFG_MRP_LVTIMER_VAL_MILAN_DEFAULT			5000
#define CFG_MRP_LVTIMER_VAL_802_1Q_DEFAULT			1000
#define CFG_MRP_LVTIMER_VAL_MIN					600
#define CFG_MRP_LVTIMER_VAL_MAX					7500

#endif /* _SRP_CFG_H_ */
