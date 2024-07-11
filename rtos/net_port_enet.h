/*
* Copyright 2019-2020, 2022-2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief RTOS specific Network service implementation
 @details
*/

#ifndef _RTOS_NET_PORT_ENET_H_
#define _RTOS_NET_PORT_ENET_H_

#include "net_port.h"

int enet_init(struct net_port *port);

#endif /* _RTOS_NET_PORT_ENET_H_ */
