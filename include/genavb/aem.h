/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2018-2019, 2022-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file aem.h
 \brief GenAVB public API
 \details 1722.1 AECP format definition and helper functions.

 \copyright Copyright 2014-2016 Freescale Semiconductor, Inc.
 \copyright Copyright 2016, 2018-2019, 2022-2023 NXP
*/

#ifndef _GENAVB_PUBLIC_AEM_H_
#define _GENAVB_PUBLIC_AEM_H_

#include "types.h"

#define AEM_STR_LEN_MAX 64

/* Throughout all different configurations */
#define AEM_NUM_DESCRIPTORS_MAX			23
#define AEM_NUM_SAMPLING_RATES_MAX		2
#define AEM_NUM_FORMATS_MAX			16
#define AEM_NUM_CLOCK_SOURCES_MAX			8
#define AEM_NUM_AUDIO_MAPS_MAX			16
#define AEM_NUM_VIDEO_MAPS_MAX			8
#define AEN_NUM_VIDEO_FORMATS_MAX		1
#define AEM_NUM_VIDEO_RATES_MAX			1
#define AEM_NUM_VIDEO_ASPECT_RATIOS_MAX		1
#define AEM_NUM_VIDEO_FRAME_SIZES_MAX		1
#define AEM_NUM_VIDEO_COLOR_SPACES_MAX		1
/* Spec maximum 404 */
#define AEM_NUM_VALUE_DETAILS_LINEAR_U8_MAX	1
#define AEM_NUM_VALUE_DETAILS_LINEAR_UTF8_MAX	1
#define AEM_UTF8_MAX_LENGTH			96
#define AEM_DESC_MAX_LENGTH			508
#define AEM_DESC_MAX_NUM			65536

#define AEM_DESCRIPTOR_COUNTS_OFFSET		74
#define AEM_SAMPLING_RATES_OFFSET		144
#define AEM_SAMPLING_RATES_COUNT_MAX		91
#define AEM_FORMATS_OFFSET			132
#define AEM_NUMBER_OF_FORMATS_MAX		47
#define AEM_MAPPINGS_OFFSET			8
#define AEM_NUMBER_OF_MAPPINGS_MAX		62
#define AEM_SUPPORTED_FORMAT_SPECIFICS_OFFSET	121
#define AEM_VALUES_OFFSET			104
#define AEM_VALUE_DETAILS_LEN_MAX		404
#define AEM_CLOCK_SOURCES_OFFSET		76
#define AEM_CLOCK_SOURCES_COUNT_MAX		249

/* Entity model status codes Table 7.126 */
#define AEM_SUCCESS                  0
#define AEM_NOT_IMPLEMENTED          1
#define AEM_NO_SUCH_DESCRIPTOR       2
#define AEM_ENTITY_LOCKED            3
#define AEM_ENTITY_ACQUIRED          4
#define AEM_NOT_AUTHENTICATED        5
#define AEM_AUTHENTICATION_DISABLED  6
#define AEM_BAD_ARGUMENTS            7
#define AEM_NO_RESOURCES             8
#define AEM_IN_PROGRESS              9
#define AEM_ENTITY_MISBEHAVING       10
#define AEM_NOT_SUPPORTED            11
#define AEM_STREAM_IS_RUNNING        12

/* Descriptor types Table 7.1 */
#define AEM_DESC_TYPE_ENTITY			0x00
#define AEM_DESC_TYPE_CONFIGURATION		0x01
#define AEM_DESC_TYPE_AUDIO_UNIT		0x02
#define AEM_DESC_TYPE_VIDEO_UNIT		0x03
#define AEM_DESC_TYPE_SENSOR_UNIT		0x04
#define AEM_DESC_TYPE_STREAM_INPUT		0x05
#define AEM_DESC_TYPE_STREAM_OUTPUT		0x06
#define AEM_DESC_TYPE_JACK_INPUT		0x07
#define AEM_DESC_TYPE_JACK_OUTPUT		0x08
#define AEM_DESC_TYPE_AVB_INTERFACE		0x09
#define AEM_DESC_TYPE_CLOCK_SOURCE		0x0a
#define AEM_DESC_TYPE_MEMORY_OBJECT		0x0b
#define AEM_DESC_TYPE_LOCALE			0x0c
#define AEM_DESC_TYPE_STRINGS			0x0d
#define AEM_DESC_TYPE_STREAM_PORT_INPUT		0x0e
#define AEM_DESC_TYPE_STREAM_PORT_OUTPUT	0x0f
#define AEM_DESC_TYPE_EXTERNAL_PORT_INPUT	0x10
#define AEM_DESC_TYPE_EXTERNAL_PORT_OUTPUT	0x11
#define AEM_DESC_TYPE_INTERNAL_PORT_INPUT	0x12
#define AEM_DESC_TYPE_INTERNAL_PORT_OUTPUT	0x13
#define AEM_DESC_TYPE_AUDIO_CLUSTER		0x14
#define AEM_DESC_TYPE_VIDEO_CLUSTER		0x15
#define AEM_DESC_TYPE_SENSOR_CLUSTER		0x16
#define AEM_DESC_TYPE_AUDIO_MAP			0x17
#define AEM_DESC_TYPE_VIDEO_MAP			0x18
#define AEM_DESC_TYPE_SENSOR_MAP		0x19
#define AEM_DESC_TYPE_CONTROL			0x1a
#define AEM_DESC_TYPE_SIGNAL_SELECTOR		0x1b
#define AEM_DESC_TYPE_MIXER			0x1c
#define AEM_DESC_TYPE_MATRIX			0x1d
#define AEM_DESC_TYPE_MATRIX_SIGNAL		0x1e
#define AEM_DESC_TYPE_SIGNAL_SPLITTER		0x1f
#define AEM_DESC_TYPE_SIGNAL_COMBINER		0x20
#define AEM_DESC_TYPE_SIGNAL_DEMULTIPLEXER	0x21
#define AEM_DESC_TYPE_SIGNAL_MULTIPLEXER	0x22
#define AEM_DESC_TYPE_SIGNAL_TRANSCODER		0x23
#define AEM_DESC_TYPE_CLOCK_DOMAIN		0x24
#define AEM_DESC_TYPE_CONTROL_BLOCK		0x25
#define AEM_DESC_TYPE_INVALID			0xffff

#define AEM_NUM_DESC_TYPES			(AEM_DESC_TYPE_CONTROL_BLOCK + 1)

/* JACK descriptor flags Table 7.11 */
#define AEM_JACK_FLAG_CLOCK_SYNC_SOURCE		(1 << 0)
#define AEM_JACK_FLAG_CAPTIVE			(1 << 1)

/* JACK types Table 7.12 */
#define AEM_JACK_TYPE_SPEAKER			0x0000
#define AEM_JACK_TYPE_HEADPHONE			0x0001
#define AEM_JACK_TYPE_ANALOG_MICROPHONE		0x0002
#define AEM_JACK_TYPE_SPDIF			0x0003
#define AEM_JACK_TYPE_ADAT			0x0004
#define AEM_JACK_TYPE_TDIF			0x0005
#define AEM_JACK_TYPE_MADI			0x0006
#define AEM_JACK_TYPE_UNBALANCED_ANALOG		0x0007
#define AEM_JACK_TYPE_BALANCED_ANALOG		0x0008
#define AEM_JACK_TYPE_DIGITAL			0x0009
#define AEM_JACK_TYPE_MIDI			0x000a
#define AEM_JACK_TYPE_AES_EBU			0x000b
#define AEM_JACK_TYPE_COMPOSITE_VIDEO		0x000c
#define AEM_JACK_TYPE_S_VHS_VIDEO		0x000d
#define AEM_JACK_TYPE_COMPONENT_VIDEO		0x000e
#define AEM_JACK_TYPE_DVI			0x000f
#define AEM_JACK_TYPE_HDMI			0x0010
#define AEM_JACK_TYPE_UDI			0x0011
#define AEM_JACK_TYPE_DISPLAYPORT		0x0012
#define AEM_JACK_TYPE_ANTENNA			0x0013
#define AEM_JACK_TYPE_ANALOG_TUNER		0x0014
#define AEM_JACK_TYPE_ETHERNET			0x0015
#define AEM_JACK_TYPE_WIFI			0x0016
#define AEM_JACK_TYPE_USB			0x0017
#define AEM_JACK_TYPE_PCI			0x0018
#define AEM_JACK_TYPE_PCI_E			0x0019
#define AEM_JACK_TYPE_SCSI			0x001a
#define AEM_JACK_TYPE_ATA			0x001b
#define AEM_JACK_TYPE_IMAGER			0x001c
#define AEM_JACK_TYPE_IR			0x001d
#define AEM_JACK_TYPE_THUNDERBOLT		0x001e
#define AEM_JACK_TYPE_SATA			0x001f
#define AEM_JACK_TYPE_SMPTE_LTC			0x0020
#define AEM_JACK_TYPE_DIGITAL_MICROPHONE	0x0021
#define AEM_JACK_TYPE_AUDIO_MEDIA_CLOCK		0x0022
#define AEM_JACK_TYPE_VIDEO_MEDIA_CLOCK		0x0023
#define AEM_JACK_TYPE_GNSS_CLOCK		0x0024
#define AEM_JACK_TYPE_PPS			0x0025

/* AVB interface flags Table 7.14 */
#define AEM_AVB_FLAGS_GPTP_GRANDMASTER_SUPPORTED	(1 << 0)
#define AEM_AVB_FLAGS_GPTP_SUPPORTED			(1 << 1)
#define AEM_AVB_FLAGS_SRP_SUPPORTED			(1 << 2)

/* Clock source flags Table 7.16 */
#define AEM_CLOCK_SOURCE_FLAGS_STREAM_ID        (1 << 0)
#define AEM_CLOCK_SOURCE_FLAGS_LOCAL_ID         (1 << 1)

/* Clock source type Table 7.17 */
#define AEM_CLOCK_SOURCE_TYPE_INTERNAL		0x0000
#define AEM_CLOCK_SOURCE_TYPE_EXTERNAL		0x0001
#define AEM_CLOCK_SOURCE_TYPE_INPUT_STREAM	0x0002

/* Audio cluster format Table 7.28 */
#define AEM_AUDIO_CLUSTER_FORMAT_IEC_60958	0x00
#define AEM_AUDIO_CLUSTER_FORMAT_MBLA		0x40
#define AEM_AUDIO_CLUSTER_FORMAT_MIDI		0x80
#define AEM_AUDIO_CLUSTER_FORMAT_SMPTE		0x88

/* Video cluster format Table 7.30 */
#define AEM_VIDEO_CLUSTER_FORMAT_MPEG_PES		0x00
#define AEM_VIDEO_CLUSTER_FORMAT_AVTP			0x01
#define AEM_VIDEO_CLUSTER_FORMAT_RTP_PAYLOAD		0x02
#define AEM_VIDEO_CLUSTER_FORMAT_VENDOR_SPECIFIC	0xfe
#define AEM_VIDEO_CLUSTER_FORMAT_EXPERIMENTAL		0xff


/* Input/output stream related */
#define AEM_STREAM_FLAG_CLOCK_SYNC_SOURCE		(1 << 0)
#define AEM_STREAM_FLAG_CLASS_A				(1 << 1)
#define AEM_STREAM_FLAG_CLASS_B				(1 << 2)
#define AEM_STREAM_FLAG_SUPPORTS_ENCRYPTED		(1 << 3)
#define AEM_STREAM_FLAG_PRIMARY_BACKUP_SUPPORTED	(1 << 4)
#define AEM_STREAM_FLAG_PRIMARY_BACKUP_VALID		(1 << 5)
#define AEM_STREAM_FLAG_SECONDARY_BACKUP_SUPPORTED	(1 << 6)
#define AEM_STREAM_FLAG_SECONDARY_BACKUP_VALID		(1 << 7)
#define AEM_STREAM_FLAG_TERTIARY_BACKUP_SUPPORTED	(1 << 8)
#define AEM_STREAM_FLAG_TERTIARY_BACKUP_VALID		(1 << 9)

/* Control types 7.3.4 */
#define AEM_CONTROL_TYPE_ENABLE				0x90e0f00000000000
#define AEM_CONTROL_TYPE_IDENTIFY			0x90e0f00000000001
#define AEM_CONTROL_TYPE_MUTE				0x90e0f00000000002
#define AEM_CONTROL_TYPE_INVERT				0x90e0f00000000003
#define AEM_CONTROL_TYPE_GAIN				0x90e0f00000000004
#define AEM_CONTROL_TYPE_ATTENUATE			0x90e0f00000000005
#define AEM_CONTROL_TYPE_MEDIA_TRACK			0x90e0f00000030004
#define AEM_CONTROL_TYPE_MEDIA_TRACK_NAME		0x90e0f00000030005
#define AEM_CONTROL_TYPE_MEDIA_PLAYBACK_TRANSPORT	0x90e0f00000030008

/* Control value units format codes - table 7.70 */
#define AEM_CONTROL_CODE_UNITLESS		0x00
#define AEM_CONTROL_CODE_COUNT			0x01
#define AEM_CONTROL_CODE_PERCENT		0x02
#define AEM_CONTROL_CODE_HERTZ			0x10
#define AEM_CONTROL_CODE_DB			0xb0

/* Control value types 7.106 */
#define AEM_CONTROL_LINEAR_INT8			0
#define AEM_CONTROL_LINEAR_UINT8		1
#define AEM_CONTROL_LINEAR_INT16		2
#define AEM_CONTROL_LINEAR_UINT16		3
#define AEM_CONTROL_LINEAR_INT32		4
#define AEM_CONTROL_LINEAR_UINT32		5
#define AEM_CONTROL_UTF8			0x1f

/* 7.18 */
#define AEM_CONTROL_SET_UNIT_FORMAT(mult, code) \
	(((mult & 0xFF) << 8) | (code & 0xFF))

/* 7.18 */
#define AEM_CONTROL_SET_VALUE_TYPE(r, u, value_type) \
	(((r & 0x1) << 15) | ((u & 0x1) << 14) | (value_type & 0x3FFF))

#define AEM_CONTROL_GET_R(control_value_type) ((control_value_type) & (1 << 15))
#define AEM_CONTROL_GET_U(control_value_type) ((control_value_type) & (1 << 14))
#define AEM_CONTROL_GET_VALUE_TYPE(control_value_type) ((control_value_type) & 0x3fff)

/* Entity descriptor Table 7.2 */
struct __attribute__ ((packed)) entity_descriptor {
	avb_u16 descriptor_type;
	avb_u16 descriptor_index;
	avb_u64 entity_id;
	avb_u64 entity_model_id;
	avb_u32 entity_capabilities;
	avb_u16 talker_stream_sources;
	avb_u16 talker_capabilities;
	avb_u16 listener_stream_sinks;
	avb_u16 listener_capabilities;
	avb_u32 controller_capabilities;
	avb_u32 available_index;
	avb_u64 association_id;
	avb_u8 entity_name[64];
	avb_u16 vendor_name_string;
	avb_u16 model_name_string;
	avb_u8 firmware_version[64];
	avb_u8 group_name[64];
	avb_u8 serial_number[64];
	avb_u16 configurations_count;
	avb_u16 current_configuration;
};


/* Configuration descriptor Table 7.3 */
struct __attribute__ ((packed)) configuration_descriptor {
	avb_u16 descriptor_type;
	avb_u16 descriptor_index;
	avb_u8 object_name[64];
	avb_u16 localized_description;
	avb_u16 descriptor_counts_count;
	avb_u16 descriptor_counts_offset;
	avb_u16 descriptors_counts[2 * AEM_NUM_DESCRIPTORS_MAX];
};

/* Audio unit descriptor Table 7.5 */
struct __attribute__ ((packed)) audio_unit_descriptor {
	avb_u16 descriptor_type;
	avb_u16 descriptor_index;
	avb_u8 object_name[64];
	avb_u16 localized_description;
	avb_u16 clock_domain_index;
	avb_u16 number_of_stream_input_ports;
	avb_u16 base_stream_input_port;
	avb_u16 number_of_stream_output_ports;
	avb_u16 base_stream_output_port;
	avb_u16 number_of_external_input_ports;
	avb_u16 base_external_input_port;
	avb_u16 number_of_external_output_ports;
	avb_u16 base_external_output_port;
	avb_u16 number_of_internal_input_ports;
	avb_u16 base_internal_input_port;
	avb_u16 number_of_internal_output_ports;
	avb_u16 base_internal_output_port;
	avb_u16 number_of_controls;
	avb_u16 base_control;
	avb_u16 number_of_signal_selectors;
	avb_u16 base_signal_selector;
	avb_u16 number_of_mixers;
	avb_u16 base_mixer;
	avb_u16 number_of_matrices;
	avb_u16 base_matrix;
	avb_u16 number_of_splitters;
	avb_u16 base_splitter;
	avb_u16 number_of_combiners;
	avb_u16 base_combiner;
	avb_u16 number_of_demultiplexers;
	avb_u16 base_demultiplexer;
	avb_u16 number_of_multiplexers;
	avb_u16 base_multiplexer;
	avb_u16 number_of_transcoders;
	avb_u16 base_transcoder;
	avb_u16 number_of_control_blocks;
	avb_u16 base_control_block;
	avb_u32 current_sampling_rate;
	avb_u16 sampling_rates_offset;
	avb_u16 sampling_rates_count;
	avb_u32 sampling_rates[AEM_NUM_SAMPLING_RATES_MAX];
};

/* Video unit descriptor Table 7.6 */
struct __attribute__ ((packed)) video_unit_descriptor {
	avb_u16 descriptor_type;
	avb_u16 descriptor_index;
	avb_u8 object_name[64];
	avb_u16 localized_description;
	avb_u16 clock_domain_index;
	avb_u16 number_of_stream_input_ports;
	avb_u16 base_stream_input_port;
	avb_u16 number_of_stream_output_ports;
	avb_u16 base_stream_output_port;
	avb_u16 number_of_external_input_ports;
	avb_u16 base_external_input_port;
	avb_u16 number_of_external_output_ports;
	avb_u16 base_external_output_port;
	avb_u16 number_of_internal_input_ports;
	avb_u16 base_internal_input_port;
	avb_u16 number_of_internal_output_ports;
	avb_u16 base_internal_output_port;
	avb_u16 number_of_controls;
	avb_u16 base_control;
	avb_u16 number_of_signal_selectors;
	avb_u16 base_signal_selector;
	avb_u16 number_of_mixers;
	avb_u16 base_mixer;
	avb_u16 number_of_matrices;
	avb_u16 base_matrix;
	avb_u16 number_of_splitters;
	avb_u16 base_splitter;
	avb_u16 number_of_combiners;
	avb_u16 base_combiner;
	avb_u16 number_of_demultiplexers;
	avb_u16 base_demultiplexer;
	avb_u16 number_of_multiplexers;
	avb_u16 base_multiplexer;
	avb_u16 number_of_transcoders;
	avb_u16 base_transcoder;
	avb_u16 number_of_control_blocks;
	avb_u16 base_control_block;
};



/* Stream input descriptor Table 7.8 */
struct __attribute__ ((packed)) stream_descriptor {
	avb_u16 descriptor_type;
	avb_u16 descriptor_index;
	avb_u8 object_name[64];
	avb_u16 localized_description;
	avb_u16 clock_domain_index;
	avb_u16 stream_flags;
	avb_u64 current_format;
	avb_u16 formats_offset;
	avb_u16 number_of_formats;
	avb_u64 backup_talker_entity_id_0;
	avb_u16 backup_talker_unique_id_0;
	avb_u64 backup_talker_entity_id_1;
	avb_u16 backup_talker_unique_id_1;
	avb_u64 backup_talker_entity_id_2;
	avb_u16 backup_talker_unique_id_2;
	avb_u64 backedup_talker_entity_id;
	avb_u16 backedup_talker_unique_id;
	avb_u16 avb_interface_index;
	avb_u32 buffer_length;
	avb_u64 formats[AEM_NUM_FORMATS_MAX];
};

/* Jack descriptor Table 7.10 */
struct __attribute__ ((packed)) jack_descriptor {
	avb_u16 descriptor_type;
	avb_u16 descriptor_index;
	avb_u8 object_name[64];
	avb_u16 localized_description;
	avb_u16 jack_flags;
	avb_u16 jack_type;
	avb_u16 number_of_controls;
	avb_u16 base_control;
};

/* AVB interface descriptor Table 7.13 */
struct __attribute__ ((packed)) avb_interface_descriptor {
	avb_u16 descriptor_type;
	avb_u16 descriptor_index;
	avb_u8 object_name[64];
	avb_u16 localized_description;
	avb_u8 mac_address[6];
	avb_u16 interface_flags;
	avb_u64 clock_identity;
	avb_u8 priority1;
	avb_u8  clock_class;
	avb_u16 offset_scaled_log_variance;
	avb_u8 clock_accuracy;
	avb_u8 priority2;
	avb_u8 domain_number;
	avb_u8 log_sync_interval;
	avb_u8 log_announce_interval;
	avb_u8 log_pdelay_interval;
	avb_u16 port_number;
};

/* Clock source descriptor Table 7.15 */
struct __attribute__ ((packed)) clock_source_descriptor {
	avb_u16 descriptor_type;
	avb_u16 descriptor_index;
	avb_u8 object_name[64];
	avb_u16 localized_description;
	avb_u16 clock_source_flags;
	avb_u16 clock_source_type;
	avb_u64 clock_source_identifier;
	avb_u16 clock_source_location_type;
	avb_u16 clock_source_location_index;
};

/* Locale descriptor Table 7.21 */
struct __attribute__ ((packed)) locale_descriptor {
	avb_u16 descriptor_type;
	avb_u16 descriptor_index;
	avb_u8 locale_identifier[64];
	avb_u16 number_of_strings;
	avb_u16 base_strings;
};

/* Locale descriptor Table 7.22 */
struct __attribute__ ((packed)) strings_descriptor {
	avb_u16 descriptor_type;
	avb_u16 descriptor_index;
	avb_u8 strings_0[64];
	avb_u8 strings_1[64];
	avb_u8 strings_2[64];
	avb_u8 strings_3[64];
	avb_u8 strings_4[64];
	avb_u8 strings_5[64];
	avb_u8 strings_6[64];
};

/* Clock domain Table 7.61 */
struct __attribute__ ((packed)) clock_domain_descriptor {
	avb_u16 descriptor_type;
	avb_u16 descriptor_index;
	avb_u8 object_name[64];
	avb_u16 localized_description;
	avb_u16 clock_source_index;
	avb_u16 clock_sources_offset;
	avb_u16 clock_sources_count;
	avb_u16 clock_sources[AEM_NUM_CLOCK_SOURCES_MAX];
};

/* Stream port Table 7.23 */
struct __attribute__ ((packed)) stream_port_descriptor {
	avb_u16 descriptor_type;
	avb_u16 descriptor_index;
	avb_u16 clock_domain_index;
	avb_u16 port_flags;
	avb_u16 number_of_controls;
	avb_u16 base_control;
	avb_u16 number_of_clusters;
	avb_u16 base_cluster;
	avb_u16 number_of_maps;
	avb_u16 base_map;
};

/* Audio cluster Table 7.26 */
struct __attribute__ ((packed)) audio_cluster_descriptor {
	avb_u16 descriptor_type;
	avb_u16 descriptor_index;
	avb_u8 object_name[64];
	avb_u16 localized_description;
	avb_u16 signal_type;
	avb_u16 signal_index;
	avb_u16 signal_output;
	avb_u32 path_latency;
	avb_u32 block_latency;
	avb_u16 channel_count;
	avb_u8 format;
};

/* Video cluster Table 7.29 */
struct __attribute__ ((packed)) video_cluster_descriptor {
	avb_u16 descriptor_type;
	avb_u16 descriptor_index;
	avb_u8 object_name[64];
	avb_u16 localized_description;
	avb_u16 signal_type;
	avb_u16 signal_index;
	avb_u16 signal_output;
	avb_u32 path_latency;
	avb_u32 block_latency;
	avb_u8 format;
	avb_u32 current_format_specific;
	avb_u16 supported_format_specifics_offset;
	avb_u16 supported_format_specifics_count;
	avb_u32 current_sampling_rate;
	avb_u16 supported_sampling_rates_offset;
	avb_u16 supported_sampling_rates_count;
	avb_u16 current_aspect_ratio;
	avb_u16 supported_aspect_ratios_offset;
	avb_u16 supported_aspect_ratios_count;
	avb_u32 current_size;
	avb_u16 supported_sizes_offset;
	avb_u16 supported_sizes_count;
	avb_u16 current_color_space;
	avb_u16 supported_color_spaces_offset;
	avb_u16 supported_color_spaces_count;

	/* FIXME all the arrays bellow are variable size and must be fixed up at run time*/
	avb_u32 supported_format_specifics[AEN_NUM_VIDEO_FORMATS_MAX];
	avb_u32 supported_sampling_rates[AEM_NUM_VIDEO_RATES_MAX];
	avb_u16 supported_aspect_ratios[AEM_NUM_VIDEO_ASPECT_RATIOS_MAX];
	avb_u32 supported_sizes[AEM_NUM_VIDEO_FRAME_SIZES_MAX];
	avb_u16 supported_color_spaces[AEM_NUM_VIDEO_COLOR_SPACES_MAX];
};

/* Audio map Table 7.33 */
struct __attribute__ ((packed)) audio_mapping {
	avb_u16 mapping_stream_index;
	avb_u16 mapping_stream_channel;
	avb_u16 mapping_cluster_offset;
	avb_u16 mapping_cluster_channel;
};

/* Audio map Table 7.32 */
struct __attribute__ ((packed)) audio_map_descriptor {
	avb_u16 descriptor_type;
	avb_u16 descriptor_index;
	avb_u16 mappings_offset;
	avb_u16 number_of_mappings;
	struct audio_mapping mappings[AEM_NUM_AUDIO_MAPS_MAX];
};


/* Video map Table 7.33 */
struct __attribute__ ((packed)) video_mapping {
	avb_u16 mapping_stream_index;
	avb_u16 mapping_program_stream;
	avb_u16 mapping_elementary_stream;
	avb_u16 mapping_cluster_offset;
};

/* Video map Table 7.34 */
struct __attribute__ ((packed)) video_map_descriptor {
	avb_u16 descriptor_type;
	avb_u16 descriptor_index;
	avb_u16 mappings_offset;
	avb_u16 number_of_mappings;
	struct video_mapping mappings[AEM_NUM_VIDEO_MAPS_MAX];
};


struct __attribute__ ((packed)) external_port_descriptor {
	avb_u16 descriptor_type;
	avb_u16 descriptor_index;
	avb_u16 clock_domain_index;
	avb_u16 port_flags;
	avb_u16 number_of_controls;
	avb_u16 base_control;
	avb_u16 signal_type;
	avb_u16 signal_index;
	avb_u16 signal_output;
	avb_u32 block_latency;
	avb_u16 jack_index;
};

struct __attribute__ ((packed)) value_details_linear_int8 {
	avb_u8 min;
	avb_u8 max;
	avb_u8 step;
	avb_u8 _default;
	avb_u8 current;
	avb_u16 unit;
	avb_u16 string;
};

struct __attribute__ ((packed)) value_details_utf8 {
	avb_u8 string[AEM_UTF8_MAX_LENGTH];
};

/* Control descriptor Table 7.38 */
/* Note : For now only avb_u8 linear value supported */
struct __attribute__ ((packed)) control_descriptor {
	avb_u16 descriptor_type;
	avb_u16 descriptor_index;
	avb_u8 object_name[64];
	avb_u16 localized_description;
	avb_u32 block_latency;
	avb_u32 control_latency;
	avb_u16 control_domain;
	avb_u16 control_value_type;
	avb_u64 control_type;
	avb_u32 reset_time;
	avb_u16 values_offset;
	avb_u16 number_of_values;
	avb_u16 signal_type;
	avb_u16 signal_index;
	avb_u16 signal_output;
	/* FIXME, how to support the various value details at compile time */
	union __attribute__ ((packed)) {
		struct value_details_linear_int8 linear_int8[AEM_NUM_VALUE_DETAILS_LINEAR_U8_MAX];
		struct value_details_utf8 utf8;
	} value_details;
};

#endif /* _GENAVB_PUBLIC_AEM_H_ */
