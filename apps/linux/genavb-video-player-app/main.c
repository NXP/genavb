/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2020, 2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sched.h>
#include <signal.h>
#include <sys/time.h>

#include <genavb/genavb.h>
#include <genavb/helpers.h>
#include <genavb/srp.h>

#include "../common/common.h"
#include "../common/stats.h"
#include "../common/time.h"
#include "../common/ts_parser.h"
#include "gstreamer.h"
#include "gst_pipelines.h"
#include "gstreamer_single.h"

//#define POLL_STATS

#define OPT_MODE_STREAMING	(1 << 0)
#define OPT_MODE_LOCAL_FILE	(1 << 1)

#define OPT_TYPE_VIDEO	(1 << 0)
#define OPT_TYPE_AUDIO	(1 << 1)
#define OPT_TYPE_DEBUG	(1 << 2)
#define OPT_TYPE_CAMERA	(1 << 4)


#define APP_PRIORITY	1 /* RT_FIFO priority to be used for the process */
#define GST_PRIORITY	1 /* RT_FIFO priority to be used for the process */


#ifdef CFG_AVTP_1722A
struct avb_stream_params salsa_camera_stream_params = {
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
#endif

struct media_app {
	unsigned int mode;
	unsigned int config;
	char *media_file_name;
	struct avb_control_handle *ctrl_h;

	struct gstreamer_pipeline gst_pipeline;

	char *device;
	unsigned int width;
	unsigned int height;
	unsigned int type;

	unsigned long long pts_offset;

	struct stats poll_delay;

	struct gstreamer_stream stream;
};

static int signal_terminate = 0;

#ifdef POLL_STATS
void poll_stats_show(struct stats *s)
{
	printf("delay (us): poll: %7d / %7d / %7d\n",
		s->min, s->mean, s->max);
}
#endif

static void usage (void)
{
	printf("\nUsage:\napp [options]\n");
	printf("\nOptions:\n"
		"\t-v                    video rendering\n"
		"\t-a                    audio rendering\n"
		"\t-d <device>           display device (lvds (default), hdmi)\n"
		"\t-s <scaling>          scaling (1080, 720, 768 (default), 480)\n"
		"\t-p <offset>           media stack presentation time offset (in ns)\n"
		"\t-f <file path>        read media from local file (no streaming)\n"
#ifdef CFG_AVTP_1722A
		"\t-S                    Salsa camera mode, static stream configuration\n"
#endif
		"\t-c <custom pipeline>  use the specified custom gstreamer pipeline\n"
		"\t                      instead of the hard-coded ones. Must be the\n"
		"\t                      last option on the command-line, overrides\n"
		"\t                      all other options except -f.\n"
		"\t-h                    print this help text\n");
	printf("\nDefault: audio and video (768p lvds) in streaming mode\n");
}


static void dump_gst_config(const struct media_app *app)
{
	printf("\nPLAYOUT TYPE: ");
	if (app->type & OPT_TYPE_VIDEO)
		printf("VIDEO ");
	if (app->type & OPT_TYPE_AUDIO)
		printf("AUDIO ");

	printf("\nDISPLAY DEVICE: %s", app->device);

	printf("\nDISPLAY SCALING: %dx%d\n", app->width, app->height);

	printf("\nPresentation offset: %llu ns\n", app->stream.pipe_source.gst_pipeline->pts_offset + app->stream.pipe_source.gst_pipeline->local_pts_offset);

	if (app->mode == OPT_MODE_LOCAL_FILE)
		printf("\nMEDIA FILE: %s\n", app->media_file_name);
}


static void set_avb_config(unsigned int *avb_flags)
{
	*avb_flags = 0;
}

static int handle_avdecc_event(struct avb_control_handle *ctrl_h, unsigned int *msg_type, union avb_media_stack_msg *msg)
{
	unsigned int msg_len = sizeof(union avb_media_stack_msg);
	int rc;

	rc = avb_control_receive(ctrl_h, msg_type, msg, &msg_len);
	if (rc != AVB_SUCCESS)
		goto receive_error;

	switch (*msg_type) {
	case AVB_MSG_MEDIA_STACK_CONNECT:
		printf("\nevent: AVB_MSG_MEDIA_STACK_CONNECT\n");
		break;

	case AVB_MSG_MEDIA_STACK_DISCONNECT:
		printf("\nevent: AVB_MSG_MEDIA_STACK_DISCONNECT\n");
		break;

	default:
		break;
	}

receive_error:
	return rc;
}


static int open_media_file(char *media_file_name)
{
	int media_fd = 0;

	media_fd = open(media_file_name, O_RDONLY);
	if (media_fd < 0)
		printf("open(%s) failed: %s\n", media_file_name, strerror(errno));

	return media_fd;
}


static int read_media_file (int media_fd, struct gstreamer_pipeline *gst)
{
	GstBuffer *buffer;
	GstMapInfo info;
	unsigned char *data_buf;
	int nbytes;
	int rc = 0;
	static int count = 0;

	while (1) {
		if (signal_terminate) {
			printf("processing terminate signal\n");
			rc = -1;
			goto exit;
		}

		/* Create a new empty buffer */
		buffer = gst_buffer_new_allocate(NULL, BATCH_SIZE, NULL);
		if (!buffer) {
			printf("Couldn't allocate Gstreamer buffer\n");
			rc = -1;
			goto exit;
		}

		buffer = gst_buffer_make_writable(buffer);

		if (gst_buffer_map(buffer, &info, GST_MAP_WRITE))
			data_buf = info.data;
		else
			data_buf = NULL;

		if (!data_buf) {
			printf("Couldn't get data buffer\n");
			rc = -1;
			goto exit;
		}

		nbytes = read(media_fd, data_buf, BATCH_SIZE);
		gst_buffer_unmap(buffer, &info);
		/* no more data to read, we are done*/
		if (nbytes <= 0) {
			if (nbytes < 0)
				printf("read() failed: %s\n", strerror(errno));

			gst_buffer_unref(buffer);

			goto exit;
		}

		gst_buffer_set_size(buffer, nbytes);

		/* Send data to Gstreamer pipeline */
		rc = gst_app_src_push_buffer(gst->u.listener.source[0].source, buffer);
		if (rc != GST_FLOW_OK) {
			if (rc == GST_FLOW_FLUSHING)
				printf("Pipeline not in PAUSED or PLAYING state\n");
			else
				printf("End-of-Stream occurred\n");

			goto exit;
		}

		count += nbytes;
	}

exit:
	if (rc == 0)
		gst_process_bus_messages(gst);

	return rc;
}


static int apply_gst_config(struct gstreamer_stream *stream, struct media_app *app)
{
	struct gstreamer_pipeline *gst_pipeline = stream->pipe_source.gst_pipeline;
	const struct avdecc_format *format = &stream->params.format;
	int rc = 0;

	gst_pipeline->local_pts_offset = 0;

	/* apply requested gstreamer pipeline */

#if 0
	gst_pipeline->pipeline_string = pipeline_listener_debug;
	printf("pipeline_listener_debug selected\n");
#else

	switch (format->u.s.subtype) {
	case AVTP_SUBTYPE_61883_IIDC:
		if (format->u.s.subtype_u.iec61883.sf == 0) {
			printf("Unsupported 61883_IIDC format\n");
			return -1;
		} else
			switch (format->u.s.subtype_u.iec61883.fmt) {
			case IEC_61883_CIP_FMT_6:
				gst_pipeline->pipeline_string = pipeline_listener_61883_6;
				gst_pipeline->pipeline_latency = latency_61883_6;
				stream->frame_size = stream->batch_size;
				printf("pipeline_listener_61883_6 selected\n");
				break;

			case IEC_61883_CIP_FMT_4:
				stream->frame_size = avdecc_fmt_sample_size(format);

				if (app->type == OPT_TYPE_VIDEO) {
					gst_pipeline->pipeline_string = pipeline_listener_61883_4_video_only;
					gst_pipeline->pipeline_latency = latency_61883_4_video_only;
					printf("pipeline_listener_61883_4_video_only selected\n");
				}
				else if (app->type == OPT_TYPE_AUDIO) {
					gst_pipeline->pipeline_string = pipeline_listener_61883_4_audio_only;
					gst_pipeline->pipeline_latency = latency_61883_4_audio_only;
					printf("pipeline_61883_4_audio_only selected\n");
				}
				else {
					gst_pipeline->pipeline_string = pipeline_listener_61883_4_audio_video;
					gst_pipeline->pipeline_latency = latency_61883_4_audio_video;
					printf("pipeline_listener_61883_4_audio_video selected\n");
				}

				break;

			case IEC_61883_CIP_FMT_8:
			default:
				printf("Unsupported IEC-61883 format: %d\n", format->u.s.subtype);
				return -1;
			}

		stream->pipe_source.dropping = 0;
		gst_pipeline->local_pts_offset = LOCAL_PTS_OFFSET;

		if (app->pts_offset == GST_CLOCK_TIME_NONE)
			gst_pipeline->pts_offset = DEFAULT_PTS_OFFSET;
		else
			gst_pipeline->pts_offset = app->pts_offset;

		stream->listener_gst_handler = listener_gst_handler;

		break;

#ifdef CFG_AVTP_1722A
	case AVTP_SUBTYPE_CVF:
		if (format->u.s.subtype_u.cvf.format == CVF_FORMAT_RFC) {
			switch (format->u.s.subtype_u.cvf.subtype) {
			case CVF_FORMAT_SUBTYPE_MJPEG:
				gst_pipeline->pipeline_string = pipeline_listener_cvf_mjpeg;
				gst_pipeline->pipeline_latency = latency_cvf_mjpeg;

				printf("pipeline_listener_cvf_mjpeg selected\n");
				break;

			case CVF_FORMAT_SUBTYPE_H264:
			case CVF_FORMAT_SUBTYPE_JPEG2000:
			default:
				printf("Unsupported CVF subtype: %d\n", format->u.s.subtype_u.cvf.subtype);
				return -1;
			}
		} else {
			printf("Unsupported CVF format: %d\n", format->u.s.subtype_u.cvf.format);
			return -1;
		}

		stream->pipe_source.dropping = 1;
		gst_pipeline->local_pts_offset = CVF_PTS_OFFSET;

		if (app->pts_offset == GST_CLOCK_TIME_NONE)
			gst_pipeline->pts_offset = CVF_PTS_OFFSET;
		else
			gst_pipeline->pts_offset = app->pts_offset;

		stream->listener_gst_handler = listener_gst_handler_cvf;

		break;
#endif

	default:
		printf("Unsupported AVTP subtype: %d\n", format->u.s.subtype);
		return -1;
	}
#endif

	if (gst_pipeline->pts_offset < (gst_pipeline->pipeline_latency + gst_pipeline->local_pts_offset)) {
		gst_pipeline->pts_offset = gst_pipeline->pipeline_latency + gst_pipeline->local_pts_offset;
		printf("Warning: PTS offset too small, resetting to %lld ns.\n", gst_pipeline->pipeline_latency + gst_pipeline->local_pts_offset);
	}

	gst_pipeline->pts_offset -= gst_pipeline->local_pts_offset;

	stream->pipe_source.buffer = NULL;
	stream->pipe_source.memory = NULL;

	stream->pipe_source.buffer_byte_count = 0;
	stream->pipe_source.memory_byte_count = 0;

	gst_pipeline->device = app->device;
	gst_pipeline->overlay_height = app->height;
	gst_pipeline->overlay_width = app->width;
	gst_pipeline->crop_height = 0;
	gst_pipeline->crop_width = 0;

	gst_pipeline->u.listener.num_sources = 1;

	dump_gst_config(app);

	return rc;
}


static void signal_terminate_handler (int signal_num)
{
	signal_terminate = 1;
}


#define LISTENER_POLL_DELAY_MS 100
#define LISTENER_POLL_RETRIES	20	/* Retry polling 20 times before considering end-of-stream has been reached */

static int run_listener(struct media_app *app)
{
	struct pollfd poll_fds[2];
	int rc = 0;
	int ctrl_rx_fd = -1;
	int ready, i, n, nfds;
	unsigned int event_type;
	unsigned int failed_count = 0;
	union avb_media_stack_msg msg;
	struct gstreamer_stream *stream = &app->stream;
#ifdef POLL_STATS
	uint64_t poll_time = 0;
	uint64_t now = 0;
	unsigned char poll_done = 0;

	stats_init(&app->poll_delay, 7, &app, poll_stats_show);
#endif

	printf("Starting listener loop, non-blocking mode, timeout %dms\n", LISTENER_POLL_DELAY_MS);

	stream_init_stats(stream);

	/*
	* listen to read event from the stack
	*/

	nfds = 0;

	poll_fds[0].fd = stream->stream_fd;
	poll_fds[0].events = POLLIN;
	poll_fds[0].revents = 0;
	nfds++;

	if (app->type != OPT_TYPE_CAMERA) {
		ctrl_rx_fd = avb_control_rx_fd(app->ctrl_h);
		poll_fds[1].fd = ctrl_rx_fd;
		poll_fds[1].events = POLLIN;
		poll_fds[1].revents = 0;
		nfds++;
	}

	while (1) {
		if (signal_terminate) {
			printf("processing terminate signal\n");
			rc = -1;
			goto exit;
		}

#ifdef POLL_STATS
		if (poll_done) {
			gettime_us(&poll_time);
			poll_done = 0;
		}
#endif
		if ((ready = poll(poll_fds, nfds, LISTENER_POLL_DELAY_MS)) == -1) {
			if (errno == EINTR) {
				continue;
			} else {
				printf("poll(%d) failed while processing listener errno %d: %s\n", stream->stream_fd, errno, strerror(errno));
				rc = -1;
				goto exit;
			}
		}

#ifdef POLL_STATS
		poll_done = 1;
		if (poll_time) {
			gettime_us(&now);
			stats_update(&app->poll_delay, (now - poll_time));
		}
#endif
		if (ready > 0) {
			for (n = 0, i = 0; i < nfds && n < ready; i++) {
				if (poll_fds[i].revents & POLLIN) {
					if (poll_fds[i].fd == ctrl_rx_fd) {
						n++;

						/*
						* read control event from avdecc
						*/
						if (handle_avdecc_event(app->ctrl_h, &event_type, &msg) == AVB_SUCCESS) {
							if (event_type == AVB_MSG_MEDIA_STACK_DISCONNECT) {
								rc = 0;
								goto exit; /* disconnected, stop processing on this stream */
							}
						}
					} else if (poll_fds[i].fd == stream->stream_fd) {
						n++;

						stream->started = 1;

						/*
						* read data from avb stack and write back to gstreamer pipeline
						*/
						rc = stream->listener_gst_handler(stream, POLLIN);
						if (rc < 0)
							goto exit;
					}
				}
			}
		} else {
			if (stream->started) {
				/* some data already received, likely end of stream, flush pending data */
				rc = stream->listener_gst_handler(stream, 0);

				if ( rc <= 0) {
					failed_count++;
					if (failed_count > LISTENER_POLL_RETRIES)
						goto exit;
				}

			} else {
				/* no data received so far, please wait...*/
				continue;
			}
		}
	}
exit:
	listener_stream_flush(stream->stream_h);

	return rc;
}


int main(int argc, char *argv[])
{
	struct media_app app;
	struct avb_handle *avb_h;
	union avb_media_stack_msg msg;
	unsigned int avb_flags;
	int option;
	unsigned int event_type;
	int ctrl_rx_fd;
	struct pollfd ctrl_poll;
	int media_fd;
	int rc = 0;
	struct sched_param param = {
		.sched_priority = APP_PRIORITY,
	};
	struct sigaction action;

	setlinebuf(stdout);

	printf("NXP's GenAVB reference video player application\n");

	app.stream.pipe_source.gst_pipeline = &app.gst_pipeline;
	memset(&app.gst_pipeline, 0, sizeof(app.gst_pipeline));
	gstreamer_init();

	/*
	* Increase process priority to match the AVTP thread priority
	*/

	if (sched_setscheduler(0, SCHED_FIFO, &param) < 0) {
		printf("sched_setscheduler(), %s\n", strerror(errno));
		rc = -1;
		goto exit;
	}

	/*
	* retrieve user's configuration parameters
	*/

	app.type = 0;
	app.device = LVDS_DEVICE_FILE;
	app.width = 0;
	app.height = 0;
	app.mode = OPT_MODE_STREAMING;
	app.pts_offset = GST_CLOCK_TIME_NONE;
	app.stream.source_index = 0;

#ifdef CFG_AVTP_1722A
	while ((option = getopt(argc, argv,"vaSd:s:f:hp:t:")) != -1) {
#else
	while ((option = getopt(argc, argv,"vad:s:f:hp:t:")) != -1) {
#endif
		switch (option) {
		case 'p':
			if (h_strtoull(&app.pts_offset, optarg, NULL, 0) < 0)
				goto exit;

			if (app.pts_offset > MAX_PTS_OFFSET)
				app.pts_offset = MAX_PTS_OFFSET;
			break;

		case 'v':
			app.type |= OPT_TYPE_VIDEO;
			break;

		case 'a':
			app.type |= OPT_TYPE_AUDIO;
			break;

		case 'f':
			app.media_file_name = optarg;
			app.mode = OPT_MODE_LOCAL_FILE;
			break;

		case 'd':
			if (!strcasecmp(optarg, "lvds"))
				app.device = LVDS_DEVICE_FILE;
			else if (!strcasecmp(optarg, "hdmi"))
				app.device = HDMI_DEVICE_FILE;
			else {
				usage();
				goto exit;
			}

			break;

		case 's':
			if (!strcasecmp(optarg, "1080")) {
				app.width = 1920;
				app.height = 1080;
			} else if (!strcasecmp(optarg, "720")) {
				app.width = 1280;
				app.height = 720;
			} else if (!strcasecmp(optarg, "768")) {
				app.width = 1024;
				app.height = 768;
			} else if (!strcasecmp(optarg, "480")) {
				app.width = 640;
				app.height = 480;
			} else {
				usage();
				goto exit;
			}

			break;
#ifdef CFG_AVTP_1722A
		case 'S':
			app.type = OPT_TYPE_CAMERA;
			break;
#endif

		case 'h':
		default:
			usage();
			rc = -1;
			goto exit;
		}

	}

	/*
	* set signals handler
	*/
	action.sa_handler = signal_terminate_handler;
	action.sa_flags = 0;

	if (sigemptyset(&action.sa_mask) < 0)
		printf("sigemptyset(): %s\n", strerror(errno));

	if (sigaction(SIGTERM, &action, NULL) < 0) /* Termination signal */
		printf("sigaction(): %s\n", strerror(errno));

	if (sigaction(SIGQUIT, &action, NULL) < 0) /* Quit from keyboard */
		printf("sigaction(): %s\n", strerror(errno));

	if (sigaction(SIGINT, &action, NULL) < 0) /* Interrupt from keyboard */
		printf("sigaction(): %s\n", strerror(errno));

	/*
	* apply default configuration
	*/
	if (!app.type)
		app.type = OPT_TYPE_VIDEO | OPT_TYPE_AUDIO;


	/*
	* setup avb stack
	*/
	if (app.mode == OPT_MODE_STREAMING) {
		printf("Running in streaming mode\n");

		set_avb_config(&avb_flags);

		rc = avb_init(&avb_h, avb_flags);
		if (rc != AVB_SUCCESS) {
			printf("avb_init() failed: %s\n", avb_strerror(rc));
			rc = -1;
			goto error_avb_init;
		}

		if(app.type != OPT_TYPE_CAMERA) {
			/*
			 * listen to avdecc events to get stream parameters
			 */

			rc = avb_control_open(avb_h, &app.ctrl_h, AVB_CTRL_AVDECC_MEDIA_STACK);
			if (rc != AVB_SUCCESS) {
				printf("avb_control_open() failed: %s\n", avb_strerror(rc));
				goto error_control_open;
			}

			ctrl_rx_fd = avb_control_rx_fd(app.ctrl_h);
			ctrl_poll.fd = ctrl_rx_fd;
			ctrl_poll.events = POLLIN;
		}

wait_new_stream:
		printf("\nwait for new stream...\n");

		if(app.type != OPT_TYPE_CAMERA) {
			while (1) {

				if (poll(&ctrl_poll, 1, -1) == -1) {
					printf("poll(%d) failed on waiting for connect\n", ctrl_poll.fd);
					rc = -1;
					goto error_ctrl_poll;
				}

				if (ctrl_poll.revents & POLLIN) {
					/*
					 * read control event from avdecc
					 */
					if (handle_avdecc_event(app.ctrl_h, &event_type, &msg) == AVB_SUCCESS) {
						if (event_type == AVB_MSG_MEDIA_STACK_CONNECT) {
							break; /* connected, start stream processing */
						}
					}
				}
			}

			/*
			 * setup the stream
			 */

			apply_config(&app.stream, &msg.media_stack_connect.stream_params);
		} else {
#ifdef CFG_AVTP_1722A
			apply_config(&app.stream, &salsa_camera_stream_params);
			if (system("salsa-camera.sh start")) {
				printf("Couldn't start Salsa camera, aborting.\n");
				goto error_start_salsa_camera;
			}

#endif
		}

		rc = avb_stream_create (avb_h, &app.stream.stream_h, &app.stream.params, &app.stream.batch_size, app.stream.flags);
		if (rc != AVB_SUCCESS) {
			printf("avb_stream_create() failed: %s\n", avb_strerror(rc));
			rc = -1;
			goto error_stream_create;
		}
		printf("Configured AVB batch size (bytes): %d\n", app.stream.batch_size);

		/*
		* retrieve the file descriptor associated to the stream
		*/
		app.stream.stream_fd = avb_stream_fd(app.stream.stream_h);
		if (app.stream.stream_fd < 0) {
			printf("avb_stream_fd() failed: %s\n", avb_strerror(app.stream.stream_fd));
			rc = -1;
			goto error_stream_fd;
		}

		apply_gst_config(&app.stream, &app);

gst_restart:
		/*
		* setup and kick-off gstreamer pipeline
		*/
		if (gst_start_pipeline(&app.gst_pipeline, GST_PRIORITY, GST_DIRECTION_LISTENER) < 0) {
			printf("start_gst_pipeline() failed\n");
			rc = -1;
			goto error_gst_pipeline;
		}
		app.stream.pipe_source.source = app.gst_pipeline.u.listener.source[0].source;

		/*
		* handle media data from avb stack
		*/
		rc = run_listener(&app);

		gst_stop_pipeline(&app.gst_pipeline);

		/*
		* main processing loop exited. could be due to error or avdecc disconnect
		*/
		if (rc < 0) {
			printf("Loop function exited with error code %d\n", rc);

			/* kill child process upon parent termination */
			if (signal_terminate) {
				printf("stop gst pipeline\n");
			} else {
				printf("restart gst pipeline\n");
				goto gst_restart;
			}
		} else {
			if (rc) {
				printf("Loop function exited upon avdecc disconnect\n");
				avb_stream_destroy(app.stream.stream_h);
				goto wait_new_stream;
			} else {
				printf("Loop function exited upon end of media (received %llu bytes)\n", app.stream.pipe_source.byte_count);
				goto gst_restart;
			}
		}
	} else if (app.mode == OPT_MODE_LOCAL_FILE) {
		printf("Running in local file mode\n");

gst_restart_local:
		/*
		* no avb streaming, gstreamer feed from local media file
		*/
		if ((media_fd = open_media_file(app.media_file_name)) < 0) {
			rc = -1;
			goto error_media_file;
		}

		/*
		* setup and kick-off gstreamer pipeline
		*/
		if (gst_start_pipeline(&app.gst_pipeline, GST_PRIORITY, GST_DIRECTION_LISTENER) < 0) {
			printf("start_gst_pipeline() failed\n");
			rc = -1;
			goto exit;
		}


		read_media_file(media_fd, app.stream.pipe_source.gst_pipeline);

		close(media_fd);

		gst_stop_pipeline(&app.gst_pipeline);

		if (!signal_terminate)
			goto gst_restart_local;
	}
	else {
		printf("Unknown app mode\n");
		goto exit;
	}
	/*
	* destroy the stream, clean-up avb stack, close gst pipeline...
	*/

error_media_file:
	/* local file mode, just quit application once we are done */
	if (app.mode == OPT_MODE_LOCAL_FILE)
		goto exit;

error_gst_pipeline:
error_stream_fd:
	avb_stream_destroy(app.stream.stream_h);

error_start_salsa_camera:
error_stream_create:
error_ctrl_poll:
	if (app.ctrl_h)
		avb_control_close(app.ctrl_h);

error_control_open:
	avb_exit(avb_h);

error_avb_init:
exit:
	gstreamer_reset();

	return rc;
}
