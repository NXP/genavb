/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include <genavb/genavb.h>

#include "clock_domain.h"
#include "log.h"

static struct avb_control_handle *s_clk_handle = NULL;
static int audio_clk_sync[GENAVB_CLOCK_DOMAIN_MAX] = {0};
static unsigned int clk_domain_is_valid[GENAVB_CLOCK_DOMAIN_MAX] = {0};
static pthread_mutex_t clk_domain_mutex = PTHREAD_MUTEX_INITIALIZER;

static const char *clk_domain_name[] = {
	[GENAVB_CLOCK_DOMAIN_0] = "GENAVB_CLOCK_DOMAIN_0",
	[GENAVB_CLOCK_DOMAIN_1] = "GENAVB_CLOCK_DOMAIN_1",
	[GENAVB_CLOCK_DOMAIN_2] = "GENAVB_CLOCK_DOMAIN_2",
	[GENAVB_CLOCK_DOMAIN_3] = "GENAVB_CLOCK_DOMAIN_3",
	[GENAVB_CLOCK_DOMAIN_MAX] = "GENAVB_CLOCK_DOMAIN_MAX"
};

static const char *clk_source_type_name[] = {
	[GENAVB_CLOCK_SOURCE_TYPE_INTERNAL] = "GENAVB_CLOCK_SOURCE_TYPE_INTERNAL",
	[GENAVB_CLOCK_SOURCE_TYPE_INPUT_STREAM] = "GENAVB_CLOCK_SOURCE_TYPE_INPUT_STREAM"
};

static const char *clk_source_name[] = {
	[GENAVB_CLOCK_SOURCE_AUDIO_CLK] = "GENAVB_CLOCK_SOURCE_AUDIO_CLK",
	[GENAVB_CLOCK_SOURCE_PTP_CLK] = "GENAVB_CLOCK_SOURCE_PTP_CLK"
};

static const char *clk_domain_status_name[] = {
	[GENAVB_CLOCK_DOMAIN_STATUS_UNLOCKED] = "GENAVB_CLOCK_DOMAIN_STATUS_UNLOCKED",
	[GENAVB_CLOCK_DOMAIN_STATUS_LOCKED] = "GENAVB_CLOCK_DOMAIN_STATUS_LOCKED",
	[GENAVB_CLOCK_DOMAIN_STATUS_FREE_WHEELING] = "GENAVB_CLOCK_DOMAIN_STATUS_FREE_WHEELING",
	[GENAVB_CLOCK_DOMAIN_STATUS_HW_ERROR] = "GENAVB_CLOCK_DOMAIN_STATUS_HW_ERROR"
};

const char* get_genavb_clock_domain_t_name(genavb_clock_domain_t type)
{
	if ((type < GENAVB_CLOCK_DOMAIN_0) || (type > GENAVB_CLOCK_DOMAIN_MAX))
		return "<Unknown clock domain>";
	else
		return clk_domain_name[type];
}

const char* get_genavb_clock_source_type_t_name(genavb_clock_source_type_t type)
{
if ((type < GENAVB_CLOCK_SOURCE_TYPE_INTERNAL) || (type > GENAVB_CLOCK_SOURCE_TYPE_INPUT_STREAM))
		return "<Unknown clock source>";
	else
		return clk_source_type_name[type];
}

const char* get_genavb_clock_source_local_id_t_name(genavb_clock_source_local_id_t type)
{
	if ((type < GENAVB_CLOCK_SOURCE_AUDIO_CLK) || (type > GENAVB_CLOCK_SOURCE_PTP_CLK))
		return "<Unknown clock source local id>";
	else
		return clk_source_name[type];
}

const char* get_genavb_clock_domain_status_t_name(genavb_clock_domain_status_t type)
{
	if ((type < GENAVB_CLOCK_DOMAIN_STATUS_UNLOCKED) || (type > GENAVB_CLOCK_DOMAIN_STATUS_HW_ERROR))
		return "<Unknown clock domain status>";
	else
		return clk_domain_status_name[type];
}

int get_clk_domain_validity(avb_clock_domain_t clk_domain)
{
	unsigned int validity = 0;

	pthread_mutex_lock(&clk_domain_mutex);

	if (clk_domain < GENAVB_CLOCK_DOMAIN_MAX)
		validity = clk_domain_is_valid[clk_domain];
	else
		validity = -1;

	pthread_mutex_unlock(&clk_domain_mutex);

	return validity;
}

void set_clk_domain_validity(avb_clock_domain_t clk_domain, int validity)
{
	pthread_mutex_lock(&clk_domain_mutex);

	if (clk_domain < GENAVB_CLOCK_DOMAIN_MAX)
		clk_domain_is_valid[clk_domain] = validity;

	pthread_mutex_unlock(&clk_domain_mutex);
}

int handle_clock_domain_event(void)
{
	union avb_msg_clock_domain msg;
	unsigned int msg_type, msg_len;
	int rc;

	msg_len = sizeof(union avb_msg_clock_domain);
	rc = avb_control_receive(s_clk_handle, &msg_type, &msg, &msg_len);
	if (rc != AVB_SUCCESS)
		goto error_control_receive;

	switch (msg_type) {
	case AVB_MSG_CLOCK_DOMAIN_STATUS:

		if ((msg.status.status == GENAVB_CLOCK_DOMAIN_STATUS_LOCKED) || (msg.status.status == GENAVB_CLOCK_DOMAIN_STATUS_FREE_WHEELING)) {
			INF("AVB_MSG_CLOCK_DOMAIN_STATUS - domain: %s, source_type: %s, status: %s, Setting clk domain validity to TRUE", get_genavb_clock_domain_t_name(msg.status.domain), get_genavb_clock_source_type_t_name(msg.status.source_type), get_genavb_clock_domain_status_t_name(msg.status.status));
			set_clk_domain_validity(msg.status.domain, 1);
		}

		if (msg.status.status == GENAVB_CLOCK_DOMAIN_STATUS_UNLOCKED) {
			INF("AVB_MSG_CLOCK_DOMAIN_STATUS - domain: %s, source_type: %s, status: %s, Setting clk domain validity to FALSE", get_genavb_clock_domain_t_name(msg.status.domain), get_genavb_clock_source_type_t_name(msg.status.source_type), get_genavb_clock_domain_status_t_name(msg.status.status));
			set_clk_domain_validity(msg.status.domain, 0);
		}

		break;

	default:
		break;
	}

error_control_receive:
	return rc;
}

void clock_domain_get_status(avb_clock_domain_t domain)
{
	struct avb_msg_clock_domain_get_status get_status;
	unsigned int msg_len = sizeof(get_status);
	avb_msg_type_t msg_type = AVB_MSG_CLOCK_DOMAIN_GET_STATUS;
	int rc;

	get_status.domain = domain;

	rc = avb_control_send(s_clk_handle, msg_type, &get_status, msg_len);
	if (rc != AVB_SUCCESS)
		ERR("clock_domain_get_status (AVB_CTRL_CLOCK_DOMAIN) failed: %s\n", avb_strerror(rc));
}

int get_audio_clk_sync(avb_clock_domain_t clk_domain)
{
	if (clk_domain < GENAVB_CLOCK_DOMAIN_MAX)
		return audio_clk_sync[clk_domain];

	return -1;
}

void set_audio_clk_sync(avb_clock_domain_t clk_domain, int clk_sync)
{
	if (clk_domain < GENAVB_CLOCK_DOMAIN_MAX)
		audio_clk_sync[clk_domain] = clk_sync;
	else
		ERR("cannot set audio clock sync, clk_domain unknown");
}

static int clock_domain_set_source(struct avb_msg_clock_domain_set_source *set_source)
{
	struct avb_msg_clock_domain_response set_source_rsp;
	unsigned int msg_len = sizeof(*set_source);
	unsigned int msg_type = AVB_MSG_CLOCK_DOMAIN_SET_SOURCE;
	int rc;

	rc = avb_control_send_sync(s_clk_handle, &msg_type, set_source, msg_len, &set_source_rsp, &msg_len, 1000);
	if ((rc == AVB_SUCCESS) && (msg_type == AVB_MSG_CLOCK_DOMAIN_RESPONSE))
		rc = set_source_rsp.status;

	return rc;
}

int clock_domain_set_source_internal(avb_clock_domain_t domain,
							avb_clock_source_local_id_t local_id)
{
	struct avb_msg_clock_domain_set_source set_source;

	set_source.domain = domain;
	set_source.source_type = AVB_CLOCK_SOURCE_TYPE_INTERNAL;
	set_source.local_id = local_id;

	return clock_domain_set_source(&set_source);
}

int clock_domain_set_source_stream(avb_clock_domain_t domain,
						struct avb_stream_params *stream_params)
{
	struct avb_msg_clock_domain_set_source set_source;

	set_source.domain = domain;
	set_source.source_type = AVB_CLOCK_SOURCE_TYPE_INPUT_STREAM;
	memcpy(set_source.stream_id, stream_params->stream_id, 8);

	return clock_domain_set_source(&set_source);
}

int clock_domain_set_role(media_clock_role_t role, avb_clock_domain_t domain,
					struct avb_stream_params *stream_params)
{
	int rc = 0;

	set_audio_clk_sync(domain, 0);

	/* Master */
	if (role == MEDIA_CLOCK_MASTER) {
		/* If possible try to configure with internal HW source */
		if (clock_domain_set_source_internal(domain, AVB_CLOCK_SOURCE_AUDIO_CLK) != AVB_SUCCESS) {
			INF("cannot set clock source to internal audio clock");

			/* Fallback */
			rc = clock_domain_set_source_internal(domain, AVB_CLOCK_SOURCE_PTP_CLK);
			if (rc != AVB_SUCCESS) {
				ERR("cannot set clock source to PTP based clock, rc = %d", rc);
				goto exit;
			}
			INF("successfull fallback to PTP based clock");
		}
		else {
			set_audio_clk_sync(domain, 1);
			INF("clock source setup to internal audio clock");
		}
	}
	/* Slave */
	else {
		if (!stream_params) {
			ERR("slave role requires a stream argument");
			goto exit;
		}

		rc = clock_domain_set_source_stream(domain, stream_params);
		if (rc != AVB_SUCCESS) {
			ERR("clock_domain_set_source_stream error, rc = %d", rc);
			goto exit;
		}

		set_audio_clk_sync(domain, 1);
		INF("clock source setup to stream ID" STREAM_STR_FMT, STREAM_STR(stream_params->stream_id));
	}

exit:
	return rc;
}

int clock_domain_init(struct avb_handle *s_avb_handle, int *clk_fd)
{
	int rc;

	rc = avb_control_open(s_avb_handle, &s_clk_handle, AVB_CTRL_CLOCK_DOMAIN);
	if (rc != AVB_SUCCESS) {
		ERR("avb_control_open(AVB_CTRL_CLOCK_DOMAIN)  failed: %s\n", avb_strerror(rc));
		*clk_fd = -1;
		return -1;
	}

	*clk_fd = avb_control_rx_fd(s_clk_handle);
	DBG("clock domain control handle: %p", s_clk_handle);

	return 0;
}

int clock_domain_exit(void)
{
	avb_control_close(s_clk_handle);

	s_clk_handle = NULL;

	return 0;
}
