/*
 * Copyright 2018-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file control.c
 \brief GenAVB public API for linux
 \details API definition for the GenAVB library
 \copyright Copyright 2018-2021, 2023 NXP
*/

#define _POSIX_C_SOURCE 200809L

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/poll.h>
#include <pthread.h>

#include "common/ipc.h"

#include "api/control.h"

int avb_ipc_receive_sync(struct ipc_rx const *rx, unsigned int *msg_type, void *msg, unsigned int *msg_len, int timeout)
{
	struct pollfd sync_poll;
	int rc;

	sync_poll.fd = rx->fd;
	sync_poll.events = POLLIN;
	rc = poll(&sync_poll, 1, timeout);
	while (rc == -1) {
		if (errno == EINTR)
			rc = poll(&sync_poll, 1, timeout);
		else {
			rc = -GENAVB_ERR_CTRL_RX;
			goto exit;
		}
	}

	if (rc != 0) {
		if (sync_poll.revents & POLLIN)
			rc = avb_ipc_receive(rx, msg_type, msg, msg_len);
		else
			rc = -GENAVB_ERR_CTRL_RX;
	} else
		rc = -GENAVB_ERR_CTRL_TIMEOUT;

exit:
	return rc;
}

int genavb_control_rx_fd(struct genavb_control_handle const *handle)
{
	return handle->rx.fd;
}

int genavb_control_tx_fd(struct genavb_control_handle const *handle)
{
	return handle->tx.fd;
}

int genavb_control_open(struct genavb_handle const *genavb, struct genavb_control_handle **control, genavb_control_id_t id)
{
	int rc;

	/*
	* allocate new stream
	*/

	if (id >= GENAVB_CTRL_ID_MAX) {
		rc = (-GENAVB_ERR_CTRL_INIT);
		goto err_invalid_id;
	}

	*control = malloc(sizeof(struct genavb_control_handle));
	if (!*control) {
		rc = (-GENAVB_ERR_NO_MEMORY);
		goto err_alloc;
	}
	memset((*control), 0, sizeof(struct genavb_control_handle));

	(*control)->id = id;

	if (ipc_rx_init_no_notify(&(*control)->rx, ipc_id[id][CTRL_RX]) < 0) {
		rc = (-GENAVB_ERR_CTRL_INIT);
		goto error_control_rx_init;
	}

	if (ipc_tx_init(&(*control)->tx, ipc_id[id][CTRL_TX]) < 0) {
		rc = (-GENAVB_ERR_CTRL_INIT);
		goto error_control_tx_init;
	}

	if (ipc_id[id][CTRL_RX_SYNC] != IPC_ID_NONE) {
		if (ipc_rx_init_no_notify(&(*control)->rx_sync, ipc_id[id][CTRL_RX_SYNC]) < 0) {
			rc = (-GENAVB_ERR_CTRL_INIT);
			goto error_control_rx_sync_init;
		}
	}

	switch (id) {
	case GENAVB_CTRL_AVDECC_MEDIA_STACK:
	case GENAVB_CTRL_AVDECC_CONTROLLED:
		if (send_heartbeat(&(*control)->tx, &(*control)->rx, 0) < 0) {
			rc = -GENAVB_ERR_STACK_NOT_READY;
			goto err_tx_heartbeat;
		}

		break;

	case GENAVB_CTRL_AVDECC_CONTROLLER:
		if (send_heartbeat(&(*control)->tx, &(*control)->rx_sync, IPC_FLAGS_AVB_MSG_SYNC) < 0) {
			rc = -GENAVB_ERR_STACK_NOT_READY;
			goto err_tx_heartbeat;
		}

		break;

	case GENAVB_CTRL_MSRP:
	case GENAVB_CTRL_MSRP_BRIDGE:
	case GENAVB_CTRL_MVRP:
	case GENAVB_CTRL_MVRP_BRIDGE:
	case GENAVB_CTRL_GPTP:
	case GENAVB_CTRL_GPTP_BRIDGE:
	case GENAVB_CTRL_CLOCK_DOMAIN:
	case GENAVB_CTRL_MAAP:
		if (ipc_tx_connect(&(*control)->tx, &(*control)->rx) < 0) {
			rc = (-GENAVB_ERR_CTRL_INIT);
			goto err_connect;
		}

		if (ipc_tx_connect(&(*control)->tx, &(*control)->rx_sync) < 0) {
			rc = (-GENAVB_ERR_CTRL_INIT);
			goto err_connect;
		}

		break;

	default:
		break;
	}

	return GENAVB_SUCCESS;

err_connect:
err_tx_heartbeat:
	if (ipc_id[id][CTRL_RX_SYNC] != IPC_ID_NONE)
		ipc_rx_exit(&(*control)->rx_sync);

error_control_rx_sync_init:
	ipc_tx_exit(&(*control)->tx);

error_control_tx_init:
	ipc_rx_exit(&(*control)->rx);

error_control_rx_init:
	free(*control);
	*control = NULL;

err_alloc:
err_invalid_id:
	return rc;
}
