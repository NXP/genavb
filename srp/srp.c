/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
  @file		srp.c
  @brief	SRP module implementation.
  @details	MVRP, MSRP, MMRP entry point
*/


#include "os/stdlib.h"
#include "genavb/qos.h"

#include "common/log.h"
#include "common/ether.h"

#include "srp.h"


static void srp_ipc_get_mac_status(struct srp_ctx *srp, struct ipc_tx *ipc, unsigned int port_id)
{
	struct ipc_desc *desc;
	struct ipc_mac_service_get_status *get_status;
	int rc;

	desc = ipc_alloc(ipc, sizeof(struct ipc_mac_service_get_status));
	if (desc) {
		desc->type = IPC_MAC_SERVICE_GET_STATUS;
		desc->len = sizeof(struct ipc_mac_service_get_status);
		desc->flags = 0;

		get_status = &desc->u.mac_service_get_status;

		get_status->port_id = port_id;

		rc = ipc_tx(ipc, desc);
		if (rc < 0) {
			if (rc != -IPC_TX_ERR_NO_READER)
				os_log(LOG_ERR, "ipc_tx() failed\n");

			ipc_free(ipc, desc);
		}
	} else
		os_log(LOG_ERR, "ipc_alloc() failed\n");
}

static void srp_ipc_rx_mac_service(struct ipc_rx const *rx, struct ipc_desc *desc)
{
	struct srp_ctx *srp = container_of(rx, struct srp_ctx, ipc_rx_mac_service);
	struct ipc_mac_service_status *status;

	switch (desc->type) {
	case IPC_MAC_SERVICE_STATUS:
		status = &desc->u.mac_service_status;

		os_log(LOG_DEBUG, "IPC_MAC_SERVICE_STATUS: logical_port(%u)\n", status->port_id);

		msrp_port_status(srp->msrp, status);

		mvrp_port_status(srp->mvrp, status);

		break;

	default:
		break;
	}

	ipc_free(rx, desc);
}

void srp_ipc_managed_get(struct srp_ctx*srp, struct ipc_tx *ipc, unsigned int ipc_dst, uint8_t *in, uint8_t *in_end)
{
	struct ipc_desc *desc;
	int rc;

	os_log(LOG_DEBUG, "srp_ipc_managed_get\n");

	desc = ipc_alloc(ipc, GENAVB_MAX_MANAGED_SIZE);
	if (desc) {

		desc->len = srp_managed_objects_get(&srp->module, in, in_end, desc->u.data, desc->u.data + GENAVB_MAX_MANAGED_SIZE);

		desc->dst = ipc_dst;
		desc->type = GENAVB_MSG_MANAGED_GET_RESPONSE;

		rc = ipc_tx(ipc, desc);
		if (rc < 0) {
			if (rc != -IPC_TX_ERR_NO_READER)
				os_log(LOG_ERR, "ipc_tx() failed\n");

			ipc_free(ipc, desc);
		}
	} else
		os_log(LOG_ERR, "ipc_alloc() failed\n");
}

void srp_ipc_managed_set(struct srp_ctx*srp, struct ipc_tx *ipc, unsigned int ipc_dst, uint8_t *in, uint8_t *in_end)
{
	struct ipc_desc *desc;
	int rc;

	desc = ipc_alloc(ipc, GENAVB_MAX_MANAGED_SIZE);
	if (desc) {

		desc->len = srp_managed_objects_set(&srp->module, in, in_end, desc->u.data, desc->u.data + GENAVB_MAX_MANAGED_SIZE);

		desc->dst = ipc_dst;
		desc->type = GENAVB_MSG_MANAGED_SET_RESPONSE;

		rc = ipc_tx(ipc, desc);
		if (rc < 0) {
			if (rc != -IPC_TX_ERR_NO_READER)
				os_log(LOG_ERR, "ipc_tx() failed\n");

			ipc_free(ipc, desc);
		}
	} else
		os_log(LOG_ERR, "ipc_alloc() failed\n");
}

static int srp_port_init(struct srp_ctx *srp, struct srp_config *cfg, unsigned long priv)
{
	struct net_address addr;
	unsigned int i;

	addr.ptype = PTYPE_MRP;
	addr.priority = MRP_DEFAULT_PRIORITY;

	for (i = 0; i < srp->port_max; i++) {
		/* FIXME, the code assumes the configuration port list is the same for msrp, mvrp, ... */
		addr.port = cfg->msrp_cfg.logical_port_list[i];

		srp->port[i].port_id = i;

		srp->port[i].mrp.leave_timeout = cfg->mrp_cfg.leave_timeout;

		srp->port[i].initialized = false;

		if (net_rx_init(&srp->port[i].net_rx, &addr, srp_net_rx, priv) < 0)
			goto err_net_rx_init;

		if (net_tx_init(&srp->port[i].net_tx, &addr) < 0)
			goto err_net_tx_init;

		srp->port[i].initialized = true;

		if (srp->management_enabled)
			srp_ipc_get_mac_status(srp, &srp->ipc_tx_mac_service, cfg->msrp_cfg.logical_port_list[i]);

		continue;

	err_net_tx_init:
		net_rx_exit(&srp->port[i].net_rx);

	err_net_rx_init:
		continue;
	}

	return 0;
}

static void srp_port_exit(struct srp_ctx *srp)
{
	unsigned int i;

	for (i = 0; i < srp->port_max; i++) {
		if (!srp->port[i].initialized)
			continue;

		net_tx_exit(&srp->port[i].net_tx);
		net_rx_exit(&srp->port[i].net_rx);
	}
}

__init static struct srp_ctx *srp_alloc(unsigned int ports, unsigned int timer_n)
{
	struct srp_ctx *srp;
	unsigned int size;

	size = sizeof(struct srp_ctx) + ports * sizeof(struct srp_port);
	size += sizeof(struct msrp_ctx) + ports * sizeof(struct msrp_port);
	size += sizeof(struct mvrp_ctx) + ports * sizeof(struct mvrp_port);
	size += timer_pool_size(timer_n);

	srp = os_malloc(size);
	if (!srp)
		goto err;

	os_memset(srp, 0, size);

	srp->port_max = ports;

	srp->msrp = (struct msrp_ctx *)((u8 *)(srp + 1) + ports * sizeof(struct srp_port));
	srp->mvrp = (struct mvrp_ctx *)((u8 *)(srp->msrp + 1) + ports * sizeof(struct msrp_port));
	srp->timer_ctx = (struct timer_ctx *)((u8 *)(srp->mvrp + 1) + ports * sizeof(struct mvrp_port));

	srp->msrp->srp = srp;
	srp->mvrp->srp = srp;

	return srp;

err:
	return NULL;
}

/** Common SRP code entry points. Initialize MSRP, MVRP and MMRP sub-components
 * \return	0 on success or negative value on failure
 * \param srp	pointer to the main SRP context
 */
__init void *srp_init(struct srp_config *cfg, unsigned long priv)
{
	struct srp_ctx *srp;
	unsigned int timer_n;
	unsigned int ipc_tx, ipc_rx;

	timer_n = cfg->port_max * CFG_SRP_MAX_TIMERS_PER_PORT;

	srp = srp_alloc(cfg->port_max, timer_n);
	if (!srp)
		goto err_malloc;

	log_level_set(srp_COMPONENT_ID, cfg->log_level);

	srp->management_enabled = cfg->management_enabled;

	if (srp->management_enabled) {
		if (!cfg->is_bridge) {
			if (cfg->logical_port_list[0] == CFG_ENDPOINT_0_LOGICAL_PORT) {
				ipc_rx = IPC_MAC_SERVICE_MEDIA_STACK;
				ipc_tx = IPC_MEDIA_STACK_MAC_SERVICE;
			} else {
				ipc_rx = IPC_MAC_SERVICE_1_MEDIA_STACK;
				ipc_tx = IPC_MEDIA_STACK_MAC_SERVICE_1;
			}
		} else {
			ipc_rx = IPC_MAC_SERVICE_BRIDGE_MEDIA_STACK;
			ipc_tx = IPC_MEDIA_STACK_MAC_SERVICE_BRIDGE;
		}

		if (ipc_rx_init(&srp->ipc_rx_mac_service, ipc_rx, srp_ipc_rx_mac_service, priv) < 0)
			goto err_ipc_rx_mac_service;

		if (ipc_tx_init(&srp->ipc_tx_mac_service, ipc_tx) < 0)
			goto err_ipc_tx_mac_service;

		if (ipc_tx_connect(&srp->ipc_tx_mac_service, &srp->ipc_rx_mac_service) < 0)
			goto err_ipc_tx_connect;
	}

	if (srp_port_init(srp, cfg, priv) < 0)
		goto err_port_init;

	if (timer_pool_init(srp->timer_ctx, timer_n, priv) < 0)
		goto err_timer_pool_init;

	if (msrp_init(srp->msrp, &cfg->msrp_cfg, priv) < 0)
		goto err_msrp_init;

	if (mvrp_init(srp->mvrp, &cfg->mvrp_cfg, priv) < 0)
		goto err_mvrp_init;

	if (mmrp_init(&srp->mmrp) < 0)
		goto err_mmrp_init;

	srp_managed_objects_init(&srp->module, srp);

	os_log(LOG_INIT, "srp(%p) done\n", srp);

	return srp;

err_mmrp_init:
	mvrp_exit(srp->mvrp);

err_mvrp_init:
	msrp_exit(srp->msrp);

err_msrp_init:
	timer_pool_exit(srp->timer_ctx);

err_timer_pool_init:
	srp_port_exit(srp);

err_port_init:
err_ipc_tx_connect:
	if (srp->management_enabled)
		ipc_tx_exit(&srp->ipc_tx_mac_service);

err_ipc_tx_mac_service:
	if (srp->management_enabled)
		ipc_rx_exit(&srp->ipc_rx_mac_service);

err_ipc_rx_mac_service:
	os_free(srp);

err_malloc:
	return NULL;
}


/** SRP exit function. Terminate and clean-up MSRP, MVRP and MMRP sub-components
 * \return	0 on success or negative value on failure
 * \param srp	pointer to the main SRP context
 */
__exit int srp_exit(void *srp_h)
{
	struct srp_ctx *srp = (struct srp_ctx *)srp_h;

	mmrp_exit(&srp->mmrp);

	mvrp_exit(srp->mvrp);

	msrp_exit(srp->msrp);

	timer_pool_exit(srp->timer_ctx);

	srp_port_exit(srp);

	if (srp->management_enabled) {
		ipc_tx_exit(&srp->ipc_tx_mac_service);
		ipc_rx_exit(&srp->ipc_rx_mac_service);
	}

	os_log(LOG_INIT, "srp(%p)\n", srp);

	os_free(srp_h);

	return 0;
}


/** SRP packet receive function. Dispatch SRP packets to the corresponding component
 * \return	none
 * \param rx	pointer to network receive context
 * \param desc	pointer to received packet  descriptor
 */
void srp_net_rx(struct net_rx *rx, struct net_rx_desc *desc)
{
	struct srp_port *port = container_of(rx, struct srp_port, net_rx);
	struct srp_ctx *srp = container_of(port, struct srp_ctx, port[port->port_id]);

	/* first level parsing of an MRP frame should be done here to dispatch to corresponding application */
	switch (desc->ethertype) {
	case ETHERTYPE_MSRP:
		msrp_process_packet(srp->msrp, port->port_id, desc);
		break;

	case ETHERTYPE_MVRP:
		mvrp_process_packet(srp->mvrp, port->port_id, desc);
		break;

	case ETHERTYPE_MMRP:
		mmrp_process_packet(&srp->mmrp, port->port_id, desc);
		break;

	default:
		break;
	}

	net_rx_free(desc);
}
