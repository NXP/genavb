/*
 * Copyright 2016-2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @brief Talker + Listener test entity
 @details Talker + Listener AVDECC entity definition with eight input streams + eight output streams with several formats and frequencies
*/

#include "genavb/adp.h"
#include "genavb/aem_helpers.h"

#include "talker_listener_audio_multi_format.h"

AEM_ENTITY_STORAGE();

void talker_listener_audio_multi_format_init(struct aem_desc_hdr *aem_desc)
{
	AEM_ENTITY_INIT(aem_desc);
}
