/*
* Copyright 2019-2021, 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
  @file		mac_service.h
  @brief	Definition of MAC service entry point functions and global context structure.
  @details
*/

#ifndef _MAC_SERVICE_H_
#define _MAC_SERVICE_H_

#include "common/ipc.h"

#include "common/timer.h"

#define MAC_STATUS_PERIOD_MS	10

struct mac_port {
	bool operational;	/* MAC_Operational defined in 802.1AC-2016, section 11.2 */
	bool point_to_point;	/* operPointToPointMAC defined in 802.1AC-2016, section 11.3 */
	unsigned int rate;	/* portTransmitRate defined in 802.1Q-2018, section 8.6.8.2 c) */

	unsigned int logical_port;
};


struct mac_service {
	struct timer timer;

	struct management_ctx *management;

	struct net_tx net_tx;

	struct ipc_rx ipc_rx;
	struct ipc_tx ipc_tx_sync;
	struct ipc_tx ipc_tx;

	unsigned int port_max;

	/* variable size array */
	struct mac_port port[];
};

int mac_service_init(struct mac_service *mac, struct management_config *cfg, unsigned long priv);
int mac_service_exit(struct mac_service *mac);

#endif /* _MAC_SERVICE_H_ */
