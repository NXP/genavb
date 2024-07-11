/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2020, 2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "../common/avb_stream_config.h"
#include "../common/crf_stream.h"
#include <genavb/crf.h>


const unsigned int g_max_avb_talker_streams = 6;
const unsigned int g_max_avb_listener_streams = 6;

#define DEFAULT_BATCH_SIZE_NS	1000000

#define AAF_DEFAULT_FORMAT					\
{								\
	.v = 0,							\
	.subtype = AVTP_SUBTYPE_AAF,				\
	.subtype_u.aaf = {					\
		.nsr = AAF_NSR_48000,				\
		.ut = 0,					\
		.rsvd = 0,					\
		.format = AAF_FORMAT_INT_32BIT,			\
		.format_u.pcm = {				\
			.bit_depth = 24,			\
			AAF_PCM_CHANNELS_PER_FRAME_INIT(2),	\
			AAF_PCM_SAMPLES_PER_FRAME_INIT(24),	\
			.reserved_msb = 0,			\
			.reserved_lsb = 0,			\
		}						\
	}							\
}

#define CRF_DEFAULT_FORMAT					\
{								\
	.v = 0,							\
	.subtype = AVTP_SUBTYPE_CRF,				\
	.subtype_u.crf = {					\
		.type = CRF_TYPE_AUDIO_SAMPLE,			\
		CRF_TIMESTAMP_INTERVAL_INIT(320),		\
		.timestamps_per_pdu = 6,			\
		.pull = CRF_PULL_1_1,				\
		CRF_BASE_FREQUENCY_INIT(96000),			\
	}							\
}

aar_avb_stream_t g_avb_talker_streams[] = {
	// Stream 0
	{
		.stream_params = {
			.direction = AVTP_DIRECTION_TALKER,
			.subtype = AVTP_SUBTYPE_AAF,
			.stream_class = SR_CLASS_B,
			.clock_domain = AVB_CLOCK_DOMAIN_0,
			.flags = 0,
			.format.u.s = AAF_DEFAULT_FORMAT,
			.port = AAR_AVB_ETH_PORT,
			.stream_id = { 0x00 },
			.dst_mac = { 0x00 },
			.talker = {
				.latency = DEFAULT_BATCH_SIZE_NS,
			},
		},

		.stream_handle = NULL,

		.batch_size_ns = DEFAULT_BATCH_SIZE_NS,
	},
	// Stream 1
	{
		.stream_params = {
			.direction = AVTP_DIRECTION_TALKER,
			.subtype = AVTP_SUBTYPE_AAF,
			.stream_class = SR_CLASS_B,
			.clock_domain = AVB_CLOCK_DOMAIN_0,
			.flags = 0,
			.format.u.s = AAF_DEFAULT_FORMAT,
			.port = AAR_AVB_ETH_PORT,
			.stream_id = { 0x00 },
			.dst_mac = { 0x00 },
			.talker = {
				.latency = DEFAULT_BATCH_SIZE_NS,
			},
		},

		.stream_handle = NULL,

		.batch_size_ns = DEFAULT_BATCH_SIZE_NS,
	},
	// Stream 2
	{
		.stream_params = {
			.direction = AVTP_DIRECTION_TALKER,
			.subtype = AVTP_SUBTYPE_AAF,
			.stream_class = SR_CLASS_B,
			.clock_domain = AVB_CLOCK_DOMAIN_0,
			.flags = 0,
			.format.u.s = AAF_DEFAULT_FORMAT,
			.port = AAR_AVB_ETH_PORT,
			.stream_id = { 0x00 },
			.dst_mac = { 0x00 },
			.talker = {
				.latency = DEFAULT_BATCH_SIZE_NS,
			},
		},

		.stream_handle = NULL,

		.batch_size_ns = DEFAULT_BATCH_SIZE_NS,
	},
	// Stream 3
	{
		.stream_params = {
			.direction = AVTP_DIRECTION_TALKER,
			.subtype = AVTP_SUBTYPE_AAF,
			.stream_class = SR_CLASS_B,
			.clock_domain = AVB_CLOCK_DOMAIN_0,
			.flags = 0,
			.format.u.s = AAF_DEFAULT_FORMAT,
			.port = AAR_AVB_ETH_PORT,
			.stream_id = { 0x00 },
			.dst_mac = { 0x00 },
			.talker = {
				.latency = DEFAULT_BATCH_SIZE_NS,
			},
		},

		.stream_handle = NULL,

		.batch_size_ns = DEFAULT_BATCH_SIZE_NS,
	},
	// Stream 4
	{
		.stream_params = {
			.direction = AVTP_DIRECTION_TALKER,
			.subtype = AVTP_SUBTYPE_AAF,
			.stream_class = SR_CLASS_B,
			.clock_domain = AVB_CLOCK_DOMAIN_0,
			.flags = 0,
			.format.u.s = AAF_DEFAULT_FORMAT,
			.port = AAR_AVB_ETH_PORT,
			.stream_id = { 0x00 },
			.dst_mac = { 0x00 },
			.talker = {
				.latency = DEFAULT_BATCH_SIZE_NS,
			},
		},

		.stream_handle = NULL,

		.batch_size_ns = DEFAULT_BATCH_SIZE_NS,
	},
	// Stream 5
	{
		.stream_params = {
			.direction = AVTP_DIRECTION_TALKER,
			.subtype = AVTP_SUBTYPE_AAF,
			.stream_class = SR_CLASS_B,
			.clock_domain = AVB_CLOCK_DOMAIN_0,
			.flags = 0,
			.format.u.s = AAF_DEFAULT_FORMAT,
			.port = AAR_AVB_ETH_PORT,
			.stream_id = { 0x00 },
			.dst_mac = { 0x00 },
			.talker = {
				.latency = DEFAULT_BATCH_SIZE_NS,
			},
		},

		.stream_handle = NULL,

		.batch_size_ns = DEFAULT_BATCH_SIZE_NS,
	},
};

aar_avb_stream_t g_avb_listener_streams[] = {
	// Stream 0
	{
		.stream_params = {
			.direction = AVTP_DIRECTION_LISTENER,
			.subtype = AVTP_SUBTYPE_AAF,
			.stream_class = SR_CLASS_B,
			.clock_domain = AVB_CLOCK_DOMAIN_0,
			.flags = 0,
			.format.u.s = AAF_DEFAULT_FORMAT,
			.port = AAR_AVB_ETH_PORT,
			.stream_id = { 0x00 },
			.dst_mac = { 0x00 },
		},

		.stream_handle = NULL,

		.batch_size_ns = DEFAULT_BATCH_SIZE_NS,
	},
	// Stream 1
	{
		.stream_params = {
			.direction = AVTP_DIRECTION_LISTENER,
			.subtype = AVTP_SUBTYPE_AAF,
			.stream_class = SR_CLASS_B,
			.clock_domain = AVB_CLOCK_DOMAIN_0,
			.flags = 0,
			.format.u.s = AAF_DEFAULT_FORMAT,
			.port = AAR_AVB_ETH_PORT,
			.stream_id = { 0x00 },
			.dst_mac = { 0x00 },
		},

		.stream_handle = NULL,

		.batch_size_ns = DEFAULT_BATCH_SIZE_NS,
	},
	// Stream 2
	{
		.stream_params = {
			.direction = AVTP_DIRECTION_LISTENER,
			.subtype = AVTP_SUBTYPE_AAF,
			.stream_class = SR_CLASS_B,
			.clock_domain = AVB_CLOCK_DOMAIN_0,
			.flags = 0,
			.format.u.s = AAF_DEFAULT_FORMAT,
			.port = AAR_AVB_ETH_PORT,
			.stream_id = { 0x00 },
			.dst_mac = { 0x00 },
		},

		.stream_handle = NULL,

		.batch_size_ns = DEFAULT_BATCH_SIZE_NS,
	},
	// Stream 3
	{
		.stream_params = {
			.direction = AVTP_DIRECTION_LISTENER,
			.subtype = AVTP_SUBTYPE_AAF,
			.stream_class = SR_CLASS_B,
			.clock_domain = AVB_CLOCK_DOMAIN_0,
			.flags = 0,
			.format.u.s = AAF_DEFAULT_FORMAT,
			.port = AAR_AVB_ETH_PORT,
			.stream_id = { 0x00 },
			.dst_mac = { 0x00 },
		},

		.stream_handle = NULL,

		.batch_size_ns = DEFAULT_BATCH_SIZE_NS,
	},
	// Stream 4
	{
		.stream_params = {
			.direction = AVTP_DIRECTION_LISTENER,
			.subtype = AVTP_SUBTYPE_AAF,
			.stream_class = SR_CLASS_B,
			.clock_domain = AVB_CLOCK_DOMAIN_0,
			.flags = 0,
			.format.u.s = AAF_DEFAULT_FORMAT,
			.port = AAR_AVB_ETH_PORT,
			.stream_id = { 0x00 },
			.dst_mac = { 0x00 },
		},

		.stream_handle = NULL,

		.batch_size_ns = DEFAULT_BATCH_SIZE_NS,
	},
	// Stream 5
	{
		.stream_params = {
			.direction = AVTP_DIRECTION_LISTENER,
			.subtype = AVTP_SUBTYPE_AAF,
			.stream_class = SR_CLASS_B,
			.clock_domain = AVB_CLOCK_DOMAIN_0,
			.flags = 0,
			.format.u.s = AAF_DEFAULT_FORMAT,
			.port = AAR_AVB_ETH_PORT,
			.stream_id = { 0x00 },
			.dst_mac = { 0x00 },
		},

		.stream_handle = NULL,

		.batch_size_ns = DEFAULT_BATCH_SIZE_NS,
	},
};

aar_crf_stream_t g_crf_streams[] = {
	{
		.stream_params = {
			.direction = AVTP_DIRECTION_LISTENER,
			.subtype = AVTP_SUBTYPE_CRF,
			.stream_class = SR_CLASS_B,
			.clock_domain = AVB_CLOCK_DOMAIN_0,
			.flags = AVB_STREAM_FLAGS_MCR,
			.format.u.s = CRF_DEFAULT_FORMAT,
			.port = AAR_AVB_ETH_PORT,
			.stream_id = {0x00, 0x00, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff},
			.dst_mac = {0x91, 0xe0, 0xf0, 0x00, 0xfe, 0xff},
			.talker = {
				.latency = 0,
			},
		},

		.stream_handle = NULL,
		.batch_size_ns = 0,

		.is_static_config = false,
	},
};
