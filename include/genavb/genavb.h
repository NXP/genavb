/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2020-2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file genavb.h
 \brief GenAVB/TSN public API
 \details API definition for the GenAVB/TSN library

 \copyright Copyright 2014-2016 Freescale Semiconductor, Inc.
 \copyright Copyright 2016-2018, 2020-2021, 2023-2024 NXP
*/

/**
*
* \defgroup library		Library API
* GenAVB/TSN public API definition.
* The public API is made of the following subsets:
* * \ref stream "Streaming functions": create/destroy a stream, send/receive data
* * \ref control "Control functions": open a control handle, send/receive commands through it
* * \ref socket "Socket functions": open/close a socket, send/receive network packets
*
* \defgroup other		Additional definitions
* Additional definitions
* * [Network protocol headers](@ref protocol)
* * Kernel includes

* \defgroup stream			Streaming
* \ingroup library
*
* \defgroup control 			Control
* \ingroup library
*
* \defgroup protocol			Network protocol headers
* \ingroup other
*
* \defgroup aem				AEM protocol
* AVDECC AECP AEM protocol definitions, from IEEE 1722.1-2013.
* Note: the description of AEM PDUs is based on the official protocol specification. For details on actual usage of those structures
* with the GenAVB/TSN stack control API, see @ref control_usage.
* \ingroup protocol
*/

#ifndef _GENAVB_PUBLIC_API_H_
#define _GENAVB_PUBLIC_API_H_

#include "init.h"
#include "control_avdecc.h"
#include "control_srp.h"
#include "control_clock_domain.h"
#include "control_gptp.h"
#include "control_maap.h"
#include "streaming.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "types.h"
#include "error.h"
#include "avdecc.h"
#include "srp.h"
#include "avtp.h"
#include "sr_class.h"


#ifdef __cplusplus
}
#endif

#endif /* _GENAVB_PUBLIC_API_H_ */
