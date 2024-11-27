/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Standard library OS abstraction
 @details
*/

#ifndef _OS_STDLIB_H_
#define _OS_STDLIB_H_

#include "os/sys_types.h"

/** Allocate new memory block
 * \return	pointer to the allocated memory block
 * \param size	size in bytes of the memory block to allocate
 */
void *os_malloc(size_t size);

/** Free memory allocated by os_malloc()
 * \return	none
 * \param p	pointer to the memory block to free
 */
void os_free(void *p);

/** Generate random value
 * \return	random value between 0 and OS_RAND_MAX
 */
long int os_random(void);

/** compute absolute value from integer
 * \return	the absolute value of the integer argument
 * \param j	integer value
 */
int os_abs(int j);

/** compute absolute value from double
 * \return	the absolute value of the double argument
 * \param j	double value
 */
double os_fabs(double j);

/** compute absolute value from long long integer
 * \return	the absolute value of the long long argument
 * \param j	long long value
 */
long long int os_llabs(long long int j);


#include "osal/stdlib.h"

#endif /* _OS_STDLIB_H_ */
