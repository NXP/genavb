/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "genavb/error.h"
#include "genavb/dsa.h"

int genavb_port_dsa_add(unsigned int cpu_port, uint8_t *mac_addr, unsigned int slave_port)
{
	if (!mac_addr)
		return -GENAVB_ERR_INVALID;

	return net_port_dsa_add(cpu_port, mac_addr, slave_port);
}

int genavb_port_dsa_delete(unsigned int slave_port)
{
	return net_port_dsa_delete(slave_port);
}
