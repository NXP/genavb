/*
* Copyright 2017, 2020, 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief RTOS specific IPC service implementation
 @details
*/

#ifndef _RTOS_OSAL_IPC_H_
#define _RTOS_OSAL_IPC_H_

#define DEFAULT_IPC_DATA_SIZE	1024

struct ipc_rx {
	void *ipc_slot;		/* must match struct ipc_tx */
	void (*func)(struct ipc_rx const *, struct ipc_desc *);
	unsigned long priv;
};

struct ipc_tx {
	void *ipc_slot;
};

int ipc_rx_set_callback(struct ipc_rx *rx, int (*callback)(void *), void *data);
int ipc_rx_enable_callback(struct ipc_rx *rx);

#endif /* _RTOS_OSAL_IPC_H_ */
