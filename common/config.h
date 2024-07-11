/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVB global static configuration
 @details Contains all compile time configuration options for the entire AVB stack
*/

#ifndef _CFG_H_
#define _CFG_H_

#include "genavb/config.h"
#include "os/config.h"

#define CFG_LOG		LOG_INFO

#define common_CFG_LOG	CFG_LOG

#define AVTP_CFG_NUM_DOMAINS		4


#define CFG_AVTP_MIN_LATENCY		500000	/* minimum wakeup latency */
#define CFG_AVTP_DEFAULT_LATENCY	1000000	/* default wakeup latency */
#define CFG_AVTP_MAX_LATENCY		20000000 /* maximum wakeup latency */

#endif /* _CFG_H_ */
