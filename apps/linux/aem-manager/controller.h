/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_

#define AEM_ENTITY_MODEL_ID				0x00049f0000080001 /* to be incremented by one upon change in the structure of entity model  - 17722_1-6.2.1.9 */


/* Entity config */
#define AEM_CFG_ENTITY_NAME				"Freescale AVB controller"
#define AEM_CFG_ENTITY_GROUP_NAME			"FSL demo"
#define AEM_CFG_ENTITY_SERIAL				"0000000000000001"
#define AEM_CFG_ENTITY_VENDOR_NAME			0
#define AEM_CFG_ENTITY_MODEL_NAME			1
#define AEM_CFG_ENTITY_FW_VERSION			"0.0.1"
#define AEM_CFG_ENTITY_CAPABILITIES			(ADP_ENTITY_AEM_SUPPORTED)  /* FIXME Needed or not for a controller? */
#define AEM_CFG_ENTITY_TALKER_CAPABILITIES		0
#define AEM_CFG_ENTITY_LISTENER_CAPABILITIES		0
#define AEM_CFG_ENTITY_CONTROLLER_CAPABILITIES		(ADP_CONTROLLER_IMPLEMENTED)
#define AEM_CFG_ENTITY_CURRENT_CONF			0


/* Configuration config */
#define AEM_CFG_CONFIG_NAME_0				"Unique configuration"
#define AEM_CFG_CONFIG_LOC_DESC_0			7

#define AEM_CFG_CONFIG_DESCRIPTORS {AEM_CFG_CONFIG_DESCRIPTOR(0)}


/* AVB interface config */
#define AEM_CFG_AVB_ITF_NAME_0			"AVB interface"
#define AEM_CFG_AVB_ITF_LOC_DESC_0		7
#define AEM_CFG_AVB_ITF_ITF_FLAGS_0		AEM_AVB_FLAGS_GPTP_SUPPORTED
#define AEM_CFG_AVB_ITF_CLOCK_ID_0		0
#define AEM_CFG_AVB_ITF_PRIO1_0			0xFF
#define AEM_CFG_AVB_ITF_CLOCK_CLASS_0		0xFF
#define AEM_CFG_AVB_ITF_OFF_SCALED_VAR_0	0
#define AEM_CFG_AVB_ITF_CLOCK_ACCURACY_0	0xFF
#define AEM_CFG_AVB_ITF_PRIO2_0			0xFF
#define AEM_CFG_AVB_ITF_DOMAIN_NB_0		0
#define AEM_CFG_AVB_ITF_LOG_SYN_INTER_0		0
#define AEM_CFG_AVB_ITF_LOG_ANN_INTER_0		0
#define AEM_CFG_AVB_ITF_POG_PDEL_INTER_0	0
#define AEM_CFG_AVB_ITF_PORT_NB_0		0


#define AEM_CFG_AVB_ITF_DESCRIPTORS {AEM_CFG_AVB_ITF_DESCRIPTOR(0)}


/* Locale config */
#define AEM_CFG_LOCALE_IDENTIFIER_0		"en"
#define AEM_CFG_LOCALE_NB_STRINGS_0		1
#define AEM_CFG_LOCALE_BASE_STRINGS_0		0

#define AEM_CFG_LOCALE_DESCRIPTORS {AEM_CFG_LOCALE_DESCRIPTOR(0)}
/* Strings config */
#define AEM_CFG_STRINGS_0_0			"Freescale AVB"
#define AEM_CFG_STRINGS_1_0			{}
#define AEM_CFG_STRINGS_2_0			{}
#define AEM_CFG_STRINGS_3_0			{}
#define AEM_CFG_STRINGS_4_0			{}
#define AEM_CFG_STRINGS_5_0			{}
#define AEM_CFG_STRINGS_6_0			{}

#define AEM_CFG_STRINGS_DESCRIPTORS {AEM_CFG_STRINGS_DESCRIPTOR(0)}


#include "genavb/aem_entity.h"

#endif /* _CONTROLLER_H_ */
