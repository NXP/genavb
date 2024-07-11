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

#if CFG_NUM_ENETC_EP_MAC || CFG_NUM_NETC_SW

#include "fsl_netc_mdio.h"

static netc_mdio_handle_t mdio_handle;

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

__init int netc_mdio_init(struct net_mdio *mdio)
{
	netc_mdio_config_t mdioConfig = {
		.isPreambleDisable = false,
		.isNegativeDriven  = false,
	};

	if (mdio->drv_index)
		goto err;

	mdioConfig.mdio.type = kNETC_EMdio;
	mdioConfig.srcClockHz = dev_get_net_core_freq(SW0_BASE);

	if (NETC_MDIOInit(&mdio_handle, &mdioConfig) != kStatus_Success)
		goto err;

	mdio->exit = netc_mdio_exit;
	mdio->write = netc_mdio_write;
	mdio->read = netc_mdio_read;
	mdio->handle = &mdio_handle;

	return 0;

err:
	return -1;
}
#else
__init int netc_mdio_init(struct net_mdio *mdio) { return -1; }
#endif
