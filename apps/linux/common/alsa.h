/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _ALSA_H_
#define _ALSA_H_

#include <alsa/asoundlib.h>
#include <genavb/genavb.h>

#include "../common/stats.h"


#define CFG_ALSA_PLAYBACK_LATENCY_NS		2000000		// Additional fixed playback latency in ns
#define CFG_ALSA_CAPTURE_LATENCY_NS		2000000		// Additional fixed capture latency in ns
#define CFG_ALSA_PLAYBACK_BUFFER_SIZE_NS	(sr_class_max_transit_time(SR_CLASS_B) + 1000000)		// Requested Alsa playback buffer size, in ns
#define CFG_ALSA_CAPTURE_BUFFER_SIZE_NS		20000000	// Requested Alsa capture buffer size, in ns
#define CFG_ALSA_PERIOD_SIZE			24		// Note: period_size cannot be less than max(16, buffer_size/128)


struct alsa_common {
	snd_pcm_t *handle;
	snd_pcm_hw_params_t *hwparams;
	snd_pcm_sw_params_t *swparams;
	snd_output_t *output;
	snd_pcm_uframes_t buffer_size;
	snd_pcm_format_t format;
	snd_pcm_stream_t direction;
	unsigned int batch_size;
	unsigned int running;				/**< 1 if stream is running, 0 otherwise. */
	unsigned int count;				/**< Number of frames already played out. */
	struct avdecc_format avdecc_format;		/**< stream format provided at stream init time */
	unsigned int rate;					/**< Sample rate in Hz */
	unsigned int bytes_per_sample;			/**< Number of bytes for each sample (directly related to sample_format) */
	unsigned int unused_bits;				/**< Number of unused bits in each sample */
	unsigned int channels_per_frame;			/**< Number of active channels per frame */
	unsigned int frame_size;				/**< Size of a full frame, in bytes */
	unsigned int nsecs_per_frame;			/**< Duration of an audio frame, in ns */
};

struct alsa_rx {
	struct alsa_common common;
	int fd;

	unsigned int payload_size;
	unsigned int payload_offset;
};

struct alsa_tx {
	struct alsa_common common;
	struct stats latency;
	struct stats latency_min;
	struct stats frame_rate;
	uint64_t frame_rate_last;
	unsigned int frame_rate_count;
	void (*alsa_process_samples)(struct alsa_tx *alsa, void *src_frame, snd_pcm_uframes_t to_commit);
};

void *alsa_tx_init(void *stream_id, struct avdecc_format *avdecc_format, unsigned int batch_size, const char *alsa_device);
void alsa_tx_exit(void *priv);
int alsa_tx(void *priv, struct avb_stream_handle *stream_h, struct avb_stream_params *stream_params);

#endif /* _ALSA_H_ */
