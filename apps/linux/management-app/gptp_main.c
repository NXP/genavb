/*
 * Copyright 2018-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <genavb/genavb.h>
#include <genavb/helpers.h>
#include "common.h"

static char *module_name = "ptp";
static char *device_name;


static int dump_stats(struct genavb_control_handle *ctrl_h, unsigned int port)
{
	struct genavb_msg_managed_set_response get_response;
	uint16_t cmd[38];
	uint16_t rep[7];
	uint8_t *data;
	int i;
	uint16_t id, length, status, total_length;
	char name[][64] = {
		"rxSyncCount",
		"rxFollowUpCount",
		"rxPdelayRequestCount",
		"rxPdelayResponseCount",
		"rxPdelayResponseFollowUpCount",
		"rxAnnounceCount",
		"rxPTPPacketDiscardCount",
		"syncReceiptTimeoutCount",
		"announceReceiptTimeoutCount",
		"pdelayAllowedLostResponsesExceededCount",
		"txSyncCount",
		"txFollowUpCount",
		"txPdelayRequestCount",
		"txPdelayResponseCount",
		"txPdelayResponseFollowUpCount",
		"txAnnounceCount"
	};

	cmd[0] = 5; /* port_parameter_statistics */
	cmd[1] = 0;

	cmd[2] = 0; /* key index, port number */
	cmd[3] = 2;
	cmd[4] = port; /* key value */
	cmd[1] += 3 * 2;

	for (i = 0; i < 16; i++) {
		cmd[5 + 2 * i] = i + 1; /* stat index */
		cmd[5 + 2 * i + 1] = 0;
		cmd[1] += 2 * 2;
	}

	if (managed_get(ctrl_h, cmd, 4 + cmd[1], &get_response, sizeof(get_response)) < 0)
		goto err;

	rep[0] = 5; /* port_parameter_statistics */
	rep[1] = 0xaa;
	rep[2] = 0; /* status Ok*/
	rep[3] = 0; /* key index, port number */
	rep[4] = 4;
	rep[5] = 0; /* status ok */
	rep[6] = port;

	if (check_response(rep, (u_int16_t *)&get_response, 7) < 0)
		goto err;

	/* table header (id, total length and status) */
	data = get_response_header((uint8_t *)&get_response, &id, &total_length, &status);

	/* entry key header (port) */
	data = get_node_header(data, &id, &length, &status);

	printf("%s %s port: %u stats\n", module_name, device_name, ((uint16_t *)data)[0]);

	data = get_node_next(data, length);

	for (i = 0; i < 16; i++) {
		data = get_node_header(data, &id, &length, &status);

		if (!status)
			printf("%-40s %8u\n", name[i], ((uint32_t *)data)[0]);

		data = get_node_next(data, length);
	}

	return 0;

err:
	printf("%s %s port: %u stats, error\n", module_name, device_name, port);

	return -1;
}

static int set_port_state(struct genavb_control_handle *ctrl_h, unsigned int port, unsigned int enable)
{
	struct genavb_msg_managed_set_response set_response;
	uint16_t cmd[8];
	uint16_t rep[10];

	cmd[0] = 4; /* port_parameter_data_set */
	cmd[1] = 11;
	cmd[2] = 0; /* key index, port number */
	cmd[3] = 2;
	cmd[4] = port; /* key value */
	cmd[5] = 3; /* pttPortEnabled */
	cmd[6] = 1;
	cmd[7] = enable;

	if (managed_set(ctrl_h, cmd, 4 + cmd[1], &set_response, sizeof(set_response)) < 0)
		goto err;

	rep[0] = 4; /* port_parameter_data_set */
	rep[1] = 16;
	rep[2] = 0; /* status Ok*/
	rep[3] = 0; /* key index, port number */
	rep[4] = 4;
	rep[5] = 0; /* status ok */
	rep[6] = port;
	rep[7] = 3; /* pttPortEnabled */
	rep[8] = 2;
	rep[9] = 0; /* status ok */

	if (check_response(rep, (u_int16_t *)&set_response, 10) < 0)
		goto err;

	printf("%s %s set port: %u, state: %u, success\n", module_name, device_name, port, enable);

	return 0;

err:
	printf("%s %s set port: %u, state: %u, error\n", module_name, device_name, port, enable);

	return -1;
}

static int get_port_state(struct genavb_control_handle *ctrl_h, unsigned int port)
{
	struct genavb_msg_managed_get_response get_response;
	uint16_t cmd[8];
	uint16_t rep[10];

	cmd[0] = 4; /* port_parameter_data_set */
	cmd[1] = 11;
	cmd[2] = 0; /* key index, port number */
	cmd[3] = 2;
	cmd[4] = port; /* key value */
	cmd[5] = 3; /* pttPortEnabled */
	cmd[6] = 0;

	if (managed_get(ctrl_h, cmd, 4 + cmd[1], &get_response, sizeof(get_response)) < 0)
		goto err;

	rep[0] = 4; /* port_parameter_data_set */
	rep[1] = 17;
	rep[2] = 0; /* status Ok*/
	rep[3] = 0; /* key index, port number */
	rep[4] = 4;
	rep[5] = 0; /* status ok */
	rep[6] = port;
	rep[7] = 3; /* pttPortEnabled */
	rep[8] = 3;
	rep[9] = 0; /* status ok */

	if (check_response(rep, (u_int16_t *)&get_response, 10) < 0)
		goto err;

	printf("%s %s get port: %u, state: %u\n", module_name, device_name, port, get_response.data[10 * 2]);

	return 0;

err:
	printf("%s %s get port: %u, state: error\n", module_name, device_name, port);

	return -1;
}


static int set_priority1(struct genavb_control_handle *ctrl_h, unsigned int priority1)
{
	struct genavb_msg_managed_set_response set_response;
	uint16_t cmd[5];
	uint16_t rep[6];

	cmd[0] = 0; /* default_parameter_data_set */
	cmd[1] = 5;
	cmd[2] = 5; /* priority 1 */
	cmd[3] = 1;
	cmd[4] = priority1;

	if (managed_set(ctrl_h, cmd, 4 + cmd[1], &set_response, sizeof(set_response)) < 0)
		goto err;

	rep[0] = 0; /* default_parameter_data_set */
	rep[1] = 8;
	rep[2] = 0; /* status Ok */
	rep[3] = 5; /* priority 1 */
	rep[4] = 2;
	rep[5] = 0; /* status Ok */

	if (check_response(rep, (u_int16_t *)&set_response, 6) < 0)
		goto err;

	printf("%s %s set priority1: %u, success\n", module_name, device_name, priority1);

	return 0;

err:
	printf("%s %s set priority1: %u, error\n", module_name, device_name, priority1);

	return -1;
}

static int get_priority1(struct genavb_control_handle *ctrl_h)
{
	struct genavb_msg_managed_get_response get_response;
	uint16_t cmd[4];
	uint16_t rep[6];

	cmd[0] = 0; /* default_parameter_data_set */
	cmd[1] = 4;
	cmd[2] = 5; /* priority 1 */
	cmd[3] = 0;

	if (managed_get(ctrl_h, cmd, 4 + cmd[1], &get_response, sizeof(get_response)) < 0)
		goto err;

	rep[0] = 0; /* default_parameter_data_set */
	rep[1] = 9;
	rep[2] = 0; /* status Ok */
	rep[3] = 5; /* priority 1 */
	rep[4] = 3;
	rep[5] = 0; /* status Ok */

	if (check_response(rep, (u_int16_t *)&get_response, 6) < 0)
		goto err;

	printf("%s %s get priority1: %u\n", module_name, device_name, ((uint8_t *)&get_response)[12]);

	return 0;

err:
	printf("%s %s get priority1: error\n", module_name, device_name);

	return -1;
}

int gptp_main(struct genavb_handle *avb_h, int argc, char *argv[])
{
	struct genavb_control_handle *ctrl_h;
	struct genavb_control_handle *endpoint_ctrl_h;
	struct genavb_control_handle *bridge_ctrl_h;
	unsigned int priority1;
	unsigned int port;
	unsigned int set;
	unsigned int state;
	int option;
	int rc;
	unsigned long optval_ul;

	rc = genavb_control_open(avb_h, &endpoint_ctrl_h, GENAVB_CTRL_GPTP);
	if (rc != GENAVB_SUCCESS)
		endpoint_ctrl_h = NULL;

	rc = genavb_control_open(avb_h, &bridge_ctrl_h, GENAVB_CTRL_GPTP_BRIDGE);
	if (rc != GENAVB_SUCCESS)
		bridge_ctrl_h = NULL;

	if ((endpoint_ctrl_h == NULL) && (bridge_ctrl_h == NULL))
		goto err_control_open;

	/* default options */
	ctrl_h = endpoint_ctrl_h;
	device_name = "endpoint";
	port = 0;
	set = 0;

	while ((option = getopt(argc, argv, "EBGSP:psdh")) != -1) {
		/* common options */
		switch (option) {
		case 'E':
			ctrl_h = endpoint_ctrl_h;
			device_name = "endpoint";
			break;

		case 'B':
			ctrl_h = bridge_ctrl_h;
			device_name = "bridge";
			break;

		case 'G':
			set = 0;
			break;

		case 'S':
			set = 1;
			break;

		case 'P':
			if (h_strtoul(&optval_ul, optarg, NULL, 0) < 0) {
				usage();
				rc = -1;
				goto exit;
			}
			port = (unsigned int)optval_ul;
			break;

		case 'p':
			if (set) {
				if (!argv[optind] || argv[optind][0] == '-' || (h_strtoul(&optval_ul, argv[optind], NULL, 0) < 0)) {
					usage();
					rc = -1;
					goto exit;
				}
				priority1 = (unsigned int)optval_ul;
				rc = set_priority1(ctrl_h, priority1);
			} else {
				rc = get_priority1(ctrl_h);
			}
			break;

		case 's':
			if (set) {
				if (!argv[optind] || argv[optind][0] == '-' || (h_strtoul(&optval_ul, argv[optind], NULL, 0) < 0)) {
					usage();
					rc = -1;
					goto exit;
				}

				state = (unsigned int)optval_ul;
				rc = set_port_state(ctrl_h, port, state);
			} else {
				rc = get_port_state(ctrl_h, port);
			}
			break;

		case 'd':
			rc = dump_stats(ctrl_h, port);
			break;

		case 'h':
		default:
			usage();
			rc = -1;
			goto exit;
		}
	}

exit:
	if (endpoint_ctrl_h)
		genavb_control_close(endpoint_ctrl_h);

	if (bridge_ctrl_h)
		genavb_control_close(bridge_ctrl_h);

err_control_open:
	return rc;
}
