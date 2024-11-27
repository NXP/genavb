/*
 * Copyright 2017-2018, 2020-2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <dlfcn.h>
#include <sys/epoll.h>

#include <genavb/genavb.h>

#include "h264_camera.h"
#include "gstreamer_stream.h"
#include "../common/avb_stream.h"
#include "../common/thread.h"
#include "../common/gst_pipeline_definitions.h"


#ifdef CFG_AVTP_1722A

struct avb_stream_params h264_camera_stream_params_1722_2016 = {
	.direction = AVTP_DIRECTION_LISTENER,
	.subtype = AVTP_SUBTYPE_CVF,
	.stream_class = SR_CLASS_B,
	.clock_domain = AVB_MEDIA_CLOCK_DOMAIN_STREAM,
	.flags = 0,
	.format.u.s = {
		.v = 0,
		.subtype = AVTP_SUBTYPE_CVF,
		.subtype_u.cvf = {
			.format = CVF_FORMAT_RFC,
			.subtype = CVF_FORMAT_SUBTYPE_H264,
			.format_u.h264 = {
				.spec_version = CVF_H264_IEEE_1722_2016,
			},
		},
	},
	.port = 0,
	.stream_id = { 0x0e, 0x0a, 0x35, 0x10, 0x20, 0x30, 0x33, 0x50 },
	.dst_mac = { 0x91, 0xe0, 0xf0, 0x00, 0x06, 0x00 },
};

struct avb_stream_params h264_camera_stream_params_1722_2013 = {
       .direction = AVTP_DIRECTION_LISTENER,
       .subtype = AVTP_SUBTYPE_CVF,
       .stream_class = SR_CLASS_B,
       .clock_domain = AVB_MEDIA_CLOCK_DOMAIN_STREAM,
       .flags = 0,
       .format.u.s = {
		.v = 0,
		.subtype = AVTP_SUBTYPE_CVF,
		.subtype_u.cvf = {
			.format = CVF_FORMAT_RFC,
			.subtype = CVF_FORMAT_SUBTYPE_H264,
			.format_u.h264 = {
				.spec_version = CVF_H264_IEEE_1722_2013,
			},
		},
	},
	.port = 0,
	.stream_id = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02 },
	.dst_mac = { 0x91, 0xe0, 0xf0, 0x00, 0x06, 0x00 },
};

int h264_camera_connect(struct gstreamer_stream *h264_stream)
{
	struct avb_handle *avb_h = avbstream_get_avb_handle();
	int rc;
	struct avb_stream_params *params;
	struct gstreamer_pipeline_config *gst_config = &h264_stream->pipe_source.gst_pipeline->config;

	if(h264_stream->created) {
		printf("%s : H264 Camera stream already connected \n", __func__);
		return -1;
	}

	if (gst_config->listener.camera_type == CAMERA_TYPE_H264_1722_2013)
		params = &h264_camera_stream_params_1722_2013;
	else if (gst_config->listener.camera_type == CAMERA_TYPE_H264_1722_2016)
		params = &h264_camera_stream_params_1722_2016;
	else {
		printf("%s : Unsupported camera type \n", __func__);
		return -1;
	}

	apply_stream_params(h264_stream, params);

	if (gst_config->listener.stream_ids_mappping[gst_config->nstreams - 1].stream_id)
		memcpy(&h264_stream->params.stream_id, &gst_config->listener.stream_ids_mappping[gst_config->nstreams - 1].stream_id, sizeof(gst_config->listener.stream_ids_mappping[gst_config->nstreams - 1].stream_id));

	select_gst_listener_pipeline(h264_stream->pipe_source.gst_pipeline, &h264_stream->params.format);
	gst_pipeline_configure_pts_offset(h264_stream->pipe_source.gst_pipeline, &h264_stream->params.format);
	dump_gst_config(h264_stream->pipe_source.gst_pipeline);

	/*
	 * setup and kick-off gstreamer pipeline
	 * For now, assume 1 AVB stream <=> 1 pipeline
	 */

	if (gst_start_pipeline(h264_stream->pipe_source.gst_pipeline, GST_PRIORITY, GST_DIRECTION_LISTENER) < 0) {
		printf("gst_start_pipeline() failed\n");
		rc = -1;
		goto error_gst_pipeline;
	}

	h264_stream->pipe_source.source = h264_stream->pipe_source.gst_pipeline->source[0].source;

	rc = avb_stream_create(avb_h, &h264_stream->stream_h, &h264_stream->params, &h264_stream->batch_size, h264_stream->flags);
	if (rc != AVB_SUCCESS) {
		printf("avb_stream_create() failed: %s\n", avb_strerror(rc));
		rc = -1;
		goto error_stream_create;
	}
	printf("Configured AVB batch size (bytes): %d\n", h264_stream->batch_size);

	/*
	 * retrieve the file descriptor associated to the stream
	 */
	h264_stream->stream_fd = avb_stream_fd(h264_stream->stream_h);
	if (h264_stream->stream_fd < 0) {
		printf("avb_stream_fd() failed: %s\n", avb_strerror(h264_stream->stream_fd));
		rc = -1;
		goto error_stream_fd;
	}

	if (thread_slot_add(THR_CAP_STREAM_LISTENER, h264_stream->stream_fd, EPOLLIN, h264_stream, (int (*)(void *, unsigned int))h264_stream->listener_gst_handler, NULL, 0, (thr_thread_slot_t **)&h264_stream->thread) < 0)
		goto error_thread;

	h264_stream->created = 1;
	return 0;


error_thread:
error_stream_fd:
	avb_stream_destroy(h264_stream->stream_h);
error_stream_create:
	gst_stop_pipeline(h264_stream->pipe_source.gst_pipeline);

error_gst_pipeline:
	return -1;

}



void h264_camera_disconnect(struct gstreamer_stream *h264_stream)
{
	if(h264_stream->created) {
		thread_slot_free(h264_stream->thread);

		avb_stream_destroy(h264_stream->stream_h);

		gst_stop_pipeline(h264_stream->pipe_source.gst_pipeline);
		h264_stream->created = 0;
	}
}

void h264_camera_init(void)
{
	/* Nothing to do for now */
	return;
}

#endif
