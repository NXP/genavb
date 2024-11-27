/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2019, 2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Basic types OS abstraction
 @details
*/

#ifndef _COMMON_TYPES_H_
#define _COMMON_TYPES_H_

#include "genavb/types.h"
#include "genavb/net_types.h"
#include "os/sys_types.h"

#define container_of(entry, type, member) ((type *)((unsigned char *)(entry) - offsetof(type, member)))

#define min(a,b)  ((a)<(b)?(a):(b))
#define max(a,b)  ((a)>(b)?(a):(b))

/* The functions below use 32bit accesses to possibly non 32bit aligned addresses,
 * this works for ARMv7 and above CPU's. For other architectures we may need
 * to re-define these functions.
 * Volatile is used when casting to u32 * to prevent compiler from "merging" loads
 * and stores (use ldrd/strd/ldm/stm instructions for possibly unaligned addresses).
 */
static inline u64 get_64(const void *a)
{
#if defined(__BIG_ENDIAN__)
	return (((u64)(*(volatile u32 *)a)) << 32) | ((u64)(*((volatile u32 *)a + 1)));
#elif defined(__LITTLE_ENDIAN__)
	return (((u64)(*((volatile u32 *)a + 1))) << 32) | ((u64)(*(volatile u32 *)a));
#else
	#error
#endif
}

static inline u64 get_48(const void *a)
{
#if defined(__BIG_ENDIAN__)
	return (((u64)(*(volatile u32 *)a)) << 16) | ((u64)(*((u16 *)a + 2)));
#elif defined(__LITTLE_ENDIAN__)
	return (((u64)(*((u16 *)a + 2))) << 32) | ((u64)(*(volatile u32 *)a));
#else
	#error
#endif
}

static inline u64 get_ntohll(const void *a)
{
	return ntohll(get_64(a));
}

static inline u64 get_htonll(const void *a)
{
	return htonll(get_64(a));
}

static inline int cmp_64(const void *a, const void *b)
{
	if ((*(volatile u32 *)a == *(volatile u32 *)b) && (*((volatile u32 *)a + 1) == *((volatile u32 *)b + 1)))
		return 1;
	else
		return 0;
}

static inline void copy_64(void *dst, void const *src)
{
	*(volatile u32 *)dst = *(volatile u32 *)src;
	*((volatile u32 *)dst + 1) = *(volatile u32 *)((volatile u32 *)src + 1);
}

#endif /* _COMMON_TYPES_H_ */
