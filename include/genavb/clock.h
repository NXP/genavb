/*
 * Copyright 2019-2020, 2022-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file clock.h
 \brief GenAVB public API
 \details clock definitions.

 \copyright Copyright 2019-2020, 2022-2023 NXP
*/
#ifndef _GENAVB_PUBLIC_CLOCK_H_
#define _GENAVB_PUBLIC_CLOCK_H_

#include "types.h"

/**
 * \ingroup clock
 * GenAVB clock type.
 */
typedef enum {
	GENAVB_CLOCK_MONOTONIC,		/**< Monotonic system clock */
	GENAVB_CLOCK_GPTP_0_0,		/**< gPTP clock interface 0 domain 0 */
	GENAVB_CLOCK_GPTP_0_1,		/**< gPTP clock interface 0 domain 1 */
	GENAVB_CLOCK_GPTP_1_0,		/**< gPTP clock interface 1 domain 0 */
	GENAVB_CLOCK_GPTP_1_1,		/**< gPTP clock interface 1 domain 1 */
	GENAVB_CLOCK_BR_0_0,		/**< gPTP clock bridge 0 domain 0 */
	GENAVB_CLOCK_BR_0_1,		/**< gPTP clock bridge 0 domain 1 */
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

#endif /* _GENAVB_PUBLIC_CLOCK_H_ */
