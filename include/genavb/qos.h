/*
 * Copyright 2020, 2022-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief GenAVB public API
 \details 802.1Q QoS definitions.

 \copyright Copyright 2020, 2022-2024 NXP
*/
#ifndef _GENAVB_PUBLIC_QOS_H_
#define _GENAVB_PUBLIC_QOS_H_

#include "genavb/types.h"
#include "genavb/clock.h"

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

#define QOS_MAX_PDU_BYTES 0x600U
#define QOS_MAX_SDU_BYTES (QOS_MAX_PDU_BYTES - 12 - 4)

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

/**
 * \ingroup qos
 * Scheduled traffic configuration type
 */
typedef enum {
	GENAVB_ST_OPER,
	GENAVB_ST_ADMIN
} genavb_st_config_type_t;

/**
 * \ingroup qos
 * Scheduled traffic gate operations
 * 802.1Q-2018 - Table 8.7 Gate operations
 */
typedef enum {
	GENAVB_ST_SET_GATE_STATES,    /**< SetGateStates */
	GENAVB_ST_SET_AND_HOLD_MAC,   /**< Set-And-Hold-MAC */
	GENAVB_ST_SET_AND_RELEASE_MAC /**< Set-And-Release-MAC */
} genavb_st_operations_t;

/**
 * \ingroup qos
 * Scheduled traffic gate control entry
 * 802.1Q-2018 - 12.29.1.2.1 GateControlEntry
 */
struct genavb_st_gate_control_entry {
	uint8_t operation;	/**< Table 8.7 Operation */
	uint8_t gate_states;	/**< 12.29.1.2.2 gateStatesValue */
	uint32_t time_interval; /**< 12.29.1.2.3 timeIntervalValue */
};

/**
 * \ingroup qos
 * Scheduled traffic configuration
 * 802.1Q-2018 - Table 12-29
 */
struct genavb_st_config {
	int enable;					   /**< GateEnabled */
	uint64_t base_time;				   /**< AdminBaseTime or OperBaseTime in nanoseconds */
	uint32_t cycle_time_p;				   /**< AdminCycleTime or OperCycleTime (numerator) in seconds */
	uint32_t cycle_time_q;				   /**< AdminCycleTime or OperCycleTime (denominator) in seconds */
	uint32_t cycle_time_ext;			   /**< AdminCycleTimeExtension or OperCycleTimeExtension in nanoseconds */
	unsigned int list_length;			   /**< AdminControlListLength or OperControlListLength */
	struct genavb_st_gate_control_entry *control_list; /**< AdminControlList or OperControlList */
};

/**
 * \ingroup qos
 * Scheduled traffic queue max SDU
 * 802.1Q-2018 - Table 12-29
 */
struct genavb_st_max_sdu {
	uint8_t traffic_class; /* port traffic class*/
	uint32_t queue_max_sdu; /* max SDU size*/
	uint64_t transmission_overrun; /* transmission overrun counter */
};

/**
 * \ingroup qos
 * Frame preemption admin status
 */
typedef enum {
	GENAVB_FP_ADMIN_STATUS_EXPRESS = 1,		/**< Express frame priority */
	GENAVB_FP_ADMIN_STATUS_PREEMPTABLE = 2		/**< Preemptable frame priority */
} genavb_fp_admin_status_t;

/**
 * \ingroup qos
 * Frame preemption hold request
 */
typedef enum {
	GENAVB_FP_HOLD_REQUEST_HOLD = 1,		/**< Hold request */
	GENAVB_FP_HOLD_REQUEST_RELEASE = 2		/**< Release request */
} genavb_fp_hold_request_t;

/**
 * \ingroup qos
 * Frame preemption 802.1Q configuration
 * 802.1Q-2018 - Table 12-30
 */
struct genavb_fp_config_802_1Q {
	genavb_fp_admin_status_t admin_status[QOS_PRIORITY_MAX];	/**< framePreemptionStatusTable, Read/Write, 802.1Q-2018, section 12.30.1.1 */
	uint32_t hold_advance;						/**< holdAdvance, Read Only, 802.1Q-2018, section 12.30.1.2 */
	uint32_t release_advance;					/**< releaseAdvance, Read Only, 802.1Q-2018, section 12.30.1.3 */
	int preemption_active;						/**< preemptionActive, Read Only, 802.1Q-2018, section 12.30.1.4 */
	genavb_fp_hold_request_t hold_request;				/**< holdRequest, Read Only, 802.1Q-2018, section 12.30.1.5 */
};

/**
 * \ingroup qos
 * Frame preemption support
 */
typedef enum {
	GENAVB_FP_SUPPORT_SUPPORTED,			/**< Preemption supported */
	GENAVB_FP_SUPPORT_NOT_SUPPORTED			/**< Preemption not supported */
} genavb_fp_support_t;

/**
 * \ingroup qos
 * Frame preemption status verify
 */
typedef enum {
	GENAVB_FP_STATUS_VERIFY_UNKNOWN,		/**< Unknown state */
	GENAVB_FP_STATUS_VERIFY_INITIAL,		/**< Initial state */
	GENAVB_FP_STATUS_VERIFY_VERIFYING,		/**< Verying state */
	GENAVB_FP_STATUS_VERIFY_SUCCEEDED,		/**< Succeeded state */
	GENAVB_FP_STATUS_VERIFY_FAILED,			/**< Failed state */
	GENAVB_FP_STATUS_VERIFY_DISABLED		/**< Disabled state */
} genavb_fp_status_verify_t;

/**
 * \ingroup qos
 * Frame preemption 802.3 configuration
 * 802.3br-2016 - Section 30.14
 */
struct genavb_fp_config_802_3 {
	uint32_t support;		/**< aMACMergeSupport, Read Only, 802.3br-2016, section 30.14.1.1 */
	uint32_t status_verify;		/**< aMACMergeStatusVerify, Read Only, 802.3br-2016, section 30.14.1.2 */
	uint32_t enable_tx;		/**< aMACMergeEnableTx, Read/Write, 802.3br-2016, section 30.14.1.3 */
	uint32_t verify_disable_tx;	/**< aMACMergeVerifyDisableTx, Read/Write, 802.3br-2016, section 30.14.1.4 */
	uint32_t status_tx;		/**< aMACMergeStatusTx, Read Only, 802.3br-2016, section 30.14.1.5 */
	uint32_t verify_time;		/**< aMACMergeVerifyTime, Read/Write, 802.3br-2016, section 30.14.1.6 */
	uint32_t add_frag_size;		/**< aMACMergeAddFragSize, Read/Write, 802.3br-2016, section 30.14.1.7 */
};

/**
 * \ingroup qos
 * Frame preemption configuration type
 */
typedef enum {
	GENAVB_FP_CONFIG_802_3,		/**< 802.3 configuration */
	GENAVB_FP_CONFIG_802_1Q		/**< 802.1Q configuration */
} genavb_fp_config_type_t;

/**
 * \ingroup qos
 * Frame preemption configuration
 */
struct genavb_fp_config {
	union {
		struct genavb_fp_config_802_1Q cfg_802_1Q;	/**< Only valid for configuration type ::GENAVB_FP_CONFIG_802_1Q */
		struct genavb_fp_config_802_3 cfg_802_3;	/**< Only valid for configuration type ::GENAVB_FP_CONFIG_802_3 */
	} u;
};

/* OS specific headers */
#include "os/qos.h"

#endif /* _GENAVB_PUBLIC_QOS_H_ */
