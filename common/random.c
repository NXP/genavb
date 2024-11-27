/*
* Copyright 2021, 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief Generic random functions
 @details
*/

#include "random.h"
#include "os/stdlib.h"

/**
 * Random helper function
 * \return a random number in [min : max] (min and max included)
 * \param min
 * \param max
 */
long int random_range(long int min, long int max)
{
	/* rand between MIN and MAX => (rand % (MAX + 1 - MIN)) + MIN */
	return ((os_random() % (max + 1 - min)) + min);
}
