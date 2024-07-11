/*
 * Copyright 2018, 2020-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file init.h
 \brief GenAVB API private includes
 \details private definitions for the GenAVB library

 \copyright Copyright 2018, 2020-2021, 2023 NXP
*/

#ifndef _LINUX_PRIVATE_INIT_H_
#define _LINUX_PRIVATE_INIT_H_

#include "common/ipc.h"
#include "common/list.h"

struct genavb_handle {
	int flags;
	struct list_head streams;
	struct ipc_tx avtp_tx;
	struct ipc_rx avtp_rx;
};

#endif /* _LINUX_PRIVATE_INIT_H_ */
