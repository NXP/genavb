/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __AVB_STREAM_H__
#define __AVB_STREAM_H__

#include "avb_stream_config.h"

/**
 * @addtogroup avb_stream
 * @{
 */

/**
 * @brief      Initialize AVB stack and AVB resources.
 *
 * @return     0 if success or negative error code.
 */
int avbstream_init();

/**
 * @brief      De-initialize AVB stack and AVB resources.
 *
 * @return     0 if success or negative error code.
 */
int avbstream_exit();

/**
 * @brief      Get max transit time (in ms) of AVB stream
 *
 * @param      stream  The AVB stream handle pointer
 *
 * @return     max transit time (in ms)
 */
unsigned int avbstream_get_max_transit_time(aar_avb_stream_t *stream);

int avbstream_listener_add(unsigned int unique_id, struct avb_stream_params *params, aar_avb_stream_t **stream);
int avbstream_talker_add(unsigned int unique_id, struct avb_stream_params *params, aar_avb_stream_t **stream);
int avbstream_listener_remove(unsigned int unique_id);
int avbstream_talker_remove(unsigned int unique_id);
unsigned int avbstream_batch_size(unsigned int batch_size_ns, struct avb_stream_params *params);

struct avb_handle *avbstream_get_avb_handle(void);
/** @} */
#endif /* __AVB_STREAM_H__ */
