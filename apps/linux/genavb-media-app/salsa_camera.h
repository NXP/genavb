/*
 * Copyright 2017-2018, 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _SALSA_CAMERA_H_
#define _SALSA_CAMERA_H_

#ifdef CFG_AVTP_1722A
#include "multi_frame_sync.h"
#include "../common/gstreamer.h"
#include "../common/gst_pipeline_definitions.h"
#include "../common/gstreamer_single.h"

#define MAX_CAMERA			(MFS_MAX_STREAMS)

void salsa_camera_init(void);
int salsa_camera_connect(struct gstreamer_stream *salsa_stream);
void salsa_camera_disconnect(struct gstreamer_stream *salsa_stream);

int multi_salsa_split_screen_start(unsigned int nstreams, struct gstreamer_pipeline *pipeline);
int multi_salsa_surround_start(struct gstreamer_pipeline_config *gst_config);
void multi_salsa_camera_disconnect(unsigned int nstreams);

int surround_exit(void);

#endif


#endif /* _SALSA_CAMERA_H_ */
