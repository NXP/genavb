/*
 * Copyright 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Clock service OS abstraction
 @details
*/

#ifndef _OS_CLOCK_H_
#define _OS_CLOCK_H_

#include "os/sys_types.h"

#include "osal/clock.h"

#define NSECS_PER_SEC		1000000000
#define NSECS_PER_MS		1000000
#define USECS_PER_SEC		1000000

typedef enum {
	OS_CLOCK_SYSTEM_MONOTONIC = 0, /**< Monotonic system clock */
	OS_CLOCK_SYSTEM_MONOTONIC_COARSE = 1, /**< Monotonic coarse system clock */
	OS_CLOCK_MEDIA_HW_0 = 2,       /**< Media clock hardware domain 0 */
	OS_CLOCK_MEDIA_HW_1 = 3,       /**< Media clock hardware domain 1 */
	OS_CLOCK_MEDIA_REC_0 = 4,      /**< Media clock recovery domain 0 */
	OS_CLOCK_MEDIA_REC_1 = 5,      /**< Media clock recovery domain 1 */
	OS_CLOCK_MEDIA_PTP_0 = 6,      /**< Media clock gPTP 0 */
	OS_CLOCK_MEDIA_PTP_1 = 7,      /**< Media clock gPTP 0 */
	OS_CLOCK_GPTP_EP_0_0 = 8,      /**< gPTP clock endpoint interface 0 domain 0 */
	OS_CLOCK_GPTP_EP_0_1 = 9,      /**< gPTP clock endpoint interface 0 domain 1 */
	OS_CLOCK_GPTP_EP_1_0 = 10,     /**< gPTP clock endpoint interface 1 domain 0 */
	OS_CLOCK_GPTP_EP_1_1 = 11,     /**< gPTP clock endpoint interface 1 domain 1 */
	OS_CLOCK_GPTP_BR_0_0 = 12,     /**< gPTP clock bridge interface 0 domain 0 */
	OS_CLOCK_GPTP_BR_0_1 = 13,     /**< gPTP clock bridge interface 0 domain 1 */
	OS_CLOCK_LOCAL_EP_0 = 14,      /**< Local clock endpoint interface 0 */
	OS_CLOCK_LOCAL_EP_1 = 15,      /**< Local clock endpoint interface 1 */
	OS_CLOCK_LOCAL_BR_0 = 16,      /**< Local clock bridge interface 0 */
	OS_CLOCK_SYSTEM_MONOTONIC_1 = 17,
	OS_CLOCK_MAX
} os_clock_id_t;

typedef enum {
	OS_CLOCK_ADJUST_MODE_HW_RATIO = (1 << 0),		/**< Clock ratio adjustments affect the hardware clock */
	OS_CLOCK_ADJUST_MODE_HW_OFFSET= (1 << 1),	/**< Clock offset adjustments affect the hardware clock */
} os_clock_adjust_mode_t;

/** Adjust the clock frequency by the given amount.
 *  This function may not be supported by the clock.
 * \param id	clock id.
 * \param ppb	adjustment to apply to the nominal clock, in parts per billion.
 * \return	0 on success or -1 on error.
 */
int os_clock_setfreq(os_clock_id_t id, s32 ppb);

/** Add the given offset to the clock.
 *  This function may not be supported by the clock.
 * \param id		clock id.
 * \param offset	offset to add to the clock, in ns.
 * \return		0 on success or -1 on error.
 */
int os_clock_setoffset(os_clock_id_t id, s64 offset);

/**
 * Get time in nanoseconds modulo 2^32.
 * \param id	clock id.
 * \param ns	pointer to u32 variable that will hold the result.
 * \return	0 on success, or negative value on error.
 */
int os_clock_gettime32(os_clock_id_t id, u32 *ns);

/**
 * Get time in nanoseconds.
 * \param id	clock id.
 * \param ns	pointer to u64 variable that will hold the result.
 * \return	0 on success, or negative value on error.
 */
int os_clock_gettime64(os_clock_id_t id, u64 *ns);

/**
 * Converts time from one clock base to another.
 * Both clocks must be based on the same hardware clock.
 * \param id_src source clock id.
 * \param ns_src source time
 * \param id_dst destination clock id
 * \param nd_dst pointer to destination time.
 * \return      0 on success, or negative value and destination time set to zero on error.
 */
int os_clock_convert(os_clock_id_t id_src, u64 ns_src, os_clock_id_t id_dst, u64 *ns_dst);

/**
 * Get local clock id from port id.
 * \param port_id 	port id
 * \return		clock id
 */
os_clock_id_t logical_port_to_local_clock(unsigned int port_id);

/**
 * Get gPTP clock id from port id and  gPTP domain.
 * \param port_id 	port id
 * \param domain 	gPTP domain number
 * \return		clock id
 */
os_clock_id_t logical_port_to_gptp_clock(unsigned int port_id, unsigned int domain);

/**
 * Returns clock adjustment mode.
 * \param id	clock id
 * \return		clock adjustment mode
 */
unsigned int os_clock_adjust_mode(os_clock_id_t id);

#endif /* _OS_CLOCK_H_ */
