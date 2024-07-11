/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2020, 2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>

#include "udpcc2.h"

#include "genavb/helpers.h"

static void usage (void)
{
	printf("\nUsage:\nsalsacamctrl [options]\n");
	printf("\nOptions:\n"
		"\t-i <ipaddr>		IP address of the camera to control\n"
		"\t-p <port>		UDP control port to connect to (default port will be used if none is specified)\n"
		"\n"
		"\t-R			Reboot camera\n"
		"\t-S			Start camera\n"
		"\t-K			Stop camera\n"
		"\n"
		"\t-I <IPv4 address>	Set camera IPv4 address, in the form a.b.c.d. DANGEROUS (an incorrect value may render the camera unreachable). Only RFC1918 unicast addresses are allowed.\n"
		"\t-M <MAC address>	Set camera MAC address, in the form uu:vv:ww:xx:yy:zz. DANGEROUS. \n"
		"\t-B <rate>		Set camera stream bandwidth\n"
		"\t-A <AVB stream ID>	Set camera AVB stream ID\n"
		"\t-T <stream type>	Set stream type:\n"
		"\t				-T 1 for AVB\n"
		"\t				-T 0 for UDP\n"
		"\t-Y <phy>		Set PHY:\n"
		"\t				-Y 0 for 100BaseT\n"
		"\t				-Y 1 for BroadR-Reach\n"
		"\t-V E:xxxx		Set camera VLAN configuration for AVB streams:\n"
		"\t				E = 0 to disable VLANS, E = 1 to enable VLANs\n"
		"\t				xxxx: VLAN ID to configure\n"
		"\t-Z <auto-start mode>	Set auto-start:\n"
		"\t				-Z 1 to start streaming on boot\n"
		"\t				-Z 0 to wait for start command\n"
		"\n"
		"\t-s			Get current camera state\n"
		"\t-m			Get current camera MAC address\n"
		"\t-b			Get current camera bandwidth\n"
		"\t-a			Get current camera AVB stream ID\n"
		"\t-t			Get current stream type\n"
		"\t-y			Get current camera interface\n"
		"\t-v			Get current VLAN ID\n"
		"\t-z			Get auto-start setting\n"
		"\t-h			This help message\n"
		"Note: All changes resulting from Set commands are effective after reboot.\n");
}


int main(int argc, char *argv[])
{
	int option;
	int rc = 0;
	char ipaddr[32] = UDPCC2_DEFAULT_IPADDR;
	unsigned int port = UDPCC2_DEFAULT_PORT;
	unsigned long optval;

	setlinebuf(stdout);

	printf("NXP's Salsa camera control application\n");


	while ((option = getopt(argc, argv,"i:p:RSKB:A:I:M:T:Y:V:Z:sbamtyvzh")) != -1) {

		switch (option) {
		case 'i':
			h_strncpy(ipaddr, optarg, 32);
			break;

		case 'p':
			if (h_strtoul(&optval, optarg, NULL, 0) < 0) {
				printf("port(%s) is not a valid integer\n", optarg);
				goto exit;
			}
			port = (unsigned int)optval;
			if (port > 65535)  {
				printf("Invalid port(%s)\n", optarg);
				port = UDPCC2_DEFAULT_PORT;
			}
			break;

		case 'R':
			rc = udpcc2_camera_reboot(ipaddr, port);
			if (rc == 1)
				printf("Camera %s rebooting\n", ipaddr);
			else
				printf("Configuration error\n");
			break;

		case 'S':
			rc = udpcc2_camera_start(ipaddr, port);
			if (rc == 1)
				printf("Camera %s started\n", ipaddr);
			else
				printf("Configuration error\n");
			break;

		case 'K':
			rc = udpcc2_camera_stop(ipaddr, port);
			if (rc == 1)
				printf("Camera %s stopped\n", ipaddr);
			else
				printf("Configuration error\n");
			break;

		case 'B':
		{
			int rate;
			if (h_strtoul(&optval, optarg, NULL, 0) < 0) {
				printf("bandwidth(%s) is not a valid integer\n", optarg);
				goto exit;
			}
			rate = (int)optval;
			if (rate > 100000)  {
				printf("Invalid bandwidth(%s)\n", optarg);
			} else {
				rc = udpcc2_camera_set_rate(ipaddr, port, rate);
				if (rc == 1)
					printf("Camera %s rate configured to %d\n", ipaddr, rate);
				else
					printf("Configuration error\n");
			}
			break;
		}

		case 'A':
		{
			unsigned long long stream_id;
			if (h_strtoull(&stream_id, optarg, NULL, 0) < 0) {
				printf("stream ID(%s) is not a valid integer\n", optarg);
				goto exit;
			}
			rc = udpcc2_camera_set_avb_stream_id(ipaddr, port, stream_id);
			if (rc == 1)
				printf("Camera %s AVB stream ID configured to 0x%llx\n", ipaddr, stream_id);
			else
				printf("Configuration error\n");

			break;
		}

		case 'I':
		{
			unsigned int control_ipaddr;
			unsigned char *control_ipaddr_p = (unsigned char *)&control_ipaddr;
			rc = sscanf(optarg, "%hhu.%hhu.%hhu.%hhu", control_ipaddr_p, control_ipaddr_p + 1, control_ipaddr_p + 2, control_ipaddr_p + 3);
			if (rc < 4)
				printf("Invalid control IP address(%s)\n", optarg);
			else {
				rc = udpcc2_camera_set_control_ipaddr(ipaddr, port, control_ipaddr);
				if (rc == 1)
					printf("Camera %s IP address configured to %s\n", ipaddr, optarg);
				else
					printf("Configuration error\n");
			}
			break;
		}

		case 'M':
		{
			unsigned long long mac_addr;
			unsigned char *mac_addr_p = (unsigned char *)&mac_addr;
			rc = sscanf(optarg, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", mac_addr_p + 2, mac_addr_p + 3, mac_addr_p + 4, mac_addr_p + 5, mac_addr_p + 6, mac_addr_p + 7);
			if (rc < 6)
				printf("Invalid MAC address(%s)\n", optarg);
			else {
				rc = udpcc2_camera_set_mac_addr(ipaddr, port, mac_addr);
				if (rc == 1)
					printf("Camera %s MAC address configured to %s\n", ipaddr, optarg);
				else
					printf("Configuration error\n");
			}
			break;
		}

		case 'T':
		{
			int type;
			if (h_strtoul(&optval, optarg, NULL, 0) < 0) {
				printf("stream type(%s) is not a valid integer\n", optarg);
				goto exit;
			}
			type = (int)optval;
			if ((type != 0) && (type != 1))  {
				printf("Invalid stream type(%s)\n", optarg);
			} else {
				rc = udpcc2_camera_set_stream_type(ipaddr, port, type);
				if (rc == 1)
					printf("Camera %s stream type configured to %s\n", ipaddr, (type == 1) ? "AVB" : "UDP");
				else
					printf("Configuration error\n");
			}
			break;
		}

		case 'Y':
		{
			int phy;
			if (h_strtoul(&optval, optarg, NULL, 0) < 0) {
				printf("PHY(%s) is not a valid integer\n", optarg);
				goto exit;
			}
			phy = (int)optval;
			if ((phy != 0) && (phy != 1))  {
				printf("Invalid PHY(%s)\n", optarg);
			} else {
				rc = udpcc2_camera_set_phy(ipaddr, port, phy);
				if (rc == 1)
					printf("Camera %s PHY configured to %s\n", ipaddr, (phy == 1) ? "BroadRReach" : "100BaseT");
				else
					printf("Configuration error\n");
			}
			break;
		}

		case 'V':
		{
			unsigned char enable;
			unsigned short vlan_id;
			rc = sscanf(optarg, "%hhu:%hu", &enable, &vlan_id);
			if (rc < 2)
				printf("Invalid VLAN configuration(%s)\n", optarg);
			else {
				rc = udpcc2_camera_set_avb_vlan(ipaddr, port, enable, vlan_id);
				if (rc == 1)
					printf("Camera %s VLAN configured to: enable: %d id: %u\n", ipaddr, enable, vlan_id);
				else
					printf("Configuration error\n");
			}
			break;
		}

		case 'Z':
		{
			int autostart;
			if (h_strtoul(&optval, optarg, NULL, 0) < 0) {
				printf("auto-start(%s) is not a valid integer\n", optarg);
				goto exit;
			}
			autostart = (int)optval;
			if ((autostart != 0) && (autostart != 1))  {
				printf("Invalid auto-start value(%s)\n", optarg);
			} else {
				rc = udpcc2_camera_set_autostart(ipaddr, port, autostart);
				if (rc == 1)
					printf("Camera %s auto-start configured to %d\n", ipaddr, autostart);
				else
					printf("Configuration error\n");
			}
			break;
		}


		case 's':
			rc = udpcc2_camera_get_state(ipaddr, port);
			if (rc >= 0)
				printf("Camera %s state: %d\n", ipaddr, rc);
			else
				printf("Request error\n");
			break;

		case 'b':
			rc = udpcc2_camera_get_rate(ipaddr, port);
			if (rc >= 0)
				printf("Camera %s rate: %d\n", ipaddr, rc);
			else
				printf("Request error\n");
			break;

		case 'a':
		{
			unsigned long long stream_id = udpcc2_camera_get_avb_stream_id(ipaddr, port);
			if (stream_id)
				printf("Camera %s AVB stream ID: 0x%llx\n", ipaddr, stream_id);
			else
				printf("Request error\n");
			break;
		}

		case 'm':
		{
			unsigned long long mac_addr = udpcc2_camera_get_mac_addr(ipaddr, port);
			unsigned char *mac_addr_p = (unsigned char *)&mac_addr;
			if (mac_addr)
				printf("Camera %s MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n", ipaddr,
						mac_addr_p[2], mac_addr_p[3], mac_addr_p[4], mac_addr_p[5], mac_addr_p[6], mac_addr_p[7]);
			else
				printf("Request error\n");
			break;
		}

		case 't':
			rc = udpcc2_camera_get_stream_type(ipaddr, port);
			if (rc >= 0)
				printf("Camera %s stream type: %s\n", ipaddr, (rc == 1) ? "AVB" : "UDP");
			else
				printf("Request error\n");
			break;

		case 'y':
			rc = udpcc2_camera_get_phy(ipaddr, port);
			if (rc >= 0)
				printf("Camera %s PHY: %s\n", ipaddr, (rc == 1) ? "BroadRReach" : "100BaseT");
			else
				printf("Request error\n");
			break;

		case 'v':
		{
			unsigned short vlan_id;
			rc = udpcc2_camera_get_avb_vlan(ipaddr, port, &vlan_id);
			if (rc >= 0)
				printf("Camera %s VLAN enabled: %d id: %d\n", ipaddr, rc, vlan_id);
			else
				printf("Request error\n");

			break;
		}
		case 'z':
			rc = udpcc2_camera_get_autostart(ipaddr, port);
			if (rc >= 0)
				printf("Camera %s auto-start: %d\n", ipaddr, rc);
			else
				printf("Request error\n");
			break;


		case 'h':
		default:
			usage();
			rc = -1;
			goto exit;
		}
	}


exit:
	return rc;

}
