/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2020-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file genavb.h
 \brief GenAVB public API
 \details API definition for the GenAVB library

 \copyright Copyright 2014-2016 Freescale Semiconductor, Inc.
 \copyright Copyright 2016-2018, 2020-2021, 2023 NXP
*/

/**
*
* \defgroup library		Library API
* GenAVB public API definition.
* The public API is made of the following subsets:
* * \ref init "Initialization functions" (open/close library, etc)
* * \ref stream "Streaming functions": create/destroy a stream, send/receive data
* * \ref control "Control functions": open a control handle, send/receive commands through it
* * \ref socket "Socket functions": open/close a socket, send/receive network packets
* * \ref clock "Clock functions": retrieve time
* \if RTOS
* * \ref timer "Timer functions": create/start/stop timers
* * \ref qos "QoS functions": set/get network QoS configuration
* * \ref stream_identification "Stream Identification functions": read/update/delete stream identification entries.
* * \ref vlan "Vlan filtering functions": read/update/delete/dump vlan filtering entries.
* * \ref frer "FRER functions": frame replication and elimination for reliability related functions.
* * \ref psfp "PSFP functions": per-stream filtering and policing related functions.
* \endif
* * \ref generic "Generic functions" (logging, etc)
*
*
* \defgroup other		Additional definitions
* Additional definitions
* * [Network protocol headers](@ref protocol)
* * Kernel includes
*
* \defgroup init 			Init
* \ingroup library
*
* \defgroup stream			Streaming
* \ingroup library
*
* \defgroup control 			Control
* \ingroup library
*
* \defgroup socket			Socket
* \ingroup library
*
* \defgroup clock			Clock
* \ingroup library
*
* \cond RTOS
* \defgroup timer			Timer
* \ingroup library
*
* \defgroup qos				802.1Q qos
* \ingroup library
*
* \defgroup stream_identification	802.1CB stream identification
* \ingroup library
*
* \defgroup vlan			802.1Q VLAN registration
* \ingroup library
*
* \defgroup frer			802.1CB-2017 FRER
* \ingroup library
*
* \defgroup psfp			802.1Q PSFP
* \ingroup library
*
* \endcond
*
* \defgroup generic 			Generic
* \ingroup library
*
* \defgroup protocol			Network protocol headers
* \ingroup other
*
* \defgroup aem				AEM protocol
* AVDECC AECP AEM protocol definitions, from IEEE 1722.1-2013.
* Note: the description of AEM PDUs is based on the official protocol specification. For details on actual usage of those structures
* with the GenAVB stack control API, see @ref control_usage.
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
