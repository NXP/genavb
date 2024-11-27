/*
 * Copyright 2017-2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _GSTREAMER_STREAM_H_
#define _GSTREAMER_STREAM_H_

#include "../common/gstreamer_single.h"
#include "../common/gstreamer_multisink.h"
#include "../common/thread_config.h"

/* Default input video file name */
#define DEFAULT_MEDIA_FILE_NAME "sample1.mp4"
/* Default dump file*/
#define DEFAULT_DUMP_LOCATION "/var/avb_listener_dump"

#define MAX_ALSA_TALKERS        	6
#define MAX_ALSA_LISTENERS      	6
#define MAX_GSTREAMER_LISTENERS 	4
#define MAX_GSTREAMER_TALKERS   	4

#define CAMERA_TYPE_SALSA               0
#define CAMERA_TYPE_H264_1722_2016      1
#define CAMERA_TYPE_H264_1722_2013      2

/*Maximum number of pipeline bus to monitor simultaneously for messages*/
#define MAX_GST_PIPELINES_MSG_MONITOR	8
#define GST_BUS_TIMER_TIMEOUT	200000000 /*100 ms*/
struct gstreamer_bus_messages_monitor {
	struct gstreamer_pipeline *gst_pipelines[MAX_GST_PIPELINES_MSG_MONITOR];
	pthread_mutex_t list_lock;
	unsigned int nb_pipelines;
	thr_thread_slot_t *thread_slot;
	int timer_fd;
};

extern struct gstreamer_pipeline gstreamer_listener_pipelines[MAX_GSTREAMER_LISTENERS];
extern struct gstreamer_stream gstreamer_listener_stream[MAX_GSTREAMER_LISTENERS];
extern struct gstreamer_pipeline gstreamer_talker_pipelines[MAX_GSTREAMER_TALKERS];
extern struct gstreamer_stream gstreamer_talker_stream[MAX_GSTREAMER_TALKERS];

void gstreamer_pipeline_init(struct gstreamer_pipeline *pipeline);

int select_gst_listener_pipeline(struct gstreamer_pipeline *gst_pipeline, const struct avdecc_format *format);
int select_prepare_gst_talker_pipeline(struct gstreamer_pipeline *gst_pipeline, const struct avdecc_format *format);
void gst_pipeline_configure_pts_offset(struct gstreamer_pipeline *gst_pipeline, const struct avdecc_format *format);
void dump_gst_config(const struct gstreamer_pipeline *pipeline);

int listener_gstreamer_connect(struct gstreamer_stream *listener, struct avb_stream_params *params, unsigned int avdecc_stream_index);
void listener_gstreamer_disconnect(struct gstreamer_stream *listener);

int talker_gstreamer_connect(struct gstreamer_stream *talker, struct avb_stream_params *params, unsigned int avdecc_stream_index);
int talker_gstreamer_multi_connect(struct avb_stream_params *params, struct gstreamer_talker_multi_handler *talker_multi, unsigned int sink_index, unsigned int stream_index);
void talker_gstreamer_disconnect(struct gstreamer_stream *talker);
void talker_gstreamer_multi_disconnect(struct gstreamer_talker_multi_handler *talker_multi, unsigned int sink_index);

int gstreamer_split_screen_start(struct gstreamer_pipeline *pipeline);

int create_stream_timer(unsigned int interval);

#endif /* _GSTREAMER_STREAM_H_ */
