/*
 * Copyright 2017, 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _MULTI_FRAME_SYNC_H_
#define _MULTI_FRAME_SYNC_H_

#include <gst/gstbuffer.h>
#include <gst/app/gstappsink.h>

#define MFS_MAX_STREAMS	4

void mfs_init(unsigned int nstreams, int (*push_buffers)(void *priv, GstBuffer **buffers), void *priv);
int mfs_add_sync(unsigned int i, GstAppSink *appsink);
void mfs_remove_sync(unsigned int i);

#endif /* _MULTI_FRAME_SYNC_H_ */
