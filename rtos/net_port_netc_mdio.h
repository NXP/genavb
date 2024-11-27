/*
* Copyright 2022-2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief RTOS specific Network service implementation
 @details
*/

#ifndef _RTOS_NET_PORT_NETC_MDIO_H_
#define _RTOS_NET_PORT_NETC_MDIO_H_

int netc_emdio_init(struct net_mdio *mdio);
int netc_port_emdio_init(struct net_mdio *mdio);

#endif /* _RTOS_NET_PORT_NETC_MDIO_H_ */
