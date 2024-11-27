/*
 * Copyright 2018, 2020-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file streaming.h
 \brief GenAVB API private streaming API includes
 \details private definitions for the GenAVB library streaming API

 \copyright Copyright 2018, 2020-2021, 2023 NXP
*/

#ifndef _PRIVATE_STREAMING_H_
#define _PRIVATE_STREAMING_H_

#include "genavb/genavb.h"

#include "api/init.h"
#include "api_os/streaming.h"

int connect_avtp(struct genavb_handle *genavb, struct genavb_stream_handle *stream);

int disconnect_avtp(struct genavb_handle *genavb, struct genavb_stream_params const *params);

#endif /* _PRIVATE_STREAMING_H_ */
