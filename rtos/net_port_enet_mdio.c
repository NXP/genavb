/*
* Copyright 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief RTOS specific Network service implementation
 @details
*/

#include "common/log.h"
#include "config.h"
#include "net_mdio.h"

#if CFG_NUM_ENET_MAC

#include "fsl_enet.h"

extern void *enet_get_handle(unsigned int id);

static int enet_mdio_write(void *handle, uint8_t phyAddr, uint8_t regAddr, uint16_t data)
{
	return ENET_MDIOWrite(handle, phyAddr, regAddr, data);
}

static int enet_mdio_read(void *handle, uint8_t phyAddr, uint8_t regAddr, uint16_t *data)
{
	return ENET_MDIORead(handle, phyAddr, regAddr, data);
}

__exit static void enet_mdio_exit(struct net_mdio *mdio)
{

}

__init int enet_mdio_init(struct net_mdio *mdio)
{
	mdio->handle = enet_get_handle(mdio->drv_index);
	if (mdio->handle == NULL)
		goto err;

	mdio->exit = enet_mdio_exit;
	mdio->write = enet_mdio_write;
	mdio->read = enet_mdio_read;

	return 0;

err:
	return -1;
}

#else
__init int enet_mdio_init(struct net_mdio *mdio) { return -1; }
#endif

