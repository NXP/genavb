/*
 * Copyright 2017-2018, 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <dlfcn.h>
#include <stdlib.h>
#include <sys/epoll.h>

#include <genavb/genavb.h>

#include "salsa_camera.h"
#include "gstreamer_stream.h"
#include "multi_frame_sync.h"
#include "../common/avb_stream.h"
#include "../common/thread.h"
#include "../common/gst_pipeline_definitions.h"


#ifdef CFG_AVTP_1722A

static struct gstreamer_pipeline salsa_decode_pipeline[MAX_CAMERA];
static struct gstreamer_stream salsa_camera_stream[MAX_CAMERA];



struct avb_stream_params salsa_camera_stream_params = {
	.direction = AVTP_DIRECTION_LISTENER,
	.subtype = AVTP_SUBTYPE_CVF,
	.stream_class = SR_CLASS_B,
	.clock_domain = AVB_CLOCK_DOMAIN_0,
	.flags = 0,
	.format.u.s = {
		.v = 0,
		.subtype = AVTP_SUBTYPE_CVF,
		.subtype_u.cvf = {
			.format = CVF_FORMAT_RFC,
			.subtype = CVF_FORMAT_SUBTYPE_MJPEG,
			.format_u.mjpeg = {
				.p = CVF_MJPEG_P_PROGRESSIVE,
				.type = CVF_MJPEG_TYPE_YUV420,
				.width = 160,
				.height = 100,
			},
		},
	},
	.port = 0,
	.stream_id = { 0x00, 0x00, 0x00, 0x04, 0x9f, 0x00, 0x4a, 0x50 },
	.dst_mac = { 0x91, 0xe0, 0xf0, 0x00, 0x06, 0x00 },
};



int salsa_camera_connect(struct gstreamer_stream *salsa_stream)
{
	struct avb_handle *avb_h = avbstream_get_avb_handle();
	int rc;

	select_gst_listener_pipeline(salsa_stream->pipe_source.gst_pipeline, &salsa_camera_stream_params.format);
	gst_pipeline_configure_pts_offset(salsa_stream->pipe_source.gst_pipeline, &salsa_camera_stream_params.format);
	dump_gst_config(salsa_stream->pipe_source.gst_pipeline);


	if (gst_start_pipeline(salsa_stream->pipe_source.gst_pipeline, GST_PRIORITY, GST_DIRECTION_LISTENER) < 0) {
		printf("gst_start_pipeline() failed\n");
		rc = -1;
		goto error_gst_pipeline;
	}

	rc = gst_cvf_mjpeg_warm_up_pipeline(salsa_stream->pipe_source.gst_pipeline, 1);

	salsa_stream->pipe_source.source = salsa_stream->pipe_source.gst_pipeline->source[0].source;
	salsa_stream->source_index = 0;

	apply_stream_params(salsa_stream, &salsa_camera_stream_params);

	rc = avb_stream_create(avb_h, &salsa_stream->stream_h, &salsa_stream->params, &salsa_stream->batch_size, salsa_stream->flags);
	if (rc != AVB_SUCCESS) {
		printf("avb_stream_create() failed: %s\n", avb_strerror(rc));
		rc = -1;
		goto error_stream_create;
	}
	printf("Configured AVB batch size (bytes): %d\n", salsa_stream->batch_size);

	/*
	 * retrieve the file descriptor associated to the stream
	 */
	salsa_stream->stream_fd = avb_stream_fd(salsa_stream->stream_h);
	if (salsa_stream->stream_fd < 0) {
		printf("avb_stream_fd() failed: %s\n", avb_strerror(salsa_stream->stream_fd));
		rc = -1;
		goto error_stream_fd;
	}

	stream_init_stats(salsa_stream);

	if (thread_slot_add(THR_CAP_STREAM_LISTENER, salsa_stream->stream_fd, EPOLLIN, salsa_stream, (int (*)(void *, unsigned int))salsa_stream->listener_gst_handler, NULL, 0, (thr_thread_slot_t **)&salsa_stream->thread) < 0)
		goto error_thread;

	salsa_stream->created = 1;


	return 0;


error_thread:
error_stream_fd:
	avb_stream_destroy(salsa_stream->stream_h);

error_stream_create:
	gst_stop_pipeline(salsa_stream->pipe_source.gst_pipeline);

error_gst_pipeline:
	return -1;
}

void salsa_camera_disconnect(struct gstreamer_stream *salsa_stream)
{
	if (salsa_stream->created) {
		thread_slot_free(salsa_stream->thread);

		avb_stream_destroy(salsa_stream->stream_h);

		gst_stop_pipeline(salsa_stream->pipe_source.gst_pipeline);

		salsa_stream->created = 0;
	}
}

void salsa_camera_init(void)
{
	int i;

	for (i = 0; i < MAX_CAMERA; i++)
		gstreamer_pipeline_init(&salsa_decode_pipeline[i]);
}

static int mfs_split_screen_push_buffers(void *priv, GstBuffer **buffers)
{
	int index, rc = 0;
	struct gstreamer_pipeline *split_screen_pipeline = (struct gstreamer_pipeline *)priv;
	long long offset;

	if (!GST_CLOCK_TIME_IS_VALID(split_screen_pipeline->basetime))
		split_screen_pipeline->basetime = gst_element_get_base_time(GST_ELEMENT(split_screen_pipeline->pipeline));

	offset = split_screen_pipeline->listener.pts_offset + split_screen_pipeline->listener.local_pts_offset - split_screen_pipeline->basetime;

	for (index = 0; index < split_screen_pipeline->config.nstreams; index++) {
		GstBuffer *buffer = buffers[index];
		if (buffer) {
			gst_buffer_ref(buffer);
			GST_BUFFER_PTS(buffer) += offset;
			rc = gst_app_src_push_buffer(split_screen_pipeline->source[index].source, buffer);
			if (rc != GST_FLOW_OK) {
				if (rc == GST_FLOW_FLUSHING)
					printf("Pipeline not in PAUSED or PLAYING state\n");
				else
					printf("End-of-Stream occurred\n");
			}
		}
	}

	return 0;
}


static void *svm_library;

static int (*svm_render)(unsigned char * buffer_cam, int index_cam);
static void (*svm_eye_position)(float irx, float iry, float ipx, float ipy, float ipz);
static void (*svm_render_start)(void);
static void (*svm_render_end)(void);
static int (*svm_init)(const char *settings_filename);
static int (*svm_close)(void);
static int (*svm_open)(unsigned int fb_index);

GstMapInfo surround_prev_info[MAX_CAMERA];
GstBuffer *surround_prev_buffer[MAX_CAMERA];
unsigned int surround_ready = 0;
float surround_rx = 0.0f, surround_ry = -0.3f;
float surround_dry = -0.001;

static int mfs_surround_push_buffers(void *priv, GstBuffer **buffers)
{
	int index, rc = GST_FLOW_OK;
	unsigned char * current_data =  NULL;

	if (!surround_ready) {
		svm_init("/home/media/surround_view/settings.xml");
		surround_ready = 1;
	}

	surround_rx += 0.005;
	if (surround_rx > 3.1416)
		surround_rx -= 2*3.1416;

	if (surround_ry < -1.3)
		surround_dry = 0.001;
	if (surround_ry > -0.3)
		surround_dry = -0.001;
	surround_ry += surround_dry;

	svm_eye_position(surround_rx, surround_ry, 0.0, 0.0, -3.0);
	svm_render_start();
	for (index = 0; index < MAX_CAMERA; index++) {
		GstBuffer *buffer = buffers[index];

		if (buffer) {
			if (buffer != surround_prev_buffer[index]) {
				if (surround_prev_buffer[index]) {
					if (surround_prev_info[index].data) {
						gst_buffer_unmap(surround_prev_buffer[index], &surround_prev_info[index]);
						surround_prev_info[index].data = NULL;
					}
					gst_buffer_unref(surround_prev_buffer[index]);
				}
				gst_buffer_ref(buffer);
				surround_prev_buffer[index] = buffer;

				if (gst_buffer_map(buffer, &surround_prev_info[index], GST_MAP_READ))
					current_data = surround_prev_info[index].data;
			} else
				current_data = surround_prev_info[index].data;

			if (!current_data) {
				printf("Couldn't get data buffer%d\n", index);
				rc = -1;
				goto exit;
			}

			svm_render(current_data, index); //stitching function
		}
	}

	svm_render_end();

exit:
	return rc;
}


int surround_init(struct gstreamer_pipeline_config *gst_config)
{
	int rc, fb_index;

	svm_library = dlopen("/home/media/surround_view/libsvm.so", RTLD_NOW);
	if (!svm_library) {
		printf("Could not open svm library: %s\n", dlerror());
		return -1;
	}

	svm_render = dlsym(svm_library, "svm_render");
	if (!svm_render)
		goto symbol_err;

	svm_eye_position = dlsym(svm_library, "svm_eye_position");
	if (!svm_eye_position)
		goto symbol_err;

	svm_render_start = dlsym(svm_library, "svm_render_start");
	if (!svm_render_start)
		goto symbol_err;

	svm_render_end = dlsym(svm_library, "svm_render_end");
	if (!svm_render_end)
		goto symbol_err;

	svm_init = dlsym(svm_library, "svm_init");
	if (!svm_init)
		goto symbol_err;

	svm_close = dlsym(svm_library, "svm_close");
	if (!svm_close)
		goto symbol_err;

	svm_open = dlsym(svm_library, "svm_open");
	if (!svm_open)
		goto symbol_err;

	rc = sscanf(gst_config->device, "/dev/fb%d", &fb_index);
	if (rc != 1) {
		printf("Invalid fb display device %s\n", gst_config->device);
		goto err;
	}

	svm_open(fb_index);

	memset(surround_prev_info, 0, sizeof(GstMapInfo) * MAX_CAMERA);
	memset(surround_prev_buffer, 0, sizeof(GstBuffer *) * MAX_CAMERA);

	return 0;

symbol_err:
	printf("Missing symbol in avbsvm library: %s\n", dlerror());
err:
	dlclose(svm_library);
	return -1;

}

int surround_exit(void)
{
	int index;

	svm_close();

	for (index = 0; index < MAX_CAMERA; index++) {
		if (surround_prev_buffer[index]) {
			if (surround_prev_info[index].data) {
				gst_buffer_unmap(surround_prev_buffer[index], &surround_prev_info[index]);
				surround_prev_info[index].data = NULL;
			}

			gst_buffer_unref(surround_prev_buffer[index]);
		}
	}

	dlclose(svm_library);

	return 0;
}










/* Warm up the overlay pipeline */
static void multi_salsa_camera_warmup(unsigned int nstreams, struct gstreamer_pipeline *split_screen_pipeline)
{
	GstSample *sample;
	unsigned int i;

	for (i = 0; i < nstreams; i++) {
		gst_cvf_mjpeg_warm_up_pipeline(&salsa_decode_pipeline[i], 1);

		sample =  gst_app_sink_pull_sample(GST_APP_SINK(salsa_decode_pipeline[i].sink[0].sink));
		if (split_screen_pipeline)
			gst_app_src_push_sample(split_screen_pipeline->source[i].source, sample);
		gst_sample_unref(sample);
	}
}


static int multi_salsa_decoding_start(unsigned int nstreams)
{
	unsigned int i;
	int rc = 0;

	/* Setup the decoding pipelines */
	for (i = 0; i < nstreams; i++) {
		select_gst_listener_pipeline(&salsa_decode_pipeline[i], &salsa_camera_stream_params.format);
		salsa_decode_pipeline[i].config.device = malloc(48);
		memcpy(salsa_decode_pipeline[i].config.device, "/tmp/cam1-%d.jpg", sizeof("/tmp/cam1-%d.jpg"));
		salsa_decode_pipeline[i].config.device[8] = '0' + i + 1;
		gst_pipeline_configure_pts_offset(&salsa_decode_pipeline[i], &salsa_camera_stream_params.format);

		if (gst_start_pipeline(&salsa_decode_pipeline[i], GST_PRIORITY, GST_DIRECTION_LISTENER) < 0) {
			printf("gst_play_pipeline() failed for salsa_decode_pipeline %d \n", i);
			rc = -1;
			goto err_start;
		}
	}

	return rc;

err_start:
	while (i > 0)  {
		i--;
		gst_stop_pipeline(&salsa_decode_pipeline[i]);
	}

	return rc;
}

static void multi_salsa_decoding_stop(unsigned int nstreams)
{
	unsigned int i;

	/* Stop the decoding pipelines */
	for (i = 0; i < nstreams; i++) {
		gst_stop_pipeline(&salsa_decode_pipeline[i]);
	}
}

static int multi_salsa_decoding_connect_avb(struct gstreamer_pipeline_config *gst_config)
{
	struct avb_handle *avb_h = avbstream_get_avb_handle();
	unsigned int i;
	int rc;
	struct gstreamer_stream *salsa_stream;
	unsigned int nstreams = gst_config->nstreams;

	/* Prepare the streams */
	for (i = 0; i < nstreams; i++) {
		salsa_stream = &salsa_camera_stream[i];
		salsa_stream->pipe_source.gst_pipeline = &salsa_decode_pipeline[i];
		salsa_stream->pipe_source.source = salsa_decode_pipeline[i].source[0].source;
		salsa_stream->source_index = i;

		stream_init_stats(salsa_stream);

		if(!(gst_config->listener.stream_ids_mappping[i].stream_id)) {

			switch(gst_config->listener.stream_ids_mappping[i].source_index) {
			case 0:
				salsa_camera_stream_params.stream_id[7] = 0x50;
				break;
			case 1:
				salsa_camera_stream_params.stream_id[7] = 0x60;
				break;
			case 2:
				salsa_camera_stream_params.stream_id[7] = 0x70;
				break;
			case 3:
				salsa_camera_stream_params.stream_id[7] = 0x80;
				break;
			default:
				break;
			}
		}
		apply_stream_params(salsa_stream, &salsa_camera_stream_params);

		if(gst_config->listener.stream_ids_mappping[i].stream_id)
			memcpy(&salsa_stream->params.stream_id, &gst_config->listener.stream_ids_mappping[i].stream_id, sizeof(gst_config->listener.stream_ids_mappping[i].stream_id));

		print_stream_id(salsa_stream->params.stream_id);
	}

	/* Create the AVB streams */
	for (i = 0; i < nstreams; i++) {
		salsa_stream = &salsa_camera_stream[i];

		rc = avb_stream_create(avb_h, &salsa_stream->stream_h, &salsa_stream->params, &salsa_stream->batch_size, salsa_stream->flags);
		if (rc != AVB_SUCCESS) {
			printf("avb_stream_create() failed: %s\n", avb_strerror(rc));
			rc = -1;
			goto error_stream_create;
		}
		printf("Configured AVB batch size (bytes): %d\n", salsa_stream->batch_size);

		/*
		 * retrieve the file descriptor associated to the stream
		 */
		salsa_stream->stream_fd = avb_stream_fd(salsa_stream->stream_h);
		if (salsa_stream->stream_fd < 0) {
			printf("avb_stream_fd() failed: %s\n", avb_strerror(salsa_stream->stream_fd));
			rc = -1;
			goto error_stream_fd;
		}
	}

	/* Add the data handlers for the AVB streams */
	for (i = 0; i < nstreams; i++) {
		salsa_stream = &salsa_camera_stream[i];

		if (thread_slot_add(THR_CAP_STREAM_LISTENER, salsa_stream->stream_fd, EPOLLIN, salsa_stream, (int (*)(void *, unsigned int))salsa_stream->listener_gst_handler, NULL, 0, (thr_thread_slot_t **)&salsa_stream->thread) < 0) {
			printf("thread_slot_add() failed\n");
			goto error_thread;
		}
		salsa_stream->created = 1;
	}

	return 0;

error_thread:
	while (i > 0) {
		i--;
		salsa_stream = &salsa_camera_stream[i];
		thread_slot_free(salsa_stream->thread);
		salsa_stream->created = 0;
	}

	i = nstreams;

error_stream_create:
	while (i > 0) {
		i--;
error_stream_fd:
		salsa_stream = &salsa_camera_stream[i];
		avb_stream_destroy(salsa_stream->stream_h);
	}

	return -1;
}

static int multi_salsa_decoding_connect(struct gstreamer_pipeline_config *gst_config)
{
	unsigned int i;
	unsigned int src_index;
	unsigned int nstreams = gst_config->nstreams;

	/* Connect the output of the decoding pipelines to our handling functions */
	for (i = 0; i < nstreams; i++) {
		src_index = gst_config->listener.stream_ids_mappping[i].source_index;
		if (mfs_add_sync(i, salsa_decode_pipeline[src_index].sink[0].sink) < 0) {
			printf("mfs_add_sync failed for mfs.sync %d\n", i);
			goto error_mfs_add_sync;
		}
	}

	if (multi_salsa_decoding_connect_avb(gst_config) < 0)
		goto error_mfs_add_sync;


	return 0;

error_mfs_add_sync:
	while (i > 0) {
		i--;
		mfs_remove_sync(i);
	}

	return -1;
}

static void multi_salsa_decoding_disconnect(unsigned int nstreams)
{
	unsigned int i;

	for (i = 0; i < nstreams; i++) {
		struct gstreamer_stream *salsa_stream = &salsa_camera_stream[i];

		if (salsa_stream->created) {
			thread_slot_free(salsa_stream->thread);
			mfs_remove_sync(i);

			avb_stream_destroy(salsa_stream->stream_h);

			salsa_stream->created = 0;
		}
	}

}


void multi_salsa_camera_disconnect(unsigned int nstreams)
{
	multi_salsa_decoding_disconnect(nstreams);

	multi_salsa_decoding_stop(nstreams);
}

int multi_salsa_split_screen_start(unsigned int nstreams, struct gstreamer_pipeline *pipeline)
{
	if (gstreamer_split_screen_start(pipeline) < 0) {
		printf("gstreamer_split_screen_start() failed\n");
		goto error_gst_split_screen;
	}

	mfs_init(pipeline->config.nstreams, mfs_split_screen_push_buffers, pipeline);


	if (multi_salsa_decoding_start(nstreams) < 0)
		goto error_decoding_start;

	multi_salsa_camera_warmup(nstreams, pipeline);

	if (multi_salsa_decoding_connect(&pipeline->config) < 0)
		goto error_decoding_connect;

	return 0;

error_decoding_connect:
	multi_salsa_decoding_stop(nstreams);

error_decoding_start:
error_gst_split_screen:
	return -1;
}

int multi_salsa_surround_start(struct gstreamer_pipeline_config *gst_config)
{
	if (surround_init(gst_config) < 0) {
		printf("Couldn't initialize Surround view\n");
		goto error_surround;
	}

	mfs_init(MAX_CAMERA, mfs_surround_push_buffers, NULL); //FIXME create surround struct to hold all surround vars

	if (multi_salsa_decoding_start(gst_config->nstreams) < 0)
		goto error_decoding_start;

	multi_salsa_camera_warmup(gst_config->nstreams, NULL);

	if (multi_salsa_decoding_connect(gst_config) < 0)
		goto error_decoding_connect;

	return 0;

error_decoding_connect:
	multi_salsa_decoding_stop(gst_config->nstreams);

error_decoding_start:
error_surround:
	return -1;
}


















#endif
