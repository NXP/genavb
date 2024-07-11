/*
 * Copyright 2020-2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <genavb/genavb.h>

//#define DEBUG

void usage (void)
{
	printf("\nUsage:\nmanagement-app [ptp|srp] [options]\n");
	printf("\nOptions:\n"
		"\nCommon options:\n"
		"\t-E                    manages endpoint stack (default)\n"
		"\t-B                    manages bridge stack\n"
		"\t-S                    sets managed object\n"
		"\t-G                    gets managed object (default)\n"
		"\t-P <port>             manages port <port> (default 0)\n"
		"\t-D <direction>        manages direction <direction> (default 0: talker, 1: listener)\n"
		"\t-h                    print this help text\n"
		"\nPTP options:\n"
		"\t-p <priority1>        set/get gPTP priority1 value\n"
		"\t-s <state>            set/get gPTP port state\n"
		"\t-d                    dump port stats\n"
		"\nSRP options:\n"
		"\t-M <status>           set/get msrpEnabledStatus (0: disabled, 1: enabled)\n"
		"\t-e <status>           set/get msrpPortEnabledStatus (0: disabled, 1: enabled)\n"
		"\t-s <stream_id>        get streams table (default wilcard if <streamID> omitted)\n"
		"\t-r <stream_id>        get reservations table per <port> <direction> <stream_id> (default wilcard if <stream_id> omitted)\n"
		"\t-b                    get bridge base table\n"
		"\t-p                    get bridge port table\n"
		"\t-l <traffic_class>    get latency parameter table\n");
}

int managed_set(struct genavb_control_handle *ctrl_h, void *cmd, unsigned int cmd_len, void *response, unsigned int response_len)
{
	unsigned int msg_type;
#ifdef DEBUG
	int i;
#endif
	int rc;

	if (!ctrl_h)
		goto err;

	msg_type = GENAVB_MSG_MANAGED_SET;
	rc = genavb_control_send_sync(ctrl_h, &msg_type, cmd, cmd_len, response, &response_len, 100);
	if (rc != GENAVB_SUCCESS) {
		printf("genavb_control_send_sync() failed: %s\n", genavb_strerror(rc));
		goto err;
	}

	if (msg_type != GENAVB_MSG_MANAGED_SET_RESPONSE) {
		printf("genavb_control_send_sync() wrong response message type: %d\n", msg_type);
		goto err;
	}

#ifdef DEBUG
	printf("%d", response_len);

	for (i = 0; i < response_len; i++)
		printf(" %1x", ((uint8_t *)response)[i]);

	printf("\n");
#endif

	return 0;

err:
	return -1;
}

int managed_get(struct genavb_control_handle *ctrl_h, void *cmd, unsigned int cmd_len, void *response, unsigned int response_len)
{
	unsigned int msg_type;
#ifdef DEBUG
	int i;
#endif
	int rc;

	if (!ctrl_h)
		goto err;

	msg_type = GENAVB_MSG_MANAGED_GET;
	rc = genavb_control_send_sync(ctrl_h, &msg_type, cmd, cmd_len, response, &response_len, 100);
	if (rc != GENAVB_SUCCESS) {
		printf("genavb_control_send_sync() failed: %s\n", genavb_strerror(rc));
		goto err;
	}

	if (msg_type != GENAVB_MSG_MANAGED_GET_RESPONSE) {
		printf("genavb_control_send_sync() wrong response message type: %d\n", msg_type);
		goto err;
	}

#ifdef DEBUG
	printf("%d", response_len);

	for (i = 0; i < response_len; i++)
		printf(" %1x", ((uint8_t *)response)[i]);

	printf("\n");
#endif

	return 0;

err:
	return -1;
}

int check_response(uint16_t *expected, uint16_t *response, unsigned int len)
{
	int i;

	for (i = 0; i < len; i++)
		if (expected[i] != response[i]) {
			return -1;
		}

	return 0;
}

uint8_t *get_response_header(uint8_t *buf, uint16_t *id, uint16_t *length, uint16_t *status)
{
	*id = ((uint16_t *)buf)[0];
	*length = ((uint16_t *)buf)[1];
	*status = ((uint16_t *)buf)[2];

	buf += 6;

	return buf;
}

uint8_t *get_node_next(uint8_t *buf, uint16_t length)
{
	buf += length - 2;

	return buf;
}

uint8_t *get_node_header(uint8_t *buf, uint16_t *id, uint16_t *length, uint16_t *status)
{
	buf = get_response_header(buf, id, length, status);

	return buf;
}
