/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file stream_identification.c
 \brief GenAVB public API
 \details 802.1CB stream identification

 \copyright Copyright 2023 NXP
*/

#include "genavb/stream_identification.h"
#include "os/stream_identification.h"

int genavb_stream_identification_update(uint32_t index, struct genavb_stream_identity *entry)
{
	return stream_identity_update(index, entry);
}

int genavb_stream_identification_delete(uint32_t index)
{
	return stream_identity_delete(index);
}

int genavb_stream_identification_read(uint32_t index, struct genavb_stream_identity *entry)
{
	return stream_identity_read(index, entry);
}

