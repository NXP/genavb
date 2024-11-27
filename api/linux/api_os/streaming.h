/*
 * Copyright 2018, 2020-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file streaming.h
 \brief GenAVB API private includes
 \details private definitions for the GenAVB library

 \copyright Copyright 2018, 2020-2021, 2023 NXP
*/

#ifndef _LINUX_PRIVATE_STREAMING_H_
#define _LINUX_PRIVATE_STREAMING_H_

#include "common/list.h"
#include "include/genavb/genavb.h"

#include "init.h"

struct genavb_stream_handle {
	struct list_head list;
	int fd;
	struct genavb_handle *genavb;
	struct genavb_stream_params params;
	unsigned int max_payload_size;	/* Transmit maximum packet payload size (in byte units) */
	unsigned int partial_iovec;
	int expect_new_frame;
	unsigned int batch;		/* Transmit batch (in packet units) */
};

#endif /* _LINUX_PRIVATE_STREAMING_H_ */
