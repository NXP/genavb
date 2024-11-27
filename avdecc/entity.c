/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @brief AECP common code
 @details Handles AECP stack
*/

#include "common/types.h"
#include "common/net.h"

#include "genavb/aem.h"

#include "aem.h"

#include "entity.h"

AEM_ENTITY_STORAGE();

static struct aem_desc_hdr aem_desc[AEM_NUM_DESC_TYPES] = {{0, }, };

__init struct aem_desc_hdr *aem_entity_static_init(void)
{
	AEM_ENTITY_INIT(aem_desc);

	aem_entity_desc_fixup(aem_desc);
	aem_configuration_desc_fixup(aem_desc);
	aem_video_cluster_desc_fixup(aem_desc);
	aem_stream_desc_fixup(aem_desc);

	return aem_desc;
}
