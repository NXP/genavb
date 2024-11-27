/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Linux specific standard library implementation
 @details
*/

#include <string.h>

#include "os/string.h"

void *os_memset(void *s, int c, size_t n)
{
	return memset(s, c, n);
}

void *os_memcpy(void *d, const void *s, size_t n)
{
	return memcpy(d, s, n);
}

void *os_memmove(void *d, const void *s, size_t n)
{
	return memmove(d, s, n);
}

int os_memcmp(const void *s1, const void *s2, size_t n)
{
	return memcmp(s1, s2, n);
}

int os_strnlen(const char *s, size_t maxlen)
{
	int i = 0;
	const char *c = s;

	while (i < maxlen) {
		if (*c == '\0')
			break;

		c++;
		i++;
	}

	return i;
}
