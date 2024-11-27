/*
 * Copyright 2022-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file net_port_netc_1588.h
 @brief RTOS specific Timer service implementation
 @details
*/

#ifndef _RTOS_NET_PORT_NETC_1588_H_
#define _RTOS_NET_PORT_NETC_1588_H_

/**
 * \brief Entry point for the NETC 1588 Timer driver
 *
 * \return int
 */
int netc_1588_init(void);

/**
 * \brief Exit point for the NETC 1588 Timer driver
 *
 */
void netc_1588_exit(void);

/**
 * \brief It extend a 32 bit time (nanoseconds) to 64 bit for the NETC 1588 timer
 *
 * \param handle
 * \param hwts_ns
 * \return uint64_t value containing the 64 bit time
 */
uint64_t netc_1588_hwts_to_u64(struct hw_clock *clk, uint32_t hwts_ns);

bool netc_1588_freerunning_available(unsigned int hw_clock_id);

#endif /* _RTOS_NET_PORT_NETC_1588_H_ */
