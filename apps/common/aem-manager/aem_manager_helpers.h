/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file aem_manager_helpers.h
 \brief AEM Manager helper functions
 \details Helper functions to handle AEM Manager structures

 \copyright Copyright 2014-2016 Freescale Semiconductor, Inc.
 \copyright Copyright 2023-2024 NXP
 All Rights Reserved.
*/

#ifndef _COMMON_AEM_MANAGER_HELPERS_H_
#define _COMMON_AEM_MANAGER_HELPERS_H_

#include "genavb/aem.h"
#include "genavb/aem_helpers.h"

struct aem_desc_handler {
	void (*print)(struct aem_desc_handler *desc_handler, struct aem_desc_hdr *aem_desc, int level, int min, int max);
	void (*fixup)(struct aem_desc_hdr *aem_desc);
	int (*check)(struct aem_desc_hdr *aem_desc);
	void (*update_name)(struct aem_desc_hdr *aem_desc, char *name);
};

int entity_desc_handler_init(struct aem_desc_handler *desc_handler);
int entity_desc_check(struct aem_desc_hdr *aem_desc);
int configuration_desc_check(struct aem_desc_hdr *aem_desc);
void aem_entity_fixup(struct aem_desc_handler *desc_handler, struct aem_desc_hdr *aem_desc);
int aem_entity_check(struct aem_desc_handler *desc_handler, struct aem_desc_hdr *aem_desc);

void entity_desc_print(struct aem_desc_handler *desc_handler, struct aem_desc_hdr *aem_desc, int level, int min, int max);
void entity_desc_update_name(struct aem_desc_hdr *aem_desc, char *name);
void configuration_desc_print(struct aem_desc_handler *desc_handler, struct aem_desc_hdr *aem_desc, int level, int min, int max);
void audio_unit_desc_print(struct aem_desc_handler *desc_handler, struct aem_desc_hdr *aem_desc, int level, int min, int max);
void video_unit_desc_print(struct aem_desc_handler *desc_handler, struct aem_desc_hdr *aem_desc, int level, int min, int max);
void stream_input_desc_print(struct aem_desc_handler *desc_handler, struct aem_desc_hdr *aem_desc, int level, int min, int max);
void stream_input_desc_update_name(struct aem_desc_hdr *aem_desc, char *names);
void stream_output_desc_print(struct aem_desc_handler *desc_handler, struct aem_desc_hdr *aem_desc, int level, int min, int max);
void stream_output_desc_update_name(struct aem_desc_hdr *aem_desc, char *names);
void control_desc_print(struct aem_desc_handler *desc_handler, struct aem_desc_hdr *aem_desc, int level, int min, int max);

#endif /* _COMMON_AEM_MANAGER_HELPERS_H_ */
