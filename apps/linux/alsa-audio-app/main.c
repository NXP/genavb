/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2022, 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief GenAVB alsa application
 @details
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <arpa/inet.h>
#include <sched.h>
#include <signal.h>

#include <genavb/genavb.h>
#include <genavb/srp.h>

#include "../common/alsa.h"
#include "../common/avdecc.h"
#include "../common/msrp.h"

/* Application main modes */
#define MODE_AVDECC	0 /* the application relies on avdecc indication from avb stack*/
#define MODE_LISTENER	1 /* acting as media files server if avdecc is not used*/
#define MODE_TALKER	2 /* acting as media files server if avdecc is not used*/

#define PROCESS_PRIORITY		60  /* RT_FIFO priority to be used for the process */

#define BATCH_SIZE	512

#define DEFAULT_ALSA_DEVICE	"plughw:0,0"

static int signal_terminate = 0;

/* application main context */
struct avb_app {
	unsigned int mode;
	unsigned int config;
	int media_fd;
	struct avb_stream_params stream_params;
	unsigned int stream_batch_size;
	unsigned int stream_flags;
	struct avb_control_handle *ctrl_h;
	struct avb_control_handle *controlled_h;
	char *binding_file_name;
	char *alsa_device;
	int connected_stream_index;
};

struct avb_app app;

struct avb_stream_params default_stream_params = {
	.direction = AVTP_DIRECTION_LISTENER,
	.subtype = AVTP_SUBTYPE_61883_IIDC,
	.stream_class = SR_CLASS_A,
	.clock_domain = AVB_MEDIA_CLOCK_DOMAIN_STREAM,
	.flags = AVB_STREAM_FLAGS_MCR,
	.format.u.s = {
		.v = 0,
		.subtype = AVTP_SUBTYPE_61883_IIDC,
		.subtype_u.iec61883 = {
			.sf = IEC_61883_SF_61883,
			.fmt = IEC_61883_CIP_FMT_6,
			.r = 0,
			.format_u.iec61883_6 = {
				.fdf_u.fdf = {
					.evt = IEC_61883_6_FDF_EVT_AM824,
					.sfc = IEC_61883_6_FDF_SFC_48000,
				},
				.dbs = 2,
				.b = 0,
				.nb = 1,
				.rsvd = 0,
				.label_iec_60958_cnt = 0,
				.label_mbla_cnt = 2,
				.label_midi_cnt = 0,
				.label_smptecnt = 0,
			},
		},
	},
	.port = 0,
	.stream_id = { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 },
	.dst_mac = { 0x91, 0xE0, 0xF0, 0x00, 0xeb, 0x15 }
};

static void usage (void)
{
	printf("\nUsage:\napp [options]\n");
	printf("\nOptions:\n"
		"\t-m <mode>             application mode: avdecc(default), listener\n"
		"\t-h                    print this help text\n");

}


static void set_avb_config(unsigned int *avb_flags)
{
	*avb_flags = 0;
}


static int apply_config(struct avb_stream_params *stream_params)
{
	memcpy(&app.stream_params, stream_params, sizeof(struct avb_stream_params));

	app.stream_batch_size = BATCH_SIZE;

	app.stream_flags = AVTP_NONBLOCK;

	/* display the whole configuration */
	printf("stream ID: %02x%02x%02x%02x%02x%02x%02x%02x\n", stream_params->stream_id[0],stream_params->stream_id[1],stream_params->stream_id[2],stream_params->stream_id[3],
								stream_params->stream_id[4],stream_params->stream_id[5],stream_params->stream_id[6],stream_params->stream_id[7]);

	printf("mode: ");
	if (app.mode == MODE_LISTENER)
		printf("LISTENER\n");
	else if (app.mode == MODE_TALKER)
		printf("TALKER\n");
	else
		printf("AVDECC %s\n", (app.stream_params.direction == AVTP_DIRECTION_LISTENER)? "LISTENER":"TALKER");

	return 0;
}

static int handle_avdecc_event(struct avb_control_handle *ctrl_h, unsigned int *msg_type, const char *binding_file_name, bool *is_audio_stream)
{
	union avb_media_stack_msg msg;
	unsigned int msg_len = sizeof(union avb_media_stack_msg);
	int rc;

	*is_audio_stream = false;

	rc = avb_control_receive(ctrl_h, msg_type, &msg, &msg_len);
	if (rc != AVB_SUCCESS)
		goto receive_error;

	switch (*msg_type) {
	case GENAVB_MSG_MEDIA_STACK_CONNECT:
		if (!(avdecc_format_is_61883_6(&msg.media_stack_connect.stream_params.format) || avdecc_format_is_aaf_pcm(&msg.media_stack_connect.stream_params.format))) {
			printf("\nIgnoring stream formats other than 61883_6 or AAF\n");
			goto exit;
		}

		*is_audio_stream = true;

		printf("GENAVB_MSG_MEDIA_STACK_CONNECT\n");
		apply_config(&msg.media_stack_connect.stream_params);
		app.connected_stream_index = msg.media_stack_connect.stream_index;
		break;

	case GENAVB_MSG_MEDIA_STACK_DISCONNECT:
		if (msg.media_stack_disconnect.stream_index != app.connected_stream_index) {
			printf("\nIgnoring stream with a different id than the current connected stream\n");
			goto exit;
		}

		*is_audio_stream = true;

		printf("GENAVB_MSG_MEDIA_STACK_DISCONNECT\n");
		app.connected_stream_index = -1;
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

exit:
receive_error:
	return rc;
}

static int handle_avdecc_controlled_event(struct avb_control_handle *controlled_h)
{
	unsigned int msg_len = sizeof(union avb_controlled_msg);
	unsigned short status = AECP_AEM_SUCCESS;
	avb_msg_type_t msg_type;
	union avb_controlled_msg msg;
	struct aecp_aem_pdu *pdu;
	avb_u16 cmd_type;
	int rc;

	rc = avb_control_receive(controlled_h, &msg_type, &msg, &msg_len);
	if (rc != AVB_SUCCESS) {
		printf("%s: WARNING: Got error message %d(%s) while trying to receive AECP command.\n", __func__, rc, avb_strerror(rc));
		goto receive_error;
	}

	switch (msg_type) {
	case AVB_MSG_AECP:
		printf("AVB_MSG_AECP\n");

		pdu = (struct aecp_aem_pdu *)msg.aecp.buf;

		cmd_type = AECP_AEM_GET_CMD_TYPE(pdu);
		printf("aecp command type (0x%x) seq_id (%d)\n", cmd_type, ntohs(pdu->sequence_id));

		switch (cmd_type) {
		case AECP_AEM_CMD_GET_AUDIO_MAP:
		{
			/* GET_AUDIO_MAP not fully supported by alsa-audio-app, simply respond with empty audio mappings */
			struct aecp_aem_get_audio_map_rsp_pdu *audio_map_rsp  = (struct aecp_aem_get_audio_map_rsp_pdu *)(pdu + 1);

			audio_map_rsp->number_of_maps = htons(0);
			audio_map_rsp->number_of_mappings = htons(0);
			audio_map_rsp->reserved = htons(0);

			/* Set the AECP Response PDU length to match the aecp_aem_get_audio_map_rsp_pdu to be sent back */
			msg.aecp.len = sizeof(struct aecp_aem_pdu) + sizeof(struct aecp_aem_get_audio_map_rsp_pdu);

			break;
		}
		default:
			printf("Command type (0x%x) not handled in this app, skip\n", cmd_type);
			status = AECP_AEM_NOT_IMPLEMENTED;

			/* Multiple apps can be listening/handling controlled commands,
			 * if this app do not handle it, do not answer the stack and let other apps handle that. */
			goto exit;
		}

		msg.aecp.msg_type = AECP_AEM_RESPONSE;
		msg.aecp.status = status;

		rc = avb_control_send(controlled_h, msg_type, &msg, msg_len);
		if (rc != AVB_SUCCESS) {
			printf("%s: WARNING: Got error message %d(%s) while trying to send response to AECP command.\n", __func__, rc, avb_strerror(rc));
		}
		break;
	default:
		printf("%s: WARNING: Received unsupported AVDECC message type %d.\n", __func__, msg_type);
		break;
	}

exit:
receive_error:
	return rc;
}


static int run_listener(struct avb_stream_handle *stream_h, int stream_fd, unsigned int batch_size, void *alsa_h)
{
	struct pollfd poll_fds[3];
	int rc = 0;
	int ctrl_rx_fd = -1;
	int controlled_rx_fd = -1;
	int ready, i, n, nfds;
	unsigned int event_type;
	bool is_audio_stream;

	/*
	* setup ALSA to receive samples
	*/

	alsa_h = alsa_tx_init((void*)&app.stream_params.stream_id, &app.stream_params.format, batch_size, app.alsa_device);
	if (!alsa_h) {
		printf("Couldn't initialize alsa tx, aborting...\n");
		rc = -1;
		goto err_alsa_init;
	}

	printf("Starting listener loop, non-blocking mode\n");
	/*
	* listen to read event from the stack
	*/

	poll_fds[0].fd = stream_fd;
	poll_fds[0].events = POLLIN;
	poll_fds[0].revents = 0;

	nfds = 1;

	if (app.ctrl_h) {
		ctrl_rx_fd = avb_control_rx_fd(app.ctrl_h);
		poll_fds[1].fd = ctrl_rx_fd;
		poll_fds[1].events = POLLIN;
		poll_fds[1].revents = 0;

		nfds++;
	} else {
		poll_fds[1].fd = -1;
		poll_fds[1].events = 0;
		poll_fds[1].revents = 0;
	}

	if (app.controlled_h) {
		controlled_rx_fd = avb_control_rx_fd(app.controlled_h);
		poll_fds[2].fd = controlled_rx_fd;
		poll_fds[2].events = POLLIN;
		poll_fds[2].revents = 0;

		nfds++;
	} else {
		poll_fds[2].fd = -1;
		poll_fds[2].events = 0;
		poll_fds[2].revents = 0;
	}

	while (1) {
		if ((ready = poll(poll_fds, nfds, -1)) == -1) {
			if (errno == EINTR) {
				if (signal_terminate) {
					printf("processing terminate signal\n");
					signal_terminate = 0;
					rc = -1;
					goto exit;
				}
			} else {
				printf("poll(%d) failed while processing listener\n", stream_fd);
				rc = -1;
				goto exit;
			}
		}

		if (ready > 0) {
			for (n = 0, i = 0; i < nfds && n < ready; i++) {
				if (poll_fds[i].revents & POLLIN) {
					if (poll_fds[i].fd == ctrl_rx_fd) {
						n++;

						/*
						* read control event from avdecc
						*/
						if (handle_avdecc_event(app.ctrl_h, &event_type, app.binding_file_name, &is_audio_stream) == AVB_SUCCESS) {
							if (event_type == GENAVB_MSG_MEDIA_STACK_DISCONNECT && is_audio_stream) {
								rc = 0;
								goto exit; /* disconnected, stop stream processing */
							}
						}
					} else if (poll_fds[i].fd == stream_fd) {
						n++;

						/*
						* send samples to alsa
						*/
						if (alsa_tx(alsa_h, stream_h, &app.stream_params) < 0) {
							printf("Error writing data to alsa...\n");
							rc = -1;
						}
					} else if (poll_fds[i].fd == controlled_rx_fd) {
						n++;

						/*
						* read controlled event from avdecc
						*/
						if (handle_avdecc_controlled_event(app.controlled_h) != AVB_SUCCESS) {
							printf("handle_avdecc_controlled_event error\n");
							rc = -1;
						}
					}
				}
			}
		}
	}

exit:
	alsa_tx_exit(alsa_h);

err_alsa_init:
	return rc;

}

void signal_terminate_handler (int signal_num)
{
	signal_terminate = 1;
}

int main(int argc, char *argv[])
{
	struct avb_handle *avb_h;
	struct avb_stream_handle* stream_h;
	unsigned int avb_flags;
	int stream_fd, option;
	unsigned int event_type;
	int ctrl_rx_fd = -1;
	int controlled_rx_fd = -1;
	int nfds, ready, i, n;
	struct pollfd ctrl_poll[2];
	int rc = 0;
	void *alsa_h;
	struct sched_param param = {
			.sched_priority = PROCESS_PRIORITY,
		};
	struct sigaction action;
	bool is_audio_stream;

	/*
	 * Increase process priority to match the AVTP thread priority
	 */
	if (sched_setscheduler(0, SCHED_FIFO, &param) < 0) {
		printf("sched_setscheduler(), %s\n", strerror(errno));
		rc = -1;
		goto exit;
	}

	setlinebuf(stdout);

	printf("NXP's GenAVB ALSA reference application\n");


	/*
	* retrieve user's configuration parameters
	*/

	app.mode = MODE_AVDECC; app.config = 0; /*default is AVDECC, NON-BLOCKING, SINGLE BUFFER */
	app.alsa_device = DEFAULT_ALSA_DEVICE;
	app.connected_stream_index = -1;

	while ((option = getopt(argc, argv,"m:b:d:h")) != -1) {
		switch (option) {
		case 'm':
			if (!strcasecmp(optarg, "listener"))
				app.mode = MODE_LISTENER;
			else if(!strcasecmp(optarg, "avdecc"))
				app.mode = MODE_AVDECC;
			else {
				usage();
				goto exit;
			}
			break;

		case 'b':
			app.binding_file_name = optarg;
			break;

		case 'd':
			app.alsa_device = optarg;
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

	/*
	* Setup the avb stack
	*/
	set_avb_config(&avb_flags);

	rc = avb_init(&avb_h, avb_flags);
	if (rc != AVB_SUCCESS) {
		printf("avb_init() failed: %s\n", avb_strerror(rc));
		rc = -1;
		goto error_avb_init;
	}

	if (app.mode == MODE_AVDECC) {
		/*
		* Open control channel for AVDECC events.
		*/
		rc = avb_control_open(avb_h, &app.ctrl_h, AVB_CTRL_AVDECC_MEDIA_STACK);
		if (rc != AVB_SUCCESS) {
			printf("avb_control_open() failed on control chanel: %s\n", avb_strerror(rc));
			goto error_control_open;
		}

		ctrl_rx_fd = avb_control_rx_fd(app.ctrl_h);
		ctrl_poll[0].fd = ctrl_rx_fd;
		ctrl_poll[0].events = POLLIN;
		nfds = 1;

		/*
		* Open controlled channel for AVDECC events.
		*/
		rc = avb_control_open(avb_h, &app.controlled_h, AVB_CTRL_AVDECC_CONTROLLED);
		if (rc != AVB_SUCCESS) {
			printf("avb_control_open() failed on controlled chanel: %s\n", avb_strerror(rc));
			goto error_controlled_open;
		}

		controlled_rx_fd = avb_control_rx_fd(app.controlled_h);
		ctrl_poll[1].fd = controlled_rx_fd;
		ctrl_poll[1].events = POLLIN;
		nfds++;

		/* If we have a filename for non-volatile binding params parse it and init the avdecc stack with its parameters. */
		if (app.binding_file_name) {
			if (avdecc_nvm_bindings_init(app.ctrl_h, app.binding_file_name) < 0) {
				printf("failed to parse binding file %s\n", app.binding_file_name);
				rc = -1;
				goto err_bind;
			}
		}
	} else {
		rc = msrp_init(avb_h);
		if (rc < 0)
			goto err_msrp_init;
	}

wait_new_stream:
	if (app.mode == MODE_AVDECC) {

		printf("\nwait for new stream...\n");

		/*
		* Listen to AVDECC events to get stream parameters
 		*/
		while (1) {
			if ((ready = poll(ctrl_poll, nfds, -1)) == -1) {
				printf("poll failed on waiting for connect\n");
				rc = -1;
				goto error_ctrl_poll;
			}

			if (ready > 0) {
				for (n = 0, i = 0; i < nfds && n < ready; i++) {
					if (ctrl_poll[i].revents & POLLIN) {
						if (ctrl_poll[i].fd == ctrl_rx_fd) {
							n++;

							/*
							* read control event from avdecc
							*/
							if (handle_avdecc_event(app.ctrl_h, &event_type, app.binding_file_name, &is_audio_stream) == AVB_SUCCESS) {
								if (event_type == GENAVB_MSG_MEDIA_STACK_CONNECT && is_audio_stream)
									goto start_stream; /* connected, start stream processing */
							}

						} else if (ctrl_poll[i].fd == controlled_rx_fd) {
							n++;

							/*
							* read controlled event from avdecc
							*/
							handle_avdecc_controlled_event(app.controlled_h);
						}
					}
				}
			}
		}
	} else {
		/*
		* no avdecc, static configuration used
		*/

		if (app.mode == MODE_TALKER)
			default_stream_params.direction = AVTP_DIRECTION_TALKER;
		else
			default_stream_params.direction = AVTP_DIRECTION_LISTENER;

		apply_config(&default_stream_params);

		if (app.mode == MODE_TALKER) {
			rc = msrp_talker_register(&default_stream_params);
			if (rc != AVB_SUCCESS) {
				printf("msrp_talker_register error, rc = %d\n", rc);
				goto err_msrp;
			}
		} else if (app.mode == MODE_LISTENER) {
			rc = msrp_listener_register(&default_stream_params);
			if (rc != AVB_SUCCESS) {
				printf("msrp_listener_register error, rc = %d\n", rc);
				goto err_msrp;
			}
		}
	}

start_stream:
	/*
	* setup the stream
	*/
	rc = avb_stream_create (avb_h, &stream_h, &app.stream_params, &app.stream_batch_size, app.stream_flags);
	if (rc != AVB_SUCCESS) {
		printf("avb_stream_create() failed: %s\n", avb_strerror(rc));
		rc = -1;
		goto error_stream_create;
	}
	printf("Configured AVB batch size (bytes): %d\n", app.stream_batch_size);


	/*
	* retrieve the file descriptor associated to the stream
	*/

	stream_fd = avb_stream_fd(stream_h);
	if (stream_fd < 0) {
		printf("avb_stream_fd() failed: %s\n", avb_strerror(stream_fd));
		rc = -1;
		goto error_stream_fd;
	}

	/*
	* run listener/talker main processing function
	*/

	if (app.stream_params.direction & AVTP_DIRECTION_TALKER) {
		printf("Talker direction not supported yet.\n");
		rc = -1;
	}
	else
		rc = run_listener(stream_h, stream_fd,  app.stream_batch_size, alsa_h);

	if (rc < 0) {
		printf("Loop function exited with error code %d\n", rc);
	} else {
		printf("Loop function exited upon avdecc disconnect\n");

		avb_stream_destroy(stream_h);

		goto wait_new_stream;
	}

	/*
	* destroy the stream, disconnect for avb stack
	*/
error_stream_fd:
	avb_stream_destroy(stream_h);

error_stream_create:
	if (app.mode == MODE_TALKER)
		msrp_talker_deregister(&default_stream_params);
	else if (app.mode == MODE_LISTENER)
		msrp_listener_deregister(&default_stream_params);

err_msrp:
error_ctrl_poll:
err_bind:
	if (app.controlled_h)
		avb_control_close(app.controlled_h);

error_controlled_open:
	if (app.ctrl_h)
		avb_control_close(app.ctrl_h);

error_control_open:
	if (app.mode != MODE_AVDECC)
		msrp_exit();
err_msrp_init:
	avb_exit(avb_h);

error_avb_init:
exit:
	return rc;

}
