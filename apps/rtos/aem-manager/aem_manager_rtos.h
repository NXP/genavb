/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _AEM_MANAGER_COMMON_RTOS_H_
#define _AEM_MANAGER_COMMON_RTOS_H_

#include "genavb/aem_helpers.h"

struct aem_desc_hdr *aem_entity_load_from_reference_entity(struct aem_desc_hdr *aem_src);

int aem_entity_create(struct aem_desc_hdr *aem_desc, void (*entity_init)(struct aem_desc_hdr *aem_desc));
void aem_entity_free(struct aem_desc_hdr *aem_desc);

#endif /* _AEM_MANAGER_COMMON_RTOS_H_ */
