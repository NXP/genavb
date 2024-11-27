/*
 * Copyright 2020, 2022-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief GenAVB/TSN public API
 \details 802.1Q/802.3 Frame Preemption definitions.

 \copyright Copyright 2020, 2022-2024 NXP
*/
#ifndef _GENAVB_PUBLIC_FRAME_PREEMPTION_H_
#define _GENAVB_PUBLIC_FRAME_PREEMPTION_H_

#include "qos.h"
#include "types.h"

/** \cond RTOS */

/**
 * \defgroup fp	            802.1Q/802.3 Frame Preemption
 * \ingroup library
 *
 * This API is used to configure the frame preemption feature of a given network port (as defined in IEEE 802.3br-2016 and IEEE 802.1Qbu-2016).
 * Frame preemption allows Express traffic to interrupt on-going Preemptable traffic. Preemption can be enabled or disabled at the port level.
 * Frame priorities can be configured as either Preemptable or Express.
 *
 * This feature requires specific hardware support and returns an error if the corresponding network port doesn't support it.
 *
 * The 802.1Q configuration is set using ::genavb_fp_set:
 * * port_id: the logical port ID.
 * * type: the ::genavb_fp_config_type_t configuration type. Must be set to ::GENAVB_FP_CONFIG_802_1Q.
 * * config: the ::genavb_fp_config configuration. Only the write members of the ::genavb_fp_config_802_1Q union should be set.
 *
 * Frame priorities mapped to the same Traffic class must have the same Preemptable/Express setting.
 *
 * The 802.1Q configuration is retrieved using ::genavb_fp_get:
 * * port_id: the logical port ID.
 * * type: the ::genavb_fp_config_type_t configuration type. Must be set to ::GENAVB_FP_CONFIG_802_1Q.
 * * config: the ::genavb_fp_config configuration. Only the read members of the ::genavb_fp_config_802_1Q union will be set when the function returns.
 *
 * The 802.3 configuration is set using ::genavb_fp_set:
 * * port_id: the logical port ID.
 * * type: the ::genavb_fp_config_type_t configuration type. Must be set to ::GENAVB_FP_CONFIG_802_3.
 * * config: the ::genavb_fp_config configuration. Only the write members of the ::genavb_fp_config_802_3 union should be set.
 *
 * If verification is enabled (verify_disable_tx == false), only after the verification process is complete successfully (status_verify == ::GENAVB_FP_STATUS_VERIFY_SUCCEEDED), is frame preemption enabled in the port.
 *
 * In the current release, Frame Preemption can not be enabled at the same time as Scheduled Traffic.
 *
 *
 * The 802.3 configuration is retrieved using ::genavb_fp_get:
 * * port_id: the logical port ID.
 * * type: the ::genavb_fp_config_type_t configuration type. Must be set to ::GENAVB_FP_CONFIG_802_3.
 * * config: the ::genavb_fp_config configuration. Only the read members of the ::genavb_fp_config_802_3 union will be set when the function returns.
 *
 */

/**
 * \ingroup fp
 * Frame preemption admin status
 */
typedef enum {
	GENAVB_FP_ADMIN_STATUS_EXPRESS = 1,		/**< Express frame priority */
	GENAVB_FP_ADMIN_STATUS_PREEMPTABLE = 2		/**< Preemptable frame priority */
} genavb_fp_admin_status_t;

/**
 * \ingroup fp
 * Frame preemption hold request
 */
typedef enum {
	GENAVB_FP_HOLD_REQUEST_HOLD = 1,		/**< Hold request */
	GENAVB_FP_HOLD_REQUEST_RELEASE = 2		/**< Release request */
} genavb_fp_hold_request_t;

/**
 * \ingroup fp
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
 * \ingroup fp
 * Frame preemption support
 */
typedef enum {
	GENAVB_FP_SUPPORT_SUPPORTED,			/**< Preemption supported */
	GENAVB_FP_SUPPORT_NOT_SUPPORTED			/**< Preemption not supported */
} genavb_fp_support_t;

/**
 * \ingroup fp
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
 * \ingroup fp
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
 * \ingroup fp
 * Frame preemption configuration type
 */
typedef enum {
	GENAVB_FP_CONFIG_802_3,		/**< 802.3 configuration */
	GENAVB_FP_CONFIG_802_1Q		/**< 802.1Q configuration */
} genavb_fp_config_type_t;

/**
 * \ingroup fp
 * Frame preemption configuration
 */
struct genavb_fp_config {
	union {
		struct genavb_fp_config_802_1Q cfg_802_1Q;	/**< Only valid for configuration type ::GENAVB_FP_CONFIG_802_1Q */
		struct genavb_fp_config_802_3 cfg_802_3;	/**< Only valid for configuration type ::GENAVB_FP_CONFIG_802_3 */
	} u;
};

/** \endcond */

/* OS specific headers */
#include "os/frame_preemption.h"

#endif /* _GENAVB_PUBLIC_FRAME_PREEMPTION_H_ */
