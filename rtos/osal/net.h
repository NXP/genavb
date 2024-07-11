/*
 * Copyright 2018-2019, 2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief RTOS specific NET implementation
 @details
*/
#ifndef _RTOS_OSAL_NET_H_
#define _RTOS_OSAL_NET_H_

#include "common/net_types.h"

#define DEFAULT_NET_DATA_SIZE		1600

struct net_tx {
	void (*func_tx_ts)(struct net_tx *, uint64_t ts, unsigned int private);

	void *socket;

	unsigned int port_id;
};

struct net_rx {
	void (*func)(struct net_rx *, struct net_rx_desc *);
	void (*func_multi)(struct net_rx *, struct net_rx_desc **, unsigned int);

	void *socket;

	unsigned int batch;
};

int net_rx_set_callback(struct net_rx*, void (*callback)(void *), void *data);
int net_rx_enable_callback(struct net_rx *rx);
int net_tx_set_callback(struct net_tx*, void (*callback)(void *), void *data);
int net_tx_enable_callback(struct net_tx *tx);

#endif /* _RTOS_OSAL_NET_H_ */
