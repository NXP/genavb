/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2018-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
  @file		mvrp.h
  @brief	MVRP module interface.
  @details	All prototypes and definitions necessary for MVRP module usage
		are provided within this header file.
*/


#ifndef _MVRP_H_
#define _MVRP_H_

#include "common/net.h"
#include "common/ipc.h"

#include "mrp.h"

/* 802.1Q, section 10.3.1. MAP Context */
#define MVRP_MAX_MAP_CONTEXT 1

/**
 * MVRP client instance definition.
 */
struct mvrp_vlan {
	u16 registered_state;	/**< attribute state registration bit mask (bit per port) */
	u16 declared_state;	/**< attribute state declaration bit mask (bit per port) */
	u16 dynamic_vlan_state; /**< tracks FDB dynamic vlan entry state */
	struct list_head list;
	unsigned short vlan_id;		/**< associated VLAN ID */
	u16 user_declared;			  /**< Boolean bit mask (bit per port); indicates if the instance is declared by the user and not by the MAP */

	/* variable size array */
	unsigned int ref_count[]; /**< number of streams using this vlan */
};

/**
 * MVRP port structure
 */
struct mvrp_port {
	unsigned int port_id;
	unsigned int logical_port;

	struct srp_port *srp_port;

	struct mrp_application mrp_app;
	unsigned int num_rx_pkts;
	unsigned int num_tx_pkts;
	unsigned int num_tx_err;
};

/**
 * MVRP MAP context structure
 */
struct mvrp_map {
	unsigned int map_id;
	struct list_head vlans;
	unsigned int num_vlans;
	u16 forwarding_state; /**< Bitmask (bit per port): state of the port (Forwarding/Discarding/...) */
};

/**
 * MVRP global context structure
 */
struct mvrp_ctx {
	struct mvrp_map map[MVRP_MAX_MAP_CONTEXT];

	struct srp_ctx *srp;

	struct ipc_rx ipc_rx;		/**< receive context for MEDIA STACK IPC */
	struct ipc_tx ipc_tx;		/**< transmit context for MVRP IPC */
	struct ipc_tx ipc_tx_sync;	/**< transmit context for MVRP sync IPC */

	u16 operational_state; /**< Bitmask (bit per port): link state of the port (up/down) */

	unsigned int port_max;
	bool is_bridge;

	/* variable size array */
	struct mvrp_port port[];
};

#define mvrp_attribute_value mvrp_pdu_fv

int mvrp_init(struct mvrp_ctx *mvrp, struct mvrp_config *cfg, unsigned long priv);
int mvrp_exit(struct mvrp_ctx *mvrp);
int mvrp_process_packet(struct mvrp_ctx *mvrp, unsigned int port_id, struct net_rx_desc *desc);
void mvrp_free_vlan(struct mvrp_map *map, struct mvrp_vlan *vlan);
int mvrp_register_vlan_member(struct mvrp_ctx *mvrp, unsigned int port_id, unsigned short vlan_id);
int mvrp_deregister_vlan_member(struct mvrp_ctx *mvrp, unsigned int port_id, unsigned short vlan_id);

void mvrp_port_status(struct mvrp_ctx *mvrp, struct ipc_mac_service_status *status);

#endif /* _MVRP_H_ */
