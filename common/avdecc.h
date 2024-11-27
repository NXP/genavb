/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file		avdecc.h
 @brief   	AVDECC common definitions and functions
*/

#ifndef _PROTO_AVDECC_H_
#define _PROTO_AVDECC_H_

#include "genavb/avdecc.h"
#include "genavb/sr_class.h"

unsigned int avdecc_fmt_sample_stride(const struct avdecc_format *format);

static inline unsigned int avdecc_fmt_payload_size(const struct avdecc_format *format, sr_class_t sr_class)
{
	unsigned int max_frame_size, max_interval_frames;

	return __avdecc_fmt_payload_size(format, sr_class, &max_frame_size, &max_interval_frames);
}

#endif /* _PROTO_AVDECC_H_ */
