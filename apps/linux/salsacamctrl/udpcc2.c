/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>

#include "udpcc2.h"

#define RESPONSE_TIMEOUT 1

static unsigned char sequence = 0;

#define htonll(x) (((unsigned long long)htonl((unsigned int)x))<<32 | htonl((unsigned int)((unsigned long long)x>>32)))
#define ntohll(x) (((unsigned long long)ntohl((unsigned int)x))<<32 | ntohl((unsigned int)((unsigned long long)x>>32)))



int udpcc2_send_msg(const char *ipaddr, unsigned int port, struct udpcc2_message *cmd, struct udpcc2_message *resp, unsigned int resplen)
{
	int sock, rc, i, msglen;
	struct sockaddr_in camera_addr;
	unsigned char csum;
	fd_set readfs;
	struct timeval timeout;

	sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock == -1) {
		printf("%s: Could not create socket: errno %d(%s)\n", __func__, errno, strerror(errno));
		return -1;
	}

	memset(&camera_addr, 0, sizeof(camera_addr));
	camera_addr.sin_family = AF_INET;
	camera_addr.sin_port = htons(port);
	rc = inet_pton(AF_INET, ipaddr, &camera_addr.sin_addr.s_addr);
	if (rc <= 0) {
		printf("%s: Invalid IP address (%s)\n", __func__, ipaddr);
		goto err;
	}

	csum = 0;
	msglen = sizeof(struct udpcc2_message) + cmd->payload_length;
	for (i = 0; i < msglen; i++) {
		csum += ((unsigned char *)cmd)[i];
	}
	csum = ~csum  + 1;

	cmd->configuration_option = htons(cmd->configuration_option);
	cmd->payload_length = htons(cmd->payload_length);
	((unsigned char *)cmd)[msglen] = csum;
	msglen++;

	rc = sendto(sock, cmd, msglen, 0, (struct sockaddr *)&camera_addr, sizeof(camera_addr));
	if (rc <= 0) {
		printf("%s: Coudln't send UDPCC2 message: errno %d(%s)\n", __func__, errno, strerror(errno));
		goto err;
	}

	if (resp) {
		FD_ZERO(&readfs);
		FD_SET(sock, &readfs);
		timeout.tv_sec = RESPONSE_TIMEOUT;
		timeout.tv_usec = 0;
		rc = select(sock + 1, &readfs, NULL, NULL, &timeout);
		if (rc <= 0) {
			printf("%s: Timeout waiting for UDPCC2 response: errno %d(%s)\n", __func__, errno, strerror(errno));
			goto err;
		}

		rc = recvfrom(sock, resp, resplen, 0, NULL,  NULL);

		if (rc <= 0) {
			printf("%s: Couldn't receive UDPCC2 message: errno %d(%s)\n", __func__, errno, strerror(errno));
			goto err;
		}

		if (resp->request_number != cmd->request_number) {
			printf("%s: Request number mismatch (cmd = %d, resp = %d)\n", __func__, cmd->request_number, resp->request_number);
			goto err;
		}

		//TODO verify checksum?
	}

	if (rc > 0)
		rc = 1;

	close(sock);
	return rc;

err:
	rc = -1;
	close(sock);
	return rc;
}

#define MAX_BUFFER_SIZE (sizeof(struct udpcc2_message) + 32)
int udpcc2_send_set_msg(const char *ipaddr, unsigned int port, udpcc2_configuration_option_t config_option, udpcc2_change_type_t change_type, void *payload, unsigned short payload_length)
{
	unsigned int buffer_size = sizeof(struct udpcc2_message) + payload_length + 1;
	unsigned char cmd_buffer[buffer_size];
	unsigned char resp_buffer[buffer_size];
	struct udpcc2_message *cmd = (struct udpcc2_message *)&cmd_buffer;
	struct udpcc2_message *resp = (struct udpcc2_message *)&resp_buffer;
	int rc = 0;

	cmd->type = UDPCC2_MSG_TYPE_SetMessage;
	cmd->code = UDPCC2_CODE_UNSET;
	cmd->configuration_option = config_option;
	cmd->change_type = change_type;
	cmd->request_number = sequence;
	cmd->payload_length = payload_length;
	if (payload)
		memcpy(cmd->payload, payload, payload_length);

	rc = udpcc2_send_msg(ipaddr, port, cmd, resp, buffer_size);
	sequence++;

	if (rc > 0) {
		if (resp->code != UDPCC2_CODE_ACK) {
			printf("%s: Received Error code %d\n", __func__, resp->code);
			rc = -1;
		}
		else
			rc = 1;
	}
	return rc;
}

int udpcc2_send_get_msg(const char *ipaddr, unsigned int port, udpcc2_configuration_option_t config_option, void *payload, unsigned short payload_length)
{
	unsigned char cmd_buffer[sizeof(struct udpcc2_message) + 1];
	unsigned char resp_buffer[sizeof(struct udpcc2_message) + payload_length + 1];
	struct udpcc2_message *cmd = (struct udpcc2_message *)&cmd_buffer;
	struct udpcc2_message *resp = (struct udpcc2_message *)&resp_buffer;
	int rc = 0;

	cmd->type = UDPCC2_MSG_TYPE_GetMessage;
	cmd->code = UDPCC2_CODE_UNSET;
	cmd->configuration_option = config_option;
	cmd->change_type = UDPCC2_CHANGE_IMMEDIATE;
	cmd->request_number = sequence;
	cmd->payload_length = 0;

	rc = udpcc2_send_msg(ipaddr, port, cmd, resp, sizeof(resp_buffer));
	sequence++;

	if (rc > 0) {
		if (resp->code != UDPCC2_CODE_ACK) {
			printf("%s: Received Error code %d\n", __func__, resp->code);
			rc = -1;
		}
		else {
			rc = 1;
			memcpy(payload, resp->payload, payload_length);
		}
	}
	return rc;
}


int udpcc2_camera_reboot(const char *ipaddr, unsigned int port)
{
	return udpcc2_send_set_msg(ipaddr, port, UDPCC2_OPT_REBOOT, UDPCC2_CHANGE_IMMEDIATE, NULL, 0);
}

int udpcc2_camera_start(const char *ipaddr, unsigned int port)
{
	return udpcc2_send_set_msg(ipaddr, port, UDPCC2_OPT_CAMERA_START, UDPCC2_CHANGE_IMMEDIATE, NULL, 0);
}

int udpcc2_camera_stop(const char *ipaddr, unsigned int port)
{
	unsigned char payload[4] = { 0, 0, 0, 1};
	return udpcc2_send_set_msg(ipaddr, port, UDPCC2_OPT_CAMERA_STOP, UDPCC2_CHANGE_IMMEDIATE, payload, 4);

}

int udpcc2_camera_get_state(const char *ipaddr, unsigned int port)
{
	int state;
	int rc = udpcc2_send_get_msg(ipaddr, port, UDPCC2_OPT_CAMERA_START, &state, 4);

	if (rc == 1)
		rc = ntohl(state);

	return rc;
}


int udpcc2_camera_set_rate(const char *ipaddr, unsigned int port, unsigned int rate)
{
	unsigned int payload = htonl(rate);
	return udpcc2_send_set_msg(ipaddr, port, UDPCC2_OPT_DATA_RATE, UDPCC2_CHANGE_PERSISTENT, &payload, 4);
}


int udpcc2_camera_get_rate(const char *ipaddr, unsigned int port)
{
	int rate;
	int rc = udpcc2_send_get_msg(ipaddr, port, UDPCC2_OPT_DATA_RATE, &rate, 4);

	if (rc == 1)
		rc = ntohl(rate);

	return rc;
}


int udpcc2_camera_set_avb_stream_id(const char *ipaddr, unsigned int port, unsigned long long stream_id)
{
	unsigned long long payload = htonll(stream_id);
	return udpcc2_send_set_msg(ipaddr, port, UDPCC2_OPT_AVB_STREAM_ID, UDPCC2_CHANGE_PERSISTENT, &payload, 8);
}

unsigned long long udpcc2_camera_get_avb_stream_id(const char *ipaddr, unsigned int port)
{
	unsigned long long stream_id;
	int rc = udpcc2_send_get_msg(ipaddr, port, UDPCC2_OPT_AVB_STREAM_ID, &stream_id, 8);

	if (rc == 1)
		return ntohll(stream_id);
	else
		return 0;
}


// Returns 1 if IP address belongs to a private network (RFC1918) and is a valid unicast address, 0 otherwise
static int is_ip_usable(const unsigned int ipaddr)
{
	unsigned char *a = (unsigned char *)&ipaddr;
	unsigned char *b = a + 1;
//	unsigned char *c = a + 2;
	unsigned char *d = a + 3;

	// Broadcast address
	if ((*d & 0xff) == 0xff)
		return 0;

	// Network address
	if ((*d & 0xff) == 0)
		return 0;

	// 10.0.0.0/8
	if (*a == 0x0a)
		return 1;

	// 172.16.0.0/12
	if ((*a == 0xac) && ((*b & 0xf0) == 0x10))
		return 1;

	// 192.168.0.0/16
	if ((*a == 0xc0) && (*b == 0xa8))
		return 1;

	return 0;
}


int udpcc2_camera_set_control_ipaddr(const char *ipaddr, unsigned int port, unsigned int control_ipaddr)
{
	if (is_ip_usable(control_ipaddr))
		return udpcc2_send_set_msg(ipaddr, port, UDPCC2_OPT_IP_ADDR, UDPCC2_CHANGE_PERSISTENT, &control_ipaddr, 4);
	else {
		printf("Invalid IP address\n");
		return -1;
	}
}


int udpcc2_camera_set_mac_addr(const char *ipaddr, unsigned int port, unsigned long long mac_addr)
{
	unsigned char *mac_addr_p = (unsigned char *)&mac_addr;
	return udpcc2_send_set_msg(ipaddr, port, UDPCC2_OPT_MAC_ADDR, UDPCC2_CHANGE_PERSISTENT, mac_addr_p + 2, 6);
}

unsigned long long udpcc2_camera_get_mac_addr(const char *ipaddr, unsigned int port)
{
	unsigned long long mac_addr;
	unsigned char *mac_addr_p = (unsigned char *)&mac_addr;
	int rc = udpcc2_send_get_msg(ipaddr, port, UDPCC2_OPT_MAC_ADDR, mac_addr_p + 2, 6);

	if (rc == 1)
		return mac_addr;
	else
		return 0;

}


int udpcc2_camera_set_stream_type(const char *ipaddr, unsigned int port, udpcc2_stream_type_t avb)
{
	unsigned char payload = (avb != UDPCC2_STREAM_UDP) ? 1 : 2;
	return udpcc2_send_set_msg(ipaddr, port, UDPCC2_OPT_STREAM_TYPE, UDPCC2_CHANGE_PERSISTENT, &payload, 1);
}

udpcc2_stream_type_t udpcc2_camera_get_stream_type(const char *ipaddr, unsigned int port)
{
	unsigned char type;

	int rc = udpcc2_send_get_msg(ipaddr, port, UDPCC2_OPT_STREAM_TYPE, &type, 1);

	if (rc == 1)
		rc = (type == 1) ? UDPCC2_STREAM_AVB : UDPCC2_STREAM_UDP;
	else
		rc = UDPCC2_STREAM_ERROR;


	return rc;
}


int udpcc2_camera_set_autostart(const char *ipaddr, unsigned int port, unsigned int autostart)
{
	unsigned char payload = (autostart != 0) ? 1 : 0;
	return udpcc2_send_set_msg(ipaddr, port, UDPCC2_OPT_AUTOSTART, UDPCC2_CHANGE_PERSISTENT, &payload, 1);
}

int udpcc2_camera_get_autostart(const char *ipaddr, unsigned int port)
{
	unsigned char autostart;

	int rc = udpcc2_send_get_msg(ipaddr, port, UDPCC2_OPT_AUTOSTART, &autostart, 1);

	if (rc == 1)
		rc = autostart;

	return rc;
}


int udpcc2_camera_set_avb_vlan(const char *ipaddr, unsigned int port, unsigned int enable, unsigned short vlan_id)
{
	unsigned int payload = 0;
	unsigned char *payload_p = (unsigned char *)&payload;

	*(unsigned short *)(payload_p + 2) = htons(vlan_id);
	if (enable != 0)
		*(payload_p + 1) = 0x01;
	return udpcc2_send_set_msg(ipaddr, port, UDPCC2_OPT_AVB_VLAN, UDPCC2_CHANGE_PERSISTENT, payload_p + 1, 3);
}



int udpcc2_camera_get_avb_vlan(const char *ipaddr, unsigned int port, unsigned short *vlan_id)
{
	int payload;
	int rc = udpcc2_send_get_msg(ipaddr, port, UDPCC2_OPT_AVB_VLAN, &payload, 3);

	if (rc == 1) {
		rc = payload & 0x1;
		if (vlan_id)
			*vlan_id = ntohs(payload >> 8);
	}

	return rc;
}


int udpcc2_camera_set_phy(const char *ipaddr, unsigned int port, udpcc2_phy_type_t phy)
{
	unsigned char payload = (phy != 0) ? 1 : 0;
	return udpcc2_send_set_msg(ipaddr, port, UDPCC2_OPT_PHY, UDPCC2_CHANGE_PERSISTENT, &payload, 1);
}

udpcc2_phy_type_t udpcc2_camera_get_phy(const char *ipaddr, unsigned int port)
{
	unsigned char phy;

	int rc = udpcc2_send_get_msg(ipaddr, port, UDPCC2_OPT_PHY, &phy, 1);

	if (rc == 1)
		rc = (phy == 1) ? UDPCC2_PHY_BRR : UDPCC2_PHY_100BASET;
	else
		rc = UDPCC2_PHY_ERROR;

	return rc;
}
