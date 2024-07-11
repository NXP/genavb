/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @brief AVNU certification entity
 @details Talker + Listener AVDECC entity definition with eight input streams + eight output streams
*/

#include "genavb/adp.h"
#include "genavb/aem_helpers.h"

#include "avnu_certification.h"

AEM_ENTITY_STORAGE();

void avnu_certification_init(struct aem_desc_hdr *aem_desc)
{
	AEM_ENTITY_INIT(aem_desc);
}
