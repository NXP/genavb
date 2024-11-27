/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file aem_manager_helpers.c
 \brief AEM Manager helper functions
 \details Helper functions to handle AEM Manager structures

 \copyright Copyright 2014-2016 Freescale Semiconductor, Inc.
 \copyright Copyright 2023-2024 NXP
 All Rights Reserved.
*/


#include <stdarg.h>
#include <string.h>
#include <inttypes.h>

#include "genavb/types.h"
#include "genavb/net_types.h"

#include "genavb/aem.h"
#include "genavb/adp.h"
#include "genavb/aem_helpers.h"
#include "genavb/helpers.h"

#include "aem_manager_helpers.h"

#define DESC_MIN 0
#define DESC_MAX 0xffff

extern void aem_print_level(int level, const char *format, ...);

static void default_desc_fixup(struct aem_desc_hdr *aem_desc)
{
}

static int default_desc_check(struct aem_desc_hdr *aem_desc)
{
	return 0;
}

static void default_desc_print(struct aem_desc_handler *desc_handler, struct aem_desc_hdr *aem_desc, int level, int min, int max)
{
}

static void default_desc_update_name(struct aem_desc_hdr *aem_desc, char *name)
{
}

int entity_desc_handler_init(struct aem_desc_handler *desc_handler)
{
	int i;

	for (i = 0; i < AEM_NUM_DESC_TYPES; i++) {
		if (!desc_handler[i].print)
			desc_handler[i].print = default_desc_print;

		if (!desc_handler[i].fixup)
			desc_handler[i].fixup = default_desc_fixup;

		if (!desc_handler[i].check)
			desc_handler[i].check = default_desc_check;

		if (!desc_handler[i].update_name)
			desc_handler[i].update_name = default_desc_update_name;
	}

	return 0;
}

int entity_desc_check(struct aem_desc_hdr *aem_desc)
{
	struct entity_descriptor *entity;
	unsigned short len;

	entity = aem_get_descriptor(aem_desc, AEM_DESC_TYPE_ENTITY, 0, &len);
	if (!entity)
		return -1;

	if (ntohs(entity->current_configuration) > ntohs(entity->configurations_count))
		return -1;

	return 0;
}

int configuration_desc_check(struct aem_desc_hdr *aem_desc)
{
	struct configuration_descriptor *configuration;
	int desc_counts_count, desc_num, desc_type, desc_max;
	int cfg, cfg_max;
	int rc = 0;

	cfg_max = aem_get_descriptor_max(aem_desc, AEM_DESC_TYPE_CONFIGURATION);

	for (cfg = 0; cfg < cfg_max; cfg++) {
		configuration = aem_get_descriptor(aem_desc, AEM_DESC_TYPE_CONFIGURATION, cfg, NULL);
		if (!configuration)
			return -1;

		for (desc_counts_count = 0; desc_counts_count < ntohs(configuration->descriptor_counts_count); desc_counts_count++) {
			desc_type = ntohs(configuration->descriptors_counts[2 * desc_counts_count]);
			desc_num = ntohs(configuration->descriptors_counts[2 * desc_counts_count + 1]);

			if (desc_type >= AEM_NUM_DESC_TYPES) {
				aem_print_level(0, "aem(%p) configuration(%u) error: unsupported descriptor type(%u) in descriptors_count\n", aem_desc, cfg, desc_type);
				rc = -1;
				goto out;
			}

			desc_max = aem_get_descriptor_max(aem_desc, desc_type);

			if (desc_num > desc_max) {
				aem_print_level(0, "aem(%p) configuration(%u) error: descriptor count(%u) exceeds maximum(%u) for descriptor type(%u)\n", aem_desc, cfg, desc_num, desc_max, desc_type);
				rc = -1;
				goto out;
			}
		}
	}

out:
	return rc;
}

void aem_entity_fixup(struct aem_desc_handler *desc_handler, struct aem_desc_hdr *aem_desc)
{
	int i;

	for (i = 0; i < AEM_NUM_DESC_TYPES; i++) {
		if (desc_handler[i].fixup)
			desc_handler[i].fixup(aem_desc);
	}
}

int aem_entity_check(struct aem_desc_handler *desc_handler, struct aem_desc_hdr *aem_desc)
{
	int i;
	int rc = 0;

	for (i = 0; i < AEM_NUM_DESC_TYPES; i++) {
		if (desc_handler[i].check) {
			rc = desc_handler[i].check(aem_desc);
			if (rc < 0)
				goto out;
		}
	}

out:
	return rc;
}

void entity_desc_print(struct aem_desc_handler *desc_handler, struct aem_desc_hdr *aem_desc, int level, int min, int max)
{
	struct entity_descriptor *entity;
	char string[128];
	unsigned short len;

	entity = aem_get_descriptor(aem_desc, AEM_DESC_TYPE_ENTITY, 0, &len);
	if (!entity)
		return;

	aem_print_level(level, "Entity id: %016"PRIx64"\n", ntohll(entity->entity_id));
	h_strncpy(string, (char *)entity->entity_name, 64);
	aem_print_level(level, " name:     %s\n", string);
	aem_print_level(level, " sources:  %u\n", ntohs(entity->talker_stream_sources));
	aem_print_level(level, " sinks:    %u\n", ntohs(entity->listener_stream_sinks));

	desc_handler[AEM_DESC_TYPE_CONFIGURATION].print(desc_handler, aem_desc, level + 1, DESC_MIN, DESC_MAX);
}

void entity_desc_update_name(struct aem_desc_hdr *aem_desc, char *name)
{
	struct entity_descriptor *entity;
	unsigned short len;

	entity = aem_get_descriptor(aem_desc, AEM_DESC_TYPE_ENTITY, 0, &len);
	if (!entity)
		return;

	/* Clear destination */
	memset((char *)entity->entity_name, 0, 64);

	len = strlen(name);
	if (len > 64)
		len = 64;

	memcpy((char *)entity->entity_name, name, len);
}

void configuration_desc_print(struct aem_desc_handler *desc_handler, struct aem_desc_hdr *aem_desc, int level, int min, int max)
{
	struct configuration_descriptor *configuration;
	char string[128];
	unsigned short len;
	int cfg, cfg_max;
	int i, type;

	cfg_max = aem_get_descriptor_max(aem_desc, AEM_DESC_TYPE_CONFIGURATION);

	if (cfg_max > max)
		cfg_max = max;

	for (cfg = min; cfg < cfg_max; cfg++) {
		configuration = aem_get_descriptor(aem_desc, AEM_DESC_TYPE_CONFIGURATION, cfg, &len);
		if (!configuration)
			continue;

		aem_print_level(level, "Configuration: %d\n", cfg);
		h_strncpy(string, (char *)configuration->object_name, 64);
		aem_print_level(level, " name:         %s\n", string);
		aem_print_level(level, " descriptors:  %d\n", ntohs(configuration->descriptor_counts_count));

		for (i = 0; i < ntohs(configuration->descriptor_counts_count); i++) {
			type = ntohs(configuration->descriptors_counts[2 * i]);

			desc_handler[type].print(desc_handler, aem_desc, level + 1, DESC_MIN, DESC_MAX);
		}
	}
}

void audio_unit_desc_print(struct aem_desc_handler *desc_handler, struct aem_desc_hdr *aem_desc, int level, int min, int max)
{
	struct audio_unit_descriptor *audio_unit;
	char string[128];
	unsigned short len;
	int i, i_max;
	int _min, _max;

	i_max = aem_get_descriptor_max(aem_desc, AEM_DESC_TYPE_AUDIO_UNIT);

	if (i_max > max)
		i_max = max;

	for (i = min; i < i_max; i++) {
		audio_unit = aem_get_descriptor(aem_desc, AEM_DESC_TYPE_AUDIO_UNIT, i, &len);
		if (!audio_unit)
			continue;

		aem_print_level(level, "Audio Unit: %u\n", i);
		h_strncpy(string, (void *)audio_unit->object_name, 64);
		aem_print_level(level, " name:            %s\n", string);
		aem_print_level(level, " stream input port:    %u\n", ntohs(audio_unit->number_of_stream_input_ports));
		aem_print_level(level, " stream output port:   %u\n", ntohs(audio_unit->number_of_stream_output_ports));
		aem_print_level(level, " external input port:  %u\n", ntohs(audio_unit->number_of_external_input_ports));
		aem_print_level(level, " external output port: %u\n", ntohs(audio_unit->number_of_external_output_ports));
		aem_print_level(level, " internal input port:  %u\n", ntohs(audio_unit->number_of_internal_input_ports));
		aem_print_level(level, " internal output port: %u\n", ntohs(audio_unit->number_of_internal_output_ports));

		_min = ntohs(audio_unit->base_stream_input_port);
		_max = _min + ntohs(audio_unit->number_of_stream_input_ports);
		desc_handler[AEM_DESC_TYPE_STREAM_PORT_INPUT].print(desc_handler, aem_desc, level + 1, _min, _max);

		_min = ntohs(audio_unit->base_stream_output_port);
		_max = _min + ntohs(audio_unit->number_of_stream_output_ports);
		desc_handler[AEM_DESC_TYPE_STREAM_PORT_OUTPUT].print(desc_handler, aem_desc, level + 1, _min, _max);
	}
}

void video_unit_desc_print(struct aem_desc_handler *desc_handler, struct aem_desc_hdr *aem_desc, int level, int min, int max)
{
	struct video_unit_descriptor *video_unit;
	char string[128];
	unsigned short len;
	int i, i_max;
	int _min, _max;

	i_max = aem_get_descriptor_max(aem_desc, AEM_DESC_TYPE_VIDEO_UNIT);

	if (i_max > max)
		i_max = max;

	for (i = min; i < i_max; i++) {
		video_unit = aem_get_descriptor(aem_desc, AEM_DESC_TYPE_VIDEO_UNIT, i, &len);
		if (!video_unit)
			continue;

		aem_print_level(level, "Video Unit: %u\n", i);
		h_strncpy(string, (void *)video_unit->object_name, 64);
		aem_print_level(level, " name:            %s\n", string);
		aem_print_level(level, " stream input port:    %u\n", ntohs(video_unit->number_of_stream_input_ports));
		aem_print_level(level, " stream output port:   %u\n", ntohs(video_unit->number_of_stream_output_ports));
		aem_print_level(level, " external input port:  %u\n", ntohs(video_unit->number_of_external_input_ports));
		aem_print_level(level, " external output port: %u\n", ntohs(video_unit->number_of_external_output_ports));
		aem_print_level(level, " internal input port:  %u\n", ntohs(video_unit->number_of_internal_input_ports));
		aem_print_level(level, " internal output port: %u\n", ntohs(video_unit->number_of_internal_output_ports));

		_min = ntohs(video_unit->base_stream_input_port);
		_max = _min + ntohs(video_unit->number_of_stream_input_ports);
		desc_handler[AEM_DESC_TYPE_STREAM_PORT_INPUT].print(desc_handler, aem_desc, level + 1, _min, _max);

		_min = ntohs(video_unit->base_stream_output_port);
		_max = _min + ntohs(video_unit->number_of_stream_output_ports);
		desc_handler[AEM_DESC_TYPE_STREAM_PORT_OUTPUT].print(desc_handler, aem_desc, level + 1, _min, _max);
	}
}

void stream_input_desc_print(struct aem_desc_handler *desc_handler, struct aem_desc_hdr *aem_desc, int level, int min, int max)
{
	struct stream_descriptor *stream;
	char string[128];
	unsigned short len;
	int i, i_max;

	i_max = aem_get_descriptor_max(aem_desc, AEM_DESC_TYPE_STREAM_INPUT);

	if (i_max > max)
		i_max = max;

	for (i = min; i < i_max; i++) {
		stream = aem_get_descriptor(aem_desc, AEM_DESC_TYPE_STREAM_INPUT, i, &len);
		if (!stream)
			continue;

		aem_print_level(level, "Stream Input: %u\n", i);
		h_strncpy(string, (void *)stream->object_name, 64);
		aem_print_level(level, " name:    %s\n", string);
	}
}

void stream_input_desc_update_name(struct aem_desc_hdr *aem_desc, char *names)
{
	struct stream_descriptor *stream;
	char *string;
	unsigned short len;
	int i, i_max;

	i_max = aem_get_descriptor_max(aem_desc, AEM_DESC_TYPE_STREAM_INPUT);

	string = strtok(names, ";");
	for (i = 0; i < i_max; i++) {
		stream = aem_get_descriptor(aem_desc, AEM_DESC_TYPE_STREAM_INPUT, i, &len);
		if (!stream)
			continue;

		if (!string)
			break;

		/* Clear destination */
		memset((char *)stream->object_name, 0, 64);

		len = strlen(string);
		if (len > 64)
			len = 64;

		memcpy((void *)stream->object_name, string, len);

		string = strtok(NULL, ";");
	}
}

void stream_output_desc_print(struct aem_desc_handler *desc_handler, struct aem_desc_hdr *aem_desc, int level, int min, int max)
{
	struct stream_descriptor *stream;
	char string[128];
	unsigned short len;
	int i, i_max;

	i_max = aem_get_descriptor_max(aem_desc, AEM_DESC_TYPE_STREAM_OUTPUT);

	if (i_max > max)
		i_max = max;

	for (i = min; i < i_max; i++) {
		stream = aem_get_descriptor(aem_desc, AEM_DESC_TYPE_STREAM_OUTPUT, i, &len);
		if (!stream)
			continue;

		aem_print_level(level, "Stream Output: %u\n", i);
		h_strncpy(string, (void *)stream->object_name, 64);
		aem_print_level(level, " name:    %s\n", string);
	}
}

void stream_output_desc_update_name(struct aem_desc_hdr *aem_desc, char *names)
{
	struct stream_descriptor *stream;
	char *string;
	unsigned short len;
	int i, i_max;

	i_max = aem_get_descriptor_max(aem_desc, AEM_DESC_TYPE_STREAM_OUTPUT);

	string = strtok(names, ";");
	for (i = 0; i < i_max; i++) {
		stream = aem_get_descriptor(aem_desc, AEM_DESC_TYPE_STREAM_OUTPUT, i, &len);
		if (!stream)
			continue;

		if (!string)
			break;

		/* Clear destination */
		memset((char *)stream->object_name, 0, 64);

		len = strlen(string);
		if (len > 64)
			len = 64;

		memcpy((void *)stream->object_name, string, len);

		string = strtok(NULL, ";");
	}
}

void control_desc_print(struct aem_desc_handler *desc_handler, struct aem_desc_hdr *aem_desc, int level, int min, int max)
{
	struct control_descriptor *control;
	char string[128];
	unsigned short len;
	int i, i_max;

	i_max = aem_get_descriptor_max(aem_desc, AEM_DESC_TYPE_CONTROL);

	if (i_max > max)
		i_max = max;

	for (i = min; i < i_max; i++) {
		control = aem_get_descriptor(aem_desc, AEM_DESC_TYPE_CONTROL, i, &len);
		if (!control)
			continue;

		aem_print_level(level, "Control: %u\n", i);
		h_strncpy(string, (void *)control->object_name, 64);
		aem_print_level(level, " name:    %s\n", string);
	}
}
