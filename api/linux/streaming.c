/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file streaming.c
 \brief GenAVB public API for linux
 \details API definition for the GenAVB library
 \copyright Copyright 2014-2016 Freescale Semiconductor, Inc.
 \copyright Copyright 2016-2021, 2023 NXP
*/

#define _POSIX_C_SOURCE 200809L

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#include "modules/media.h"

#include "common/ipc.h"
#include "common/avdecc.h"
#include "common/avtp.h"
#include "common/cvf.h"

#include "api/streaming.h"
#include "api/control.h"

#include "genavb/streaming.h"

#define MEDIA_QUEUE_API_FILE "/dev/media_queue_api"
#define API_SYNC_POLL_TIMEOUT 1000

extern pthread_mutex_t avb_mutex;
extern struct genavb_handle *genavb_handle;

int __avb_stream_destroy(struct genavb_stream_handle *handle)
{
	disconnect_avtp(handle->genavb, &handle->params);

	if (handle->fd >= 0)
		close(handle->fd);

	list_del(&handle->list);

	free(handle);

	return GENAVB_SUCCESS;
}


int streaming_init(struct genavb_handle *genavb)
{
	int rc;

	/*
	* setup media stack to avtp ipc
	*/
	if (ipc_tx_init(&genavb->avtp_tx, IPC_MEDIA_STACK_AVTP) < 0) {
		rc = -GENAVB_ERR_CTRL_INIT;
		goto err_ipc_tx_init;
	}

	if (ipc_rx_init_no_notify(&genavb->avtp_rx, IPC_AVTP_MEDIA_STACK) < 0) {
		rc = -GENAVB_ERR_CTRL_INIT;
		goto err_ipc_rx_init;
	}

	if (ipc_tx_connect(&genavb->avtp_tx, &genavb->avtp_rx) < 0) {
		rc = -GENAVB_ERR_CTRL_INIT;
		goto err_ipc_tx_connect;
	}

	/* We don't wait for a reply HEARTBEAT message here: because of current limitations of the IPC implementations (one single
	 * reader per IPC), we cannot open the RX channel here, as we want to open it only for applications doing stream create/destroy.
	 */
	if (send_heartbeat(&genavb->avtp_tx, NULL, 0) < 0) {
		rc = -GENAVB_ERR_STACK_NOT_READY;
		goto err_tx_heartbeat;
	}

	return GENAVB_SUCCESS;

err_tx_heartbeat:
err_ipc_tx_connect:
	ipc_rx_exit(&genavb->avtp_rx);

err_ipc_rx_init:
	ipc_tx_exit(&genavb->avtp_tx);

err_ipc_tx_init:
	return rc;
}

void streaming_exit(struct genavb_handle *genavb)
{
	struct list_head *entry, *next;
	struct genavb_stream_handle *stream;

	for (entry = list_first(&genavb->streams); next = list_next(entry), entry != &genavb->streams; entry = next) {
		stream = container_of(entry, struct genavb_stream_handle, list);
		__avb_stream_destroy(stream);
	}

	if (genavb->flags & AVTP_INITIALIZED) {
		ipc_rx_exit(&genavb->avtp_rx);

		ipc_tx_exit(&genavb->avtp_tx);
	}
}

static unsigned int avtp_fmt_sample_stride(unsigned int subtype, const struct avdecc_format *format)
{
	unsigned int sample_stride = 0;

	switch (subtype) {
	case AVTP_SUBTYPE_NTSCF: /* not avdecc defined format */
		sample_stride = 1;
		break;
	default:
		sample_stride = avdecc_fmt_sample_stride(format);
		break;
	}

	return sample_stride;
}

static int avtp_subtype_mode_check(unsigned int flags, unsigned int subtype)
{
	int rc = 0;

	/* TSCF and NTSF only supported in Datagram mode */
	if ((flags & AVTP_DGRAM) && (subtype != AVTP_SUBTYPE_TSCF) && (subtype != AVTP_SUBTYPE_NTSCF))
		rc = -1;

	/* Datagram mode only supports TSCF and NTSCF */
	if (!(flags & AVTP_DGRAM)  && ((subtype == AVTP_SUBTYPE_TSCF) || (subtype == AVTP_SUBTYPE_NTSCF)))
		rc = -1;

	return rc;
}

int genavb_stream_create(struct genavb_handle *genavb, struct genavb_stream_handle **stream, struct genavb_stream_params const *params,
							unsigned int *batch_size, genavb_stream_create_flags_t flags)
{
	struct media_queue_api_params msg;
	int fd, rc;

	if (!genavb) {
		rc = -GENAVB_ERR_INVALID;
		goto err_genavb_handle_null;
	}

	pthread_mutex_lock(&avb_mutex);

	if (genavb_handle != genavb) {
		rc = -GENAVB_ERR_INVALID;
		goto err_genavb_handle_invalid;
	}

	/*
	* allocate new stream
	*/
	*stream = malloc(sizeof(struct genavb_stream_handle));
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
		fd = -1;
		*batch_size = 0;
	} else {
		/*
		* open device file and attach to the stream
		*/
		if (params->direction == AVTP_DIRECTION_LISTENER)
			fd = open(MEDIA_QUEUE_API_FILE, O_RDONLY | O_CLOEXEC);
		else
			fd = open(MEDIA_QUEUE_API_FILE, O_WRONLY | O_CLOEXEC);

		if (fd < 0) {
			rc = -GENAVB_ERR_STREAM_API_OPEN;
			goto err_open;
		}

		if (avtp_subtype_mode_check(flags, params->subtype) < 0) {
			rc = -GENAVB_ERR_INVALID_PARAMS;
			goto err_subtype_mode;
		}

		if (params->direction == AVTP_DIRECTION_TALKER) {
			if (flags & AVTP_DGRAM) {
				msg.max_payload_size = (*stream)->max_payload_size;
			} else {
				/* Align batch size to AVTP thread batch size */
				unsigned int min_batch_size = (*stream)->batch * (*stream)->max_payload_size;

				if (*batch_size > min_batch_size)
					*batch_size = (*batch_size / min_batch_size) * min_batch_size;

				msg.max_payload_size = (*stream)->max_payload_size;
			}
		} else {
			if (flags & AVTP_DGRAM)
				msg.max_payload_size = (*stream)->max_payload_size;
			else
				msg.max_payload_size = avdecc_fmt_payload_size(&params->format, params->stream_class);
		}

		/*
		* bind file descriptor and stream id
		*/
		msg.port = params->port;
		copy_64(msg.stream_id, &params->stream_id);
		msg.batch_size = *batch_size;
		msg.frame_stride = avtp_fmt_sample_stride(params->subtype, &params->format);
		msg.frame_size = avtp_fmt_sample_size(params->subtype, &params->format);
		msg.queue_size = 0;
		msg.flags = flags;

		if (ioctl(fd, MEDIA_IOC_API_BIND, &msg) < 0) {
			rc = -GENAVB_ERR_STREAM_BIND;
			goto err_ioctl;
		}

		*batch_size = msg.batch_size;
		(*stream)->max_payload_size = msg.max_payload_size;
		(*stream)->expect_new_frame = 1;

		if ((params->direction == AVTP_DIRECTION_TALKER) &&
			(avdecc_format_is_cvf_h264(&params->format)) &&
			(msg.max_payload_size < FU_HEADER_SIZE)) {

			rc = -GENAVB_ERR_STREAM_BIND;
			goto err_ioctl;
		}
	}

	(*stream)->fd = fd;


	(*stream)->genavb = genavb;

	list_add(&genavb->streams, &(*stream)->list);

	pthread_mutex_unlock(&avb_mutex);

	return GENAVB_SUCCESS;

err_ioctl:
err_subtype_mode:
	if (fd >= 0)
		close(fd);

err_open:
	disconnect_avtp(genavb, params);

err_connect:
	free(*stream);

err_alloc:
err_genavb_handle_invalid:
	pthread_mutex_unlock(&avb_mutex);

err_genavb_handle_null:
	*stream = NULL;

	return rc;
}


int genavb_stream_fd(struct genavb_stream_handle const *handle)
{
	if (handle->fd < 0)
		return -GENAVB_ERR_STREAM_INVALID;

	return handle->fd;
}


int genavb_stream_receive(struct genavb_stream_handle const *handle, void *data, unsigned int data_len,
				struct genavb_event *event, unsigned int *event_len)
{
	struct genavb_iovec data_iov, event_iov;

	data_iov.iov_base = data;
	data_iov.iov_len = data_len;

	if (event && event_len) {
		event_iov.iov_base = event;
		event_iov.iov_len = *event_len;

		return genavb_stream_receive_iov(handle, &data_iov, 1, &event_iov, 1, event_len);
	} else
		return genavb_stream_receive_iov(handle, &data_iov, 1, NULL, 0, NULL);
}


int genavb_stream_receive_iov(struct genavb_stream_handle const *handle, struct genavb_iovec const *data_iov, unsigned int data_iov_len,
				struct genavb_iovec const *event_iov, unsigned int event_iov_len, unsigned int *event_len)
{
	struct media_queue_rx msg;
	int rc = GENAVB_SUCCESS;

	if (event_iov && event_iov_len && !event_len)
		return (-GENAVB_ERR_STREAM_RX);

	msg.data_iov_len = data_iov_len;
	msg.data_iov = data_iov;

	msg.event_iov_len = event_iov_len;
	msg.event_iov = event_iov;

	if (ioctl(handle->fd, MEDIA_IOC_RX, &msg) < 0)
		rc = (-GENAVB_ERR_STREAM_RX);
	else
		rc = msg.data_read;

	if (event_len)
		*event_len = msg.event_read;

	return rc;
}


/** Checks for the start code in a H264 ByteStream and return its length
 *
 * Start Code Prefix can be 0x0.0x0.0x0.0x1 or 0x0.0x0.0x1
 *
 * \return lenght of the start code (3 or 4) or 0 if not found
 * \param buf pointer to NALUto parse
 * \param len length ofthe NALU to parse
 */
static unsigned int _h264_check_start_code(const u8 *buf, unsigned int len)
{
	unsigned int ret = 0;

	if (len < 4)
		return ret;

	if ((buf[0] == 0x0) && (buf[1] == 0x0)) {
		if(buf[2] == 0x1)
			ret = 3;
		else if ((buf[2] == 0x0) && buf[3] == 0x1)
			ret = 4;
		else
			ret = 0;
	}
	else
		ret = 0;

	return ret;

}

int genavb_stream_h264_send(struct genavb_stream_handle *handle, void *data, unsigned int data_len,
				struct genavb_event *event, unsigned int event_len)
{
	struct genavb_iovec data_iov[IOV_MAX + 1] = { {NULL, 0} };

	unsigned int start_code;
	unsigned int max_fu_payload_size;
	unsigned int iov_idx = 0;
	int rc;
	unsigned int first_fu = 0;
	u16 fu_header[IOV_MAX] = { 0xdead }; /*At max every two iovec will be an FU HEADER of 2 bytes*/
	unsigned int fu_hdr_idx = 0;
	unsigned int data_to_send;
	int real_offset[IOV_MAX + 1] = { 0 }; /*Used to keep track of nalu offset, useful for incomplete data send to know the exact value to return to app*/
	unsigned int offset_idx = 0;
	unsigned int total_sent = 0;
	unsigned int sent,left_data;
	int real_written,total_offset;
	unsigned int already_written = 0;
	unsigned int is_end_of_frame = 0;
	u8 *b_data = (u8 *) data;
	unsigned int idx,off_idx,current_iovec;

	/*Check the H264 start code prefix*/
	start_code = _h264_check_start_code((u8 *)data, data_len);

	max_fu_payload_size = handle->max_payload_size - FU_HEADER_SIZE;

	if(!data_len && !data) {
		/*Clear the previous state if an empty AVTP_FLUSH or AVTP_FRAME_END is performed*/
		if ((event && event_len) && ((event->event_mask & AVTP_FLUSH) || (event->event_mask & AVTP_FRAME_END))) {
			handle->partial_iovec = 0;
			handle->expect_new_frame = 1;
		}

		data_iov[iov_idx].iov_base = (void *)data;
		data_iov[iov_idx].iov_len = data_len;

		return genavb_stream_send_iov(handle, data_iov, 1, event, event_len);
	}

	if (start_code && handle->partial_iovec)
		return -GENAVB_ERR_STREAM_TX;

	if (start_code && !handle->expect_new_frame)
		return -GENAVB_ERR_STREAM_TX;

	if (!start_code && handle->expect_new_frame) {
		if (data_len < 4)
			return -GENAVB_ERR_STREAM_TX_NOT_ENOUGH_DATA;
		else
			return -GENAVB_ERR_STREAM_TX;
	}


	is_end_of_frame = event->event_mask & (AVTP_FRAME_END);

	left_data = data_len;

	if(start_code) {
		if (data_len - start_code <= handle->max_payload_size) {
			/* At this stage, the next call will introduce a start of frame:
			 * Either we successfully send all the bytes (the whole NALU) or we fail to write any of them*/
			handle->expect_new_frame = 1;


			/*This is the start of a new frame whith the rest yet to come
			 * Return a not enough data error to make the caller send more data (either the full NALU
			 * with the frame end flag or at least enough to exceed the max payload and make it a FU)*/
			if (!is_end_of_frame)
				return -GENAVB_ERR_STREAM_TX_NOT_ENOUGH_DATA;

			/*The whole NALU can be send in one packet*/
			data_to_send = data_len - start_code;
			data_iov[iov_idx].iov_base = (void *)(b_data + start_code); /*Get rid of the start_code*/
			data_iov[iov_idx].iov_len = data_to_send;
			rc = genavb_stream_send_iov(handle, data_iov, 1, event, event_len);

			if (rc > 0 && rc != data_to_send) {
			/* We should never have a partial send because either there is at least one media buffer (of size max_payload_size)
			 * to accept the data or there is none and we get 0 bytes written*/
				rc = -GENAVB_ERR_STREAM_TX;
			} else if (rc > 0) /*We should return the right written value, start_code included*/
				rc += start_code;

			return rc;
		}
		else
		{
			first_fu = 1;
			b_data += (start_code - 1);
			left_data -= (start_code - 1);
		}
	}

	/*Clear the end of frame flags to make sure we send it only with the last bytes later*/
	event->event_mask &= (~AVTP_FRAME_END);

	if ((handle->partial_iovec))
	{
		data_to_send = (left_data > handle->partial_iovec) ? handle->partial_iovec : left_data;

		data_iov[iov_idx].iov_base = (void *) b_data;
		data_iov[iov_idx].iov_len = data_to_send;
		iov_idx++;
		b_data += data_to_send;
		left_data -= data_to_send;
		total_sent += data_to_send;

		handle->partial_iovec -= data_to_send;
		real_offset[offset_idx++] = 0;

		/* The new frame is smaller or equal to partial_iovec
		 * jump to send_iov and send end of frame */
		if (!left_data)
			goto send_iov;

	}

	/* Construct the first FU header outside the loop and jump directly to the next one
	 * This header contain the NALU HDR (1 byte) then an empty byte */
	if(first_fu && left_data) {
		data_iov[iov_idx].iov_base = (void *) b_data;
		data_iov[iov_idx].iov_len = FU_HEADER_SIZE;
		total_sent += FU_HEADER_SIZE;

		/* 3 bytes of the start code (sent by user) were not sent to the driver, log them for further
		 * error handling */
		real_offset[offset_idx++] = 3;

		/*Set a marker for further identification :
		 * To differentiate from a single nal packet and a first
		 * FU-A packet */
		b_data[0] = CVF_H264_NALU_TYPE_FU_A;
		b_data += FU_HEADER_SIZE;
		left_data -= FU_HEADER_SIZE;
		first_fu = 0;
		iov_idx++;

		goto second_iovec_member;

	}

	/* Send iovec two by two : first one should contain the FU HEADER
	 * and second should contain FU Payload */
	while(left_data) {
		/* This is an empty (both bytes) FU header*/
		data_iov[iov_idx].iov_base = &(fu_header[fu_hdr_idx++]);
		data_iov[iov_idx].iov_len = FU_HEADER_SIZE;
		total_sent += FU_HEADER_SIZE;

		/* Two additional bytes (not sent by user) are sent to the driver */
		real_offset[offset_idx++] = -2;

		iov_idx++;

second_iovec_member:
		data_to_send = (left_data > max_fu_payload_size) ? max_fu_payload_size : left_data;
		data_iov[iov_idx].iov_base = b_data;
		data_iov[iov_idx].iov_len = data_to_send;
		b_data += data_to_send;
		left_data -= data_to_send;
		total_sent += data_to_send;
		real_offset[offset_idx++] = 0;
		iov_idx++;

		/*Send by batch of IOV_MAX*/
		if(iov_idx > (IOV_MAX - 2)) {

			if (is_end_of_frame) {
				/*Put the AVTP_FRAME_END flag if we are sending the last bytes of the frame*/
				if(!left_data)
					event->event_mask |= AVTP_FRAME_END;
			} else {
				/* Check and keep track of partial iovec and not and of frame to
				 * prevent an injection of unnecessary FU header */
				handle->partial_iovec = max_fu_payload_size - data_iov[iov_idx - 1].iov_len;
			}

			rc = genavb_stream_send_iov(handle, data_iov, iov_idx , event, event_len);

			if (rc < 0)
				return rc;
			else if (rc != total_sent)
				goto incomplete_send;

			/* Send the AVTP_SYNC only on first packet*/
			event->event_mask &= (~AVTP_SYNC);

			if (event->event_mask & AVTP_FRAME_END)
			        handle->expect_new_frame = 1; /*Successfully sent the last bytes with an end of frame*/
			else if (!left_data)
			        handle->expect_new_frame = 0; /*Successfully sent the last bytes without an end of frame*/

			iov_idx = 0;
			fu_hdr_idx = 0;
			offset_idx = 0;
			/*At this stage, all data were successfully written*/
			already_written = (data_len - left_data);
			total_sent = 0;
		}
	}

send_iov:

	if(iov_idx) {

		if (is_end_of_frame)
		{
			handle->partial_iovec = 0;
			/*Put the AVTP_FRAME_END flag to declare end of NALU */
			event->event_mask |= AVTP_FRAME_END;
		} else {
			/* Check and keep track of partial iovec and not and of frame to prevent
			 * an injection of unnecessary FU header */
			handle->partial_iovec = max_fu_payload_size - data_iov[iov_idx - 1].iov_len;
		}

		rc = genavb_stream_send_iov(handle, data_iov, iov_idx , event, event_len);

		if (rc < 0)
			return rc;
		else if (rc != total_sent)
			goto incomplete_send;

		/* All bytes were successfully sent, if this is an end of frame, expect a new frame on the next call*/
		if (event->event_mask & AVTP_FRAME_END)
		        handle->expect_new_frame = 1;
		else
		        handle->expect_new_frame = 0;

	}

	return data_len;

incomplete_send:

	/*If a non-zero size of bytes was sent, the next call should always send the rest of the NALU*/
	if (rc > 0 || already_written)
		handle->expect_new_frame = 0;

	/*Make sure we return the right number of written bytes */
	sent = 0;
	real_written = 0;
	idx = 0;
	off_idx = 0;
	total_offset = 0;
	current_iovec = 0;

	while (sent < rc) {
		total_offset += real_offset[off_idx++];
		current_iovec = data_iov[idx++].iov_len;
		sent += current_iovec;
		real_written = ((int) sent + total_offset);
	}

	if (sent > rc) {
		return -GENAVB_ERR_STREAM_TX;
	} else { /*sent == rc*/
		if (current_iovec && (current_iovec != max_fu_payload_size))
			 return -GENAVB_ERR_STREAM_TX;

		/*reset the handle->partial_iovec, it was not sent anyway*/
		handle->partial_iovec = 0;
	}

	real_written += already_written;
	return real_written;
}

int genavb_stream_send(struct genavb_stream_handle const *handle, void const *data, unsigned int data_len,
				struct genavb_event const *event, unsigned int event_len)
{
	struct genavb_iovec data_iov;

	data_iov.iov_base = (void *)data; /* We need to remove the const qualifier here, or we would need to define 2 separate iovec structures for tx/rx. */
	data_iov.iov_len = data_len;

	return genavb_stream_send_iov(handle, &data_iov, 1, event, event_len);
}


int genavb_stream_send_iov(struct genavb_stream_handle const *handle, struct genavb_iovec const *data_iov, unsigned int data_iov_len,
				struct genavb_event const *event, unsigned int event_len)
{
	struct media_queue_tx msg;
	int rc;

	if (!handle) {
		rc = -GENAVB_ERR_STREAM_INVALID;
		goto exit;
	}

	msg.data_iov_len = data_iov_len;
	msg.data_iov = data_iov;
	msg.event = event;
	msg.event_len = event_len;

	rc = ioctl(handle->fd, MEDIA_IOC_TX, &msg);
	if (rc < 0) {
		if (errno == EAGAIN || errno == EINTR)
			rc = 0;
		else
			rc = (-GENAVB_ERR_STREAM_TX);
	}

exit:
	return rc;
}


int genavb_stream_destroy(struct genavb_stream_handle *handle)
{
	struct list_head *entry, *next;
	struct genavb_stream_handle *stream;
	int rc;

	if (!handle) {
		rc = -GENAVB_ERR_STREAM_INVALID;
		goto err_handle_null;
	}

	pthread_mutex_lock(&avb_mutex);

	if (handle->genavb != genavb_handle) {
		rc = -GENAVB_ERR_STREAM_INVALID;
		goto err_handle_invalid;
	}

	rc = -GENAVB_ERR_STREAM_INVALID;
	for (entry = list_first(&genavb_handle->streams); next = list_next(entry), entry != &genavb_handle->streams; entry = next) {
		stream = container_of(entry, struct genavb_stream_handle, list);

		if (stream == handle) {
			rc =  __avb_stream_destroy(stream);
			break;
		}
	}

	pthread_mutex_unlock(&avb_mutex);

	return rc;

err_handle_invalid:
	pthread_mutex_unlock(&avb_mutex);

err_handle_null:
	return rc;
}
