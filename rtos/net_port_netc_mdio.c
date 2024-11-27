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

#include "common/log.h"
#include "config.h"
#include "net_mdio.h"

#if CFG_NUM_NETC_EMDIO || CFG_NUM_NETC_PORT_EMDIO

#include "fsl_netc_mdio.h"

struct netc_emdio_handle {
	netc_mdio_handle_t handle;
};

struct netc_port_emdio_handle {
	netc_mdio_handle_t handle;
	unsigned int port;
};

static struct netc_emdio_handle emdio[CFG_NUM_NETC_EMDIO];

static struct netc_port_emdio_handle port_emdio[CFG_NUM_NETC_PORT_EMDIO] = {
#if CFG_NUM_NETC_PORT_EMDIO > 0
	[0] = {
		.port = BOARD_NETC_PORT_EMDIO_PORT0,
	},
#endif
#if CFG_NUM_NETC_PORT_EMDIO > 1
	[1] = {
		.port = BOARD_NETC_PORT_EMDIO_PORT1,
	},
#endif
#if CFG_NUM_NETC_PORT_EMDIO > 2
	[2] = {
		.port = BOARD_NETC_PORT_EMDIO_PORT2,
	},
#endif
#if CFG_NUM_NETC_PORT_EMDIO > 3
	[3] = {
		.port = BOARD_NETC_PORT_EMDIO_PORT3,
	},
#endif
#if CFG_NUM_NETC_PORT_EMDIO > 4
	[4] = {
		.port = BOARD_NETC_PORT_EMDIO_PORT4,
	},
#endif
};

static int netc_mdio_write(void *handle, uint8_t phyAddr, uint8_t regAddr, uint16_t data)
{
	netc_mdio_handle_t *mdio_handle = handle;

	return NETC_MDIOWrite(mdio_handle, phyAddr, regAddr, data);
}

static int netc_mdio_read(void *handle, uint8_t phyAddr, uint8_t regAddr, uint16_t *data)
{
	netc_mdio_handle_t *mdio_handle = handle;

	return NETC_MDIORead(mdio_handle, phyAddr, regAddr, data);
}

__exit static void netc_mdio_exit(struct net_mdio *mdio)
{

}

static __init int netc_mdio_common_init(struct net_mdio *mdio, netc_mdio_config_t *mdioConfig, netc_mdio_handle_t *handle)
{
	mdioConfig->isPreambleDisable = false;
	mdioConfig->isNegativeDriven  = false;

	mdioConfig->srcClockHz = dev_get_net_core_freq(SW0_BASE);

	if (NETC_MDIOInit(handle, mdioConfig) != kStatus_Success)
		goto err;

	mdio->exit = netc_mdio_exit;
	mdio->write = netc_mdio_write;
	mdio->read = netc_mdio_read;
	mdio->handle = handle;

	return 0;

err:
	return -1;
}

__init int netc_emdio_init(struct net_mdio *mdio)
{
	netc_mdio_config_t mdioConfig;

	if (mdio->drv_index >= CFG_NUM_NETC_EMDIO)
		goto err;

	mdioConfig.mdio.type = kNETC_EMdio;

	if (netc_mdio_common_init(mdio, &mdioConfig, &emdio[mdio->drv_index].handle) < 0)
		goto err;

	return 0;

err:
	return -1;
}

__init int netc_port_emdio_init(struct net_mdio *mdio)
{
	netc_mdio_config_t mdioConfig;

	if (mdio->drv_index >= CFG_NUM_NETC_PORT_EMDIO)
		goto err;

	mdioConfig.mdio.port = port_emdio[mdio->drv_index].port;
	mdioConfig.mdio.type = kNETC_ExternalMdio;

	if (netc_mdio_common_init(mdio, &mdioConfig, &port_emdio[mdio->drv_index].handle) < 0)
		goto err;

	return 0;

err:
	return -1;
}
#else
__init int netc_emdio_init(struct net_mdio *mdio) { return -1; }
__init int netc_port_emdio_init(struct net_mdio *mdio) { return -1; }
#endif
