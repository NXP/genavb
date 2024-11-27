/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __AUDIO_MAPPINGS_H__
#define __AUDIO_MAPPINGS_H__

#include <genavb/genavb.h>

int audio_mappings_init(void);
int audio_mappings_remove(avb_u16 desc_index, avb_u16 desc_type, avb_u16 mappings_count, void *audio_mappings);
int audio_mappings_add(avb_u16 desc_index, avb_u16 desc_type, avb_u16 mappings_count, void *audio_mappings);
int audio_mappings_get(avb_u16 desc_index, avb_u16 desc_type, avb_u16 map_index, avb_u16 *number_of_maps, avb_u16 *mappings_count, void *audio_mappings);

#endif /* __AUDIO_MAPPINGS_H__ */
