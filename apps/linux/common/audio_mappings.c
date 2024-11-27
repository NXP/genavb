/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

 #include <stdio.h>
 #include <string.h>
 #include <inttypes.h>

 #include <genavb/aem.h>

#include "audio_mappings.h"

#define MAX_STREAM_PORT_INPUT	4
#define MAX_STREAM_PORT_OUTPUT	4
#define MAX_AUDIO_MAPS		1 /* Use a single audio map for now to store all audio mappings for a single STREAM_PORT_INPUT/OUTPUT */
#define MAX_AUDIO_MAP_SIZE	16 /* Maximum number of audio mappings in a single audio map */
#define AUDIO_MAP_IDX		0

/* Make sure that maximum number of audio mappings does not exceed the IPC (and PDU) message size */
#if ((MAX_AUDIO_MAP_SIZE * 8 + 32) > AVB_AECP_MAX_MSG_SIZE)
#error Maximum audio mapping count exceeds size of AECP IPC and PDU
#endif

struct audio_mapping_entry {
	struct aecp_aem_get_audio_map_mappings_format mapping;
	avb_u8 valid:1;
	avb_u8 selected:1;
	avb_u8 reserved:6;
};

struct mapping_entry {
	int registered_index; /* index in the registered array */
	int mapping_index;   /* index in the received mappings array */
};

static struct audio_mapping_entry stream_port_input_audio_mappings[MAX_STREAM_PORT_INPUT][MAX_AUDIO_MAPS][MAX_AUDIO_MAP_SIZE];
static struct audio_mapping_entry stream_port_output_audio_mappings[MAX_STREAM_PORT_OUTPUT][MAX_AUDIO_MAPS][MAX_AUDIO_MAP_SIZE];

static int get_free_audio_mapping_index(struct audio_mapping_entry *stream_port_audio_mappings)
{
	int index = -1;
	unsigned int i;

	for (i = 0; i < MAX_AUDIO_MAP_SIZE; i++) {
		if (!stream_port_audio_mappings[i].valid && !stream_port_audio_mappings[i].selected) {
			stream_port_audio_mappings[i].selected = true;
			index = i;
			goto exit;
		}
	}

exit:
	return index;
}

int audio_mappings_remove(avb_u16 desc_index, avb_u16 desc_type, avb_u16 mappings_count, void *audio_mappings)
{
	struct audio_mapping_entry (*stream_port_audio_mappings)[MAX_AUDIO_MAPS][MAX_AUDIO_MAP_SIZE];
	struct mapping_entry remove_entries[MAX_AUDIO_MAP_SIZE] = {{-1 , -1}};
	unsigned int remove_entries_count = 0;
	int rc = AECP_AEM_SUCCESS;
	bool found = false;
	unsigned int i, j;

	switch(desc_type) {
	case AEM_DESC_TYPE_STREAM_PORT_INPUT:
		stream_port_audio_mappings = stream_port_input_audio_mappings;

		if (desc_index >= MAX_STREAM_PORT_INPUT) {
			printf("%s: Error: Unsupported AEM_DESC_TYPE_STREAM_PORT_INPUT index(%u). Max supported(%u)\n", __func__, desc_index, MAX_STREAM_PORT_INPUT - 1);
			rc = AECP_AEM_NOT_SUPPORTED;
			goto exit;
		}

		break;

	case AEM_DESC_TYPE_STREAM_PORT_OUTPUT:
		stream_port_audio_mappings = stream_port_output_audio_mappings;

		if (desc_index >= MAX_STREAM_PORT_OUTPUT) {
			printf("%s: Error: Unsupported AEM_DESC_TYPE_STREAM_PORT_OUTPUT index(%u). Max supported(%u)\n", __func__, desc_index, MAX_STREAM_PORT_OUTPUT - 1);
			rc = AECP_AEM_NOT_SUPPORTED;
			goto exit;
		}

		break;

	default:
		printf("%s: Error: Unknown descriptor type(%u)\n", __func__, desc_type);
		rc = AECP_AEM_BAD_ARGUMENTS;
		goto exit;
	}

	for (i = 0; i < mappings_count; i++) {
		found = false;

		for (j = 0; j < MAX_AUDIO_MAP_SIZE; j++) {
			/* Only current valid audio mappings can possibly be removed */
			if (!stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][j].valid)
				continue;

			if (!memcmp(&((struct aecp_aem_get_audio_map_mappings_format *)audio_mappings)[i], &stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][j].mapping, sizeof(struct aecp_aem_get_audio_map_mappings_format))) {
				remove_entries[remove_entries_count++].registered_index = j;
				found = true;
				break;
			}
		}

		if (!found) {
			printf("%s: Error: AEM_DESC_TYPE_STREAM_PORT_%s index(%u) does not contain the mapping at index(%u) from the REMOVE_AUDIO_MAPPINGS command\n",
				__func__, (desc_type == AEM_DESC_TYPE_STREAM_PORT_INPUT) ? "INPUT" : "OUTPUT", desc_index, i);
			rc = AECP_AEM_BAD_ARGUMENTS;
			goto exit;
		}
	}

	for (i = 0; i < remove_entries_count; i++) {
		stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][remove_entries[i].registered_index].valid = false;
	}

exit:
	return rc;
}

int audio_mappings_add(avb_u16 desc_index, avb_u16 desc_type, avb_u16 mappings_count, void *audio_mappings)
{
	struct audio_mapping_entry (*stream_port_audio_mappings)[MAX_AUDIO_MAPS][MAX_AUDIO_MAP_SIZE];
	avb_u16 mapping_stream_index, mapping_stream_channel, mapping_cluster_offset, mapping_cluster_channel;
	struct mapping_entry new_entries[MAX_AUDIO_MAP_SIZE] = {{-1 , -1}};
	unsigned int new_entries_count = 0;
	int rc = AECP_AEM_SUCCESS;
	unsigned int i, j;
	int index;

	switch(desc_type) {
	case AEM_DESC_TYPE_STREAM_PORT_INPUT:
		stream_port_audio_mappings = stream_port_input_audio_mappings;

		if (desc_index >= MAX_STREAM_PORT_INPUT) {
			printf("%s: Error: Unsupported AEM_DESC_TYPE_STREAM_PORT_INPUT index(%u). Max supported(%u)\n", __func__, desc_index, MAX_STREAM_PORT_INPUT - 1);
			rc = AECP_AEM_NOT_SUPPORTED;
			goto exit;
		}

		break;

	case AEM_DESC_TYPE_STREAM_PORT_OUTPUT:
		stream_port_audio_mappings = stream_port_output_audio_mappings;

		if (desc_index >= MAX_STREAM_PORT_OUTPUT) {
			printf("%s: Error: Unsupported AEM_DESC_TYPE_STREAM_PORT_OUTPUT index(%u). Max supported(%u)\n", __func__, desc_index, MAX_STREAM_PORT_OUTPUT - 1);
			rc = AECP_AEM_NOT_SUPPORTED;
			goto exit;
		}

		break;

	default:
		printf("%s: Error: Unknown descriptor type(%u)\n", __func__, desc_type);
		rc = AECP_AEM_BAD_ARGUMENTS;
		goto exit;
	}

	for (i = 0; i < mappings_count; i++) {
		mapping_stream_index = ((struct aecp_aem_get_audio_map_mappings_format *)audio_mappings)[i].mapping_stream_index;
		mapping_stream_channel = ((struct aecp_aem_get_audio_map_mappings_format *)audio_mappings)[i].mapping_stream_channel;
		mapping_cluster_offset = ((struct aecp_aem_get_audio_map_mappings_format *)audio_mappings)[i].mapping_cluster_offset;
		mapping_cluster_channel = ((struct aecp_aem_get_audio_map_mappings_format *)audio_mappings)[i].mapping_cluster_channel;

		for (j = 0; j < MAX_AUDIO_MAP_SIZE; j++) {
			/* Only compare current valid audio mappings to the audio mappings of the command */
			if (!stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][j].valid)
				continue;

			if (!memcmp(&((struct aecp_aem_get_audio_map_mappings_format *)audio_mappings)[i], &stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][j].mapping, sizeof(struct aecp_aem_get_audio_map_mappings_format))) {
				/* The same audio mapping is already here and valid */
				goto next_mapping;
			}

			if (desc_type == AEM_DESC_TYPE_STREAM_PORT_INPUT) {
				/* If the command, on a Stream Port Input, contains a mapping
				 * that references an existing mapping with the same cluster’s channel
				 * and two different stream’s channels that are not redundant,
				 * the app may accept the command and override the previous mapping.
				 * As per MILAN Specification v1.2 5.4.2.27
				 */
				if ((mapping_cluster_offset == stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][j].mapping.mapping_cluster_offset) &&
				    (mapping_cluster_channel == stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][j].mapping.mapping_cluster_channel) &&
				    ((mapping_stream_index != stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][j].mapping.mapping_stream_index) ||
				    (mapping_stream_channel != stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][j].mapping.mapping_stream_channel))) {
					new_entries[new_entries_count].registered_index = j;
					new_entries[new_entries_count].mapping_index = i;
					new_entries_count++;
					goto next_mapping;
				}

			} else { /* AEM_DESC_TYPE_STREAM_PORT_OUTPUT */
				/* If the command, on a Stream Port Output, contains a mapping
				 * that references an existing mapping with the same stream’s channel
				 * and two different cluster’s channels that are not redundant,
				 * the app may accept the command and override the previous mapping.
				 * As per MILAN Specification v1.2 5.4.2.27
				 */
				if ((mapping_stream_index == stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][j].mapping.mapping_stream_index) &&
				    (mapping_stream_channel == stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][j].mapping.mapping_stream_channel) &&
				    ((mapping_cluster_offset != stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][j].mapping.mapping_cluster_offset) ||
				    (mapping_cluster_channel != stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][j].mapping.mapping_cluster_channel))) {
					new_entries[new_entries_count].registered_index = j;
					new_entries[new_entries_count].mapping_index = i;
					new_entries_count++;
					goto next_mapping;
				}
			}
		}

		index = get_free_audio_mapping_index(stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX]);
		if (index < 0) {
			printf("%s: Error: AEM_DESC_TYPE_STREAM_PORT_%s index(%u)'s mappings are full. Max supported(%u)\n",
				__func__, (desc_type == AEM_DESC_TYPE_STREAM_PORT_INPUT) ? "INPUT" : "OUTPUT", desc_index, MAX_AUDIO_MAP_SIZE);
			rc = AECP_AEM_NOT_SUPPORTED;
			goto mappings_full; /* Unable to get a free entry from our audio mappings array */
		}

		/* Save the entry of the new audio mapping to add */
		new_entries[new_entries_count].registered_index = index;
		new_entries[new_entries_count].mapping_index = i;
		new_entries_count++;

next_mapping:
		continue;
	}

	/* Command is valid, update the audio mappings */
	for (i = 0; i < new_entries_count; i++) {
		memcpy(&stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][new_entries[i].registered_index].mapping,
			&((struct aecp_aem_get_audio_map_mappings_format *)audio_mappings)[new_entries[i].mapping_index],
			sizeof(struct aecp_aem_get_audio_map_mappings_format));

		stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][new_entries[i].registered_index].valid = true;
	}

mappings_full:
	/* Reset selected state of audio_mapping_entries, they either became valid or the status is AECP_AEM_NOT_SUPPORTED */
	for (i = 0; i < MAX_AUDIO_MAP_SIZE; i++) {
		stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][i].selected = false;
	}

exit:
	return rc;
}

int audio_mappings_get(avb_u16 desc_index, avb_u16 desc_type, avb_u16 map_index, avb_u16 *number_of_maps, avb_u16 *mappings_count, void *audio_mappings)
{
	struct audio_mapping_entry (*stream_port_audio_mappings)[MAX_AUDIO_MAPS][MAX_AUDIO_MAP_SIZE];
	avb_u16 mapping_index = 0;
	int rc = AECP_AEM_SUCCESS;
	unsigned int i;

	if (map_index >= MAX_AUDIO_MAPS) {
		printf("%s: Error: Unsupported MAP INDEX index(%u). Max supported(%u)\n", __func__, map_index, MAX_AUDIO_MAPS - 1);
		rc = AECP_AEM_BAD_ARGUMENTS;
		goto exit;
	}

	switch(desc_type) {
	case AEM_DESC_TYPE_STREAM_PORT_INPUT:
		stream_port_audio_mappings = stream_port_input_audio_mappings;

		if (desc_index >= MAX_STREAM_PORT_INPUT) {
			printf("%s: Error: Unsupported AEM_DESC_TYPE_STREAM_PORT_INPUT index(%u). Max supported(%u)\n", __func__, desc_index, MAX_STREAM_PORT_INPUT - 1);
			rc = AECP_AEM_NOT_SUPPORTED;
			goto exit;
		}

		break;

	case AEM_DESC_TYPE_STREAM_PORT_OUTPUT:
		stream_port_audio_mappings = stream_port_output_audio_mappings;

		if (desc_index >= MAX_STREAM_PORT_OUTPUT) {
			printf("%s: Error: Unsupported AEM_DESC_TYPE_STREAM_PORT_OUTPUT index(%u). Max supported(%u)\n", __func__, desc_index, MAX_STREAM_PORT_OUTPUT - 1);
			rc = AECP_AEM_NOT_SUPPORTED;
			goto exit;
		}

		break;

	default:
		printf("%s: Error: Unknown descriptor type(%u)\n", __func__, desc_type);
		rc = AECP_AEM_BAD_ARGUMENTS;
		goto exit;
	}

	for (i = 0; i < MAX_AUDIO_MAP_SIZE; i++) {
		if (stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][i].valid) {
			memcpy(&((struct aecp_aem_get_audio_map_mappings_format *)audio_mappings)[mapping_index], &stream_port_audio_mappings[desc_index][AUDIO_MAP_IDX][i].mapping, sizeof(struct aecp_aem_get_audio_map_mappings_format));
			mapping_index++;
		}
	}

	*mappings_count = mapping_index;
	*number_of_maps = MAX_AUDIO_MAPS;

exit:
	return rc;
}

int audio_mappings_init(void)
{
	unsigned int i, j;
	int rc = AECP_AEM_SUCCESS;

	for (i = 0; i < MAX_STREAM_PORT_INPUT; i++) {
		for (j = 0; j < MAX_AUDIO_MAP_SIZE; j++) {
			stream_port_input_audio_mappings[i][AUDIO_MAP_IDX][j].valid = false;
			stream_port_input_audio_mappings[i][AUDIO_MAP_IDX][j].selected = false;
		}
	}

	for (i = 0; i < MAX_STREAM_PORT_OUTPUT; i++) {
		for (j = 0; j < MAX_AUDIO_MAP_SIZE; j++) {
			stream_port_output_audio_mappings[i][AUDIO_MAP_IDX][j].valid = false;
			stream_port_output_audio_mappings[i][AUDIO_MAP_IDX][j].selected = false;
		}
	}

	return rc;
}
