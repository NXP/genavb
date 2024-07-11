/*
 * Copyright 2018, 2020-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file control.h
 \brief GenAVB API private includes
 \details private definitions for the GenAVB library

 \copyright Copyright 2018, 2020-2021, 2023 NXP
*/

#ifndef _LINUX_PRIVATE_CONTROL_H_
#define _LINUX_PRIVATE_CONTROL_H_

#include "common/ipc.h"

struct genavb_control_handle {
	int id;
	struct ipc_tx tx;
	struct ipc_rx rx;
	struct ipc_rx rx_sync;
};

#endif /* _LINUX_PRIVATE_CONTROL_H_ */
