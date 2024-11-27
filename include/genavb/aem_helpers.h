/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file aem_helpers.h
 \brief AEM helper functions
 \details Helper functions to handle AEM structures

 \copyright Copyright 2014-2016 Freescale Semiconductor, Inc.
 \copyright Copyright 2016, 2023 NXP
*/

#ifndef _GENAVB_PUBLIC_AEM_HELPERS_H_
#define _GENAVB_PUBLIC_AEM_HELPERS_H_

#include "aem.h"

struct aem_desc_hdr {
	void *ptr;
	avb_u16 size;
	avb_u16 total;
};

unsigned int aem_get_descriptor_max(struct aem_desc_hdr *aem_desc, avb_u16 type);
void *aem_get_descriptor(struct aem_desc_hdr *aem_desc, avb_u16 type, avb_u16 index, avb_u16 *len);
void aem_entity_desc_fixup(struct aem_desc_hdr *aem_desc);
void aem_configuration_desc_fixup(struct aem_desc_hdr *aem_desc);
void aem_video_cluster_desc_fixup(struct aem_desc_hdr *aem_desc);
void aem_stream_desc_fixup(struct aem_desc_hdr *aem_desc);
unsigned int aem_get_listener_streams(struct aem_desc_hdr *aem_desc);
unsigned int aem_get_talker_streams(struct aem_desc_hdr *aem_desc);

struct aem_desc_hdr *aem_entity_load_from_file(const char *name);

#endif /* _GENAVB_PUBLIC_AEM_HELPERS_H_ */
