/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @brief Talker entity
 @details Talker AVDECC entity definition with one audio and one video streams
*/

#include "genavb/adp.h"
#include "genavb/aem_helpers.h"

#include "talker_audio_video.h"

AEM_ENTITY_STORAGE();

void talker_audio_video_init(struct aem_desc_hdr *aem_desc)
{
	AEM_ENTITY_INIT(aem_desc);
}
