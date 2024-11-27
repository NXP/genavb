/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2020, 2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief GenAVB simple AVDECC controls handling demo application
 @details
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <inttypes.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <arpa/inet.h>

#include <genavb/genavb.h>
#include <genavb/aecp.h>
#include <genavb/aem.h>
#include <genavb/net_types.h>
#include <genavb/helpers.h>

#include "../common/adp.h"
#include "../common/acmp.h"
#include "../common/aecp.h"

#define MAX_UTF8_STRING_SIZE	(AEM_UTF8_MAX_LENGTH + 4) /* Add few characters after the stack limit for unit testing */

static void usage (void)
{
	printf("\nUsage:\napp [options]\n");
	printf("\nOptions:\n"
		"\t-S <control_type> <entity_id> <control_index> <value>	Set a given control to the given value where control_type \n"
		"								must be uint8 or utf8 (For utf8: <value> must be string of max %u characters)\n"
		"\t-G <control_type> <entity_id> <control_index>		Get a control value where control_type must be uint8 or utf8 \n"
		"\t-l                    					list discovered AVDECC entities\n"
		"\t-c <talker_entity_id> <talker_unique_id> <listener_entity_id> <listener_unique_id> <flags>	connect a stream between a talker and a listener\n"
		"\t-d <talker_entity_id> <talker_unique_id> <listener_entity_id> <listener_unique_id>		disconnect a stream between a talker and a listener\n"
		"\t-r <listener_entity_id> <listener_unique_id>		Get information about a listener sink\n"
		"\t-t <talker_entity_id> <talker_unique_id>		Get information about a talker source\n"
		"\t-s <talker_entity_id> <talker_unique_id> <index>	Get information from a talker about a given connection/stream\n"
		"\t-T <talker_entity_id> <talker_unique_id> <start|stop>		Send START_STREAMING or STOP_STREAMING command to a talker\n"
		"\t-L <listener_entity_id> <listener_unique_id> <start|stop>	Send START_STREAMING or STOP_STREAMING command to a listener\n"
		"\t-h                    						print this help text\n", MAX_UTF8_STRING_SIZE - 1);
}


#define FORMAT_STR_SIZE 128
void pretty_print_format(struct avdecc_format *format)
{
	char format_str[FORMAT_STR_SIZE];
	int rc;

	rc = avdecc_fmt_pretty_printf(format, format_str, FORMAT_STR_SIZE);
	if ((rc >= FORMAT_STR_SIZE) || rc < 0)
		printf("( format decoding error %d )\n", rc);
	else
		printf("( %s )\n", format_str);
}

/** Display information about the STREAM_INPUTs or STREAM_OUTPUTs of an entity.
 *
 * \param	ctrl_h					AVB control handle to use (must be for a AVB_CTRL_AVDECC_CONTROLLER channel)
 * \param	entity_id				ID of the entity to display information about.
 * \param 	configuration_index		Index of the entity configuration to use.
 * \param	descriptor_type			Type of descriptor to use. Must be either AEM_DESC_TYPE_STREAM_OUTPUT or AEM_DESC_TYPE_STREAM_INPUT.
 * \param	total					Number of stream inputs or outputs to display information about.
 *
 */
void display_stream_inputs_outputs(struct avb_control_handle *ctrl_h, avb_u64 *entity_id,  avb_u16 configuration_index, avb_u16 descriptor_type, avb_u16 total)
{
	struct stream_descriptor stream_desc;
	int i, rc;
	avb_u8 status;
	avb_u16 len = sizeof(struct stream_descriptor);
	avb_u64 format;

	for (i = 0; i < total; i++) {
			rc = aecp_aem_send_read_descriptor(ctrl_h, entity_id, configuration_index, descriptor_type, i, &stream_desc, &len, &status, 1);

			if ((rc == AVB_SUCCESS) && (status == AECP_AEM_SUCCESS)) {
				format = stream_desc.current_format;
				printf("        Stream  %2d: name = %20s    interface index = %u    number of formats = %d    "
						"flags = 0x%x    current_format = 0x%016" PRIx64 " ",
						i, stream_desc.object_name, ntohs(stream_desc.avb_interface_index), ntohs(stream_desc.number_of_formats),
						ntohs(stream_desc.stream_flags), ntohll(format));
				pretty_print_format((struct avdecc_format *)&format);
			}
		}
}

/** Display information about the various CONTROLs of an entity.
 *
 * Note: only LINEAR UNIT8 and UTF8 value types are fully decoded.
 * \param	ctrl_h					AVB control handle to use (must be for a AVB_CTRL_AVDECC_CONTROLLER channel)
 * \param	entity_id				ID of the entity to display information about (in network order).
 * \param 	configuration_index		Index of the entity configuration to use.
 */
void display_controls(struct avb_control_handle *ctrl_h, avb_u64 *entity_id,  avb_u16 configuration_index)
{
	struct control_descriptor control_desc;
	int i, rc = AVB_SUCCESS;
	avb_u8 status = AECP_AEM_SUCCESS;
	avb_u16 len = sizeof(struct control_descriptor);
	avb_u16 value_type, control_value_type;

	printf("     Controls:\n");

	i = 0;
	rc = aecp_aem_send_read_descriptor(ctrl_h, entity_id, configuration_index, AEM_DESC_TYPE_CONTROL, i, &control_desc, &len, &status, 1);

	while ((rc == AVB_SUCCESS) && (status == AECP_AEM_SUCCESS)) {
		control_value_type = ntohs(control_desc.control_value_type);
		value_type = AEM_CONTROL_GET_VALUE_TYPE(control_value_type);
		printf("         Control %2d: name = %20s    type = 0x%" PRIx64 "    read-only = %3s    value_type = %d  ", i, control_desc.object_name, ntohll(control_desc.control_type), AEM_CONTROL_GET_R(control_value_type)?"Yes":"No", value_type);
		if (value_type == AEM_CONTROL_LINEAR_UINT8)
			printf("min = %d current = %d max = %d step = %d\n", control_desc.value_details.linear_int8[0].min, control_desc.value_details.linear_int8[0].current, control_desc.value_details.linear_int8[0].max, control_desc.value_details.linear_int8[0].step);
		if (value_type == AEM_CONTROL_UTF8)
			printf("current = %s\n", control_desc.value_details.utf8.string);

		i++;
		rc = aecp_aem_send_read_descriptor(ctrl_h, entity_id, configuration_index, AEM_DESC_TYPE_CONTROL, i, &control_desc, &len, &status, 1);
	}

	if (i == 0)
		printf("         None\n");
}

/** Display information about a given entity.
 * Display the information about an entity retrieved through ADP, and also display details about the STREAM INPUTs, STREAM OUTPUTS and CONTROL available on the entity.
 *
 * \param	ctrl_h					AVB control handle to use (must be for a AVB_CTRL_AVDECC_CONTROLLER channel)
 * \param	entity_info				Pointer to an entity info structure received through an ADP message.
 */
void display_entity_info(struct avb_control_handle *ctrl_h, struct entity_info *info)
{
	int total;

	printf("Entity ID = 0x%" PRIx64 "     Model ID = 0x%" PRIx64 "    Capabilities = 0x%x Association ID = 0x%" PRIx64 ""
			"   MAC address= %02X:%02X:%02X:%02X:%02X:%02X   Local MAC address= %02X:%02X:%02X:%02X:%02X:%02X \n",
			ntohll(info->entity_id), ntohll(info->entity_model_id), ntohl(info->entity_capabilities), ntohll(info->association_id),
			info->mac_addr[0], info->mac_addr[1], info->mac_addr[2], info->mac_addr[3], info->mac_addr[4], info->mac_addr[5],
			info->local_mac_addr[0], info->local_mac_addr[1], info->local_mac_addr[2], info->local_mac_addr[3], info->local_mac_addr[4], info->local_mac_addr[5]);

	if (info->controller_capabilities)
		printf("     Controller\n");

	if (info->talker_stream_sources) {
		total = ntohs(info->talker_stream_sources);
		printf("     Talker:     sources = %d     capabilities = 0x%x\n", total, ntohs(info->talker_capabilities));
		display_stream_inputs_outputs(ctrl_h, &info->entity_id, 0, AEM_DESC_TYPE_STREAM_OUTPUT, total);
	}

	if (info->listener_stream_sinks) {
		total = ntohs(info->listener_stream_sinks);
		printf("     Listener:   sinks   = %d     capabilities = 0x%x\n", ntohs(info->listener_stream_sinks), ntohs(info->listener_capabilities));
		display_stream_inputs_outputs(ctrl_h, &info->entity_id, 0, AEM_DESC_TYPE_STREAM_INPUT, total);
	}

	display_controls(ctrl_h, &info->entity_id, 0);

	printf("\n\n");
}

int main(int argc, char *argv[])
{
	struct avb_handle *avb_h;
	struct avb_control_handle *ctrl_h = NULL;
	unsigned long long optval_ull;
	unsigned long optval_ul;
	int option;
	int rc = 0;
	unsigned int control_type = AEM_CONTROL_LINEAR_UINT8;

	setlinebuf(stdout);

	printf("NXP's GenAVB AVDECC controller demo application\n");

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
	 * Open AVB_CTRL_AVDECC_CONTROLLER API control type
	 */
	rc = avb_control_open(avb_h, &ctrl_h, AVB_CTRL_AVDECC_CONTROLLER);
	if (rc != AVB_SUCCESS) {
		printf("avb_control_open() failed: %s\n", avb_strerror(rc));
		goto error_control_open;
	}

	while ((option = getopt(argc, argv,"S:G:c:d:r:t:s:lhL:T:")) != -1) {
		if (optind > 3) {
			printf("Cannot parse arguments!\n");
			rc = -1;
			usage();
			goto exit;
		}

		switch (option) {
		case 'S':
		{
			avb_u64 entity_id;
			avb_u16 descriptor_index;
			avb_u8 value;
			avb_u8 status;
			char utf8_val[MAX_UTF8_STRING_SIZE];

			if (argc != 6) {
				printf("Cannot set control value: wrong number of argument(s)!\n");
				rc = -1;
				usage();
				goto exit;
			}

			if (!strcasecmp(optarg,"uint8"))
				control_type = AEM_CONTROL_LINEAR_UINT8;
			else if (!strcasecmp(optarg,"utf8"))
				control_type = AEM_CONTROL_UTF8;
			else {
				printf("SET/GET_CONTROL command type should be either uint8 or utf8 \n");
				rc = -1;
				goto exit;
			}

			if (h_strtoull(&optval_ull, argv[optind], NULL, 0) < 0) {
				rc = -1;
				goto exit;
			}

			entity_id = (avb_u64)optval_ull;
			entity_id = htonll(entity_id);

			if (h_strtoul(&optval_ul, argv[optind + 1], NULL, 0) < 0) {
				rc = -1;
				goto exit;
			}

			descriptor_index = (avb_u16)optval_ul;

			if (control_type == AEM_CONTROL_LINEAR_UINT8) {
				if (h_strtoul(&optval_ul, argv[optind + 2], NULL, 0) < 0) {
					rc = -1;
					goto exit;
				}
				value = (avb_u8)optval_ul;

				rc = aecp_aem_send_set_control_single_u8_command(ctrl_h, &entity_id, descriptor_index, &value, &status, 1);
				if ((rc == AVB_SUCCESS) && (status == AECP_AEM_SUCCESS)) {
					printf("aecp_aem_send_set_control_single_u8_command successful with the uint8 value (%u) \n", value);
				} else if (rc == AVB_SUCCESS) {
					printf("aecp_aem_send_set_control_single_u8_command failed with status %u (returned value %u)\n", status, value);
				} else {
					printf("aecp_aem_send_set_control_single_u8_command send failed rc %d \n", rc);
				}
			} else {
				strncpy(utf8_val, argv[optind + 2], MAX_UTF8_STRING_SIZE - 1);

				rc = aecp_aem_send_set_control_utf8_command(ctrl_h, &entity_id, descriptor_index, utf8_val, &status, 1);
				if ((rc == AVB_SUCCESS) && (status == AECP_AEM_SUCCESS)) {
					printf("aecp_aem_send_set_control_utf8_command successful with the utf8 string (%s) \n", utf8_val);
				} else if (rc == AVB_SUCCESS) {
					printf("aecp_aem_send_set_control_single_u8_command failed with status %u (returned value %s)\n", status, utf8_val);
				} else {
					printf("aecp_aem_send_set_control_single_u8_command send failed rc %d \n", rc);
				}
			}

			break;
		}
		case 'G':
		{
			avb_u64 entity_id;
			avb_u16 descriptor_index;
			avb_u8 value;
			char utf8_val[MAX_UTF8_STRING_SIZE];
			avb_u16 utf8_len = MAX_UTF8_STRING_SIZE;
			avb_u16 uint8_len = 1;
			avb_u8 status;

			if (argc != 5) {
				printf("Cannot get control value: wrong number of argument(s)!\n");
				rc = -1;
				usage();
				goto exit;
			}

			if (!strcasecmp(optarg,"uint8"))
				control_type = AEM_CONTROL_LINEAR_UINT8;
			else if (!strcasecmp(optarg,"utf8"))
				control_type = AEM_CONTROL_UTF8;
			else {
				printf("SET/GET_CONTROL command type should be either uint8 or utf8 \n");
				rc = -1;
				goto exit;
			}

			if (h_strtoull(&optval_ull, argv[optind], NULL, 0) < 0) {
				rc = -1;
				goto exit;
			}

			entity_id = (avb_u64)optval_ull;
			entity_id = htonll(entity_id);

			if (h_strtoul(&optval_ul, argv[optind + 1], NULL, 0) < 0) {
				rc = -1;
				goto exit;
			}

			descriptor_index = (avb_u16)optval_ul;

			if (control_type == AEM_CONTROL_LINEAR_UINT8) {

				rc = aecp_aem_send_get_control(ctrl_h, &entity_id, descriptor_index, &value, &uint8_len, &status, 1);
				if ((rc == AVB_SUCCESS) && (status == AECP_AEM_SUCCESS)) {
					printf("aecp_aem_send_get_control successful with resp_len (%u) and uint8 value (%u) \n", uint8_len, value);
				} else if (rc == AVB_SUCCESS) {
					printf("aecp_aem_send_get_control failed with status %u \n", status);
				} else {
					printf("aecp_aem_send_get_control send failed rc %d \n", rc);
				}
			} else {

				rc = aecp_aem_send_get_control(ctrl_h, &entity_id, descriptor_index, utf8_val, &utf8_len, &status, 1);
				if ((rc == AVB_SUCCESS) && (status == AECP_AEM_SUCCESS)) {
					printf("aecp_aem_send_get_control successful with resp_len (%u) and utf8 string value (%s) \n", uint8_len, utf8_val);
				} else if (rc == AVB_SUCCESS) {
					printf("aecp_aem_send_get_control failed with status %u \n", status);
				} else {
					printf("aecp_aem_send_get_control send failed rc %d \n", rc);
				}
			}

			break;
		}
		case 'l':
		{
			struct entity_info *entities = NULL;
			int n_entities, i;

			n_entities = adp_dump_entities(ctrl_h, &entities);
			printf("Number of discovered entities: %d\n", n_entities);
			for (i = 0; i < n_entities; i++)
				display_entity_info(ctrl_h, &entities[i]);
			break;
		}
		case 'c':
		case 'd':
		{
			avb_u64 talker_entity_id, listener_entity_id;
			avb_u16 talker_unique_id, listener_unique_id;
			avb_u16 flags;
			struct avb_acmp_response acmp_rsp;

			if (((option == 'c') && (argc != 7)) || ((option == 'd') && (argc != 6))) {
				printf("Cannot connect or disconnect stream: wrong number of argument(s)!\n");
				rc = -1;
				usage();
				goto exit;
			}

			if (h_strtoull(&optval_ull, optarg, NULL, 0) < 0) {
				rc = -1;
				goto exit;
			}
			talker_entity_id = htonll(optval_ull);

			if (h_strtoul(&optval_ul, argv[optind], NULL, 0) < 0) {
				rc = -1;
				goto exit;
			}
			talker_unique_id = (avb_u16)optval_ul;
			talker_unique_id = htons(talker_unique_id);

			if (h_strtoull(&optval_ull, argv[optind + 1], NULL, 0) < 0) {
				rc = -1;
				goto exit;
			}
			listener_entity_id = htonll(optval_ull);

			if (h_strtoul(&optval_ul, argv[optind + 2], NULL, 0) < 0) {
				rc = -1;
				goto exit;
			}
			listener_unique_id = (avb_u16)optval_ul;
			listener_unique_id = htons(listener_unique_id);

			if (option == 'c') {
				if (h_strtoul(&optval_ul, argv[optind + 3], NULL, 0) < 0) {
					rc = -1;
					goto exit;
				}
				flags = (avb_u16)optval_ul;
				flags = htons(flags);
				rc = acmp_connect_stream(ctrl_h, talker_entity_id, talker_unique_id, listener_entity_id, listener_unique_id, flags, &acmp_rsp);
				printf("Stream connection");
			} else {
				rc = acmp_disconnect_stream(ctrl_h, talker_entity_id, talker_unique_id, listener_entity_id, listener_unique_id, &acmp_rsp);
				printf("Stream disconnection");
			}

			if (rc == ACMP_STAT_SUCCESS) {
				printf(" successful: stream id = 0x%" PRIx64 "  Destination MAC address %02X:%02X:%02X:%02X:%02X:%02X flags = 0x%x connection_count = %d VLAN id = %d\n",
						ntohll(acmp_rsp.stream_id),
						acmp_rsp.stream_dest_mac[0], acmp_rsp.stream_dest_mac[1], acmp_rsp.stream_dest_mac[2], acmp_rsp.stream_dest_mac[3], acmp_rsp.stream_dest_mac[4], acmp_rsp.stream_dest_mac[5],
						ntohs(acmp_rsp.flags), ntohs(acmp_rsp.connection_count), ntohs(acmp_rsp.stream_vlan_id));
			} else
				printf(" failed with error %d\n", rc);

			break;
		}
		case 'r':
		case 't':
		{
			avb_u64 entity_id;
			avb_u16 unique_id;
			struct avb_acmp_response acmp_rsp;

			if (argc != 4) {
				printf("Cannot get state information: wrong number of argument(s)!\n");
				rc = -1;
				usage();
				goto exit;
			}

			if (h_strtoull(&optval_ull, optarg, NULL, 0) < 0) {
				rc = -1;
				goto exit;
			}
			entity_id = htonll(optval_ull);

			if (h_strtoul(&optval_ul, argv[optind], NULL, 0) < 0) {
				rc = -1;
				goto exit;
			}
			unique_id = (avb_u16)optval_ul;
			unique_id = htons(unique_id);

			if (option == 'r')
				rc = acmp_get_rx_state(ctrl_h, entity_id, unique_id, &acmp_rsp);
			else
				rc = acmp_get_tx_state(ctrl_h, entity_id, unique_id, &acmp_rsp);


			if (rc == ACMP_STAT_SUCCESS) {
				if (option == 'r')
					printf("Listener sink information: talker entity id 0x%" PRIx64 " unique id = %d flags = 0x%x",
							ntohll(acmp_rsp.talker_entity_id), ntohs(acmp_rsp.talker_unique_id), ntohs(acmp_rsp.flags));
				else
					printf("Talker source information");

				printf(" stream id = 0x%" PRIx64 "  Destination MAC address %02X:%02X:%02X:%02X:%02X:%02X connection_count = %d VLAN id = %d\n",
						ntohll(acmp_rsp.stream_id),
						acmp_rsp.stream_dest_mac[0], acmp_rsp.stream_dest_mac[1], acmp_rsp.stream_dest_mac[2], acmp_rsp.stream_dest_mac[3], acmp_rsp.stream_dest_mac[4], acmp_rsp.stream_dest_mac[5],
						ntohs(acmp_rsp.connection_count), ntohs(acmp_rsp.stream_vlan_id));
			} else
				printf(" retrieval failed with error %d\n", rc);

			break;

		}
		case 'T':
		case 'L':
		{
			avb_u64 entity_id;
			avb_u16 unique_id;
			avb_u8 status = AECP_AEM_SUCCESS;
#define MAX_CMD_LEN 16
			char command[MAX_CMD_LEN];

			if (argc != 5) {
				printf("Cannot start or stop stream: wrong number of argument(s)!\n");
				rc = -1;
				usage();
				goto exit;
			}

			if (h_strtoull(&optval_ull, optarg, NULL, 0) < 0) {
				rc = -1;
				goto exit;
			}
			entity_id = htonll(optval_ull);

			if (h_strtoul(&optval_ul, argv[optind], NULL, 0) < 0) {
				rc = -1;
				goto exit;
			}

			unique_id = (avb_u16)optval_ul;

			h_strncpy(command, argv[optind + 1], MAX_CMD_LEN);

			if (option == 'T') {
				if (!strcmp(command, "start"))
					rc = aecp_aem_send_start_streaming(ctrl_h, &entity_id, AEM_DESC_TYPE_STREAM_OUTPUT, unique_id, &status, 1);
				else if (!strcmp(command, "stop"))
					rc = aecp_aem_send_stop_streaming(ctrl_h, &entity_id, AEM_DESC_TYPE_STREAM_OUTPUT, unique_id, &status, 1);
				else {
					printf("Wrong AECP command \n");
					rc = -1;
					goto exit;
				}
			} else {
				if (!strcmp(command, "start"))
					rc = aecp_aem_send_start_streaming(ctrl_h, &entity_id, AEM_DESC_TYPE_STREAM_INPUT, unique_id, &status, 1);
				else if (!strcmp(command, "stop"))
					rc = aecp_aem_send_stop_streaming(ctrl_h, &entity_id, AEM_DESC_TYPE_STREAM_INPUT, unique_id, &status, 1);
				else {
					printf("Wrong AECP command \n");
					rc = -1;
					goto exit;
				}
			}

			if (rc == AVB_SUCCESS) {
				printf("Streaming command [%s] for %s [entity ID: (0x%" PRIx64 ") - unique ID (%u)] sent successfully : returned with status %d \n", command, (option == 'T') ? "Talker" : "Listener", ntohll(entity_id), unique_id, status);
			} else
				printf("Streaming command [%s] for %s [entity ID: (0x%" PRIx64 ") - unique ID (%u)] sending failed with error %d\n", command, (option == 'T') ? "Talker" : "Listener", ntohll(entity_id), unique_id, rc);

			break;
		}
		case 's':
		{
			avb_u64 talker_entity_id;
			avb_u16 talker_unique_id;
			avb_u16 connection_count;
			struct avb_acmp_response acmp_rsp;

			if (argc != 5) {
				printf("Cannot get connection state information: wrong number of argument(s)!\n");
				rc = -1;
				usage();
				goto exit;
			}

			 if (h_strtoull(&optval_ull, optarg, NULL, 0) < 0) {
				 rc = -1;
				 goto exit;
			}
			talker_entity_id = htonll(optval_ull);

			if (h_strtoul(&optval_ul, argv[optind], NULL, 0) < 0) {
				rc = -1;
				goto exit;
			}
			talker_unique_id =(avb_u16)optval_ul;
			talker_unique_id = htons(talker_unique_id);

			if (h_strtoull(&optval_ull, argv[optind + 1], NULL, 0) < 0) {
				rc = -1;
				goto exit;
			}
			connection_count = (avb_u16)optval_ull;
			connection_count = htons(connection_count);

			rc = acmp_get_tx_connection(ctrl_h, talker_entity_id, talker_unique_id, connection_count, &acmp_rsp);

			if (rc == ACMP_STAT_SUCCESS) {
				printf("Connection information: stream id = 0x%" PRIx64 "    listener entity id 0x%" PRIx64 "    unique id = %d    Destination MAC address %02X:%02X:%02X:%02X:%02X:%02X    connection_count = %d    VLAN id = %d    flags = 0x%x \n",
						ntohll(acmp_rsp.stream_id), ntohll(acmp_rsp.listener_entity_id), ntohs(acmp_rsp.listener_unique_id),
						acmp_rsp.stream_dest_mac[0], acmp_rsp.stream_dest_mac[1], acmp_rsp.stream_dest_mac[2], acmp_rsp.stream_dest_mac[3], acmp_rsp.stream_dest_mac[4], acmp_rsp.stream_dest_mac[5],
						ntohs(acmp_rsp.connection_count), ntohs(acmp_rsp.stream_vlan_id), ntohs(acmp_rsp.flags));
			} else
				printf(" retrieval failed with error %d\n", rc);

			break;

		}
		case 'h':
		default:
			usage();
			rc = -1;
			goto exit;
		}
	}


exit:
	if (ctrl_h)
		avb_control_close(ctrl_h);

error_control_open:
	avb_exit(avb_h);

error_avb_init:
	return rc;

}
