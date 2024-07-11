/*
 * Copyright 2018-2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file streaming.c
 \brief streaming public API for rtos
 \details
 \copyright Copyright 2018-2021, 2023-2024 NXP
*/

#include <string.h>

#include "api/control.h"
#include "api/streaming.h"

#include "common/avdecc.h"

#include "rtos/media_queue.h"

__init int streaming_init(struct genavb_handle *genavb)
{
	int rc;

	if (rtos_mqueue_init(&genavb->event_queue, AVTP_EVENT_QUEUE_LENGTH, sizeof(struct event), genavb->event_queue_buffer) < 0) {
		rc = -GENAVB_ERR_CTRL_INIT;
		goto err_event_queue;
	}

	/*
	* setup media stack to avtp ipc
	*/
	if (ipc_tx_init(&genavb->avtp_tx, IPC_MEDIA_STACK_AVTP) < 0) {
		rc = -GENAVB_ERR_CTRL_INIT;
		goto err_ipc_tx_init;
	}

	if (ipc_rx_init(&genavb->avtp_rx, IPC_AVTP_MEDIA_STACK, NULL, (unsigned long)&genavb->event_queue) < 0) {
		rc = -GENAVB_ERR_CTRL_INIT;
		goto err_ipc_rx_init;
	}

	if (ipc_tx_connect(&genavb->avtp_tx, &genavb->avtp_rx) < 0) {
		rc = -GENAVB_ERR_CTRL_INIT;
		goto err_ipc_tx_connect;
	}

	return GENAVB_SUCCESS;

err_ipc_tx_connect:
	ipc_rx_exit(&genavb->avtp_rx);

err_ipc_rx_init:
	ipc_tx_exit(&genavb->avtp_tx);

err_ipc_tx_init:
err_event_queue:
	return rc;
}

__exit void streaming_exit(struct genavb_handle *genavb)
{
	if (genavb->flags & AVTP_INITIALIZED) {
		ipc_rx_exit(&genavb->avtp_rx);
		ipc_tx_exit(&genavb->avtp_tx);
	}
}

int genavb_stream_create(struct genavb_handle *genavb, struct genavb_stream_handle **stream,
	struct genavb_stream_params const *params, unsigned int *batch_size, genavb_stream_create_flags_t flags)
{
	struct media_queue_api_params api_params;
	int rc;

	if (!genavb) {
		rc = -GENAVB_ERR_INVALID;
		goto err_genavb_handle_null;
	}

	/*
	* allocate new stream
	*/
	*stream = rtos_malloc(sizeof(struct genavb_stream_handle));
	if (!*stream) {
		rc = -GENAVB_ERR_NO_MEMORY;
		goto err_alloc;
	}

	memset(*stream, 0, sizeof(struct genavb_stream_handle));

	memcpy(&(*stream)->params, params, sizeof(struct genavb_stream_params));

	/*
	* connect avtp
	*/
	rc = connect_avtp(genavb, *stream);
	if (rc != GENAVB_SUCCESS) {
		goto err_connect;
	}

	if (params->subtype == AVTP_SUBTYPE_CRF) {
		(*stream)->mqueue.id = NULL;
		*batch_size = 0;
	} else {
		if (params->direction == AVTP_DIRECTION_TALKER) {
			/* Align batch size to AVTP thread batch size */

			unsigned int min_batch_size = (*stream)->batch * (*stream)->max_payload_size;

			if (*batch_size > min_batch_size)
				*batch_size = (*batch_size / min_batch_size) * min_batch_size;

			api_params.max_payload_size = (*stream)->max_payload_size;
		} else
			api_params.max_payload_size = avdecc_fmt_payload_size(&params->format, params->stream_class);

		api_params.port = params->port;
		//FIXME use logical port to clock
		api_params.clock_gptp = OS_CLOCK_GPTP_EP_0_0;
		copy_64(api_params.stream_id, &params->stream_id);
		api_params.batch_size = *batch_size;
		api_params.frame_stride = avdecc_fmt_sample_stride(&params->format);
		api_params.frame_size = avdecc_fmt_sample_size(&params->format);

		if (media_api_open(&(*stream)->mqueue, &api_params, params->direction == AVTP_DIRECTION_TALKER) < 0) {
			rc = -GENAVB_ERR_STREAM_API_OPEN;
			goto err_open;
		}

		*batch_size = api_params.batch_size;
		(*stream)->max_payload_size = api_params.max_payload_size;
	}

	(*stream)->genavb = genavb;

	return GENAVB_SUCCESS;

err_open:
	disconnect_avtp(genavb, params);

err_connect:
	rtos_free(*stream);

err_alloc:
err_genavb_handle_null:
	*stream = NULL;

	return rc;
}

int genavb_stream_receive(struct genavb_stream_handle const *handle, void *data,
	unsigned int data_len, struct genavb_event *event, unsigned int *event_len)
{
	struct genavb_iovec data_iov, event_iov;
	int rc;

	data_iov.iov_base = data;
	data_iov.iov_len = data_len;

	if (event && event_len) {
		event_iov.iov_base = event;
		event_iov.iov_len = *event_len;

		rc = media_api_read((struct media_queue *)&handle->mqueue, &data_iov, 1, &event_iov, 1, event_len);
	} else {
		rc = media_api_read((struct media_queue *)&handle->mqueue, &data_iov, 1, NULL, 0, NULL);
	}

	if (rc < 0)
		return (-GENAVB_ERR_STREAM_RX);

	return rc;
}


int genavb_stream_set_callback(struct genavb_stream_handle const *handle, int (*callback)(void *), void *data)
{
	if (!handle->mqueue.id)
		return -GENAVB_ERR_STREAM_INVALID;

	media_api_set_callback((struct media_queue *)&handle->mqueue, callback, data);

	return GENAVB_SUCCESS;
}

int genavb_stream_enable_callback(struct genavb_stream_handle const *handle)
{
	if (media_api_enable_callback((struct media_queue *)&handle->mqueue) < 0)
		return -GENAVB_ERR_STREAM_NO_CALLBACK;

	return GENAVB_SUCCESS;
}

int genavb_stream_send(struct genavb_stream_handle const *handle, void const *data,
	unsigned int data_len, struct genavb_event const *event, unsigned int event_len)
{
	struct genavb_iovec data_iov;
	int rc;

	if (unlikely(!handle)) {
		rc = -GENAVB_ERR_STREAM_INVALID;
		goto err;
	}

	if (unlikely((!event && event_len) || (!data && data_len))) {
		rc = -GENAVB_ERR_STREAM_TX;
		goto err;
	}

	data_iov.iov_base = (void *)data; /* We need to remove the const qualifier here, or we would need to define 2 separate iovec structures for tx/rx. */
	data_iov.iov_len = data_len;

	rc = media_api_write((struct media_queue *)&handle->mqueue, &data_iov, 1, event, event_len);
	if (rc < 0)
		return (-GENAVB_ERR_STREAM_TX);

err:
	return rc;
}

static int __avb_stream_destroy(struct genavb_stream_handle *handle)
{
	if (handle->mqueue.id)
		media_api_close(&handle->mqueue);

	disconnect_avtp(handle->genavb, &handle->params);

	rtos_free(handle);

	return GENAVB_SUCCESS;
}

int genavb_stream_destroy(struct genavb_stream_handle *handle)
{
	int rc;

	if (!handle) {
		rc = -GENAVB_ERR_STREAM_INVALID;
		goto err_handle_null;
	}

	rc =  __avb_stream_destroy(handle);

	return rc;

err_handle_null:
	return rc;
}
