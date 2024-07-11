/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2020, 2022-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sched.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "gstreamer.h"
#include "time.h"

#define DUMP_GST_THREADS 	0

void gst_pipeline_set_alsasink_config(struct gstreamer_pipeline *gst)
{
	const char* GstAudioBaseSinkSlaveMethodNames[] = {
			"GST_AUDIO_BASE_SINK_SLAVE_RESAMPLE",
			"GST_AUDIO_BASE_SINK_SLAVE_SKEW",
			"GST_AUDIO_BASE_SINK_SLAVE_NONE",
			"GST_AUDIO_BASE_SINK_SLAVE_CUSTOM"};

	if (gst->audio_sink != NULL) {
		g_object_set(G_OBJECT(gst->audio_sink),
							  "slave-method",
							  gst->config.sink_slave_method,
							  NULL);

		printf("Set pipeline audio_sink slave-method : %s\n", GstAudioBaseSinkSlaveMethodNames[gst->config.sink_slave_method]);

		g_object_set(G_OBJECT(gst->audio_sink),
							  "device",
							  gst->config.alsa_device,
							  NULL);

		printf("Set pipeline audio_sink device : %s\n", gst->config.alsa_device);
	} else {
		printf("Cannot set pipeline audio_sink configuration (gst->audio_sink is NULL)\n");
	}
}

static GstBusSyncReply sync_bus_handler (GstBus * bus, GstMessage * message, struct gstreamer_pipeline *gst)
{
	switch (GST_MESSAGE_TYPE (message)) {
	case GST_MESSAGE_STREAM_STATUS:
	{
		GstStreamStatusType type;
		GstElement *owner;
		const GValue *val;
#if DUMP_GST_THREADS
		gchar *path;
#endif
		GstTask *task = NULL;

		gst_message_parse_stream_status (message, &type, &owner);

		val = gst_message_get_stream_status_object (message);
#if DUMP_GST_THREADS
		g_message ("type:   %d", type);
		path = gst_object_get_path_string (GST_MESSAGE_SRC (message));
		g_message ("source: %s", path);
		g_free (path);
		path = gst_object_get_path_string (GST_OBJECT (owner));
		g_message ("owner:  %s", path);
		g_free (path);

		if (G_VALUE_HOLDS_OBJECT (val)) {
			g_message ("object: type %s, value %p", G_VALUE_TYPE_NAME (val),
			g_value_get_object (val));
		} else if (G_VALUE_HOLDS_POINTER (val)) {
			g_message ("object: type %s, value %p", G_VALUE_TYPE_NAME (val),
			g_value_get_pointer (val));
		} else if (G_IS_VALUE (val)) {
			g_message ("object: type %s", G_VALUE_TYPE_NAME (val));
		} else {
			g_message ("object: (null)");
			break;
		}
#endif
		/* see if we know how to deal with this object */
		if (G_VALUE_TYPE (val) == GST_TYPE_TASK) {
			task = g_value_get_object (val);
		}

		switch (type) {
		case GST_STREAM_STATUS_TYPE_CREATE:
			if (task) {
				printf("GST_STREAM_STATUS_TYPE_CREATE message received:  set pool (%p) for task (%p) \n", gst->pool, task);
				gst_task_set_pool (task, gst->pool);
			}
		break;
		case GST_STREAM_STATUS_TYPE_ENTER:
		break;
		case GST_STREAM_STATUS_TYPE_LEAVE:
		break;
		default:
		break;
		}
		break;
	}
	case GST_MESSAGE_ASYNC_DONE:
	{
		pthread_mutex_lock(&gst->msg_lock);
		gst->async_msg_received = 1;
		pthread_mutex_unlock(&gst->msg_lock);

		break;
	}
	default:
	  break;
	}
/* pass all messages on the async queue */
return GST_BUS_PASS;
}


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

void gst_set_listener_latency(struct gstreamer_pipeline *gst)
{
	if (gst->video_sink) {
		g_object_set(G_OBJECT(gst->video_sink),
			"ts-offset", gst->listener.pts_offset - gst->definition->latency,
			NULL);

		printf("Set video sink ts-offset: %ju\n", gst->listener.pts_offset - gst->definition->latency);

	}

	if (gst->audio_sink) {
		g_object_set(G_OBJECT(gst->audio_sink),
			"ts-offset", gst->listener.pts_offset - gst->definition->latency,
			NULL);

		printf("Set audio sink ts-offset: %ju\n", gst->listener.pts_offset - gst->definition->latency);
	}
}


void gst_blank_screen(char *device_str)
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

static int gst_pipeline_prepare_clock(struct gstreamer_pipeline *gst)
{
	gst->clock = g_object_new (GST_TYPE_SYSTEM_CLOCK, "name", "GstSystemClock", NULL);
	if (!gst->clock) {
		printf("Error: could not obtain system clock.\n");
		goto err;
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

	return 0;

err:
	return -1;
}

int gst_pipeline_prepare_videosink(struct gstreamer_pipeline *gst)
{
	gchar *factory_name;
	factory_name = GST_OBJECT_NAME (gst_element_get_factory(gst->video_sink));

	if (!strcmp(factory_name, "glimagesink")) {

		GValue render_rectangle = G_VALUE_INIT;
		GValue val = G_VALUE_INIT;
		g_value_init (&val, G_TYPE_INT);
		g_value_init (&render_rectangle, GST_TYPE_ARRAY);

		g_value_set_int (&val, 0);
		gst_value_array_append_value (&render_rectangle, &val);
		g_value_set_int (&val, 0);
		gst_value_array_append_value (&render_rectangle, &val);
		g_value_set_int (&val, gst->config.width);
		gst_value_array_append_value (&render_rectangle, &val);
		g_value_set_int (&val, gst->config.height);
		gst_value_array_append_value (&render_rectangle, &val);

		g_value_unset (&val);

		g_object_set_property (G_OBJECT(gst->video_sink), "render-rectangle",
			&render_rectangle);

		g_value_unset (&render_rectangle);

	} else if (!strcmp(factory_name, "imxv4l2sink")) {

		gst_blank_screen(gst->config.device);

		g_object_set(G_OBJECT(gst->video_sink),
				"device", gst->config.device,
				"crop-width", gst->config.crop_width,
				"crop-height", gst->config.crop_height,
				"overlay-width", gst->config.width,
				"overlay-height", gst->config.height,
				NULL);
	}

	if (!gst->config.sync_render_to_clock)
		printf("This pipeline (%p) will not sync to clock on video rendering \n", gst);

	g_object_set(G_OBJECT(gst->video_sink),
			"sync", gst->config.sync_render_to_clock,
			NULL);

	return 0;
}

int gst_pipeline_prepare_appsrcs(struct gstreamer_pipeline *gst)
{
	unsigned int i;

	for (i = 0; i < gst->definition->num_sources; i++) {
		GstAppSrc *source = gst->source[i].source;

		g_object_set(G_OBJECT(source), "format", GST_FORMAT_TIME, NULL);
	}

	return 0;
}

int gst_pipeline_prepare_appsrcs_and_filesink(struct gstreamer_pipeline *gst)
{
	unsigned int i;
	GstElement *element;

	for (i = 0; i < gst->definition->num_sources; i++) {
		GstAppSrc *source = gst->source[i].source;

		g_object_set(G_OBJECT(source), "format", GST_FORMAT_TIME, NULL);
	}

	element = gst_bin_get_by_name(GST_BIN(gst->pipeline), "filesink");
	if (!element) {
		printf("File sink element not found in pipeline\n");
		return -1;
	}

	g_object_set(G_OBJECT(element), "location", gst->config.listener.debug_file_dump_location, NULL);
	gst_object_unref(element);

	return 0;
}

int gst_pipeline_prepare_filesrc(struct gstreamer_pipeline *gst)
{
	GstElement *element;

	element = gst_bin_get_by_name(GST_BIN(gst->pipeline), "filesrc");
	if (!element) {
		printf("File source element not found in pipeline\n");
		goto err;
	}

	g_object_set(G_OBJECT(element), "location", gst->config.talker.file_src_location, NULL);
	gst_object_unref(element);

	return 0;

err:
	return -1;
}

static int gst_get_sources_sinks(struct gstreamer_pipeline *gst)
{
	int i, j = 0, rc;

	for (i = 0; i < gst->definition->num_sources; i++) {
		GstAppSrc *source;
		char source_name[16];

		rc = snprintf(source_name, 16, "source%d", i);
		if ((rc < 0) || (rc >= 16)) {
			printf("Error %d while generating source name \n", rc);
			goto err;
		}

		source = GST_APP_SRC(gst_bin_get_by_name(GST_BIN(gst->pipeline), source_name));
		if (!source) {
			printf("gst_bin_get_by_name(%s) failed\n", source_name);
			goto err;
		}

		gst->source[i].source = source;
	}

	for (j = 0; j < gst->definition->num_sinks; j++) {
		GstAppSink *sink;
		char sink_name[16];

		rc = snprintf(sink_name, 16, "sink%d", j);
		if ((rc < 0) || (rc >= 16)) {
			printf("Error %d while generating sink name \n", rc);
			goto err;
		}

		sink = GST_APP_SINK(gst_bin_get_by_name(GST_BIN(gst->pipeline), sink_name));
		if (!sink) {
			printf("gst_bin_get_by_name(%s) failed\n", sink_name);
			goto err;
		}

		gst->sink[j].sink = sink;
	}


	return 0;

err:
	while (i > 0) {
		gst_object_unref(gst->source[i - 1].source);
		i--;
	}

	while (j > 0) {
		gst_object_unref(gst->sink[j - 1].sink);
		j--;
	}

	return -1;
}

static void gst_release_sources_sinks(struct gstreamer_pipeline *gst)
{
	int i;

	for (i = 0; i < gst->definition->num_sources; i++)
		gst_object_unref(gst->source[i].source);

	for (i = 0; i < gst->definition->num_sinks; i++)
		gst_object_unref(gst->sink[i].sink);
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
}

int gst_stop_pipeline(struct gstreamer_pipeline *gst)
{
	int rc;

	rc = gst_element_set_state(gst->pipeline, GST_STATE_NULL);
	if (rc == GST_STATE_CHANGE_FAILURE)
		printf("Unable to set the pipeline to the NULL state.\n");
	else if (rc == GST_STATE_CHANGE_ASYNC) {
		printf("Changing Asynchronously to the NULL state.\n");
		rc = gst_element_get_state (gst->pipeline, NULL, NULL, GST_CLOCK_TIME_NONE);
		if (rc != GST_STATE_CHANGE_SUCCESS)
			printf("Unable to set the pipeline synchronously to the NULL state.\n");
	}

	if (gst->custom_gst_pipeline_teardown)
		gst->custom_gst_pipeline_teardown(gst);

	gst_release_sources_sinks(gst);

	gst_teardown_pipeline(gst);

	gst_object_unref(gst->clock);

	gst_object_unref(gst->bus);

	gst_object_unref(gst->pipeline);

	return rc;
}

int gst_setup_pipeline(struct gstreamer_pipeline *gst, int priority, unsigned int direction)
{
	gst->direction = direction;

	gst->pipeline = gst_parse_launch((const gchar *)gst->definition->pipeline_string, NULL);
	if (!gst->pipeline) {
		printf("gst_parse_launch() failed\n");
		goto err_pipeline;
	}

	if (gst_get_sources_sinks(gst) < 0)
		goto err_src_sink;

	if (gst_pipeline_prepare_clock(gst) < 0)
		goto err_clock;

	if (gst->definition->prepare(gst) < 0)
		goto err_setup;

	gst->bus = gst_pipeline_get_bus(GST_PIPELINE(gst->pipeline));
	if (!gst->bus) {
		printf("gst_pipeline_get_bus() failed\n");
		goto err_bus;
	}

	if (gst->custom_gst_pipeline_setup && gst->custom_gst_pipeline_setup(gst)) {
		printf("custom_gst_pipeline_setup() failed\n");
		goto error_custom_setup;
	}

	return 0;

error_custom_setup:
	gst_object_unref(gst->bus);

err_bus:
	gst_teardown_pipeline(gst);

err_setup:
	gst_object_unref(gst->clock);

err_clock:
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

	printf("GStreamer : enable the verbose output for the gst-pipeline \n");
	g_signal_connect( gst->pipeline, "deep-notify", G_CALLBACK( gst_object_default_deep_notify ), NULL );

	printf("GStreamer : Install sync_bus_handler \n");

	gst_bus_set_sync_handler (gst->bus, (GstBusSyncHandler) sync_bus_handler, gst,
NULL);

	pthread_mutex_lock(&gst->msg_lock);
	gst->async_msg_received = 0;
	pthread_mutex_unlock(&gst->msg_lock);

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

	printf("%s : Starting pipeline: %s \n", __func__, gst->definition->pipeline_string);

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
	GstMessage *msg = NULL;
	GError *err;
	gchar *debug_info;

	while (gst_bus_have_pending(gst->bus)) {
		msg = gst_bus_pop_filtered(gst->bus, GST_MESSAGE_ERROR | GST_MESSAGE_WARNING | GST_MESSAGE_EOS | GST_MESSAGE_STATE_CHANGED);

		if (!msg)
			break;

		switch (GST_MESSAGE_TYPE(msg)) {

		case GST_MESSAGE_STATE_CHANGED: {
			GstState old_state, new_state, pending_state;

			gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);

			/* Only check pipeline state change messages */
			if (GST_MESSAGE_SRC (msg) != GST_OBJECT_CAST (gst->pipeline))
				break;


			if ( new_state == GST_STATE_PLAYING) {
				g_print ("Element %s changed state from %s to %s (target %s)\n", GST_OBJECT_NAME(msg->src),
					gst_element_state_get_name(old_state), gst_element_state_get_name(new_state), gst_element_state_get_name(pending_state));
				gst_get_latency(gst);
			}

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

/*
 * scandir will skip the file if input_file_filter_mp4 returns 0,
 * and add it to the list otherwise.
 */
static int input_file_filter_mp4(const struct dirent *file)
{
	if (!strcasestr(file->d_name, ".mp4"))
		return 0;
	else
		return 1;

}

int gst_build_media_file_list(struct gstreamer_pipeline_config *gst_config)
{
	struct stat status;
	struct dirent **file_list;
	int rc = 0;
	int n = 1;
	int i = 0;
	int len;


	if (stat(gst_config->talker.input_media_file_name, &status) < 0) {
		printf("Couldn't get file status for %s, got error %s\n", gst_config->talker.input_media_file_name, strerror(errno));
		goto err;
	}

	if (S_ISDIR(status.st_mode)) {
		n = scandir(gst_config->talker.input_media_file_name, &file_list, input_file_filter_mp4, alphasort);
		if (n < 0) {
			printf("Couldn't scan directory %s, got error %s\n", gst_config->talker.input_media_file_name, strerror(errno));
			goto err;
		}
		if (n == 0) {
			printf("Didn't find any media files in directory %s\n", gst_config->talker.input_media_file_name);
			goto err;
		}

		gst_config->talker.n_input_media_files = n;
		gst_config->talker.input_media_files = malloc(n * sizeof(char *));

		if (!gst_config->talker.input_media_files) {
			printf("Error while allocating gst talker's %d input media files\n", n);
			goto err_inputs_alloc;
		}

		for (i = 0; i < n; i++) {
			len = strlen(gst_config->talker.input_media_file_name) + strlen(file_list[i]->d_name) + 1;
			gst_config->talker.input_media_files[i] = malloc(len);

			if (!gst_config->talker.input_media_files[i]) {
				printf("Error while allocating gst talker's input media files at index %d\n", i);
				goto err_input_member_alloc;
			}

			rc = snprintf(gst_config->talker.input_media_files[i], len, "%s%s", gst_config->talker.input_media_file_name, file_list[i]->d_name);
			if ((rc < 0) || (rc >= len)) {
				printf("Error %d while generating filenames\n", rc);
				goto err_name_gen;
			}

		}

	} else {
		gst_config->talker.n_input_media_files = 1;
		gst_config->talker.input_media_files = malloc(sizeof(char *));

		if (!gst_config->talker.input_media_files) {
			printf("Error while allocating gst talker's input media files\n");
			goto err_inputs_alloc;
		}

		*gst_config->talker.input_media_files = gst_config->talker.input_media_file_name;
	}

	return 0;

err_name_gen:
	free(gst_config->talker.input_media_files[i]);

err_input_member_alloc:
	while (i--) {
		free(gst_config->talker.input_media_files[i]);
	}

	free(gst_config->talker.input_media_files);

err_inputs_alloc:
err:
	return -1;
}
