/*
 * Copyright 2017-2020, 2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>

#include <genavb/genavb.h>

#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>

#include "gstreamer_stream.h"
#include "../common/avb_stream.h"
#include "../common/thread.h"
#include "../common/gstreamer_single.h"
#include "../common/gst_pipeline_definitions.h"
#include "../common/gstreamer_custom_rt_pool.h"
#include "../common/clock_domain.h"
#include "../common/timer.h"

#define NSEC_PER_SEC		1000000000

#define MAX_CONNECTED_GST_TALKERS	1
#define MAX_CONNECTED_GST_LISTENERS	1

#define DEFAULT_GST_ALSA_DEVICE	"plughw:0,0"

static int connected_gst_talkers = 0;
static int connected_gst_listeners = 0;

/* Single Stream handler structs*/
static struct talker_gst_media  gstreamer_talker_media[MAX_GSTREAMER_TALKERS] = { [0 ... MAX_GSTREAMER_TALKERS - 1 ].stream_lock = PTHREAD_MUTEX_INITIALIZER };
static struct talker_gst_multi_app gstreamer_talker_multi_app[MAX_GSTREAMER_TALKERS] = { [0 ... MAX_GSTREAMER_TALKERS - 1].samples_lock = PTHREAD_MUTEX_INITIALIZER};

void gstreamer_pipeline_init(struct gstreamer_pipeline *pipeline)
{
	GstTaskPool *pool;
	memset(pipeline, 0, sizeof(struct gstreamer_pipeline));

	/*By default, the skew sync slave method is enabled */
	pipeline->config.sink_slave_method = GST_AUDIO_BASE_SINK_SLAVE_SKEW;

	/*By default we sync to clock on render*/
	pipeline->config.sync_render_to_clock = 1;
	pipeline->config.listener.pts_offset = GST_CLOCK_TIME_NONE;

	pipeline->config.alsa_device = DEFAULT_GST_ALSA_DEVICE;

	/* create a custom thread pool */
	pool = avb_rt_pool_new ();
	printf("[GSTREAMER] Creating Custom Task pool %p \n",pool);
	pipeline->pool = pool;
}

void dump_gst_config(const struct gstreamer_pipeline *pipeline)
{
	printf("\nPLAYOUT TYPE: \n");
	if (pipeline->config.type & GST_TYPE_LISTENER)
		printf("LISTENER: \n");
	if (pipeline->config.type & GST_TYPE_TALKER)
		printf("TALKER: \n");
	if (pipeline->config.type & GST_TYPE_VIDEO)
		printf("\t -> Video \n");
	if (pipeline->config.type & GST_TYPE_AUDIO)
		printf("\t -> Audio \n");
	if (pipeline->config.listener.flags & GST_FLAG_DEBUG)
		printf("\t -> Debug Mode : dump to file \n");
	if (pipeline->config.listener.flags & GST_FLAG_CAMERA)
		printf("\t -> Camera \n");
	if (pipeline->config.listener.flags & GST_FLAG_BEV)
		printf("\t -> BEV Mode \n");
	if (pipeline->config.talker.flags & GST_FLAG_PREVIEW)
		printf("\t -> Preview mode : render on server \n");
	if (((pipeline->config.type & GST_TYPE_LISTENER) && (pipeline->config.type & GST_TYPE_VIDEO))
	|| ((pipeline->config.type & GST_TYPE_TALKER) && (pipeline->config.talker.flags & GST_FLAG_PREVIEW))) {
		printf("\nDISPLAY DEVICE: %s", pipeline->config.device);
		printf("\nDISPLAY SCALING: %dx%d\n", pipeline->config.width, pipeline->config.height);
		printf("\t -> Presentation offset %llu ns\n", (pipeline->config.type & GST_TYPE_TALKER) ? pipeline->config.talker.preview_ts_offset : pipeline->config.listener.pts_offset);
	}

	if ((pipeline->config.type & GST_TYPE_LISTENER) && (pipeline->config.type & GST_TYPE_AUDIO)) {
		printf("\nALSA DEVICE: %s", pipeline->config.alsa_device);
	}

}

int create_stream_timer(unsigned int interval_ms)
{
	int timer_fd;

	timer_fd = create_timerfd_periodic(CLOCK_REALTIME);
	if (timer_fd < 0)
		goto err;

	if (start_timerfd_periodic(timer_fd, 0, interval_ms * NSECS_PER_MSEC) < 0)
		goto err_close;

	return timer_fd;

err_close:
	close(timer_fd);

err:
	return -1;
}

static void apply_config_post(struct gstreamer_stream *stream)
{
	const struct avdecc_format *format = &stream->params.format;

	if ((format->u.s.subtype == AVTP_SUBTYPE_61883_IIDC)
	&& (format->u.s.subtype_u.iec61883.fmt == IEC_61883_CIP_FMT_6))
		stream->frame_size = stream->batch_size;
}

int select_prepare_gst_talker_pipeline(struct gstreamer_pipeline *gst_pipeline, const struct avdecc_format *format)
{
	const struct gstreamer_pipeline_config *config = &gst_pipeline->config;

	/* select adequate gstreamer pipeline */

	switch (format->u.s.subtype) {
	case AVTP_SUBTYPE_61883_IIDC:
		if (format->u.s.subtype_u.iec61883.sf == 0) {
			printf("Unsupported 61883_IIDC format\n");
			return -1;
		} else
			switch (format->u.s.subtype_u.iec61883.fmt) {
			case IEC_61883_CIP_FMT_4:
				if (config->type & GST_TYPE_MULTI_TALKER) {
						if ((config->talker.flags & GST_FLAG_PREVIEW)) {
							gst_pipeline->definition = &pipeline_talker_file_61883_4_61883_6_h264_preview;
							printf("pipeline_talker_file_61883_4_61883_6_h264_preview selected\n");
						} else {
							gst_pipeline->definition = &pipeline_talker_file_61883_4_61883_6_h264;
							printf("pipeline_talker_file_61883_4_61883_6_h264 selected\n");
						}
				} else {
					if (config->type & GST_TYPE_VIDEO) {
						if ((config->talker.flags & GST_FLAG_PREVIEW)) {
							gst_pipeline->definition = &pipeline_talker_file_61883_4_preview;
							printf("pipeline_talker_file_61883_4_preview selected\n");
						} else {
							gst_pipeline->definition = &pipeline_talker_file_61883_4;
							printf("pipeline_talker_file_61883_4 selected\n");
						}
					} else { /*Send audio only over 61883_4*/
						if (strstr(config->talker.input_media_files[config->talker.input_media_file_index], ".m4a") ||
					strstr(config->talker.input_media_files[config->talker.input_media_file_index], ".mp4") ) {
							gst_pipeline->definition = &pipeline_talker_file_61883_4_audio_m4a;
							printf("pipeline_talker_file_61883_4_audio_m4a selected\n");
						} else if (strstr(config->talker.input_media_files[config->talker.input_media_file_index], ".mp3")) {
							gst_pipeline->definition = &pipeline_talker_file_61883_4_audio_mp3;
							printf("pipeline_talker_file_61883_4_audio_mp3 selected\n");
						} else if (strstr(config->talker.input_media_files[config->talker.input_media_file_index], ".wav"))
							printf("No gst pipeline selected, wav format not supported over 61883_4\n");
						else
							printf("[Talker] No gst pipeline selected, input file format not supported for audio only IEC_61883_CIP_FMT_4\n");
					}
				}
				break;
			case IEC_61883_CIP_FMT_6:
				if (config->type & GST_TYPE_MULTI_TALKER) {
						if ((config->talker.flags & GST_FLAG_PREVIEW)) {
							gst_pipeline->definition = &pipeline_talker_file_61883_4_61883_6_h264_preview;
							printf("pipeline_talker_file_61883_4_61883_6_h264_preview selected\n");
						} else {
							gst_pipeline->definition = &pipeline_talker_file_61883_4_61883_6_h264;
							printf("pipeline_talker_file_61883_4_61883_6_h264 selected\n");
						}
				} else {
					if (strstr(config->talker.input_media_files[config->talker.input_media_file_index], ".m4a") ||
					strstr(config->talker.input_media_files[config->talker.input_media_file_index], ".mp4") ) {
						gst_pipeline->definition = &pipeline_talker_file_61883_6_audio_m4a;
						printf("pipeline_talker_file_61883_6_audio_m4a selected\n");
					} else if (strstr(config->talker.input_media_files[config->talker.input_media_file_index], ".mp3")) {
						gst_pipeline->definition = &pipeline_talker_file_61883_6_audio_mp3;
						printf("pipeline_talker_file_61883_6_audio_mp3 selected\n");
					} else if (strstr(config->talker.input_media_files[config->talker.input_media_file_index], ".wav")) {
						gst_pipeline->definition = &pipeline_talker_file_61883_6_audio_wav;
						printf("pipeline_talker_file_61883_6_audio_wav selected\n");
					} else
						printf("[Talker] No gst pipeline selected, input file format not supported for IEC_61883_CIP_FMT_6\n");
				}
				break;
			case IEC_61883_CIP_FMT_8:
			default:
				printf("[Talker] Unsupported IEC-61883 format: %d\n", format->u.s.subtype);
				return -1;
			}

		break;

#ifdef CFG_AVTP_1722A
	case AVTP_SUBTYPE_CVF:
		if (format->u.s.subtype_u.cvf.format == CVF_FORMAT_RFC) {
			switch (format->u.s.subtype_u.cvf.subtype) {
			case CVF_FORMAT_SUBTYPE_H264:
				if (config->type & GST_TYPE_MULTI_TALKER) {
						if ((config->talker.flags & GST_FLAG_PREVIEW)) {
							gst_pipeline->definition = &pipeline_talker_file_61883_4_61883_6_h264_preview;
							printf("pipeline_talker_file_61883_4_61883_6_h264_preview selected\n");
						} else {
							gst_pipeline->definition = &pipeline_talker_file_61883_4_61883_6_h264;
							printf("pipeline_talker_file_61883_4_61883_6_h264 selected\n");
						}
				} else {
					if ((config->talker.flags & GST_FLAG_PREVIEW)) {
						gst_pipeline->definition = &pipeline_talker_file_cvf_h264_preview;
						printf("pipeline_talker_file_cvf_h264_preview selected : %s \n", gst_pipeline->definition->pipeline_string);
					} else {
						gst_pipeline->definition = &pipeline_talker_file_cvf_h264;
						printf("pipeline_talker_file_cvf_h264 selected %s \n", pipeline_talker_file_cvf_h264.pipeline_string);
					}
				}
			break;

			case CVF_FORMAT_SUBTYPE_JPEG2000:
			case CVF_FORMAT_SUBTYPE_MJPEG:
			default:
				printf("[Talker] Unsupported CVF subtype: %d\n", format->u.s.subtype_u.cvf.subtype);
				return -1;
			}
		} else {
			printf("[Talker] Unsupported CVF format: %d\n", format->u.s.subtype_u.cvf.format);
			return -1;
		}

		break;
#endif

	default:
		printf("[Talker] Unsupported AVTP subtype: %d\n", format->u.s.subtype);
		return -1;
	}
	return 0;
}

int select_gst_listener_pipeline(struct gstreamer_pipeline *gst_pipeline, const struct avdecc_format *format)
{
	const struct gstreamer_pipeline_config *config = &gst_pipeline->config;

	/* select adequate gstreamer pipeline */

	if ((config->type & GST_TYPE_LISTENER) && (config->listener.flags & GST_FLAG_DEBUG)) {
		gst_pipeline->definition = &pipeline_listener_debug;
		printf("pipeline_listener_debug selected, file dump at %s \n",config->listener.debug_file_dump_location);

		return 0;
	}

	switch (format->u.s.subtype) {
	case AVTP_SUBTYPE_61883_IIDC:
		if (format->u.s.subtype_u.iec61883.sf == 0) {
			printf("Unsupported 61883_IIDC format\n");
			return -1;
		} else
			switch (format->u.s.subtype_u.iec61883.fmt) {
			case IEC_61883_CIP_FMT_6:
				gst_pipeline->definition = &pipeline_listener_61883_6;
				printf("pipeline_listener_61883_6 selected\n");
				break;

			case IEC_61883_CIP_FMT_4:
				if ((config->type & GST_TYPE_LISTENER) && (config->type & GST_TYPE_VIDEO) && !(config->type & GST_TYPE_AUDIO)) {
					gst_pipeline->definition = &pipeline_listener_61883_4_video_only;
					printf("pipeline_listener_61883_4_video_only selected\n");
				}
				else if ((config->type & GST_TYPE_LISTENER) && (config->type & GST_TYPE_AUDIO) && !(config->type & GST_TYPE_VIDEO) ) {
					gst_pipeline->definition = &pipeline_listener_61883_4_audio_only;
					printf("pipeline_61883_4_audio_only selected\n");
				}
				else {
					gst_pipeline->definition = &pipeline_listener_61883_4_audio_video;
					printf("pipeline_listener_61883_4_audio_video selected\n");
				}

				break;

			case IEC_61883_CIP_FMT_8:
			default:
				printf("[Listener] Unsupported IEC-61883 format: %d\n", format->u.s.subtype);
				return -1;
			}

		break;

#ifdef CFG_AVTP_1722A
	case AVTP_SUBTYPE_CVF:
		if (format->u.s.subtype_u.cvf.format == CVF_FORMAT_RFC) {
			switch (format->u.s.subtype_u.cvf.subtype) {
			case CVF_FORMAT_SUBTYPE_MJPEG:
				if (config->nstreams == 1) {
					gst_pipeline->definition = &pipeline_listener_cvf_mjpeg;
					printf("pipeline_listener_cvf_mjpeg selected\n");
				} else {
					gst_pipeline->definition = &pipeline_cvf_mjpeg_decode_only;
					printf("pipeline_listener_cvf_mjpeg_decode_only selected\n");
				}
				break;

			case CVF_FORMAT_SUBTYPE_H264:
					gst_pipeline->definition = &pipeline_listener_cvf_h264;
					printf("pipeline_listener_cvf_h264 selected %s \n", pipeline_listener_cvf_h264.pipeline_string);
				break;

			case CVF_FORMAT_SUBTYPE_JPEG2000:
			default:
				printf("[Listener] Unsupported CVF subtype: %d\n", format->u.s.subtype_u.cvf.subtype);
				return -1;
			}
		} else {
			printf("[Listener] Unsupported CVF format: %d\n", format->u.s.subtype_u.cvf.format);
			return -1;
		}

		break;
#endif

	default:
		printf("[Listener] Unsupported AVTP subtype: %d\n", format->u.s.subtype);
		return -1;
	}

	return 0;
}

void gst_pipeline_configure_pts_offset(struct gstreamer_pipeline *gst_pipeline, const struct avdecc_format *format)
{
	const struct gstreamer_pipeline_config *config = &gst_pipeline->config;

	gst_pipeline->listener.local_pts_offset = 0;
	gst_pipeline->listener.pts_offset = config->listener.pts_offset;

	switch (format->u.s.subtype) {
	case AVTP_SUBTYPE_61883_IIDC:
		gst_pipeline->listener.local_pts_offset = LOCAL_PTS_OFFSET;

		if (gst_pipeline->listener.pts_offset == GST_CLOCK_TIME_NONE)
			gst_pipeline->listener.pts_offset = DEFAULT_PTS_OFFSET;

		break;

#ifdef CFG_AVTP_1722A
	case AVTP_SUBTYPE_CVF:
		if (format->u.s.subtype_u.cvf.format == CVF_FORMAT_RFC) {
			switch (format->u.s.subtype_u.cvf.subtype) {
			case CVF_FORMAT_SUBTYPE_MJPEG:
				gst_pipeline->listener.local_pts_offset = SALSA_LATENCY;
				if (gst_pipeline->listener.pts_offset == GST_CLOCK_TIME_NONE)
					gst_pipeline->listener.pts_offset = CVF_MJPEG_PTS_OFFSET;
				break;

			case CVF_FORMAT_SUBTYPE_H264:
				gst_pipeline->listener.local_pts_offset = LOCAL_PTS_OFFSET;
				if (gst_pipeline->listener.pts_offset == GST_CLOCK_TIME_NONE)
					gst_pipeline->listener.pts_offset = DEFAULT_PTS_OFFSET;
				break;

			default:
				break;
			}
		}

		break;
#endif

	default:
		break;
	}

	if (gst_pipeline->listener.pts_offset == GST_CLOCK_TIME_NONE)
		gst_pipeline->listener.pts_offset = 0;


	if (gst_pipeline->listener.pts_offset < (gst_pipeline->definition->latency + gst_pipeline->listener.local_pts_offset)) {
		gst_pipeline->listener.pts_offset = gst_pipeline->definition->latency + gst_pipeline->listener.local_pts_offset;
		printf("Warning: PTS offset too small, resetting to %lld ns.\n", gst_pipeline->definition->latency + gst_pipeline->listener.local_pts_offset);
	}

	gst_pipeline->listener.pts_offset -= gst_pipeline->listener.local_pts_offset;
}

static int timeout_handler(struct gstreamer_stream *_stream, unsigned int events)
{
	int rc;

	char tmp[8];
	int ret;

	ret = read(_stream->timer_fd, tmp, 8);
	if (ret < 0)
		printf("%s: [stream %p] timer_fd read() failed: %s\n", __func__, _stream, strerror(errno));


	struct talker_gst_multi_app *stream = _stream->data;
	rc = talker_gst_multi_stream_fsm(stream, STREAM_EVENT_TIMER);
	if (rc < 0)
		goto err;

	rc = talker_gst_multi_fsm(stream->gst, GST_EVENT_TIMER);
	if (rc < 0)
		goto err;

	return 0;

err:
	return rc;
}

static int listener_stream_fd_timeout_handler(struct gstreamer_stream *_stream, unsigned int events)
{
	/*Check if we started reading data before*/
	if(_stream->data_received) {

		listener_stream_flush(_stream->stream_h);

		gst_stop_pipeline(_stream->pipe_source.gst_pipeline);
		stream_init_stats(_stream);

		_stream->data_received = 0;

		if (gst_start_pipeline(_stream->pipe_source.gst_pipeline, GST_PRIORITY, GST_DIRECTION_LISTENER) < 0) {
			printf("gst_start_pipeline() failed inside timeout handler\n");
			goto error_gst_pipeline;
		}
		_stream->pipe_source.source = _stream->pipe_source.gst_pipeline->source[0].source;
	}
	return 0;
error_gst_pipeline:
	return -1;

}

static void media_app_stream_poll_set (void *data, int enable)
{
	struct gstreamer_stream *talker = (struct gstreamer_stream *) data;

	if(talker->thread)
		thread_slot_set_events(talker->thread, enable, EPOLLOUT);
}

static int talker_gst_multi_stream_handler(void *data, unsigned int events)
{
	struct gstreamer_stream *_stream = (struct gstreamer_stream *) data;
	struct talker_gst_multi_app *stream = _stream->data;
	int rc = 0;

	pthread_mutex_lock(&stream->gst->stream_lock);
	rc = talker_gst_multi_stream_fsm(stream, STREAM_EVENT_DATA);
	pthread_mutex_unlock(&stream->gst->stream_lock);

	return rc;
}

int talker_gstreamer_connect(struct gstreamer_stream *talker, struct avb_stream_params *params, unsigned int avdecc_stream_index)
{
	struct avb_handle *avb_h = avbstream_get_avb_handle();
	int rc;
	struct talker_gst_multi_app *_stream = NULL;

	if (talker->created)
		goto error_stream_create;

	if (connected_gst_talkers >= MAX_CONNECTED_GST_TALKERS) {
		printf("%s : Error stream (%d) Maximum supported gstreamer pipeline talkers exceeded (%d) \n", __func__, avdecc_stream_index, MAX_CONNECTED_GST_TALKERS);
		goto error_stream_create;
	}

	apply_stream_params(talker, params);

	print_stream_id(talker->params.stream_id);

	rc = avb_stream_create(avb_h, &talker->stream_h, &talker->params, &talker->batch_size, talker->flags);
	if (rc != AVB_SUCCESS) {
		printf("avb_stream_create() failed: %s\n", avb_strerror(rc));
		rc = -1;
		goto error_stream_create;
	}
	printf("Configured AVB batch size (bytes): %d\n", talker->batch_size);
	apply_config_post(talker);

	/*
	 * retrieve the file descriptor associated to the stream
	 */
	talker->stream_fd = avb_stream_fd(talker->stream_h);
	if (talker->stream_fd < 0) {
		printf("avb_stream_fd() failed: %s\n", avb_strerror(talker->stream_fd));
		rc = -1;
		goto error_stream_fd;
	}

	/*Delay to let SRP complete its init phase*/
	sleep(5);

	select_prepare_gst_talker_pipeline(talker->pipe_source.gst_pipeline, &talker->params.format);

	talker->pipe_source.gst_pipeline->talker.gst = &gstreamer_talker_media[talker->source_index];

	talker->pipe_source.gst_pipeline->talker.gst->gst_pipeline = talker->pipe_source.gst_pipeline;
	talker->pipe_source.gst_pipeline->talker.gst->state = GST_STATE_STOPPED;

	talker->data =  &gstreamer_talker_multi_app[talker->source_index];

	/* Init the gst_multi_app struct*/
	_stream = (struct talker_gst_multi_app *) talker->data;

	_stream->gst = talker->pipe_source.gst_pipeline->talker.gst;
	_stream->stream_h = talker->stream_h;
	_stream->batch_size = talker->batch_size;
	_stream->state = STREAM_STATE_DISCONNECTED;
	_stream->sink_index = 0;
	_stream->index = talker->source_index;
	_stream->stream_poll_set = media_app_stream_poll_set;
	_stream->stream_poll_data = talker;
	memcpy(&_stream->params, &talker->params, sizeof(struct avb_stream_params));

	/*NOTE 1 AVB Stream <=> one pipeline */
	talker->pipe_source.gst_pipeline->talker.stream[0] = talker->data;
	talker->pipe_source.gst_pipeline->sink[0].data = talker->data;
	talker->pipe_source.gst_pipeline->talker.nb_streams++;

	dump_gst_config(talker->pipe_source.gst_pipeline);

	/*
	 * setup and kick-off gstreamer pipeline
	 * For now, assume 1 AVB stream <=> 1 pipeline
	 */


	talker->timer_fd = create_stream_timer(POLL_TIMEOUT_MS);
	if (talker->timer_fd < 0) {
		printf("%s timerfd_create() failed %s\n", __func__, strerror(errno));
		goto error_stream_timer_create;
	}

	printf("%s Connect the Gstreamer pipeline \n",__func__);

	rc = talker_gst_multi_stream_fsm(_stream, STREAM_EVENT_CONNECT);
	if (rc < 0 ) {
		printf("talker_gst_multi_stream_fsm CONNECT failed\n");
		goto error_stream_connect;
	}

	printf("\n %s Setting the timeout handler \n", __func__);

	if (thread_slot_add(THR_CAP_TIMER, talker->timer_fd, EPOLLIN, talker, (int (*)(void *, unsigned int events))timeout_handler, NULL, 0, (thr_thread_slot_t **)&talker->thread_timer) < 0)
		goto error_thread_timer;

	talker->pipe_source.sink = talker->pipe_source.gst_pipeline->sink[0].sink;

	printf("\n %s : Add the talker data slot \n",__func__);

	if (thread_slot_add(THR_CAP_STREAM_TALKER | THR_CAP_CONTROLLED | THR_CAP_TIMER, talker->stream_fd, EPOLLOUT, talker, talker_gst_multi_stream_handler, NULL, 0, (thr_thread_slot_t **)&talker->thread) < 0)
		goto error_thread_talker;

	talker->created = 1;
	connected_gst_talkers++;

	return 0;

error_thread_talker:
	thread_slot_free(talker->thread_timer);

error_thread_timer:
	talker_gst_multi_stream_fsm(_stream, STREAM_EVENT_DISCONNECT);

error_stream_connect:
error_stream_timer_create:
error_stream_fd:
	avb_stream_destroy(talker->stream_h);

error_stream_create:
	return -1;
}

int talker_gstreamer_multi_connect(struct avb_stream_params *params, struct gstreamer_talker_multi_handler *talker_multi, unsigned int sink_index, unsigned int stream_index)
{
	struct avb_handle *avb_h = avbstream_get_avb_handle();
	int rc;
	struct talker_gst_multi_app *_stream = NULL;
	struct gstreamer_stream *talker = talker_multi->talkers[sink_index];

	if (talker->created)
		goto error_stream_create;

	if ((!talker_multi->connected_streams) && (connected_gst_talkers >= MAX_CONNECTED_GST_TALKERS)) {
		printf("%s : Error stream (%d) Maximum supported gstreamer talkers exceeded (%d) \n", __func__, stream_index, MAX_CONNECTED_GST_TALKERS);
		goto error_stream_create;
	}

	apply_stream_params(talker, params);

	print_stream_id(talker->params.stream_id);

	rc = avb_stream_create(avb_h, &talker->stream_h, &talker->params, &talker->batch_size, talker->flags);
	if (rc != AVB_SUCCESS) {
		printf("avb_stream_create() failed: %s\n", avb_strerror(rc));
		rc = -1;
		goto error_stream_create;
	}
	printf("Configured AVB batch size (bytes): %d\n", talker->batch_size);

	apply_config_post(talker);

	/*
	 * retrieve the file descriptor associated to the stream
	 */
	talker->stream_fd = avb_stream_fd(talker->stream_h);
	if (talker->stream_fd < 0) {
		printf("avb_stream_fd() failed: %s\n", avb_strerror(talker->stream_fd));
		rc = -1;
		goto error_stream_fd;
	}

	/*Delay to let SRP complete its init phase*/
	sleep(5);

	/* Select pipeline once*/
	if (!talker_multi->connected_streams)
		select_prepare_gst_talker_pipeline(&talker_multi->gst_pipeline, &talker->params.format);

	pthread_mutex_lock(&talker_multi->gst_media->stream_lock);

	talker->pipe_source.gst_pipeline = &talker_multi->gst_pipeline;

	talker->data = talker_multi->multi_app[sink_index];
	talker->source_index = stream_index;

	/* Init the gst_multi_app struct*/
	_stream = (struct talker_gst_multi_app *) talker->data;

	_stream->gst = talker->pipe_source.gst_pipeline->talker.gst;
	_stream->stream_h = talker->stream_h;
	_stream->batch_size = talker->batch_size;
	_stream->state = STREAM_STATE_DISCONNECTED;
	_stream->sink_index = sink_index;
	_stream->index = talker->source_index;
	_stream->stream_poll_set = media_app_stream_poll_set;
	_stream->stream_poll_data = talker;

	memcpy(&_stream->params, &talker->params, sizeof(struct avb_stream_params));

	talker->pipe_source.gst_pipeline->talker.stream[sink_index] = talker->data;
	talker->pipe_source.gst_pipeline->talker.nb_streams++;

	dump_gst_config(talker->pipe_source.gst_pipeline);

	printf("%s Connect the Gstreamer pipeline \n",__func__);

	rc = talker_gst_multi_stream_fsm(_stream, STREAM_EVENT_CONNECT);
	if (rc < 0 ) {
		printf("talker_gst_multi_stream_fsm CONNECT failed\n");
		goto error_stream_connect;
	}

	pthread_mutex_unlock(&talker_multi->gst_media->stream_lock);

	talker->pipe_source.sink = talker->pipe_source.gst_pipeline->sink[sink_index].sink;

	printf("\n %s : Add the talker data slot \n",__func__);

	if (thread_slot_add(THR_CAP_GST_MULTI | THR_CAP_TIMER, talker->stream_fd, EPOLLOUT, talker, talker_gst_multi_stream_handler, NULL, 0, (thr_thread_slot_t **)&talker->thread) < 0)
		goto error_thread_talker;

	talker->created = 1;

	if (!talker_multi->connected_streams)
		connected_gst_talkers++;

	talker_multi->connected_streams++;

	return 0;

error_thread_talker:
	pthread_mutex_lock(&talker_multi->gst_media->stream_lock);
	talker_gst_multi_stream_fsm(_stream, STREAM_EVENT_DISCONNECT);
	pthread_mutex_unlock(&talker_multi->gst_media->stream_lock);

error_stream_connect:
error_stream_fd:
	avb_stream_destroy(talker->stream_h);

error_stream_create:
	return -1;
}

int listener_gstreamer_connect(struct gstreamer_stream *listener, struct avb_stream_params *params, unsigned int avdecc_stream_index)
{
	struct avb_handle *avb_h = avbstream_get_avb_handle();
	int rc;
	unsigned int slot_timeout = 0;

	if (listener->created)
		goto error_stream_create;

	if (connected_gst_listeners >= MAX_CONNECTED_GST_LISTENERS) {
		printf("%s : Error stream (%d) Maximum supported gstreamer listeners exceeded (%d) \n", __func__, avdecc_stream_index, MAX_CONNECTED_GST_LISTENERS);
		goto error_stream_create;
	}

	apply_stream_params(listener, params);

	rc = avb_stream_create(avb_h, &listener->stream_h, &listener->params, &listener->batch_size, listener->flags);
	if (rc != AVB_SUCCESS) {
		printf("avb_stream_create() failed: %s\n", avb_strerror(rc));
		rc = -1;
		goto error_stream_create;
	}
	printf("Configured AVB batch size (bytes): %d\n", listener->batch_size);
	apply_config_post(listener);

	/*
	 * retrieve the file descriptor associated to the stream
	 */
	listener->stream_fd = avb_stream_fd(listener->stream_h);
	if (listener->stream_fd < 0) {
		printf("avb_stream_fd() failed: %s\n", avb_strerror(listener->stream_fd));
		rc = -1;
		goto error_stream_fd;
	}

	select_gst_listener_pipeline(listener->pipe_source.gst_pipeline, &listener->params.format);
	gst_pipeline_configure_pts_offset(listener->pipe_source.gst_pipeline, &listener->params.format);
	dump_gst_config(listener->pipe_source.gst_pipeline);

	if (get_audio_clk_sync(listener->params.clock_domain))
		listener->pipe_source.gst_pipeline->config.sink_slave_method = GST_AUDIO_BASE_SINK_SLAVE_NONE;

	stream_init_stats(listener);
	/*
	 * setup and kick-off gstreamer pipeline
	 * For now, assume 1 AVB stream <=> 1 pipeline
	 */
	if (gst_start_pipeline(listener->pipe_source.gst_pipeline, GST_PRIORITY, GST_DIRECTION_LISTENER) < 0) {
		printf("gst_start_pipeline() failed\n");
		rc = -1;
		goto error_gst_pipeline;
	}

	listener->pipe_source.source = listener->pipe_source.gst_pipeline->source[0].source;
	listener->data_received = 0;
	slot_timeout = 1 * NSEC_PER_SEC;

	printf("\n %s: Adding listener data slot with timeout %u ns\n", __func__, slot_timeout);

	if (thread_slot_add(THR_CAP_STREAM_LISTENER, listener->stream_fd, EPOLLIN, listener, (int (*)(void *, unsigned int))listener->listener_gst_handler,
				(int (*)(void *))listener_stream_fd_timeout_handler, slot_timeout, (thr_thread_slot_t **)&listener->thread) < 0)
		goto error_thread;

	listener->created = 1;
	connected_gst_listeners++;

	return 0;


error_thread:
	gst_stop_pipeline(listener->pipe_source.gst_pipeline);
error_gst_pipeline:

error_stream_fd:
	avb_stream_destroy(listener->stream_h);

error_stream_create:
	return -1;
}

void listener_gstreamer_disconnect(struct gstreamer_stream *listener)
{
	if (listener->created) {
		thread_slot_free(listener->thread);

		gst_stop_pipeline(listener->pipe_source.gst_pipeline);

		avb_stream_destroy(listener->stream_h);

		listener->created = 0;

		connected_gst_listeners--;
	}
}

void talker_gstreamer_disconnect(struct gstreamer_stream *talker)
{
	if(talker->created) {
		thread_slot_free(talker->thread);

		thread_slot_free(talker->thread_timer);

		close(talker->timer_fd);

		talker_gst_multi_stream_fsm(talker->data, STREAM_EVENT_DISCONNECT);

		avb_stream_destroy(talker->stream_h);

		talker->created = 0;

		connected_gst_talkers--;
	}

}

void talker_gstreamer_multi_disconnect(struct gstreamer_talker_multi_handler *talker_multi, unsigned int sink_index)
{
	struct gstreamer_stream *talker = talker_multi->talkers[sink_index];
	struct talker_gst_multi_app *stream;

	if(talker->created) {
		thread_slot_free(talker->thread);

		stream = (struct talker_gst_multi_app *) talker->data;
		pthread_mutex_lock(&stream->gst->stream_lock);
		talker_gst_multi_stream_fsm(stream, STREAM_EVENT_DISCONNECT);
		pthread_mutex_unlock(&stream->gst->stream_lock);

		avb_stream_destroy(talker->stream_h);

		talker->created = 0;

		talker_multi->connected_streams--;

		if (!talker_multi->connected_streams)
			connected_gst_talkers--;

	}

}

int gstreamer_split_screen_start(struct gstreamer_pipeline *pipeline)
{
	/* Setup the Gstreamer pipeline for the split screen display */
	pipeline->definition = &pipeline_split_screen;
	pipeline->listener.pts_offset = OVERLAY_PIPELINE_LATENCY;
	pipeline->listener.local_pts_offset = 0;

	dump_gst_config(pipeline);

	if (gst_start_pipeline(pipeline, GST_PRIORITY, GST_DIRECTION_LISTENER) < 0) {
		printf("gst_start_pipeline() failed for pipeline_split_screen\n");
		goto error_gst_pipeline;
	}

	return 0;
error_gst_pipeline:
	return -1;
}
