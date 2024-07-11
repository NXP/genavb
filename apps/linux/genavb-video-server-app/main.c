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
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sched.h>
#include <signal.h>
#include <time.h>
#include <dirent.h>

#include <genavb/genavb.h>
#include <genavb/helpers.h>
#include <genavb/aem.h>
#include <genavb/srp.h>

#include "../common/gstreamer_multisink.h"
#include "../common/common.h"
#include "../common/stats.h"
#include "../common/time.h"
#include "../common/ts_parser.h"
#include "../common/file_buffer.h"
#include "../common/gstreamer.h"
#include "../common/aecp.h"
#include "../common/gst_pipeline_definitions.h"

//#define AVB_TALKER_TS_LOG

#define APP_PRIORITY		1 /* RT_FIFO priority to be used for the process */

#define K		1024
#define DATA_BUF_SZ	(16*K)
#define EVENT_BUF_SZ	(K)
#define BATCH_SIZE	2048
#define BATCH_SIZE_H264	102400   // in bytes /* Set batch size of h264 stream to 100kb*/

#define EVENT_MAX	(BATCH_SIZE / PES_SIZE)

/* default input video file name */
#define DEFAULT_MEDIA_FILE_NAME "sample1.mp4"



struct media_app {

	struct talker_gst_media gst;
	struct talker_gst_multi_app stream[APP_MAX_ACTIVE_STREAMS];

	struct stats poll_delay;
	struct stats poll_data;

	unsigned int ts_parser_enabled;

	unsigned int rendering_delay;

	avb_u8 input_media_file_index;
	unsigned int n_input_media_files;
	char **input_media_files;
	char *input_media_file_name;

	unsigned int width;
	unsigned int height;
	unsigned int type;

	char *device;

	struct media_thread thread;
	struct gstreamer_pipeline gst_pipe;
};

static int signal_terminate = 0;
static int signal_child_terminate = 0;


static void usage (void)
{
	printf("\nUsage:\napp [options]\n");
	printf("\nOptions:\n"
		"\t-f <infile name>      video file name (default video.ts)\n"
		"\t-l                    local video preview\n"
		"\t-v                    Video H264 streaming (No local preview)\n"
		"\t-L                    local video preview rendering latency (in ms)\n"
		"\t-d <device>           display device for local preview (lvds (default), hdmi)\n"
		"\t-s <scaling>          scaling for local preview (1080, 720, 768 (default), 480)\n"
		"\t-c <custom pipeline>  use the specified custom gstreamer pipeline\n"
		"\t                      instead of the hard-coded ones. Must be the\n"
		"\t                      last option on the command-line, overrides\n"
		"\t                      all other options.\n"
		"\t-h                    print this help text\n");
}


/*
 * scandir will skip the file if input_file_filter_mp4 returns 0, and add it to the list otherwise.
 */
int input_file_filter_mp4(const struct dirent *file)
{
	if (!strcasestr(file->d_name, ".mp4"))
		return 0;
	else
		return 1;

}

int build_media_file_list(struct media_app *app)
{
	struct stat status;
	struct dirent **file_list;
	int rc = 0;
	int n = 1;
	int i = 0;
	int len;


	if (stat(app->input_media_file_name, &status) < 0) {
		printf("Couldn't get file status for %s, got error %s\n", app->input_media_file_name, strerror(errno));
		goto err;
	}

	if (S_ISDIR(status.st_mode)) {
		n = scandir(app->input_media_file_name, &file_list, input_file_filter_mp4, alphasort);
		if (n < 0) {
			printf("Couldn't scan directory %s, got error %s\n", app->input_media_file_name, strerror(errno));
			goto err;
		}
		if (n == 0) {
			printf("Didn't find any media files in directory %s\n", app->input_media_file_name);
			goto err;
		}

		app->n_input_media_files = n;
		app->input_media_files = malloc(n*sizeof(char *));

		if (!app->input_media_files) {
			printf("Error while allocating %d input media files\n", n);
			goto err_inputs_alloc;
		}

		for (i = 0; i < n; i++) {
			len = strlen(app->input_media_file_name) + strlen(file_list[i]->d_name) + 1;
			app->input_media_files[i] = malloc(len);

			if (!app->input_media_files[i]) {
				printf("Error while allocating input media files at index %d\n", i);
				goto err_input_member_alloc;
			}

			rc = snprintf(app->input_media_files[i], len, "%s%s", app->input_media_file_name, file_list[i]->d_name);
			if ((rc < 0) || (rc >= len)) {
				printf("Error %d while generating filenames\n", rc);
				goto err_name_gen;
			}

		}

	} else {
		app->n_input_media_files = 1;
		app->input_media_files = malloc(sizeof(char *));

		if (!app->input_media_files) {
			printf("Error while allocating input media files\n");
			goto err_inputs_alloc;
		}

		*app->input_media_files = app->input_media_file_name;
	}

	return 0;

err_name_gen:
	free(app->input_media_files[i]);

err_input_member_alloc:
	while (i--) {
		free(app->input_media_files[i]);
	}

	free(app->input_media_files);

err_inputs_alloc:
err:
	return -1;
}

static int apply_gst_config(struct talker_gst_media *gst, struct media_app *app)
{
	int rc = 0;

	gst->gst_pipeline = &app->gst_pipe;

	//TODO Add local preview for H264
	if (app->type == OPT_TYPE_VIDEO_ONLY) {
			gst->gst_pipeline->definition = &pipeline_talker_file_cvf_h264;
			printf("pipeline_talker_file_cvf_h264 selected\n");

	} else {
		if (app->type == OPT_TYPE_PREVIEW) {
			gst->gst_pipeline->definition = &pipeline_talker_file_61883_4_61883_6_preview;
			printf("pipeline_talker_file_61883_4_61883_6_preview selected\n");
		} else {
			gst->gst_pipeline->definition = &pipeline_talker_file_61883_4_61883_6;
			printf("pipeline_talker_file_61883_4_61883_6 selected\n");
		}
	}
	gst->gst_pipeline->config.device = app->device;
	gst->gst_pipeline->config.height = app->height;
	gst->gst_pipeline->config.width = app->width;
	gst->gst_pipeline->config.crop_height = 0;
	gst->gst_pipeline->config.crop_width = 0;
	gst->gst_pipeline->config.sync_render_to_clock = 1;

	/* FIXME for now use the same file for all pipelines */
	gst->gst_pipeline->config.talker.file_src_location = app->input_media_files[app->input_media_file_index];

	gst->gst_pipeline->config.talker.preview_ts_offset = (app->rendering_delay * 1000000);

	if (app->ts_parser_enabled)
		gst->gst_pipeline->talker.sync = 0;
	else
		gst->gst_pipeline->talker.sync = 1;

	return rc;
}


static void dump_gst_config(struct media_app *app)
{
	printf("INPUT MEDIA FILE: %s\n", app->input_media_file_name);

	if (app->type == OPT_TYPE_PREVIEW) {
		printf("LOCAL DISPLAY DEVICE: %s\n", app->device);

		printf("LOCAL DISPLAY SCALING: %dx%d\n", app->width, app->height);
	}
}


static void set_avb_config(unsigned int *avb_flags)
{
	*avb_flags = 0;
}


static int config_handler(struct media_stream *_stream)
{
	struct media_app *app = _stream->thread->data;

	_stream->params.clock_domain = AVB_MEDIA_CLOCK_DOMAIN_PTP;
	_stream->params.talker.latency = max(CFG_TALKER_LATENCY_NS, sr_class_interval_p(_stream->params.stream_class) / sr_class_interval_q(_stream->params.stream_class));

	if (_stream->params.format.u.s.subtype_u.cvf.subtype == CVF_FORMAT_SUBTYPE_H264)
		_stream->batch_size = BATCH_SIZE_H264;
	else
		_stream->batch_size = BATCH_SIZE;

	_stream->flags = AVTP_NONBLOCK;

	print_stream_id(_stream->params.stream_id);

	dump_gst_config(app);

	return 0;
}


static void signal_child_terminate_handler(int signal_num)
{
	printf("child terminated\n");
	signal_child_terminate = 1;
}


static void signal_terminate_handler(int signal_num)
{
	signal_terminate = 1;
}


static int data_handler(struct media_stream *_stream)
{
	struct talker_gst_multi_app *stream = _stream->data;
	return talker_gst_multi_stream_fsm(stream, STREAM_EVENT_DATA);
}


static void start_stream(struct media_thread *thread)
{
	int i;
	for (i = 0; i < thread->num_streams; i++)
		talker_gst_multi_stream_fsm(thread->stream[i].data, STREAM_EVENT_PLAY);

}

static void stop_stream(struct media_thread *thread)
{
	int i;
	for (i = 0; i < thread->num_streams; i++)
		talker_gst_multi_stream_fsm(thread->stream[i].data, STREAM_EVENT_STOP);
}


void aem_send_aecp_playstop_control_unsolicited_response(struct media_app *app, avb_u8 play_stop)
{
	aecp_aem_send_set_control_single_u8_unsolicited_response(app->thread.controlled.handle, 1, &play_stop);
}

void aem_send_media_track_name_control_unsolicited_response(struct media_app *app, const char *track)
{
	aecp_aem_send_set_control_utf8_unsolicited_response(app->thread.controlled.handle, 3, track);
}

void aem_send_media_track_control_unsolicited_response(struct media_app *app, avb_u8 track)
{
	aecp_aem_send_set_control_single_u8_unsolicited_response(app->thread.controlled.handle, 2, &track);
}


static int aem_set_control_handler(struct avdecc_controlled *controlled, avb_u16 ctrl_index, void *ctrl_value)
{
	struct media_thread *thread = (struct media_thread *)controlled->data;
	struct talker_gst_multi_app *stream = thread->stream[0].data;
	struct media_app *app = thread->data;
	avb_u8 value = *(avb_u8 *)ctrl_value;
	int rc = AECP_AEM_SUCCESS;
	int restart_stream;

	/* Only descriptor indices 1,2 have been mapped here */
	switch(ctrl_index) {
	case 1: /* play/stop */
		if (value) { /* PLAY <--> 255 */
			printf("Start playing stream\n");
			start_stream(thread);
			} else { /* STOP <--> 0 */
			printf("Stop playing stream\n");
			stop_stream(thread);
		}
		break;

	case 2: /* media track ID */
		printf("Received SET_CONTROL command for media track(%d) from AVDECC\n", value);

		if ((value == 0) || (value > app->n_input_media_files)) {
			rc = AECP_AEM_BAD_ARGUMENTS;
			value = app->input_media_file_index + 1;
			printf("media track index out-of-bounds, clamping to %d\n", value);
		}

		*(avb_u8 *)ctrl_value = value;
		if (value != (app->input_media_file_index + 1)) {
			if (app->gst.state != GST_STATE_STOPPED) {
				stop_stream(thread);
				restart_stream = 1;
			} else
				restart_stream = 0;
			app->input_media_file_index = value - 1;
			stream->gst->gst_pipeline->config.talker.file_src_location = app->input_media_files[app->input_media_file_index];
			aem_send_media_track_name_control_unsolicited_response(app, stream->gst->gst_pipeline->config.talker.file_src_location);
			printf("Setting input file to %s\n", stream->gst->gst_pipeline->config.talker.file_src_location);
			sleep(PREVNEXT_STOP_DURATION);
			if (restart_stream) {
				start_stream(thread);
			}
		}
		break;

	default:
		printf("Unsupported index(%d) for CONTROL descriptor.\n", ctrl_index);
		rc = AECP_AEM_NO_SUCH_DESCRIPTOR;
		break;
	}

	return rc;
}


static int connect_handler(struct media_stream *_stream)
{
	struct media_app *app = (struct media_app *)_stream->thread->data;
	aem_send_aecp_playstop_control_unsolicited_response(app, 1);
	aem_send_media_track_control_unsolicited_response(app, app->input_media_file_index + 1);
	aem_send_media_track_name_control_unsolicited_response(app, app->input_media_files[app->input_media_file_index]);

	struct talker_gst_multi_app *gst_multi_app = (struct talker_gst_multi_app *) _stream->data;
	gst_multi_app->batch_size = _stream->batch_size;
	gst_multi_app->stream_h = _stream->handle;
	memcpy(&gst_multi_app->params, &_stream->params, sizeof(struct avb_stream_params));

	return talker_gst_multi_stream_fsm(gst_multi_app, STREAM_EVENT_CONNECT);
}

static int disconnect_handler(struct media_stream *_stream)
{
	aem_send_aecp_playstop_control_unsolicited_response(_stream->thread->data, 0);
	return talker_gst_multi_stream_fsm(_stream->data, STREAM_EVENT_DISCONNECT);
}

static int timeout_handler(struct media_thread *thread)
{
	struct media_app *app = thread->data;
	int rc;
	int i;

	for (i = 0; i < thread->num_streams; i++) {
		struct media_stream *_stream = &thread->stream[i];

		rc = talker_gst_multi_stream_fsm(_stream->data, STREAM_EVENT_TIMER);
		if (rc < 0)
			goto err;
	}

	rc = talker_gst_multi_fsm(&app->gst, GST_EVENT_TIMER);
	if (rc < 0)
		goto err;

	return 0;

err:
	return rc;
}

static int signal_handler(struct media_thread *thread)
{
	int rc = 0;

	if (signal_terminate) {
		signal_terminate = 0;
		printf("processing terminate signal\n");
		rc = -1;
	}

#if 0
	if (signal_child_terminate) {
		signal_child_terminate = 0;
		printf("processing child terminate signal\n");
		rc = -1;
	}
#endif

	return rc;
}

static int init_handler(struct media_thread *thread)
{
	printf("%s\n", __func__);

	return 0;
}

static int exit_handler(struct media_thread *thread)
{
	struct media_app *app = thread->data;
	int i;

	printf("%s\n", __func__);

	for (i = 0; i < app->thread.num_streams; i++)
		talker_gst_multi_stream_fsm(app->thread.stream[i].data, STREAM_EVENT_DISCONNECT);

	return 0;
}
static void video_server_stream_poll_set (void* data, int enable)
{
	struct media_stream *stream = (struct media_stream *) data;
	media_stream_poll_set(stream, enable);
}


int main(int argc, char *argv[])
{
	struct media_app app;
	unsigned int avb_flags;
	int option;
	int i;
	int rc = 0;
	struct sched_param param = {
		.sched_priority = APP_PRIORITY,
	};
	struct sigaction action;
	unsigned long optval_ul;

	setlinebuf(stdout);

	printf("NXP's GenAVB reference video server application\n");

	memset(&app, 0, sizeof(struct media_app));

	gstreamer_init();

	/*
	* Increase process priority to match the AVTP thread priority
	*/

	if (sched_setscheduler(0, SCHED_FIFO, &param) < 0) {
		printf("sched_setscheduler() failed: %s\n", strerror(errno));
		rc = -1;
		goto exit;
	}

	/*
	* retrieve user's configuration parameters
	*/

	app.type = OPT_TYPE_DEFAULT;
	app.input_media_file_index = 0;
	app.n_input_media_files = 1;
	app.input_media_file_name = DEFAULT_MEDIA_FILE_NAME;
	app.ts_parser_enabled = 0;
	app.rendering_delay = 0;
	app.device = V4L2_LVDS_DEVICE_FILE;
	app.width = 0;
	app.height = 0;

	while ((option = getopt(argc, argv,"f:lL:d:s:htv")) != -1) {

		switch (option) {
		case 'f':
		{
			app.input_media_file_name = optarg;
			rc = build_media_file_list(&app);
			if (rc < 0)
				goto exit;

			if (app.n_input_media_files > 1) {
				app.input_media_file_index = app.n_input_media_files - 1;
				printf("Found %d media files in %s, using %s as first file.\n", app.n_input_media_files, app.input_media_file_name, app.input_media_files[app.input_media_file_index]);
			}


			break;
		}
		case 'l':
			app.type = OPT_TYPE_PREVIEW;
			break;

		case 'v':
			app.type = OPT_TYPE_VIDEO_ONLY;
			break;

		case 'L':
			if (h_strtoul(&optval_ul, optarg, NULL, 10) < 0)
				goto exit;
			app.rendering_delay = (unsigned int)optval_ul;
			break;

		case 'd':
			if (!strcasecmp(optarg, "lvds"))
				app.device = V4L2_LVDS_DEVICE_FILE;
			else if (!strcasecmp(optarg, "hdmi"))
				app.device = V4L2_HDMI_DEVICE_FILE;
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

		case 't':
			app.ts_parser_enabled = 1;
			break;

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

	action.sa_handler = signal_child_terminate_handler;
	action.sa_flags = 0;

	if (sigemptyset(&action.sa_mask) < 0)
		printf("sigemptyset(): %s\n", strerror(errno));

	if (sigaction(SIGCHLD, &action, NULL) < 0) /* Child process termination signal */
		printf("sigaction(): %s\n", strerror(errno));

	/*
	* setup avb stack
	*/
	printf("Running in streaming mode\n");

	set_avb_config(&avb_flags);

	rc = avb_init(&app.thread.avb_h, avb_flags);
	if (rc != AVB_SUCCESS) {
		printf("avb_init() failed: %s\n", avb_strerror(rc));
		rc = -1;
		goto error_avb_init;
	}

	/*
	* listen to avdecc events to get stream parameters
	*/

	rc = avb_control_open(app.thread.avb_h, &app.thread.ctrl.handle, AVB_CTRL_AVDECC_MEDIA_STACK);
	if (rc != AVB_SUCCESS) {
		printf("avb_control_open() for AVB_CTRL_AVDECC_MEDIA_STACK channel failed: %s\n", avb_strerror(rc));
		goto error_control_open;
	}

	rc = avb_control_open(app.thread.avb_h, &app.thread.controlled.handle, AVB_CTRL_AVDECC_CONTROLLED);
	if (rc != AVB_SUCCESS) {
		printf("avb_control_open() for AVB_CTRL_AVDECC_CONTROLLED channel failed: %s\n", avb_strerror(rc));
		goto error_controlled_open;
	}


	app.gst.state = GST_STATE_STOPPED;
	apply_gst_config(&app.gst, &app);

	app.thread.init_handler = init_handler;
	app.thread.exit_handler = exit_handler;
	app.thread.signal_handler = signal_handler;
	app.thread.timeout_handler = timeout_handler;
	app.thread.data_handler = data_handler;

	app.thread.controlled.aem_set_control_handler = aem_set_control_handler;

	app.thread.ctrl.connect_handler = connect_handler;
	app.thread.ctrl.config_handler = config_handler;
	app.thread.ctrl.disconnect_handler = disconnect_handler;

	app.thread.timeout_ms = POLL_TIMEOUT_MS;

	app.thread.data = &app;

	for (i = 0; i < APP_MAX_ACTIVE_STREAMS; i++) {

		app.stream[i].stream_poll_data = (void *) &app.thread.stream[i];
		app.stream[i].stream_poll_set = video_server_stream_poll_set;

		app.stream[i].index = i;

		app.thread.stream[i].data = &app.stream[i];

		app.stream[i].state = STREAM_STATE_DISCONNECTED;
	}

	app.thread.max_supported_streams = APP_MAX_SUPPORTED_STREAMS;

	if (app.gst.gst_pipeline->definition->num_sinks > 1) {
		/* Fixed mapping between streams (talker_gst_multi_app) and gst pipeline sinks, must match avdecc mapping */
		/* Audio stream */
		app.stream[0].gst = &app.gst;
		app.stream[0].sink_index = 1;
		app.stream[0].ts_parser_enabled = 0;
		app.stream[0].audio = 1;
		app.gst.gst_pipeline->sink[1].data = app.thread.stream[0].data;

		/* Video stream */
		app.stream[1].gst = &app.gst;
		app.stream[1].sink_index = 0;
		app.stream[1].ts_parser_enabled = app.ts_parser_enabled;
		app.stream[1].audio = 0;
		app.gst.gst_pipeline->sink[0].data = app.thread.stream[1].data;

		app.thread.num_streams = 2;
	} else {
		/* Fixed mapping between streams and gst pipeline sinks, must match avdecc mapping */
		/* H264 Video stream will be at stream_index 2*/
		app.stream[0].gst = &app.gst;
		app.stream[0].sink_index = 0;
		app.stream[0].ts_parser_enabled = app.ts_parser_enabled;
		app.stream[0].audio = 0;
		app.gst.gst_pipeline->sink[0].data = app.thread.stream[0].data;

		app.thread.num_streams = 1;
	}

	media_thread_loop(&app.thread);

	avb_control_close(app.thread.controlled.handle);

error_controlled_open:
	avb_control_close(app.thread.ctrl.handle);

error_control_open:
	avb_exit(app.thread.avb_h);

error_avb_init:
exit:
	gstreamer_reset();

	return rc;
}
