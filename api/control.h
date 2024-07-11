/*
 * Copyright 2018-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file control.h
 \brief GenAVB API private control API includes
 \details private definitions for the GenAVB library control API

 \copyright Copyright 2018-2021, 2023 NXP
*/

#ifndef _PRIVATE_CONTROL_H_
#define _PRIVATE_CONTROL_H_

#include "common/ipc.h"

#include "api_os/control.h"

#define CTRL_TX		0
#define CTRL_RX		1
#define CTRL_RX_SYNC	2

extern const ipc_id_t ipc_id[GENAVB_CTRL_ID_MAX][3];

int _avb_control_send(struct genavb_control_handle const *handle, genavb_msg_type_t msg_type, void const *msg, unsigned int msg_len, unsigned int flags);

int avb_ipc_send(const struct ipc_tx *tx, genavb_msg_type_t msg_type, void const *msg, unsigned int msg_len, unsigned int flags);

int avb_ipc_receive(struct ipc_rx const *rx, unsigned int *msg_type, void *msg, unsigned int *msg_len);

int avb_ipc_receive_sync(struct ipc_rx const *rx, unsigned int *msg_type, void *msg, unsigned int *msg_len, int timeout);

int send_heartbeat(struct ipc_tx *tx, struct ipc_rx *rx, unsigned int flags);

#endif /* _PRIVATE_CONTROL_H_ */
