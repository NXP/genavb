/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2020, 2022, 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _TALKER_VIDEO_SINGLE_H_
#define _TALKER_VIDEO_SINGLE_H_

#define AEM_ENTITY_MODEL_ID				0x00049f0000040001 /* to be incremented by one upon change in the structure of entity model  - 17722_1-6.2.1.9 */


/* Entity config */
#define AEM_CFG_ENTITY_NAME				"NXP AVB talker"
#define AEM_CFG_ENTITY_GROUP_NAME			"NXP demo"
#define AEM_CFG_ENTITY_SERIAL				"0000000000000001"
#define AEM_CFG_ENTITY_VENDOR_NAME			0
#define AEM_CFG_ENTITY_MODEL_NAME			1
#define AEM_CFG_ENTITY_FW_VERSION			"0.0.1"
#define AEM_CFG_ENTITY_CAPABILITIES			(ADP_ENTITY_CLASS_A_SUPPORTED | ADP_ENTITY_CLASS_B_SUPPORTED | ADP_ENTITY_GPTP_SUPPORTED | ADP_ENTITY_AEM_SUPPORTED | ADP_ENTITY_ASSOCIATION_ID_SUPPORTED | ADP_ENTITY_ASSOCIATION_ID_VALID)
#define AEM_CFG_ENTITY_TALKER_CAPABILITIES		(ADP_TALKER_VIDEO_SOURCE | ADP_TALKER_MEDIA_CLOCK_SOURCE | ADP_TALKER_IMPLEMENTED)
#define AEM_CFG_ENTITY_LISTENER_CAPABILITIES		0
#define AEM_CFG_ENTITY_CONTROLLER_CAPABILITIES		0
#define AEM_CFG_ENTITY_CURRENT_CONF			0


/* Configuration config */
#define AEM_CFG_CONFIG_NAME_0				"Unique configuration"
#define AEM_CFG_CONFIG_LOC_DESC_0			7
#define AEM_CFG_CONFIG_CONTROL_COUNT_0			0


#define AEM_CFG_CONFIG_DESCRIPTORS {AEM_CFG_CONFIG_DESCRIPTOR(0)}


/* Video unit config */
#define AEM_CFG_VIDEO_UNIT_NAME_0			"Video unit"
#define AEM_CFG_VIDEO_UNIT_LOC_DESC_0			7
#define AEM_CFG_VIDEO_UNIT_CLK_DOMAIN_IDX_0		0
#define AEM_CFG_VIDEO_UNIT_NB_STREAM_IN_PORT_0		0
#define AEM_CFG_VIDEO_UNIT_BASE_STREAM_IN_PORT_0	0
#define AEM_CFG_VIDEO_UNIT_NB_STREAM_OUT_PORT_0		1
#define AEM_CFG_VIDEO_UNIT_BASE_STREAM_OUT_PORT_0	0
#define AEM_CFG_VIDEO_UNIT_NB_EXT_IN_PORT_0		1
#define AEM_CFG_VIDEO_UNIT_BASE_EXT_IN_PORT_0		0
#define AEM_CFG_VIDEO_UNIT_NB_EXT_OUT_PORT_0		0
#define AEM_CFG_VIDEO_UNIT_BASE_EXT_OUT_PORT_0		0
#define AEM_CFG_VIDEO_UNIT_NB_INT_IN_PORT_0		0
#define AEM_CFG_VIDEO_UNIT_BASE_INT_IN_PORT_0		0
#define AEM_CFG_VIDEO_UNIT_NB_INT_OUT_PORT_0		0
#define AEM_CFG_VIDEO_UNIT_BASE_INT_OUT_PORT_0		0
#define AEM_CFG_VIDEO_UNIT_NB_CONTROLS_0		3
#define AEM_CFG_VIDEO_UNIT_BASE_CONTROLS_0		0
#define AEM_CFG_VIDEO_UNIT_NB_SIGNAL_SEL_0		0
#define AEM_CFG_VIDEO_UNIT_BASE_SIGNAL_SEL_0		0
#define AEM_CFG_VIDEO_UNIT_NB_MIXERS_0			0
#define AEM_CFG_VIDEO_UNIT_BASE_MIXER_0			0
#define AEM_CFG_VIDEO_UNIT_NB_MATRICES_0		0
#define AEM_CFG_VIDEO_UNIT_BASE_MATRIX_0		0
#define AEM_CFG_VIDEO_UNIT_NB_SPLITTERS_0		0
#define AEM_CFG_VIDEO_UNIT_BASE_SPLITTER_0		0
#define AEM_CFG_VIDEO_UNIT_NB_COMBINERS_0		0
#define AEM_CFG_VIDEO_UNIT_BASE_COMBINER_0		0
#define AEM_CFG_VIDEO_UNIT_NB_MUX_0			0
#define AEM_CFG_VIDEO_UNIT_BASE_MUX_0			0
#define AEM_CFG_VIDEO_UNIT_NB_DEMUX_0			0
#define AEM_CFG_VIDEO_UNIT_BASE_DEMUX_0			0
#define AEM_CFG_VIDEO_UNIT_NB_TRANSCODERS_0		0
#define AEM_CFG_VIDEO_UNIT_BASE_TRANSCODERS_0		0
#define AEM_CFG_VIDEO_UNIT_NB_CONTROL_BLOCKS_0		0
#define AEM_CFG_VIDEO_UNIT_BASE_CONTROL_BLOCK_0		0


#define AEM_CFG_VIDEO_UNIT_DESCRIPTORS {AEM_CFG_VIDEO_UNIT_DESCRIPTOR(0)}


/* Stream output config */
#define AEM_CFG_STREAM_OUTPUT_NAME_0			"Stream output 0"
#define AEM_CFG_STREAM_OUTPUT_LOC_DESC_0		7
#define AEM_CFG_STREAM_OUTPUT_CLOCK_DOMAIN_IDX_0	0
#define AEM_CFG_STREAM_OUTPUT_STREAM_FLAGS_0		(AEM_STREAM_FLAG_CLASS_A | AEM_STREAM_FLAG_CLASS_B)
#define AEM_CFG_STREAM_OUTPUT_CURRENT_FORMAT_0		0x00C0000000000000
#define AEM_CFG_STREAM_OUTPUT_NB_FORMATS_0		1
#define AEM_CFG_STREAM_OUTPUT_BACKUP_TALKER_ENTITY_0_0	0
#define AEM_CFG_STREAM_OUTPUT_BACKUP_TALKER_UNIQUE_0_0	0
#define AEM_CFG_STREAM_OUTPUT_BACKUP_TALKER_ENTITY_1_0	0
#define AEM_CFG_STREAM_OUTPUT_BACKUP_TALKER_UNIQUE_1_0	0
#define AEM_CFG_STREAM_OUTPUT_BACKUP_TALKER_ENTITY_2_0	0
#define AEM_CFG_STREAM_OUTPUT_BACKUP_TALKER_UNIQUE_2_0	0
#define AEM_CFG_STREAM_OUTPUT_BACKEDUP_TALKER_ENTITY_0	0
#define AEM_CFG_STREAM_OUTPUT_BACKEDUP_TALKER_UNIQUE_0	0
#define AEM_CFG_STREAM_OUTPUT_AVB_ITF_INDEX_0		0
#define AEM_CFG_STREAM_OUTPUT_BUFFER_LENGTH_0		0
#define AEM_CFG_STREAM_OUTPUT_NB_REDUNDANT_STREAMS_0    0
#define AEM_CFG_STREAM_OUTPUT_FORMATS_0			{ htonll(0x00C0000000000000) }
#define AEM_CFG_STREAM_OUTPUT_REDUNDANT_STREAMS_0       { 0 }

#define AEM_CFG_STREAM_OUTPUT_DESCRIPTORS {AEM_CFG_STREAM_OUTPUT_DESCRIPTOR(0)}


/* Jack input config */
#define AEM_CFG_JACK_INPUT_NAME_0		"Jack input"
#define AEM_CFG_JACK_INPUT_LOC_DESC_0		7
#define AEM_CFG_JACK_INPUT_FLAGS_0		0
#define AEM_CFG_JACK_INPUT_TYPE_0		AEM_JACK_TYPE_UNBALANCED_ANALOG
#define AEM_CFG_JACK_INPUT_NUM_CTRL_0		0
#define AEM_CFG_JACK_INPUT_BASE_CTRL_0		0


#define AEM_CFG_JACK_INPUT_DESCRIPTORS {AEM_CFG_JACK_INPUT_DESCRIPTOR(0)}


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
#define AEM_CFG_CLK_SOURCE_NAME_0		"Clock source"
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


/* Stream port output config */
#define AEM_CFG_STREAM_PORT_OUT_CLK_DOM_IDX_0		0
#define AEM_CFG_STREAM_PORT_OUT_PORT_FLAGS_0		0
#define AEM_CFG_STREAM_PORT_OUT_NB_CONTROLS_0		0
#define AEM_CFG_STREAM_PORT_OUT_BASE_CONTROL_0		0
#define AEM_CFG_STREAM_PORT_OUT_NB_CLUSTERS_0		2
#define AEM_CFG_STREAM_PORT_OUT_BASE_CLUSTER_0		0
#define AEM_CFG_STREAM_PORT_OUT_NB_MAPS_0		1
#define AEM_CFG_STREAM_PORT_OUT_BASE_MAP_0		0


#define AEM_CFG_STREAM_PORT_OUT_DESCRIPTORS {AEM_CFG_STREAM_PORT_OUT_DESCRIPTOR(0)}


/* Video cluster config */
#define AEM_CFG_VIDEO_CLUSTER_NAME_0					"Video cluster 0"
#define AEM_CFG_VIDEO_CLUSTER_LOC_DESC_0				7
#define AEM_CFG_VIDEO_CLUSTER_SIGNAL_TYPE_0				AEM_DESC_TYPE_EXTERNAL_PORT_INPUT
#define AEM_CFG_VIDEO_CLUSTER_SIGNAL_IDX_0				0
#define AEM_CFG_VIDEO_CLUSTER_SIGNAL_OUTPUT_0				0
#define AEM_CFG_VIDEO_CLUSTER_PATH_LAT_0				0
#define AEM_CFG_VIDEO_CLUSTER_BLOCK_LAT_0				0
#define AEM_CFG_VIDEO_CLUSTER_FORMAT_0					AEM_VIDEO_CLUSTER_FORMAT_MPEG_PES
#define AEM_CFG_VIDEO_CLUSTER_CURRENT_FORMAT_SPECIFIC_0			0
#define AEM_CFG_VIDEO_CLUSTER_SUPPORTED_FORMAT_SPECIFICS_COUNT_0	1
#define AEM_CFG_VIDEO_CLUSTER_CURRENT_SAMPLING_RATE_0			0
#define AEM_CFG_VIDEO_CLUSTER_SUPPORTED_SAMPLING_RATES_COUNT_0		1
#define AEM_CFG_VIDEO_CLUSTER_CURRENT_ASPECT_RATIO_0			0
#define AEM_CFG_VIDEO_CLUSTER_SUPPORTED_ASPECT_RATIOS_COUNT_0		1
#define AEM_CFG_VIDEO_CLUSTER_CURRENT_SIZE_0				0
#define AEM_CFG_VIDEO_CLUSTER_SUPPORTED_SIZES_COUNT_0			1
#define AEM_CFG_VIDEO_CLUSTER_CURRENT_COLOR_SPACE_0			0
#define AEM_CFG_VIDEO_CLUSTER_SUPPORTED_COLOR_SPACES_COUNT_0		1
#define AEM_CFG_VIDEO_CLUSTER_SUPPORTED_FORMAT_SPECIFICS_0		{ htonl(0) }
#define AEM_CFG_VIDEO_CLUSTER_SUPPORTED_SAMPLING_RATES_0		{ htonl(0) }
#define AEM_CFG_VIDEO_CLUSTER_SUPPORTED_ASPECT_RATIOS_0			{ htons(0) }
#define AEM_CFG_VIDEO_CLUSTER_SUPPORTED_SIZES_0				{ htonl(0) }
#define AEM_CFG_VIDEO_CLUSTER_SUPPORTED_COLOR_SPACES_0			{ htons(0) }

#define AEM_CFG_VIDEO_CLUSTER_NAME_1					"Video cluster 1"
#define AEM_CFG_VIDEO_CLUSTER_LOC_DESC_1				7
#define AEM_CFG_VIDEO_CLUSTER_SIGNAL_TYPE_1				AEM_DESC_TYPE_EXTERNAL_PORT_INPUT
#define AEM_CFG_VIDEO_CLUSTER_SIGNAL_IDX_1				0
#define AEM_CFG_VIDEO_CLUSTER_SIGNAL_OUTPUT_1				0
#define AEM_CFG_VIDEO_CLUSTER_PATH_LAT_1				0
#define AEM_CFG_VIDEO_CLUSTER_BLOCK_LAT_1				0
#define AEM_CFG_VIDEO_CLUSTER_FORMAT_1					AEM_VIDEO_CLUSTER_FORMAT_MPEG_PES
#define AEM_CFG_VIDEO_CLUSTER_CURRENT_FORMAT_SPECIFIC_1			0
#define AEM_CFG_VIDEO_CLUSTER_SUPPORTED_FORMAT_SPECIFICS_COUNT_1	1
#define AEM_CFG_VIDEO_CLUSTER_CURRENT_SAMPLING_RATE_1			0
#define AEM_CFG_VIDEO_CLUSTER_SUPPORTED_SAMPLING_RATES_COUNT_1		1
#define AEM_CFG_VIDEO_CLUSTER_CURRENT_ASPECT_RATIO_1			0
#define AEM_CFG_VIDEO_CLUSTER_SUPPORTED_ASPECT_RATIOS_COUNT_1		1
#define AEM_CFG_VIDEO_CLUSTER_CURRENT_SIZE_1				0
#define AEM_CFG_VIDEO_CLUSTER_SUPPORTED_SIZES_COUNT_1			1
#define AEM_CFG_VIDEO_CLUSTER_CURRENT_COLOR_SPACE_1			0
#define AEM_CFG_VIDEO_CLUSTER_SUPPORTED_COLOR_SPACES_COUNT_1		1
#define AEM_CFG_VIDEO_CLUSTER_SUPPORTED_FORMAT_SPECIFICS_1		{ htonl(0) }
#define AEM_CFG_VIDEO_CLUSTER_SUPPORTED_SAMPLING_RATES_1		{ htonl(0) }
#define AEM_CFG_VIDEO_CLUSTER_SUPPORTED_ASPECT_RATIOS_1			{ htons(0) }
#define AEM_CFG_VIDEO_CLUSTER_SUPPORTED_SIZES_1				{ htonl(0) }
#define AEM_CFG_VIDEO_CLUSTER_SUPPORTED_COLOR_SPACES_1			{ htons(0) }

#define AEM_CFG_VIDEO_CLUSTER_DESCRIPTORS {AEM_CFG_VIDEO_CLUSTER_DESCRIPTOR(0),AEM_CFG_VIDEO_CLUSTER_DESCRIPTOR(1)}


/* Video map config */
#define AEM_CFG_VIDEO_MAP_NB_MAPPINGS_0		2
#define AEM_CFG_VIDEO_MAP_MAPPINGS_0 		{\
							{	.mapping_stream_index = htons(0x0000),\
								.mapping_program_stream = htons(0x0000),\
								.mapping_elementary_stream = htons(0x0000),\
								.mapping_cluster_offset = htons(0x0000),\
							},\
							{	.mapping_stream_index = htons(0x0000),\
								.mapping_program_stream = htons(0x0000),\
								.mapping_elementary_stream = htons(0x0001),\
								.mapping_cluster_offset = htons(0x0001),\
							}\
						}


#define AEM_CFG_VIDEO_MAP_DESCRIPTORS {AEM_CFG_VIDEO_MAP_DESCRIPTOR(0)}


/* External port input config */
#define AEM_CFG_EXT_PORT_INPUT_CLK_DOM_IDX_0	0
#define AEM_CFG_EXT_PORT_INPUT_PORT_FLAGS_0	0
#define AEM_CFG_EXT_PORT_INPUT_NB_CONTROL_0	0
#define AEM_CFG_EXT_PORT_INPUT_BASE_CONTROL_0	0
#define AEM_CFG_EXT_PORT_INPUT_SIGNAL_TYPE_0	AEM_DESC_TYPE_INVALID
#define AEM_CFG_EXT_PORT_INPUT_SIGNAL_IDX_0	0
#define AEM_CFG_EXT_PORT_INPUT_SIGNAL_OUTPUT_0	0
#define AEM_CFG_EXT_PORT_INPUT_BLOCK_LAT_0	100
#define AEM_CFG_EXT_PORT_INPUT_JACK_IDX_0	0


#define AEM_CFG_EXT_PORT_INPUT_DESCRIPTORS {AEM_CFG_EXT_PORT_INPUT_DESCRIPTOR(0)}

/* Control for volume, in percent */
#define AEM_CFG_CONTROL_NAME_0			"Volume Control"
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

/* Control for Enable/Disable, used for media play/stop */
#define AEM_CFG_CONTROL_NAME_1			"Play/Stop"
#define AEM_CFG_CONTROL_LOC_DESC_1		7
#define AEM_CFG_CONTROL_BLOCK_LAT_1		0
#define AEM_CFG_CONTROL_CTRL_LAT_1		200
#define AEM_CFG_CONTROL_DOMAIN_1		0
#define AEM_CFG_CONTROL_VALUE_TYPE_1		AEM_CONTROL_SET_VALUE_TYPE(0, 0, AEM_CONTROL_LINEAR_UINT8)
#define AEM_CFG_CONTROL_TYPE_1			AEM_CONTROL_TYPE_ENABLE
#define AEM_CFG_CONTROL_RESET_TIME_1		0
#define AEM_CFG_CONTROL_NB_VALUES_1		1
#define AEM_CFG_CONTROL_SIGNAL_TYPE_1		AEM_DESC_TYPE_AUDIO_CLUSTER
#define AEM_CFG_CONTROL_SIGNAL_INDEX_1		0
#define AEM_CFG_CONTROL_SIGNAL_OUTPUT_1		0
#define AEM_CFG_CONTROL_VALUE_DETAILS_1		{\
		.linear_int8 = {{0, 255, 255, 0, 0, htons(AEM_CONTROL_SET_UNIT_FORMAT(0, AEM_CONTROL_CODE_UNITLESS)), 0}}}

/* Control for media track number (0 to 255) */
#define AEM_CFG_CONTROL_NAME_2			"Media track"
#define AEM_CFG_CONTROL_LOC_DESC_2		7
#define AEM_CFG_CONTROL_BLOCK_LAT_2		0
#define AEM_CFG_CONTROL_CTRL_LAT_2		200
#define AEM_CFG_CONTROL_DOMAIN_2		0
#define AEM_CFG_CONTROL_VALUE_TYPE_2		AEM_CONTROL_SET_VALUE_TYPE(0, 0, AEM_CONTROL_LINEAR_UINT8)
#define AEM_CFG_CONTROL_TYPE_2			AEM_CONTROL_TYPE_MEDIA_TRACK
#define AEM_CFG_CONTROL_RESET_TIME_2		0
#define AEM_CFG_CONTROL_NB_VALUES_2		1
#define AEM_CFG_CONTROL_SIGNAL_TYPE_2		AEM_DESC_TYPE_AUDIO_CLUSTER
#define AEM_CFG_CONTROL_SIGNAL_INDEX_2		0
#define AEM_CFG_CONTROL_SIGNAL_OUTPUT_2		0
#define AEM_CFG_CONTROL_VALUE_DETAILS_2		{\
		.linear_int8 = {{0, 255, 1, 0, 0, htons(AEM_CONTROL_SET_UNIT_FORMAT(0, AEM_CONTROL_CODE_COUNT)), 0}}}


#define AEM_CFG_CONTROL_DESCRIPTORS {AEM_CFG_CONTROL_DESCRIPTOR(0), AEM_CFG_CONTROL_DESCRIPTOR(1), AEM_CFG_CONTROL_DESCRIPTOR(2)}


#include "genavb/aem_entity.h"

#endif /* _TALKER_VIDEO_SINGLE_H_ */
