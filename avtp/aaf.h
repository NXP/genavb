/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVTP Audio Format (AAF) handling functions
 @details
*/

#ifndef _AAF_H_
#define _AAF_H_

#ifdef CFG_AVTP_1722A
#include "common/net.h"
#include "genavb/aaf.h"
#include "genavb/avdecc.h"

#include "stream.h"

int listener_stream_aaf_check(struct stream_listener *stream, struct avdecc_format const *format, u16 flags);
int talker_stream_aaf_check(struct stream_talker *stream, struct avdecc_format const *format, struct ipc_avtp_connect *ipc);

#endif

#endif /* _AAF_H_ */
