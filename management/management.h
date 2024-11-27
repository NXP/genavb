/*
* Copyright 2019-2021, 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
  @file		management.h
  @brief	Management module common definitions
  @details	prototypes and definitions for the Management common code (module entry point and external interfaces)
  		are provided within this header file.
*/


#ifndef _MANAGEMENT_H_
#define _MANAGEMENT_H_

#include "common/net.h"
#include "common/timer.h"
#include "common/ipc.h"

#include "mac_service.h"

#include "management_entry.h"

/**
 * Management global context structure
 */
struct management_ctx {
	struct timer_ctx *timer_ctx; /**< timer context */

	struct mac_service *mac;
};

#endif /* _MANAGEMENT_H_ */
