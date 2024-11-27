/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "gst_pipelines.h"


/* Talker pipelines */

char const * const pipeline_talker_file_61883_4 =
	"filesrc name=filesrc typefind=true ! qtdemux name=demux"
	" demux. ! queue ! video/x-h264 ! h264parse config-interval=1 ! mux."
	" demux. ! queue ! audio/mpeg ! mux."
	" mpegtsmux name=mux ! appsink name=sink0"
;

char const * const pipeline_talker_file_61883_4_61883_6 =
	"filesrc name=filesrc typefind=true ! qtdemux name=demux"
	" demux. ! queue ! video/x-h264 ! h264parse config-interval=1 ! mux." 	/* video */
	" demux. ! queue ! audio/mpeg ! aacparse ! tee name=t"			/* audio */
	" t. ! queue ! avdec_aac ! audioconvert ! audio/x-raw,channels=2,format=S24_32BE,rate=48000 ! appsink name=sink1"
	" t. ! queue ! mux."
	" mpegtsmux name=mux ! appsink name=sink0"
;

char const * const pipeline_talker_file_61883_4_preview =
	"filesrc name=filesrc typefind=true ! qtdemux name=demux"
	" demux. ! queue max-size-bytes=200000000 max-size-buffers=0 max-size-time=0 ! audio/mpeg ! mux."
	" demux. ! tee name=t"
#ifdef WL_BUILD
	" t. ! queue ! vpudec frame-drop=false ! glimagesink name=videosink force-aspect-ratio=false sync=true" 				/* local display tee */
#else
	" t. ! queue ! vpudec frame-drop=false ! imxv4l2sink name=videosink force-aspect-ratio=true sync=true" 				/* local display tee */
#endif
	" t. ! queue max-size-bytes=200000000 max-size-buffers=0 max-size-time=0 ! video/x-h264 ! h264parse config-interval=1 ! mux."	/* avb streaming tee */
	" mpegtsmux name=mux ! appsink name=sink0"
;

char const * const pipeline_talker_file_61883_4_61883_6_preview =
	"filesrc name=filesrc typefind=true ! qtdemux name=demux"
	" demux. ! tee name=tvideo"
#ifdef WL_BUILD
	" tvideo. ! queue ! vpudec frame-drop=false ! glimagesink  name=videosink force-aspect-ratio=false sync=true"   				/* local display tee */
#else
	" tvideo. ! queue ! vpudec frame-drop=false ! imxv4l2sink name=videosink force-aspect-ratio=true sync=true"   				/* local display tee */
#endif
	" tvideo. ! queue max-size-bytes=200000000 max-size-buffers=0 max-size-time=0 ! video/x-h264 ! h264parse config-interval=1 ! mux."	/* video */
	" demux. ! queue max-size-bytes=200000000 max-size-buffers=0 max-size-time=0 ! audio/mpeg ! aacparse ! tee name=taudio" 		/* audio */
	" taudio. ! queue ! avdec_aac ! audioconvert ! audio/x-raw,channels=2,format=S24_32BE,rate=48000 ! appsink name=sink1"
	" taudio. ! queue ! mux."
	" mpegtsmux name=mux alignment=1 ! appsink name=sink0"
;


/* Listener pipelines */

char const * const pipeline_listener_61883_4_audio_video =
	"appsrc name=source0 is-live=true ! tsdemux name=demux"
#ifdef WL_BUILD
	" demux. ! video/x-h264 ! queue max-size-bytes=200000000 max-size-buffers=0 max-size-time=0 ! vpudec frame-drop=false ! glimagesink name=videosink force-aspect-ratio=false max-lateness=1000000 sync=true"
#else
	" demux. ! video/x-h264 ! queue max-size-bytes=200000000 max-size-buffers=0 max-size-time=0 ! vpudec frame-drop=false ! imxv4l2sink name=videosink force-aspect-ratio=true max-lateness=1000000 sync=true"
#endif
	" demux. ! audio/mpeg ! queue max-size-bytes=200000000 max-size-buffers=0 max-size-time=0 ! aacparse ! avdec_aac ! audioconvert ! alsasink name=audiosink device=default max-lateness=1000000 alignment-threshold=1000000 latency-time=5000 buffer-time=50000 sync=true"
;

char  const * const pipeline_listener_61883_4_audio_only =
	"appsrc name=source0 is-live=true ! tsdemux"
	" ! audio/mpeg ! queue ! aacparse ! avdec_aac ! audioconvert"
	" ! alsasink name=audiosink device=default max-lateness=1000000 alignment-threshold=1000000 latency-time=5000 buffer-time=50000 sync=true"
;

char const * const pipeline_listener_61883_4_video_only =
	"appsrc name=source0 is-live=true ! tsdemux"
#ifdef WL_BUILD
	" ! video/x-h264 ! queue ! vpudec frame-drop=false ! glimagesink name=videosink force-aspect-ratio=false max-lateness=1000000 sync=true"
#else
	" ! video/x-h264 ! queue ! vpudec frame-drop=false ! imxv4l2sink name=videosink force-aspect-ratio=true max-lateness=1000000 sync=true"
#endif
;


char const * const pipeline_listener_61883_6 =
	"appsrc name=source0 is-live=true"
	" ! audio/x-raw,format=S24_32BE,rate=48000,channels=2 ! queue max-size-bytes=200000000 max-size-buffers=0 max-size-time=0 ! audioconvert"
	" ! alsasink name=audiosink device=default max-lateness=1000000 alignment-threshold=1000000 latency-time=5000 buffer-time=50000 sync=true"
;

char const * const pipeline_listener_cvf_mjpeg =
	"appsrc name=source0 is-live=true"
	" ! image/jpeg, width=1280, height=800, framerate=30/1 ! vpudec frame-drop=false ! imxv4l2sink name=videosink0 force-aspect-ratio=true sync=true max-lateness=0"
;

char const * const pipeline_cvf_mjpeg_four_cameras =
	"appsrc name=source0 is-live=true"
	" ! image/jpeg, width=1280, height=800, framerate=30/1 ! vpudec frame-drop=false ! overlaysink name=videosink0 force-aspect-ratio=true sync=true max-lateness=0"
	" appsrc name=source1 is-live=true"
	" ! image/jpeg, width=1280, height=800, framerate=30/1 ! vpudec frame-drop=false ! overlaysink name=videosink1 force-aspect-ratio=true sync=true max-lateness=0"
	" appsrc name=source2 is-live=true"
	" ! image/jpeg, width=1280, height=800, framerate=30/1 ! vpudec frame-drop=false ! overlaysink name=videosink2 force-aspect-ratio=true sync=true max-lateness=0"
	" appsrc name=source3 is-live=true"
	" ! image/jpeg, width=1280, height=800, framerate=30/1 ! vpudec frame-drop=false ! overlaysink name=videosink3 force-aspect-ratio=true sync=true max-lateness=0"
;

char const * const pipeline_listener_debug =
	"appsrc name=source0 is-live=true"
	" ! filesink location=/storage/dump.ts"
;


unsigned long long latency_61883_4_audio_video = MPEGTS_LATENCY + ALSA_LATENCY;
unsigned long long latency_61883_4_audio_only = MPEGTS_LATENCY + ALSA_LATENCY;
unsigned long long latency_61883_4_video_only = MPEGTS_LATENCY;
unsigned long long latency_61883_6 = ALSA_LATENCY;
unsigned long long latency_cvf_mjpeg = MJPEG_PIPELINE_LATENCY;
