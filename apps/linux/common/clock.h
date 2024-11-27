/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _CLOCK_H_
#define _CLOCK_H_

/** Initialize the PTP clock framework for a given Ethernet port.
 * \param port_id	id of the Ethernet port.
 * \return	0 on success or -1 on error.
 */
int clock_init(int port_id);

/** Close the PTP clock framework for a given Ethernet port.
 * \param port_id	id of the Ethernet port.
 * \return	0 on success or -1 on error.
 */
int clock_exit(int port_id);

/**
 * Get GPTP time in nanoseconds modulo 2^32.
 * \param port_id	id of the Ethernet port from which PTP clock is to be read.
 * \param ns	pointer to u32 variable that will hold the result.
 * \return	0 on success, or negative value on error.
 */
int clock_gettime32(int port_id, unsigned int *ns);

/**
 * Get GPTP time in nanoseconds.
 * \param port_id	id of the Ethernet port from which PTP clock is to be read.
 * \param ns	pointer to u64 variable that will hold the result.
 * \return	0 on success, or negative value on error.
 */
int clock_gettime64(int port_id, unsigned long long *ns);

#endif /* _OS_CLOCK_H_ */
