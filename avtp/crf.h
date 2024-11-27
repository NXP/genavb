/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVTP Clock Reference Format (CRF) handling functions
 @details
*/

#ifndef _CRF_H_
#define _CRF_H_

#ifdef CFG_AVTP_1722A
#include "common/net.h"
#include "genavb/crf.h"
#include "genavb/avdecc.h"

#include "stream.h"

int listener_crf_check(struct stream_listener *stream, struct avdecc_format const *format, u16 flags);
int talker_crf_check(struct stream_talker *stream, struct avdecc_format const *format, struct ipc_avtp_connect *ipc);
void crf_os_timer_handler(struct os_timer *t, int count);

unsigned int crf_stream_presentation_offset(struct stream_talker *stream);

#define CRF_FREE_WHEELING_TO_LOCKED_DELAY_MS	1000	/* 1 second. Amount of time to wait before trying to switch from free-wheeling to locked
							(if there is a discontinuity in received timestamps) */

#define CRF_LATENCY_MAX		2000000
#define CRF_TX_BATCH		1	/* Works well for 50Hz pdu rate (as specified in 1722-2016, Table 28) */

#endif

#endif /* _CRF_H_ */
