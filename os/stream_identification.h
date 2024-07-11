/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Stream Identification service OS abstraction
 @details
*/

#ifndef _OS_STREAM_IDENTIFICATION_H_
#define _OS_STREAM_IDENTIFICATION_H_

#include "genavb/stream_identification.h"

int stream_identity_update(uint32_t index, struct genavb_stream_identity *entry);
int stream_identity_delete(uint32_t index);
int stream_identity_read(uint32_t index, struct genavb_stream_identity *entry);

#endif /* _OS_STREAM_IDENTIFICATION_H_ */

