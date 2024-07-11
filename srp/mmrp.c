/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2018-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
  @file		mmrp.c
  @brief	MMRP module implementation.
  @details
*/


#include "os/stdlib.h"
#include "os/string.h"

#include "common/log.h"

#include "mmrp.h"



/** Main MMRP transmit function. Generates PDU according to the action setup by the state machines
 * \return	none
 * \param mmrp	pointer to mmrp context
 */
void mmrp_transmit(struct mmrp_ctx *mmrp)
{

}


/** Decode and process the MMRP frame received by the upper SRP layer. The packet points directly to MMRP data.
 * \return	0 on success, negative value on failure
 * \param mmrp	pointer to the mmrp context
 * \param port_id identifier of the port
 * \param desc	pointer to the received packet descriptor
 */
void mmrp_process_packet(struct mmrp_ctx *mmrp, unsigned int port_id, struct net_rx_desc *desc)
{

}


/** MMRP component initialization (timers, state machines,...)
 * \return	0 on success, negative value on failure
 * \param mmrp	pointer to the mmrp context
 */
__init int mmrp_init(struct mmrp_ctx *mmrp)
{
	//Add mmrp multicast adress

	os_log(LOG_INIT, "mmrp(%p) done\n", mmrp);

	return 0;
}


/** MMRP component clean-up, destroy all pending attributes
 * \return	0 on success, negative value on failure
 * \param mmrp	pointer to the mmrp context
 */
__exit int mmrp_exit(struct mmrp_ctx *mmrp)
{
	os_log(LOG_INIT, "done\n");

	//Del mmrp multicast adress

	return 0;
}
