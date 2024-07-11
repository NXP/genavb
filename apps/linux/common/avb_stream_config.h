/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __AVB_STREAM_CONFIG_H__
#define __AVB_STREAM_CONFIG_H__

#include <stdio.h>
#include <genavb/genavb.h>
#include "stats.h"


#define AAR_AVB_ETH_PORT	0

/**
 * @addtogroup avb_stream
 * @{
 */

/** AVB counter statistic */
typedef struct {
	unsigned int tx_err;      /**< AVB send error counter */
	unsigned int rx_err;      /**< AVB receive error counter */

	unsigned int batch_rx;    /**< AVB batches received counter */
	unsigned int batch_tx;    /**< AVB batches transmitted counter */
} aar_avb_counter_stats_t;

typedef struct {
	aar_avb_counter_stats_t counter_stats; /**< AVB Simple counter statistic */
	struct stats gptp_2cont_wakeup;        /**< gPTP time delta since last wakeup */
	struct stats event_2cont_wakeup;       /**< AVB event time delta since last wakeup (1st event) */
	struct stats event_gptp;               /**< time delta between event time vs current gPTP time */
} aar_avb_stats_t;

typedef struct _AAR_AVB_STREAM {
	struct avb_stream_params stream_params;
	struct avb_stream_handle *stream_handle;
	unsigned int batch_size_ns;
	unsigned int cur_batch_size;

	unsigned int last_gptp_time;		/**< gPTP time of last wakeup */
	unsigned int last_event_ts;		/**< Event timestamp of last wakeup */
	unsigned int last_event_frame_offset;
	unsigned int last_exchanged_frames;	/**< Number of frames exchanged in the last batch */
	char is_first_wakeup;                  /**< Is first wakeup flag */

	aar_avb_stats_t stats;                 /**< AVB statistics */
} aar_avb_stream_t;

extern const unsigned int g_max_avb_talker_streams;
extern const unsigned int g_max_avb_listener_streams;
extern aar_avb_stream_t g_avb_talker_streams[];
extern aar_avb_stream_t g_avb_listener_streams[];

/** @} */
#endif /* __AVB_STREAM_CONFIG_H__ */
