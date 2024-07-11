/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "rtos_abstraction_layer.h"
#include "log.h"

#include "aem_manager_helpers.h"

#include "genavb/aem.h"
#include "genavb/aem_helpers.h"

#include "genavb_sdk.h"

void aem_print_level(int level, const char *format, ...)
{
	int i;
	va_list ap;

	for (i = 0; i < level; i++)
		sdk_printf("\t");

	va_start(ap, format);

	sdk_vprintf(format, ap);

	va_end(ap);
}

struct aem_desc_hdr *aem_entity_load_from_reference_entity(struct aem_desc_hdr *aem_src)
{
	struct aem_desc_hdr *aem_desc;
	unsigned int offset = 0, len;
	size_t total_desc_size = 0;
	int i;

	for (i = 0; i < AEM_NUM_DESC_TYPES; i++)
		total_desc_size += aem_src[i].total * aem_src[i].size;

	aem_desc = rtos_malloc(AEM_NUM_DESC_TYPES * sizeof(struct aem_desc_hdr) + total_desc_size);
	if (!aem_desc) {
		aem_print_level(0, "entity() malloc() failed\n");
		goto err_malloc;
	}

	memset(aem_desc, 0, AEM_NUM_DESC_TYPES * sizeof(struct aem_desc_hdr) + total_desc_size);

	for (i = 0; i < AEM_NUM_DESC_TYPES; i++) {
		memcpy(&aem_desc[i].total, &aem_src[i].total, sizeof(avb_u16));

		if (aem_desc[i].total > AEM_DESC_MAX_NUM) {
			aem_print_level(0, "entity() desc header(%d): invalid total desc number: %d\n", i, aem_desc[i].total);
			goto err_read;
		}

		memcpy(&aem_desc[i].size, &aem_src[i].size, sizeof(avb_u16));

		if (aem_desc[i].size > AEM_DESC_MAX_LENGTH) {
			aem_print_level(0, "entity() desc header(%d): invalid length: %d\n", i, aem_desc[i].size);
			goto err_read;
		}
	}

	offset = AEM_NUM_DESC_TYPES * sizeof(struct aem_desc_hdr);

	for (i = 0; i < AEM_NUM_DESC_TYPES; i++) {
		aem_desc[i].ptr = (char *)aem_desc + offset;

		len = aem_desc[i].size * aem_desc[i].total;

		memcpy(aem_desc[i].ptr, aem_src[i].ptr, len);

		offset += len;
	}

	aem_print_level(0, "Loaded AVDECC entity()\n");

	return aem_desc;

err_read:
	rtos_free(aem_desc);

err_malloc:
	return NULL;
}

struct aem_desc_handler desc_handler[AEM_NUM_DESC_TYPES] = {
	[AEM_DESC_TYPE_ENTITY] = {
		.fixup = aem_entity_desc_fixup,
		.check = entity_desc_check,
		.print = entity_desc_print,
		.update_name = entity_desc_update_name,
	},

	[AEM_DESC_TYPE_CONFIGURATION] = {
		.fixup = aem_configuration_desc_fixup,
		.check = configuration_desc_check,
		.print = configuration_desc_print,
	},

	[AEM_DESC_TYPE_AUDIO_UNIT] = {
		.print = audio_unit_desc_print,
	},

	[AEM_DESC_TYPE_VIDEO_UNIT] = {
		.print = video_unit_desc_print,
	},

	[AEM_DESC_TYPE_STREAM_INPUT] = {
		.print = stream_input_desc_print,
		.update_name = stream_input_desc_update_name,
	},

	[AEM_DESC_TYPE_STREAM_OUTPUT] = {
		.print = stream_output_desc_print,
		.update_name = stream_output_desc_update_name,
	},

	[AEM_DESC_TYPE_VIDEO_CLUSTER] = {
		.fixup = aem_video_cluster_desc_fixup,
	},

	[AEM_DESC_TYPE_CONTROL] = {
		.print = control_desc_print,
	},

};

int aem_entity_create(struct aem_desc_hdr *aem_desc, void (*entity_init)(struct aem_desc_hdr *aem_desc))
{
	int rc = 0;

	entity_init(aem_desc);

	aem_entity_fixup(desc_handler, aem_desc);

	if (aem_entity_check(desc_handler, aem_desc) < 0) {
		aem_print_level(0, "aem_desc(%p) failed to create entity\n", aem_desc);
		rc = -1;
		goto out;
	}

out:
	return rc;
}

void aem_entity_free(struct aem_desc_hdr *aem_desc)
{
	rtos_free(aem_desc);
}
