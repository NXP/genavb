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

#ifndef _OS_STRING_H_
#define _OS_STRING_H_

#include "os/sys_types.h"


/** fills memory area with constant byte value
 * \return	pointer to the start of the filled memory area
 * \param s	pointer to the first byte of the memory are to fill (d)
 * \param c	filling value
 * \param n	length in bytes of the memory area to set
 */
void *os_memset(void *s, int c, size_t n);


/** copy a memory area
 * \return	pointer to the start of the copy destination area (d)
 * \param d	pointer to the memory area to copy to
 * \param s	pointer to the memory area o copy from
 * \param n	numbers of bytes to copy
 */
void *os_memcpy(void *d, const void *s, size_t n);

/** move a memory area
 * \return	pointer to the start of the copy destination area (d)
 * \param d	pointer to the memory area to move to
 * \param s	pointer to the memory area to move from
 * \param n	numbers of bytes to move
 */
void *os_memmove(void *d, const void *s, size_t n);

/** compare two memory areas
 * \return 	0 if memory areas match, return <0 or >0 if different
 * \param s1	pointer to the first memory area
 * \param s2	pointer to the second memory area
 * \param n	number of bytes to compare
 */
int os_memcmp(const void *s1, const void *s2, size_t n);

/** Determines the length of the NULL-terminated character string.
 *
 * \return	number of bytes in the string pointed to by s, excluding the terminating null byte ('\0'), but at most maxlen.
 * \param s			NULL-terminated string whose length must be determined.
 * \param maxlen	Maximum value to be returned: strnlen will only look at the first maxlen bytes of s.
 */
int os_strnlen(const char *s, size_t maxlen);

#endif /* _OS_STRING_H_ */
