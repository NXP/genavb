/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @brief Listener entity
 @details Listener AVDECC entity definition with a single input stream
*/

#include "genavb/adp.h"
#include "genavb/aem_helpers.h"

#include "listener_audio_single.h"

AEM_ENTITY_STORAGE();

void listener_audio_single_init(struct aem_desc_hdr *aem_desc)
{
	AEM_ENTITY_INIT(aem_desc);
}
