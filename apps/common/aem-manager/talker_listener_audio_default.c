/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @brief Talker + Listener entity
 @details Talker + Listener AVDECC entity definition with eight input streams + eight output streams and IEC 61883-6 format
*/

#include "genavb/adp.h"
#include "genavb/aem_helpers.h"

#include "talker_listener_audio_default.h"

AEM_ENTITY_STORAGE();

void talker_listener_audio_default_init(struct aem_desc_hdr *aem_desc)
{
	AEM_ENTITY_INIT(aem_desc);
}
