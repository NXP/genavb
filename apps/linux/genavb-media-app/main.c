/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <signal.h>
#include <inttypes.h>

#include <genavb/genavb.h>
#include <genavb/helpers.h>

#include "../common/log.h"
#include "../common/time.h"
#include "../common/avb_stream.h"
#include "../common/thread.h"
#include "../common/alsa2.h"
#include "../common/alsa_stream.h"
#include "../common/clock.h"
#include "../common/clock_domain.h"
#include "../common/msrp.h"
#include "../common/crf_stream.h"
#include "../common/gstreamer_single.h"
#include "../common/gstreamer_multisink.h"
#include "../common/aecp.h"
#include "../common/avdecc.h"
#include "../common/audio_mappings.h"
#include "gstreamer_stream.h"
#include "salsa_camera.h"
#include "h264_camera.h"


#define FB_LVDS_DEVICE_FILE	"/dev/fb0"
#define FB_HDMI_DEVICE_FILE	"/dev/fb2"

/*Supported Static Streams:
 * - SALSA
 * - H264
 * - BEV
 */
#define MAX_STATIC_STREAM_LISTENERS	3
#define MAX_STATIC_STREAM_TALKERS	3
#define SALSA_IDX	0
#define H264_IDX	1
#define BEV_IDX		2

#define DEFAULT_ALSA_CAPTURE_DEVICE	"plughw:0,0"
#define DEFAULT_ALSA_PLAYBACK_DEVICE	"plughw:0,0"

static int signal_terminate = 0;

static struct avb_handle *avb_handle;

static struct alsa_stream alsa_talker_stream[MAX_ALSA_TALKERS];
static struct alsa_stream alsa_listener_stream[MAX_ALSA_LISTENERS];

struct gstreamer_pipeline gstreamer_listener_pipelines[MAX_GSTREAMER_LISTENERS] = { [0 ... MAX_GSTREAMER_LISTENERS - 1].msg_lock = PTHREAD_MUTEX_INITIALIZER };; // Less than MAX_LISTENERS may be needed, but this is the worst case
struct gstreamer_stream gstreamer_listener_stream[MAX_GSTREAMER_LISTENERS];

struct gstreamer_pipeline gstreamer_talker_pipelines[MAX_GSTREAMER_TALKERS] = { [0 ... MAX_GSTREAMER_TALKERS - 1].msg_lock = PTHREAD_MUTEX_INITIALIZER };; // Less than MAX_TALKERS may be needed, but this is the worst case
struct gstreamer_stream gstreamer_talker_stream[MAX_GSTREAMER_TALKERS];

static struct gstreamer_pipeline split_screen_pipeline;

static struct gstreamer_talker_multi_handler gst_talker_multi_handler[MAX_GST_MULTI_HANDLERS] = { [0 ... MAX_GST_MULTI_HANDLERS - 1].gst_pipeline.msg_lock = PTHREAD_MUTEX_INITIALIZER };
static struct gstreamer_listener_multi_handler gst_listener_multi_handler[MAX_GST_MULTI_HANDLERS] = { [0 ... MAX_GST_MULTI_HANDLERS - 1].gst_pipeline.msg_lock = PTHREAD_MUTEX_INITIALIZER };

/*Multi Handler structs*/
static struct talker_gst_media gstreamer_multi_talker_media[MAX_GST_MULTI_HANDLERS] = { [0 ... MAX_GST_MULTI_HANDLERS - 1].stream_lock = PTHREAD_MUTEX_INITIALIZER };
static struct talker_gst_multi_app gstreamer_multi_talker_multi_app[MAX_GST_MULTI_HANDLERS_STREAMS] = { [0 ... MAX_GST_MULTI_HANDLERS_STREAMS - 1].samples_lock = PTHREAD_MUTEX_INITIALIZER };
static struct gstreamer_stream  gstreamer_multi_talker_streams[MAX_GST_MULTI_HANDLERS_STREAMS];

static struct gstreamer_bus_messages_monitor gstreamer_bus_monitor = { .list_lock = PTHREAD_MUTEX_INITIALIZER, .timer_fd = -1 };

#define MAX_AVDECC_LISTENERS		8
#define MAX_AVDECC_TALKERS		8

/*Control FDs :
 * 1- AVB_CTRL_AVDECC_MEDIA_STACK
 */
#define NUM_CONTROL_FDS	3

enum stream_handler_type {
	STREAM_NONE = 0,
	STREAM_ALSA,
	STREAM_GST,
	STREAM_MULTI_GST,
	STREAM_CRF,
};

struct media_generic_stream {
	enum stream_handler_type handler_type;
	enum stream_handler_type requested_type;
	unsigned long long stream_id;

	struct alsa_config {
		int index;
	} alsa;

	struct gst_config {
		int index;
	} gst;

	struct multi_gst_config {
		int input_index;
		int output_index;
	} multi_gst;

	struct crf_config {
		int index;
	} crf;
};

static struct media_generic_stream listener_streams[MAX_AVDECC_LISTENERS];
static struct media_generic_stream talker_streams[MAX_AVDECC_TALKERS];
static struct media_generic_stream listener_static_streams[MAX_STATIC_STREAM_LISTENERS];
static struct media_generic_stream talker_static_streams[MAX_STATIC_STREAM_TALKERS];

static void handler_start(int handler_index)
{
	int i;
	//FIXME Only one multi handler is supported
	struct gstreamer_talker_multi_handler *talker_multi = &gst_talker_multi_handler[handler_index];

	if (!talker_multi->configured_streams)
		return;

	pthread_mutex_lock(&talker_multi->gst_media->stream_lock);

	for (i = 0; i < GST_MAX_SINKS; i++)
		talker_gst_multi_stream_fsm(talker_multi->multi_app[i], STREAM_EVENT_PLAY);

	pthread_mutex_unlock(&talker_multi->gst_media->stream_lock);
}

static void handler_stop(int handler_index)
{
	int i;
	struct gstreamer_talker_multi_handler *talker_multi = &gst_talker_multi_handler[handler_index];

	if (!talker_multi->configured_streams)
		return;

	pthread_mutex_lock(&talker_multi->gst_media->stream_lock);

	for (i = 0; i < GST_MAX_SINKS; i++)
		talker_gst_multi_stream_fsm(talker_multi->multi_app[i], STREAM_EVENT_STOP);

	pthread_mutex_unlock(&talker_multi->gst_media->stream_lock);
}

static void aem_send_media_track_name_control_unsolicited_response(struct avdecc_controlled *controlled, const char *track)
{
	aecp_aem_send_set_control_utf8_unsolicited_response(controlled->handle, 3, track);
}

static int aem_set_control_handler(struct avdecc_controlled *controlled, avb_u16 ctrl_index, void *ctrl_value)
{
	avb_u8 value = *(avb_u8 *)ctrl_value;
	int rc = AECP_AEM_SUCCESS;
	struct gstreamer_pipeline_config *gst_config = NULL;

	/* Only descriptor indices 1,2 have been mapped here */
	switch(ctrl_index) {
	case 1: /* play/stop */
		if (value) { /* PLAY <--> 255 */
			printf("Start playing stream\n");
			//FIXME Only one multi handler is supported
			handler_start(0);
			} else { /* STOP <--> 0 */
			printf("Stop playing stream\n");
			handler_stop(0);
		}
		break;

	case 2: /* media track ID */
		printf("Received SET_CONTROL command for media track(%d) from AVDECC\n", value);
		//FIXME Apply control to first mutli streams pileine only
		gst_config = &gst_talker_multi_handler[0].gst_pipeline.config;
		if (!(gst_config->type & GST_TYPE_MULTI_TALKER)) {
			printf("[ERROR] Received SET_CONTROL command  for a non-configured stream \n");
			rc = AECP_AEM_NO_RESOURCES;
			break;
		}

		if ((value == 0) || (value > gst_config->talker.n_input_media_files)) {
			rc = AECP_AEM_BAD_ARGUMENTS;
			value = gst_config->talker.input_media_file_index + 1;
			printf("media track index out-of-bounds, clamping to %d\n", value);
		}

		*(avb_u8 *)ctrl_value = value;
		if (value != (gst_config->talker.input_media_file_index + 1)) {

			handler_stop(0);

			gst_config->talker.input_media_file_index = value - 1;
			gst_config->talker.file_src_location = gst_config->talker.input_media_files[gst_config->talker.input_media_file_index];
			aem_send_media_track_name_control_unsolicited_response(controlled, gst_config->talker.file_src_location);
			printf("Setting input file to %s\n", gst_config->talker.file_src_location);
			sleep(PREVNEXT_STOP_DURATION);

			handler_start(0);

		}
		break;

	default:
		printf("AECP Command SET_CONTROL with index (0x%x) not handled in this app, skip\n", ctrl_index);
		rc = AECP_AEM_NOT_IMPLEMENTED;
		break;
	}

	return rc;
}

static void avdecc_streams_init(void)
{
	unsigned int i;

	for (i= 0; i < MAX_AVDECC_LISTENERS; i++) {
		listener_streams[i].handler_type =  STREAM_NONE;
		listener_streams[i].requested_type =  STREAM_NONE;
		listener_streams[i].alsa.index = i;
		listener_streams[i].gst.index = 0;  // Only one Gstreamer video sink supported for now
		listener_streams[i].crf.index = 0;	// Only support clock domain 0
	}

	for (i= 0; i < MAX_AVDECC_TALKERS; i++) {
		talker_streams[i].handler_type =  STREAM_NONE;
		talker_streams[i].requested_type =  STREAM_NONE;
		talker_streams[i].alsa.index = i;
		talker_streams[i].gst.index = i;  //FIXME ???
		talker_streams[i].crf.index = 0;
	}
}

static void print_available_multi_handlers() {

	printf("\n The available multi handlers are the following: \n");
	printf("\t AVDECC: \n");
	printf("\t \t Talkers: \n");
	printf("\t \t \t -m 0:0 -->  A/V IEC_61883_CIP_FMT_4\n\n");
	printf("\t \t \t -m 0:1 -->  A   IEC_61883_CIP_FMT_6 \n");
	printf("\t \t \t -m 0:2 -->  V   CVF H264 \n");
	printf("\t \t Listeners: N/A \n");
	printf("\t STATIC: \n");
	printf("\t \t Talkers: N/A \n");
	printf("\t \t Listeners: \n");
	printf("\t \t \t CVF MJPEG --> -m X:0 (Where 0 <= X <= 3)\n\n");
	printf("\t \t \t CVF MJPEG --> -m X:0 (Where 0 <= X <= 3)\n\n");
	printf("\t \t \t CVF MJPEG --> -m X:0 (Where 0 <= X <= 3)\n\n");
	printf("\t \t \t CVF MJPEG --> -m X:0 (Where 0 <= X <= 3)\n\n");
	printf("\n");
}

static void usage (void)
{
	printf("\nUsage:\napp [options]\n");
	printf("\ngenavb-media-app can be configured for listener [L] or talker [T]\n");
	printf("\n Configuration options should be passed in the following order: \n");
	printf("\n \t {-L|-T} {-A|-S} <stream_index> {-g|-m|-z} <handler_index> ... \n");
	printf("\n If no Talker [-T] nor Listener [-L] options are passed, genavb-media-app will handle by default avdecc audio streams with alsa handlers\n");
	printf("\nOptions:\n"
		"\t-L 			 The following options are for listener config\n"
		"\t-T 			 The following options are for Talker config\n"
		"\t-A <stream index>     The stream with avdecc stream index\n"
		"\t-S <stream index>     The static stream index\n"
		"\t-g <gst handler>      Index of the gstreamer handler\n"
		"\t-m <input:output>     Input and output indexes for the gstreamer multi handler\n"
		"\t-z <alsa handler>     Index of the alsa handler\n"
		"\t-c <crf handler>      Index of the clock domain (which act as the CRF handler)\n"
		"\t-v                    [L] video rendering of IEC_61883_CIP_FMT_4\n"
		"\t-a                    [T/L] For Listener :audio rendering of IEC_61883_CIP_FMT_4\n"
		"\t			       For Talker: sending audio stream only over IEC_61883_CIP_FMT_4\n"
		"\t-d <device>           [T/L] display device (lvds (default), hdmi)\n"
		"\t-r <resolution>       [T/L] scaling (1080, 720, 768 (default), 480)\n"
		"\t-p <offset>           [T/L] For Listener :media stack presentation time offset (in ns)\n"
		"\t                            For Talker: render delay for local preview (in ns)\n"
#ifdef CFG_AVTP_1722A
		"\t-M                	 [L] MJPEG camera mode, static stream configuration\n"
		"\t-H <type>             [L] H264 camera mode, static stream configuration \n"
		"\t                            H264_1722_2016: H264 camera mode IEEE1722_2016 based, static stream configuration\n"
		"\t                            H264_1722_2013: H264 camera mode IEEE1722_2013 based, static stream configuration\n"
		"\t-I <static stream ID>   [L] Stream ID for the H264 static stream configuration \n"
		"\t-F			 [T/L] Free-run mode : Disable sync to clock on gst pipeline\n"
		"\t			       video sinks\n"
#endif
		"\t-f <infile name>      [T] input file name \n"
		"\t-l                    [T] local video preview\n"
		"\t-D <dump_file_location> [L] dump received stream to given location (Default: /var/avb_listener_dump)\n"
		"\t-C <alsa_device>      ALSA capture device to be used for either gst handler or alsa handler\n"
		"\t-P <alsa_device>      ALSA playback device to be used for either gst handler or alsa handler\n"
		"\t-b <file name>        The file used to save Milan bindings parameters\n"
		"\t-h                    print this help text\n");
	printf("\nDefault: audio and video, auto-detect resolution, lvds\n");
	print_available_multi_handlers();
}

static int gst_bus_messages_handler(void *data, unsigned int events)
{
	struct gstreamer_bus_messages_monitor *gst_bus_monitor = (struct gstreamer_bus_messages_monitor *) data;
	int i, rc;
	char buf[10];

	// FIXME check read value (number of times timer expired)
	rc = read(gst_bus_monitor->timer_fd, buf, 10);
	if (rc < 0) {
		ERR_LOG("read error: %s", strerror(errno));
		return -1;
	}

	pthread_mutex_lock(&gst_bus_monitor->list_lock);

	for (i = 0; i < MAX_GST_PIPELINES_MSG_MONITOR; i++) {
		if (gst_bus_monitor->gst_pipelines[i] && gst_bus_monitor->gst_pipelines[i]->bus) {
			gst_process_bus_messages(gst_bus_monitor->gst_pipelines[i]);
		}
	}

	pthread_mutex_unlock(&gst_bus_monitor->list_lock);

	return 0;
}

static int stats_handler(void *data, unsigned int events)
{
	int fd = (int)(intptr_t)data;
	int i, rc;
	char buf[10];

	// FIXME check read value (number of times timer expired)
	rc = read(fd, buf, 10);
	if (rc < 0) {
		ERR_LOG("read error: %s", strerror(errno));
		return -1;
	}

	// Handle stats dump
	for (i = 0; i < MAX_ALSA_TALKERS; ++i) {
		struct alsa_stream_stats *stats = &alsa_talker_stream[i].stats;

		if (stream_stats_is_updated(&stats->gen_stats)) {
			alsa_stats_dump(stats);
			stats->gen_stats.is_updated_flag = 0;
		}
	}

	for (i = 0; i < MAX_ALSA_LISTENERS; ++i) {
		struct alsa_stream_stats *stats = &alsa_listener_stream[i].stats;

		if (stream_stats_is_updated(&stats->gen_stats)) {
			alsa_stats_dump(stats);
			stats->gen_stats.is_updated_flag = 0;
		}
	}

	thread_print_stats();

	return 0;
}

static int gst_bus_messages_timer_thread_init(struct gstreamer_bus_messages_monitor *gst_bus_monitor)
{
	struct itimerspec timeout;

	gst_bus_monitor->timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
	if (gst_bus_monitor->timer_fd < 0) {
		ERR("timerfd_create() failed: %s", strerror(errno));
		goto err_create;
	}

	timeout.it_interval.tv_sec = 0;
	timeout.it_interval.tv_nsec = GST_BUS_TIMER_TIMEOUT;
	timeout.it_value.tv_sec = 0;
	timeout.it_value.tv_nsec = GST_BUS_TIMER_TIMEOUT;
	if (timerfd_settime(gst_bus_monitor->timer_fd, 0, &timeout, NULL) < 0) {
		ERR("timerfd_settime() failed: %s", strerror(errno));
		goto err_settime;
	}

	printf("\n %s : Add the gst bus timer thread slot \n",__func__);

	if (thread_slot_add(THR_CAP_GST_BUS_TIMER, gst_bus_monitor->timer_fd, EPOLLIN, (void *)gst_bus_monitor, gst_bus_messages_handler, NULL, 0, &gst_bus_monitor->thread_slot) < 0) {
		ERR("thread_slot_add() failed");
		goto err_slot_add;
	}

	return 0;

err_slot_add:
err_settime:
	close(gst_bus_monitor->timer_fd);
	gst_bus_monitor->timer_fd = -1;

err_create:
	gst_bus_monitor->thread_slot = NULL;

	return -1;
}

static void gst_bus_messages_timer_thread_exit(struct gstreamer_bus_messages_monitor *gst_bus_monitor)
{
	if (gst_bus_monitor->thread_slot)
		thread_slot_free(gst_bus_monitor->thread_slot);

	if (gst_bus_monitor->timer_fd >= 0) {
		close(gst_bus_monitor->timer_fd);
		gst_bus_monitor->timer_fd = -1;
	}
}

static int stats_thread_init(thr_thread_slot_t **thread_slot)
{
	int i;
	int fd;
	struct itimerspec timeout;

	for (i = 0; i < g_max_avb_talker_streams; ++i) {
		struct alsa_stream_stats *stats = &alsa_talker_stream[i].stats;

		stats->gen_stats.app_ptr = (unsigned long)&alsa_talker_stream[i];
		stats->alsa_device = i;
		stats->alsa_direction = AAR_DATA_DIR_INPUT;
		stats->alsa_handle_ptr = (unsigned long)&alsa_talker_stream[i].alsa_handle;
		stats->gen_stats.avb_stream_ptr = (unsigned long)&g_avb_talker_streams[i];
		stats->gen_stats.is_listener = 0;
		stats->gen_stats.period = 10;
	}

	for (i = 0; i < g_max_avb_listener_streams; ++i) {
		struct alsa_stream_stats *stats = &alsa_listener_stream[i].stats;

		stats->gen_stats.app_ptr = (unsigned long)&alsa_listener_stream[i];
		stats->alsa_device = i;
		stats->alsa_direction = AAR_DATA_DIR_OUTPUT;
		stats->alsa_handle_ptr = (unsigned long)&alsa_listener_stream[i].alsa_handle;
		stats->gen_stats.avb_stream_ptr = (unsigned long)&g_avb_listener_streams[i];
		stats->gen_stats.is_listener = 1;
		stats->gen_stats.period = 10;
	}

	fd = timerfd_create(CLOCK_MONOTONIC, 0);
	if (fd < 0) {
		ERR("timerfd_create() failed: %s", strerror(errno));
		goto err_create;
	}

	timeout.it_interval.tv_sec = 1;
	timeout.it_interval.tv_nsec = 0;
	timeout.it_value.tv_sec = 1;
	timeout.it_value.tv_nsec = 0;
	if (timerfd_settime(fd, 0, &timeout, NULL) < 0) {
		ERR("timerfd_settime() failed: %s", strerror(errno));
		goto err_settime;
	}

	printf("\n %s : Add the stats thread slot \n",__func__);

	if (thread_slot_add(THR_CAP_STATS, fd, EPOLLIN, (void *)(intptr_t)fd, stats_handler, NULL, 0, thread_slot) < 0) {
		ERR("thread_slot_add() failed");
		goto err_slot_add;
	}

	return fd;

err_slot_add:
err_settime:
	close(fd);

err_create:
	*thread_slot = NULL;

	return -1;
}

static void stats_thread_exit(int fd, thr_thread_slot_t *thread_slot)
{
	if (thread_slot)
		thread_slot_free(thread_slot);

	if (fd >= 0)
		close(fd);
}

static int controlled_slot_init(thr_thread_slot_t **controlled_thread_slot, struct avdecc_controlled *controlled)
{
	int controllled_rx_fd;
	controllled_rx_fd = avb_control_rx_fd(controlled->handle);
	controlled->aem_set_control_handler = aem_set_control_handler;
	controlled->aem_get_audio_mappings_handler = audio_mappings_get;
	controlled->aem_add_audio_mappings_handler = audio_mappings_add;
	controlled->aem_remove_audio_mappings_handler = audio_mappings_remove;

	printf("\n %s : Add the controlled data slot \n",__func__);

	if (thread_slot_add(THR_CAP_CONTROLLED, controllled_rx_fd, EPOLLIN, controlled, (int (*)(void *, unsigned int events))avdecc_controlled_handler, NULL, 0, (thr_thread_slot_t **)controlled_thread_slot) < 0)
		goto err;


	return 0;

err:
	*controlled_thread_slot = NULL;

	return -1;
}

static void controlled_slot_exit(thr_thread_slot_t *controlled_thread_slot)
{
	if (controlled_thread_slot)
		thread_slot_free(controlled_thread_slot);
}

static int init_media_clock_source(unsigned int domain_index, struct avb_stream_params *listener_stream_params)
{
	int rc;
	aar_crf_stream_t *crf;
	genavb_clock_domain_t clock_domain;

	crf = crf_stream_get(domain_index);

	if (!crf) {
		printf("init_media_clock_source: current crf config is null\n");
		goto err;
	}

	clock_domain = crf_get_clock_domain(domain_index);

	/* If CRF is already connected, do nothing: the media clock domain source should be properly set. */
	if (crf->stream_handle)
		goto exit;

	if (!listener_stream_params) {
		/* If connecting an AVTP talker stream without any previous CRF connection: set as media clock master. */
		rc = clock_domain_set_role(MEDIA_CLOCK_MASTER, clock_domain, NULL);
		if (rc != AVB_SUCCESS) {
			printf("clock_domain_set_role failed: %s\n", avb_strerror(rc));
			goto err;
		}
	} else {
		/* If connecting an AVTP listener stream without any previous CRF connection: set as media clock slave to that stream. */
		rc = clock_domain_set_role(MEDIA_CLOCK_SLAVE, clock_domain, listener_stream_params);
		if (rc != AVB_SUCCESS) {
			printf("clock_domain_set_role failed: %s\n", avb_strerror(rc));
			goto err;
		}
	}

exit:
	return 0;

err:
	return -1;
}

static int media_clock_init(unsigned int domain_index)
{
	int rc;
	aar_crf_stream_t *crf;
	genavb_clock_domain_t clock_domain;
	media_clock_role_t role;

	crf = crf_stream_get(domain_index);

	if (!crf) {
		printf("media_clock_init current crf config is null\n");
		goto err_crf;
	}

	clock_domain = crf_get_clock_domain(domain_index);

	if (crf->stream_params.direction == AVTP_DIRECTION_TALKER) {
		role = MEDIA_CLOCK_MASTER;
	} else {
		role = MEDIA_CLOCK_SLAVE;
	}

	if (crf->stream_handle) {
		ERR("CRF stream already initialized/connected for domain_index (%u)", domain_index);
		goto exit;
	}

	/*
	 * CRF static setup without AVDECC
	 */
	if (role == MEDIA_CLOCK_MASTER) {
		rc = clock_domain_set_role(role, clock_domain, NULL);
		if (rc != AVB_SUCCESS) {
			printf("clock_domain_set_role failed: %s\n", avb_strerror(rc));
			goto err_crf;
		}

		rc = crf_stream_create(domain_index);
		if (rc != AVB_SUCCESS) {
			printf("crf_stream_create failed: %s\n", avb_strerror(rc));
			goto err_clock_domain;
		}
	}
	else {
		rc = crf_stream_create(domain_index);
		if (rc != AVB_SUCCESS) {
			printf("crf_stream_create failed: %s\n", avb_strerror(rc));
			goto err_crf;
		}

		rc = clock_domain_set_role(role, clock_domain, &crf->stream_params);
		if (rc != AVB_SUCCESS) {
			printf("clock_domain_set_role failed: %s\n", avb_strerror(rc));
			goto err_clock_domain;
		}
	}

exit:
	return 0;

err_clock_domain:
	crf_stream_destroy(domain_index);
err_crf:
	return -1;
}

static void media_clock_exit(unsigned int domain_index)
{
	int rc;

	rc = crf_stream_destroy(domain_index);
	if (rc != AVB_SUCCESS)
		printf("clock_domain_set_role failed: %s\n", avb_strerror(rc));
}

static void static_crf_streams_exit(void)
{
	unsigned int i;

	for (i= 0; i < MAX_STATIC_STREAM_LISTENERS; i++) {
		if (listener_static_streams[i].requested_type ==  STREAM_CRF) {
			media_clock_exit(listener_static_streams[i].crf.index);
			break;
		}
	}

	for (i= 0; i < MAX_STATIC_STREAM_TALKERS; i++) {
		if (talker_static_streams[i].requested_type ==  STREAM_CRF) {
			media_clock_exit(talker_static_streams[i].crf.index);
			break;
		}
	}
}

static int static_crf_streams_init(void)
{
	unsigned int i;
	int rc;
	struct media_generic_stream *static_stream;

	for (i = 0; i < MAX_STATIC_STREAM_LISTENERS; i++) {
		static_stream = &listener_static_streams[i];
		if (static_stream->requested_type ==  STREAM_CRF) {
			rc = media_clock_init(static_stream->crf.index);
			if (rc != AVB_SUCCESS) {
				printf("media_clock_init for domain %u as listener failed\n", static_stream->crf.index);
				goto err;
			}
		} else {
			continue;
		}
	}

	for (i = 0; i < MAX_STATIC_STREAM_TALKERS; i++) {
		static_stream = &talker_static_streams[i];
		if (static_stream->requested_type ==  STREAM_CRF) {
			rc = media_clock_init(static_stream->crf.index);
			if (rc != AVB_SUCCESS) {
				printf("media_clock_init for domain %u as talker failed\n", static_stream->crf.index);
				goto err;
			}
		} else {
			continue;
		}
	}

	return 0;

err:
	static_crf_streams_exit();
	return -1;
}

static void talker_connect(struct avb_stream_params *params, unsigned int avdecc_stream_index)
{
	struct media_generic_stream *avdecc_talker;

	if  (avdecc_stream_index >= MAX_AVDECC_TALKERS)
		return;

	avdecc_talker = &talker_streams[avdecc_stream_index];

	if (avdecc_talker->handler_type != STREAM_NONE)
		return;

	if (avdecc_format_is_crf(&params->format)) {
		if (!crf_connect(MEDIA_CLOCK_MASTER, avdecc_talker->crf.index, params))
			avdecc_talker->handler_type = STREAM_CRF;

	} else {

		/* Before connecting any AVTP stream, check if we need to set the clock domain source for AVB_CLOCK_DOMAIN_0 */
		if (init_media_clock_source(0, NULL) < 0) {
			printf("%s:  init_media_clock_source() failed for AVB_CLOCK_DOMAIN_0, can not connect stream output (%u)\n", __func__, avdecc_stream_index);
			return;
		}

		if ((avdecc_format_is_aaf_pcm(&params->format)) || ((avdecc_format_is_61883_6(&params->format)) && (avdecc_talker->requested_type != STREAM_GST) && (avdecc_talker->requested_type != STREAM_MULTI_GST))) {
			struct alsa_stream *talker = &alsa_talker_stream[avdecc_talker->alsa.index];

			talker->index = avdecc_talker->alsa.index;

			if (!talker_alsa_connect(talker, params))
				avdecc_talker->handler_type = STREAM_ALSA;
		} else if ( avdecc_format_is_61883_6(&params->format) || avdecc_format_is_61883_4(&params->format) || (params->subtype == AVTP_SUBTYPE_CVF)) {
			if (avdecc_talker->requested_type == STREAM_MULTI_GST) {
				struct gstreamer_talker_multi_handler *talker_multi = &gst_talker_multi_handler[avdecc_talker->multi_gst.input_index];
;
				if (!talker_gstreamer_multi_connect(params, talker_multi, avdecc_talker->multi_gst.output_index, avdecc_stream_index))
					avdecc_talker->handler_type = STREAM_MULTI_GST;
			} else {
				struct gstreamer_stream *talker = &gstreamer_talker_stream[avdecc_talker->gst.index];

				talker->source_index = avdecc_talker->gst.index;

				if (!talker_gstreamer_connect(talker, params, avdecc_stream_index))
					avdecc_talker->handler_type = STREAM_GST;
			}
		}
	}

}

static void talker_disconnect(unsigned int stream_index)
{
	struct media_generic_stream *avdecc_talker;

	if  (stream_index >= MAX_AVDECC_TALKERS)
		return;

	avdecc_talker = &talker_streams[stream_index];

	switch (avdecc_talker->handler_type) {
	case STREAM_ALSA:
		talker_alsa_disconnect(&alsa_talker_stream[avdecc_talker->alsa.index]);
		break;

	case STREAM_GST:
		talker_gstreamer_disconnect(&gstreamer_talker_stream[avdecc_talker->gst.index]);
		break;

	case STREAM_MULTI_GST:
		talker_gstreamer_multi_disconnect(&gst_talker_multi_handler[avdecc_talker->multi_gst.input_index], avdecc_talker->multi_gst.output_index);
		break;

	case STREAM_CRF:
		crf_disconnect(avdecc_talker->crf.index);
		break;

	case STREAM_NONE:
	default:
		break;
	}

	avdecc_talker->handler_type = STREAM_NONE;
}

static void listener_connect(struct avb_stream_params *params, unsigned int avdecc_stream_index)
{
	struct media_generic_stream *avdecc_listener;

	if  (avdecc_stream_index >= MAX_AVDECC_LISTENERS)
		return;

	avdecc_listener = &listener_streams[avdecc_stream_index];

	if (avdecc_listener->handler_type != STREAM_NONE)
		return;

	if (avdecc_format_is_crf(&params->format)) {
		if (!crf_connect(MEDIA_CLOCK_SLAVE, avdecc_listener->crf.index, params))
			avdecc_listener->handler_type = STREAM_CRF;
	} else {
		/* Before connecting any AVTP stream, check if we need to set the clock domain source for AVB_CLOCK_DOMAIN_0 */
		if (init_media_clock_source(0, params) < 0) {
			printf("%s:  init_media_clock_source() failed for AVB_CLOCK_DOMAIN_0, can not connect stream input (%u)\n", __func__, avdecc_stream_index);
			return;
		}

		if ((avdecc_format_is_aaf_pcm(&params->format)) || ((avdecc_format_is_61883_6(&params->format)) && (avdecc_listener->requested_type != STREAM_GST))) {
			struct alsa_stream *listener = &alsa_listener_stream[avdecc_listener->alsa.index];

			listener->index = avdecc_listener->alsa.index;

			if (!listener_alsa_connect(listener, params))
				avdecc_listener->handler_type = STREAM_ALSA;
		} else if (avdecc_format_is_61883_6(&params->format) || avdecc_format_is_61883_4(&params->format) || (params->subtype == AVTP_SUBTYPE_CVF)) {
			struct gstreamer_stream *listener = &gstreamer_listener_stream[avdecc_listener->gst.index];

			listener->source_index = avdecc_listener->gst.index;

			if (!listener_gstreamer_connect(listener, params, avdecc_stream_index))
				avdecc_listener->handler_type = STREAM_GST;
		}
	}
}

static void listener_disconnect(int stream_index)
{
	struct media_generic_stream *avdecc_listener;

	if  (stream_index >= MAX_AVDECC_LISTENERS)
		return;

	avdecc_listener = &listener_streams[stream_index];


	switch (avdecc_listener->handler_type) {
	case STREAM_ALSA:
		listener_alsa_disconnect(&alsa_listener_stream[avdecc_listener->alsa.index]);
		break;

	case STREAM_GST:
		listener_gstreamer_disconnect(&gstreamer_listener_stream[avdecc_listener->gst.index]);
		break;

	case STREAM_CRF:
		crf_disconnect(avdecc_listener->crf.index);
		break;

	case STREAM_NONE:
	default:
		break;
	}

	avdecc_listener->handler_type = STREAM_NONE;
}

static void avdecc_streams_disconnect(void)
{
	int i;

	for (i = 0; i < MAX_AVDECC_LISTENERS; i++)
		listener_disconnect(i);

	for (i = 0; i < MAX_ALSA_TALKERS; i++)
		talker_disconnect(i);

}

static int multi_talker_timeout_handler(struct gstreamer_talker_multi_handler *talker_multi, unsigned int events)
{
	int rc;
	int i;
	char tmp[8];
	int ret;
	struct talker_gst_multi_app *stream;

	ret = read(talker_multi->timer_fd, tmp, 8);
	if (ret < 0)
		printf("%s: [talker_multi %p] timer_fd read() failed: %s\n", __func__, talker_multi, strerror(errno));

	pthread_mutex_lock(&talker_multi->gst_media->stream_lock);

	for (i = 0; i < GST_MAX_SINKS; i++) {
		stream = talker_multi->multi_app[i];
		rc = talker_gst_multi_stream_fsm(stream, STREAM_EVENT_TIMER);
		if (rc < 0)
			goto err_unlock;
	}

	rc = talker_gst_multi_fsm(talker_multi->gst_media, GST_EVENT_TIMER);
	if (rc < 0)
		goto err_unlock;

	pthread_mutex_unlock(&talker_multi->gst_media->stream_lock);

	return 0;

err_unlock:
	pthread_mutex_unlock(&talker_multi->gst_media->stream_lock);
	return rc;
}

static int media_app_custom_gst_pipeline_setup(struct gstreamer_pipeline *gst)
{
	int i;
	int rc = 0;

	printf("%s Add pipeline  %p to gst bus monitored list \n", __func__, gst);
	/* Add pipeline to monitor list*/
	pthread_mutex_lock(&gstreamer_bus_monitor.list_lock);

	for (i = 0; i < MAX_GST_PIPELINES_MSG_MONITOR; i++) {
		if (!gstreamer_bus_monitor.gst_pipelines[i]) {
			gstreamer_bus_monitor.gst_pipelines[i] = gst;
			/* If first pipeline to monitor, enable the slot fd monitor*/
			if (!gstreamer_bus_monitor.nb_pipelines) {
				thread_slot_set_events(gstreamer_bus_monitor.thread_slot, 1, EPOLLIN);
			}
			gstreamer_bus_monitor.nb_pipelines++;
			goto exit;
		}
	}

	printf("[Error] %s: Can not find a free spot in the gst bus monitor list for pipeline %p \n", __func__, gst);
	rc = -1;

exit:
	pthread_mutex_unlock(&gstreamer_bus_monitor.list_lock);
	return rc;
}

static void media_app_custom_gst_pipeline_teardown(struct gstreamer_pipeline *gst)
{
	int i;
	printf("%s Remove pipeline  %p from gst bus monitored list \n", __func__, gst);

	/* Remove pipeline to monitor list*/
	pthread_mutex_lock(&gstreamer_bus_monitor.list_lock);

	for (i = 0; i < MAX_GST_PIPELINES_MSG_MONITOR; i++) {
		if (gstreamer_bus_monitor.gst_pipelines[i] == gst) {
			gstreamer_bus_monitor.gst_pipelines[i] = NULL;
			gstreamer_bus_monitor.nb_pipelines--;
			/* If there is no more  pipelines to monitor, disbale the slot fd monitor*/
			if (!gstreamer_bus_monitor.nb_pipelines) {
				thread_slot_set_events(gstreamer_bus_monitor.thread_slot, 0, EPOLLIN);
			}
			pthread_mutex_unlock(&gstreamer_bus_monitor.list_lock);
			return;
		}
	}

	printf("[Error] %s: Can not find gst %p  in the gst bus monitor list \n", __func__, gst);
	pthread_mutex_unlock(&gstreamer_bus_monitor.list_lock);

}


static int avdecc_multi_talker_streams_init(void)
{
	int i,j;
	struct gstreamer_talker_multi_handler *talker_multi;

	for (i = 0; i < MAX_GST_MULTI_HANDLERS; i++) {
		talker_multi = &gst_talker_multi_handler[i];

		if(!talker_multi->configured_streams)
			continue;

		talker_multi->gst_media = &gstreamer_multi_talker_media[i];
		talker_multi->gst_media->state = GST_STATE_STOPPED;
		talker_multi->gst_media->gst_pipeline = &talker_multi->gst_pipeline;
		talker_multi->gst_media->gst_pipeline->custom_gst_pipeline_setup = media_app_custom_gst_pipeline_setup;
		talker_multi->gst_media->gst_pipeline->custom_gst_pipeline_teardown = media_app_custom_gst_pipeline_teardown;
		talker_multi->gst_pipeline.talker.gst = &gstreamer_multi_talker_media[i];

		/* Init internal arrays */
		for (j = 0; j < GST_MAX_SINKS; j++ ) {
			talker_multi->multi_app[j] = &gstreamer_multi_talker_multi_app[j];
			talker_multi->talkers[j] = &gstreamer_multi_talker_streams[j];
			talker_multi->multi_app[j]->gst = talker_multi->gst_media;
			talker_multi->multi_app[j]->state = STREAM_STATE_DISCONNECTED;
			talker_multi->multi_app[j]->sink_index = j;
			talker_multi->gst_pipeline.sink[j].data = talker_multi->multi_app[j];
		}

		talker_multi->timer_fd = create_stream_timer(POLL_TIMEOUT_MS);
		if (talker_multi->timer_fd < 0) {
			printf("%s timerfd_create() failed %s\n", __func__, strerror(errno));
			goto error_stream_timer;
		}

		if (thread_slot_add(THR_CAP_TIMER | THR_CAP_GST_MULTI, talker_multi->timer_fd, EPOLLIN, talker_multi, (int (*)(void *, unsigned int events))multi_talker_timeout_handler, NULL, 0, (thr_thread_slot_t **)&talker_multi->thread_timer) < 0)
			goto error_timer_slot;
	}
	return 0;

error_timer_slot:
	close(talker_multi->timer_fd);
error_stream_timer:
	return -1;

}

static void avdecc_multi_talker_streams_exit(void)
{
	int i,j;
	struct gstreamer_talker_multi_handler *talker_multi;

	for (i = 0; i < MAX_GST_MULTI_HANDLERS; i++) {
		talker_multi = &gst_talker_multi_handler[i];

		if(!talker_multi->configured_streams)
			continue;

		thread_slot_free(talker_multi->thread_timer);

		close(talker_multi->timer_fd);

		pthread_mutex_lock(&talker_multi->gst_media->stream_lock);

		for (j = 0; j < GST_MAX_SINKS; j++ )
			talker_gst_multi_stream_fsm(talker_multi->multi_app[j], STREAM_EVENT_DISCONNECT);

		pthread_mutex_unlock(&talker_multi->gst_media->stream_lock);
	}

}

static int handle_avdecc_event(struct avb_control_handle *ctrl_h, const char *binding_file_name)
{
	struct avb_stream_params *params;
	union avb_media_stack_msg msg;
	unsigned int msg_type, msg_len;
	int rc;

	msg_len = sizeof(union avb_media_stack_msg);
	rc = avb_control_receive(ctrl_h, &msg_type, &msg, &msg_len);
	if (rc != AVB_SUCCESS)
		goto error_control_receive;

	switch (msg_type) {
	case AVB_MSG_MEDIA_STACK_CONNECT:

		params = &msg.media_stack_connect.stream_params;

		printf("AVB_MSG_MEDIA_STACK_CONNECT %u with flags 0x%x \n", msg.media_stack_connect.stream_index, msg.media_stack_connect.flags);

		if (params->direction == AVTP_DIRECTION_TALKER) {
			talker_connect(params, msg.media_stack_connect.stream_index);
		} else {
			listener_connect(params, msg.media_stack_connect.stream_index);
		}

		break;

	case AVB_MSG_MEDIA_STACK_DISCONNECT:

		printf("AVB_MSG_MEDIA_STACK_DISCONNECT %u\n", msg.media_stack_disconnect.stream_index);

		if (msg.media_stack_disconnect.direction == AVTP_DIRECTION_TALKER) {
			talker_disconnect(msg.media_stack_disconnect.stream_index);
		} else {
			listener_disconnect(msg.media_stack_disconnect.stream_index);
		}

		break;

	case GENAVB_MSG_MEDIA_STACK_BIND:
		printf("GENAVB_MSG_MEDIA_STACK_BIND: Controller (%016"PRIx64") bound listener stream (%016"PRIx64", %u, %s) to talker stream (%016"PRIx64", %u) \n",
				msg.media_stack_bind.controller_entity_id,
				msg.media_stack_bind.entity_id, msg.media_stack_bind.listener_stream_index, (msg.media_stack_bind.started == ACMP_LISTENER_STREAM_STARTED) ? "STARTED" : "STOPPED",
				msg.media_stack_bind.talker_entity_id, msg.media_stack_bind.talker_stream_index);

		if (binding_file_name)
			avdecc_nvm_bindings_update(binding_file_name, &msg.media_stack_bind);
		break;

	case GENAVB_MSG_MEDIA_STACK_UNBIND:
		printf("GENAVB_MSG_MEDIA_STACK_UNBIND: listener stream (%016"PRIx64", %u) has been unbound\n",
				msg.media_stack_bind.entity_id, msg.media_stack_bind.listener_stream_index);

		if (binding_file_name)
			avdecc_nvm_bindings_remove(binding_file_name, &msg.media_stack_unbind);
		break;

	default:
		break;
	}

error_control_receive:
	return rc;
}

void signal_terminate_handler (int signal_num)
{
	signal_terminate = 1;
}

static void alsa_talkers_device_config_init(char *alsa_device)
{
	struct alsa_stream *alsa_stream;
	unsigned int i;

	for (i = 0; i < MAX_ALSA_TALKERS; i++) {
		alsa_stream = &alsa_talker_stream[i];

		// All alsa handlers point to the same default device
		alsa_stream->alsa_device = alsa_device;
	}
}

static void alsa_listeners_device_config_init(char *alsa_device)
{
	struct alsa_stream *alsa_stream;
	unsigned int i;

	for (i = 0; i < MAX_ALSA_LISTENERS; i++) {
		alsa_stream = &alsa_listener_stream[i];

		// All alsa handlers point to the same default device
		alsa_stream->alsa_device = alsa_device;
	}
}

void gstreamer_config_init(void)
{
	struct gstreamer_pipeline *pipeline;
	unsigned int i;

	gstreamer_init();

	for (i = 0; i < MAX_GSTREAMER_LISTENERS; i++) {
		pipeline = &gstreamer_listener_pipelines[i];

		// Map each gstreamer stream to its pipeline
		gstreamer_listener_stream[i].pipe_source.gst_pipeline = pipeline;
		gstreamer_pipeline_init(pipeline);
		pipeline->custom_gst_pipeline_setup = media_app_custom_gst_pipeline_setup;
		pipeline->custom_gst_pipeline_teardown = media_app_custom_gst_pipeline_teardown;
	}
	gstreamer_pipeline_init(&split_screen_pipeline);

	for (i = 0; i < MAX_GSTREAMER_TALKERS; i++) {
		pipeline = &gstreamer_talker_pipelines[i];

		// Map each gstreamer stream to its pipeline
		gstreamer_talker_stream[i].pipe_source.gst_pipeline = pipeline;

		gstreamer_pipeline_init(pipeline);
		pipeline->custom_gst_pipeline_setup = media_app_custom_gst_pipeline_setup;
		pipeline->custom_gst_pipeline_teardown = media_app_custom_gst_pipeline_teardown;
	}

	for (i = 0; i < MAX_GST_MULTI_HANDLERS; i++) {
		pipeline = &gst_talker_multi_handler[i].gst_pipeline;

		gstreamer_pipeline_init(pipeline);
		pipeline->custom_gst_pipeline_setup = media_app_custom_gst_pipeline_setup;
		pipeline->custom_gst_pipeline_teardown = media_app_custom_gst_pipeline_teardown;
	}

	for (i = 0; i < MAX_GST_MULTI_HANDLERS; i++) {
		pipeline = &gst_listener_multi_handler[i].gst_pipeline;

		gstreamer_pipeline_init(pipeline);
		pipeline->custom_gst_pipeline_setup = media_app_custom_gst_pipeline_setup;
		pipeline->custom_gst_pipeline_teardown = media_app_custom_gst_pipeline_teardown;
	}

#ifdef CFG_AVTP_1722A
	salsa_camera_init();
	h264_camera_init();
#endif
}

void gstreamer_config_exit(void)
{
	gstreamer_reset();
}

static int static_streams_init(void)
{
	int rc = 0;
#ifdef CFG_AVTP_1722A
	int i;
	struct gstreamer_pipeline_config *gst_config = NULL;
	struct media_generic_stream *static_stream;

	for (i = 0; i < MAX_STATIC_STREAM_LISTENERS; i++) {
		static_stream = &listener_static_streams[i];

		if (static_stream->requested_type == STREAM_GST)
			gst_config = &gstreamer_listener_pipelines[static_stream->gst.index].config;
		else if (static_stream->requested_type == STREAM_MULTI_GST)
			gst_config = &gst_listener_multi_handler[static_stream->multi_gst.output_index].gst_pipeline.config;
		else
			continue;

		/* Check if a previous stream has passed for the same config*/
		if(gst_config->configured)
			continue;

		if (gst_config->listener.flags & GST_FLAG_CAMERA) {
			if(gst_config->listener.camera_type == CAMERA_TYPE_SALSA) {
				switch (gst_config->nstreams) {
				case 1:
					if (static_stream->requested_type == STREAM_GST) {
						if (salsa_camera_connect(&gstreamer_listener_stream[static_stream->gst.index]) < 0) {
							printf("Couldn't start Salsa camera listener.\n");
							rc = -1;
							goto exit;
						}
					} else if (static_stream->requested_type == STREAM_MULTI_GST) {
						if (multi_salsa_split_screen_start(1, &gst_listener_multi_handler[static_stream->multi_gst.output_index].gst_pipeline) < 0) {
							printf("Couldn't start split screen\n");
							rc = -1;
							goto exit;
						}

					}
					break;
				case 2:
				case 3:
				case 4:
				default:
					if (multi_salsa_split_screen_start(gst_config->nstreams, &gst_listener_multi_handler[static_stream->multi_gst.output_index].gst_pipeline) < 0) {
						printf("Couldn't start split screen\n");
						rc = -1;
						goto exit;
					}
					break;
				}
			}
			else if (gst_config->listener.camera_type == CAMERA_TYPE_H264_1722_2013 || gst_config->listener.camera_type == CAMERA_TYPE_H264_1722_2016) {

				if(gst_config->nstreams != 1) {
					printf("[ERROR] H264 Multi stream is not supported\n");
					rc = -1;
					goto exit;
				}

				if (h264_camera_connect(&gstreamer_listener_stream[static_stream->gst.index]) < 0) {
						printf("Couldn't start H264 camera listener.\n");
						rc = -1;
						goto exit;
				}
			}
		} else if (gst_config->listener.flags & GST_FLAG_BEV) {
			if (multi_salsa_surround_start(gst_config) < 0) {
				printf("Couldn't start Surround view\n");
				rc = -1;
				goto exit;
			}
		}
		gst_config->configured = 1;
	}

exit:
#endif
	return rc;
}

static void static_streams_exit(void)
{
#ifdef CFG_AVTP_1722A
	int i;
	struct gstreamer_pipeline_config *gst_config = NULL;
	struct media_generic_stream *static_stream;

	for (i = 0; i < MAX_STATIC_STREAM_LISTENERS; i++) {

		static_stream = &listener_static_streams[i];

		if (static_stream->requested_type == STREAM_GST)
			gst_config = &gstreamer_listener_pipelines[static_stream->gst.index].config;
		else if (static_stream->requested_type == STREAM_MULTI_GST)
			gst_config = &gst_listener_multi_handler[static_stream->multi_gst.output_index].gst_pipeline.config;
		else
			continue;

		/* Check if a previous stream has passed for the same config*/
		if(!gst_config->configured)
			continue;

		if (gst_config->listener.flags & GST_FLAG_CAMERA) {
			if(gst_config->listener.camera_type == CAMERA_TYPE_SALSA) {
				switch (gst_config->nstreams) {
				case 2:
				case 3:
				case 4:
					multi_salsa_camera_disconnect(gst_config->nstreams);
					gst_stop_pipeline(&gst_listener_multi_handler[static_stream->multi_gst.output_index].gst_pipeline);
					break;
				case 1:
					salsa_camera_disconnect(&gstreamer_listener_stream[static_stream->gst.index]);
					break;
				default:
					printf("ERROR: invalid number of camera streams (%d)\n", gst_config->nstreams);
					break;
				}
			}
			else if (gst_config->listener.camera_type == CAMERA_TYPE_H264_1722_2013 || gst_config->listener.camera_type == CAMERA_TYPE_H264_1722_2016)
			{
				h264_camera_disconnect(&gstreamer_listener_stream[static_stream->gst.index]);
			}
		} else if (gst_config->listener.flags & GST_FLAG_BEV) {
			multi_salsa_camera_disconnect(MAX_CAMERA);
			surround_exit();
		}

		gst_config->configured = 0;
	}
#endif
}

static void set_default_device_width ( const char *device, unsigned int *width )
{
	if (!strcmp(device,V4L2_HDMI_DEVICE_FILE) || !strcmp(device,FB_HDMI_DEVICE_FILE))
		*width = DEFAULT_HDMI_WIDTH;
	else
		*width = DEFAULT_LVDS_WIDTH;
}
static void set_default_device_height ( const char *device, unsigned int *height )
{
	if (!strcmp(device,V4L2_HDMI_DEVICE_FILE) || !strcmp(device,FB_HDMI_DEVICE_FILE))
		*height = DEFAULT_HDMI_HEIGHT;
	else
		*height = DEFAULT_LVDS_HEIGHT;
}

int main(int argc, char *argv[])
{
	struct avb_control_handle *ctrl_h = NULL;
	struct avdecc_controlled controlled = { 0 };
	int ctrl_rx_fd, clock_domain_rx_fd;
	struct pollfd ctrl_poll[NUM_CONTROL_FDS];
	int option;
	thr_thread_slot_t *stats_thread_slot = NULL;
	thr_thread_slot_t *controlled_thread_slot = NULL;
	int stats_fd;
	struct sched_param param = {
		.sched_priority = 1,
	};
	struct gstreamer_pipeline_config *current_gst_config = NULL;
	struct alsa_stream *current_alsa_config = NULL;
	aar_crf_stream_t *current_crf_config = NULL;
	int rc = 0;
	avb_u64 crf_stream_id = 0;
	struct sigaction action;
	int i;
	unsigned long long pts_offset;
	unsigned int current_gst_handler_index = 0;
	unsigned int current_alsa_handler_index = 0;
	unsigned int current_crf_handler_index = 0;
	unsigned int multi_handler_input_index = 0;
	unsigned int multi_handler_output_index = 0;
	int current_avdecc_stream_index = -1;
	int current_static_stream_index = -1;
	int current_config_direction = -1;
	char *input_index, *output_index;
	unsigned long long optval_ull;
	unsigned long optval_ul;
	char *binding_file_name = NULL;

	setlinebuf(stdout);

	printf("NXP's GenAVB reference media application\n");

	rc = sched_setscheduler(0, SCHED_FIFO, &param);
	if (rc < 0) {
		printf("sched_setscheduler() failed: %s\n", strerror(errno));
		goto err_sched;
	}

	avdecc_streams_init();
	audio_mappings_init();
	gstreamer_config_init();
	alsa_talkers_device_config_init(DEFAULT_ALSA_CAPTURE_DEVICE);
	alsa_listeners_device_config_init(DEFAULT_ALSA_PLAYBACK_DEVICE);

#ifdef CFG_AVTP_1722A
	while ((option = getopt(argc, argv,"c:vaBS:d:r:hp:t:f:lLTH:I:FD:A:Mg:m:z:C:P:b:")) != -1) {
#else
	while ((option = getopt(argc, argv,"c:vad:r:hp:t:f:lLTFD:A:S:g:m:z:C:P:b:")) != -1) {
#endif
		switch (option) {
		case 'L':
			current_avdecc_stream_index = -1;
			current_static_stream_index = -1;
			current_gst_config = NULL;
			current_alsa_config = NULL;
			current_crf_config = NULL;
			multi_handler_input_index = 0;
			multi_handler_output_index = 0;
			current_config_direction = AVTP_DIRECTION_LISTENER;
			break;
		case 'T':
			current_config_direction = AVTP_DIRECTION_TALKER;
			current_avdecc_stream_index = -1;
			current_static_stream_index = -1;
			current_gst_config = NULL;
			current_alsa_config = NULL;
			current_crf_config = NULL;
			multi_handler_input_index = 0;
			multi_handler_output_index = 0;
			break;
		case 'A':
			if (h_strtoul(&optval_ul, optarg, NULL, 0) < 0)
				goto err_sched;

			current_avdecc_stream_index = (int)optval_ul;
			current_gst_config = NULL;
			current_alsa_config = NULL;
			current_crf_config = NULL;
			break;
		case 'S':
			if (h_strtoul(&optval_ul, optarg, NULL, 0) < 0)
				goto err_sched;

			current_static_stream_index = (int)optval_ul;
			current_gst_config = NULL;
			current_alsa_config = NULL;
			current_crf_config = NULL;
			break;
		case 'z':
			if (h_strtoul(&optval_ul, optarg, NULL, 0) < 0)
				goto err_sched;

			current_alsa_handler_index = (unsigned int)optval_ul;

			if (current_avdecc_stream_index >= 0) {
				if(current_config_direction == AVTP_DIRECTION_TALKER) {
					if (current_alsa_handler_index >= MAX_ALSA_TALKERS){
						printf("[ERROR] Alsa handler index above max (MAX_ALSA_TALKERS=%d)\n", MAX_ALSA_TALKERS);
						goto err_sched;
					}
					talker_streams[current_avdecc_stream_index].requested_type = STREAM_ALSA;
					talker_streams[current_avdecc_stream_index].alsa.index = current_alsa_handler_index;
					current_alsa_config = &alsa_talker_stream[current_alsa_handler_index];
				} else if (current_config_direction ==  AVTP_DIRECTION_LISTENER) {
					if (current_alsa_handler_index >= MAX_ALSA_LISTENERS){
						printf("[ERROR] Alsa handler index above max (MAX_ALSA_LISTENERS=%d)\n", MAX_ALSA_LISTENERS);
						goto err_sched;
					}
					listener_streams[current_avdecc_stream_index].requested_type = STREAM_ALSA;
					listener_streams[current_avdecc_stream_index].alsa.index = current_alsa_handler_index;
					current_alsa_config = &alsa_listener_stream[current_alsa_handler_index];
				} else {
					printf("[ERROR] No Stream direction specified for the avdecc stream. \n");
					goto err_sched;
				}
			} else {
				printf("[ERROR] No stream index specified. \n");
				goto err_sched;
			}
			break;
		case 'g':
			if (h_strtoul(&optval_ul, optarg, NULL, 0) < 0)
				goto err_sched;

			current_gst_handler_index = (unsigned int)optval_ul;

			if (current_avdecc_stream_index >= 0) {
				if(current_config_direction == AVTP_DIRECTION_TALKER) {
					talker_streams[current_avdecc_stream_index].requested_type = STREAM_GST;
					talker_streams[current_avdecc_stream_index].gst.index = current_gst_handler_index;
					current_gst_config = &gstreamer_talker_pipelines[current_gst_handler_index].config;
					current_gst_config->type = GST_TYPE_TALKER;
				} else if (current_config_direction ==  AVTP_DIRECTION_LISTENER) {
					listener_streams[current_avdecc_stream_index].requested_type = STREAM_GST;
					listener_streams[current_avdecc_stream_index].gst.index = current_gst_handler_index;
					current_gst_config = &gstreamer_listener_pipelines[current_gst_handler_index].config;
					current_gst_config->type = GST_TYPE_LISTENER;
				} else {
					printf("[ERROR] No Stream direction specified for the avdecc stream. \n");
					goto err_sched;
				}
			} else if (current_static_stream_index >= 0) {
				/* Only support static listeners*/
				listener_static_streams[current_static_stream_index].requested_type = STREAM_GST;
				listener_static_streams[current_static_stream_index].gst.index = current_gst_handler_index;
				current_gst_config = &gstreamer_listener_pipelines[current_gst_handler_index].config;
			} else {
				printf("[ERROR] No stream index specified. \n");
				goto err_sched;
			}
			current_gst_config->nstreams = 1; /* Single stream handler*/
			break;
		case 'm':
			input_index = strtok(optarg, ":");
			if (!input_index) {
				printf("[ERROR] Bad multi handler specification.\n");
				goto err_sched;
			}

			output_index = strtok(NULL, ":");
			if (!output_index) {
				printf("[ERROR] Bad multi handler specification.\n");
				goto err_sched;
			}

			if (h_strtoul(&optval_ul, input_index, NULL, 0) < 0)
				goto err_sched;

			multi_handler_input_index = (unsigned int) optval_ul;

			if (h_strtoul(&optval_ul, output_index, NULL, 0) < 0)
				goto err_sched;

			multi_handler_output_index = (unsigned int)optval_ul;

			if (current_avdecc_stream_index >= 0) {
				if(current_config_direction == AVTP_DIRECTION_TALKER) {
					talker_streams[current_avdecc_stream_index].requested_type = STREAM_MULTI_GST;
					talker_streams[current_avdecc_stream_index].multi_gst.input_index = multi_handler_input_index;
					talker_streams[current_avdecc_stream_index].multi_gst.output_index = multi_handler_output_index;
					current_gst_config = &gst_talker_multi_handler[multi_handler_input_index].gst_pipeline.config;
					current_gst_config->type |= GST_TYPE_MULTI_TALKER;
					gst_talker_multi_handler[multi_handler_input_index].configured_streams++;
				} else if (current_config_direction == GST_TYPE_LISTENER) {
					printf("[ERROR] Avdecc listener gstreamer multi handler is not supported\n");
					goto err_sched;
				}
			} else if (current_static_stream_index >= 0) {
				if(current_config_direction == AVTP_DIRECTION_LISTENER) {
					listener_static_streams[current_static_stream_index].requested_type = STREAM_MULTI_GST;
					listener_static_streams[current_static_stream_index].multi_gst.input_index = multi_handler_input_index;
					listener_static_streams[current_static_stream_index].multi_gst.output_index = multi_handler_output_index;
					current_gst_config = &gst_listener_multi_handler[multi_handler_output_index].gst_pipeline.config;
					current_gst_config->listener.stream_ids_mappping[current_gst_config->nstreams].source_index = multi_handler_input_index;
					current_gst_config->listener.stream_ids_mappping[current_gst_config->nstreams].sink_index = multi_handler_output_index;
				} else if (current_config_direction == AVTP_DIRECTION_TALKER) {
					printf("[ERROR] STATIC talker gstreamer multi handler is not supported\n");
					goto err_sched;
				}
			} else {
				printf("[ERROR] No stream index specified. \n");
				goto err_sched;
			}
			current_gst_config->nstreams++;
			break;
		case 'c':
			if (h_strtoul(&optval_ul, optarg, NULL, 0) < 0)
				goto err_sched;

			current_crf_handler_index = (unsigned int)optval_ul;

			if (current_crf_handler_index >= MAX_CLOCK_DOMAIN) {
				printf("[ERROR] CRF handler index above max (MAX_CLOCK_DOMAIN=%d)\n", MAX_CLOCK_DOMAIN);
				goto err_sched;
			}

			current_crf_config = crf_stream_get(current_crf_handler_index);

			if (!current_crf_config) {
				printf("CRF stream not found for domain_index (%u)\n", current_crf_handler_index);
				goto err_sched;
			}

			if (current_avdecc_stream_index >= 0) {
				if(current_config_direction == AVTP_DIRECTION_TALKER) {
					talker_streams[current_avdecc_stream_index].requested_type = STREAM_CRF;
					talker_streams[current_avdecc_stream_index].crf.index = current_crf_handler_index;
					if (crf_configure(current_crf_handler_index, AVTP_DIRECTION_TALKER, 0) < 0) {
						printf("[ERROR] Cannot configure CRF current config\n");
						goto err_sched;
					}

				} else if (current_config_direction ==  AVTP_DIRECTION_LISTENER) {
					listener_streams[current_avdecc_stream_index].requested_type = STREAM_CRF;
					listener_streams[current_avdecc_stream_index].crf.index = current_crf_handler_index;
					if (crf_configure(current_crf_handler_index, AVTP_DIRECTION_LISTENER, AVB_STREAM_FLAGS_MCR) < 0) {
						printf("[ERROR] Cannot configure CRF current config\n");
						goto err_sched;
					}

				} else {
					printf("[ERROR] No Stream direction specified for the avdecc stream. \n");
					goto err_sched;
				}

			} else if (current_static_stream_index >= 0) {
				if (!current_crf_config->is_static_config){
					if(current_config_direction == AVTP_DIRECTION_TALKER) {
						talker_static_streams[current_static_stream_index].requested_type = STREAM_CRF;
						talker_static_streams[current_static_stream_index].crf.index = current_crf_handler_index;
						if (crf_configure(current_crf_handler_index, AVTP_DIRECTION_TALKER, 0) < 0) {
							printf("[ERROR] Cannot configure CRF current config\n");
							goto err_sched;
						}
						current_crf_config->is_static_config = true;

					} else if (current_config_direction ==  AVTP_DIRECTION_LISTENER) {
						listener_static_streams[current_static_stream_index].requested_type = STREAM_CRF;
						listener_static_streams[current_static_stream_index].crf.index = current_crf_handler_index;
						if (crf_configure(current_crf_handler_index, AVTP_DIRECTION_LISTENER, AVB_STREAM_FLAGS_MCR) < 0) {
							printf("[ERROR] Cannot configure CRF current config\n");
							goto err_sched;
						}
						current_crf_config->is_static_config = true;

					} else {
						printf("[ERROR] No Stream direction specified for the static stream. \n");
						goto err_sched;
					}
				} else {
					printf("[ERROR] Support for single static configuration per clock domain: crf handler %u previously configured with static stream\n", current_crf_handler_index);
					goto err_sched;
				}
			} else {
				printf("[ERROR] No stream index specified. \n");
				goto err_sched;
			}
			break;
		case 'p':
			if (!current_gst_config) {
				printf("[Error]: No stream is specified\n");
				goto err_sched;
			}
			if (h_strtoull(&pts_offset, optarg, NULL, 0) < 0)
				goto err_sched;

			if (pts_offset > MAX_PTS_OFFSET)
				pts_offset = MAX_PTS_OFFSET;

			if (current_config_direction == AVTP_DIRECTION_TALKER)
				current_gst_config->talker.preview_ts_offset = pts_offset;
			else
				current_gst_config->listener.pts_offset = pts_offset;

			break;

		case 'v':
			if (!current_gst_config) {
				printf("[Error]: No stream is specified\n");
				goto err_sched;
			}
			current_gst_config->type |= GST_TYPE_VIDEO;
			break;

		case 'a':
			if (!current_gst_config) {
				printf("[Error]: No stream is specified\n");
				goto err_sched;
			}
			current_gst_config->type |= GST_TYPE_AUDIO;
			break;

		case 'd':
			if (!current_gst_config) {
				printf("[Error]: No stream is specified\n");
				goto err_sched;
			}
			if (!strcasecmp(optarg, "lvds")) {
				if ((current_gst_config->type & GST_TYPE_LISTENER) && (current_gst_config->listener.flags & GST_FLAG_BEV))
					current_gst_config->device = FB_LVDS_DEVICE_FILE;
				else
					current_gst_config->device = V4L2_LVDS_DEVICE_FILE;
			}
			else if (!strcasecmp(optarg, "hdmi")) {
				if  ((current_gst_config->type & GST_TYPE_LISTENER) && (current_gst_config->listener.flags & GST_FLAG_BEV))
					current_gst_config->device = FB_HDMI_DEVICE_FILE;
				else
					current_gst_config->device = V4L2_HDMI_DEVICE_FILE;
			}
			else {
				usage();
				goto err_sched;
			}

			break;

		case 'r':
			if (!current_gst_config) {
				printf("[Error]: No stream index is specified\n");
				goto err_sched;
			}
			if (!strcasecmp(optarg, "1080")) {
				current_gst_config->width = 1920;
				current_gst_config->height = 1080;
			} else if (!strcasecmp(optarg, "720")) {
				current_gst_config->width = 1280;
				current_gst_config->height = 720;
			} else if (!strcasecmp(optarg, "768")) {
				current_gst_config->width = 1024;
				current_gst_config->height = 768;
			} else if (!strcasecmp(optarg, "480")) {
				current_gst_config->width = 640;
				current_gst_config->height = 480;
			} else {
				usage();
				goto err_sched;
			}

			break;
#ifdef CFG_AVTP_1722A
		case 'M':
			if (!current_gst_config) {
				printf("[Error]: No stream is specified\n");
				goto err_sched;
			}
			current_gst_config->type = GST_TYPE_LISTENER;
			current_gst_config->listener.flags |= GST_FLAG_CAMERA;
			current_gst_config->type |= GST_TYPE_VIDEO;
			current_gst_config->listener.camera_type = CAMERA_TYPE_SALSA;
			break;
		case 'B':
			if (!current_gst_config) {
				printf("[Error]: No stream is specified\n");
				goto err_sched;
			}
			current_gst_config->type = GST_TYPE_LISTENER;
			current_gst_config->listener.flags |= GST_FLAG_BEV;
			current_gst_config->type |= GST_TYPE_VIDEO;
			break;
		case 'H':
			if (!current_gst_config) {
				printf("[Error]: No stream is specified\n");
				goto err_sched;
			}
			current_gst_config->type = GST_TYPE_LISTENER;
			current_gst_config->listener.flags |= GST_FLAG_CAMERA;
			current_gst_config->type |= GST_TYPE_VIDEO;
			if (!strcasecmp(optarg, "H264_1722_2016")) {
				current_gst_config->listener.camera_type = CAMERA_TYPE_H264_1722_2016;
				printf("H264 camera based on IEEE1722_2016 selected\n");
			} else if (!strcasecmp(optarg, "H264_1722_2013")) {
				current_gst_config->listener.camera_type = CAMERA_TYPE_H264_1722_2013;
				printf("H264 camera based on IEEE1722_2013 selected\n");
			}
			break;
		case 'I':
			if (!current_gst_config && !current_crf_config) {
				printf("[Error]: No stream is specified\n");
				goto err_sched;
			}

			if (h_strtoull(&optval_ull, optarg, NULL, 0) < 0)
				goto err_sched;

			if (current_gst_config) {
				current_gst_config->listener.stream_ids_mappping[current_gst_config->nstreams - 1].stream_id = htonll(optval_ull);
				current_gst_config->listener.stream_ids_mappping[current_gst_config->nstreams - 1].source_index = multi_handler_input_index;
				current_gst_config->listener.stream_ids_mappping[current_gst_config->nstreams - 1].sink_index = multi_handler_output_index;

			} else if (current_crf_config) {
				crf_stream_id = htonll(optval_ull);
				memcpy(&current_crf_config->stream_params.stream_id, &crf_stream_id, sizeof(crf_stream_id));
			}
			break;
		case 'F':
			if (!current_gst_config) {
				printf("[Error]: No stream is specified\n");
				goto err_sched;
			}
			current_gst_config->sync_render_to_clock = 0;
			break;
#endif

		case 'f':
			if (!current_gst_config) {
				printf("[Error]: No stream is specified\n");
				goto err_sched;
			}
			current_gst_config->talker.input_media_file_name = optarg;
			rc = gst_build_media_file_list(current_gst_config);

			if (rc < 0)
				goto err_sched;
			printf("Talker mode : argument file name %s \n",optarg);
			if (current_gst_config->talker.n_input_media_files > 1) {
				current_gst_config->talker.input_media_file_index = current_gst_config->talker.n_input_media_files - 1;
				printf("Found %d media files in %s, using %s as first file.\n", current_gst_config->talker.n_input_media_files, current_gst_config->talker.input_media_file_name, current_gst_config->talker.input_media_files[current_gst_config->talker.input_media_file_index]);
			}

			break;
		case 'D':
			if (!current_gst_config) {
				printf("[Error]: No stream is specified\n");
				goto err_sched;
			}
			current_gst_config->listener.flags |= GST_FLAG_DEBUG;
			current_gst_config->listener.debug_file_dump_location = optarg;
			break;
		case 'l':
			if (!current_gst_config) {
				printf("[Error]: No stream is specified\n");
				goto err_sched;
			}
			current_gst_config->talker.flags |= GST_FLAG_PREVIEW;
			break;
		case 'C':
		case 'P':
			if (current_gst_config || current_alsa_config) {
				if (current_config_direction >= 0 &&
					((current_config_direction == AVTP_DIRECTION_TALKER && option == 'P')
					|| (current_config_direction == AVTP_DIRECTION_LISTENER && option == 'C'))) {
					printf("[Error]: Wrong ALSA device config\n");
					goto err_sched;
				}

				if (current_gst_config)
					current_gst_config->alsa_device = optarg;
				else
					current_alsa_config->alsa_device = optarg;

			} else {
				/* If no specific media handler is specified, set for all avdecc talker or listener alsa handlers */
				if (option == 'C')
					alsa_talkers_device_config_init(optarg);
				else
					alsa_listeners_device_config_init(optarg);
			}
			break;
		case 'b':
			binding_file_name = optarg;
			break;
		case 'h':
		default:
			usage();
			rc = -1;
			goto err_sched;
		}
	}

	/*Apply default configurations for enabled listener streams*/
	for (i = 0; i < MAX_GSTREAMER_LISTENERS; i++) {
		current_gst_config = &gstreamer_listener_pipelines[i].config;

		if (!(current_gst_config->type & GST_TYPE_LISTENER))
			continue;

		if (!current_gst_config->device)
				current_gst_config->device = V4L2_LVDS_DEVICE_FILE;

		if (!current_gst_config->width)
			set_default_device_width(current_gst_config->device, &current_gst_config->width);

		if(!current_gst_config->height)
			set_default_device_height(current_gst_config->device, &current_gst_config->height);

		if (!(current_gst_config->type & GST_TYPE_VIDEO) && !(current_gst_config->type & GST_TYPE_AUDIO))
			current_gst_config->type |= GST_TYPE_VIDEO | GST_TYPE_AUDIO;

		if((current_gst_config->listener.flags & GST_FLAG_DEBUG) &&  (!current_gst_config->listener.debug_file_dump_location))
			current_gst_config->listener.debug_file_dump_location = DEFAULT_DUMP_LOCATION;
	}

	/*Apply default configurations for enabled talker streams*/
	for (i = 0; i < MAX_GSTREAMER_TALKERS; i++) {
		current_gst_config = &gstreamer_talker_pipelines[i].config;

		if (!(current_gst_config->type & GST_TYPE_TALKER))
			continue;

		if (!current_gst_config->device)
				current_gst_config->device = V4L2_LVDS_DEVICE_FILE;

		if (!current_gst_config->width)
			set_default_device_width(current_gst_config->device, &current_gst_config->width);

		if(!current_gst_config->height)
			set_default_device_height(current_gst_config->device, &current_gst_config->height);

		if (!(current_gst_config->type & GST_TYPE_VIDEO) && !(current_gst_config->type & GST_TYPE_AUDIO))
			current_gst_config->type |= GST_TYPE_VIDEO | GST_TYPE_AUDIO;

		if(!current_gst_config->talker.input_media_file_name)
			current_gst_config->talker.input_media_file_name = DEFAULT_MEDIA_FILE_NAME;
		else /* set the file src location to first file */
			current_gst_config->talker.file_src_location = current_gst_config->talker.input_media_files[current_gst_config->talker.input_media_file_index];
	}

	/*Apply default configurations for enabled multi talker streams*/
	for (i = 0; i < MAX_GST_MULTI_HANDLERS; i++) {
		current_gst_config = &gst_talker_multi_handler[i].gst_pipeline.config;

		if (!(current_gst_config->type & GST_TYPE_MULTI_TALKER))
			continue;

		if (!current_gst_config->device)
				current_gst_config->device = V4L2_LVDS_DEVICE_FILE;

		if (!current_gst_config->width)
			set_default_device_width(current_gst_config->device, &current_gst_config->width);

		if(!current_gst_config->height)
			set_default_device_height(current_gst_config->device, &current_gst_config->height);

		if (!(current_gst_config->type & GST_TYPE_VIDEO) && !(current_gst_config->type & GST_TYPE_AUDIO))
			current_gst_config->type |= GST_TYPE_VIDEO | GST_TYPE_AUDIO;

		if(!current_gst_config->talker.input_media_file_name)
			current_gst_config->talker.input_media_file_name = DEFAULT_MEDIA_FILE_NAME;
		else /* set the file src location to first file */
			current_gst_config->talker.file_src_location = current_gst_config->talker.input_media_files[current_gst_config->talker.input_media_file_index];
	}

	/*Apply default configurations for enabled multi listener streams*/
	for (i = 0; i < MAX_GST_MULTI_HANDLERS; i++) {
		current_gst_config = &gst_listener_multi_handler[i].gst_pipeline.config;

		if (!(current_gst_config->type & GST_TYPE_LISTENER))
			continue;

		if (!current_gst_config->device) {
			if (current_gst_config->listener.flags & GST_FLAG_BEV)
				current_gst_config->device = FB_LVDS_DEVICE_FILE;
			else
				current_gst_config->device =  V4L2_LVDS_DEVICE_FILE;
		}

		if (!current_gst_config->width)
			set_default_device_width(current_gst_config->device, &current_gst_config->width);

		if(!current_gst_config->height)
			set_default_device_height(current_gst_config->device, &current_gst_config->height);

		if((current_gst_config->listener.flags & GST_FLAG_DEBUG) &&  (!current_gst_config->listener.debug_file_dump_location))
			current_gst_config->listener.debug_file_dump_location = DEFAULT_DUMP_LOCATION;
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


	rc = clock_init(0);
	if (rc < 0)
		goto err_clock;


	rc = aar_log_init(NULL);
	if (rc < 0)
		goto err_log;

	rc = avbstream_init();
	if (rc < 0)
		goto err_stream;

	avb_handle = avbstream_get_avb_handle();

	rc = msrp_init(avb_handle);
	if (rc < 0)
		goto err_msrp;

	rc = clock_domain_init(avb_handle, &clock_domain_rx_fd);
	if (rc < 0)
		goto err_clock_domain_init;

	ctrl_poll[2].fd = clock_domain_rx_fd;
	ctrl_poll[2].events = POLLIN;

	clock_domain_get_status(AVB_CLOCK_DOMAIN_0);

	/*
	 * Setup static CRF and clock domain
	 */
	rc = static_crf_streams_init();
	if (rc < 0)
		goto err_static_crf_streams_init;

	rc = thread_init();
	if (rc < 0)
		goto err_thread;

	stats_fd = stats_thread_init(&stats_thread_slot);
	if (stats_fd < 0) {
		rc = -1;
		goto err_stats_thread;
	}

	rc = gst_bus_messages_timer_thread_init(&gstreamer_bus_monitor);
	if (rc < 0)
		goto err_gst_bus_timer_thread;

	/* Do not start monitoring until at least one pipeline is launched*/
	thread_slot_set_events(gstreamer_bus_monitor.thread_slot, 0, EPOLLIN);

	if (static_streams_init() < 0)
		goto err_static;

	if (avdecc_multi_talker_streams_init() < 0)
		goto err_avdecc_multi;

	/*
	 * listen to avdecc events to get stream parameters
	 */

	rc = avb_control_open(avb_handle, &ctrl_h, AVB_CTRL_AVDECC_MEDIA_STACK);
	if (rc != AVB_SUCCESS) {
		printf("avb_control_open(AVB_CTRL_AVDECC_MEDIA_STACK) failed: %s\n", avb_strerror(rc));
		printf("WARNING: will not listen for AVDECC notifications messages\n");
		rc = 0;

		ctrl_poll[0].fd = -1;
		ctrl_poll[0].events = POLLIN;
	} else {

		ctrl_rx_fd = avb_control_rx_fd(ctrl_h);
		ctrl_poll[0].fd = ctrl_rx_fd;
		ctrl_poll[0].events = POLLIN;
	}

	rc = avb_control_open(avb_handle, &controlled.handle, AVB_CTRL_AVDECC_CONTROLLED);
	if (rc != AVB_SUCCESS) {
		printf("avb_control_open() for AVB_CTRL_AVDECC_CONTROLLED channel failed: %s\n", avb_strerror(rc));
		printf("WARNING: will not listen for AVDECC CONTROLL messages\n");
		rc = 0;

		ctrl_poll[1].fd = -1;
		ctrl_poll[1].events = POLLIN;
	} else {
		printf("avb_control_open() for AVB_CTRL_AVDECC_CONTROLLED channel success : %p\n", controlled.handle);

		ctrl_poll[1].events = POLLIN;
		ctrl_poll[1].fd = -1;

		if (controlled_slot_init(&controlled_thread_slot, &controlled) < 0)
			goto exit;
	}

	/* If we have a filename for non-volatile binding params parse it and init the avdecc stack with its parameters. */
	if (binding_file_name) {
		if (avdecc_nvm_bindings_init(ctrl_h, binding_file_name) < 0) {
			printf("failed to parse binding file %s\n", binding_file_name);
			rc = -1;
			goto exit;
		}
	}

	while (1) {
		rc = poll(ctrl_poll, NUM_CONTROL_FDS, -1);
		if ((errno == EINTR) && signal_terminate) {
			printf("processing terminate signal\n");
			rc = -1;
			goto exit;
		}
		if (rc == -1) {
			printf("poll() failed errno(%d,%s)\n", errno, strerror(errno));
			break;
		}

		if (ctrl_poll[0].revents & POLLIN)
			handle_avdecc_event(ctrl_h, binding_file_name);

		if (ctrl_poll[2].revents & POLLIN)
			handle_clock_domain_event();
	}

exit:
	if (ctrl_h)
		avb_control_close(ctrl_h);

	controlled_slot_exit(controlled_thread_slot);

	if (controlled.handle)
		avb_control_close(controlled.handle);

err_avdecc_multi:
err_static:
	gst_bus_messages_timer_thread_exit(&gstreamer_bus_monitor);
err_gst_bus_timer_thread:
	stats_thread_exit(stats_fd, stats_thread_slot);

err_stats_thread:
	thread_exit();

	avdecc_streams_disconnect();

	static_streams_exit();
	avdecc_multi_talker_streams_exit();

err_thread:
	static_crf_streams_exit();

err_static_crf_streams_init:
	clock_domain_exit();

err_clock_domain_init:
	msrp_exit();

err_msrp:
	avbstream_exit();

err_stream:
	aar_log_exit();

err_log:
	clock_exit(0);

err_clock:
	gstreamer_config_exit();

err_sched:
	return rc;
}
