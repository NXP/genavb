/*
 * Copyright 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Linux specific type definitions
 @details
*/

#ifndef _LINUX_OSAL_SYS_TYPES_H_
#define _LINUX_OSAL_SYS_TYPES_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>     /* for offsetof() */
#include <inttypes.h>
#include <unistd.h>     /* for size_t */

#define __init
#define __exit

typedef uint64_t	u64;
typedef uint32_t	u32;
typedef uint16_t	u16;
typedef uint8_t		u8;
typedef int64_t		s64;
typedef int32_t		s32;
typedef int16_t		s16;
typedef int8_t		s8;

/* FIXME this will break for non gcc compilers */
#define unlikely(x)	__builtin_expect(x, 0)
#define likely(x)	__builtin_expect(x, 1)

#endif /* _LINUX_OSAL_SYS_TYPES_H_ */
