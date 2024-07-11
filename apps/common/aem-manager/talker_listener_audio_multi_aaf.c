/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @brief Talker + Listener entity
 @details Talker + Listener AVDECC entity definition with eight input streams + eight output streams and AAF format
*/

#include "genavb/adp.h"
#include "genavb/aem_helpers.h"

#include "talker_listener_audio_multi_aaf.h"

AEM_ENTITY_STORAGE();

void talker_listener_audio_multi_aaf_init(struct aem_desc_hdr *aem_desc)
{
	AEM_ENTITY_INIT(aem_desc);
}
