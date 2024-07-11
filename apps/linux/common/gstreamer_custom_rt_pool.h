/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2018-2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __CUSTOM_RT_POOL_H__
#define __CUSTOM_RT_POOL_H__

#include <gst/gst.h>
#include "gstreamer.h"

GstTaskPool *   avb_rt_pool_new         (void);

#endif /* __CUSTOM_RT_POOL_H__ */
