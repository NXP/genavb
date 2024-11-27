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

#include "hash.h"

/** Compute a simple 8-bit rotating hash on the given input key.
 * \return	8-bit hash of the input key
 * \param	key	pointer to character array to hash
 * \param	len	length of the key array, in bytes
 * \param	init	initial value of the hash (may be used e.g. to compute a single hash value from separate variables)
 *
 * This function computes an 8-bit hash of the input key, using a simple rotating hash.
 */
u8 rotating_hash_u8(u8 *key, unsigned int len, u8 init)
{
	u8 hash = init;
	unsigned int i;

	for (i = 0; i < len; i++)
		hash = (hash << 3) ^ (hash >> 5) ^ key[i];

	return hash;
}
