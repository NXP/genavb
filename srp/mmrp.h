/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2020-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
  @file		mmrp.h
  @brief	MMRP module interface.
  @details	All prototypes and definitions necessary for MMRP module usage
		are provided within this header file.
*/


#ifndef _MMRP_H_
#define _MMRP_H_

#include "common/net.h"

#include "srp/mrp.h"



/**
 * MMRP global context structure
 */
struct mmrp_ctx {
	struct srp_ctx *srp;
	unsigned int num_rx_pkts;
	unsigned int num_tx_pkts;
};


int mmrp_init(struct mmrp_ctx *mrp);
int mmrp_exit(struct mmrp_ctx *mrp);
void mmrp_process_packet(struct mmrp_ctx *mmrp, unsigned int port_id, struct net_rx_desc *desc);

#endif /* _MMRP_H_ */
