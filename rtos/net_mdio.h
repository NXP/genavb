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

#ifndef __RTOS_NET_MDIO_H_
#define __RTOS_NET_MDIO_H_

struct net_mdio {
	int drv_type;
	unsigned int drv_index;
	void *handle;
	int (*read)(void *, uint8_t, uint8_t, uint16_t *);
	int (*write)(void *, uint8_t, uint8_t, uint16_t);
	void (*exit)(struct net_mdio *);
};

void *mdio_read(unsigned int id);
void *mdio_write(unsigned int id);
int mdio_init(void);
void mdio_exit(void);

#endif /* __RTOS_NET_MDIO_H_ */
