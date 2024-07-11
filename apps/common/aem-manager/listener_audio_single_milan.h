/*
 * Copyright 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2020-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LISTENER_AUDIO_SINGLE_MILAN_H_
#define _LISTENER_AUDIO_SINGLE_MILAN_H_

#define AEM_ENTITY_MODEL_ID				0x00049f00000c0001 /* to be incremented by one upon change in the structure of entity model  - 17722_1-6.2.1.9 */


/* Entity config */
#define AEM_CFG_ENTITY_NAME				"NXP AVB Milan Listener"
#define AEM_CFG_ENTITY_GROUP_NAME			"NXP demo"
#define AEM_CFG_ENTITY_SERIAL				"0000000000000001"
#define AEM_CFG_ENTITY_VENDOR_NAME			0
#define AEM_CFG_ENTITY_MODEL_NAME			1
#define AEM_CFG_ENTITY_FW_VERSION			"0.0.1"
#define AEM_CFG_ENTITY_CAPABILITIES                     (ADP_ENTITY_CLASS_A_SUPPORTED | ADP_ENTITY_GPTP_SUPPORTED | ADP_ENTITY_AEM_SUPPORTED | ADP_ENTITY_VENDOR_UNIQUE_SUPPORTED /*| ADP_ENTITY_AEM_IDENTIFY_CONTROL_INDEX_VALID*/ | ADP_ENTITY_AEM_INTERFACE_INDEX_VALID)
#define AEM_CFG_ENTITY_TALKER_CAPABILITIES		0
#define AEM_CFG_ENTITY_LISTENER_CAPABILITIES		(ADP_LISTENER_AUDIO_SINK | ADP_LISTENER_MEDIA_CLOCK_SINK | ADP_LISTENER_IMPLEMENTED)
#define AEM_CFG_ENTITY_CONTROLLER_CAPABILITIES		0
#define AEM_CFG_ENTITY_CURRENT_CONF			0


/* Configuration config */
#define AEM_CFG_CONFIG_NAME_0				"Unique configuration"
#define AEM_CFG_CONFIG_LOC_DESC_0			7
#define AEM_CFG_CONFIG_CONTROL_COUNT_0			0


#define AEM_CFG_CONFIG_DESCRIPTORS {AEM_CFG_CONFIG_DESCRIPTOR(0)}


/* Audio unit config */
#define AEM_CFG_AUDIO_UNIT_NAME_0			"Audio unit"
#define AEM_CFG_AUDIO_UNIT_LOC_DESC_0			7
#define AEM_CFG_AUDIO_UNIT_CLK_DOMAIN_IDX_0		0
#define AEM_CFG_AUDIO_UNIT_NB_STREAM_IN_PORT_0		1
#define AEM_CFG_AUDIO_UNIT_BASE_STREAM_IN_PORT_0	0
#define AEM_CFG_AUDIO_UNIT_NB_STREAM_OUT_PORT_0		0
#define AEM_CFG_AUDIO_UNIT_BASE_STREAM_OUT_PORT_0	0
#define AEM_CFG_AUDIO_UNIT_NB_EXT_IN_PORT_0		0
#define AEM_CFG_AUDIO_UNIT_BASE_EXT_IN_PORT_0		0
#define AEM_CFG_AUDIO_UNIT_NB_EXT_OUT_PORT_0		2
#define AEM_CFG_AUDIO_UNIT_BASE_EXT_OUT_PORT_0		0
#define AEM_CFG_AUDIO_UNIT_NB_INT_IN_PORT_0		0
#define AEM_CFG_AUDIO_UNIT_BASE_INT_IN_PORT_0		0
#define AEM_CFG_AUDIO_UNIT_NB_INT_OUT_PORT_0		0
#define AEM_CFG_AUDIO_UNIT_BASE_INT_OUT_PORT_0		0
#define AEM_CFG_AUDIO_UNIT_NB_CONTROLS_0		2
#define AEM_CFG_AUDIO_UNIT_BASE_CONTROLS_0		0
#define AEM_CFG_AUDIO_UNIT_NB_SIGNAL_SEL_0		0
#define AEM_CFG_AUDIO_UNIT_BASE_SIGNAL_SEL_0		0
#define AEM_CFG_AUDIO_UNIT_NB_MIXERS_0			0
#define AEM_CFG_AUDIO_UNIT_BASE_MIXER_0			0
#define AEM_CFG_AUDIO_UNIT_NB_MATRICES_0		0
#define AEM_CFG_AUDIO_UNIT_BASE_MATRIX_0		0
#define AEM_CFG_AUDIO_UNIT_NB_SPLITTERS_0		0
#define AEM_CFG_AUDIO_UNIT_BASE_SPLITTER_0		0
#define AEM_CFG_AUDIO_UNIT_NB_COMBINERS_0		0
#define AEM_CFG_AUDIO_UNIT_BASE_COMBINER_0		0
#define AEM_CFG_AUDIO_UNIT_NB_MUX_0			0
#define AEM_CFG_AUDIO_UNIT_BASE_MUX_0			0
#define AEM_CFG_AUDIO_UNIT_NB_DEMUX_0			0
#define AEM_CFG_AUDIO_UNIT_BASE_DEMUX_0			0
#define AEM_CFG_AUDIO_UNIT_NB_TRANSCODERS_0		0
#define AEM_CFG_AUDIO_UNIT_BASE_TRANSCODERS_0		0
#define AEM_CFG_AUDIO_UNIT_NB_CONTROL_BLOCKS_0		0
#define AEM_CFG_AUDIO_UNIT_BASE_CONTROL_BLOCK_0		0
#define AEM_CFG_AUDIO_UNIT_CUR_SAMPLING_RATE_0		48000
#define AEM_CFG_AUDIO_UNIT_SAMP_RATES_COUNT_0		1
#define AEM_CFG_AUDIO_UNIT_SAMP_RATES_0			{ htonl(48000) }


#define AEM_CFG_AUDIO_UNIT_DESCRIPTORS {AEM_CFG_AUDIO_UNIT_DESCRIPTOR(0)}


/* Stream input config */
#define AEM_CFG_STREAM_INPUT_NAME_0			"Stream input 0"
#define AEM_CFG_STREAM_INPUT_LOC_DESC_0			7
#define AEM_CFG_STREAM_INPUT_CLOCK_DOMAIN_IDX_0		0
#define AEM_CFG_STREAM_INPUT_STREAM_FLAGS_0		(AEM_STREAM_FLAG_CLASS_A)
#define AEM_CFG_STREAM_INPUT_CURRENT_FORMAT_0		0x0205022000806000 //7.3.2 AAF 2chans 32/32bits 48kHz 6samples/packet
#define AEM_CFG_STREAM_INPUT_NB_FORMATS_0		1
#define AEM_CFG_STREAM_INPUT_BACKUP_TALKER_ENTITY_0_0	0
#define AEM_CFG_STREAM_INPUT_BACKUP_TALKER_UNIQUE_0_0	0
#define AEM_CFG_STREAM_INPUT_BACKUP_TALKER_ENTITY_1_0	0
#define AEM_CFG_STREAM_INPUT_BACKUP_TALKER_UNIQUE_1_0	0
#define AEM_CFG_STREAM_INPUT_BACKUP_TALKER_ENTITY_2_0	0
#define AEM_CFG_STREAM_INPUT_BACKUP_TALKER_UNIQUE_2_0	0
#define AEM_CFG_STREAM_INPUT_BACKEDUP_TALKER_ENTITY_0	0
#define AEM_CFG_STREAM_INPUT_BACKEDUP_TALKER_UNIQUE_0	0
#define AEM_CFG_STREAM_INPUT_AVB_ITF_INDEX_0		0
#define AEM_CFG_STREAM_INPUT_BUFFER_LENGTH_0		4000000 //32 buffers as socket queue size
#define AEM_CFG_STREAM_INPUT_FORMATS_0			{ htonll(AEM_CFG_STREAM_INPUT_CURRENT_FORMAT_0) }


#define AEM_CFG_STREAM_INPUT_DESCRIPTORS {AEM_CFG_STREAM_INPUT_DESCRIPTOR(0)}


/* Jack output config */
#define AEM_CFG_JACK_OUTPUT_NAME_0		"Jack output"
#define AEM_CFG_JACK_OUTPUT_LOC_DESC_0		7
#define AEM_CFG_JACK_OUTPUT_FLAGS_0		0
#define AEM_CFG_JACK_OUTPUT_TYPE_0		AEM_JACK_TYPE_UNBALANCED_ANALOG
#define AEM_CFG_JACK_OUTPUT_NUM_CTRL_0		0
#define AEM_CFG_JACK_OUTPUT_BASE_CTRL_0		0


#define AEM_CFG_JACK_OUTPUT_DESCRIPTORS {AEM_CFG_JACK_OUTPUT_DESCRIPTOR(0)}


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


/* Clock source config */
#define AEM_CFG_CLK_SOURCE_NAME_0		"Clock source 0"
#define AEM_CFG_CLK_SOURCE_LOC_DESC_0		7
#define AEM_CFG_CLK_SOURCE_FLAGS_0		AEM_CLOCK_SOURCE_FLAGS_LOCAL_ID
#define AEM_CFG_CLK_SOURCE_TYPE_0		AEM_CLOCK_SOURCE_TYPE_INPUT_STREAM
#define AEM_CFG_CLK_SOURCE_ID_0			0
#define AEM_CFG_CLK_SOURCE_LOC_TYPE_0		AEM_DESC_TYPE_STREAM_INPUT
#define AEM_CFG_CLK_SOURCE_LOC_INDEX_0		0


#define AEM_CFG_CLK_SOURCE_DESCRIPTORS {AEM_CFG_CLK_SOURCE_DESCRIPTOR(0)}

/* Locale config */
#define AEM_CFG_LOCALE_IDENTIFIER_0		"en"
#define AEM_CFG_LOCALE_NB_STRINGS_0		1
#define AEM_CFG_LOCALE_BASE_STRINGS_0		0


#define AEM_CFG_LOCALE_DESCRIPTORS {AEM_CFG_LOCALE_DESCRIPTOR(0)}

/* Strings config */
#define AEM_CFG_STRINGS_0_0			"NXP AVB"
#define AEM_CFG_STRINGS_1_0			{}
#define AEM_CFG_STRINGS_2_0			{}
#define AEM_CFG_STRINGS_3_0			{}
#define AEM_CFG_STRINGS_4_0			{}
#define AEM_CFG_STRINGS_5_0			{}
#define AEM_CFG_STRINGS_6_0			{}

#define AEM_CFG_STRINGS_DESCRIPTORS {AEM_CFG_STRINGS_DESCRIPTOR(0)}

/* Clock domain config */
#define AEM_CFG_CLK_DOMAIN_NAME_0		"Clock domain"
#define AEM_CFG_CLK_DOMAIN_LOC_DESC_0		7
#define AEM_CFG_CLK_DOMAIN_SOURCE_IDX_0		0
#define AEM_CFG_CLK_DOMAIN_SOURCES_COUNT_0	1
#define AEM_CFG_CLK_DOMAIN_SOURCES_0		{htons(0)}

#define AEM_CFG_CLK_DOMAIN_DESCRIPTORS {AEM_CFG_CLK_DOMAIN_DESCRIPTOR(0)}

/* Stream port input config */
#define AEM_CFG_STREAM_PORT_IN_CLK_DOM_IDX_0		0
#define AEM_CFG_STREAM_PORT_IN_PORT_FLAGS_0		0
#define AEM_CFG_STREAM_PORT_IN_NB_CONTROLS_0		0
#define AEM_CFG_STREAM_PORT_IN_BASE_CONTROL_0		0
#define AEM_CFG_STREAM_PORT_IN_NB_CLUSTERS_0		2
#define AEM_CFG_STREAM_PORT_IN_BASE_CLUSTER_0		0
#define AEM_CFG_STREAM_PORT_IN_NB_MAPS_0		0
#define AEM_CFG_STREAM_PORT_IN_BASE_MAP_0		0


#define AEM_CFG_STREAM_PORT_IN_DESCRIPTORS {AEM_CFG_STREAM_PORT_IN_DESCRIPTOR(0)}


/* Audio cluster config */
#define AEM_CFG_AUDIO_CLUSTER_NAME_0			"Audio cluster 0"
#define AEM_CFG_AUDIO_CLUSTER_LOC_DESC_0		7
#define AEM_CFG_AUDIO_CLUSTER_SIGNAL_TYPE_0		AEM_DESC_TYPE_INVALID
#define AEM_CFG_AUDIO_CLUSTER_SIGNAL_IDX_0		0
#define AEM_CFG_AUDIO_CLUSTER_SIGNAL_OUTPUT_0		0
#define AEM_CFG_AUDIO_CLUSTER_PATH_LAT_0		1000000
#define AEM_CFG_AUDIO_CLUSTER_BLOCK_LAT_0		0
#define AEM_CFG_AUDIO_CLUSTER_CHAN_COUNT_0		1
#define AEM_CFG_AUDIO_CLUSTER_FORMAT_0			AEM_AUDIO_CLUSTER_FORMAT_MBLA

#define AEM_CFG_AUDIO_CLUSTER_NAME_1			"Audio cluster 1"
#define AEM_CFG_AUDIO_CLUSTER_LOC_DESC_1		7
#define AEM_CFG_AUDIO_CLUSTER_SIGNAL_TYPE_1		AEM_DESC_TYPE_INVALID
#define AEM_CFG_AUDIO_CLUSTER_SIGNAL_IDX_1		0
#define AEM_CFG_AUDIO_CLUSTER_SIGNAL_OUTPUT_1		0
#define AEM_CFG_AUDIO_CLUSTER_PATH_LAT_1		1000000
#define AEM_CFG_AUDIO_CLUSTER_BLOCK_LAT_1		0
#define AEM_CFG_AUDIO_CLUSTER_CHAN_COUNT_1		1
#define AEM_CFG_AUDIO_CLUSTER_FORMAT_1			AEM_AUDIO_CLUSTER_FORMAT_MBLA

#define AEM_CFG_AUDIO_CLUSTER_DESCRIPTORS {AEM_CFG_AUDIO_CLUSTER_DESCRIPTOR(0), AEM_CFG_AUDIO_CLUSTER_DESCRIPTOR(1)}

/* External port output config */
#define AEM_CFG_EXT_PORT_OUTPUT_CLK_DOM_IDX_0	0
#define AEM_CFG_EXT_PORT_OUTPUT_PORT_FLAGS_0	0
#define AEM_CFG_EXT_PORT_OUTPUT_NB_CONTROL_0	0
#define AEM_CFG_EXT_PORT_OUTPUT_BASE_CONTROL_0	0
#define AEM_CFG_EXT_PORT_OUTPUT_SIGNAL_TYPE_0	AEM_DESC_TYPE_CONTROL
#define AEM_CFG_EXT_PORT_OUTPUT_SIGNAL_IDX_0	0
#define AEM_CFG_EXT_PORT_OUTPUT_SIGNAL_OUTPUT_0	0
#define AEM_CFG_EXT_PORT_OUTPUT_BLOCK_LAT_0	100
#define AEM_CFG_EXT_PORT_OUTPUT_JACK_IDX_0	0

#define AEM_CFG_EXT_PORT_OUTPUT_CLK_DOM_IDX_1	0
#define AEM_CFG_EXT_PORT_OUTPUT_PORT_FLAGS_1	0
#define AEM_CFG_EXT_PORT_OUTPUT_NB_CONTROL_1	0
#define AEM_CFG_EXT_PORT_OUTPUT_BASE_CONTROL_1	0
#define AEM_CFG_EXT_PORT_OUTPUT_SIGNAL_TYPE_1	AEM_DESC_TYPE_CONTROL
#define AEM_CFG_EXT_PORT_OUTPUT_SIGNAL_IDX_1	1
#define AEM_CFG_EXT_PORT_OUTPUT_SIGNAL_OUTPUT_1	0
#define AEM_CFG_EXT_PORT_OUTPUT_BLOCK_LAT_1	100
#define AEM_CFG_EXT_PORT_OUTPUT_JACK_IDX_1	0

#define AEM_CFG_EXT_PORT_OUTPUT_DESCRIPTORS {AEM_CFG_EXT_PORT_OUTPUT_DESCRIPTOR(0), AEM_CFG_EXT_PORT_OUTPUT_DESCRIPTOR(1)}


#define AEM_CFG_CONTROL_NAME_0			"Volume Control Channel 0"
#define AEM_CFG_CONTROL_LOC_DESC_0		7
#define AEM_CFG_CONTROL_BLOCK_LAT_0		0
#define AEM_CFG_CONTROL_CTRL_LAT_0		200
#define AEM_CFG_CONTROL_DOMAIN_0		0
#define AEM_CFG_CONTROL_VALUE_TYPE_0		AEM_CONTROL_SET_VALUE_TYPE(0, 0, AEM_CONTROL_LINEAR_UINT8)
#define AEM_CFG_CONTROL_TYPE_0			AEM_CONTROL_TYPE_GAIN
#define AEM_CFG_CONTROL_RESET_TIME_0		0
#define AEM_CFG_CONTROL_NB_VALUES_0		1
#define AEM_CFG_CONTROL_SIGNAL_TYPE_0		AEM_DESC_TYPE_AUDIO_CLUSTER
#define AEM_CFG_CONTROL_SIGNAL_INDEX_0		0
#define AEM_CFG_CONTROL_SIGNAL_OUTPUT_0		0
#define AEM_CFG_CONTROL_VALUE_DETAILS_0		{\
		.linear_int8 = {{0, 100, 1, 50, 100, htons(AEM_CONTROL_SET_UNIT_FORMAT(0, AEM_CONTROL_CODE_PERCENT)), 0}}}

#define AEM_CFG_CONTROL_NAME_1			"Volume Control Channel 1"
#define AEM_CFG_CONTROL_LOC_DESC_1		7
#define AEM_CFG_CONTROL_BLOCK_LAT_1		0
#define AEM_CFG_CONTROL_CTRL_LAT_1		200
#define AEM_CFG_CONTROL_DOMAIN_1		0
#define AEM_CFG_CONTROL_VALUE_TYPE_1		AEM_CONTROL_SET_VALUE_TYPE(0, 0, AEM_CONTROL_LINEAR_UINT8)
#define AEM_CFG_CONTROL_TYPE_1			AEM_CONTROL_TYPE_GAIN
#define AEM_CFG_CONTROL_RESET_TIME_1		0
#define AEM_CFG_CONTROL_NB_VALUES_1		1
#define AEM_CFG_CONTROL_SIGNAL_TYPE_1		AEM_DESC_TYPE_AUDIO_CLUSTER
#define AEM_CFG_CONTROL_SIGNAL_INDEX_1		1
#define AEM_CFG_CONTROL_SIGNAL_OUTPUT_1		0
#define AEM_CFG_CONTROL_VALUE_DETAILS_1		{\
		.linear_int8 = {{0, 100, 1, 50, 100, htons(AEM_CONTROL_SET_UNIT_FORMAT(0, AEM_CONTROL_CODE_PERCENT)), 0}}}


#define AEM_CFG_CONTROL_DESCRIPTORS {AEM_CFG_CONTROL_DESCRIPTOR(0), AEM_CFG_CONTROL_DESCRIPTOR(1)}

#include "genavb/aem_entity.h"

#endif /* _LISTENER_AUDIO_SINGLE_MILAN_H_ */
