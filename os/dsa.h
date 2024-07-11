/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief DSA OS abstraction
 @details
*/

#ifndef _OS_DSA_H_
#define _OS_DSA_H_

#include "os/sys_types.h"

int net_port_dsa_add(unsigned int cpu_port, uint8_t *mac_addr, unsigned int slave_port);
int net_port_dsa_delete(unsigned int slave_port);

#endif /* _OS_DSA_H_ */
