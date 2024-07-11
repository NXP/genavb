/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file aem_helpers.c
 \brief AEM helper functions
 \details Helper functions to handle AEM structures

 \copyright Copyright 2014-2016 Freescale Semiconductor, Inc.
 \copyright Copyright 2016-2018, 2023-2024 NXP
 All Rights Reserved.
*/


#include "genavb/types.h"
#include "genavb/net_types.h"

#include "genavb/aem.h"
#include "genavb/adp.h"
#include "genavb/aem_helpers.h"


unsigned int aem_get_descriptor_max(struct aem_desc_hdr *aem_desc, avb_u16 type)
{
	struct aem_desc_hdr *desc = &aem_desc[type];

	return desc->total;
}

void *aem_get_descriptor(struct aem_desc_hdr *aem_desc, avb_u16 type, avb_u16 index, avb_u16 *len)
{
	struct aem_desc_hdr *desc = &aem_desc[type];

	if (index < desc->total) {
		if (len)
			*len = desc->size;

		return ((char *)desc->ptr + (desc->size * index));
	}
	else
		return NULL;
}

unsigned int aem_get_listener_streams(struct aem_desc_hdr *aem_desc)
{
	struct entity_descriptor *entity;

	entity = aem_get_descriptor(aem_desc, AEM_DESC_TYPE_ENTITY, 0, NULL);

	if (entity &&
	   (entity->listener_capabilities & htons(ADP_LISTENER_IMPLEMENTED)) &&
	   (entity->listener_capabilities & htons(ADP_LISTENER_VIDEO_SINK | ADP_LISTENER_AUDIO_SINK)))
		return aem_get_descriptor_max(aem_desc, AEM_DESC_TYPE_STREAM_INPUT);

	return 0;
}

unsigned int aem_get_talker_streams(struct aem_desc_hdr *aem_desc)
{
	struct entity_descriptor *entity;

	entity = aem_get_descriptor(aem_desc, AEM_DESC_TYPE_ENTITY, 0, NULL);

	if (entity &&
	   (entity->talker_capabilities & htons(ADP_TALKER_IMPLEMENTED)) &&
	   (entity->talker_capabilities & htons(ADP_TALKER_VIDEO_SOURCE | ADP_TALKER_AUDIO_SOURCE)))
		return aem_get_descriptor_max(aem_desc, AEM_DESC_TYPE_STREAM_OUTPUT);

	return 0;
}

void aem_entity_desc_fixup(struct aem_desc_hdr *aem_desc)
{
	struct entity_descriptor *entity;

	entity = aem_get_descriptor(aem_desc, AEM_DESC_TYPE_ENTITY, 0, NULL);

	if (entity) {
		entity->talker_stream_sources = htons(aem_get_descriptor_max(aem_desc, AEM_DESC_TYPE_STREAM_OUTPUT));
		entity->listener_stream_sinks = htons(aem_get_descriptor_max(aem_desc, AEM_DESC_TYPE_STREAM_INPUT));
		entity->configurations_count = htons(aem_get_descriptor_max(aem_desc, AEM_DESC_TYPE_CONFIGURATION));
	}
}


void aem_configuration_desc_fixup(struct aem_desc_hdr *aem_desc)
{
	struct configuration_descriptor *configuration;
	int desc_type, desc_num, desc_type_total;
	int cfg, cfg_max;

	cfg_max = aem_get_descriptor_max(aem_desc, AEM_DESC_TYPE_CONFIGURATION);

	for (cfg = 0; cfg < cfg_max; cfg++) {
		configuration = aem_get_descriptor(aem_desc, AEM_DESC_TYPE_CONFIGURATION, cfg, NULL);

		/*
		 * desc_type_total starts at 1 as the CONTROL descriptor count
		 * in the descriptors_counts array is already handled in the
		 * entity's configuration
		 */
		desc_type_total = 1;

		for (desc_type = AEM_DESC_TYPE_AUDIO_UNIT; desc_type < AEM_NUM_DESC_TYPES; desc_type++) {
			switch (desc_type) {
			case AEM_DESC_TYPE_STRINGS:
			case AEM_DESC_TYPE_STREAM_PORT_INPUT:
			case AEM_DESC_TYPE_STREAM_PORT_OUTPUT:
			case AEM_DESC_TYPE_EXTERNAL_PORT_INPUT:
			case AEM_DESC_TYPE_EXTERNAL_PORT_OUTPUT:
			case AEM_DESC_TYPE_INTERNAL_PORT_INPUT:
			case AEM_DESC_TYPE_INTERNAL_PORT_OUTPUT:
			case AEM_DESC_TYPE_AUDIO_CLUSTER:
			case AEM_DESC_TYPE_VIDEO_CLUSTER:
			case AEM_DESC_TYPE_SENSOR_CLUSTER:
			case AEM_DESC_TYPE_AUDIO_MAP:
			case AEM_DESC_TYPE_VIDEO_MAP:
			case AEM_DESC_TYPE_SENSOR_MAP:
			case AEM_DESC_TYPE_CONTROL:
				break;

			default:
				desc_num = aem_get_descriptor_max(aem_desc, desc_type);

				if (desc_num) {
					configuration->descriptors_counts[2 * desc_type_total] = htons(desc_type);
					configuration->descriptors_counts[2 * desc_type_total + 1] = htons(desc_num);

					desc_type_total++;
				}

				break;
			}
		}

		configuration->descriptor_counts_count = htons(desc_type_total);
	}
}


void aem_video_cluster_desc_fixup(struct aem_desc_hdr *aem_desc)
{
	struct video_cluster_descriptor *video_cluster;
	int i, video_cluster_max;
	unsigned int offset;

	video_cluster_max = aem_get_descriptor_max(aem_desc, AEM_DESC_TYPE_VIDEO_CLUSTER);

	for (i = 0; i < video_cluster_max; i++) {
		video_cluster = aem_get_descriptor(aem_desc, AEM_DESC_TYPE_VIDEO_CLUSTER, i, NULL);

		offset = ntohs(video_cluster->supported_format_specifics_offset);
		offset += 4 * ntohs(video_cluster->supported_format_specifics_count);

		video_cluster->supported_sampling_rates_offset = htons(offset);
		offset += 4 *  ntohs(video_cluster->supported_sampling_rates_count);

		video_cluster->supported_aspect_ratios_offset = htons(offset);
		offset += 2 * ntohs(video_cluster->supported_aspect_ratios_count);

		video_cluster->supported_sizes_offset = htons(offset);
		offset += 4 * ntohs(video_cluster->supported_sizes_count);

		video_cluster->supported_color_spaces_offset = htons(offset);
	}
}
