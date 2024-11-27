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

#include "os/sys_types.h"

#include "config.h"

#include "net_mdio.h"
#include "net_port.h"
#include "net_port_netc_mdio.h"
#include "net_port_enet_mdio.h"
#include "net_port_enet_qos_mdio.h"

#if CFG_NUM_MDIO > 0

static struct net_mdio mdio[CFG_NUM_MDIO] = {
	[0] = {
		.drv_type = BOARD_MDIO0_DRV_TYPE,
		.drv_index = BOARD_MDIO0_DRV_INDEX,
	},
#if CFG_NUM_MDIO > 1
	[1] = {
		.drv_type = BOARD_MDIO1_DRV_TYPE,
		.drv_index = BOARD_MDIO1_DRV_INDEX,
	},
#endif
#if CFG_NUM_MDIO > 2
#error invalid CFG_NUM_MDIO
#endif
};

static int mdio0_write(uint8_t phyAddr, uint8_t regAddr, uint16_t data)
{
	return mdio[0].write(mdio[0].handle, phyAddr, regAddr, data);
}

#if CFG_NUM_MDIO > 1
static int mdio1_write(uint8_t phyAddr, uint8_t regAddr, uint16_t data)
{
	return mdio[1].write(mdio[0].handle, phyAddr, regAddr, data);
}
#endif

static int mdio0_read(uint8_t phyAddr, uint8_t regAddr, uint16_t *data)
{
	return mdio[0].read(mdio[0].handle, phyAddr, regAddr, data);
}

#if CFG_NUM_MDIO > 1
static int mdio1_read(uint8_t phyAddr, uint8_t regAddr, uint16_t *data)
{
	return mdio[1].read(mdio[0].handle, phyAddr, regAddr, data);
}
#endif

void *mdio_read(unsigned int id)
{
	switch (id) {
	case 0:
	default:
		return mdio0_read;
		break;
#if CFG_NUM_MDIO > 1
	case 1:
		return mdio1_read;
		break;
#endif
	}
}

void *mdio_write(unsigned int id)
{
	switch (id) {
	case 0:
	default:
		return mdio0_write;
		break;
#if CFG_NUM_MDIO > 1
	case 1:
		return mdio1_write;
		break;
#endif
	}
}

__exit static void __mdio_exit(struct net_mdio *mdio)
{
	mdio->exit(mdio->handle);
}

__init static int __mdio_init(struct net_mdio *mdio)
{
	int rc;

	switch (mdio->drv_type) {
	case NETC_EMDIO_t:
		rc = netc_emdio_init(mdio);
		break;
	case NETC_PORT_EMDIO_t:
		rc = netc_port_emdio_init(mdio);
		break;
	case ENET_t:
	case ENET_1G_t:
		rc = enet_mdio_init(mdio);
		break;
	case ENET_QOS_t:
		rc = enet_qos_mdio_init(mdio);
		break;
	default:
		rc = -1;
		break;
	}

	return rc;
}

__init int mdio_init(void)
{
	int i;

	for (i = 0; i < CFG_NUM_MDIO; i++) {
		if (__mdio_init(&mdio[i]) < 0)
			goto err;
	}

	return 0;

err:
#if CFG_NUM_MDIO > 1
	for (i--; i >= 0; i--)
		__mdio_exit(&mdio[i]);
#endif

	return -1;
}

__exit void mdio_exit(void)
{
	int i;

	for (i = 0; i < CFG_NUM_MDIO; i++)
		__mdio_exit(&mdio[i]);
}

#else
__init int mdio_init(void) { return 0; }
__exit void mdio_exit(void) { }
#endif /* CFG_NUM_MDIO */
