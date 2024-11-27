/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _GSTREAMER_MULTISINK_H_
#define _GSTREAMER_MULTISINK_H_

#include <unistd.h>
#include <sys/stat.h>

#include "gstreamer.h"
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define APP_MAX_SUPPORTED_STREAMS	3
#define APP_MAX_ACTIVE_STREAMS	3

#define STR(a) [a] = #a

#define OPT_TYPE_DEFAULT	(1 << 0)
#define OPT_TYPE_PREVIEW	(1 << 1)
#define OPT_TYPE_VIDEO_ONLY	(1 << 2)

#define GST_PRIORITY		1

#define POLL_TIMEOUT_MS			10
#define PIPELINE_LOOP_TIMEOUT		(4000/POLL_TIMEOUT_MS)
#define STREAM_WAIT_DATA_TIMEOUT	(1000/POLL_TIMEOUT_MS + 1)
#define STREAM_PIPELINE_START_TIMEOUT	(10000/POLL_TIMEOUT_MS + 1)
#define STREAM_FLUSH_TIMEOUT		(200/POLL_TIMEOUT_MS)
#define PREVNEXT_STOP_DURATION	(1)  // Stop stream and wait 1s when changing tracks before starting stream again

#define MAX_GST_MULTI_HANDLERS	1
#define MAX_GST_MULTI_HANDLERS_STREAMS	(MAX_GST_MULTI_HANDLERS * GST_MAX_SINKS)

enum gst_state {
	GST_STATE_STARTING,	/**< transition state to GST_STATE_STARTED */
	GST_STATE_STARTED,	/**< pipeline is started. At least one stream is started. */
	GST_STATE_STOPPED,	/**< pipeline is stopped. All streams are stopped. */
	GST_STATE_LOOP		/**< pipeline is stopped. All streams are looping. Waiting for timeout before switching to GST_STATE_STARTING. */
};

enum stream_state {
	STREAM_STATE_CONNECTING,	/**< transition state to CONNECTED */
	STREAM_STATE_CONNECTED,		/**< avb stream is open and being polled, actively reading from media stack */
	STREAM_STATE_DISCONNECTING,	/**< transition state to DISCONNECTED */
	STREAM_STATE_DISCONNECTED,	/**< avb stream is closed, not reading from media stack */
	STREAM_STATE_WAIT_DATA,		/**< avb stream is open but not polled, actively reading from media stack (based on a timer). On timeout transition to STREAM_STATE_LOOP */
	STREAM_STATE_WAIT_START,	/**< avb stream is open but not polled, waiting for the pipeline to finish its start. On aync msg reception from pipeline, transition to STREAM_STATE_WAIT_DATA or on timeout expiration transition to STREAM_STATE_DISCONNECTED */
	STREAM_STATE_LOOP,		/**< avb stream is open but not polled, not reading from media stack. On timeout, loop media file and transition to STREAM_STATE_CONNECTING */
	STREAM_STATE_STOPPED		/**< avb stream is open but stopped, not reading from media stack. On play, transition to STREAM_STATE_CONNECTING. */
};

enum stream_event {
	STREAM_EVENT_CONNECT,		/**< Connect event from control handler */
	STREAM_EVENT_DISCONNECT,	/**< Disconnect event from control handler */
	STREAM_EVENT_DATA,		/**< Data event from stream file descriptor */
	STREAM_EVENT_TIMER,		/**< Timer event from thread loop */
	STREAM_EVENT_LOOP_DONE,		/**< Loop done event from pipeline state machine */
	STREAM_EVENT_PLAY,		/**< Play event from external AVDECC controller */
	STREAM_EVENT_STOP		/**< Stop event from external AVDECC controller */
};

enum gst_event {
	GST_EVENT_START,	/**< start event from stream state machine */
	GST_EVENT_STOP,		/**< stop event from stream state machine */
	GST_EVENT_LOOP,		/**< loop event from gst state machine, when all streams are in looping state */
	GST_EVENT_TIMER		/**< timer event from thread loop timeout */
};

struct talker_gst_multi_app {  //TODO: merge with struct media_stream
	struct talker_gst_media *gst;
	int index;

	unsigned int sink_index;

	unsigned int audio;

	GstAppSink *sink;
	GstSample *current_sample;
	GstBuffer *current_buffer;
	GstMapInfo current_info;
	void *current_data;
	unsigned int current_size;
	unsigned long long samples;
	unsigned int last_ts; /*Used for H264 Stream :
				Gstreamer sometimes output buffers with an invalid PTS , so use the last valid one */

	unsigned int timer_count;

	struct stats delay;
	unsigned long long byte_count;
	unsigned int count;

	enum stream_state state;

	unsigned int ts_parser_enabled;

	/* Handler for stream poll set*/
	void (*stream_poll_set) (void *, int);
	void *stream_poll_data;

	struct avb_stream_handle *stream_h;
	struct avb_stream_params params;
	unsigned int batch_size;
	pthread_mutex_t samples_lock;
};

struct talker_gst_media {    //TODO: merge with struct media_stream and talker_gst_multi_app
	struct gstreamer_pipeline *gst_pipeline;

	pthread_mutex_t stream_lock;

	enum gst_state state;

	unsigned int timer_count;
};

struct gstreamer_talker_multi_handler {
	int configured_streams;
	int connected_streams;
	struct talker_gst_media *gst_media;
	struct gstreamer_pipeline gst_pipeline;
	int timer_fd;
	void *thread_timer;
	struct talker_gst_multi_app *multi_app[GST_MAX_SINKS];
	struct gstreamer_stream *talkers[GST_MAX_SINKS];
};

struct gstreamer_listener_multi_handler {
	int configured_streams;
	int type;
	struct gstreamer_pipeline gst_pipeline;
};

int talker_gst_multi_data_handler(struct talker_gst_multi_app *stream);
int talker_gst_multi_stream_fsm(struct talker_gst_multi_app *stream, enum stream_event event);
int talker_gst_multi_fsm(struct talker_gst_media *gst, enum gst_event event);

#ifdef __cplusplus
}
#endif

#endif /* _GSTREAMER_MULTISINK_H_ */
