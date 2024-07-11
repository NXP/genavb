/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2020, 2022-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include "gstreamer.h"
#include "gst_pipeline_definitions.h"


/* Talker pipelines */

struct gstreamer_pipeline_definition pipeline_talker_file_61883_4 = {
		.pipeline_string =
			"filesrc name=filesrc typefind=true ! qtdemux name=demux"
			" demux. ! queue max-size-buffers=0 max-size-time=0 ! audio/mpeg ! mux."
			" demux. ! queue max-size-buffers=0 max-size-time=0 ! video/x-h264 ! h264parse config-interval=1 ! queue max-size-buffers=0 max-size-time=0 ! mux."
			" mpegtsmux name=mux ! queue max-size-buffers=0 max-size-time=0 ! appsink name=sink0",
		.num_sources = 0,
		.num_sinks = 1,
		.prepare = gst_pipeline_prepare_filesrc,
};

struct gstreamer_pipeline_definition pipeline_talker_file_61883_4_61883_6 = {
		.pipeline_string =
			"filesrc name=filesrc typefind=true ! qtdemux name=demux"
			" demux. ! queue max-size-buffers=0 max-size-time=0 ! audio/mpeg ! aacparse ! tee name=t"			/* audio */
			" t. ! queue max-size-buffers=0 max-size-time=0 ! decodebin ! audioresample ! audioconvert ! audio/x-raw,channels=2,format=S24_32BE,rate=48000 ! queue max-size-buffers=0 max-size-time=0 !  appsink name=sink1"
			" t. !  queue max-size-buffers=0 max-size-time=0 ! mux."
			" demux. ! queue max-size-buffers=0 max-size-time=0 ! video/x-h264 ! h264parse config-interval=1 ! queue max-size-buffers=0 max-size-time=0 ! mux." 	/* video */
			" mpegtsmux name=mux !  queue max-size-buffers=0 max-size-time=0 ! appsink name=sink0",
		.num_sources = 0,
		.num_sinks = 2,
		.prepare = gst_pipeline_prepare_filesrc,
};

struct gstreamer_pipeline_definition pipeline_talker_file_61883_4_61883_6_h264 = {
		.pipeline_string =
			"filesrc name=filesrc typefind=true ! qtdemux name=demux"
			" demux. ! queue max-size-buffers=0 max-size-time=0 ! audio/mpeg ! aacparse ! tee name=ta"			/* Audio */
			" ta. ! queue max-size-buffers=0 max-size-time=0 ! decodebin ! audioresample ! audioconvert ! audio/x-raw,channels=2,format=S24_32BE,rate=48000 ! queue max-size-buffers=0 max-size-time=0 !  appsink name=sink1"
			" ta. !  queue max-size-buffers=0 max-size-time=0 ! mux."
			" demux. ! queue max-size-buffers=0 max-size-time=0 ! video/x-h264 ! tee name=tv"
			" tv. ! queue max-size-buffers=0 max-size-time=0 ! h264parse config-interval=1 ! mux." 	/* Video MPEG2TS */
			" tv. ! queue max-size-bytes=0 max-size-buffers=0 max-size-time=0 ! h264parse config-interval=1 ! capsfilter caps=\"video/x-h264, stream-format=byte-stream, alignment=nal\" ! appsink name=sink2" /* Video CVF H264 */
			" mpegtsmux name=mux !  queue max-size-buffers=0 max-size-time=0 ! appsink name=sink0",
		.num_sources = 0,
		.num_sinks = 3,
		.prepare = gst_pipeline_prepare_filesrc,
};

struct gstreamer_pipeline_definition pipeline_talker_file_61883_4_audio_mp3 = {
		.pipeline_string =
			"filesrc name=filesrc typefind=true ! audio/mpeg !"
			" mpegaudioparse ! queue max-size-buffers=0 max-size-time=0 ! mpegtsmux ! queue max-size-buffers=0 max-size-time=0 ! appsink name=sink0",
		.num_sources = 0,
		.num_sinks = 1,
		.prepare = gst_pipeline_prepare_filesrc,
};

struct gstreamer_pipeline_definition pipeline_talker_file_61883_4_audio_m4a = {
		.pipeline_string =
			"filesrc name=filesrc typefind=true ! qtdemux !"
			" audio/mpeg ! aacparse ! queue max-size-buffers=0 max-size-time=0 ! mpegtsmux ! queue max-size-buffers=0 max-size-time=0 ! appsink name=sink0",
		.num_sources = 0,
		.num_sinks = 1,
		.prepare = gst_pipeline_prepare_filesrc,
};

struct gstreamer_pipeline_definition pipeline_talker_file_61883_6_audio_mp3 = {
		.pipeline_string =
			"filesrc name=filesrc typefind=true ! audio/mpeg !"
			" mpegaudioparse ! queue max-size-buffers=0 max-size-time=0 ! decodebin ! audioresample ! audioconvert ! audio/x-raw,channels=2,format=S24_32BE,rate=48000 ! queue max-size-buffers=0 max-size-time=0 ! appsink name=sink0",
		.num_sources = 0,
		.num_sinks = 1,
		.prepare = gst_pipeline_prepare_filesrc,
};

struct gstreamer_pipeline_definition pipeline_talker_file_61883_6_audio_m4a = {
		.pipeline_string =
			"filesrc name=filesrc typefind=true ! qtdemux ! audio/mpeg !"
			" aacparse ! queue max-size-buffers=0 max-size-time=0 ! decodebin ! audioresample ! audioconvert ! audio/x-raw,channels=2,format=S24_32BE,rate=48000 ! queue max-size-buffers=0 max-size-time=0 ! appsink name=sink0",
		.num_sources = 0,
		.num_sinks = 1,
		.prepare = gst_pipeline_prepare_filesrc,
};

struct gstreamer_pipeline_definition pipeline_talker_file_61883_6_audio_wav = {
		.pipeline_string =
			"filesrc name=filesrc typefind=true ! wavparse !"
			" queue max-size-buffers=0 max-size-time=0 ! audioresample ! audioconvert ! audio/x-raw,channels=2,format=S24_32BE,rate=48000 ! queue max-size-buffers=0 max-size-time=0 ! appsink name=sink0",
		.num_sources = 0,
		.num_sinks = 1,
		.prepare = gst_pipeline_prepare_filesrc,
};

struct gstreamer_pipeline_definition pipeline_talker_file_61883_4_61883_6_audio_mp3 = {
		.pipeline_string =
			"filesrc name=filesrc typefind=true ! audio/mpeg !"
			" mpegaudioparse ! tee name=t t. !" /*61883_6 streaming tee*/
			" queue max-size-buffers=0 max-size-time=0 ! decodebin ! audioresample !"
			"  audioconvert ! audio/x-raw,channels=2,format=S24_32BE,rate=48000 ! queue max-size-buffers=0 max-size-time=0 !  appsink name=sink1" /*  61883_4 streaming tee */
			" t. !  queue max-size-buffers=0 max-size-time=0 !"
			" mpegtsmux name=mux !  queue max-size-buffers=0 max-size-time=0 ! appsink name=sink0",
		.num_sources = 0,
		.num_sinks = 2,
		.prepare = gst_pipeline_prepare_filesrc,
};

struct gstreamer_pipeline_definition pipeline_talker_file_61883_4_61883_6_audio_m4a = {
		.pipeline_string =
			"filesrc name=filesrc typefind=true ! qtdemux name=demux demux. ! audio/mpeg !"
			" aacparse ! tee name=t t. !" /*61883_6 streaming tee*/
			" queue max-size-buffers=0 max-size-time=0 ! decodebin ! audioresample !"
			"  audioconvert ! audio/x-raw,channels=2,format=S24_32BE,rate=48000 ! queue max-size-buffers=0 max-size-time=0 !  appsink name=sink1" /*  61883_4 streaming tee */
			" t. !  queue max-size-buffers=0 max-size-time=0 !"
			" mpegtsmux name=mux !  queue max-size-buffers=0 max-size-time=0 ! appsink name=sink0",
		.num_sources = 0,
		.num_sinks = 2,
		.prepare = gst_pipeline_prepare_filesrc,
};

int pipeline_talker_file_preview_prepare(struct gstreamer_pipeline *gst)
{
	gst->video_sink = gst_bin_get_by_name(GST_BIN(gst->pipeline), "videosink");
	if (!gst->video_sink) {
		printf("Error: Video sink not found in pipeline\n");
		goto err_video_sink;
	}

	gst_pipeline_prepare_videosink(gst);

	g_object_set(G_OBJECT(gst->video_sink),
		"ts-offset", (gint64) gst->config.talker.preview_ts_offset,
		NULL);

	if (gst_pipeline_prepare_filesrc(gst) < 0)
		goto err_file_src;

	return 0;

	err_file_src:
	g_object_unref(gst->video_sink);
	gst->video_sink = NULL;

err_video_sink:
	return -1;
}

struct gstreamer_pipeline_definition pipeline_talker_file_61883_4_preview = {
		.pipeline_string =
			"filesrc name=filesrc typefind=true ! qtdemux name=demux"
			" demux. ! queue  max-size-buffers=0 max-size-time=0 ! audio/mpeg ! mux."
			" demux. ! tee name=t"
#ifdef WL_BUILD
			" t. ! queue max-size-buffers=0 max-size-time=0 ! vpudec frame-drop=false ! glimagesink name=videosink force-aspect-ratio=true sync=true async=false" 				/* local display tee */
#else
			" t. ! queue max-size-buffers=0 max-size-time=0 ! vpudec frame-drop=false ! imxv4l2sink name=videosink force-aspect-ratio=true sync=true async=false" 				/* local display tee */
#endif
			" t. ! queue max-size-buffers=0 max-size-time=0 ! video/x-h264 ! h264parse config-interval=1 ! queue max-size-buffers=0 max-size-time=0 ! mux."	/* avb streaming tee */
			" mpegtsmux name=mux ! queue max-size-buffers=0 max-size-time=0 ! appsink name=sink0",
		.num_sources = 0,
		.num_sinks = 1,
		.prepare = pipeline_talker_file_preview_prepare,
};

struct gstreamer_pipeline_definition pipeline_talker_file_61883_4_61883_6_preview = {
		.pipeline_string =
			"filesrc name=filesrc typefind=true ! qtdemux name=demux"
			" demux. ! queue max-size-bytes=200000000 max-size-buffers=0 max-size-time=0 ! audio/mpeg ! aacparse ! tee name=taudio" 		/* audio */
			" taudio. ! queue ! decodebin ! audioconvert ! audio/x-raw,channels=2,format=S24_32BE,rate=48000 ! appsink name=sink1"
			" taudio. ! queue ! mux."
			" demux. ! tee name=tvideo"
#ifdef WL_BUILD
			" tvideo. ! queue ! vpudec frame-drop=false ! glimagesink name=videosink force-aspect-ratio=true sync=true"   				/* local display tee */
#else
			" tvideo. ! queue ! vpudec frame-drop=false ! imxv4l2sink name=videosink force-aspect-ratio=true sync=true"   				/* local display tee */
#endif
			" tvideo. ! queue max-size-bytes=200000000 max-size-buffers=0 max-size-time=0 ! video/x-h264 ! h264parse config-interval=1 ! mux."	/* video */
			" mpegtsmux name=mux alignment=1 ! appsink name=sink0",
		.num_sources = 0,
		.num_sinks = 2,
		.prepare = pipeline_talker_file_preview_prepare,
};

struct gstreamer_pipeline_definition pipeline_talker_file_61883_4_61883_6_h264_preview = {
		.pipeline_string =
			"filesrc name=filesrc typefind=true ! qtdemux name=demux"
			" demux. ! queue max-size-buffers=0 max-size-time=0 ! audio/mpeg ! aacparse ! tee name=ta"			/* audio */
			" ta. ! queue max-size-buffers=0 max-size-time=0 ! decodebin ! audioresample ! audioconvert ! audio/x-raw,channels=2,format=S24_32BE,rate=48000 ! queue max-size-buffers=0 max-size-time=0 !  appsink name=sink1"
			" ta. !  queue max-size-buffers=0 max-size-time=0 ! mux."
			" demux. ! video/x-h264 ! queue max-size-buffers=0 max-size-time=0 ! tee name=tvideo"
			" tvideo. ! queue max-size-buffers=0 max-size-time=0 max-size-bytes=0 ! h264parse config-interval=1 ! tee name=tvideoparsed "
#ifdef WL_BUILD
			" tvideoparsed. ! queue max-size-buffers=0 max-size-time=0 ! vpudec frame-drop=false ! glimagesink name=videosink force-aspect-ratio=true sync=true" /*local display*/
#else
			" tvideoparsed. ! queue max-size-buffers=0 max-size-time=0 ! vpudec frame-drop=false ! imxv4l2sink name=videosink force-aspect-ratio=true sync=true" /*local display*/
#endif
			" tvideoparsed. ! queue max-size-buffers=0 max-size-time=0 ! mux." 	/* video mpegts*/
			" tvideo. ! queue max-size-bytes=0 max-size-buffers=0 max-size-time=0 ! h264parse config-interval=1 ! capsfilter caps=\"video/x-h264, stream-format=byte-stream, alignment=nal\" ! queue max-size-buffers=0 max-size-time=0 ! appsink name=sink2" /*video h264*/
			" mpegtsmux name=mux !  queue max-size-buffers=0 max-size-time=0 ! appsink name=sink0",
		.num_sources = 0,
		.num_sinks = 3,
		.prepare = pipeline_talker_file_preview_prepare,
};

struct gstreamer_pipeline_definition pipeline_talker_file_cvf_h264 = {
		.pipeline_string =
			"filesrc name=filesrc typefind=true ! qtdemux "
			"! queue max-size-bytes=0 max-size-buffers=0 max-size-time=0 "
			"! video/x-h264 ! h264parse config-interval=1 ! queue max-size-bytes=0 max-size-buffers=0 max-size-time=0 ! capsfilter caps=\"video/x-h264, stream-format=byte-stream, alignment=nal\"  "
			"! appsink name=sink0",
		.num_sources = 0,
		.num_sinks = 1,
		.prepare = gst_pipeline_prepare_filesrc,
};

struct gstreamer_pipeline_definition pipeline_talker_file_cvf_h264_preview = {
		.pipeline_string =
			"filesrc name=filesrc typefind=true ! qtdemux name=demux"
			" demux. ! queue max-size-bytes=0 max-size-buffers=0 max-size-time=0 "
			"  ! video/x-h264 ! tee name=t "
			"  t. ! queue max-size-bytes=0 max-size-buffers=0 max-size-time=0 "
			"  ! h264parse ! capsfilter caps=\"video/x-h264, stream-format=byte-stream, alignment=au\" "
#ifdef WL_BUILD
			" ! queue max-size-bytes=0 max-size-buffers=0 max-size-time=0 ! vpudec frame-drop=false ! glimagesink name=videosink force-aspect-ratio=true sync=true" 				/* local display tee */
#else
			" ! queue max-size-bytes=0 max-size-buffers=0 max-size-time=0 ! vpudec frame-drop=false ! imxv4l2sink name=videosink force-aspect-ratio=true sync=true" 				/* local display tee */
#endif
			" t. ! queue max-size-bytes=0 max-size-buffers=0 max-size-time=0 "
			" ! h264parse config-interval=1 ! queue max-size-bytes=0 max-size-buffers=0 max-size-time=0 ! capsfilter caps=\"video/x-h264, stream-format=byte-stream, alignment=nal\" "
			" ! appsink name=sink0",	/* avb streaming tee */
		.num_sources = 0,
		.num_sinks = 1,
		.prepare = pipeline_talker_file_preview_prepare,
};

/* Listener pipelines */
int pipeline_listener_audio_video_prepare(struct gstreamer_pipeline *gst)
{

	gst->audio_sink = gst_bin_get_by_name(GST_BIN(gst->pipeline), "audiosink");
	if (!gst->audio_sink) {
		printf("Error: Audio sink not found in pipeline\n");
		goto err_audio_sink;
	}

	gst->video_sink = gst_bin_get_by_name(GST_BIN(gst->pipeline), "videosink");
	if (!gst->video_sink) {
		printf("Error: Video sink not found in pipeline\n");
		goto err_video_sink;
	}

	gst_pipeline_prepare_videosink(gst);

	gst_set_listener_latency(gst);

	gst_pipeline_prepare_appsrcs(gst);

	gst_pipeline_set_alsasink_config(gst);

	return 0;

err_video_sink:
	g_object_unref(gst->audio_sink);
	gst->audio_sink = NULL;

err_audio_sink:
	return -1;
}

int pipeline_listener_audio_only_prepare(struct gstreamer_pipeline *gst)
{

	gst->audio_sink = gst_bin_get_by_name(GST_BIN(gst->pipeline), "audiosink");
	if (!gst->audio_sink) {
		printf("Error: Audio sink not found in pipeline\n");
		goto err_audio_sink;
	}

	gst_set_listener_latency(gst);

	gst_pipeline_prepare_appsrcs(gst);

	gst_pipeline_set_alsasink_config(gst);

	return 0;

err_audio_sink:
	return -1;
}

int pipeline_listener_video_only_prepare(struct gstreamer_pipeline *gst)
{
	gst->video_sink = gst_bin_get_by_name(GST_BIN(gst->pipeline), "videosink");
	if (!gst->video_sink) {
		printf("Error: Video sink not found in pipeline\n");
		goto err_video_sink;
	}

	gst_pipeline_prepare_videosink(gst);

	gst_set_listener_latency(gst);

	gst_pipeline_prepare_appsrcs(gst);

	return 0;

err_video_sink:
	return -1;
}

int pipeline_listener_cvf_mjpeg_prepare(struct gstreamer_pipeline *gst)
{
	gst->video_sink = gst_bin_get_by_name(GST_BIN(gst->pipeline), "videosink");
	if (!gst->video_sink) {
		printf("Error: Video sink not found in pipeline\n");
		goto err_video_sink;
	}

	gst_pipeline_prepare_videosink(gst);

	gst_pipeline_prepare_appsrcs(gst);

	return 0;

err_video_sink:
	return -1;
}

int pipeline_listener_cvf_h264_prepare(struct gstreamer_pipeline *gst)
{
	gst->video_sink = gst_bin_get_by_name(GST_BIN(gst->pipeline), "videosink");
	if (!gst->video_sink) {
		printf("Error: Video sink not found in pipeline\n");
		goto err_video_sink;
	}

	gst_pipeline_prepare_videosink(gst);

	gst_set_listener_latency(gst);

	gst_pipeline_prepare_appsrcs(gst);

	return 0;

err_video_sink:
	return -1;
}



struct gstreamer_pipeline_definition pipeline_listener_61883_4_audio_video = {
		.pipeline_string =
			"appsrc name=source0 is-live=true ! video/mpegts ! tsdemux name=demux latency=100"
#ifdef WL_BUILD
			" demux. ! video/x-h264 ! h264parse ! queue max-size-bytes=0 max-size-buffers=0 max-size-time=0 ! vpudec frame-drop=false ! glimagesink name=videosink force-aspect-ratio=true max-lateness=1000000 sync=true async=false"
#else
			" demux. ! video/x-h264 ! queue max-size-bytes=0 max-size-buffers=0 max-size-time=0 ! vpudec frame-drop=false ! imxv4l2sink name=videosink force-aspect-ratio=true max-lateness=1000000 sync=true async=false"
#endif
			" demux. ! audio/mpeg ! queue max-size-buffers=0 max-size-time=0 ! decodebin ! audioconvert ! alsasink name=audiosink max-lateness=1000000 alignment-threshold=1000000 latency-time=5000 buffer-time=50000 sync=true",
		.num_sources = 1,
		.num_sinks = 0,
		.latency = TSDEMUX_LATENCY + ALSA_LATENCY,
		.prepare = pipeline_listener_audio_video_prepare,
};

struct gstreamer_pipeline_definition pipeline_listener_61883_4_audio_only = {
		.pipeline_string =
			"appsrc name=source0 is-live=true ! video/mpegts ! tsdemux latency=100"
			" ! audio/mpeg ! queue max-size-buffers=0 max-size-time=0 ! decodebin ! audioconvert"
			" ! alsasink name=audiosink max-lateness=1000000 alignment-threshold=1000000 latency-time=5000 buffer-time=50000 sync=true",
		.num_sources = 1,
		.num_sinks = 0,
		.latency = TSDEMUX_LATENCY + ALSA_LATENCY,
		.prepare = pipeline_listener_audio_only_prepare,
};

struct gstreamer_pipeline_definition pipeline_listener_61883_4_video_only = {
		.pipeline_string =
			"appsrc name=source0 is-live=true ! video/mpegts ! tsdemux latency=100"
#ifdef WL_BUILD
			" ! video/x-h264 ! queue max-size-buffers=0 max-size-time=0 ! vpudec frame-drop=false ! glimagesink name=videosink force-aspect-ratio=true max-lateness=1000000 sync=true",
#else
			" ! video/x-h264 ! queue max-size-buffers=0 max-size-time=0 ! vpudec frame-drop=false ! imxv4l2sink name=videosink force-aspect-ratio=true max-lateness=1000000 sync=true",
#endif
		.num_sources = 1,
		.num_sinks = 0,
		.latency = TSDEMUX_LATENCY,
		.prepare = pipeline_listener_video_only_prepare,
};

struct gstreamer_pipeline_definition pipeline_listener_61883_6 = {
		.pipeline_string =
			"appsrc name=source0 is-live=true"
			" ! audio/x-raw,format=S24_32BE,rate=48000,channels=2,layout=interleaved ! queue max-size-buffers=0 max-size-time=0 ! audioconvert"
			" ! alsasink name=audiosink max-lateness=1000000 alignment-threshold=1000000 latency-time=5000 buffer-time=50000 sync=true",
		.num_sources = 1,
		.num_sinks = 0,
		.latency = ALSA_LATENCY,
		.prepare = pipeline_listener_audio_only_prepare,
};

struct gstreamer_pipeline_definition pipeline_listener_cvf_mjpeg = {
		.pipeline_string =
			"appsrc name=source0 is-live=true"
#ifdef WL_BUILD
			" ! image/jpeg, width=1280, height=800, framerate=30/1 ! vpudec frame-drop=false ! glimagesink name=videosink force-aspect-ratio=true sync=true max-lateness=0",
#else
			" ! image/jpeg, width=1280, height=800, framerate=30/1 ! vpudec frame-drop=false ! imxv4l2sink name=videosink force-aspect-ratio=true sync=true max-lateness=0",
#endif
		.num_sources = 1,
		.num_sinks = 0,
		.latency = MJPEG_PIPELINE_LATENCY,
		.prepare = pipeline_listener_cvf_mjpeg_prepare,
};

struct gstreamer_pipeline_definition pipeline_listener_cvf_h264 = {
		.pipeline_string =
			"appsrc name=source0 is-live=true"
			" ! queue max-size-bytes=0 max-size-buffers=0 max-size-time=0 !  h264parse"
			" ! queue max-size-bytes=0 max-size-buffers=0 max-size-time=0  ! vpudec frame-drop=false"
#ifdef WL_BUILD
			" ! glimagesink name=videosink force-aspect-ratio=true sync=true max-lateness=0",
#else
			" ! imxv4l2sink name=videosink force-aspect-ratio=true sync=true max-lateness=0",
#endif
		.num_sources = 1,
		.num_sinks = 0,
		.latency = H264_PIPELINE_LATENCY,
		.prepare = pipeline_listener_cvf_h264_prepare,
};

int pipeline_video_overlay_prepare(struct gstreamer_pipeline *gst)
{
	GstElement *overlaysink;
	int i, rc;

	gst_blank_screen(gst->config.device);

	gst_pipeline_prepare_appsrcs(gst);


	overlaysink = gst_bin_get_by_name(GST_BIN(gst->pipeline), "videosink3");
	if (!overlaysink) {
		printf("Warning: overlaysink not found in pipeline\n");
		goto err;
	}

	if (gst->config.width == 0) {
		gst_element_set_state(GST_ELEMENT(gst->pipeline), GST_STATE_PAUSED);

		if (!strcmp(gst->config.device,V4L2_LVDS_DEVICE_FILE)) {
			g_object_get(G_OBJECT(overlaysink),
					"overlay-width-1", &gst->config.width,
					"overlay-height-1", &gst->config.height,
					NULL);
		} else if (!strcmp(gst->config.device,V4L2_HDMI_DEVICE_FILE)) {
			g_object_get(G_OBJECT(overlaysink),
					"overlay-width-2", &gst->config.width,
					"overlay-height-2", &gst->config.height,
					NULL);

		}
	}


	if (!strcmp(gst->config.device,V4L2_LVDS_DEVICE_FILE)) {
		g_object_set(G_OBJECT(overlaysink),
				"overlay-width-1", gst->config.width / 2,
				"overlay-height-1", gst->config.height / 2,
				"overlay-left-1", gst->config.width / 2,
				"overlay-top-1", gst->config.height / 2,
				NULL);
	} else if (!strcmp(gst->config.device,V4L2_HDMI_DEVICE_FILE)) {
		g_object_set(G_OBJECT(overlaysink),
				"overlay-width-2", gst->config.width / 2,
				"overlay-height-2", gst->config.height / 2,
				"overlay-left-2", gst->config.width / 2,
				"overlay-top-2", gst->config.height / 2,
				NULL);

	}

	overlaysink = gst_bin_get_by_name(GST_BIN(gst->pipeline), "videosink2");
	if (!overlaysink) {
		printf("Warning: overlaysink not found in pipeline\n");
		goto err;
	}

	if (!strcmp(gst->config.device,V4L2_LVDS_DEVICE_FILE)) {
		g_object_set(G_OBJECT(overlaysink),
				"overlay-width-1", gst->config.width / 2,
				"overlay-height-1", gst->config.height / 2,
				"overlay-top-1", gst->config.height / 2,
				NULL);
	} else if (!strcmp(gst->config.device,V4L2_HDMI_DEVICE_FILE)) {
		g_object_set(G_OBJECT(overlaysink),
				"overlay-width-2", gst->config.width / 2,
				"overlay-height-2", gst->config.height / 2,
				"overlay-top-2", gst->config.height / 2,
				NULL);
	}


	overlaysink = gst_bin_get_by_name(GST_BIN(gst->pipeline), "videosink1");
	if (!overlaysink) {
		printf("Warning: overlaysink not found in pipeline\n");
		goto err;
	}

	if (!strcmp(gst->config.device,V4L2_LVDS_DEVICE_FILE)) {
		g_object_set(G_OBJECT(overlaysink),
				"overlay-width-1", gst->config.width / 2,
				"overlay-height-1", gst->config.height / 2,
				"overlay-left-1", gst->config.width / 2,
				NULL);
	} else if (!strcmp(gst->config.device,V4L2_HDMI_DEVICE_FILE)) {
		g_object_set(G_OBJECT(overlaysink),
				"overlay-width-2", gst->config.width / 2,
				"overlay-height-2", gst->config.height / 2,
				"overlay-left-2", gst->config.width / 2,
				NULL);
	}

	overlaysink = gst_bin_get_by_name(GST_BIN(gst->pipeline), "videosink0");
	if (!overlaysink) {
		printf("Warning: overlaysink not found in pipeline\n");
		goto err;
	}

	if (!strcmp(gst->config.device,V4L2_LVDS_DEVICE_FILE)) {
		g_object_set(G_OBJECT(overlaysink),
				"overlay-width-1", gst->config.width / 2,
				"overlay-height-1", gst->config.height / 2,
				NULL);
	} else if (!strcmp(gst->config.device,V4L2_HDMI_DEVICE_FILE)) {
		g_object_set(G_OBJECT(overlaysink),
				"overlay-width-2", gst->config.width / 2,
				"overlay-height-2", gst->config.height / 2,
				NULL);
	}

	/* Enable the right display for all sinks*/
	for (i = 0; i < 4; i++ ) {

		char sink_name[16];

		rc = snprintf(sink_name, 16, "videosink%d", i);
		if ((rc < 0) || (rc >= 16)) {
			printf("Error %d while generating video sink name\n", rc);
			goto err;
		}

		overlaysink = gst_bin_get_by_name(GST_BIN(gst->pipeline), sink_name);

		if (!strcmp(gst->config.device,V4L2_LVDS_DEVICE_FILE)) {
			g_object_set(G_OBJECT(overlaysink),
					"display-master", 0,
					"display-lvds", 1,
					"display-hdmi", 0,
					NULL);
		} else if (!strcmp(gst->config.device,V4L2_HDMI_DEVICE_FILE)) {
				g_object_set(G_OBJECT(overlaysink),
					"display-master", 0,
					"display-lvds", 0,
					"display-hdmi", 1,
					NULL);
		}
	}

	return 0;
err:
	return -1;
}


struct gstreamer_pipeline_definition pipeline_cvf_mjpeg_four_cameras = {
		.pipeline_string =
			"appsrc name=source0 is-live=true"
			" ! image/jpeg, width=1280, height=800, framerate=30/1 ! vpudec frame-drop=false ! overlaysink name=videosink0 force-aspect-ratio=true sync=true max-lateness=0"
			" appsrc name=source1 is-live=true"
			" ! image/jpeg, width=1280, height=800, framerate=30/1 ! vpudec frame-drop=false ! overlaysink name=videosink1 force-aspect-ratio=true sync=true max-lateness=0"
			" appsrc name=source2 is-live=true"
			" ! image/jpeg, width=1280, height=800, framerate=30/1 ! vpudec frame-drop=false ! overlaysink name=videosink2 force-aspect-ratio=true sync=true max-lateness=0"
			" appsrc name=source3 is-live=true"
			" ! image/jpeg, width=1280, height=800, framerate=30/1 ! vpudec frame-drop=false ! overlaysink name=videosink3 force-aspect-ratio=true sync=true max-lateness=0",
		.num_sources = 4,
		.num_sinks = 0,
		.latency = MJPEG_PIPELINE_LATENCY,
		.prepare = pipeline_video_overlay_prepare,
};



int pipeline_cvf_mjpeg_decode_prepare(struct gstreamer_pipeline *gst)
{
	GstElement *filesink;

	gst_pipeline_prepare_appsrcs(gst);

	filesink = gst_bin_get_by_name(GST_BIN(gst->pipeline), "filesink");
	if (!filesink) {
		printf("Warning: filesink not found in pipeline\n");
		goto err;
	}

	g_object_set(G_OBJECT(filesink),
			"location", gst->config.device,
			NULL);
	return 0;
err:
	return 0;
}

struct gstreamer_pipeline_definition pipeline_cvf_mjpeg_decode_only = {
		.pipeline_string =
			"appsrc name=source0 is-live=true max-bytes=150000 block=true"
			" ! image/jpeg, width=1280, height=800, framerate=30/1 ! tee name=tjpeg"
			" tjpeg. ! videorate skip-to-first=true average-period=1000000000 ! image/jpeg, framerate=1/1 ! queue ! multifilesink name=filesink max-files=60"
			" tjpeg. ! vpudec frame-drop=false output-format=1 frame-plus=8"
			" ! video/x-raw,format=NV12"
			" ! appsink name=sink0 sync=true max-buffers=1 drop=true",
		.num_sources = 1,
		.num_sinks = 1,
		.latency = MJPEG_PIPELINE_LATENCY,
		.prepare = pipeline_cvf_mjpeg_decode_prepare,
};

struct gstreamer_pipeline_definition pipeline_split_screen = {
		.pipeline_string =
			"appsrc name=source0 is-live=true"
			" ! video/x-raw, format=NV12, width=1280, height=800, framerate=30/1 ! queue max-size-bytes=0 max-size-buffers=1 max-size-time=0 leaky=2 ! overlaysink name=videosink0 force-aspect-ratio=true sync=true max-lateness=0"
			" appsrc name=source1 is-live=true"
			" ! video/x-raw, format=NV12, width=1280, height=800, framerate=30/1 ! queue max-size-bytes=0 max-size-buffers=1 max-size-time=0 leaky=2 ! overlaysink name=videosink1 force-aspect-ratio=true sync=true max-lateness=0"
			" appsrc name=source2 is-live=true"
			" ! video/x-raw, format=NV12, width=1280, height=800, framerate=30/1 ! queue max-size-bytes=0 max-size-buffers=1 max-size-time=0 leaky=2 ! overlaysink name=videosink2 force-aspect-ratio=true sync=true max-lateness=0"
			" appsrc name=source3 is-live=true"
			" ! video/x-raw, format=NV12, width=1280, height=800, framerate=30/1 ! queue max-size-bytes=0 max-size-buffers=1 max-size-time=0 leaky=2 ! overlaysink name=videosink3 force-aspect-ratio=true sync=true max-lateness=0",
		.num_sources = 4,
		.num_sinks = 0,
		.latency = OVERLAY_PIPELINE_LATENCY,
		.prepare = pipeline_video_overlay_prepare,
};

struct gstreamer_pipeline_definition pipeline_split_screen_compo = {
		.pipeline_string =
			"imxcompositor_g2d name=comp sink_1::xpos=1440 sink_1::ypos=0 sink_2::xpos=0 sink_2::ypos=810 sink_3::xpos=1440 sink_3::ypos=810 ! overlaysink force-aspect-ratio=true sync=false"
			" appsrc name=source0 is-live=true"
			" ! video/x-raw, format=NV12, width=1280, height=800, framerate=30/1 ! comp.sink_0"
			" appsrc name=source1 is-live=true"
			" ! video/x-raw, format=NV12, width=1280, height=800, framerate=30/1 ! comp.sink_1"
			" appsrc name=source2 is-live=true"
			" ! video/x-raw, format=NV12, width=1280, height=800, framerate=30/1 ! comp.sink_2"
			" appsrc name=source3 is-live=true"
			" ! video/x-raw, format=NV12, width=1280, height=800, framerate=30/1 ! comp.sink_3",
		.num_sources = 4,
		.num_sinks = 0,
		.latency = OVERLAY_PIPELINE_LATENCY,
		.prepare = gst_pipeline_prepare_appsrcs,
};

struct gstreamer_pipeline_definition pipeline_cvf_mjpeg_four_cameras_compo = {
		.pipeline_string =
			"imxcompositor_ipu name=comp sink_1::xpos=1440 sink_1::ypos=0 sink_2::xpos=0 sink_2::ypos=810 sink_3::xpos=1440 sink_3::ypos=810 ! overlaysink force-aspect-ratio=true sync=true"
			" appsrc name=source0 is-live=true"
			" ! image/jpeg, width=1280, height=800, framerate=30/1 ! vpudec frame-drop=false ! comp.sink_0"
			" appsrc name=source1 is-live=true"
			" ! image/jpeg, width=1280, height=800, framerate=30/1 ! vpudec frame-drop=false ! comp.sink_1"
			" appsrc name=source2 is-live=true"
			" ! image/jpeg, width=1280, height=800, framerate=30/1 ! vpudec frame-drop=false ! comp.sink_2"
			" appsrc name=source3 is-live=true"
			" ! image/jpeg, width=1280, height=800, framerate=30/1 ! vpudec frame-drop=false ! comp.sink_3",
		.num_sources = 4,
		.num_sinks = 0,
		.latency = MJPEG_PIPELINE_LATENCY,
		.prepare = gst_pipeline_prepare_appsrcs,
};

struct gstreamer_pipeline_definition pipeline_listener_debug = {
		.pipeline_string =
			"appsrc name=source0 is-live=true"
			" ! filesink name=filesink location=/var/avb_listener_dump",
		.num_sources = 1,
		.num_sinks = 0,
		.prepare = gst_pipeline_prepare_appsrcs_and_filesink,
};
