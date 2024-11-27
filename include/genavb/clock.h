/*
 * Copyright 2019-2020, 2022-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file clock.h
 \brief GenAVB/TSN public API
 \details clock definitions.

 \copyright Copyright 2019-2020, 2022-2024 NXP
*/
#ifndef _GENAVB_PUBLIC_CLOCK_H_
#define _GENAVB_PUBLIC_CLOCK_H_

#include "types.h"

/**
 * \defgroup clock			Clock
 * \ingroup library
 *
 * The clock API offers a high-level API to retrieve the current time for various clocks.
 *
 * ::GENAVB_CLOCK_MONOTONIC is based on a monotonic system time and should be used when a continous time is needed.
 * ::GENAVB_CLOCK_GPTP_0_0 and ::GENAVB_CLOCK_GPTP_0_1 are gPTP clocks. Respectively domain 0 and 1 for interface 0
 * ::GENAVB_CLOCK_GPTP_1_0 and ::GENAVB_CLOCK_GPTP_1_1 are gPTP clocks. Respectively domain 0 and 1 for interface 1
 *
 * Specificities of the time returned by gPTP clocks:
 * it has no guarantee to be monotonic and discontinuities may happen
 * it is not guaranteed to be synchronized to gPTP Grandmaster. Making sure local system is well synchronized to the gPTP Grandmaster is out-of-scope of this API.
 *
 * The current time is retrieved using ::genavb_clock_gettime64 function which provides a 64 bits nanoseconds time value.
 */

/**
 * \ingroup clock
 * GenAVB/TSN clock type.
 */
typedef enum {
	GENAVB_CLOCK_MONOTONIC,		/**< Monotonic system clock */
	GENAVB_CLOCK_GPTP_0_0,		/**< gPTP clock interface 0 domain 0 */
	GENAVB_CLOCK_GPTP_0_1,		/**< gPTP clock interface 0 domain 1 */
	GENAVB_CLOCK_GPTP_1_0,		/**< gPTP clock interface 1 domain 0 */
	GENAVB_CLOCK_GPTP_1_1,		/**< gPTP clock interface 1 domain 1 */
	GENAVB_CLOCK_BR_0_0,		/**< gPTP clock bridge 0 domain 0 */
	GENAVB_CLOCK_BR_0_1,		/**< gPTP clock bridge 0 domain 1 */
	GENAVB_CLOCK_LOCAL_BR_0,	/**< local clock bridge 0 */
	GENAVB_CLOCK_MAX
} genavb_clock_id_t;

/**
 * Get time in nanoseconds.
 * \ingroup clock
 * \return	::GENAVB_SUCCESS or negative error code.
 * \param id	clock id.
 * \param ns	pointer to uint64_t variable that will hold the result.
 */
int genavb_clock_gettime64(genavb_clock_id_t id, uint64_t *ns);

/**
 * Add the given offset to the clock.
 * \ingroup clock
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param id		clock id.
 * \param offset	offset to add to the clock, in ns.
 */
int genavb_clock_setoffset(genavb_clock_id_t id, int64_t offset);

/**
 * Adjust the clock frequency by the given amount.
 * \ingroup clock
 * \return	::GENAVB_SUCCESS or negative error code.
 * \param id	clock id.
 * \param ppb	adjustment to apply to the nominal clock, in parts per billion.
 */
int genavb_clock_setfreq(genavb_clock_id_t id, int32_t ppb);

/**
 * Converts time from one clock base to another.
 * Both clocks must be based on the same hardware clock.
 * \ingroup clock
 * \return	::GENAVB_SUCCESS or negative error code.
 * \param clk_id_src	source clock id.
 * \param ns_src	source time
 * \param clk_id_dst	destination clock id
 * \param ns_dst	pointer to destination time.
 */
int genavb_clock_convert(genavb_clock_id_t clk_id_src, uint64_t ns_src, genavb_clock_id_t clk_id_dst, uint64_t *ns_dst);


#endif /* _GENAVB_PUBLIC_CLOCK_H_ */
