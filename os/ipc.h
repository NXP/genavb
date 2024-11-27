/*
 * Copyright 2014 Freescale Semiconductor, Inc.
 * Copyright 2017-2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief IPC Service implementation
 @details
*/

#ifndef _OS_IPC_H_
#define _OS_IPC_H_

#include "osal/ipc.h"

/** Allocate an IPC descriptor for transmit.
 * \param	tx		pointer to ipc transmit context.
 * \param	size		size of IPC message payload
 * \return	pointer to ipc_desc structure on success or NULL on error.
 */
struct ipc_desc *ipc_alloc(struct ipc_tx const *tx, unsigned int size);


/** Free IPC descriptor
 * \param	ipc		pointer to ipc context (rx or tx).
 * \param	desc		pointer to ipc descriptor to be freed.
 * \return	none.
 */
void ipc_free(void const *ipc, struct ipc_desc *desc);

/** Initialize an IPC receive service handle.
 *  and require notifications using OS-specific mechanism.
 * \param	rx		pointer to ipc receive context.
 * \return	none.
 */
int ipc_rx_init(struct ipc_rx *, ipc_id_t id, void (*func)(struct ipc_rx const *, struct ipc_desc *), unsigned long priv);

/** Initialize an IPC receive service handle.
 * \param	rx		pointer to ipc receive context.
 * \return	none.
 */
int ipc_rx_init_no_notify(struct ipc_rx *, ipc_id_t id);

/** Close a given IPC receive service.
 * \param	rx		pointer to ipc receive context.
 * \return	none.
 */
void ipc_rx_exit(struct ipc_rx *rx);


/** Initialize an IPC transmit service handle.
 * \param	rx		pointer to ipc receive context.
 * \return	none.
 */
int ipc_tx_init(struct ipc_tx *, ipc_id_t id);

/** Close a given IPC transmit service.
 * \param	tx		pointer to ipc transmit context.
 * \return	none.
 */
void ipc_tx_exit(struct ipc_tx *tx);


/** Initiate IPC transmit.
 * \param	tx		pointer to ipc transmit context.
 * \param	desc		pointer to ipc descriptor to be transmitted.
 * \return	* 0 on success.
 * * -IPC_TX_ERR_NO_READER if no reader was available to receive the message.
 * * -IPC_TX_ERR_QUEUE_FULL if the message could not be enqueued on the reader side (reader queue full).
 */
int ipc_tx(struct ipc_tx const *tx, struct ipc_desc *desc);

/** Dequeue and process received IPC messages.
 * \param	rx		pointer to ipc receive context.
 * \return	pointer to ipc_desc structure on success or NULL on error.
 */
struct ipc_desc * __ipc_rx(struct ipc_rx const *rx);

/** Dequeue and process received IPC messages.
 * \param	rx		pointer to ipc receive context.
 * \return	none.
 */
void ipc_rx(struct ipc_rx const *rx);

/** Connects one IPC writer instance to another IPC reader.
 * \param	tx		pointer to ipc transmit context.
 * \param	rx		pointer to ipc receive context.
 * \return	0 on success, -1 otherwise.
 */
int ipc_tx_connect(struct ipc_tx *, struct ipc_rx *);

#endif /* _OS_IPC_H_ */
