/*
 * Copyright 2018, 2020-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <sys/poll.h>
#include <errno.h>
#include <stdbool.h>

#include <genavb/genavb.h>
#include <genavb/helpers.h>
#include <genavb/srp.h>

#define DEFAULT_PORT			0
#define DEFAULT_STREAM_ID		0x0011223344556677
#define DEFAULT_SR_CLASS		SR_CLASS_A
#define DEFAULT_MAC_ADDR		{0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff}
#define DEFAULT_VID			VLAN_VID_DEFAULT
#define DEFAULT_MAX_FRAME_SIZE		128
#define DEFAULT_MAX_INTERVAL_FRAMES	1
#define DEFAULT_ACCUMULATED_LATENCY	10000
#define DEFAULT_RANK			NORMAL

static void usage (void)
{
	printf("\nUsage:\napp [options]\n");
	printf("\nOptions:\n"
		"\t-t                        talker stream\n"
		"\t-l                        listener stream\n"
		"\t-r                        register stream\n"
		"\t-d                        deregister stream\n"
		"\t-w                        wait and dump indications\n"
		"\t-p <port_id>              port id: default 0\n"
		"\t-s <stream_id>            stream id: default 0x0011223344556677\n"
		"\t-c <sr_class>             sr class: default 0 (SR_CLASS_A)\n"
		"\t-m <mac_address>          mac addr: default aa:bb:cc:dd:ee:ff\n"
		"\t-v <vlan_id>              vlan id\n"
		"\t-f <max_frame_size>       max frame size in bytes: default 128\n"
		"\t-i <max_interval_frames>  max interval frames: default 1\n"
		"\t-a <accumulated latency>  accumulated latency in nanoseconds: default 10000\n"
		"\t-k <rank>                 rank: default 1 (normal)\n"
		"\t-h                        print this help text\n"
		"\nTalker registration:\n"
		"\t-t -r\n"
		"\t-t -p 1 -s 0x0123456789abcdef -m 00:34:56:78:aa:55 -v 2 -f 256 -i 2 -r\n"
		"\nListener registration:\n"
		"\t-l -r\n"
		"\t-l -p 1 -s 0x0123456789abcdef -r\n"
		"\nTalker deregistration:\n"
		"\t-t -d\n"
		"\t-t -p 1 -s 0x0123456789abcdef -d\n"
		"\nListener deregistration:\n"
		"\t-l -d\n"
		"\t-l -p 1 -s 0x0123456789abcdef -d\n");

}

static int msrp_listener_register(struct genavb_control_handle *ctrl_h, uint16_t port, uint64_t stream_id)
{
	struct genavb_msg_listener_register listener_register;
	unsigned int msg_type;
	union genavb_msg_msrp msg;
	unsigned int msg_len;
	int rc;

	listener_register.port = port;
	memcpy(listener_register.stream_id, &stream_id, 8);

	msg_type = GENAVB_MSG_LISTENER_REGISTER;
	msg_len = sizeof(msg);
	rc = genavb_control_send_sync(ctrl_h, &msg_type, &listener_register, sizeof(listener_register), &msg, &msg_len, 10);
	if (rc < 0) {
		printf("genavb_control_send_sync() failed: %s\n", genavb_strerror(rc));
		goto err;
	} else if (msg_type != GENAVB_MSG_LISTENER_RESPONSE) {
		printf("response type error: %d\n", msg_type);
		goto err;
	} else if (msg.listener_response.status != GENAVB_SUCCESS) {
		printf("response status error: %s\n", genavb_strerror(msg.listener_response.status));
		goto err;
	}

	printf("Listener registered\n");

	return 0;

err:
	return -1;
}

static int msrp_listener_deregister(struct genavb_control_handle *ctrl_h, uint16_t port, uint64_t stream_id)
{
	struct genavb_msg_listener_deregister listener_deregister;
	unsigned int msg_type;
	union genavb_msg_msrp msg;
	unsigned int msg_len;
	int rc;

	listener_deregister.port = port;
	memcpy(listener_deregister.stream_id, &stream_id, 8);

	msg_type = GENAVB_MSG_LISTENER_DEREGISTER;
	msg_len = sizeof(msg);
	rc = genavb_control_send_sync(ctrl_h, &msg_type, &listener_deregister, sizeof(listener_deregister), &msg, &msg_len, 10);
	if (rc < 0) {
		printf("genavb_control_send_sync() failed: %s\n", genavb_strerror(rc));
		goto err;
	} else if (msg_type != GENAVB_MSG_LISTENER_RESPONSE) {
		printf("response type error: %d\n", msg_type);
		goto err;
	} else if (msg.listener_response.status != GENAVB_SUCCESS) {
		printf("response status error: %s\n", genavb_strerror(msg.listener_response.status));
		goto err;
	}

	printf("Listener deregistered\n");

	return 0;

err:
	return -1;
}

static int msrp_talker_register(struct genavb_control_handle *ctrl_h, uint16_t port, uint64_t stream_id, sr_class_t sr_class,
				uint8_t *mac_addr, uint16_t vid, uint16_t max_frame_size, uint16_t max_interval_frames,
				uint32_t accumulated_latency, msrp_rank_t rank)
{
	struct genavb_msg_talker_register talker_register;
	unsigned int msg_type;
	union genavb_msg_msrp msg;
	unsigned int msg_len;
	int rc;

	talker_register.port = port;
	memcpy(talker_register.stream_id, &stream_id, 8);
	talker_register.params.stream_class = sr_class;
	memcpy(talker_register.params.destination_address, mac_addr, 6);
	talker_register.params.vlan_id = vid;
	talker_register.params.max_frame_size = max_frame_size;
	talker_register.params.max_interval_frames = max_interval_frames;
	talker_register.params.accumulated_latency = accumulated_latency;
	talker_register.params.rank = rank;

	msg_type = GENAVB_MSG_TALKER_REGISTER;
	msg_len = sizeof(msg);
	rc = genavb_control_send_sync(ctrl_h, &msg_type, &talker_register, sizeof(talker_register), &msg, &msg_len, 10);
	if (rc < 0) {
		printf("genavb_control_send_sync() failed: %s\n", genavb_strerror(rc));
		goto err;
	} else if (msg_type != GENAVB_MSG_TALKER_RESPONSE) {
		printf("response type error: %d\n", msg_type);
		goto err;
	} else if (msg.talker_response.status != GENAVB_SUCCESS) {
		printf("response status error: %s\n", genavb_strerror(msg.talker_response.status));
		goto err;
	}

	printf("Talker registered\n");

	return 0;

err:
	return -1;
}

static int msrp_talker_deregister(struct genavb_control_handle *ctrl_h, uint16_t port, uint64_t stream_id)
{
	struct genavb_msg_talker_deregister talker_deregister;
	unsigned int msg_type;
	union genavb_msg_msrp msg;
	unsigned int msg_len;
	int rc;

	talker_deregister.port = port;
	memcpy(talker_deregister.stream_id, &stream_id, 8);

	msg_type = GENAVB_MSG_TALKER_DEREGISTER;
	msg_len = sizeof(msg);
	rc = genavb_control_send_sync(ctrl_h, &msg_type, &talker_deregister, sizeof(talker_deregister), &msg, &msg_len, 10);
	if (rc < 0) {
		printf("genavb_control_send_sync() failed: %s\n", genavb_strerror(rc));
		goto err;
	} else if (msg_type != GENAVB_MSG_TALKER_RESPONSE) {
		printf("response type error: %d\n", msg_type);
		goto err;
	} else if (msg.talker_response.status != GENAVB_SUCCESS) {
		printf("response status error: %s\n", genavb_strerror(msg.talker_response.status));
		goto err;
	}

	printf("Talker deregistered\n");

	return 0;

err:
	return -1;
}

static const char *talker_status_str[] = {
	[NO_LISTENER] = "NO_LISTENER",
	[FAILED_LISTENER] = "FAILED_LISTENER",
	[ACTIVE_AND_FAILED_LISTENERS] = "ACTIVE_AND_FAILED_LISTENERS",
	[ACTIVE_LISTENER] = "ACTIVE_LISTENER"
};

static const char *failure_code_str[] = {
	[INSUFFICIENT_BANDWIDTH] = "INSUFFICIENT_BANDWIDTH",
	[INSUFFICIENT_BRIDGE_RESOURCES] = "INSUFFICIENT_BRIDGE_RESOURCES",
	[INSUFFICIENT_BANDWIDTH_FOR_TRAFFIC_CLASS] = "INSUFFICIENT_BANDWIDTH_FOR_TRAFFIC_CLASS",
	[STREAM_ID_ALREADY_IN_USE] = "STREAM_ID_ALREADY_IN_USE",
	[STREAM_DESTINATION_ADDRESS_ALREADY_IN_USE] = "STREAM_DESTINATION_ADDRESS_ALREADY_IN_USE",
	[STREAM_PREEMPTED_BY_HIGHER_RANK] = "STREAM_PREEMPTED_BY_HIGHER_RANK",
	[REPORTED_LATENCY_HAS_CHANGED] = "REPORTED_LATENCY_HAS_CHANGED",
	[EGRESS_PORT_IS_NOT_AVB_CAPABLE] = "EGRESS_PORT_IS_NOT_AVB_CAPABLE",
	[USE_DIFFERENT_DESTINATION_ADDRESS] = "USE_DIFFERENT_DESTINATION_ADDRESS",
	[OUT_OF_MSRP_RESOURCES] = "OUT_OF_MSRP_RESOURCES",
	[OUT_OF_MMRP_RESOURCES] = "OUT_OF_MMRP_RESOURCES",
	[CANNOT_STORE_DESTINATION_ADDRESS] = "CANNOT_STORE_DESTINATION_ADDRESS",
	[REQUESTED_PRIORITY_IS_NOT_AN_SR_CLASS_PRIORITY] = "REQUESTED_PRIORITY_IS_NOT_AN_SR_CLASS_PRIORITY",
	[MAX_FRAME_SIZE_TOO_LARGE_FOR_MEDIA] = "MAX_FRAME_SIZE_TOO_LARGE_FOR_MEDIA",
	[FAN_IN_PORT_LIMIT_REACHED] = "FAN_IN_PORT_LIMIT_REACHED",
	[CHANGE_IN_FIRST_VALUE_FOR_REGISTED_STREAM_ID] = "CHANGE_IN_FIRST_VALUE_FOR_REGISTED_STREAM_ID",
	[VLAN_BLOCKED_ON_EGRESS_PORT] = "VLAN_BLOCKED_ON_EGRESS_PORT",
	[VLAN_TAGGING_DISABLED_ON_EGRESS_PORT] = "VLAN_TAGGING_DISABLED_ON_EGRESS_PORT",
	[SR_CLASS_PRIORITY_MISMATCH] = "SR_CLASS_PRIORITY_MISMATCH"
};


static const char *listener_status_str[] = {
	[NO_TALKER] = "NO_TALKER",
	[ACTIVE] = "ACTIVE",
	[FAILED] = "FAILED"
};

static void msrp_listen(struct genavb_control_handle *ctrl_h, int ctrl_fd)
{
	unsigned int msg_type;
	union genavb_msg_msrp msg;
	unsigned int msg_len;
	fd_set set;
	int rc;

	while (1) {
		FD_ZERO(&set);
		FD_SET(ctrl_fd, &set);

		rc = select(ctrl_fd + 1, &set, NULL, NULL, NULL);
		if (rc < 0) {
			printf ("select() failed: %s\n", strerror(errno));
			break;
		}

		if (!FD_ISSET(ctrl_fd, &set))
			continue;

		msg_len = sizeof(msg);

		rc = genavb_control_receive(ctrl_h, &msg_type, &msg, &msg_len);
		if (rc < 0) {
			printf ("genavb_control_receive() failed: %s\n", genavb_strerror(rc));
			break;
		}

		switch (msg_type) {
		case GENAVB_MSG_TALKER_RESPONSE:
			break;

		case GENAVB_MSG_LISTENER_RESPONSE:
			break;

		case GENAVB_MSG_TALKER_DECLARATION_STATUS:
			break;

		case GENAVB_MSG_LISTENER_DECLARATION_STATUS:
			break;

		case GENAVB_MSG_LISTENER_STATUS:
			printf ("(%u) Talker stream: %02x%02x%02x%02x%02x%02x%02x%02x, port: %u, status: %u, %s\n",
				getpid(),
				msg.listener_status.stream_id[0], msg.listener_status.stream_id[1], msg.listener_status.stream_id[2], msg.listener_status.stream_id[3],
				msg.listener_status.stream_id[4], msg.listener_status.stream_id[5], msg.listener_status.stream_id[6], msg.listener_status.stream_id[7],
				msg.listener_status.port, msg.listener_status.status, listener_status_str[msg.listener_status.status]);

			if (msg.listener_status.status == ACTIVE)
				printf("  class: %u, mac: %02x%02x%02x%02x%02x%02x, vid: %u, frame size: %u, interval frames: %u, latency: %u, rank: %u\n",
					msg.listener_status.params.stream_class,
					msg.listener_status.params.destination_address[0], msg.listener_status.params.destination_address[1], msg.listener_status.params.destination_address[2],
					msg.listener_status.params.destination_address[3], msg.listener_status.params.destination_address[4], msg.listener_status.params.destination_address[5],
					msg.listener_status.params.vlan_id,
					msg.listener_status.params.max_frame_size, msg.listener_status.params.max_interval_frames,
					msg.listener_status.params.accumulated_latency,
					msg.listener_status.params.rank);
			else if (msg.listener_status.status == FAILED)
				printf("  bridge id: %02x%02x%02x%02x%02x%02x, failure code: %u, %s\n",
					msg.listener_status.failure.bridge_id[0], msg.listener_status.failure.bridge_id[1], msg.listener_status.failure.bridge_id[2],
					msg.listener_status.failure.bridge_id[3], msg.listener_status.failure.bridge_id[4], msg.listener_status.failure.bridge_id[5],
					msg.listener_status.failure.failure_code, failure_code_str[msg.listener_status.failure.failure_code]);

			break;

		case GENAVB_MSG_TALKER_STATUS:
			printf ("(%u) Listener stream: %02x%02x%02x%02x%02x%02x%02x%02x, port: %u, status: %u, %s\n",
				getpid(),
				msg.talker_status.stream_id[0], msg.talker_status.stream_id[1], msg.talker_status.stream_id[2], msg.talker_status.stream_id[3],
				msg.talker_status.stream_id[4], msg.talker_status.stream_id[5], msg.talker_status.stream_id[6], msg.talker_status.stream_id[7],
				msg.talker_status.port, msg.talker_status.status, talker_status_str[msg.talker_status.status]);
			break;

		default:
			printf ("Unexpected message type %d\n", msg_type);
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	struct genavb_handle *avb_h;
	struct genavb_control_handle *ctrl_h;
	unsigned long port = DEFAULT_PORT;
	unsigned long long stream_id = DEFAULT_STREAM_ID;
	unsigned long sr_class = DEFAULT_SR_CLASS;
	uint8_t mac_addr[6] = DEFAULT_MAC_ADDR;
	unsigned long vid = DEFAULT_VID;
	unsigned long max_frame_size = DEFAULT_MAX_FRAME_SIZE;
	unsigned long max_interval_frames = DEFAULT_MAX_INTERVAL_FRAMES;
	unsigned long accumulated_latency = DEFAULT_ACCUMULATED_LATENCY;
	unsigned long rank = DEFAULT_RANK;
	bool talker = false;
	int ctrl_fd;
	int option;
	int rc = 0;

	setlinebuf(stdout);

	printf("NXP's GenAVB MRP control application\n");

	/*
	* setup the avb stack
	*/

	rc = genavb_init(&avb_h, 0);
	if (rc != GENAVB_SUCCESS) {
		printf("genavb_init() failed: %s\n", genavb_strerror(rc));
		rc = -1;
		goto error_avb_init;
	}

	rc = genavb_control_open(avb_h, &ctrl_h, GENAVB_CTRL_MSRP);
	if (rc != GENAVB_SUCCESS) {
		printf("genavb_control_open() failed: %s\n", genavb_strerror(rc));
		rc = -1;
		goto error_control_open;
	}

	ctrl_fd = genavb_control_rx_fd(ctrl_h);

	/*
	* retrieve user's configuration parameters
	*/

	while ((option = getopt(argc, argv,"p:s:c:m:v:f:i:a:k:tlrdw")) != -1) {
		switch (option) {
		case 'p':
			if (h_strtoul(&port, optarg, NULL, 0) < 0) {
				printf("invalid -p %s option\n", optarg);
				rc = -1;
				goto exit;
			}

			break;

		case 's':
			if (h_strtoull(&stream_id, optarg, NULL, 0) < 0) {
				printf("invalid -s %s option\n", optarg);
				rc = -1;
				goto exit;
			}

			stream_id = htonll(stream_id);
			break;

		case 'c':
			if (h_strtoul(&sr_class, optarg, NULL, 0) < 0) {
				printf("invalid -c %s option\n", optarg);
				rc = -1;
				goto exit;
			}

			break;

		case 'm':
			if (sscanf(optarg, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac_addr[0], &mac_addr[1], &mac_addr[2], &mac_addr[3], &mac_addr[4], &mac_addr[5]) < 6) {
				printf("invalid -m %s option\n", optarg);
				rc = -1;
				goto exit;
			}

			break;

		case 'v':
			if (h_strtoul(&vid, optarg, NULL, 0) < 0) {
				printf("invalid -v %s option\n", optarg);
				rc = -1;
				goto exit;
			}

			break;

		case 'f':
			if (h_strtoul(&max_frame_size, optarg, NULL, 0) < 0) {
				printf("invalid -f %s option\n", optarg);
				rc = -1;
				goto exit;
			}

			break;

		case 'i':
			if (h_strtoul(&max_interval_frames, optarg, NULL, 0) < 0) {
				printf("invalid -i %s option\n", optarg);
				rc = -1;
				goto exit;
			}

			break;

		case 'a':
			if (h_strtoul(&accumulated_latency, optarg, NULL, 0) < 0) {
				printf("invalid -a %s option\n", optarg);
				rc = -1;
				goto exit;
			}

			break;

		case 'k':
			if (h_strtoul(&rank, optarg, NULL, 0) < 0) {
				printf("invalid -k %s option\n", optarg);
				rc = -1;
				goto exit;
			}

			break;

		case 't':
			talker = true;
			break;

		case 'l':
			talker = false;
			break;

		case 'r':
			if (talker)
				rc = msrp_talker_register(ctrl_h, port, stream_id, sr_class, mac_addr, vid, max_frame_size, max_interval_frames, accumulated_latency, rank);
			else
				rc = msrp_listener_register(ctrl_h, port, stream_id);

			break;

		case 'd':
			if (talker)
				rc = msrp_talker_deregister(ctrl_h, port, stream_id);
			else
				rc = msrp_listener_deregister(ctrl_h, port, stream_id);

			break;
#if 0
		case 'c': {
			struct genavb_control_handle *handle;

			while (1) {
				rc = genavb_control_open(avb_h, &handle, GENAVB_CTRL_MSRP);
				if (rc < 0)
					printf("genavb_control_open() failed: %s\n", genavb_strerror(rc));

				msrp_listener_register(handle, port, stream_id);

				rc = genavb_control_close(handle);
				if (rc < 0)
					printf("genavb_control_close() failed: %s\n", genavb_strerror(rc));

			}
		}
		case 'e':

			while (1) {
				msrp_listener_register(ctrl_h, port, stream_id);
				msrp_listener_deregister(ctrl_h, port, stream_id);
			}

			break;
#endif
		case 'w':
			msrp_listen(ctrl_h, ctrl_fd);
			break;

		case 'h':
		default:
			usage();
			rc = -1;
			goto exit;
		}
	}

exit:
	genavb_control_close(ctrl_h);

error_control_open:
	genavb_exit(avb_h);

error_avb_init:
	return rc;
}
