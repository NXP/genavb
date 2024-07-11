/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2020, 2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief GenAVB AVDECC controls handling demo application
 @details
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <arpa/inet.h>
#include <errno.h>

#include <genavb/genavb.h>
#include <genavb/aecp.h>
#include <genavb/helpers.h>

#include <alsa/asoundlib.h>

static void usage (void)
{
	printf("\nUsage:\napp [options]\n");
	printf("\nOptions:\n"
		"\t-m <volume ctrl>      specify alsa volume control (default: DAC1)\n "
		"\t			 \"dummy\" Can be used as a fake alsa control to have the controlled socket opened (All avdecc controls will be ignored)\n "
		"\t-c <sound card name> specify sound card name (default: default)\n "
		"\t-h                    print this help text\n");
}

/*
 * The AECP commands map to the entity configuration
 * Only one control is implemented for playback
 *
 * Descriptor type  = AEM_DESC_TYPE_CONTROL
 * Descriptor index = 0
 * Value type       = AEM_CONTROL_LINEAR_UINT8 (One value)
 * Control type     = AEM_CONTROL_TYPE_GAIN
 * Signal type      = Audio cluster 0
 * Mapped to the DAC1 channel
 *
 * The command has been already processed by the
 * AVDECC stack and it has been checked that
 * the descriptor matches the one described in the
 * entity model.
 */

#define MAX_CONTROL_LEN	128
char control_mapping[MAX_CONTROL_LEN] = "DAC1";

snd_mixer_elem_t* alsa_elem;
snd_mixer_t *alsa_handle;
const char *sound_card_name = "default";

int alsa_init(void)
{
	snd_mixer_selem_id_t *sid;
	int err = 0;

	/* Initialize Alsa library handles */
	err = snd_mixer_open(&alsa_handle, 0);
	if (err < 0)
		goto print_error;

	err = snd_mixer_attach(alsa_handle, sound_card_name);
	if (err < 0)
		goto print_error;

	err = snd_mixer_selem_register(alsa_handle, NULL, NULL);
	if (err < 0)
		goto print_error;

	err = snd_mixer_load(alsa_handle);
	if (err < 0)
		goto print_error;

	if (!strcmp(control_mapping, "dummy")) {
		printf(" %s Using dummy alsa control ... Skip mixer setup and control\n", __func__);
		goto exit;
	}

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, 0);
	snd_mixer_selem_id_set_name(sid, control_mapping);
	alsa_elem = snd_mixer_find_selem(alsa_handle, sid);
	if (!alsa_elem) {
		printf("Alsa error: Cannot find mixer control %s \n", control_mapping);
		goto error;
	}

exit:
	return 0;

print_error:
	printf("Alsa init error: %s\n", snd_strerror(err));
error:
	return -1;
}

void alsa_close(void)
{
	if(alsa_handle)
		snd_mixer_close(alsa_handle);
}

void set_alsa_master_volume(long volume)
{
    long min, max, val;

    /* Adapt percent to audible dB format, 0-100 => -30dB-0dB */
    min = -30;
    max = 0;
    val = min + (volume * (max - min)) / 100;
    val *= 100;
    if (alsa_elem)
	    snd_mixer_selem_set_playback_dB_all(alsa_elem, val, 0);
}

int aecp_aem_cmd_rcv(struct aecp_aem_pdu *msg)
{
	avb_u16 cmd_type;
	unsigned short status = AECP_AEM_SUCCESS;

	cmd_type = AECP_AEM_GET_CMD_TYPE(msg);

	printf("aecp command type (0x%x) seq_id (%d)\n", cmd_type, ntohs(msg->sequence_id));

	switch (cmd_type) {
	case AECP_AEM_CMD_SET_CONTROL:
	{
		unsigned char avdecc_value;
		struct aecp_aem_set_get_control_pdu *set_control_cmd  = (struct aecp_aem_set_get_control_pdu *)(msg + 1);
		avb_u16 descriptor_index = ntohs(set_control_cmd->descriptor_index);

		/* Only descriptor index 0 has been mapped here: we only handle volume control in this app */
		if (descriptor_index > 0) {
			printf("AECP Command SET_CONTROL with index (0x%x) not handled in this app, skip\n", descriptor_index);
			return AECP_AEM_NOT_IMPLEMENTED;
		}

		avdecc_value = *(avb_u8 *)(set_control_cmd + 1);
		set_alsa_master_volume(avdecc_value);

		break;
	}
	default:
		printf("Command type (0x%x) not handled in this app, skip\n", cmd_type);
		status = AECP_AEM_NOT_IMPLEMENTED;

		break;
	}

	return status;
}

static int handle_avdecc_controlled(struct avb_control_handle *ctrl_h, avb_msg_type_t *msg_type)
{
	union avb_controlled_msg msg;
	unsigned int msg_len = sizeof(union avb_controlled_msg);
	int rc, status;

	rc = avb_control_receive(ctrl_h, msg_type, &msg, &msg_len);
	if (rc != AVB_SUCCESS) {
		printf("%s: WARNING: Got error message %d(%s) while trying to receive AECP command.\n", __func__, rc, avb_strerror(rc));
		goto receive_error;
	}

	switch (*msg_type) {
	case AVB_MSG_AECP:
		printf("AVB_MSG_AECP\n");

		status = aecp_aem_cmd_rcv((struct aecp_aem_pdu *)msg.aecp.buf);
		/* Multiple apps can be listening/handling controlled commands, if this app do not handle it, do not answer the stack and let other apps handle that. */
		if (status == AECP_AEM_NOT_IMPLEMENTED)
			break;

		msg.aecp.msg_type = AECP_AEM_RESPONSE;
		msg.aecp.status = status;
		rc = avb_control_send(ctrl_h, *msg_type, &msg, msg_len);
		if (rc != AVB_SUCCESS) {
			printf("%s: WARNING: Got error message %d(%s) while trying to send response to AECP command.\n", __func__, rc, avb_strerror(rc));
		}
		break;
	default:
		printf("%s: WARNING: Received unsupported AVDECC message type %d.\n", __func__, *msg_type);
		break;
	}

receive_error:
	return rc;
}


int main(int argc, char *argv[])
{
	struct avb_handle *avb_h;
	struct avb_control_handle *ctrl_h = NULL;
	int option;
	unsigned int event_type;
	int ctrl_rx_fd;
	struct pollfd ctrl_poll;
	int rc = 0;

	setlinebuf(stdout);

	printf("NXP's GenAVB AVDECC control handling demo application\n");

	while ((option = getopt(argc, argv,"hm:c:")) != -1) {
		switch (option) {
		case 'm':
			h_strncpy(control_mapping, optarg, MAX_CONTROL_LEN);
			printf("Using cmd line alsa control name for volume (%s)\n", control_mapping);
			break;

		case 'c':
			sound_card_name = optarg;
			break;

		case 'h':
		default:
			usage();
			rc = -1;
			goto exit;
		}
	}

	rc = alsa_init();
	if (rc < 0)
		goto error_alsa_init;

	/*
	 * Get avb handle
	 */
	rc = avb_init(&avb_h, 0);
	if (rc != AVB_SUCCESS) {
		printf("avb_init() failed: %s\n", avb_strerror(rc));
		rc = -1;
		goto error_avb_init;
	}

	/*
	 *  Open AVB_CTRL_AVDECC_CONTROLLED API control channel
	 */
	rc = avb_control_open(avb_h, &ctrl_h, AVB_CTRL_AVDECC_CONTROLLED);
	if (rc != AVB_SUCCESS) {
		printf("avb_control_open() failed: %s\n", avb_strerror(rc));
		goto error_control_open;
	}

	ctrl_rx_fd = avb_control_rx_fd(ctrl_h);
	ctrl_poll.fd = ctrl_rx_fd;
	ctrl_poll.events = POLLIN;
	ctrl_poll.revents = 0;

	while (1) {
		if (poll(&ctrl_poll, 1, -1) == -1) {
			printf("poll(%d) failed on waiting for connect\n", ctrl_poll.fd);
			rc = -1;
			goto error_ctrl_poll;
		}

		if (ctrl_poll.revents & POLLIN) {
			/*
			 * read control event from AVDECC
			 */
			if ((rc = handle_avdecc_controlled(ctrl_h, &event_type)) != AVB_SUCCESS)
				printf("handle_avdecc_event error(%d)\n", rc);
		}
	}

error_ctrl_poll:
	avb_control_close(ctrl_h);

error_control_open:
	avb_exit(avb_h);

error_avb_init:
error_alsa_init:
	alsa_close();
exit:
	return rc;
}
