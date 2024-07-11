/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2020, 2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _GSTREAMER_H_
#define _GSTREAMER_H_

#include <unistd.h>
#include <stdint.h>
#include <dirent.h>

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <gst/audio/gstaudiobasesink.h>

#include "gst_pipeline_definitions.h"

#define V4L2_LVDS_DEVICE_FILE	"/dev/video16"
#define V4L2_HDMI_DEVICE_FILE	"/dev/video18"
#define DEFAULT_LVDS_HEIGHT	768
#define DEFAULT_LVDS_WIDTH	1024
#define DEFAULT_HDMI_HEIGHT	1080
#define DEFAULT_HDMI_WIDTH	1920

#define GST_DIRECTION_LISTENER	0
#define GST_DIRECTION_TALKER	1

#define GST_THREADS_PRIORITY		1
#define GST_THREADS_SCHED_POLICY	SCHED_FIFO


#ifdef __cplusplus
extern "C" {
#endif

#define GST_MAX_SOURCES		4
#define GST_MAX_SINKS		3

struct gstreamer_sink {
	void *data;

	GstAppSink *sink;
};

struct gstreamer_source {
	void *data;

	GstAppSrc *source;
};

struct stream_ids_map {
	uint64_t stream_id;
	unsigned int source_index;
	unsigned int sink_index;
};

/*General config flags for talker and/or listener */
#define	GST_TYPE_VIDEO 		(1 << 0)
#define GST_TYPE_AUDIO 		(1 << 1)
#define GST_TYPE_LISTENER 	(1 << 2)
#define GST_TYPE_TALKER 	(1 << 3)
#define GST_TYPE_MULTI_TALKER 	(1 << 4)
/*Specific flags for talker or listener*/
#define GST_FLAG_CAMERA 	(1 << 0)
#define GST_FLAG_BEV 		(1 << 1)
#define GST_FLAG_PREVIEW	(1 << 2)
#define GST_FLAG_DEBUG		(1 << 3)

struct gstreamer_pipeline_config {
	unsigned int type;
	char *device;
	char *alsa_device;
	unsigned int width;
	unsigned int height;
	unsigned int crop_width;
	unsigned int crop_height;
	unsigned int nstreams;   // Number of streams that this pipeline will handle
	unsigned int configured;
	GstAudioBaseSinkSlaveMethod sink_slave_method;

	unsigned int sync_render_to_clock;
	struct {
		unsigned int flags;
		GstClockTime pts_offset;
		unsigned int camera_type;
		unsigned long long h264_stream_id;
		struct stream_ids_map stream_ids_mappping[GST_MAX_SOURCES];
		char *debug_file_dump_location;
	} listener;

	struct {
		unsigned int flags;
		char *input_media_file_name;
		char **input_media_files;
		unsigned char	 input_media_file_index;
		unsigned int n_input_media_files;
		char *file_src_location;
		unsigned long long preview_ts_offset;
	} talker;
};


struct gstreamer_pipeline {
	struct gstreamer_pipeline_config config;
	struct gstreamer_pipeline_definition *definition;

	unsigned int direction;

	struct gstreamer_source source[GST_MAX_SOURCES];
	struct gstreamer_sink sink[GST_MAX_SINKS];

	struct {
		unsigned int sync;
		struct talker_gst_media *gst;
		struct talker_gst_multi_app *stream[GST_MAX_SINKS];
		unsigned int nb_streams;
	} talker;

	struct {
		GstClockTime pts_offset;
		unsigned long long local_pts_offset;
	} listener;

	GstElement *pipeline;
	GstBus *bus;
	GstClock *clock;
	GstElement *video_sink;
	GstElement *audio_sink;
	GstClockTime time;
	GstClockTime basetime;
	GstTaskPool *pool;
	unsigned int async_msg_received;
	pthread_t event_loop_tid;
	pthread_mutex_t msg_lock;
	int (*custom_gst_pipeline_setup)(struct gstreamer_pipeline *gst);
	void (*custom_gst_pipeline_teardown)(struct gstreamer_pipeline *gst);
};


void gst_pipeline_set_alsasink_config(struct gstreamer_pipeline *gst);
void gst_blank_screen(char *device_str);
void gst_set_listener_latency(struct gstreamer_pipeline *gst);
int gst_pipeline_prepare_videosink(struct gstreamer_pipeline *gst);
int gst_pipeline_prepare_appsrcs(struct gstreamer_pipeline *gst);
int gst_pipeline_prepare_filesrc(struct gstreamer_pipeline *gst);
int gst_pipeline_prepare_appsrcs_and_filesink(struct gstreamer_pipeline *gst);

int gst_setup_pipeline(struct gstreamer_pipeline *gst, int priority, unsigned int direction);
int gst_play_pipeline(struct gstreamer_pipeline *gst);
int gst_start_pipeline(struct gstreamer_pipeline *gst, int priority, unsigned int direction);

void gst_blank_screen(char *device_str);

void gst_process_bus_messages(struct gstreamer_pipeline *gst);

int gst_stop_pipeline(struct gstreamer_pipeline *gst);

void gstreamer_init(void);
void gstreamer_reset(void);

int gst_build_media_file_list(struct gstreamer_pipeline_config *gst_config);


#ifdef __cplusplus
}
#endif

#endif /* _GSTREAMER_H_ */
