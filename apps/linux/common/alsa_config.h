/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __ALSA_CONFIG_H__
#define __ALSA_CONFIG_H__

/**
 * @addtogroup aar
 * @{
 */

enum {
	ALSA_CAPTURE0 = 0,
	ALSA_CAPTURE1,
	ALSA_CAPTURE2,
	ALSA_CAPTURE3,
	ALSA_CAPTURE4,
	ALSA_CAPTURE5,
	MAX_ALSA_CAPTURE
};

enum {
	ALSA_PLAYBACK0 = 0,
	ALSA_PLAYBACK1,
	ALSA_PLAYBACK2,
	ALSA_PLAYBACK3,
	ALSA_PLAYBACK4,
	ALSA_PLAYBACK5,
	MAX_ALSA_PLAYBACK
};

/** ALSA device parameter structure */
typedef struct _ALSA_DEVICE_PARAMETER {
	unsigned int period_time_ns;           /**< ALSA device period */
	int          format;                   /**< Audio data format */
	unsigned int fifo_threshold;           /**< FIFO threshold below which the ESAI block triggers a transfer request, in bytes.
							Only used to correct the reported ALSA latency statistics. */
	unsigned int pcm_start_delay;         /**< Estimate of Alsa snd_pcm_start overhead, in nanoseconds. This is used to correct
							the amount of silence frames that need to be added before calling snd_pcm_start,
							and improve the accuracy of the playback time of audio samples. */
} aar_alsa_param_t;

extern const char alsa_playback_device_names[MAX_ALSA_PLAYBACK][15];
extern const char alsa_capture_device_names[MAX_ALSA_CAPTURE][15];

#endif /* __ALSA_CONFIG_H__ */
