/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _GST_PIPELINE_DEFINITIONS_H_
#define _GST_PIPELINE_DEFINITIONS_H_

#include <gst/gst.h>

struct gstreamer_pipeline;

struct gstreamer_pipeline_definition {
	char const * const pipeline_string;
	unsigned int const num_sources;
	unsigned int const num_sinks;
	GstClockTime const latency;

	int (*prepare)(struct gstreamer_pipeline *gst_pipeline);
};

extern struct gstreamer_pipeline_definition pipeline_talker_file_61883_4;
extern struct gstreamer_pipeline_definition pipeline_talker_file_61883_4_61883_6;
extern struct gstreamer_pipeline_definition pipeline_talker_file_61883_4_61883_6_h264;
extern struct gstreamer_pipeline_definition pipeline_talker_file_61883_4_61883_6_h264_preview;
extern struct gstreamer_pipeline_definition pipeline_talker_file_61883_4_preview;
extern struct gstreamer_pipeline_definition pipeline_talker_file_61883_4_61883_6_preview;
extern struct gstreamer_pipeline_definition pipeline_talker_file_61883_4_audio_mp3;
extern struct gstreamer_pipeline_definition pipeline_talker_file_61883_4_audio_m4a;
extern struct gstreamer_pipeline_definition pipeline_talker_file_61883_6_audio_mp3;
extern struct gstreamer_pipeline_definition pipeline_talker_file_61883_6_audio_m4a;
extern struct gstreamer_pipeline_definition pipeline_talker_file_61883_6_audio_wav;
extern struct gstreamer_pipeline_definition pipeline_talker_file_61883_4_61883_6_audio_mp3;
extern struct gstreamer_pipeline_definition pipeline_talker_file_61883_4_61883_6_audio_m4a;
extern struct gstreamer_pipeline_definition pipeline_talker_file_cvf_h264;
extern struct gstreamer_pipeline_definition pipeline_talker_file_cvf_h264_preview;

extern struct gstreamer_pipeline_definition pipeline_listener_61883_4_audio_video;
extern struct gstreamer_pipeline_definition pipeline_listener_61883_4_audio_only;
extern struct gstreamer_pipeline_definition pipeline_listener_61883_4_video_only;
extern struct gstreamer_pipeline_definition pipeline_listener_61883_6;
extern struct gstreamer_pipeline_definition pipeline_listener_cvf_mjpeg;
extern struct gstreamer_pipeline_definition pipeline_listener_cvf_h264;
extern struct gstreamer_pipeline_definition pipeline_cvf_mjpeg_four_cameras;
extern struct gstreamer_pipeline_definition pipeline_cvf_mjpeg_decode_only;
extern struct gstreamer_pipeline_definition pipeline_split_screen;
extern struct gstreamer_pipeline_definition pipeline_split_screen_compo;
extern struct gstreamer_pipeline_definition pipeline_cvf_mjpeg_four_cameras_compo;
extern struct gstreamer_pipeline_definition pipeline_listener_debug;

#define TSDEMUX_LATENCY			100000000	/* Must match latency compiled in gstreamer plugin */
#define ALSA_LATENCY			55000000	/* Must match the sum of  buffer time (size) and period time in alsasink plugin */
#define MPEGTS_PIPELINE_LATENCY		(TSDEMUX_LATENCY + ALSA_LATENCY) /*MPEGTS pipeline have tsdemux plugin and alsasink plugin advertising their minimum latency*/
#define H264_PIPELINE_LATENCY		0 /*H264 pipeline have no plugin advertising a minimum latency*/

#define LOCAL_PTS_OFFSET		150000000		/* PTS offset to compensate for global pipeline processing time: 150ms (in ns) */
#define DEFAULT_PTS_OFFSET		(LOCAL_PTS_OFFSET + MPEGTS_PIPELINE_LATENCY) /*Default and common PTS offset to guarantee sync between different listener pipelines*/
#define MAX_PTS_OFFSET			(1 * GST_SECOND)

#define MJPEG_PIPELINE_LATENCY		8000000
#define SALSA_LATENCY			33000000
#define CVF_MJPEG_PTS_OFFSET		(SALSA_LATENCY + MJPEG_PIPELINE_LATENCY)
#define OVERLAY_PIPELINE_LATENCY	2000000

#endif /* _GST_PIPELINE_DEFINITIONS_H_ */
