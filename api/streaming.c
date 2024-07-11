/*
* Copyright 2018, 2021, 2023-2024 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 \file streaming.c
 \brief GenAVB public API for linux
 \details API definition for the GenAVB library
 \copyright Copyright 2018, 2021, 2023-2024 NXP
*/

#include "os/string.h"
#include "os/stdlib.h"
#include "common/ipc.h"
#include "common/avtp.h"

#include "streaming.h"
#include "control.h"

unsigned int genavb_stream_presentation_offset(const struct genavb_stream_handle *handle)
{
	return stream_presentation_offset(handle->params.talker.max_transit_time, handle->params.talker.latency);
}

int streaming_init(struct genavb_handle *genavb);

#define AVTP_TIMEOUT 3000
int connect_avtp(struct genavb_handle *genavb, struct genavb_stream_handle *stream)
{
	struct genavb_stream_params *params = &stream->params;
	int rc;
	unsigned int msg_type, msg_len;

	if (!(genavb->flags & AVTP_INITIALIZED)) {
		rc = streaming_init(genavb);
		if (rc < 0)
			goto exit;

		genavb->flags |= AVTP_INITIALIZED;
	}

	/*
	* Send connect to AVTP
	*/
	rc = avb_ipc_send(&genavb->avtp_tx, IPC_AVTP_CONNECT, params, sizeof(*params), 0);
	if (rc != GENAVB_SUCCESS)
		goto exit;

	if (params->direction == AVTP_DIRECTION_LISTENER) {
		struct ipc_avtp_listener_connect_response response;

		msg_len = sizeof(response);
		rc = avb_ipc_receive_sync(&genavb->avtp_rx, &msg_type, &response, &msg_len, AVTP_TIMEOUT);
		if (rc != GENAVB_SUCCESS)
			goto exit;

		if (msg_type != IPC_AVTP_LISTENER_CONNECT_RESPONSE) {
			rc = -GENAVB_ERR_CTRL_RX;
			goto exit;
		}

		if (os_memcmp(&response.stream_id, params->stream_id, 8)) {
			rc = -GENAVB_ERR_CTRL_RX;
			goto exit;
		}

		if (response.status != GENAVB_SUCCESS){
			rc = -response.status;
			goto exit;
		}

		rc = response.status;

	} else {
		struct ipc_avtp_talker_connect_response response;

		msg_len = sizeof(response);
		rc = avb_ipc_receive_sync(&genavb->avtp_rx, &msg_type, &response, &msg_len, AVTP_TIMEOUT);
		if (rc != GENAVB_SUCCESS)
			goto exit;

		if (msg_type != IPC_AVTP_TALKER_CONNECT_RESPONSE) {
			rc = -GENAVB_ERR_CTRL_RX;
			goto exit;
		}

		if (os_memcmp(&response.stream_id, params->stream_id, 8)) {
			rc = -GENAVB_ERR_CTRL_RX;
			goto exit;
		}

		if (response.status != GENAVB_SUCCESS){
			rc = -response.status;
			goto exit;
		}

		rc = response.status;

		params->talker.latency = response.latency;
		stream->batch = response.batch;
		stream->max_payload_size = response.max_payload_size;
	}

exit:
	return rc;
}

int disconnect_avtp(struct genavb_handle *genavb, struct genavb_stream_params const *params)
{
	struct ipc_avtp_disconnect avtp_disconnect;
	int rc;
	struct ipc_avtp_disconnect_response avtp_disconnect_response;
	unsigned int msg_type, msg_len;

	if (!(genavb->flags & AVTP_INITIALIZED)) {
		rc = -GENAVB_ERR_CTRL_TX;
		goto exit;
	}

	/*
	* Send talker disconnect to AVTP
	*/
	os_memcpy(&avtp_disconnect.stream_id, params->stream_id, 8);
	avtp_disconnect.stream_class = params->stream_class;
	avtp_disconnect.port = params->port;
	avtp_disconnect.direction = params->direction;

	rc = avb_ipc_send(&genavb->avtp_tx, IPC_AVTP_DISCONNECT, &avtp_disconnect, sizeof(struct ipc_avtp_disconnect), 0);
	if (rc != GENAVB_SUCCESS)
		goto exit;

	msg_len = sizeof(struct ipc_avtp_disconnect_response);
	rc = avb_ipc_receive_sync(&genavb->avtp_rx, &msg_type, &avtp_disconnect_response, &msg_len, AVTP_TIMEOUT);
	if (rc != GENAVB_SUCCESS)
		goto exit;

	if (msg_type != IPC_AVTP_DISCONNECT_RESPONSE) {
		rc = -GENAVB_ERR_CTRL_RX;
		goto exit;
	}

	if (os_memcmp(&avtp_disconnect_response.stream_id, params->stream_id, 8)) {
		rc = -GENAVB_ERR_CTRL_RX;
		goto exit;
	}

	if (avtp_disconnect_response.status != GENAVB_SUCCESS){
		rc = -avtp_disconnect_response.status;
		goto exit;
	}

	rc = avtp_disconnect_response.status;

exit:
	return rc;
}
