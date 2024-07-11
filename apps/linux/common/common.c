/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2022, 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <inttypes.h>

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/timerfd.h>

#include <genavb/aecp.h>
#include "common.h"

#define AVB_LISTENER_TS_LOG

#define NUM_CONTROL_FDS	3

void print_stream_id(avb_u8 *id)
{
	printf("stream ID: %02x%02x%02x%02x%02x%02x%02x%02x\n", id[0], id[1], id[2], id[3], id[4], id[5], id[6], id[7]);
}

#if defined (AVB_LISTENER_TS_LOG)
unsigned char listener_first_buffer = 0;
#endif

int listener_file_handler(struct avb_stream_handle *stream_h, int fd, unsigned int batch_size, struct stats *s)
{
	unsigned int event_len = EVENT_BUF_SZ;
	unsigned char data_buf[DATA_BUF_SZ];
	struct avb_event event[EVENT_BUF_SZ];
	int nbytes;
	int rc = 0;
	uint64_t poll_time = 0;
	uint64_t now = 0;

	/*
	* read data from stack...
	*/
	nbytes = avb_stream_receive(stream_h, data_buf, batch_size, event, &event_len);
	if (nbytes <= 0) {
		if (nbytes < 0)
			printf("avb_stream_receive() failed: %s\n", avb_strerror(nbytes));
		else
			printf("avb_stream_receive() incomplete\n");

		rc = nbytes;
		goto exit;
	}

#if defined (AVB_LISTENER_TS_LOG)
	if (listener_first_buffer == 0) {
		uint64_t now;

		gettime_us(&now);

		printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		printf("!!!AVB listener: first buffer received @%" PRIu64 " (us)\n", now);
		printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		listener_first_buffer = 1;
	}
#endif

	if (event_len != 0) {
		if (event[0].event_mask & AVTP_MEDIA_CLOCK_RESTART)
			printf ("AVTP media clock restarted\n");

		if (event[0].event_mask & AVTP_PACKET_LOST)
			printf ("AVTP packet lost\n");
	}
	/*
	* ...and write to local file
	*/
	if (s)
		gettime_us(&poll_time);

	rc = write(fd, data_buf, nbytes);

	if (s) {
		if (poll_time) {
			gettime_us(&now);
			stats_update(s, (now - poll_time));
		}
	}

	if (rc < nbytes) {
		if (rc < 0)
			printf("write() failed: %s\n", strerror(errno));
		else
			printf("write() incomplete\n");

		goto exit;
	}

exit:
	return rc;
}

void listener_stream_flush(struct avb_stream_handle *stream_h)
{
	unsigned int event_len = EVENT_BUF_SZ;
	unsigned char data_buf[DATA_BUF_SZ];
	struct avb_event event[EVENT_BUF_SZ];
	int nbytes;

	while ((nbytes = avb_stream_receive(stream_h, data_buf, DATA_BUF_SZ, event, &event_len)) > 0)
		printf("flushed %d bytes\n", nbytes);

#if defined (AVB_LISTENER_TS_LOG)
	listener_first_buffer = 0;
#endif
}

void talker_stream_flush(struct avb_stream_handle *stream_h, struct avb_stream_params *params)
{
	struct avb_event event;
	int rc;

	event.event_mask = AVTP_FLUSH;
	if ( !params || (params->format.u.s.subtype_u.cvf.subtype != CVF_FORMAT_SUBTYPE_H264) )
		rc = avb_stream_send(stream_h, NULL, 0, &event, 1);
	else
		rc = avb_stream_h264_send(stream_h, NULL, 0, &event, 1);

	if (rc < 0)
			printf("%s() avb_stream_send() failed: %s\n", __func__, avb_strerror(rc));
}

int file_read(int fd, unsigned char *buf, unsigned int len, unsigned int timeout)
{
	fd_set read_set;
	struct timeval read_timeout;
	int rc;

retry:
	rc = read(fd, buf, len);
	if (rc <= 0) {
		if (rc == 0) {
			/* end of file */
			//printf("%s: end of file\n", __func__);
			return 0;
		}

		if (errno == EINTR)
			goto retry;

		if (errno == EAGAIN) {
			if (!timeout)
				return 0;

			read_timeout.tv_sec = 0;
			read_timeout.tv_usec = timeout;

		wait:
			FD_ZERO(&read_set);
			FD_SET(fd, &read_set);

			rc = select(fd + 1, &read_set, NULL, NULL, &read_timeout);
			if (rc > 0)
				goto retry;
			else if (!rc) {
				/* end of file */
				//printf("%s: timeout/end of file\n", __func__);

				return 0;
			}

			if (errno == EINTR)
				goto wait;

			printf("%s(), select() failed: %s\n", __func__, strerror(errno));

			return -1;
		}

		printf("%s(), read() failed: %s\n", __func__, strerror(errno));

		return -1;
	}

	return rc;
}

int talker_file_handler(struct avb_stream_handle *stream_h, int fd, unsigned int batch_size, unsigned int flags)
{
	unsigned char data_buf[DATA_BUF_SZ];
	int nbytes;
	int rc = 0;
	int i;

	/*
	* read data from local file...
	*/
	nbytes = read(fd, data_buf, batch_size);

	/* no more data to read, we are done*/
	if (nbytes <= 0) {
		if (nbytes < 0) {
			printf("read() failed: %s\n", strerror(errno));
			rc = nbytes;
		} else {
			lseek(fd, 0, SEEK_SET);
			rc = 0;
		}

		goto exit;
	}

	/*
	 * AM824 data format requires a label in the unused part
	 * of the 32 bits (24 bits of data).
	 */
	if (flags & MEDIA_FLAGS_SET_AM824_LABEL_RAW) {
		if (nbytes & 0x3) {
			printf("nbytes: %d is not sample round\n", nbytes);
			rc = -1;
			goto exit;
		}

		for (i = 0; i < nbytes; i+= 4)
			*(avb_u8 *)(data_buf + i) = AM824_LABEL_RAW;
	}

	/*
	* ...and write to avb stack
	*/
	rc = avb_stream_send(stream_h, data_buf, nbytes, NULL, 0);
	if (rc != nbytes) {
		if (rc < 0)
			printf("avb_stream_send() failed: %s\n", avb_strerror(rc));
		else
			printf("avb_stream_send() incomplete\n");

		goto exit;
	}

exit:
	return rc;
}

void media_stream_poll_set(const struct media_stream *stream, int enable)
{
	struct media_thread *thread = stream->thread;

	/* FIXME this needs to be different for gui/QT applications */

	if (enable)
		thread->poll_fd[stream->index + NUM_CONTROL_FDS].fd = stream->fd;
	else
		thread->poll_fd[stream->index + NUM_CONTROL_FDS].fd = -1;

	if (stream->params.direction == AVTP_DIRECTION_LISTENER)
		thread->poll_fd[stream->index + NUM_CONTROL_FDS].events = POLLIN;
	else
		thread->poll_fd[stream->index + NUM_CONTROL_FDS].events = POLLOUT;

//	printf("%s: stream(%d) %s fd(%d)\n", __func__, stream->index, enable ? "enabled" : "disabled", thread->poll_fd[stream->index + 2].fd);
}

static void stream_connect_handler(const struct media_control *ctrl, const struct avb_stream_params *stream_params, unsigned int avdecc_stream_index)
{
	struct media_thread *thread = ctrl->data;
	struct media_stream *stream;
	int rc;

	if (avdecc_stream_index >= thread->max_supported_streams) {
		printf("%s stream_index(%d) out of bounds(%d)\n", __func__, avdecc_stream_index, thread->max_supported_streams);
		goto err_stream_index;
	}

	if (avdecc_stream_index < 2) /*This is for stream indexes 0 : 61883_6 audio and 1: 61883_4 video*/
		stream = &thread->stream[avdecc_stream_index];
	else /*This is for stream index 3 : H264 video*/
		stream = &thread->stream[0];

	if (stream->state & MEDIA_STREAM_STATE_CONNECTED)
		return;

	memcpy(&stream->params, stream_params, sizeof(struct avb_stream_params));

	if (ctrl->config_handler)
		ctrl->config_handler(stream);

	rc = avb_stream_create(thread->avb_h, &stream->handle, &stream->params, &stream->batch_size, stream->flags);
	if (rc != AVB_SUCCESS) {
		printf("avb_stream_create() failed: %s\n", avb_strerror(rc));
		rc = -1;
		goto err_stream_create;
	}

	/*
	* retrieve the file descriptor associated to the stream
	*/

	stream->fd = avb_stream_fd(stream->handle);
	if (stream->fd < 0) {
		printf("avb_stream_fd() failed: %s\n", avb_strerror(stream->fd));
		rc = -1;
		goto err_stream_fd;
	}

	media_stream_poll_set(stream, 1);

	if (ctrl->connect_handler) {
		if (ctrl->connect_handler(stream) < 0) {
			rc = -1;
			goto err_connect;
		}
	}

	stream->state |= MEDIA_STREAM_STATE_CONNECTED;

	printf("stream(%d) connect success\n", stream->index);

	printf("Configured AVB batch size (bytes): %d\n", stream->batch_size);

	return;

err_connect:
err_stream_fd:
	avb_stream_destroy(stream->handle);

err_stream_create:
err_stream_index:

	printf("stream(%d) connect failed\n", avdecc_stream_index);

	return;
}

static void stream_disconnect_handler(struct media_control *ctrl, unsigned int avdecc_stream_index)
{
	struct media_thread *thread = ctrl->data;
	struct media_stream *stream;

	if (avdecc_stream_index >= thread->max_supported_streams) {
		printf("%s stream_index(%d) out of bounds(%d)\n", __func__, avdecc_stream_index, thread->max_supported_streams);
		goto err_stream_index;
	}

	if (avdecc_stream_index < 2) /*This is for stream indexes 0 : 61883_6 audio and 1: 61883_4 video*/
		stream = &thread->stream[avdecc_stream_index];
	else /*This is for stream index 3 : H264 video*/
		stream = &thread->stream[0];

	if (!(stream->state & MEDIA_STREAM_STATE_CONNECTED))
		return;

	stream->state &= ~MEDIA_STREAM_STATE_CONNECTED;

	media_stream_poll_set(stream, 0);

	if (ctrl->disconnect_handler)
		ctrl->disconnect_handler(stream);

	avb_stream_destroy(stream->handle);

err_stream_index:
	return;
}

int media_control_handler(struct media_control *ctrl)
{
	union avb_media_stack_msg msg;
	unsigned int msg_type, msg_len;
	int rc;

	msg_len = sizeof(union avb_media_stack_msg);
	rc = avb_control_receive(ctrl->handle, &msg_type, &msg, &msg_len);
	if (rc != AVB_SUCCESS) {
		printf("avb_control_receive() failed, %s\n", avb_strerror(rc));
		rc = -1;
		goto err_rx;
	}

	switch (msg_type) {
	case AVB_MSG_MEDIA_STACK_CONNECT:
		printf("event: AVB_MSG_MEDIA_STACK_CONNECT\n");

		stream_connect_handler(ctrl, &msg.media_stack_connect.stream_params, msg.media_stack_connect.stream_index);

		break;

	case AVB_MSG_MEDIA_STACK_DISCONNECT:
		printf("event: AVB_MSG_MEDIA_STACK_DISCONNECT\n");

		stream_disconnect_handler(ctrl, msg.media_stack_disconnect.stream_index);

		break;

	default:
		printf("event: UNKNOWN\n");
		break;
	}

	return 0;

err_rx:
	return rc;
}

/** Generic AECP AEM SET_CONTROL command handler.
 *
 * Simply calls the application specific aem_set_control_handler.
 * \return		AECP_AEM return code, according to IEEE 1722.1-2013 table 7.126 (AECP_AEM_SUCCESS in case of success).
 * \param controlled	avdecc_controlled context that received the command.
 * \param pdu		Pointer to received AECP AEM PDU.
 */
int aem_set_control_handler(struct avdecc_controlled *controlled, struct aecp_aem_pdu *pdu)
{
	struct aecp_aem_set_get_control_pdu *set_control_cmd  = (struct aecp_aem_set_get_control_pdu *)(pdu + 1);
	avb_u16 ctrl_index = ntohs(set_control_cmd->descriptor_index);
	void *ctrl_value = set_control_cmd + 1;
	int rc = AECP_AEM_NOT_IMPLEMENTED;

	if (controlled->aem_set_control_handler)
		rc = controlled->aem_set_control_handler(controlled, ctrl_index, ctrl_value);

	return rc;
}

/** Generic AECP AEM REMOVE_AUDIO_MAPPINGS command handler.
 *
 * Simply calls the application specific aem_remove_audio_mappings_handler
 * Remove audio_mappings from a stream port input/output, stored on the app side
 * \return		AECP_AEM return code, according to IEEE 1722.1-2013 table 7.126 (AECP_AEM_SUCCESS in case of success).
 * \param controlled	avdecc_controlled context that received the command.
 * \param pdu		Pointer to received AECP AEM PDU.
 */
int aem_remove_audio_mappings_handler(struct avdecc_controlled *controlled, struct aecp_aem_pdu *pdu)
{
	struct aecp_aem_modify_audio_mappings_cmd_pdu *audio_map_cmd  = (struct aecp_aem_modify_audio_mappings_cmd_pdu *)(pdu + 1);
	avb_u16 mappings_count = ntohs(audio_map_cmd->number_of_mappings);
	avb_u16 descriptor_index = ntohs(audio_map_cmd->descriptor_index);
	avb_u16 descriptor_type = ntohs(audio_map_cmd->descriptor_type);
	void *audio_mappings = audio_map_cmd + 1;
	int rc = AECP_AEM_NOT_IMPLEMENTED;

	if (controlled->aem_remove_audio_mappings_handler)
		rc = controlled->aem_remove_audio_mappings_handler(descriptor_index, descriptor_type, mappings_count, audio_mappings);

	return rc;
}

/** Generic AECP AEM ADD_AUDIO_MAPPINGS command handler.
 *
 * Simply calls the application specific aem_add_audio_mappings_handler.
 * Add audio_mappings to a stream port input/output, stored on the app side
 * \return		AECP_AEM return code, according to IEEE 1722.1-2013 table 7.126 (AECP_AEM_SUCCESS in case of success).
 * \param controlled	avdecc_controlled context that received the command.
 * \param pdu		Pointer to received AECP AEM PDU.
 */
int aem_add_audio_mappings_handler(struct avdecc_controlled *controlled, struct aecp_aem_pdu *pdu)
{
	struct aecp_aem_modify_audio_mappings_cmd_pdu *audio_map_cmd  = (struct aecp_aem_modify_audio_mappings_cmd_pdu *)(pdu + 1);
	avb_u16 mappings_count = ntohs(audio_map_cmd->number_of_mappings);
	avb_u16 descriptor_index = ntohs(audio_map_cmd->descriptor_index);
	avb_u16 descriptor_type = ntohs(audio_map_cmd->descriptor_type);
	void *audio_mappings = audio_map_cmd + 1;
	int rc = AECP_AEM_NOT_IMPLEMENTED;

	if (controlled->aem_add_audio_mappings_handler)
		rc = controlled->aem_add_audio_mappings_handler(descriptor_index, descriptor_type, mappings_count, audio_mappings);

	return rc;
}

/** Generic AECP AEM GET_AUDIO_MAP command handler.
 *
 * Simply calls the application specific aem_get_audio_mappings_handler.
 * Retrive the stored audio mappings from the app for a given stream port input/output
 * Prepare the response using the GET_AUDIO_MAP command received and add the retrived audio mappings to the response
 * \return		AECP_AEM return code, according to IEEE 1722.1-2013 table 7.126 (AECP_AEM_SUCCESS in case of success).
 * \param controlled	avdecc_controlled context that received the command.
 * \param[in,out] pdu	Pointer to received AECP AEM CMD PDU and points to AECP RSP PDU to be sent on exit
 * \param[out] pdu_len	Length of the output AECP AEM RSP PDU.
 */
int aem_get_audio_mappings_handler(struct avdecc_controlled *controlled, struct aecp_aem_pdu *pdu, unsigned int *pdu_len)
{
	struct aecp_aem_get_audio_map_rsp_pdu *audio_map_cmd  = (struct aecp_aem_get_audio_map_rsp_pdu *)(pdu + 1);
	struct aecp_aem_get_audio_map_rsp_pdu *audio_map_rsp  = (struct aecp_aem_get_audio_map_rsp_pdu *)(pdu + 1);
	avb_u16 descriptor_index = ntohs(audio_map_cmd->descriptor_index);
	avb_u16 descriptor_type = ntohs(audio_map_cmd->descriptor_type);
	avb_u16 map_index = ntohs(audio_map_cmd->map_index);
	void *audio_mappings = audio_map_rsp + 1;
	avb_u16 mappings_count, number_of_maps;
	int rc = AECP_AEM_NOT_IMPLEMENTED;

	audio_map_rsp->number_of_maps = htons(0);
	audio_map_rsp->number_of_mappings = htons(0);
	audio_map_rsp->reserved = htons(0);

	/* Set the AECP Response PDU length to match the aecp_aem_get_audio_map_rsp_pdu to be sent back */
	*pdu_len = sizeof(struct aecp_aem_pdu) + sizeof(struct aecp_aem_get_audio_map_rsp_pdu);

	if (controlled->aem_get_audio_mappings_handler) {
		rc = controlled->aem_get_audio_mappings_handler(descriptor_index, descriptor_type, map_index, &number_of_maps, &mappings_count, audio_mappings);

		if (rc == AECP_AEM_SUCCESS) {
			audio_map_rsp->number_of_maps = htons(number_of_maps);
			audio_map_rsp->number_of_mappings = htons(mappings_count);

			*pdu_len += (mappings_count * (sizeof(struct aecp_aem_get_audio_map_mappings_format)));
		}
	}

	return rc;
}

int avdecc_controlled_handler(struct avdecc_controlled *controlled, unsigned int events)
{
	union avb_controlled_msg msg;
	struct aecp_aem_pdu *pdu = (struct aecp_aem_pdu *)msg.aecp.buf;
	unsigned int msg_len, pdu_len;
	avb_msg_type_t msg_type;
	avb_u16 cmd_type;
	avb_u8 status;
	int rc = 0;

	(void)events;

	msg_len = sizeof(struct avb_aecp_msg);
	rc = avb_control_receive(controlled->handle, &msg_type, &msg, &msg_len);
	if (rc != AVB_SUCCESS) {
		printf("%s: WARNING: Got error message %d(%s) while trying to receive AECP command.\n", __func__, rc, avb_strerror(rc));
		rc = -1;
		goto receive_error;
	}

	switch (msg_type) {
	case AVB_MSG_AECP:
		cmd_type = AECP_AEM_GET_CMD_TYPE(pdu);
		if (msg.aecp.msg_type == AECP_AEM_COMMAND) {
			printf("%s: Received AVB AECP command %d\n", __func__, cmd_type);

			switch (cmd_type) {
			case AECP_AEM_CMD_SET_CONTROL:
				status = aem_set_control_handler(controlled, pdu);
				break;

			case AECP_AEM_CMD_START_STREAMING:
				printf("%s: Received AVB AECP command : AECP_AEM_CMD_START_STREAMING ... Do nothing for the moment\n", __func__);
				status = AECP_AEM_SUCCESS;
				break;

			case AECP_AEM_CMD_STOP_STREAMING:
				printf("%s: Received AVB AECP command : AECP_AEM_CMD_STOP_STREAMING ... Do nothing for the moment\n", __func__);
				status = AECP_AEM_SUCCESS;
				break;

			case AECP_AEM_CMD_REMOVE_AUDIO_MAPPINGS:
				status = aem_remove_audio_mappings_handler(controlled, pdu);
				break;

			case AECP_AEM_CMD_ADD_AUDIO_MAPPINGS:
				status = aem_add_audio_mappings_handler(controlled, pdu);
				break;

			case AECP_AEM_CMD_GET_AUDIO_MAP:
				status = aem_get_audio_mappings_handler(controlled, pdu, &pdu_len);
				msg.aecp.len = pdu_len;
				break;

			default:
				status = AECP_AEM_NOT_IMPLEMENTED;
				printf("%s: Command type (0x%x) not handled in this app, skip\n", __func__, cmd_type);
				break;
			}

			/* Multiple apps can be listening/handling controlled commands, if this app do not handle it, do not answer the stack and let other apps handle that. */
			if (status == AECP_AEM_NOT_IMPLEMENTED)
				break;

			msg.aecp.msg_type = AECP_AEM_RESPONSE;
			msg.aecp.status = status;

			rc = avb_control_send(controlled->handle, msg_type, &msg, msg_len);
			if (rc != AVB_SUCCESS) {
				printf("%s: WARNING: Got error message %d(%s) while trying to send response to AECP command.\n", __func__, rc, avb_strerror(rc));
				rc = -1;
			}
		} else {
			printf("%s: Error: Received AVB AECP response %d but only expecting commands.\n", __func__, cmd_type);
			rc = -1;
		}

		break;
	default:
		printf("%s: WARNING: Received unsupported AVDECC message type %d.\n", __func__, msg_type);
		rc = -1;
		break;
	}

receive_error:
	return rc;
}


int media_thread_loop(struct media_thread *thread)
{
	struct media_stream *stream;
	int ready, n;
	int i, rc = 0;
	int timer_fd = -1;
	struct itimerspec timer;

	if (thread->timeout_handler) {
		timer_fd = timerfd_create(CLOCK_REALTIME, 0);
		if (timer_fd < 0) {
			printf("%s timerfd_create() failed %s\n", __func__, strerror(errno));
			goto err;
		}

		timer.it_interval.tv_sec = 0;
		timer.it_interval.tv_nsec = thread->timeout_ms * 1000000;
		timer.it_value.tv_sec = 0;
		timer.it_value.tv_nsec = thread->timeout_ms * 1000000;

		if (timerfd_settime(timer_fd, 0, &timer, NULL) < 0) {
			printf("%s timerfd_settime() failed %s\n", __func__, strerror(errno));
			goto err;
		}
	}

	thread->poll_fd[0].fd = timer_fd;
	thread->poll_fd[0].events = POLLIN;

	if (thread->ctrl.handle) {
		thread->poll_fd[1].fd = avb_control_rx_fd(thread->ctrl.handle);
		thread->poll_fd[1].events = POLLIN;
		thread->ctrl.data = thread;
	} else
		thread->poll_fd[1].fd = -1;

	if (thread->controlled.handle) {
		thread->poll_fd[2].fd = avb_control_rx_fd(thread->controlled.handle);
		thread->poll_fd[2].events = POLLIN;
		thread->controlled.data = thread;
	} else
		thread->poll_fd[2].fd = -1;

	for (i = 0; i < thread->num_streams; i++) {
		stream = &thread->stream[i];

		stream->index = i;
		stream->thread = thread;

		media_stream_poll_set(stream, 0);
	}

	if (thread->init_handler) {
		rc = thread->init_handler(thread);
		if (rc < 0)
			goto err;
	}

	while (1) {
		if (thread->signal_handler && thread->signal_handler(thread)) {
			printf("signal_handler() exiting\n");
			break;
		}

		ready = poll(thread->poll_fd, thread->num_streams + NUM_CONTROL_FDS, -1);
		if (ready < 0) {
			if (errno == EINTR) {
				continue;
			}

			printf("thread(%p): poll() failed: %s\n", thread, strerror(errno));

			break;
		}

		if (!ready)
			continue;

		n = 0;

		if (thread->poll_fd[0].revents & POLLIN) {
			char tmp[8];
			int ret;

			ret = read(thread->poll_fd[0].fd, tmp, 8);
			if (ret < 0)
				printf("thread(%p): timer_fd read() failed: %s\n", thread, strerror(errno));

			if (thread->timeout_handler && thread->timeout_handler(thread)) {
				printf("timeout_handler() exiting\n");
				break;
			}

			n++;
		}

		if (thread->poll_fd[1].revents & POLLIN) {
			if (media_control_handler(&thread->ctrl) < 0)
				break;

			n++;
		}

		if (thread->poll_fd[2].revents & POLLIN) {
			if (avdecc_controlled_handler(&thread->controlled, POLLIN) < 0)
				break;

			n++;
		}

		for (i = 0; (i < thread->num_streams) && (n < ready); i++) {
			if ((thread->poll_fd[i + NUM_CONTROL_FDS].fd > 0) && (thread->poll_fd[i + NUM_CONTROL_FDS].revents & (POLLIN | POLLOUT))) {
				stream = &thread->stream[i];

				thread->data_handler(stream);

				n++;
			}
		}
	}

	if (thread->exit_handler)
		rc = thread->exit_handler(thread);

err:
	if (timer_fd > 0)
		close(timer_fd);

	return rc;
}
