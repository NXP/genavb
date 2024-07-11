/*
* Copyright 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#include "rtos/ipc.h"
#include "api/control.h"
#include "genavb/hsr.h"
#include "api/init.h"

static int hsr_ipc_init(struct genavb_handle *genavb)
{
	int rc;

	if (ipc_tx_init(&genavb->hsr_ipc_tx, IPC_HSR_STACK) < 0) {
		rc = -GENAVB_ERR_CTRL_INIT;
		return rc;
	}

	return 0;
}

int genavb_hsr_operation_mode_set(struct genavb_handle *genavb, genavb_hsr_mode_t mode)
{
	genavb_hsr_mode_t params = mode;
	int rc;

	if (!(genavb->flags & HSR_INITIALIZED)) {
		rc = hsr_ipc_init(genavb);
		if (rc < 0)
			return rc;

		genavb->flags |= HSR_INITIALIZED;
	}

	rc = avb_ipc_send(&genavb->hsr_ipc_tx, IPC_HSR_OPERATION_MODE_SET, &params, sizeof(params), 0);

	return rc;
}
