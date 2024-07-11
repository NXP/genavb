/*
 * Copyright 2017, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Generic hashing functions
 @details
*/
#ifndef _HASH_H_
#define _HASH_H_

#include "common/types.h"

u8 rotating_hash_u8(u8 *key, unsigned int len, u8 init);

#endif
