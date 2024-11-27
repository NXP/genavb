/*
 * Copyright 2021-2022 NXP
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
#include <genavb/genavb.h>
#include <genavb/helpers.h>

#define MAAP_CONTROL_TIMEOUT			1 /* 1 sec */

#define DEFAULT_PORT			0
#define DEFAULT_RANGE_ID			0
#define DEFAULT_MAC_ADDR		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00}

static const char *maap_status_str[] = {
	[MAAP_STATUS_SUCCESS] = "MAAP_SUCCESS",
	[MAAP_STATUS_FREE] = "MAAP_FREE",
	[MAAP_STATUS_CONFLICT] = "MAAP_CONFLICT",
	[MAAP_STATUS_ERROR] = "MAAP_ERROR"
};

static const char *maap_response_str[] = {
	[MAAP_RESPONSE_SUCCESS] = "MAAP_RESPONSE_SUCCESS",
	[MAAP_RESPONSE_ERROR] = "MAAP_RESPONSE_ERROR"
};

void usage (void)
{
	printf("\nUsage:\nmaap-ctrl-app [options]\n");
	printf("\nOptions:\n"
		"\nCommon options:\n"
		"\t-n                         Create new range, takes previously set configuration (-p -i -c -f -a) in the command line, otherwise default ones. -a and -f should be used together\n"
		"\t-d                         Delete a range, takes previously set configuration (-p -i) in the command line, otherwise default ones\n"
		"\t-i                         ID of the range to create or to delete (default = 0)\n"
		"\t-f <int>                   Flag, set it to 1 if you want to use a prefered first mac address for the new range (default = 0)\n"
		"\t-a <first MAC address>     First prefered mac address of the new range, check IEEE1722-2016 for the standard ranges (default = 00:00:00:00:00:00)\n"
		"\t-c <int>                   Number of mac address to allocate in the range, max = 65024 (default = 1)\n"
		"\t-p <port>                  (Logical) Port where the range should be allocated/deleted (default = 0)\n"
		"\t-h                         Print this help text\n"
		"\t-l                         Listen to status messages from MAAP\n"
		"\nRange creation:\n"
		"\t-p 0 -i 1 -c 2 -f 1 -a 91:e0:f0:00:fd:49 -n\n"
		"\t-p 0 -i 2 -c 2 -n\n"
		"\t-p 0 -i 3 -n\n"
		"\nMultiple range creation:\n"
		"\t-p 0 -i 1 -c 2 -f 1 -a 91:e0:f0:00:fd:49 -n -i 2 -c 3 -a 91:e0:f0:00:f0:49 -n\n"
		"\t-p 0 -i 3 -n -i 4 -n\n"
		"\nRange deletion:\n"
		"\t-p 0 -i 1 -d\n"
		"\nMultiple range deletion:\n"
		"\t-p 0 -i 1 -d -i 2 -d\n");
}

static int new_range(struct genavb_control_handle *ctrl_h, uint16_t port, uint32_t range_id, bool flag, uint8_t *addr, uint16_t count)
{
	struct genavb_msg_maap_create command_msg;
	unsigned int msg_type;
	struct genavb_msg_maap_create_response response_msg;
	unsigned int msg_len;
	int rc;

	command_msg.flag = flag;
	command_msg.port_id = port;
	command_msg.range_id = range_id;
	command_msg.count = count;

	if (!flag) {
		memset(command_msg.base_address, 0, sizeof(uint8_t) * 6);
	} else {
		memcpy(command_msg.base_address, addr, sizeof(uint8_t) * 6);
	}

	msg_type = GENAVB_MSG_MAAP_CREATE_RANGE;
	msg_len = sizeof(response_msg);
	rc = genavb_control_send_sync(ctrl_h, &msg_type, &command_msg, sizeof(command_msg), &response_msg, &msg_len, MAAP_CONTROL_TIMEOUT);

	if (rc < 0) {
		printf("genavb_control_send_sync() failed: %s\n", genavb_strerror(rc));
		goto err;

	} else if (msg_type != GENAVB_MSG_MAAP_CREATE_RANGE_RESPONSE) {
		printf("response type error: %d\n", msg_type);
		goto err;

	} else if (response_msg.status != MAAP_RESPONSE_SUCCESS) {
		printf("response status error: %s\n", maap_response_str[response_msg.status]);
		goto err;
	}

	printf ("(%u) CREATED ID : %u, first MAC address: %02x-%02x-%02x-%02x-%02x-%02x, number of addresses: %u, port: %u, status: %u, %s\n",
		getpid(),
		response_msg.range_id, response_msg.base_address[0], response_msg.base_address[1], response_msg.base_address[2],
		response_msg.base_address[3], response_msg.base_address[4], response_msg.base_address[5], response_msg.count,
		response_msg.port_id, response_msg.status, maap_response_str[response_msg.status]);

	return 0;

err:
	return -1;
}

static int delete_range(struct genavb_control_handle *ctrl_h, uint16_t port, uint32_t range_id)
{
	struct genavb_msg_maap_delete command_msg;
	unsigned int msg_type;
	struct genavb_msg_maap_delete_response response_msg;
	unsigned int msg_len;
	int rc;

	command_msg.port_id = port;
	command_msg.range_id = range_id;

	msg_type = GENAVB_MSG_MAAP_DELETE_RANGE;
	msg_len = sizeof(response_msg);
	rc = genavb_control_send_sync(ctrl_h, &msg_type, &command_msg, sizeof(command_msg), &response_msg, &msg_len, MAAP_CONTROL_TIMEOUT);

	if (rc < 0) {
		printf("genavb_control_send_sync() failed: %s\n", genavb_strerror(rc));
		goto err;

	} else if (msg_type != GENAVB_MSG_MAAP_DELETE_RANGE_RESPONSE) {
		printf("response type error: %d\n", msg_type);
		goto err;

	} else if (response_msg.status != MAAP_RESPONSE_SUCCESS) {
		printf("response status error: %s\n", maap_response_str[response_msg.status]);
		goto err;
	}

	printf ("(%u) DELETED ID : %u, port: %u, status: %u, %s\n",
		getpid(),
		response_msg.range_id, response_msg.port_id, response_msg.status, maap_response_str[response_msg.status]);

	return 0;

err:
	return -1;
}

static void maap_listen(struct genavb_control_handle *ctrl_h, int ctrl_fd)
{
	unsigned int msg_type;
	struct genavb_maap_status msg;
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
		case GENAVB_MSG_MAAP_CREATE_RANGE_RESPONSE:
		case GENAVB_MSG_MAAP_DELETE_RANGE_RESPONSE:
			break;

		case GENAVB_MSG_MAAP_STATUS:
			printf ("(%u) ID : %u, first MAC address: %02x-%02x-%02x-%02x-%02x-%02x, number of addresses: %u, port: %u, status: %u, %s\n",
				getpid(),
				msg.range_id, msg.base_address[0], msg.base_address[1], msg.base_address[2],
				msg.base_address[3], msg.base_address[4], msg.base_address[5], msg.count,
				msg.port_id, msg.status, maap_status_str[msg.status]);
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
	unsigned long range_id = DEFAULT_RANGE_ID;
	unsigned long flag = 0;
	unsigned long count = 1;
	uint8_t mac_addr[6] = DEFAULT_MAC_ADDR;
	int ctrl_fd;
	int option;
	int rc = 0;

	setlinebuf(stdout);

	printf("NXP's GenAVB MAAP control application\n");

	/*
	* setup the avb stack
	*/

	rc = genavb_init(&avb_h, 0);
	if (rc != GENAVB_SUCCESS) {
		printf("genavb_init() failed: %s\n", genavb_strerror(rc));
		rc = -1;
		goto error_avb_init;
	}

	rc = genavb_control_open(avb_h, &ctrl_h, GENAVB_CTRL_MAAP);
	if (rc != GENAVB_SUCCESS) {
		printf("genavb_control_open() failed: %s\n", genavb_strerror(rc));
		rc = -1;
		goto error_control_open;
	}

	ctrl_fd = genavb_control_rx_fd(ctrl_h);

	/*
	* retrieve user's configuration parameters
	*/
	while ((option = getopt(argc, argv,"f:a:c:p:i:hlnd")) != -1) {
		switch (option) {
		case 'f':
			if (h_strtoul(&flag, optarg, NULL, 0) < 0) {
				printf("invalid -f %s option\n", optarg);
				rc = -1;
				goto exit;
			}

			break;

		case 'a':
			if (sscanf(optarg, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac_addr[0], &mac_addr[1], &mac_addr[2], &mac_addr[3], &mac_addr[4], &mac_addr[5]) < 6) {
				printf("invalid -a %s option\n", optarg);
				rc = -1;
				goto exit;
			}

			break;

		case 'c':
			if (h_strtoul(&count, optarg, NULL, 0) < 0) {
				printf("invalid -c %s option\n", optarg);
				rc = -1;
				goto exit;
			}

			break;

		case 'p':
			if (h_strtoul(&port, optarg, NULL, 0) < 0) {
				printf("invalid -p %s option\n", optarg);
				rc = -1;
				goto exit;
			}

			break;

		case 'i':
			if (h_strtoul(&range_id, optarg, NULL, 0) < 0) {
				printf("invalid -i %s option\n", optarg);
				rc = -1;
				goto exit;
			}

			break;

		case 'l':
			maap_listen(ctrl_h, ctrl_fd);
			break;

		case 'n':
			rc = new_range(ctrl_h, port, range_id, flag, mac_addr, count);
			break;

		case 'd':
			rc = delete_range(ctrl_h, port, range_id);
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
