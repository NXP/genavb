/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sched.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <limits.h>
#include <inttypes.h>

#include "gstreamer.h"
#include "../common/time.h"


static void gst_get_latency(struct gstreamer_pipeline *gst)
{
	GstQuery *q;

	q = gst_query_new_latency();
	if (gst_element_query(gst->pipeline, q)) {
		gboolean live;
		GstClockTime minlat, maxlat;

		gst_query_parse_latency(q, &live, &minlat, &maxlat);

		printf("Pipeline latency: %ju-%ju ns\n", minlat, maxlat);
	}

	gst_query_unref(q);
}

static void gst_set_latency(struct gstreamer_pipeline *gst)
{
	if (gst->video_sink) {
		g_object_set(G_OBJECT(gst->video_sink),
			"render-delay", gst->pts_offset - gst->pipeline_latency,
			NULL);

		printf("Set video sink render-delay: %ju\n", gst->pts_offset - gst->pipeline_latency);

		if (gst->audio_sink) {
			g_object_set(G_OBJECT(gst->audio_sink),
				"render-delay", gst->pts_offset - gst->pipeline_latency,
				NULL);

			printf("Set audio sink render-delay: %ju\n", gst->pts_offset - gst->pipeline_latency);
		}
	}

	if (gst->audio_sink) {
		/* Audio sink render delay affects the overall pipeline latency but is not having any effect on the actual audio playback
		 * set also ts-offset as a workaround */
		g_object_set(G_OBJECT(gst->audio_sink),
			"ts-offset", gst->pts_offset - gst->pipeline_latency,
			NULL);

		printf("Set audio sink ts-offset: %ju\n", gst->pts_offset - gst->pipeline_latency);
	}
}


static void gst_blank_screen(char *device_str)
{
	int rc;
	uint64_t now, then;
	GstElement *video_sink;
	GstElement *pipeline;
	GstBus *bus;
	GstMessage *msg;

	gettime_us(&now);

	pipeline = gst_parse_launch("videotestsrc pattern=black num-buffers=4 !video/x-raw,format=YV12 !imxv4l2sink name=videosink overlay-width=2000 overlay-height=2000", NULL);
	if (!pipeline)
		goto err_pipeline;

	video_sink = gst_bin_get_by_name(GST_BIN(pipeline), "videosink");
	if (!video_sink)
		goto err_video;

	g_object_set(G_OBJECT(video_sink), "device", device_str, NULL);

	rc = gst_element_set_state(pipeline, GST_STATE_PLAYING);
	if (rc == GST_STATE_CHANGE_FAILURE)
		goto err_state;

	bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
	if (!bus)
		goto err_bus;

	msg = gst_bus_timed_pop_filtered(bus, GST_SECOND, GST_MESSAGE_EOS);
	if (msg)
		gst_message_unref(msg);

	gst_element_set_state(pipeline, GST_STATE_NULL);

	gettime_us(&then);

	printf("Blanking screen took %" PRId64 " us\n", then - now);

	gst_object_unref(bus);

err_bus:
err_state:
	gst_object_unref(video_sink);

err_video:
	gst_object_unref(pipeline);

err_pipeline:
	return;
}

static int gst_config_pipeline(struct gstreamer_pipeline *gst)
{
	int i;

	gst_blank_screen(gst->device);

	gst->clock = g_object_new (GST_TYPE_SYSTEM_CLOCK, "name", "GstSystemClock", NULL);
	if (!gst->clock) {
		printf("Error: could not obtain system clock.\n");
		goto err_clock;
	}
	g_object_set(G_OBJECT(gst->clock), "clock-type", GST_CLOCK_TYPE_REALTIME, NULL);

	gst->time = gst_clock_get_time(gst->clock);
	gst_pipeline_use_clock(GST_PIPELINE(gst->pipeline), gst->clock);

#if 0
	if (gst->direction == GST_DIRECTION_LISTENER) {
		/* Enable to use absolute gPTP time as the running-time of the pipeline, useful for synchronization debugging */
		gst_element_set_start_time(gst->pipeline, GST_CLOCK_TIME_NONE);
		gst_element_set_base_time(gst->pipeline, 0);
	}
#endif

	gst->video_sink = gst_bin_get_by_name(GST_BIN(gst->pipeline), "videosink");
	if (!gst->video_sink) {
		printf("Warning: Video sink not found in pipeline\n");
	} else {
		g_object_set(G_OBJECT(gst->video_sink), "device", gst->device,
				"crop-width", gst->crop_width,
				"crop-height", gst->crop_height,
				"overlay-width", gst->overlay_width,
				"overlay-height", gst->overlay_height,
				NULL);

		/* Direction specific settings */
		if (gst->direction == GST_DIRECTION_LISTENER) {

		} else {
			g_object_set(G_OBJECT(gst->video_sink),
				"ts-offset", gst->u.talker.preview_ts_offset,
				NULL);
		}
	}

	gst->audio_sink = gst_bin_get_by_name(GST_BIN(gst->pipeline), "audiosink");
	if (!gst->audio_sink)
		printf("Warning: Audio sink not found in pipeline\n");

	if (gst->direction == GST_DIRECTION_LISTENER) {
		gst_set_latency(gst);

		for (i = 0; i < gst->u.listener.num_sources; i++) {
			GstAppSrc *source = gst->u.listener.source[i].source;

			g_object_set(G_OBJECT(source), "format", GST_FORMAT_TIME, NULL);
		}

	} else {
		GstElement *element;

		element = gst_bin_get_by_name(GST_BIN(gst->pipeline), "filesrc");
		if (!element) {
			printf("File source element not found in pipeline\n");
			goto err_file_src;
		}

		g_object_set(G_OBJECT(element), "location", gst->u.talker.file_src_location, NULL);
		gst_object_unref(element);
	}

	return 0;

err_file_src:
	if (gst->video_sink) {
		g_object_unref(gst->video_sink);
		gst->video_sink = NULL;
	}

	if (gst->audio_sink) {
		g_object_unref(gst->audio_sink);
		gst->audio_sink = NULL;
	}

	gst_object_unref(gst->clock);

err_clock:
	return -1;
}

static int gst_get_sources_sinks(struct gstreamer_pipeline *gst)
{
	int i, j, rc;

	if (gst->direction == GST_DIRECTION_LISTENER) {
		for (i = 0; i < gst->u.listener.num_sources; i++) {
			GstAppSrc *source;
			char source_name[16];

			rc = snprintf(source_name, 16, "source%d", i);
			if ((rc < 0) || (rc >= 16)) {
				printf("Error while generating source name, rc: %d\n", rc);
				goto err;
			}
			source = GST_APP_SRC(gst_bin_get_by_name(GST_BIN(gst->pipeline), source_name));
			if (!source) {
				printf("gst_bin_get_by_name(%s) failed\n", source_name);
				goto err;
			}

			gst->u.listener.source[i].source = source;
		}
	} else {
		for (i = 0; i < gst->u.talker.num_sinks; i++) {
			GstAppSink *sink;
			char sink_name[16];

			rc = snprintf(sink_name, 16, "sink%d", i);
			if ((rc < 0) || (rc >= 16)) {
				printf("Error while generating sink name, rc: %d\n", rc);
				goto err;
			}
			sink = GST_APP_SINK(gst_bin_get_by_name(GST_BIN(gst->pipeline), sink_name));
			if (!sink) {
				printf("gst_bin_get_by_name(%s) failed\n", sink_name);
				goto err;
			}

			gst->u.talker.sink[i].sink = sink;
		}
	}

	return 0;

err:
	if (gst->direction == GST_DIRECTION_LISTENER) {
		for (j = 0; j < i; j++)
			gst_object_unref(gst->u.listener.source[j].source);
	} else {
		for (j = 0; j < i; j++)
			gst_object_unref(gst->u.talker.sink[j].sink);
	}

	return -1;
}

static void gst_release_sources_sinks(struct gstreamer_pipeline *gst)
{
	int i;

	if (gst->direction == GST_DIRECTION_LISTENER) {
		for (i = 0; i < gst->u.listener.num_sources; i++)
			gst_object_unref(gst->u.listener.source[i].source);
	} else {
		for (i = 0; i < gst->u.talker.num_sinks; i++)
			gst_object_unref(gst->u.talker.sink[i].sink);
	}
}

static void gst_teardown_pipeline(struct gstreamer_pipeline *gst)
{
	if (gst->video_sink) {
		g_object_unref(gst->video_sink);
		gst->video_sink = NULL;
	}

	if (gst->audio_sink) {
		g_object_unref(gst->audio_sink);
		gst->audio_sink = NULL;
	}

	gst_object_unref(gst->clock);
}

int gst_stop_pipeline(struct gstreamer_pipeline *gst)
{
	int rc;

	rc = gst_element_set_state(gst->pipeline, GST_STATE_NULL);
	if (rc == GST_STATE_CHANGE_FAILURE)
		printf("Unable to set the pipeline to the NULL state.\n");

	gst_release_sources_sinks(gst);

	gst_teardown_pipeline(gst);

	gst_object_unref(gst->bus);

	gst_object_unref(gst->pipeline);

	return rc;
}

int gst_setup_pipeline(struct gstreamer_pipeline *gst, int priority, unsigned int direction)
{
	gst->direction = direction;

	gst->pipeline = gst_parse_launch((const gchar *)gst->pipeline_string, NULL);
	if (!gst->pipeline) {
		printf("gst_parse_launch() failed\n");
		goto err_pipeline;
	}

	if (gst_get_sources_sinks(gst) < 0)
		goto err_src_sink;

	if (gst_config_pipeline(gst) < 0)
		goto err_setup;

	gst->bus = gst_pipeline_get_bus(GST_PIPELINE(gst->pipeline));
	if (!gst->bus) {
		printf("gst_pipeline_get_bus() failed\n");
		goto err_bus;
	}

	return 0;

err_bus:
	gst_teardown_pipeline(gst);

err_setup:
	gst_release_sources_sinks(gst);

err_src_sink:
	gst_object_unref(gst->pipeline);

err_pipeline:
	return -1;
}

int gst_play_pipeline(struct gstreamer_pipeline *gst)
{
	int rc;

	gst_bus_set_flushing(gst->bus, TRUE);
	gst_bus_set_flushing(gst->bus, FALSE);

	rc = gst_element_set_state(GST_ELEMENT(gst->pipeline), GST_STATE_PLAYING);
	if (rc == GST_STATE_CHANGE_FAILURE) {
		printf("Unable to set the pipeline to the playing state.\n");
		goto err_state;
	}

	gst->basetime = GST_CLOCK_TIME_NONE;

	return 0;

err_state:
	gst_stop_pipeline(gst);

	return -1;
}

int gst_start_pipeline(struct gstreamer_pipeline *gst, int priority, unsigned int direction)
{
	int rc;

	rc = gst_setup_pipeline(gst, priority, direction);
	if (rc < 0)
		goto err_setup;

	rc = gst_play_pipeline(gst);

	return rc;

err_setup:
	return -1;
}

void gst_process_bus_messages(struct gstreamer_pipeline *gst)
{
	GstMessage *msg;
	GError *err;
	gchar *debug_info;

	while (gst_bus_have_pending(gst->bus)) {

		msg = gst_bus_pop_filtered(gst->bus, GST_MESSAGE_ERROR | GST_MESSAGE_WARNING | GST_MESSAGE_EOS | GST_MESSAGE_ASYNC_DONE);
		if (!msg)
			break;

		switch (GST_MESSAGE_TYPE(msg)) {
		case GST_MESSAGE_ASYNC_DONE:
			gst_get_latency(gst);

			break;

		case GST_MESSAGE_STATE_CHANGED: {
			GstState old_state, new_state, pending_state;

			gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);

			g_print ("Element %s changed state from %s to %s (target %s)\n", GST_OBJECT_NAME(msg->src),
				gst_element_state_get_name(old_state), gst_element_state_get_name(new_state), gst_element_state_get_name(pending_state));

			break;
		}

		case GST_MESSAGE_ERROR:
			gst_message_parse_error(msg, &err, &debug_info);

			printf("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
			printf("Debugging information: %s\n", debug_info ? debug_info : "none");

			g_clear_error(&err);
			g_free(debug_info);

			break;

		case GST_MESSAGE_WARNING:
			gst_message_parse_warning(msg, &err, &debug_info);

			printf("Warning received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
			printf("Debugging information: %s\n", debug_info ? debug_info : "none");

			g_clear_error(&err);
			g_free(debug_info);

			break;

		case GST_MESSAGE_INFO:
			gst_message_parse_info(msg, &err, &debug_info);
			printf("Info received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
			printf("Debugging information: %s\n", debug_info ? debug_info : "none");
			g_clear_error(&err);
			g_free(debug_info);
			break;

		case GST_MESSAGE_EOS:
			printf("End-Of-Stream reached.\n");
			break;

		default:
			/* We should not reach here because we only asked for ERRORs and EOS */
			break;
		}

		gst_message_unref (msg);
	}
}

void gstreamer_init(void)
{
	gst_init(NULL, NULL);
}

void gstreamer_reset(void)
{
	gst_deinit();
}
