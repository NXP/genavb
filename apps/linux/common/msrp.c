/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <sys/ioctl.h>
#include <arpa/inet.h>

#include "log.h"
#include <genavb/genavb.h>
#include <genavb/avdecc.h>
#include <genavb/ether.h>


#define MAX_AVB_LISTENER_REGISTRATIONS	32
#define MAX_AVB_TALKER_REGISTRATIONS	32

static struct avb_control_handle *s_msrp_handle = NULL;

struct msrp_registration_t  {
	avb_u8 stream_id[8];
	unsigned int ref_count;
};

static struct msrp_registration_t msrp_listener_registrations[MAX_AVB_LISTENER_REGISTRATIONS];
static struct msrp_registration_t msrp_talker_registrations[MAX_AVB_TALKER_REGISTRATIONS];

int msrp_init(struct avb_handle *s_avb_handle)
{
	int avb_result;
	int rc;

	avb_result = avb_control_open(s_avb_handle, &s_msrp_handle, AVB_CTRL_MSRP);
	if (avb_result != AVB_SUCCESS) {
		ERR("avb_control_open() failed: %s", avb_strerror(avb_result));
		rc = -1;
		goto err_control_open;
	}

	memset(msrp_listener_registrations, 0, sizeof(msrp_listener_registrations));
	memset(msrp_talker_registrations, 0, sizeof(msrp_talker_registrations));

	DBG("msrp control handle: %p", s_msrp_handle);

	return 0;

err_control_open:
	return rc;
}

int msrp_exit(void)
{
	avb_control_close(s_msrp_handle);

	s_msrp_handle = NULL;

	return 0;
}

static int _msrp_talker_deregister(const struct avb_stream_params *stream_params)
{
	struct avb_msg_talker_deregister talker_deregister;
	struct avb_msg_talker_response talker_response;
	unsigned int msg_type, msg_len;
	int rc;

	DBG("stream_params: %p", stream_params);
	talker_deregister.port = stream_params->port;
	memcpy(talker_deregister.stream_id, stream_params->stream_id, 8);

	msg_type = AVB_MSG_TALKER_DEREGISTER;
	msg_len = sizeof(talker_response);
	rc = avb_control_send_sync(s_msrp_handle, &msg_type, &talker_deregister, sizeof(talker_deregister), &talker_response, &msg_len, 1000);
	if ((rc != AVB_SUCCESS) || (msg_type != AVB_MSG_TALKER_RESPONSE) || (talker_response.status != AVB_SUCCESS)) {
		ERR(STREAM_STR_FMT " failed: %s", STREAM_STR(stream_params->stream_id), avb_strerror(rc));

		return -1;
	}

	return 0;
}

static int _msrp_talker_register(const struct avb_stream_params *stream_params)
{
	struct avb_msg_talker_register talker_register;
	struct avb_msg_talker_response talker_response;
	unsigned int max_frame_size, max_interval_frames;
	unsigned int msg_type, msg_len;
	int rc;

	DBG("stream_params: %p", stream_params);
	talker_register.port = stream_params->port;
	memcpy(talker_register.stream_id, stream_params->stream_id, 8);

	talker_register.params.stream_class = stream_params->stream_class;
	memcpy(talker_register.params.destination_address, stream_params->dst_mac, 6);
	talker_register.params.vlan_id = VLAN_VID_DEFAULT;

	if (stream_params->flags & GENAVB_STREAM_FLAGS_CUSTOM_TSPEC)  {
		max_frame_size = stream_params->talker.max_frame_size;
		max_interval_frames = stream_params->talker.max_interval_frames;
	} else
		avdecc_fmt_tspec(&stream_params->format, stream_params->stream_class, &max_frame_size, &max_interval_frames);

	talker_register.params.max_frame_size = max_frame_size;
	talker_register.params.max_interval_frames = max_interval_frames;
	talker_register.params.accumulated_latency = 0;
	talker_register.params.rank = NORMAL;

	msg_type = AVB_MSG_TALKER_REGISTER;
	msg_len = sizeof(talker_response);
	rc = avb_control_send_sync(s_msrp_handle, &msg_type, &talker_register, sizeof(talker_register), &talker_response, &msg_len, 1000);
	if ((rc != AVB_SUCCESS) || (msg_type != AVB_MSG_TALKER_RESPONSE) || (talker_response.status != AVB_SUCCESS)) {
		ERR(STREAM_STR_FMT " failed: %s", STREAM_STR(stream_params->stream_id), avb_strerror(rc));

		return -1;
	}

	return 0;
}

static int _msrp_listener_deregister(const struct avb_stream_params *stream_params)
{
	struct avb_msg_listener_deregister listener_deregister;
	struct avb_msg_listener_response listener_response;
	unsigned int msg_type, msg_len;
	int rc;

	DBG("stream_params: %p", stream_params);
	listener_deregister.port = stream_params->port;
	memcpy(listener_deregister.stream_id, stream_params->stream_id, 8);

	msg_type = AVB_MSG_LISTENER_DEREGISTER;
	msg_len = sizeof(listener_response);
	rc = avb_control_send_sync(s_msrp_handle, &msg_type, &listener_deregister, sizeof(listener_deregister), &listener_response, &msg_len, 1000);
	if ((rc != AVB_SUCCESS) || (msg_type != AVB_MSG_LISTENER_RESPONSE) || (listener_response.status != AVB_SUCCESS)) {
		ERR(STREAM_STR_FMT " failed: %s", STREAM_STR(stream_params->stream_id), avb_strerror(rc));

		return -1;
	}

	return 0;
}

static int _msrp_listener_register(const struct avb_stream_params *stream_params)
{
	struct avb_msg_listener_register listener_register;
	struct avb_msg_listener_response listener_response;
	unsigned int msg_type, msg_len;
	int rc;

	DBG("stream_params: %p", stream_params);
	listener_register.port = stream_params->port;
	memcpy(listener_register.stream_id, stream_params->stream_id, 8);

	msg_type = AVB_MSG_LISTENER_REGISTER;
	msg_len = sizeof(listener_response);
	rc = avb_control_send_sync(s_msrp_handle, &msg_type, &listener_register, sizeof(listener_register), &listener_response, &msg_len, 1000);
	if ((rc != AVB_SUCCESS) || (msg_type != AVB_MSG_LISTENER_RESPONSE) || (listener_response.status != AVB_SUCCESS)) {
		ERR(STREAM_STR_FMT " failed: %s", STREAM_STR(stream_params->stream_id), avb_strerror(rc));

		return -1;
	}

	return 0;
}


static struct msrp_registration_t *msrp_find_listener_registration(unsigned char *stream_id)
{
	struct msrp_registration_t *registration;
	int i;

	for (i = 0; i < MAX_AVB_LISTENER_REGISTRATIONS; ++i) {
		registration = &msrp_listener_registrations[i];

		if (memcmp(registration->stream_id, stream_id, sizeof(registration->stream_id)) == 0) {
			DBG("found matching registration %p at index %d for stream " STREAM_STR_FMT, registration, i, STREAM_STR(stream_id));
			return registration;
		}
	}

	// No existing registration, find a free one
	for (i = 0; i < MAX_AVB_LISTENER_REGISTRATIONS; ++i) {
		registration = &msrp_listener_registrations[i];

		if (!registration->ref_count){
			memcpy(registration->stream_id, stream_id, sizeof(registration->stream_id));
			DBG("found free registration %p at index %d", registration, i);
			return registration;
		}
	}

	return NULL;
}

int msrp_listener_register(struct avb_stream_params *stream_params)
{
	struct msrp_registration_t *registration;
	int result = 0;

	DBG("stream_id: " STREAM_STR_FMT, STREAM_STR(stream_params->stream_id));

	registration = msrp_find_listener_registration(stream_params->stream_id);

	if (!registration) {
		ERR("Couldn't find free registration for stream " STREAM_STR_FMT, STREAM_STR(stream_params->stream_id));
		result = -1;
		goto err;
	}

	if (!registration->ref_count)
		result = _msrp_listener_register(stream_params);

	if (result == 0)
		registration->ref_count++;

err:
	return result;
}

void msrp_listener_deregister(struct avb_stream_params *stream_params)
{
	struct msrp_registration_t *registration;

	DBG("stream_id: " STREAM_STR_FMT, STREAM_STR(stream_params->stream_id));

	registration = msrp_find_listener_registration(stream_params->stream_id);

	if (!registration) {
		ERR("Couldn't find registration for stream " STREAM_STR_FMT, STREAM_STR(stream_params->stream_id));
		return;
	}

	if (!registration->ref_count) {
		ERR("Tried to deregister an unregistered MSRP listener " STREAM_STR_FMT, STREAM_STR(stream_params->stream_id));
		return;
	}

	registration->ref_count--;

	if (!registration->ref_count)
		_msrp_listener_deregister(stream_params);
}

static struct msrp_registration_t *msrp_find_talker_registration(unsigned char *stream_id)
{
	struct msrp_registration_t *registration;
	int i;

	for (i = 0; i < MAX_AVB_TALKER_REGISTRATIONS; ++i) {
		registration = &msrp_talker_registrations[i];

		if (memcmp(registration->stream_id, stream_id, sizeof(registration->stream_id)) == 0) {
			DBG("found matching registration %p at index %d for stream " STREAM_STR_FMT, registration, i, STREAM_STR(stream_id));
			return registration;
		}
	}

	// No existing registration, find a free one
	for (i = 0; i < MAX_AVB_TALKER_REGISTRATIONS; ++i) {
		registration = &msrp_talker_registrations[i];

		if (!registration->ref_count){
			memcpy(registration->stream_id, stream_id, sizeof(registration->stream_id));
			DBG("found free registration %p at index %d", registration, i);
			return registration;
		}
	}

	return NULL;
}


int msrp_talker_register(struct avb_stream_params *stream_params)
{
	struct msrp_registration_t *registration;
	int result = 0;

	DBG("stream_id: " STREAM_STR_FMT, STREAM_STR(stream_params->stream_id));

	registration = msrp_find_talker_registration(stream_params->stream_id);

	if (!registration) {
		ERR("Couldn't find free registration for stream " STREAM_STR_FMT, STREAM_STR(stream_params->stream_id));
		result = -1;
		goto err;
	}

	if (!registration->ref_count)
		result = _msrp_talker_register(stream_params);

	if (result == 0)
		registration->ref_count++;

err:
	return result;
}

void msrp_talker_deregister(struct avb_stream_params *stream_params)
{
	struct msrp_registration_t *registration;

	DBG("stream_id: " STREAM_STR_FMT, STREAM_STR(stream_params->stream_id));
	registration = msrp_find_talker_registration(stream_params->stream_id);

	if (!registration) {
		ERR("Couldn't find registration for stream " STREAM_STR_FMT, STREAM_STR(stream_params->stream_id));
		return;
	}

	if (!registration->ref_count) {
		ERR("Tried to deregister an unregistered MSRP listener " STREAM_STR_FMT, STREAM_STR(stream_params->stream_id));
		return;
	}

	registration->ref_count--;

	if (!registration->ref_count)
		_msrp_talker_deregister(stream_params);
}
