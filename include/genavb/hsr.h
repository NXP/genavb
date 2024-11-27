/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief GenAVB/TSN public API
 \details HSR definitions.

 \copyright Copyright 2023-2024 NXP
*/
#ifndef _GENAVB_PUBLIC_HSR_API_H_
#define _GENAVB_PUBLIC_HSR_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

/** \cond RTOS */

/**
 * \defgroup hsr		IEC 62439-3 High-availability Seamless Redundancy
 * \ingroup library
 */

/**
 * \ingroup hsr
 * HSR modes
 * IEC 62439-3 - 5.3.1.1
 */
typedef enum {
	GENAVB_HSR_OPERATION_MODE_H = 0,	/**< mode H(mandatory, default mode) HSR-tagged forwarding */
	GENAVB_HSR_OPERATION_MODE_N,		/**< mode N(optional) No forwarding */
	GENAVB_HSR_OPERATION_MODE_T,		/**< mode T(optional) Transparent forwarding */
	GENAVB_HSR_OPERATION_MODE_U,		/**< mode U(optional) Unicast forwarding */
} genavb_hsr_mode_t;

/** \endcond */

/* OS specific headers */
#include "os/hsr.h"

#ifdef __cplusplus
}
#endif

#endif /* _GENAVB_PUBLIC_HSR_API_H_ */
