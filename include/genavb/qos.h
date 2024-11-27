/*
 * Copyright 2020, 2022-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief GenAVB/TSN public API
 \details 802.1Q QoS definitions.

 \copyright Copyright 2020, 2022-2024 NXP
*/
#ifndef _GENAVB_PUBLIC_QOS_H_
#define _GENAVB_PUBLIC_QOS_H_

#include "types.h"

/**
 * \defgroup qos				802.1Q QoS
 * \ingroup library
 */

/**
 * 802.1Q Traffic classes and priorities
 * \ingroup qos
 * @{
 */
#define QOS_TRAFFIC_CLASS_MIN		1	/**< Minimum traffic classes */
#define QOS_TRAFFIC_CLASS_MAX		8	/**< Maximum traffic classes (0 - 7) */
#define QOS_SR_CLASS_MAX		2	/**< Maximum SR classes */
#define QOS_PRIORITY_MAX		8	/**< Maximum priorities (0 - 7) */
/** @}*/

/**
 * \ingroup qos
 *  Configure the traffic class
 *  IEEE 802.1Q 12.6.3 Traffic Class Table
 */
struct genavb_traffic_class_config {
	uint8_t tc[QOS_PRIORITY_MAX];
};

/** Priority to traffic class mapping
 * \ingroup qos
 * \return	array of traffic classes priorities
 * \param enabled_tclass number of enabled traffic classes
 * \param enabled_sr_class number of enabled SR classes
 */
const uint8_t *priority_to_traffic_class_map(unsigned int enabled_tclass, unsigned int enabled_sr_class);

/**
 * 802.1Q Table I-2 Traffic types
 * \ingroup qos
 * @{
 */
#define QOS_BEST_EFFORT_PRIORITY	   0 /**< Best Effort */
#define QOS_BACKGROUND_PRIORITY		   1 /**< Background */
#define QOS_EXCELLENT_EFFORT_PRIORITY	   2 /**< Excellent Effort */
#define QOS_CRITICAL_APPLICATIONS_PRIORITY 3 /**< Critical applications */
#define QOS_VIDEO_PRIORITY		   4 /**< Video, < 100 ms latency and jitter */
#define QOS_VOICE_PRIORITY		   5 /**< Voice, < 10 ms latency and jitter */
#define QOS_INTERNETWORK_CONTROL_PRIORITY  6 /**< Internetwork Control */
#define QOS_NETWORK_CONTROL_PRIORITY	   7 /**< Network Control */
/** @}*/

#define AVDECC_DEFAULT_PRIORITY			QOS_INTERNETWORK_CONTROL_PRIORITY
#define MRP_DEFAULT_PRIORITY			QOS_INTERNETWORK_CONTROL_PRIORITY
#define PTPV2_DEFAULT_PRIORITY 			QOS_INTERNETWORK_CONTROL_PRIORITY /* Per 802.AS-2011 section 8.4.4, priority assigned to PTPv2 packets */
#define ISOCHRONOUS_DEFAULT_PRIORITY 		QOS_VOICE_PRIORITY /* Priority over other traffic types is guaranteed by scheduled traffic support */
#define EVENTS_DEFAULT_PRIORITY			QOS_INTERNETWORK_CONTROL_PRIORITY

/* OS specific headers */
#include "os/qos.h"

#endif /* _GENAVB_PUBLIC_QOS_H_ */
