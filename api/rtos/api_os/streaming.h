/*
* Copyright 2018, 2020, 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 \file streaming.h
 \brief GenAVB API private includes
 \details private definitions for the GenAVB library

 \copyright Copyright 2018, 2020, 2023 NXP
*/

#ifndef _RTOS_PRIVATE_STREAMING_H_
#define _RTOS_PRIVATE_STREAMING_H_

#include "rtos/media_queue.h"

struct genavb_stream_handle {
	struct genavb_handle *genavb;
	struct genavb_stream_params params;
	unsigned int max_payload_size;	/* Transmit maximum packet payload size (in byte units) */
	unsigned int batch;		/* Transmit batch (in packet units) */

	struct media_queue mqueue;
};

#endif /* _RTOS_PRIVATE_STREAMING_H_ */
