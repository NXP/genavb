/*
 * Copyright 2019-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
  @file		mac_service.c
  @brief	MAC service module implementation.
  @details
*/


#include "os/stdlib.h"

#include "common/log.h"
#include "common/ether.h"
#include "common/net.h"

#include "mac_service.h"
#include "management.h"

static struct mac_port *logical_to_mac_port(struct mac_service *mac, unsigned int port)
{
	int i;

	for (i = 0; i < mac->port_max; i++)
		if (port == mac->port[i].logical_port)
			return &mac->port[i];

	return NULL;
}

static void mac_service_ipc_status(struct mac_service *mac, struct ipc_tx *ipc, unsigned int ipc_dst, struct mac_port *port)
{
	struct ipc_desc *desc;
	int rc;

	desc = ipc_alloc(ipc, sizeof(struct ipc_mac_service_status));
	if (desc) {
		struct ipc_mac_service_status *status;

		desc->dst = ipc_dst;
		desc->type = IPC_MAC_SERVICE_STATUS;
		desc->len = sizeof(struct ipc_mac_service_status);

		status = &desc->u.mac_service_status;

		status->port_id = port->logical_port;
		status->operational = port->operational;
		status->point_to_point = port->point_to_point;
		status->rate = port->rate;

		rc = ipc_tx(ipc, desc);
		if (rc < 0) {
			if (rc != -IPC_TX_ERR_NO_READER)
				os_log(LOG_ERR, "ipc_tx() failed\n");

			ipc_free(ipc, desc);
		}
	} else
		os_log(LOG_ERR, "ipc_alloc() failed\n");
}

static void mac_service_ipc_error_response(struct mac_service *mac, struct ipc_tx *ipc, unsigned int ipc_dst, u32 type, u32 len, u32 status)
{
	struct ipc_desc *desc;
	struct genavb_msg_error_response *error;
	int rc;

	desc = ipc_alloc(ipc, sizeof(struct ipc_error_response));
	if (desc) {
		desc->dst = ipc_dst;
		desc->type = GENAVB_MSG_ERROR_RESPONSE;
		desc->len = sizeof(struct ipc_error_response);
		desc->flags = 0;

		error = &desc->u.error;

		error->type = type;
		error->len = len;
		error->status = status;

		rc = ipc_tx(ipc, desc);
		if (rc < 0) {
			if (rc != -IPC_TX_ERR_NO_READER)
				os_log(LOG_ERR, "ipc_tx() failed\n");

			ipc_free(ipc, desc);
		}
	} else
		os_log(LOG_ERR, "ipc_alloc() failed\n");
}

static void mac_service_ipc_rx_media_stack(struct ipc_rx const *rx, struct ipc_desc *desc)
{
	struct mac_service *mac = container_of(rx, struct mac_service, ipc_rx);
	struct ipc_tx *ipc_tx;
	struct ipc_mac_service_get_status *get_status;
	struct mac_port *port;
	u32 status;

	if (desc->flags & IPC_FLAGS_AVB_MSG_SYNC)
		ipc_tx = &mac->ipc_tx_sync;
	else
		ipc_tx = &mac->ipc_tx;

	switch (desc->type) {
	case IPC_MAC_SERVICE_GET_STATUS:

		if (desc->len != sizeof(struct ipc_mac_service_get_status)) {
			status = GENAVB_ERR_CTRL_LEN;
			goto err;
		}

		get_status = &desc->u.mac_service_get_status;

		port = logical_to_mac_port(mac, get_status->port_id);
		if (!port) {
			status = GENAVB_ERR_CTRL_INVALID;
			goto err;
		}

		mac_service_ipc_status(mac, ipc_tx, desc->src, port);

		break;

	default:
		status = GENAVB_ERR_CTRL_INVALID;
		goto err;
		break;
	}

	ipc_free(rx, desc);

	return;

err:
	mac_service_ipc_error_response(mac, ipc_tx, desc->src, desc->type, desc->len, status);

	ipc_free(rx, desc);
}

static void mac_service_timer_handler(void *data)
{
	struct mac_service *mac = (struct mac_service *)data;
	bool up, point_to_point;
	unsigned int rate;
	int i;

	/* Poll status of all ports and send indications if any changes */
	for (i = 0; i < mac->port_max; i++) {
		if (net_port_status(&mac->net_tx, mac->port[i].logical_port, &up, &point_to_point, &rate) < 0)
			continue;

		if ((mac->port[i].operational != up) ||
		(mac->port[i].point_to_point != point_to_point) ||
		(mac->port[i].rate != rate)) {
			mac->port[i].operational = up;
			mac->port[i].point_to_point = point_to_point;
			mac->port[i].rate = rate;

			mac_service_ipc_status(mac, &mac->ipc_tx, IPC_DST_ALL, &mac->port[i]);
		}
	}

	timer_start(&mac->timer, MAC_STATUS_PERIOD_MS);
}

/** MAC service code entry points.
 * \return	0 on success or negative value on failure
 * \param mac	pointer to the main mac service context
 */
__init int mac_service_init(struct mac_service *mac, struct management_config *cfg, unsigned long priv)
{
	unsigned int ipc_tx, ipc_tx_sync, ipc_rx;
	int i;

	mac->port_max = cfg->port_max;

	if (!cfg->is_bridge) {
		if (cfg->logical_port_list[0] == CFG_ENDPOINT_0_LOGICAL_PORT) {
			ipc_tx = IPC_MAC_SERVICE_MEDIA_STACK;
			ipc_tx_sync = IPC_MAC_SERVICE_MEDIA_STACK_SYNC;
			ipc_rx = IPC_MEDIA_STACK_MAC_SERVICE;
		} else {
			ipc_tx = IPC_MAC_SERVICE_1_MEDIA_STACK;
			ipc_tx_sync = IPC_MAC_SERVICE_1_MEDIA_STACK_SYNC;
			ipc_rx = IPC_MEDIA_STACK_MAC_SERVICE_1;
		}
	} else {
		ipc_tx = IPC_MAC_SERVICE_BRIDGE_MEDIA_STACK;
		ipc_tx_sync = IPC_MAC_SERVICE_BRIDGE_MEDIA_STACK_SYNC;
		ipc_rx = IPC_MEDIA_STACK_MAC_SERVICE_BRIDGE;
	}

	for (i = 0; i < mac->port_max; i++)
		mac->port[i].logical_port = cfg->logical_port_list[i];

	if (net_tx_init(&mac->net_tx, NULL) < 0)
		goto err_net_tx;

	if (ipc_tx_init(&mac->ipc_tx, ipc_tx) < 0)
		goto err_ipc_tx;

	if (ipc_tx_init(&mac->ipc_tx_sync, ipc_tx_sync) < 0)
		goto err_ipc_tx_sync;

	if (ipc_rx_init(&mac->ipc_rx, ipc_rx, mac_service_ipc_rx_media_stack, priv) < 0)
		goto err_ipc_rx;

	mac->timer.func = mac_service_timer_handler;
	mac->timer.data = mac;
	if (timer_create(mac->management->timer_ctx, &mac->timer, 0, MAC_STATUS_PERIOD_MS) < 0)
		goto err_timer;

	mac_service_timer_handler(mac);

	os_log(LOG_INIT, "mac(%p) done\n", mac);

	return 0;

err_timer:
	ipc_rx_exit(&mac->ipc_rx);

err_ipc_rx:
	ipc_tx_exit(&mac->ipc_tx_sync);

err_ipc_tx_sync:
	ipc_tx_exit(&mac->ipc_tx);

err_ipc_tx:
	net_tx_exit(&mac->net_tx);

err_net_tx:
	return -1;
}


/** MAC service exit function.
 * \return	0 on success or negative value on failure
 * \param mac	pointer to the main mac service context
 */
__exit int mac_service_exit(struct mac_service *mac)
{
	timer_destroy(&mac->timer);

	ipc_rx_exit(&mac->ipc_rx);

	ipc_tx_exit(&mac->ipc_tx_sync);

	ipc_tx_exit(&mac->ipc_tx);

	net_tx_exit(&mac->net_tx);

	os_log(LOG_INIT, "mac(%p)\n", mac);

	return 0;
}
