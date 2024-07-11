/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2020-2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Linux specific Network service implementation
 @details
*/

#ifndef _LINUX_NET_H_
#define _LINUX_NET_H_

#include "os/sys_types.h"
#include "osal/net.h"
#include "os_config.h"

int net_init(struct os_net_config *config, struct os_xdp_config *xdp_config);
void net_exit(void);

int net_dflt_port_status(struct net_tx *tx, unsigned int port_id, bool *up, bool *point_to_point, unsigned int *rate);
int net_std_port_status(unsigned int port_id, bool *up, bool *point_to_point, unsigned int *rate);
int net_std_add_multi(struct net_rx *rx, unsigned int port_id, const unsigned char *hw_addr);
int net_std_del_multi(struct net_rx *rx, unsigned int port_id, const unsigned char *hw_addr);
int net_port_sr_config(unsigned int port_id, uint8_t *sr_class);
void net_std_rx_parser(struct net_rx *rx, struct net_rx_desc *desc);
int bridge_software_maclearn(bool enable);

#endif /* _LINUX_NET_H_ */
