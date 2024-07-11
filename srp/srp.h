/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2018-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
  @file		srp.h
  @brief	SRP module common definitions
  @details	prototypes and definitions for the SRP common code (module entry point and external interfaces)
  		are provided within this header file.
*/


#ifndef _SRP_H_
#define _SRP_H_


#include "common/net.h"
#include "common/ipc.h"
#include "common/timer.h"

#include "srp_managed_objects.h"

#include "msrp.h"
#include "mvrp.h"
#include "mmrp.h"

/**
 * SRP port context structure
 */
struct srp_port {
	unsigned int port_id;
	bool initialized;
	struct net_rx net_rx; /**< network rx context */
	struct net_tx net_tx; /**< network tx context */
};

/**
 * SRP global context structure
 */
struct srp_ctx {
	/* Managed objects data tree */
	struct srp_managed_objects module;

	struct msrp_ctx *msrp; /**< MSRP module context */
	struct mvrp_ctx *mvrp; /**< MVRP module context */
	struct mmrp_ctx mmrp; /**< MMRP module context */
	struct timer_ctx *timer_ctx; /**< timer context */

	struct ipc_rx ipc_rx_mac_service;
	struct ipc_tx ipc_tx_mac_service;

	bool management_enabled;

	unsigned int port_max;

	/* variable size array */
	struct srp_port port[];
};

void srp_net_rx(struct net_rx *, struct net_rx_desc *);
void srp_ipc_rx_avdecc(struct ipc_rx const *, struct ipc_desc *);

void srp_ipc_managed_get(struct srp_ctx *srp, struct ipc_tx *ipc, unsigned int ipc_dst, uint8_t *in, uint8_t *in_end);
void srp_ipc_managed_set(struct srp_ctx *srp, struct ipc_tx *ipc, unsigned int ipc_dst, uint8_t *in, uint8_t *in_end);

#endif /* _SRP_H_ */
