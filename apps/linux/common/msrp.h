/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __MSRP_H__
#define __MSRP_H__


/**
 * @addtogroup avb_stream
 * @{
 */

/**
 * @brief      Initialize AVB stack and AVB resources.
 *
 * @return     0 if success or negative error code.
 */
int msrp_init(struct avb_handle *s_avb_handle);

/**
 * @brief      De-initialize AVB stack and AVB resources.
 *
 * @return     0 if success or negative error code.
 */
int msrp_exit(void);

int msrp_listener_register(struct avb_stream_params *stream_params);
void msrp_listener_deregister(struct avb_stream_params *stream_params);
int msrp_talker_register(struct avb_stream_params *stream_params);
void msrp_talker_deregister(struct avb_stream_params *stream_params);

/** @} */
#endif /* __AVB_STREAM_H__ */
