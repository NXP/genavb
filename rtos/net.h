/*
 * Copyright 2019-2020, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief RTOS specific Network service implementation
 @details
*/
#ifndef _RTOS_NET_H_
#define _RTOS_NET_H_

#include "genavb/net_types.h"

void net_tx_desc_free(void *data, unsigned long entry);
void net_rx_desc_free(void *data, unsigned long entry);

int net_pool_tx_alloc_multi(struct net_tx_desc **desc, unsigned int n, unsigned int size);
#endif /* _RTOS_NET_H_ */
