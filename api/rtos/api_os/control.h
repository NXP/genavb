/*
 * Copyright 2018, 2020-2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file control.h
 \brief GenAVB API private includes
 \details private definitions for the GenAVB library

 \copyright Copyright 2018, 2020-2021, 2023-2024 NXP
*/

#ifndef _RTOS_PRIVATE_CONTROL_H_
#define _RTOS_PRIVATE_CONTROL_H_

#include "rtos_abstraction_layer.h"

#include "common/ipc.h"

#define AVB_CONTROL_EVENT_QUEUE_LENGTH	16

struct genavb_control_handle {
	int id;
	struct ipc_tx tx;
	struct ipc_rx rx;
	struct ipc_rx rx_sync;

	rtos_mqueue_t event_queue;
	uint8_t event_queue_buffer[AVB_CONTROL_EVENT_QUEUE_LENGTH * sizeof(struct event)];
};

#endif /* _RTOS_PRIVATE_CONTROL_H_ */
