/*
 * Copyright 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2018, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Media abstraction
 @details Media abstraction implementation
*/

#ifndef _OS_MEDIA_H_
#define _OS_MEDIA_H_

#include "osal/media.h"

#define MEDIA_FLAG_WAKEUP  (1 << 0)

int media_rx(struct media_rx *media, struct media_rx_desc **desc, unsigned int n);
int media_rx_avail(struct media_rx *media);
int media_tx(struct media_tx *media, struct media_desc **desc, unsigned int n);
void media_rx_exit(struct media_rx *media);
void media_tx_exit(struct media_tx *media);
int media_rx_init(struct media_rx *media, void *stream_id, unsigned long priv, unsigned int flags, unsigned int header_len, unsigned int ts_offset);
int media_tx_init(struct media_tx *media, void *stream_id);

/** Enable receive event for the media queue.
* The event is generated when data is available for receive in the media queue.
*
 * \return	0 on success, -1 on error
 * \param rx	pointer to media receive context
 */
int media_rx_event_enable(struct media_rx *media);

/** Disable media queue data available event.
 *
 * \return	0 on success, -1 on error
 * \param rx	pointer to media receive context
 */
int media_rx_event_disable(struct media_rx *media);


#endif /* _OS_MEDIA_H_ */
