/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _GSTREAMER_H_
#define _GSTREAMER_H_

#include <unistd.h>

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>

#define LVDS_DEVICE_FILE	"/dev/video16"
#define HDMI_DEVICE_FILE	"/dev/video18"

#define GST_DIRECTION_LISTENER	0
#define GST_DIRECTION_TALKER	1


#ifdef __cplusplus
extern "C" {
#endif

#define GST_MAX_SOURCES		4
#define GST_MAX_SINKS		2

struct gstreamer_sink {
	void *data;

	GstAppSink *sink;
};

struct gstreamer_source {
	void *data;

	GstAppSrc *source;
};


#define	GST_TYPE_VIDEO 	(1 << 0)
#define GST_TYPE_AUDIO 	(1 << 1)
#define GST_TYPE_CAMERA (1 << 2)


struct gstreamer_pipeline_config {
	unsigned int type;
	char *device;
	unsigned int width;
	unsigned int height;
	unsigned int nstreams;   // Number of streams that this pipeline will handle
	GstClockTime pts_offset;

};

struct gstreamer_pipeline {
	struct gstreamer_pipeline_config config;
	char const * pipeline_string;
	unsigned int crop_width;
	unsigned int crop_height;
	unsigned int overlay_width;
	unsigned int overlay_height;
	char *device;

	unsigned int direction;

	union {
		struct {
			int num_sources;

			struct gstreamer_source source[GST_MAX_SOURCES];
		} listener;

		struct {
			int num_sinks;

			struct gstreamer_sink sink[GST_MAX_SINKS];

			char *file_src_location;
			unsigned int preview_ts_offset;
			unsigned int sync;
		} talker;
	} u;

	GstElement *pipeline;
	GstBus *bus;
	GstClock *clock;
	GstElement *video_sink;
	GstElement *audio_sink;
	GstClockTime time;
	GstClockTime pipeline_latency;
	GstClockTime pts_offset;
	unsigned long long local_pts_offset;
	unsigned long long basetime;
};

int gst_setup_pipeline(struct gstreamer_pipeline *gst, int priority, unsigned int direction);
int gst_play_pipeline(struct gstreamer_pipeline *gst);
int gst_start_pipeline(struct gstreamer_pipeline *gst, int priority, unsigned int direction);

void gst_process_bus_messages(struct gstreamer_pipeline *gst);

int gst_stop_pipeline(struct gstreamer_pipeline *gst);

void gstreamer_init(void);
void gstreamer_reset(void);


#ifdef __cplusplus
}
#endif

#endif /* _GSTREAMER_H_ */
