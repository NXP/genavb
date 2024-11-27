/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVTP Control Format (ACF) handling functions
 @details
*/

#ifndef _ACF_H_
#define _ACF_H_

#ifdef CFG_AVTP_1722A
#include "common/net.h"
#include "genavb/acf.h"
#include "genavb/avdecc.h"

#include "stream.h"

int listener_stream_acf_tscf_check(struct stream_listener *stream, struct avdecc_format const *format, u16 flags);
int listener_acf_ntscf_check(struct stream_listener *stream, u16 flags);
int talker_stream_acf_tscf_check(struct stream_talker *stream, struct avdecc_format const *format, struct ipc_avtp_connect *ipc);
int talker_acf_ntscf_check(struct stream_talker *stream, struct ipc_avtp_connect *ipc);

#endif

#endif /* _ACF_H_ */
