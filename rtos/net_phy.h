/*
* Copyright 2017-2020, 2022-2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief RTOS specific Network service implementation
 @details
*/

#ifndef _RTOS_NET_PHY_H_
#define _RTOS_NET_PHY_H_

#include "net_port.h"

typedef void (*phy_callback_t)(void *data, unsigned int event, unsigned int speed, unsigned int duplex);

void phy_port_status(unsigned int phy_id, struct net_port_status *status);
void phy_get_ts_latency(unsigned int phy_id, uint32_t *rx_latency, uint32_t *tx_latency);
int phy_connect(unsigned int phy_id, phy_callback_t callback, void *data);
void phy_disconnect(unsigned int phy_id);
int phy_init(void);
void phy_exit(void);

#endif /* _RTOS_NET_PHY_H_ */
