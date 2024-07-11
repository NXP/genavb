/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2020, 2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <alsa/asoundlib.h>
#include "../common/alsa_config.h"

#define ESAI_FIFO_THRESHOLD		64
#define CFG_ALSA_PCM_START_DELAY	35000

#define ALSA_PLAYBACK_DEFAULT_PARAMS					\
{									\
	.period_time_ns = 1000000,             /* 1 ms */			\
	.format = SND_PCM_FORMAT_S24_LE,				\
	.fifo_threshold = ESAI_FIFO_THRESHOLD,				\
	.pcm_start_delay = CFG_ALSA_PCM_START_DELAY,			\
}

#define ALSA_CAPTURE_DEFAULT_PARAMS					\
{									\
	.period_time_ns = 1000000,             /* 1 ms */			\
	.format = SND_PCM_FORMAT_S24_LE,				\
	.fifo_threshold = ESAI_FIFO_THRESHOLD,				\
	.pcm_start_delay = CFG_ALSA_PCM_START_DELAY,			\
}

const char alsa_playback_device_names[MAX_ALSA_PLAYBACK][15] = {
	[ALSA_PLAYBACK0] = "plughw:0,0",
	[ALSA_PLAYBACK1] = "plughw:0,1",
	[ALSA_PLAYBACK2] = "plughw:0,2",
	[ALSA_PLAYBACK3] = "plughw:0,3",
	[ALSA_PLAYBACK4] = "plughw:0,4",
	[ALSA_PLAYBACK5] = "plughw:0,5"
};

const char alsa_capture_device_names[MAX_ALSA_CAPTURE][15] = {
	[ALSA_CAPTURE0] = "plughw:0,0",
	[ALSA_CAPTURE1] = "plughw:0,1",
	[ALSA_CAPTURE2] = "plughw:0,2",
	[ALSA_CAPTURE3] = "plughw:0,3",
	[ALSA_CAPTURE4] = "plughw:0,4",
	[ALSA_CAPTURE5] = "plughw:0,5",
};

aar_alsa_param_t g_alsa_playback_params[MAX_ALSA_PLAYBACK] = {
	[ALSA_PLAYBACK0] = ALSA_PLAYBACK_DEFAULT_PARAMS,
	[ALSA_PLAYBACK1] = ALSA_PLAYBACK_DEFAULT_PARAMS,
	[ALSA_PLAYBACK2] = ALSA_PLAYBACK_DEFAULT_PARAMS,
	[ALSA_PLAYBACK3] = ALSA_PLAYBACK_DEFAULT_PARAMS,
	[ALSA_PLAYBACK4] = ALSA_PLAYBACK_DEFAULT_PARAMS,
	[ALSA_PLAYBACK5] = ALSA_PLAYBACK_DEFAULT_PARAMS,
};

aar_alsa_param_t g_alsa_capture_params[MAX_ALSA_CAPTURE] = {
	[ALSA_CAPTURE0] = ALSA_CAPTURE_DEFAULT_PARAMS,
	[ALSA_CAPTURE1] = ALSA_CAPTURE_DEFAULT_PARAMS,
	[ALSA_CAPTURE2] = ALSA_CAPTURE_DEFAULT_PARAMS,
	[ALSA_CAPTURE3] = ALSA_CAPTURE_DEFAULT_PARAMS,
	[ALSA_CAPTURE4] = ALSA_CAPTURE_DEFAULT_PARAMS,
	[ALSA_CAPTURE5] = ALSA_CAPTURE_DEFAULT_PARAMS,
};
