/*
 * Copyright 2014 Freescale Semiconductor, Inc.
 * Copyright 2017, 2019, 2021-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Linux static configuration
 @details Contains all compile time configuration options for linux
*/

#ifndef _LINUX_OSAL_CFG_H_
#define _LINUX_OSAL_CFG_H_

#define GPTP_CFG_PRIORITY		58
#define AVTP_CFG_PRIORITY		60
#define SRP_CFG_PRIORITY		58
#define MAAP_CFG_PRIORITY		57
#define AVDECC_CFG_PRIORITY		57
#define MANAGEMENT_CFG_PRIORITY		58
#define STATS_CFG_PRIORITY		49

#define CONFIG_DEFAULT_NET_MODE		NET_AVB
#define CONFIG_API_SOCKETS_DEFAULT_NET  NET_STD

#endif /* _LINUX_OSAL_CFG_H_ */
