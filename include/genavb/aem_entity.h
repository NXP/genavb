/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2022-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _GENAVB_PUBLIC_AEM_ENTITY_H_
#define _GENAVB_PUBLIC_AEM_ENTITY_H_

#include "net_types.h"

#ifndef AEM_CFG_AUDIO_UNIT_DESCRIPTORS
#define AEM_CFG_AUDIO_UNIT_DESCRIPTORS {}
#endif

#ifndef AEM_CFG_VIDEO_UNIT_DESCRIPTORS
#define AEM_CFG_VIDEO_UNIT_DESCRIPTORS {}
#endif

#ifndef AEM_CFG_STREAM_INPUT_DESCRIPTORS
#define AEM_CFG_STREAM_INPUT_DESCRIPTORS {}
#endif

#ifndef AEM_CFG_STREAM_OUTPUT_DESCRIPTORS
#define AEM_CFG_STREAM_OUTPUT_DESCRIPTORS {}
#endif

#ifndef AEM_CFG_JACK_INPUT_DESCRIPTORS
#define AEM_CFG_JACK_INPUT_DESCRIPTORS {}
#endif

#ifndef AEM_CFG_JACK_OUTPUT_DESCRIPTORS
#define AEM_CFG_JACK_OUTPUT_DESCRIPTORS {}
#endif

#ifndef AEM_CFG_AVB_ITF_DESCRIPTORS
#define AEM_CFG_AVB_ITF_DESCRIPTORS {}
#endif

#ifndef AEM_CFG_CLK_SOURCE_DESCRIPTORS
#define AEM_CFG_CLK_SOURCE_DESCRIPTORS {}
#endif

#ifndef AEM_CFG_CLK_DOMAIN_DESCRIPTORS
#define AEM_CFG_CLK_DOMAIN_DESCRIPTORS {}
#endif

#ifndef AEM_CFG_STREAM_PORT_IN_DESCRIPTORS
#define AEM_CFG_STREAM_PORT_IN_DESCRIPTORS {}
#endif

#ifndef AEM_CFG_STREAM_PORT_OUT_DESCRIPTORS
#define AEM_CFG_STREAM_PORT_OUT_DESCRIPTORS {}
#endif

#ifndef AEM_CFG_EXT_PORT_INPUT_DESCRIPTORS
#define AEM_CFG_EXT_PORT_INPUT_DESCRIPTORS {}
#endif

#ifndef AEM_CFG_EXT_PORT_OUTPUT_DESCRIPTORS
#define AEM_CFG_EXT_PORT_OUTPUT_DESCRIPTORS {}
#endif


#ifndef AEM_CFG_AUDIO_CLUSTER_DESCRIPTORS
#define AEM_CFG_AUDIO_CLUSTER_DESCRIPTORS {}
#endif

#ifndef AEM_CFG_VIDEO_CLUSTER_DESCRIPTORS
#define AEM_CFG_VIDEO_CLUSTER_DESCRIPTORS {}
#endif

#ifndef AEM_CFG_AUDIO_MAP_DESCRIPTORS
#define AEM_CFG_AUDIO_MAP_DESCRIPTORS {}
#endif

#ifndef AEM_CFG_VIDEO_MAP_DESCRIPTORS
#define AEM_CFG_VIDEO_MAP_DESCRIPTORS {}
#endif

#ifndef AEM_CFG_CONTROL_DESCRIPTORS
#define AEM_CFG_CONTROL_DESCRIPTORS {}
#endif

/* Entity config */
#define AEM_CFG_ENTITY_DESCRIPTOR		{\
							.descriptor_type = htons(AEM_DESC_TYPE_ENTITY),\
							.descriptor_index = 0,\
							.entity_id = 0,\
							.entity_model_id = htonll(AEM_ENTITY_MODEL_ID),\
							.entity_capabilities = htonl(AEM_CFG_ENTITY_CAPABILITIES),\
							.talker_capabilities = htons(AEM_CFG_ENTITY_TALKER_CAPABILITIES),\
							.listener_capabilities = htons(AEM_CFG_ENTITY_LISTENER_CAPABILITIES),\
							.controller_capabilities = htonl(AEM_CFG_ENTITY_CONTROLLER_CAPABILITIES),\
							.available_index = 0,\
							.association_id = 0,\
							.entity_name = AEM_CFG_ENTITY_NAME,\
							.vendor_name_string = htons(AEM_CFG_ENTITY_VENDOR_NAME),\
							.model_name_string = htons(AEM_CFG_ENTITY_MODEL_NAME),\
							.firmware_version = AEM_CFG_ENTITY_FW_VERSION,\
							.group_name = AEM_CFG_ENTITY_GROUP_NAME,\
							.serial_number = AEM_CFG_ENTITY_SERIAL,\
							.current_configuration = htons(AEM_CFG_ENTITY_CURRENT_CONF)\
						}


/* Configuration config */
#define AEM_CFG_CONFIG_DESCRIPTOR(idx)		{\
							.descriptor_type = htons(AEM_DESC_TYPE_CONFIGURATION),\
							.descriptor_index = htons(idx),\
							.object_name = AEM_CFG_CONFIG_NAME_##idx,\
							.localized_description = htons(AEM_CFG_CONFIG_LOC_DESC_##idx),\
							.descriptor_counts_offset = htons(AEM_DESCRIPTOR_COUNTS_OFFSET),\
							.descriptors_counts[0] = htons(AEM_DESC_TYPE_CONTROL),\
							.descriptors_counts[1] = htons(AEM_CFG_CONFIG_CONTROL_COUNT_##idx),\
						}


/* Audio unit config */
#define AEM_CFG_AUDIO_UNIT_DESCRIPTOR(idx)	{\
							.descriptor_type = htons(AEM_DESC_TYPE_AUDIO_UNIT),\
							.descriptor_index = htons(idx),\
							.object_name = AEM_CFG_AUDIO_UNIT_NAME_##idx,\
							.localized_description = htons(AEM_CFG_AUDIO_UNIT_LOC_DESC_##idx),\
							.clock_domain_index = htons(AEM_CFG_AUDIO_UNIT_CLK_DOMAIN_IDX_##idx),\
							.number_of_stream_input_ports = htons(AEM_CFG_AUDIO_UNIT_NB_STREAM_IN_PORT_##idx),\
							.base_stream_input_port = htons(AEM_CFG_AUDIO_UNIT_BASE_STREAM_IN_PORT_##idx),\
							.number_of_stream_output_ports = htons(AEM_CFG_AUDIO_UNIT_NB_STREAM_OUT_PORT_##idx),\
							.base_stream_output_port = htons(AEM_CFG_AUDIO_UNIT_BASE_STREAM_OUT_PORT_##idx),\
							.number_of_external_input_ports = htons(AEM_CFG_AUDIO_UNIT_NB_EXT_IN_PORT_##idx),\
							.base_external_input_port = htons(AEM_CFG_AUDIO_UNIT_BASE_EXT_IN_PORT_##idx),\
							.number_of_external_output_ports = htons(AEM_CFG_AUDIO_UNIT_NB_EXT_OUT_PORT_##idx),\
							.base_external_output_port = htons(AEM_CFG_AUDIO_UNIT_BASE_EXT_OUT_PORT_##idx),\
							.number_of_internal_input_ports = htons(AEM_CFG_AUDIO_UNIT_NB_INT_IN_PORT_##idx),\
							.base_internal_input_port = htons(AEM_CFG_AUDIO_UNIT_BASE_INT_IN_PORT_##idx),\
							.number_of_internal_output_ports = htons(AEM_CFG_AUDIO_UNIT_NB_INT_OUT_PORT_##idx),\
							.base_internal_output_port = htons(AEM_CFG_AUDIO_UNIT_BASE_INT_OUT_PORT_##idx),\
							.number_of_controls = htons(AEM_CFG_AUDIO_UNIT_NB_CONTROLS_##idx),\
							.base_control = htons(AEM_CFG_AUDIO_UNIT_BASE_CONTROLS_##idx),\
							.number_of_signal_selectors = htons(AEM_CFG_AUDIO_UNIT_NB_SIGNAL_SEL_##idx),\
							.base_signal_selector = htons(AEM_CFG_AUDIO_UNIT_BASE_SIGNAL_SEL_##idx),\
							.number_of_mixers = htons(AEM_CFG_AUDIO_UNIT_NB_MIXERS_##idx),\
							.base_mixer = htons(AEM_CFG_AUDIO_UNIT_BASE_MIXER_##idx),\
							.number_of_matrices = htons(AEM_CFG_AUDIO_UNIT_NB_MATRICES_##idx),\
							.base_matrix = htons(AEM_CFG_AUDIO_UNIT_BASE_MATRIX_##idx),\
							.number_of_splitters = htons(AEM_CFG_AUDIO_UNIT_NB_SPLITTERS_##idx),\
							.base_splitter = htons(AEM_CFG_AUDIO_UNIT_BASE_SPLITTER_##idx),\
							.number_of_combiners = htons(AEM_CFG_AUDIO_UNIT_NB_COMBINERS_##idx),\
							.base_combiner = htons(AEM_CFG_AUDIO_UNIT_BASE_COMBINER_##idx),\
							.number_of_demultiplexers = htons(AEM_CFG_AUDIO_UNIT_NB_DEMUX_##idx),\
							.base_demultiplexer = htons(AEM_CFG_AUDIO_UNIT_BASE_DEMUX_##idx),\
							.number_of_multiplexers = htons(AEM_CFG_AUDIO_UNIT_NB_MUX_##idx),\
							.base_multiplexer = htons(AEM_CFG_AUDIO_UNIT_BASE_MUX_##idx),\
							.number_of_transcoders = htons(AEM_CFG_AUDIO_UNIT_NB_TRANSCODERS_##idx),\
							.base_transcoder = htons(AEM_CFG_AUDIO_UNIT_BASE_TRANSCODERS_##idx),\
							.number_of_control_blocks = htons(AEM_CFG_AUDIO_UNIT_NB_CONTROL_BLOCKS_##idx),\
							.base_control_block = htons(AEM_CFG_AUDIO_UNIT_BASE_CONTROL_BLOCK_##idx),\
							.current_sampling_rate = htonl(AEM_CFG_AUDIO_UNIT_CUR_SAMPLING_RATE_##idx),\
							.sampling_rates_offset = htons(AEM_SAMPLING_RATES_OFFSET),\
							.sampling_rates_count = htons(AEM_CFG_AUDIO_UNIT_SAMP_RATES_COUNT_##idx),\
							.sampling_rates = AEM_CFG_AUDIO_UNIT_SAMP_RATES_##idx\
						}

/* Video unit config */
#define AEM_CFG_VIDEO_UNIT_DESCRIPTOR(idx)	{\
							.descriptor_type = htons(AEM_DESC_TYPE_VIDEO_UNIT),\
							.descriptor_index = htons(idx),\
							.object_name = AEM_CFG_VIDEO_UNIT_NAME_##idx,\
							.localized_description = htons(AEM_CFG_VIDEO_UNIT_LOC_DESC_##idx),\
							.clock_domain_index = htons(AEM_CFG_VIDEO_UNIT_CLK_DOMAIN_IDX_##idx),\
							.number_of_stream_input_ports = htons(AEM_CFG_VIDEO_UNIT_NB_STREAM_IN_PORT_##idx),\
							.base_stream_input_port = htons(AEM_CFG_VIDEO_UNIT_BASE_STREAM_IN_PORT_##idx),\
							.number_of_stream_output_ports = htons(AEM_CFG_VIDEO_UNIT_NB_STREAM_OUT_PORT_##idx),\
							.base_stream_output_port = htons(AEM_CFG_VIDEO_UNIT_BASE_STREAM_OUT_PORT_##idx),\
							.number_of_external_input_ports = htons(AEM_CFG_VIDEO_UNIT_NB_EXT_IN_PORT_##idx),\
							.base_external_input_port = htons(AEM_CFG_VIDEO_UNIT_BASE_EXT_IN_PORT_##idx),\
							.number_of_external_output_ports = htons(AEM_CFG_VIDEO_UNIT_NB_EXT_OUT_PORT_##idx),\
							.base_external_output_port = htons(AEM_CFG_VIDEO_UNIT_BASE_EXT_OUT_PORT_##idx),\
							.number_of_internal_input_ports = htons(AEM_CFG_VIDEO_UNIT_NB_INT_IN_PORT_##idx),\
							.base_internal_input_port = htons(AEM_CFG_VIDEO_UNIT_BASE_INT_IN_PORT_##idx),\
							.number_of_internal_output_ports = htons(AEM_CFG_VIDEO_UNIT_NB_INT_OUT_PORT_##idx),\
							.base_internal_output_port = htons(AEM_CFG_VIDEO_UNIT_BASE_INT_OUT_PORT_##idx),\
							.number_of_controls = htons(AEM_CFG_VIDEO_UNIT_NB_CONTROLS_##idx),\
							.base_control = htons(AEM_CFG_VIDEO_UNIT_BASE_CONTROLS_##idx),\
							.number_of_signal_selectors = htons(AEM_CFG_VIDEO_UNIT_NB_SIGNAL_SEL_##idx),\
							.base_signal_selector = htons(AEM_CFG_VIDEO_UNIT_BASE_SIGNAL_SEL_##idx),\
							.number_of_mixers = htons(AEM_CFG_VIDEO_UNIT_NB_MIXERS_##idx),\
							.base_mixer = htons(AEM_CFG_VIDEO_UNIT_BASE_MIXER_##idx),\
							.number_of_matrices = htons(AEM_CFG_VIDEO_UNIT_NB_MATRICES_##idx),\
							.base_matrix = htons(AEM_CFG_VIDEO_UNIT_BASE_MATRIX_##idx),\
							.number_of_splitters = htons(AEM_CFG_VIDEO_UNIT_NB_SPLITTERS_##idx),\
							.base_splitter = htons(AEM_CFG_VIDEO_UNIT_BASE_SPLITTER_##idx),\
							.number_of_combiners = htons(AEM_CFG_VIDEO_UNIT_NB_COMBINERS_##idx),\
							.base_combiner = htons(AEM_CFG_VIDEO_UNIT_BASE_COMBINER_##idx),\
							.number_of_demultiplexers = htons(AEM_CFG_VIDEO_UNIT_NB_DEMUX_##idx),\
							.base_demultiplexer = htons(AEM_CFG_VIDEO_UNIT_BASE_DEMUX_##idx),\
							.number_of_multiplexers = htons(AEM_CFG_VIDEO_UNIT_NB_MUX_##idx),\
							.base_multiplexer = htons(AEM_CFG_VIDEO_UNIT_BASE_MUX_##idx),\
							.number_of_transcoders = htons(AEM_CFG_VIDEO_UNIT_NB_TRANSCODERS_##idx),\
							.base_transcoder = htons(AEM_CFG_VIDEO_UNIT_BASE_TRANSCODERS_##idx),\
							.number_of_control_blocks = htons(AEM_CFG_VIDEO_UNIT_NB_CONTROL_BLOCKS_##idx),\
							.base_control_block = htons(AEM_CFG_VIDEO_UNIT_BASE_CONTROL_BLOCK_##idx),\
						}


/* Stream input config */
#define AEM_CFG_STREAM_INPUT_DESCRIPTOR(idx)	{\
							.descriptor_type = htons(AEM_DESC_TYPE_STREAM_INPUT),\
							.descriptor_index = htons(idx),\
							.object_name = AEM_CFG_STREAM_INPUT_NAME_##idx,\
							.localized_description = htons(AEM_CFG_STREAM_INPUT_LOC_DESC_##idx),\
							.clock_domain_index = htons(AEM_CFG_STREAM_INPUT_CLOCK_DOMAIN_IDX_##idx),\
							.stream_flags = htons(AEM_CFG_STREAM_INPUT_STREAM_FLAGS_##idx),\
							.current_format = htonll(AEM_CFG_STREAM_INPUT_CURRENT_FORMAT_##idx),\
							.formats_offset = htons(AEM_FORMATS_OFFSET),\
							.number_of_formats = htons(AEM_CFG_STREAM_INPUT_NB_FORMATS_##idx),\
							.backup_talker_entity_id_0 = htonll(AEM_CFG_STREAM_INPUT_BACKUP_TALKER_ENTITY_0_##idx),\
							.backup_talker_unique_id_0 = htons(AEM_CFG_STREAM_INPUT_BACKUP_TALKER_UNIQUE_0_##idx),\
							.backup_talker_entity_id_1 = htonll(AEM_CFG_STREAM_INPUT_BACKUP_TALKER_ENTITY_1_##idx),\
							.backup_talker_unique_id_1 = htons(AEM_CFG_STREAM_INPUT_BACKUP_TALKER_UNIQUE_1_##idx),\
							.backup_talker_entity_id_2 = htonll(AEM_CFG_STREAM_INPUT_BACKUP_TALKER_ENTITY_2_##idx),\
							.backup_talker_unique_id_2 = htons(AEM_CFG_STREAM_INPUT_BACKUP_TALKER_UNIQUE_2_##idx),\
							.backedup_talker_entity_id = htonll(AEM_CFG_STREAM_INPUT_BACKEDUP_TALKER_ENTITY_##idx),\
							.backedup_talker_unique_id = htons(AEM_CFG_STREAM_INPUT_BACKEDUP_TALKER_UNIQUE_##idx),\
							.avb_interface_index = htons(AEM_CFG_STREAM_INPUT_AVB_ITF_INDEX_##idx),\
							.buffer_length = htonl(AEM_CFG_STREAM_INPUT_BUFFER_LENGTH_##idx),\
							.number_of_redundant_streams = htons(AEM_CFG_STREAM_INPUT_NB_REDUNDANT_STREAMS_##idx),\
							.formats = AEM_CFG_STREAM_INPUT_FORMATS_##idx,\
							.redundant_streams = AEM_CFG_STREAM_INPUT_REDUNDANT_STREAMS_##idx\
						}


/* Stream output config */
#define AEM_CFG_STREAM_OUTPUT_DESCRIPTOR(idx)	{\
							.descriptor_type = htons(AEM_DESC_TYPE_STREAM_OUTPUT),\
							.descriptor_index = htons(idx),\
							.object_name = AEM_CFG_STREAM_OUTPUT_NAME_##idx,\
							.localized_description = htons(AEM_CFG_STREAM_OUTPUT_LOC_DESC_##idx),\
							.clock_domain_index = htons(AEM_CFG_STREAM_OUTPUT_CLOCK_DOMAIN_IDX_##idx),\
							.stream_flags = htons(AEM_CFG_STREAM_OUTPUT_STREAM_FLAGS_##idx),\
							.current_format = htonll(AEM_CFG_STREAM_OUTPUT_CURRENT_FORMAT_##idx),\
							.formats_offset = htons(AEM_FORMATS_OFFSET),\
							.number_of_formats = htons(AEM_CFG_STREAM_OUTPUT_NB_FORMATS_##idx),\
							.backup_talker_entity_id_0 = htonll(AEM_CFG_STREAM_OUTPUT_BACKUP_TALKER_ENTITY_0_##idx),\
							.backup_talker_unique_id_0 = htons(AEM_CFG_STREAM_OUTPUT_BACKUP_TALKER_UNIQUE_0_##idx),\
							.backup_talker_entity_id_1 = htonll(AEM_CFG_STREAM_OUTPUT_BACKUP_TALKER_ENTITY_1_##idx),\
							.backup_talker_unique_id_1 = htons(AEM_CFG_STREAM_OUTPUT_BACKUP_TALKER_UNIQUE_1_##idx),\
							.backup_talker_entity_id_2 = htonll(AEM_CFG_STREAM_OUTPUT_BACKUP_TALKER_ENTITY_2_##idx),\
							.backup_talker_unique_id_2 = htons(AEM_CFG_STREAM_OUTPUT_BACKUP_TALKER_UNIQUE_2_##idx),\
							.backedup_talker_entity_id = htonll(AEM_CFG_STREAM_OUTPUT_BACKEDUP_TALKER_ENTITY_##idx),\
							.backedup_talker_unique_id = htons(AEM_CFG_STREAM_OUTPUT_BACKEDUP_TALKER_UNIQUE_##idx),\
							.avb_interface_index = htons(AEM_CFG_STREAM_OUTPUT_AVB_ITF_INDEX_##idx),\
							.buffer_length = htonl(AEM_CFG_STREAM_OUTPUT_BUFFER_LENGTH_##idx),\
							.number_of_redundant_streams = htons(AEM_CFG_STREAM_OUTPUT_NB_REDUNDANT_STREAMS_##idx),\
							.formats = AEM_CFG_STREAM_OUTPUT_FORMATS_##idx,\
							.redundant_streams = AEM_CFG_STREAM_OUTPUT_REDUNDANT_STREAMS_##idx\
						}


/* Jack output config */
#define AEM_CFG_JACK_OUTPUT_DESCRIPTOR(idx)	{\
							.descriptor_type = htons(AEM_DESC_TYPE_JACK_OUTPUT),\
							.descriptor_index = htons(idx),\
							.object_name = AEM_CFG_JACK_OUTPUT_NAME_##idx,\
							.localized_description = htons(AEM_CFG_JACK_OUTPUT_LOC_DESC_##idx),\
							.jack_flags = htons(AEM_CFG_JACK_OUTPUT_FLAGS_##idx),\
							.jack_type = htons(AEM_CFG_JACK_OUTPUT_TYPE_##idx),\
							.number_of_controls = htons(AEM_CFG_JACK_OUTPUT_NUM_CTRL_##idx),\
							.base_control = htons(AEM_CFG_JACK_OUTPUT_BASE_CTRL_##idx)\
						}


/* Jack input config */
#define AEM_CFG_JACK_INPUT_DESCRIPTOR(idx)	{\
							.descriptor_type = htons(AEM_DESC_TYPE_JACK_INPUT),\
							.descriptor_index = htons(idx),\
							.object_name = AEM_CFG_JACK_INPUT_NAME_##idx,\
							.localized_description = htons(AEM_CFG_JACK_INPUT_LOC_DESC_##idx),\
							.jack_flags = htons(AEM_CFG_JACK_INPUT_FLAGS_##idx),\
							.jack_type = htons(AEM_CFG_JACK_INPUT_TYPE_##idx),\
							.number_of_controls = htons(AEM_CFG_JACK_INPUT_NUM_CTRL_##idx),\
							.base_control = htons(AEM_CFG_JACK_INPUT_BASE_CTRL_##idx)\
						}

/* AVB interface config */
#define AEM_CFG_AVB_ITF_DESCRIPTOR(idx)		{\
							.descriptor_type = htons(AEM_DESC_TYPE_AVB_INTERFACE),\
							.descriptor_index = htons(idx),\
							.object_name = AEM_CFG_AVB_ITF_NAME_##idx,\
							.localized_description = htons(AEM_CFG_AVB_ITF_LOC_DESC_##idx),\
							.mac_address = NULL_MAC,\
							.interface_flags = htons(AEM_CFG_AVB_ITF_ITF_FLAGS_##idx),\
							.clock_identity = htonll(AEM_CFG_AVB_ITF_CLOCK_ID_##idx),\
							.priority1 = AEM_CFG_AVB_ITF_PRIO1_##idx,\
							.clock_class = AEM_CFG_AVB_ITF_CLOCK_CLASS_##idx,\
							.offset_scaled_log_variance = htons(AEM_CFG_AVB_ITF_OFF_SCALED_VAR_##idx),\
							.clock_accuracy = AEM_CFG_AVB_ITF_CLOCK_ACCURACY_##idx,\
							.priority2 = AEM_CFG_AVB_ITF_PRIO2_##idx,\
							.domain_number = AEM_CFG_AVB_ITF_DOMAIN_NB_##idx,\
							.log_sync_interval = AEM_CFG_AVB_ITF_LOG_SYN_INTER_##idx,\
							.log_announce_interval = AEM_CFG_AVB_ITF_LOG_ANN_INTER_##idx,\
							.log_pdelay_interval = AEM_CFG_AVB_ITF_POG_PDEL_INTER_##idx,\
							.port_number = htons(AEM_CFG_AVB_ITF_PORT_NB_##idx)\
						}


/* Clock source config */
#define AEM_CFG_CLK_SOURCE_DESCRIPTOR(idx)	{\
							.descriptor_type = htons(AEM_DESC_TYPE_CLOCK_SOURCE),\
							.descriptor_index = htons(idx),\
							.object_name = AEM_CFG_CLK_SOURCE_NAME_##idx,\
							.localized_description = htons(AEM_CFG_CLK_SOURCE_LOC_DESC_##idx),\
							.clock_source_flags = htons(AEM_CFG_CLK_SOURCE_FLAGS_##idx),\
							.clock_source_type = htons(AEM_CFG_CLK_SOURCE_TYPE_##idx),\
							.clock_source_identifier = htonll(AEM_CFG_CLK_SOURCE_ID_##idx),\
							.clock_source_location_type = htons(AEM_CFG_CLK_SOURCE_LOC_TYPE_##idx),\
							.clock_source_location_index = htons(AEM_CFG_CLK_SOURCE_LOC_INDEX_##idx)\
						}


/* Locale config */
#define AEM_CFG_LOCALE_DESCRIPTOR(idx)		{\
							.descriptor_type = htons(AEM_DESC_TYPE_LOCALE),\
							.descriptor_index = htons(idx),\
							.locale_identifier = AEM_CFG_LOCALE_IDENTIFIER_##idx,\
							.number_of_strings = htons(AEM_CFG_LOCALE_NB_STRINGS_##idx),\
							.base_strings = htons(AEM_CFG_LOCALE_BASE_STRINGS_##idx)\
						}


/* Strings config */
#define AEM_CFG_STRINGS_DESCRIPTOR(idx)		{\
							.descriptor_type = htons(AEM_DESC_TYPE_STRINGS),\
							.descriptor_index = htons(idx),\
							.strings_0 = AEM_CFG_STRINGS_0_##idx,\
							.strings_1 = AEM_CFG_STRINGS_1_##idx,\
							.strings_2 = AEM_CFG_STRINGS_2_##idx,\
							.strings_3 = AEM_CFG_STRINGS_3_##idx,\
							.strings_4 = AEM_CFG_STRINGS_4_##idx,\
							.strings_5 = AEM_CFG_STRINGS_5_##idx,\
							.strings_6 = AEM_CFG_STRINGS_6_##idx\
						}


/* Clock domain config */
#define AEM_CFG_CLK_DOMAIN_DESCRIPTOR(idx)	{\
							.descriptor_type = htons(AEM_DESC_TYPE_CLOCK_DOMAIN),\
							.descriptor_index = htons(idx),\
							.object_name = AEM_CFG_CLK_DOMAIN_NAME_##idx,\
							.localized_description = htons(AEM_CFG_CLK_DOMAIN_LOC_DESC_##idx),\
							.clock_source_index = htons(AEM_CFG_CLK_DOMAIN_SOURCE_IDX_##idx),\
							.clock_sources_offset = htons(AEM_CLOCK_SOURCES_OFFSET),\
							.clock_sources_count = htons(AEM_CFG_CLK_DOMAIN_SOURCES_COUNT_##idx),\
							.clock_sources = AEM_CFG_CLK_DOMAIN_SOURCES_##idx\
						}


/* Stream port input config */
#define AEM_CFG_STREAM_PORT_IN_DESCRIPTOR(idx)	{\
							.descriptor_type = htons(AEM_DESC_TYPE_STREAM_PORT_INPUT),\
							.descriptor_index = htons(idx),\
							.clock_domain_index = htons(AEM_CFG_STREAM_PORT_IN_CLK_DOM_IDX_##idx),\
							.port_flags = htons(AEM_CFG_STREAM_PORT_IN_PORT_FLAGS_##idx),\
							.number_of_controls = htons(AEM_CFG_STREAM_PORT_IN_NB_CONTROLS_##idx),\
							.base_control= htons(AEM_CFG_STREAM_PORT_IN_BASE_CONTROL_##idx),\
							.number_of_clusters = htons(AEM_CFG_STREAM_PORT_IN_NB_CLUSTERS_##idx),\
							.base_cluster = htons(AEM_CFG_STREAM_PORT_IN_BASE_CLUSTER_##idx),\
							.number_of_maps = htons(AEM_CFG_STREAM_PORT_IN_NB_MAPS_##idx),\
							.base_map = htons(AEM_CFG_STREAM_PORT_IN_BASE_MAP_##idx)\
						}


/* Stream port output config */
#define AEM_CFG_STREAM_PORT_OUT_DESCRIPTOR(idx)	{\
							.descriptor_type = htons(AEM_DESC_TYPE_STREAM_PORT_OUTPUT),\
							.descriptor_index = htons(idx),\
							.clock_domain_index = htons(AEM_CFG_STREAM_PORT_OUT_CLK_DOM_IDX_##idx),\
							.port_flags = htons(AEM_CFG_STREAM_PORT_OUT_PORT_FLAGS_##idx),\
							.number_of_controls = htons(AEM_CFG_STREAM_PORT_OUT_NB_CONTROLS_##idx),\
							.base_control= htons(AEM_CFG_STREAM_PORT_OUT_BASE_CONTROL_##idx),\
							.number_of_clusters = htons(AEM_CFG_STREAM_PORT_OUT_NB_CLUSTERS_##idx),\
							.base_cluster = htons(AEM_CFG_STREAM_PORT_OUT_BASE_CLUSTER_##idx),\
							.number_of_maps = htons(AEM_CFG_STREAM_PORT_OUT_NB_MAPS_##idx),\
							.base_map = htons(AEM_CFG_STREAM_PORT_OUT_BASE_MAP_##idx)\
						}


/* Audio cluster config */
#define AEM_CFG_AUDIO_CLUSTER_DESCRIPTOR(idx)	{\
							.descriptor_type = htons(AEM_DESC_TYPE_AUDIO_CLUSTER),\
							.descriptor_index = htons(idx),\
							.object_name = AEM_CFG_AUDIO_CLUSTER_NAME_##idx,\
							.localized_description = htons(AEM_CFG_AUDIO_CLUSTER_LOC_DESC_##idx),\
							.signal_type = htons(AEM_CFG_AUDIO_CLUSTER_SIGNAL_TYPE_##idx),\
							.signal_index = htons(AEM_CFG_AUDIO_CLUSTER_SIGNAL_IDX_##idx),\
							.signal_output = htons(AEM_CFG_AUDIO_CLUSTER_SIGNAL_OUTPUT_##idx),\
							.path_latency = htonl(AEM_CFG_AUDIO_CLUSTER_PATH_LAT_##idx),\
							.block_latency = htonl(AEM_CFG_AUDIO_CLUSTER_BLOCK_LAT_##idx),\
							.channel_count = htons(AEM_CFG_AUDIO_CLUSTER_CHAN_COUNT_##idx),\
							.format = AEM_CFG_AUDIO_CLUSTER_FORMAT_##idx\
						}

/* Video cluster config */
#define AEM_CFG_VIDEO_CLUSTER_DESCRIPTOR(idx)	{\
							.descriptor_type = htons(AEM_DESC_TYPE_VIDEO_CLUSTER),\
							.descriptor_index = htons(idx),\
							.object_name = AEM_CFG_VIDEO_CLUSTER_NAME_##idx,\
							.localized_description = htons(AEM_CFG_VIDEO_CLUSTER_LOC_DESC_##idx),\
							.signal_type = htons(AEM_CFG_VIDEO_CLUSTER_SIGNAL_TYPE_##idx),\
							.signal_index = htons(AEM_CFG_VIDEO_CLUSTER_SIGNAL_IDX_##idx),\
							.signal_output = htons(AEM_CFG_VIDEO_CLUSTER_SIGNAL_OUTPUT_##idx),\
							.path_latency = htonl(AEM_CFG_VIDEO_CLUSTER_PATH_LAT_##idx),\
							.block_latency = htonl(AEM_CFG_VIDEO_CLUSTER_BLOCK_LAT_##idx),\
							.format = AEM_CFG_VIDEO_CLUSTER_FORMAT_##idx,\
							.current_format_specific = htonl(AEM_CFG_VIDEO_CLUSTER_CURRENT_FORMAT_SPECIFIC_##idx),\
							.supported_format_specifics_offset = htons(AEM_SUPPORTED_FORMAT_SPECIFICS_OFFSET),\
							.supported_format_specifics_count = htons(AEM_CFG_VIDEO_CLUSTER_SUPPORTED_FORMAT_SPECIFICS_COUNT_##idx),\
							.current_sampling_rate = htonl(AEM_CFG_VIDEO_CLUSTER_CURRENT_SAMPLING_RATE_##idx),\
							.supported_sampling_rates_count = htons(AEM_CFG_VIDEO_CLUSTER_SUPPORTED_SAMPLING_RATES_COUNT_##idx),\
							.current_aspect_ratio = htons(AEM_CFG_VIDEO_CLUSTER_CURRENT_ASPECT_RATIO_##idx),\
							.supported_aspect_ratios_count = htons(AEM_CFG_VIDEO_CLUSTER_SUPPORTED_ASPECT_RATIOS_COUNT_##idx),\
							.current_size = htonl(AEM_CFG_VIDEO_CLUSTER_CURRENT_SIZE_##idx),\
							.supported_sizes_count = htons(AEM_CFG_VIDEO_CLUSTER_SUPPORTED_SIZES_COUNT_##idx),\
							.current_color_space = htons(AEM_CFG_VIDEO_CLUSTER_CURRENT_COLOR_SPACE_##idx),\
							.supported_color_spaces_count = htons(AEM_CFG_VIDEO_CLUSTER_SUPPORTED_COLOR_SPACES_COUNT_##idx),\
							.supported_format_specifics = AEM_CFG_VIDEO_CLUSTER_SUPPORTED_FORMAT_SPECIFICS_##idx,\
							.supported_sampling_rates = AEM_CFG_VIDEO_CLUSTER_SUPPORTED_SAMPLING_RATES_##idx,\
							.supported_aspect_ratios = AEM_CFG_VIDEO_CLUSTER_SUPPORTED_ASPECT_RATIOS_##idx,\
							.supported_sizes = AEM_CFG_VIDEO_CLUSTER_SUPPORTED_SIZES_##idx,\
							.supported_color_spaces = AEM_CFG_VIDEO_CLUSTER_SUPPORTED_COLOR_SPACES_##idx,\
						}

/* Audio map config */
#define AEM_CFG_AUDIO_MAP_DESCRIPTOR(idx)	{\
							.descriptor_type = htons(AEM_DESC_TYPE_AUDIO_MAP),\
							.descriptor_index = htons(idx),\
							.mappings_offset = htons(AEM_MAPPINGS_OFFSET),\
							.number_of_mappings = htons(AEM_CFG_AUDIO_MAP_NB_MAPPINGS_##idx),\
							.mappings = AEM_CFG_AUDIO_MAP_MAP_UNIT_##idx\
						}

/* Video map config */
#define AEM_CFG_VIDEO_MAP_DESCRIPTOR(idx)	{\
							.descriptor_type = htons(AEM_DESC_TYPE_VIDEO_MAP),\
							.descriptor_index = htons(idx),\
							.mappings_offset = htons(AEM_MAPPINGS_OFFSET),\
							.number_of_mappings = htons(AEM_CFG_VIDEO_MAP_NB_MAPPINGS_##idx),\
							.mappings = AEM_CFG_VIDEO_MAP_MAPPINGS_##idx\
						}

/* External port input config */
#define AEM_CFG_EXT_PORT_INPUT_DESCRIPTOR(idx)	{\
							.descriptor_type = htons(AEM_DESC_TYPE_EXTERNAL_PORT_INPUT),\
							.descriptor_index = htons(idx),\
							.clock_domain_index = htons(AEM_CFG_EXT_PORT_INPUT_CLK_DOM_IDX_##idx),\
							.port_flags = htons( AEM_CFG_EXT_PORT_INPUT_PORT_FLAGS_##idx),\
							.number_of_controls = htons(AEM_CFG_EXT_PORT_INPUT_NB_CONTROL_##idx),\
							.base_control = htons(AEM_CFG_EXT_PORT_INPUT_BASE_CONTROL_##idx),\
							.signal_type = htons(AEM_CFG_EXT_PORT_INPUT_SIGNAL_TYPE_##idx),\
							.signal_index = htons(AEM_CFG_EXT_PORT_INPUT_SIGNAL_IDX_##idx),\
							.signal_output = htons(AEM_CFG_EXT_PORT_INPUT_SIGNAL_OUTPUT_##idx),\
							.block_latency = htonl(AEM_CFG_EXT_PORT_INPUT_BLOCK_LAT_##idx),\
							.jack_index = htons(AEM_CFG_EXT_PORT_INPUT_JACK_IDX_##idx)\
						}


/* External port output config */
#define AEM_CFG_EXT_PORT_OUTPUT_DESCRIPTOR(idx)	{\
							.descriptor_type = htons(AEM_DESC_TYPE_EXTERNAL_PORT_OUTPUT),\
							.descriptor_index = htons(idx),\
							.clock_domain_index = htons(AEM_CFG_EXT_PORT_OUTPUT_CLK_DOM_IDX_##idx),\
							.port_flags = htons( AEM_CFG_EXT_PORT_OUTPUT_PORT_FLAGS_##idx),\
							.number_of_controls = htons(AEM_CFG_EXT_PORT_OUTPUT_NB_CONTROL_##idx),\
							.base_control = htons(AEM_CFG_EXT_PORT_OUTPUT_BASE_CONTROL_##idx),\
							.signal_type = htons(AEM_CFG_EXT_PORT_OUTPUT_SIGNAL_TYPE_##idx),\
							.signal_index = htons(AEM_CFG_EXT_PORT_OUTPUT_SIGNAL_IDX_##idx),\
							.signal_output = htons(AEM_CFG_EXT_PORT_OUTPUT_SIGNAL_OUTPUT_##idx),\
							.block_latency = htonl(AEM_CFG_EXT_PORT_OUTPUT_BLOCK_LAT_##idx),\
							.jack_index = htons(AEM_CFG_EXT_PORT_OUTPUT_JACK_IDX_##idx)\
						}


/* Control config */
#define AEM_CFG_CONTROL_DESCRIPTOR(idx)		{\
							.descriptor_type = htons(AEM_DESC_TYPE_CONTROL),\
							.descriptor_index = htons(idx),\
							.object_name = AEM_CFG_CONTROL_NAME_##idx,\
							.localized_description = htons(AEM_CFG_CONTROL_LOC_DESC_##idx),\
							.block_latency = htonl(AEM_CFG_CONTROL_BLOCK_LAT_##idx),\
							.control_latency = htonl(AEM_CFG_CONTROL_CTRL_LAT_##idx),\
							.control_domain = htons(AEM_CFG_CONTROL_DOMAIN_##idx),\
							.control_value_type = htons(AEM_CFG_CONTROL_VALUE_TYPE_##idx),\
							.control_type = htonll(AEM_CFG_CONTROL_TYPE_##idx),\
							.reset_time = htonl(AEM_CFG_CONTROL_RESET_TIME_##idx),\
							.values_offset = htons(AEM_VALUES_OFFSET),\
							.number_of_values = htons(AEM_CFG_CONTROL_NB_VALUES_##idx),\
							.signal_type = htons(AEM_CFG_CONTROL_SIGNAL_TYPE_##idx),\
							.signal_index = htons(AEM_CFG_CONTROL_SIGNAL_INDEX_##idx),\
							.signal_output = htons(AEM_CFG_CONTROL_SIGNAL_OUTPUT_##idx),\
							.value_details = AEM_CFG_CONTROL_VALUE_DETAILS_##idx\
						}


#define AEM_DESC_HDR_INIT(desc_hdr, name, type)	\
	do {\
		(desc_hdr)[type].size = sizeof(name##_storage[0]);\
		(desc_hdr)[type].ptr = name##_storage;\
		(desc_hdr)[type].total = sizeof(name##_storage) /  sizeof(name##_storage[0]);\
	} while(0)

/* Descriptors storage */
#define AEM_ENTITY_STORAGE()	\
	static struct entity_descriptor entity_storage[] = { AEM_CFG_ENTITY_DESCRIPTOR };\
	static struct configuration_descriptor configuration_storage[] = AEM_CFG_CONFIG_DESCRIPTORS;\
	static struct audio_unit_descriptor audio_unit_storage[] = AEM_CFG_AUDIO_UNIT_DESCRIPTORS;\
	static struct video_unit_descriptor video_unit_storage[] = AEM_CFG_VIDEO_UNIT_DESCRIPTORS;\
	static struct stream_descriptor stream_input_storage[] = AEM_CFG_STREAM_INPUT_DESCRIPTORS;\
	static struct stream_descriptor stream_output_storage[] = AEM_CFG_STREAM_OUTPUT_DESCRIPTORS;\
	static struct jack_descriptor jack_input_storage[] = AEM_CFG_JACK_INPUT_DESCRIPTORS;\
	static struct jack_descriptor jack_output_storage[] = AEM_CFG_JACK_OUTPUT_DESCRIPTORS;\
	static struct avb_interface_descriptor avb_interface_storage[] = AEM_CFG_AVB_ITF_DESCRIPTORS;\
	static struct clock_source_descriptor clock_source_storage[] = AEM_CFG_CLK_SOURCE_DESCRIPTORS;\
	static struct locale_descriptor locale_storage[] = AEM_CFG_LOCALE_DESCRIPTORS;\
	static struct strings_descriptor strings_storage[] = AEM_CFG_STRINGS_DESCRIPTORS;\
	static struct clock_domain_descriptor clock_domain_storage[] = AEM_CFG_CLK_DOMAIN_DESCRIPTORS;\
	static struct stream_port_descriptor stream_port_input_storage[] = AEM_CFG_STREAM_PORT_IN_DESCRIPTORS;\
	static struct stream_port_descriptor stream_port_output_storage[] = AEM_CFG_STREAM_PORT_OUT_DESCRIPTORS;\
	static struct audio_cluster_descriptor audio_cluster_storage[] = AEM_CFG_AUDIO_CLUSTER_DESCRIPTORS;\
	static struct audio_map_descriptor audio_map_storage[] = AEM_CFG_AUDIO_MAP_DESCRIPTORS;\
	static struct video_cluster_descriptor video_cluster_storage[] = AEM_CFG_VIDEO_CLUSTER_DESCRIPTORS;\
	static struct video_map_descriptor video_map_storage[] = AEM_CFG_VIDEO_MAP_DESCRIPTORS;\
	static struct external_port_descriptor external_port_input_storage[] = AEM_CFG_EXT_PORT_INPUT_DESCRIPTORS;\
	static struct external_port_descriptor external_port_output_storage[] = AEM_CFG_EXT_PORT_OUTPUT_DESCRIPTORS;\
	static struct control_descriptor control_storage[] = AEM_CFG_CONTROL_DESCRIPTORS

#define AEM_ENTITY_INIT(aem_desc_hdr)	\
	AEM_DESC_HDR_INIT(aem_desc_hdr, entity, AEM_DESC_TYPE_ENTITY);\
	AEM_DESC_HDR_INIT(aem_desc_hdr, configuration, AEM_DESC_TYPE_CONFIGURATION);\
	AEM_DESC_HDR_INIT(aem_desc_hdr, audio_unit, AEM_DESC_TYPE_AUDIO_UNIT);\
	AEM_DESC_HDR_INIT(aem_desc_hdr, video_unit, AEM_DESC_TYPE_VIDEO_UNIT);\
	AEM_DESC_HDR_INIT(aem_desc_hdr, stream_input, AEM_DESC_TYPE_STREAM_INPUT);\
	AEM_DESC_HDR_INIT(aem_desc_hdr, stream_output, AEM_DESC_TYPE_STREAM_OUTPUT);\
	AEM_DESC_HDR_INIT(aem_desc_hdr, jack_input, AEM_DESC_TYPE_JACK_INPUT);\
	AEM_DESC_HDR_INIT(aem_desc_hdr, jack_output, AEM_DESC_TYPE_JACK_OUTPUT);\
	AEM_DESC_HDR_INIT(aem_desc_hdr, avb_interface, AEM_DESC_TYPE_AVB_INTERFACE);\
	AEM_DESC_HDR_INIT(aem_desc_hdr, clock_source, AEM_DESC_TYPE_CLOCK_SOURCE);\
	AEM_DESC_HDR_INIT(aem_desc_hdr, locale, AEM_DESC_TYPE_LOCALE);\
	AEM_DESC_HDR_INIT(aem_desc_hdr, strings, AEM_DESC_TYPE_STRINGS);\
	AEM_DESC_HDR_INIT(aem_desc_hdr, clock_domain, AEM_DESC_TYPE_CLOCK_DOMAIN);\
	AEM_DESC_HDR_INIT(aem_desc_hdr, stream_port_input, AEM_DESC_TYPE_STREAM_PORT_INPUT);\
	AEM_DESC_HDR_INIT(aem_desc_hdr, stream_port_output, AEM_DESC_TYPE_STREAM_PORT_OUTPUT);\
	AEM_DESC_HDR_INIT(aem_desc_hdr, audio_cluster, AEM_DESC_TYPE_AUDIO_CLUSTER);\
	AEM_DESC_HDR_INIT(aem_desc_hdr, audio_map, AEM_DESC_TYPE_AUDIO_MAP);\
	AEM_DESC_HDR_INIT(aem_desc_hdr, video_cluster, AEM_DESC_TYPE_VIDEO_CLUSTER);\
	AEM_DESC_HDR_INIT(aem_desc_hdr, video_map, AEM_DESC_TYPE_VIDEO_MAP);\
	AEM_DESC_HDR_INIT(aem_desc_hdr, external_port_input, AEM_DESC_TYPE_EXTERNAL_PORT_INPUT);\
	AEM_DESC_HDR_INIT(aem_desc_hdr, external_port_output, AEM_DESC_TYPE_EXTERNAL_PORT_OUTPUT);\
	AEM_DESC_HDR_INIT(aem_desc_hdr, control, AEM_DESC_TYPE_CONTROL)

#endif /* _GENAVB_PUBLIC_AEM_ENTITY_H_ */
