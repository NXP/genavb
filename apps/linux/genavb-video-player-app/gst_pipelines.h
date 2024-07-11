/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _GST_PIPELINES_H_
#define _GST_PIPELINES_H_

extern char const * const pipeline_talker_file_61883_4;
extern char const * const pipeline_talker_file_61883_4_61883_6;
extern char const * const pipeline_talker_file_61883_4_preview;
extern char const * const pipeline_talker_file_61883_4_61883_6_preview;

extern char const * const pipeline_listener_61883_4_audio_video;
extern char const * const pipeline_listener_61883_4_audio_only;
extern char const * const pipeline_listener_61883_4_video_only;
extern char const * const pipeline_listener_61883_6;
extern char const * const pipeline_listener_cvf_mjpeg;
extern char const * const pipeline_cvf_mjpeg_four_cameras;
extern char const * const pipeline_listener_debug;

#define MPEGTS_LATENCY	100000000	/* Must match latency compiled in gstreamer plugin */
#define ALSA_LATENCY	55000000	/* Must match latency set bellow */

#define LOCAL_PTS_OFFSET		100000000		/* 100ms (in ns) */
#define DEFAULT_PTS_OFFSET 	(LOCAL_PTS_OFFSET + MPEGTS_LATENCY + ALSA_LATENCY)
#define MAX_PTS_OFFSET 		(1 * GST_SECOND)

#define MJPEG_PIPELINE_LATENCY	10000000
#define SALSA_LATENCY		33000000
#define CVF_PTS_OFFSET 		(SALSA_LATENCY + MJPEG_PIPELINE_LATENCY)

extern unsigned long long latency_61883_4_audio_video;
extern unsigned long long latency_61883_4_audio_only;
extern unsigned long long latency_61883_4_video_only;
extern unsigned long long latency_61883_6;
extern unsigned long long latency_cvf_mjpeg;

#endif /* _GST_PIPELINES_H_ */
