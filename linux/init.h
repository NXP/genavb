/*
 * Copyright 2020-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief OS abstraction entry points
 @details
*/

#ifndef _OS_INIT_H_
#define _OS_INIT_H_

#include "os_config.h"

int os_init(struct os_net_config *net_config);
void os_exit(void);

#endif /* _OS_INIT_H_ */
