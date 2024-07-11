/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2020, 2022-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <time.h>
#include <math.h>
#include <byteswap.h>
#include <genavb/genavb.h>
#include "log.h"
#include "alsa2.h"
#include "common.h"
#include "stats.h"
#include "time.h"
#include "clock.h"
#include "common.h"
#include "clock_domain.h"

#define CFG_ALSA_PLAYBACK_LATENCY_NS	2000000	// Additional fixed playback latency in ns
#define CFG_ALSA_MIN_SILENCE_FRAMES		8		// Minimum number of silence frames to add in a single go when starting a stream

#define ALSA_EXTRA_DEBUG 0

static snd_output_t *s_alsa_output;

static unsigned int wait_clk_domain_validity = 0;

static const aar_alsa_param_t *alsa_get_param(aar_alsa_handle_t *handle);

static void alsa_add_61883_6_label_swap_data_32(aar_alsa_handle_t *handle, void *src_frame, snd_pcm_uframes_t to_commit);
static void alsa_adjust_padding_s24_le_input_swap_data_32(aar_alsa_handle_t *handle, void *src_frame, snd_pcm_uframes_t to_commit);
static void alsa_swap_data_32_adjust_padding_s24_le_output(aar_alsa_handle_t *handle, void *src_frame, snd_pcm_uframes_t to_commit);
static void alsa_swap_data_32(aar_alsa_handle_t *handle, void *src_frame, snd_pcm_uframes_t to_commit);
static void alsa_swap_data_24(aar_alsa_handle_t *handle, void *src_frame, snd_pcm_uframes_t to_commit) __attribute__((unused));
static void alsa_swap_data_16(aar_alsa_handle_t *handle, void *src_frame, snd_pcm_uframes_t to_commit) __attribute__((unused));

static void alsa_tx_stats_min_alsa(struct stats *s)
{
	aar_alsa_handle_t *handle = s->priv;

	stats_update(&handle->stats.alsa_latency, s->min * USECS_PER_SEC / handle->rate);
}

static int alsa_reset(aar_alsa_handle_t *handle)
{
	snd_pcm_t *snd_handle = handle->handle;

	// DBG("handle: %p", handle);
	snd_pcm_drop(snd_handle);
	snd_pcm_prepare(snd_handle);
	snd_pcm_reset(snd_handle);

	return 0;
}

static unsigned int alsa_ns_to_samples(unsigned int ns, struct avb_stream_params *stream_params)
{
	return (((unsigned long long)ns * avdecc_fmt_sample_rate(&stream_params->format) + NSECS_PER_SEC - 1) / NSECS_PER_SEC);
}

static unsigned int alsa_bytes_to_ns(unsigned int bytes, struct avb_stream_params *stream_params)
{
	return (((unsigned long long)bytes * NSECS_PER_SEC) / ((unsigned long long)avdecc_fmt_sample_rate(&stream_params->format) * avdecc_fmt_sample_size(&stream_params->format)));
}

 /* 61883-6 AM824 data format requires a label in the unused part of the 32 bits (24 bits of data).
 */
static void alsa_add_61883_6_label_swap_data_32(aar_alsa_handle_t *handle, void *src_frame, snd_pcm_uframes_t to_commit)
{
	void *dst_sample, *src_sample;
	int i;
	unsigned int bytes_per_sample = handle->frame_size / handle->channels;

	src_sample = dst_sample = src_frame;

	if (bytes_per_sample != 4 || handle->direction != AAR_DATA_DIR_INPUT)
		return;

	for (i = 0; i < to_commit * handle->channels; i++) {
		/* Add iec61883-6 label*/
		*(unsigned char *)dst_sample  = AM824_LABEL_RAW;
		/* Do endianess conversion */
		*(unsigned int *)dst_sample = bswap_32(*(unsigned int *)src_sample);

		dst_sample += 4;
		src_sample += 4;
	}
}
/* This function adjust padding for AAF 24/32 bits format then do endianness swap from LE to BE for input direction (stream talker)
 * As S24_LE alsa is putting the padding in the upper 8 bits (MSB padding) which will result when converting in
 * Big endian for AVTPDU to have the padding in the lower bits which contradicts AVTP IEEE 1722-2016 7.3.4
 * that imposes the unused bits to be at the upper bits inside the AVTPDU (LSB padding)
 * Apply this function on samples in LE format (e.g for talker : just after the capture from alsa and before converting
 * to BE network format and for listener: before alsa playback and after converting from BE network format)
 */
static void alsa_adjust_padding_s24_le_input_swap_data_32(aar_alsa_handle_t *handle, void *src_frame, snd_pcm_uframes_t to_commit)
{
	void *dst_sample, *src_sample;
	int i;
	unsigned int bytes_per_sample = handle->frame_size / handle->channels;

	src_sample = dst_sample = src_frame;

	if (bytes_per_sample != 4 || handle->direction != AAR_DATA_DIR_INPUT)
		return;

	for (i = 0; i < to_commit * handle->channels; i++) {
		/* Adjust padding: move unused bits from MSB (upper bits) to LSB (lower bits)*/
		*(unsigned int *)dst_sample = (*(unsigned int *)src_sample) << 8;
		/* Do endianess conversion */
		*(unsigned int *)dst_sample = bswap_32(*(unsigned int *)src_sample);

		dst_sample += 4;
		src_sample += 4;
	}
}

/* This function do the endianness conversion swap (network order BE -> LE) then adjust padding for AAF 24/32 bits format for output direction (stream listener)
 */
static void alsa_swap_data_32_adjust_padding_s24_le_output(aar_alsa_handle_t *handle, void *src_frame, snd_pcm_uframes_t to_commit)
{
	void *dst_sample, *src_sample;
	int i;
	unsigned int bytes_per_sample = handle->frame_size / handle->channels;

	src_sample = dst_sample = src_frame;

	if (bytes_per_sample != 4 || handle->direction != AAR_DATA_DIR_OUTPUT)
		return;

	for (i = 0; i < to_commit * handle->channels; i++) {
		/* Do endianess conversion */
		*(unsigned int *)dst_sample = bswap_32(*(unsigned int *)src_sample);

		/* Adjust padding: move unused bits from LSB (lower bits) to MSB (upper bits)*/
		*(unsigned int *)dst_sample = (*(unsigned int *)src_sample) >> 8;

		dst_sample += 4;
		src_sample += 4;
	}
}

static void alsa_swap_data_32(aar_alsa_handle_t *handle, void *src_frame, snd_pcm_uframes_t to_commit)
{
	void *dst_sample, *src_sample;
	int i;
	unsigned int bytes_per_sample = handle->frame_size / handle->channels;

	src_sample = dst_sample = src_frame;

	if (bytes_per_sample != 4)
		return;

	/* Do endianess conversion */
	for (i = 0; i < to_commit * handle->channels; i++) {
		*(unsigned int *)dst_sample = bswap_32(*(unsigned int *)src_sample);

		dst_sample += 4;
		src_sample += 4;
	}
}

static void alsa_swap_data_24(aar_alsa_handle_t *handle, void *src_frame, snd_pcm_uframes_t to_commit)
{
	void *dst_sample, *src_sample;
	int i;
	unsigned char tmp;
	unsigned int bytes_per_sample = handle->frame_size / handle->channels;

	src_sample = dst_sample = src_frame;

	if (bytes_per_sample != 3)
		return;

	/* Do endianess conversion */
	for (i = 0; i < to_commit * handle->channels; i++) {
		tmp = *(unsigned char *)(src_sample + 2);
		*(unsigned char *)(dst_sample + 2) = *(unsigned char *)(src_sample + 0);
		*(unsigned char *)(dst_sample + 1) = *(unsigned char *)(src_sample + 1);
		*(unsigned char *)(dst_sample + 0) = tmp;

		dst_sample += 3;
		src_sample += 3;
	}
}

static void alsa_swap_data_16(aar_alsa_handle_t *handle, void *src_frame, snd_pcm_uframes_t to_commit)
{
	void *dst_sample, *src_sample;
	int i;
	unsigned int bytes_per_sample = handle->frame_size / handle->channels;

	src_sample = dst_sample = src_frame;

	if (bytes_per_sample != 2)
		return;

	/* Do endianess conversion */
	for (i = 0; i < to_commit * handle->channels; i++) {
		*(unsigned short *)dst_sample = bswap_16(*(unsigned short *)src_sample);

		dst_sample += 2;
		src_sample += 2;
	}
}

/**
 * @brief      Initialize ALSA device
 *
 * @param[in]  handle          ALSA handle pointer
 * @param[in]  stream_params   AVB stream parameters
 * @param[in]  dev_name        ALSA device to be used
 *
 * @return     0 if everything are ok, else negative error code
 */
static int alsa_init(aar_alsa_handle_t *handle, struct avb_stream_params *stream_params, const char *dev_name)
{
	snd_pcm_t **snd_handle = &handle->handle;
	snd_pcm_hw_params_t *hw_params;
	snd_pcm_sw_params_t *sw_params;

	int err;

	snd_pcm_uframes_t buffer_size = 0;
	snd_pcm_uframes_t period_size = 0;
	int sub_dir;
	snd_pcm_stream_t dir = SND_PCM_STREAM_PLAYBACK;
	int alsa_period_time_ns;
	aar_alsa_param_t *alsa_param;

	int i;

	if (handle->direction == AAR_DATA_DIR_OUTPUT) {
		i = handle->device;
		alsa_param = (aar_alsa_param_t *)&g_alsa_playback_params[i];
		dir = SND_PCM_STREAM_PLAYBACK;
	} else {
		i = handle->device;
		alsa_param = (aar_alsa_param_t *)&g_alsa_capture_params[i];
		dir = SND_PCM_STREAM_CAPTURE;
	}

	handle->rate = avdecc_fmt_sample_rate(&stream_params->format);
	alsa_period_time_ns = alsa_param->period_time_ns;
	handle->channels = avdecc_fmt_channels_per_sample(&stream_params->format);
	handle->frame_size = avdecc_fmt_sample_size(&stream_params->format);
	handle->frame_duration = NSECS_PER_SEC / handle->rate;
	alsa_period_time_ns = max(alsa_period_time_ns, sr_class_interval_p(stream_params->stream_class) / sr_class_interval_q(stream_params->stream_class));

	/*Check format and set the right sample processing function*/
	if (avdecc_format_is_aaf_pcm(&stream_params->format)) {
		switch (avdecc_fmt_bits_per_sample(&stream_params->format)) {
		case 32:
			switch (avdecc_fmt_unused_bits(&stream_params->format)) {
			case 8:
				handle->alsa_process_samples = (handle->direction == AAR_DATA_DIR_OUTPUT) ?
					alsa_swap_data_32_adjust_padding_s24_le_output
					: alsa_adjust_padding_s24_le_input_swap_data_32;
				alsa_param->format = SND_PCM_FORMAT_S24_LE;
				break;

			case 0:
				handle->alsa_process_samples = alsa_swap_data_32;
				alsa_param->format = SND_PCM_FORMAT_S32_LE;
				break;

			default:
				ERR("%s : Unsupported bit depth for 32bits sample", dev_name);
				return -1;
			}

			break;

		default:
			ERR("%s : Unsupported Alsa format", dev_name);
			return -1;
		}
	} else if (avdecc_format_is_61883_6(&stream_params->format) && (AVDECC_FMT_61883_6_FDF_EVT(&stream_params->format) == IEC_61883_6_FDF_EVT_AM824)) {
		handle->alsa_process_samples = (handle->direction == AAR_DATA_DIR_OUTPUT) ?
			alsa_swap_data_32
			: alsa_add_61883_6_label_swap_data_32;
	} else {
			ERR("%s : Unsupported AVDECC format", dev_name);
			return -1;
	}

	// Calculate period size in frames
	period_size = alsa_ns_to_samples(alsa_period_time_ns, stream_params);

	if (handle->direction == AAR_DATA_DIR_OUTPUT) {
		/* For formats where the avtp timestamp doesn't match the first sample
		in the packet (e.g, 61883-6) there can be an extra maximum offset of
		packet size. As the buffer_size needs to be a multiple of period_size,
		adding a packet_size (which cannot be greater than 1 period_size) or a
		period _size is the same. But actually the equation should be:
		buffer_size = max_transit_time + CFG_ALSA_PLAYBACK_LATENCY_NS +
		period_size + packet_size; */
		buffer_size = alsa_ns_to_samples(sr_class_max_transit_time(stream_params->stream_class) + CFG_ALSA_PLAYBACK_LATENCY_NS, stream_params) + 2 * period_size;
		buffer_size = ((buffer_size + period_size - 1) / period_size) * period_size;
	} else {
		// Buffer size is double of period size
		buffer_size = period_size * 4;
	}

	// Restart the statistic counter
	handle->stats.counter_stats.tx_err = 0;
	handle->stats.counter_stats.rx_err = 0;
	handle->stats.counter_stats.period_rx = 0;
	handle->stats.counter_stats.period_tx = 0;

	DBG("snd_pcm_open handle %p, dev [%s], dir %d", snd_handle, dev_name, dir);
	if (dir == SND_PCM_STREAM_CAPTURE) {
		DBG("CAPTURE device");
	} else {
		DBG("PLAYBACK device");
	}

	if ((err = snd_pcm_open(snd_handle, dev_name, dir, 0)) < 0) {
		ERR("%s (%d): cannot open audio device (%s)", dev_name, handle->direction, snd_strerror(err));
		return -1;
	}
	DBG("snd_handle: %p", *snd_handle);

	if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
		ERR("%s (%d): cannot allocate hardware parameter structure(%s)", dev_name,
			handle->direction, snd_strerror(err));
		return -1;
	}

	if ((err = snd_pcm_hw_params_any(*snd_handle, hw_params)) < 0) {
		ERR("%s (%d): cannot initialize hardware parameter structure(%s)", dev_name,
			handle->direction, snd_strerror(err));
		return -1;
	}

	if ((err = snd_pcm_hw_params_set_rate_resample(*snd_handle, hw_params, 0)) < 0) {
		ERR ("Failed to set set_resample " );
		return -1;
	}

	if ((err = snd_pcm_hw_params_set_access(*snd_handle, hw_params, SND_PCM_ACCESS_MMAP_INTERLEAVED  )) < 0) {
		ERR("%s (%d): cannot set access type(%s)", dev_name, handle->direction, snd_strerror(err));
		return -1;
	}

	DBG("snd_pcm_hw_params_set_format format: %d", alsa_param->format);
	if ((err = snd_pcm_hw_params_set_format(*snd_handle, hw_params, alsa_param->format)) < 0) {
		ERR("%s (%d): cannot set sample format(%s)", dev_name, handle->direction, snd_strerror(err));
		return -1;
	}

	if ((err = snd_pcm_hw_params_set_channels(*snd_handle, hw_params, handle->channels)) < 0) {
		ERR("%s (%d): cannot set channel count(%s)", dev_name, handle->direction, snd_strerror(err));
		return -1;
	}

	if ((err = snd_pcm_hw_params_set_rate(*snd_handle, hw_params, handle->rate, 0)) < 0) {
		ERR("%s (%d): cannot set sample rate(%s)", dev_name, handle->direction, snd_strerror(err));
		return -1;
	}
	INF("%s (%d): current rate: %d", dev_name, handle->direction, handle->rate);

	if ((err = snd_pcm_hw_params_set_period_size_near(*snd_handle, hw_params, &period_size, &sub_dir)) < 0) {
		ERR("%s (%d): set period size to %d failed", dev_name, handle->direction, (int)period_size);
		return -1;
	}

	if ((err = snd_pcm_hw_params_set_buffer_size_near(*snd_handle, hw_params, &buffer_size)) < 0) {
		ERR("%s (%d): set buffer size to %d failed", dev_name, handle->direction, (int)buffer_size);
		return -1;
	}
	handle->buffer_size = buffer_size;
	INF("%s (%d): buffer_size = %d", dev_name, handle->direction, (int)buffer_size);

	if ((err = snd_pcm_hw_params_get_period_size(hw_params, &period_size, &sub_dir)) < 0) {
		ERR("%s (%d): get period size failed", dev_name, handle->direction);
		return -1;
	}
	handle->period_size = period_size;
	INF("%s (%d): period_size %d", dev_name, handle->direction, (int)period_size);

	if ((err = snd_pcm_hw_params(*snd_handle, hw_params)) < 0) {
		ERR("%s (%d): cannot set hw parameters(%s)", dev_name, handle->direction, snd_strerror(err));
		return -1;
	}

	snd_pcm_hw_params_free(hw_params);

	if ((err = snd_pcm_sw_params_malloc(&sw_params)) < 0) {
		ERR("%s (%d): cannot allocate software parameters structure(%s)", dev_name,
			handle->direction, snd_strerror(err));
		return -1;
	}

	if ((err = snd_pcm_sw_params_current(*snd_handle, sw_params)) < 0) {
		ERR("%s (%d): cannot initialize software parameters structure(%s)", dev_name,
			handle->direction, snd_strerror(err));
		return -1;
	}

	if ((err = snd_pcm_sw_params_set_stop_threshold(*snd_handle, sw_params, buffer_size)) < 0) {
		ERR("%s (%d): cannot set stop mode(%s)", dev_name, handle->direction, snd_strerror(err));
		return -1;
	}

	if ((err = snd_pcm_sw_params_set_period_event(*snd_handle, sw_params, 1)) < 0) {
		ERR("%s (%d): periodic events for capture(%s)", dev_name, handle->direction, snd_strerror(err));
		return -1;
	}

	if ((err = snd_pcm_sw_params(*snd_handle, sw_params)) < 0)
	{
		ERR("%s (%d): cannot set software parameters(%s)", dev_name, handle->direction, snd_strerror(err));
		return -1;
	}

	snd_pcm_sw_params_free (sw_params);

	if ((err = snd_pcm_prepare (*snd_handle)) < 0)
	{
		ERR ("%s (%d): Failed to prepare sound device ", dev_name, handle->direction);
		return -1;
	}

	snd_output_stdio_attach(&s_alsa_output, g_logfile_hdl, 0);
	INF("%s (%d): pcm_dump", dev_name, handle->direction);
	if ((err = snd_pcm_dump(*snd_handle, s_alsa_output)) < 0) {
		ERR("Failed to dump pcm");
		return -1;
	}

	snd_pcm_status_t *status;
	snd_pcm_status_alloca(&status);
	if ((err = snd_pcm_status(*snd_handle, status)) < 0) {
		ERR("Stream status error: %s", snd_strerror(err));
		return -1;
	}
	INF("%s (%d): pcm_status_dump", dev_name, handle->direction);
	if ((err = snd_pcm_status_dump(status, s_alsa_output)) < 0) {
		ERR("Failed to dump pcm");
		return -1;
	}

	return 0;
}

int alsa_tx_init(aar_alsa_handle_t *handle, struct avb_stream_params *stream_params, const char *alsa_device)
{
	int ret;

	DBG("handle: %p", handle);

	ret = alsa_init(handle, stream_params, alsa_device);
	if (ret < 0)
		goto err;

	handle->flags = 0;
	alsa_reset(handle);

	stats_init(&handle->stats.min_alsa_lat, 7, handle, alsa_tx_stats_min_alsa);
	stats_init(&handle->stats.alsa_latency, 31, handle, NULL);
	stats_init(&handle->stats.alsa_avail_samples, 31, handle, NULL);

err:
	return ret;
}

int alsa_rx_init(aar_alsa_handle_t *handle, struct avb_stream_params *stream_params, const char *alsa_device)
{
	int ret;

	DBG("handle: %p", handle);

	ret = alsa_init(handle, stream_params, alsa_device);
	if (ret < 0)
		goto err;

	handle->flags = 0;
	alsa_reset(handle);

	stats_init(&handle->stats.alsa_avail_samples, 31, handle, NULL);

err:
	return ret;
}

int alsa_tx_exit(aar_alsa_handle_t *handle)
{
	DBG("handle: %p", handle);
	handle->flags &= ~AAR_ALSA_RUNNING;

	if (snd_pcm_close(handle->handle) < 0) {
		return -1;
	}

	stats_reset(&handle->stats.min_alsa_lat);
	stats_reset(&handle->stats.alsa_latency);
	stats_reset(&handle->stats.alsa_avail_samples);
	return 0;
}

int alsa_rx_exit(aar_alsa_handle_t *handle)
{
	DBG("handle: %p", handle);
	handle->flags &= ~AAR_ALSA_RUNNING;
	if (snd_pcm_close(handle->handle) < 0) {
		return -1;
	}
	stats_reset(&handle->stats.alsa_avail_samples);

	return 0;
}

#define ALSA_RX_LATENCY		0	// immediate startup
int alsa_rx_start(aar_alsa_handle_t *handle)
{
	snd_pcm_t *snd_handle = handle->handle;
	const aar_alsa_param_t *alsa_param = alsa_get_param(handle);
	unsigned int port_id = 0; //FIXME
	int err;

	DBG("alsa handle: %p", handle);
	DBG("snd_handle: %p", snd_handle);
	DBG("Current ALSA device state: %d", snd_pcm_state(snd_handle));

	err = clock_gettime32(port_id, &handle->start_time);
	if (err < 0) {
		ERR("alsa(%p) clock_gettime32() failed", handle);
		return -1;  //FIXME
	}
	snd_pcm_start(snd_handle);
	handle->start_time += alsa_param->pcm_start_delay + handle->period_size * (NSECS_PER_SEC / handle->rate) + ALSA_RX_LATENCY;
	handle->flags = (AAR_ALSA_RUNNING | AAR_ALSA_FIRST_READ);

	stats_reset(&handle->stats.alsa_avail_samples);

	return 0;
}

int alsa_rx(aar_alsa_handle_t *alsa_handle, aar_avb_stream_t *avbstream)
{
	const snd_pcm_channel_area_t *areas;
	struct avb_stream_handle *stream_handle = avbstream->stream_handle;
	struct avb_event event;
	snd_pcm_sframes_t avail, delay;
	snd_pcm_uframes_t frames_remaining;
	snd_pcm_uframes_t offset, frames_to_commit;
	snd_pcm_t *snd_handle = alsa_handle->handle;
	int bytes_to_read;
	void *src_frame;
	int nbytes;
	int ret;
	int exchanged = 0;
	unsigned int gptp_time;
#if ALSA_EXTRA_DEBUG
	DBG("alsa(%p)", alsa_handle);
#endif

	// Calculate gPTP time to update stats
	if (clock_gettime32(avbstream->stream_params.port, &gptp_time) >= 0) {
		if (avbstream->is_first_wakeup) {
			// First wakeup, just store the time
			avbstream->is_first_wakeup = 0;
			avbstream->last_gptp_time = gptp_time;
		} else {
			// calculate stats
			stats_update(&avbstream->stats.gptp_2cont_wakeup, gptp_time - avbstream->last_gptp_time);
			avbstream->last_gptp_time = gptp_time;
		}
	}

	ret = snd_pcm_avail_delay(snd_handle, &avail, &delay);
	if (ret < 0) {
		alsa_handle->stats.counter_stats.rx_err ++;
		ERR("alsa(%p) - pcm_avail_delay error %s, pcm state: %d", alsa_handle, snd_strerror(ret), (int)snd_pcm_state(snd_handle));

		alsa_handle->flags &= ~AAR_ALSA_RUNNING;

		alsa_reset(alsa_handle);
		ret = alsa_rx_start(alsa_handle);
		if (ret < 0) {
			ERR("alsa(%p) - rx start failed %d", alsa_handle, ret);
		}

		return ret;
	}

	if (avail < 0) {
		alsa_handle->stats.counter_stats.rx_err ++;
#if ALSA_EXTRA_DEBUG
		ERR("*alsa(%p)-%d-%s", alsa_handle, (int)avail, snd_strerror(avail));
#endif
		alsa_reset(alsa_handle);
		ret = alsa_rx_start(alsa_handle);
		if (ret != 0) {
			ERR("alsa(%p) - rx start failed %d", alsa_handle, ret);
		}
		return ret;
	} else if (avail == 0) {
#if ALSA_EXTRA_DEBUG
		ERR("#alsa(%p)", alsa_handle);
#endif
		alsa_handle->stats.counter_stats.rx_err ++;
		return 0;
	}
#if ALSA_EXTRA_DEBUG
	DBG("+alsa(%p)-%d", alsa_handle, (int) avail);
#endif

	/* Do not process more than one period_size or it may lead to miss a wakeup */
	if (avail > alsa_handle->period_size) {
		frames_remaining = alsa_handle->period_size;
	} else
		frames_remaining = avail;

	// Update statistic
	stats_update(&alsa_handle->stats.alsa_avail_samples, avail);

	do {
		frames_to_commit = frames_remaining;
		ret = snd_pcm_mmap_begin(snd_handle, &areas, &offset, &frames_to_commit);
		if ((ret < 0) || (frames_to_commit == 0))  {
#if ALSA_EXTRA_DEBUG
			ERR("alsa(%p) - alsa_tx MMAP begin error %s", alsa_handle, snd_strerror(ret));
#endif
			alsa_handle->stats.counter_stats.rx_err ++;
			return 0;
		}
		bytes_to_read = frames_to_commit * alsa_handle->frame_size;
		src_frame = areas[0].addr + areas[0].first / 8 + offset * (areas[0].step / 8);

		/* perform audio sample processing: endianness swap, padding adjust ... */
		if (alsa_handle->alsa_process_samples)
			alsa_handle->alsa_process_samples(alsa_handle, src_frame, frames_to_commit);

		ret = bytes_to_read;
		do {
			if (alsa_handle->flags & AAR_ALSA_FIRST_READ) {
				event.index = 0;
				event.event_mask = AVTP_SYNC;
				event.ts = alsa_handle->start_time + avb_stream_presentation_offset(avbstream->stream_handle);
				nbytes = avb_stream_send(stream_handle, src_frame + (bytes_to_read - ret), ret, &event, 1);
				alsa_handle->flags &= ~AAR_ALSA_FIRST_READ;
			} else
				nbytes = avb_stream_send(stream_handle, src_frame + (bytes_to_read - ret), ret, NULL, 0);
			if (nbytes <= 0) {
#if ALSA_EXTRA_DEBUG
				ERR("?alsa(%p)-%d-%d", alsa_handle, ret, nbytes);
#endif
				/* Ignore the avb stack error and keep processing the alsa interface
				 * The error may be caused by avb stack exit (<0) or media queue full (=0)
				 */
				avbstream->stats.counter_stats.tx_err ++;
				break;
			}
			avbstream->stats.counter_stats.batch_tx ++;

			ret -= nbytes;
		} while(ret > 0);
#if ALSA_EXTRA_DEBUG
		DBG("<alsa(%p)-%d", alsa_handle, (int) (frames_to_commit));
#endif
		exchanged += bytes_to_read;

		ret = snd_pcm_mmap_commit(snd_handle, offset, frames_to_commit);
		if (ret != frames_to_commit) {
#if ALSA_EXTRA_DEBUG
			ERR("alsa(%p) - MMAP commit error %s", alsa_handle, snd_strerror(ret));
#endif
			alsa_handle->stats.counter_stats.rx_err ++;
			return 0;
		}
		frames_remaining -= frames_to_commit;
	} while (frames_remaining > 0);
	alsa_handle->stats.counter_stats.period_rx ++;

	return exchanged;
}

int alsa_get_fd_from_handle(aar_alsa_handle_t *handle, int *fd)
{
	struct pollfd ufds[2];
	int count;
	int err;

	DBG("alsa handle: %p", handle);
	count = snd_pcm_poll_descriptors_count(handle->handle);
	if (count <= 0) {
		ERR("ALSA pcm poll count = %d", count);
		return -1;
	}
	DBG("fd count = %d", count);
	if ((err = snd_pcm_poll_descriptors(handle->handle, ufds, 2)) < 0) {
		ERR("ALSA pcm get poll fds failed, %d", err);
		return -1;
	}

	/*
	 * The alsa API recommends polling on all descriptors and using snd_pcm_poll_descriptors_revents()
	 * to demangle the revents mask returned by poll().
	 * This method is not compatible with the thread API used here.
	 * Workaround: always poll on the first descriptor only.
	 */
	if (count > 1)
		INF("PCM device returned multiple file descriptors(%u), poll only on the first one.\n", count);

	*fd = ufds[0].fd;
	DBG("events = %x", ufds[0].events);

	return 0;
}

static const aar_alsa_param_t *alsa_get_param(aar_alsa_handle_t *handle)
{
	int i;
	// DBG("alsa handle: %p", handle);
	// Get device name
	if (handle->direction == AAR_DATA_DIR_OUTPUT) {
		i = handle->device;
		return &g_alsa_playback_params[i];
	} else {
		i = handle->device;
		return &g_alsa_capture_params[i];
	}

	return NULL;
}

static inline unsigned int alsa_compute_silence(aar_alsa_handle_t *handle, unsigned int max_frames, unsigned int *desired_time, unsigned int now, unsigned int total, unsigned int pcm_start)
{
	unsigned int silence_frames;
	unsigned int nsecs_per_frame = NSECS_PER_SEC / handle->rate;
	unsigned int max = max_frames - (total - pcm_start);

	if (avtp_after(now, *desired_time)) {
		if (total == pcm_start) { // We're late on the first iteration
			ERR("alsa_tx(%p) desired time (%u) likely in the past (now = %u), resetting to %d ns in the future",
					handle, *desired_time, now, CFG_ALSA_PLAYBACK_LATENCY_NS);
			*desired_time = now + CFG_ALSA_PLAYBACK_LATENCY_NS;
		} else { // We got late while adding silence, let's stop there
			ERR("alsa_tx(%p) desired time (%u) likely in the past, resetting to now(%u)",
					handle, *desired_time, now);
			*desired_time = now;
		}
	}

	silence_frames = *desired_time - now;
	silence_frames = silence_frames / nsecs_per_frame;

	if (silence_frames >= total)
		silence_frames -= total;
	else {
		ERR("alsa_tx(%p) now(%u) desired_time(%u) Added too many frames (%u instead of %u)",
				handle, now, *desired_time, total, silence_frames);
		silence_frames = 0;
	}

	if (silence_frames > max) {
		ERR("alsa_tx(%p) Amount of silence to add (%u) exceeds max space available, clamping to %u",
				handle, silence_frames, max);
		silence_frames = max;
	}


	return silence_frames;
}

static int listener_timestamp_accept(unsigned int ts, unsigned int now, aar_avb_stream_t *avbstream)
{
	/* Timestamp + playback offset must be after now (otherwise packet are too late) */
	/* Timestamp must be before now + transit time + timing uncertainty (otherwise they arrived too early) */
	if (avtp_after(ts + CFG_ALSA_PLAYBACK_LATENCY_NS, now)
	&& avtp_before(ts, now + sr_class_max_transit_time(avbstream->stream_params.stream_class)
				+ sr_class_max_timing_uncertainty(avbstream->stream_params.stream_class)))
		return 1;

	DBG("avb(%p) Invalid timestamp, ts - now: %d", avbstream->stream_handle , ts -now);

	return 0;
}

#define  START_COPY_SIZE	1024
#define  MAX_EVENTS		12
#define  MAX_DROPPED_PERIOD	4

int alsa_tx_start(aar_alsa_handle_t *handle, aar_avb_stream_t *avbstream)
{
	int err = 0;
	unsigned int desired_time, now, silence_frames, total, pcm_start_overhead;
#if ALSA_EXTRA_DEBUG
	unsigned int then, ptp_intvl;
#endif
	const aar_alsa_param_t *alsa_params = alsa_get_param(handle);
	unsigned int nsecs_per_frame = NSECS_PER_SEC / handle->rate;
	unsigned int port_id = avbstream->stream_params.port;
	struct avb_event event[MAX_EVENTS];
	unsigned int start_copy_size;
	unsigned int event_len;
	unsigned int tstamp;
	char buf[START_COPY_SIZE];
	snd_pcm_uframes_t offset, frames_read, frames_dropped;
	snd_pcm_uframes_t alsa_space, frames_written;
	snd_pcm_t *snd_handle = handle->handle;
	const snd_pcm_channel_area_t *areas;
	unsigned int bytes_to_read;
	void *src_frame;
	int idx;
	int end;
	uint64_t ts_offset;

	// DBG("alsa handle: %p, avb stream: %p", handle, avbstream);
	if (handle->flags & AAR_ALSA_RUNNING)
		return 0;

	alsa_reset(handle);

	//FIXME How many samples to fetch? Just enough to be sure of getting a time stamp? As much as possible?
	//FIXME Need to check more than one event possibly

	handle->stats.counter_stats.tx_start++;
	avbstream->last_exchanged_frames = 0;
	avbstream->last_event_frame_offset = 0;

	frames_dropped = 0;
	frames_read = 0;
	frames_written = 0;
	/* Round down the start copy size to multiple of frame size. */
	start_copy_size = (START_COPY_SIZE / handle->frame_size) * handle->frame_size;

	do {
		event_len = MAX_EVENTS;

		err = avb_stream_receive(avbstream->stream_handle, buf, min(start_copy_size, avbstream->cur_batch_size), event, &event_len);
		if (err < 0) {
			ERR("alsa(%p) - tx start failed on avb_stream_receive err(%d) event_len: %d", handle, err, event_len);
			err = -1;
			goto exit;
		}
		else if (!err) {
			ERR("alsa(%p) - tx start failed on avb_stream_receive: no data", handle);
			handle->stats.counter_stats.tx_start_no_data++;
			err = -1;
			goto exit;
		}

		frames_dropped += frames_read;
		frames_read = err / handle->frame_size;

		handle->stats.counter_stats.tx_start_drop += frames_dropped;
		if (frames_dropped >= (MAX_DROPPED_PERIOD * handle->period_size)) {
			err = -1;
			goto exit;
		}

		if (!get_clk_domain_validity(avbstream->stream_params.clock_domain)) {
			wait_clk_domain_validity++;
			err = -1;
			goto exit;
		}

		wait_clk_domain_validity = 0;

		err = clock_gettime32(port_id, &now);
		if (err < 0) {
			ERR("alsa(%p) clock_gettime failed for now", handle);
			goto exit;
		}

		if (event_len == 0) {
			ERR("alsa(%p) - tx start failed, no events received", handle);
			err = -1;
			goto exit;
		}

		for (idx = 0; idx < event_len; idx++) {
			if (event[idx].event_mask  & (AVTP_TIMESTAMP_INVALID | AVTP_TIMESTAMP_UNCERTAIN))
				continue;

			ts_offset = alsa_bytes_to_ns(event[idx].index, &avbstream->stream_params);
			event[idx].ts -= ts_offset;

			tstamp = event[idx].ts;
			if (listener_timestamp_accept(tstamp, now, avbstream))
				goto start;
		}
	}
	while (1);

start:
	pcm_start_overhead = alsa_params->pcm_start_delay / nsecs_per_frame;
	desired_time = tstamp + CFG_ALSA_PLAYBACK_LATENCY_NS;

	total = pcm_start_overhead;

	alsa_space = handle->buffer_size;
	err = snd_pcm_mmap_begin(snd_handle, &areas, &offset, &alsa_space);
	if (err < 0) {
		ERR("alsa(%p) MMAP begin error %s", handle, snd_strerror(err));
		goto exit;
	}

	if (alsa_space != handle->buffer_size) {
		ERR("alsa(%p) invalid alsa mmap_begin frames: %u", handle, (unsigned int)alsa_space);
		err = -1;
		goto exit;
	}

	/* Since the process of adding silence frames takes time by itself, we account for that delay by
	 * adding silence in several steps with a progressively lower number of frames at each step, and
	 * stop once the number of frames to be added is low enough.
	 * With 100 frames to add (about 2ms@48kHz), CFG_ALSA_MIN_SILENCE_FRAMES==8, and CFG_ALSA_PCM_START_DELAY==35000,
	 * this results in 3 steps: 75, 18, 6.
	 */
	end = 0;
	while (!end) {
		err = clock_gettime32(port_id, &now);
		if (err < 0) {
			ERR("alsa(%p) clock_gettime32() failed for now", handle);
			goto exit;
		}

		silence_frames = alsa_compute_silence(handle, handle->buffer_size, &desired_time, now, total, pcm_start_overhead);

		if (silence_frames > CFG_ALSA_MIN_SILENCE_FRAMES)
			silence_frames -= silence_frames >> 2;
		else
			end = 1;

		err = snd_pcm_areas_silence(areas, offset + frames_written, handle->channels, silence_frames, alsa_params->format);
		if (err < 0) {
			ERR("alsa_tx(%p) snd_pcm_areas_silence error", handle);
			goto exit;
		}

		total += silence_frames;
		frames_written += silence_frames;
	}

	total -= pcm_start_overhead;

	/* Start processing the first sample with a valid timestamp (shifted because of the index offset) to improve accuracy.  */
	bytes_to_read = frames_read * handle->frame_size;
	src_frame = areas[0].addr + areas[0].first / 8 + (offset + frames_written) * (areas[0].step / 8);
	memcpy(src_frame, buf, bytes_to_read);
	frames_written += frames_read;

	err = snd_pcm_mmap_commit(snd_handle, offset, frames_written);
	if (err != frames_written) {
		ERR("alsa(%p) MMAP commit error %s", handle, snd_strerror(err));
		err = -1;
		goto exit;
	}

#if ALSA_EXTRA_DEBUG
	err = clock_gettime32(port_id, &now);
	if (err < 0) {
		ERR("alsa(%p) clock_gettime32() failed for now", handle);
		goto exit;
	}
#endif

	err = snd_pcm_start(handle->handle);
	if (err < 0) {
		ERR("alsa(%p) Couldn't start stream: error = %s state = %d", handle, snd_strerror(err), snd_pcm_state(handle->handle));
		goto exit;
	}

#if ALSA_EXTRA_DEBUG
	err = clock_gettime32(port_id, &then);
	if (err < 0) {
		ERR("alsa(%p) clock_gettime32() failed for then", handle);
		goto exit;
	}

	/* All computations are done on unsigned 32-bit values, since tstamp and now are not
	 * supposed to be more than 2^32-1 ns away.
	 */
	ptp_intvl = then - now;
	now += (ptp_intvl / 2);  // (now+then)/2 would not return the right value in some wrapping cases


	INF("alsa(%p) now = %u     avtp_ts = %u",
			handle, now, tstamp);
	INF("alsa(%p) gettime accuracy intvl = %u ns     avtp_ts-now = %u us ",
			handle, ptp_intvl, (tstamp - now)/1000);
	INF("alsa(%p) initial silence = %u us     error = %d us      nsecs_per_frame = %d",
			handle, (total*nsecs_per_frame)/1000, (total*nsecs_per_frame)/1000 - (desired_time - now)/1000, nsecs_per_frame);
#endif
	stats_reset(&handle->stats.min_alsa_lat);
	stats_reset(&handle->stats.alsa_latency);
	stats_reset(&handle->stats.alsa_avail_samples);

	handle->flags |= AAR_ALSA_RUNNING;

	return frames_read;
exit:
	return err;
}


#define EVENT_LEN 16
int alsa_tx(aar_alsa_handle_t *alsa_handle, aar_avb_stream_t *avbstream)
{
	const snd_pcm_channel_area_t *areas;
	struct avb_stream_handle *stream_handle = avbstream->stream_handle;
	const aar_alsa_param_t *alsa_param = alsa_get_param(alsa_handle);
	snd_pcm_sframes_t avail, delay, start_frames;
	snd_pcm_uframes_t frames_remaining;
	snd_pcm_uframes_t offset, frames_to_commit, frames_committed;
	snd_pcm_t *snd_handle = alsa_handle->handle;
	int bytes_to_read;
	void *src_frame;
	int nbytes, i;
	int ret = 0;
	int exchanged = 0;
	struct avb_event event[EVENT_LEN] = {0};
	unsigned int event_len = EVENT_LEN;
	unsigned int gptp_time = 0;
	char is_first_event = 1;
	uint64_t ts_offset;

	// Calculate gPTP time to update stats
	if (clock_gettime32(avbstream->stream_params.port, &gptp_time) >= 0) {
		if (avbstream->is_first_wakeup) {
			// First wakeup, just store the time
			avbstream->last_gptp_time = gptp_time;
		} else {
			stats_update(&avbstream->stats.gptp_2cont_wakeup, gptp_time - avbstream->last_gptp_time);
			avbstream->last_gptp_time = gptp_time;
		}
	}

	// DBG("alsa handle: %p, avb stream: %p", alsa_handle, avbstream);
	if (!(alsa_handle->flags & AAR_ALSA_RUNNING)) {
#if ALSA_EXTRA_DEBUG
		DBG("alsa(%p)", alsa_handle);
#endif
		ret = alsa_tx_start(alsa_handle, avbstream);
		if (ret < 0) {
			alsa_handle->stats.counter_stats.tx_start_err++;
			return 0;
		}
	}

	start_frames = ret;
	ret = snd_pcm_avail_delay(snd_handle, &avail, &delay);
	if (ret < 0 || (avail < (avbstream->cur_batch_size / alsa_handle->frame_size)) || (delay < alsa_handle->period_size / 2)) {
		alsa_handle->stats.counter_stats.tx_err++;
		ERR("alsa(%p) - alsa_tx pcm_avail_delay error %s, pcm state: %d", alsa_handle,
			snd_strerror(ret), snd_pcm_state(snd_handle));
		alsa_handle->flags &= ~AAR_ALSA_RUNNING;

		ret = alsa_tx_start(alsa_handle, avbstream);
		if (ret < 0) {
			alsa_handle->stats.counter_stats.tx_start_err++;
			ERR("alsa(%p) - tx start failed %d", alsa_handle, ret);
		}
		return ret;
	}

#if ALSA_EXTRA_DEBUG
	DBG("+alsa(%p)-%d", alsa_handle, (int)avail);
#endif
	// Update statistic
	stats_update(&alsa_handle->stats.alsa_avail_samples, avail);

	// Calculate remaining frame
	frames_remaining = min(avail, (avbstream->cur_batch_size / alsa_handle->frame_size) - start_frames);

	stats_update(&alsa_handle->stats.min_alsa_lat,
		delay + (alsa_param->fifo_threshold * 4) / alsa_handle->frame_size);

	while (frames_remaining > 0) {
		frames_to_commit = frames_remaining;
		ret = snd_pcm_mmap_begin(snd_handle, &areas, &offset, &frames_to_commit);
		if ((ret < 0) || (frames_to_commit == 0)) {
#if ALSA_EXTRA_DEBUG
			ERR("alsa(%p) - alsa_tx MMAP begin error %s", alsa_handle, snd_strerror(ret));
#endif
			alsa_handle->stats.counter_stats.tx_err++;

			return 0;
		}
#if ALSA_EXTRA_DEBUG
		DBG("@alsa(%p)-%d", alsa_handle, (int)frames_to_commit);
#endif
		bytes_to_read = frames_to_commit * alsa_handle->frame_size;
		src_frame = areas[0].addr + areas[0].first / 8 + offset * (areas[0].step / 8);  // first and step are in bits
		event_len = EVENT_LEN;
		nbytes = avb_stream_receive(stream_handle, src_frame, bytes_to_read, event, &event_len);
		if (nbytes < alsa_handle->frame_size) {
#if ALSA_EXTRA_DEBUG
			ERR(":alsa(%p) %d %d\n", alsa_handle, bytes_to_read, nbytes);
#endif
			avbstream->stats.counter_stats.rx_err ++;

			ret = snd_pcm_mmap_commit(snd_handle, offset, 0);

			return 0;
		}

		if (event_len) {
			int idx;

			// Check if AVTP packet lost occurs on starting of batch
			for (i = 0; i < event_len; ++i) {
				if (event[i].event_mask & AVTP_PACKET_LOST) {
					ERR(":alsa(%p)-AVTP_PACKET_LOST %u bytes, event no %d\n", alsa_handle, event[i].event_data, i);

					avbstream->stats.counter_stats.rx_err++;

					ret = snd_pcm_mmap_commit(snd_handle, offset, 0);

					// Restart ALSA tx
					alsa_handle->flags &= ~AAR_ALSA_RUNNING;

					ret = alsa_tx_start(alsa_handle, avbstream);
					if (ret < 0) {
						alsa_handle->stats.counter_stats.tx_start_err++;
					}

					return ret;
				}
			}

			i = 0;
			while ((i < event_len) && (event[i].event_mask & (AVTP_TIMESTAMP_INVALID | AVTP_TIMESTAMP_UNCERTAIN))) i++;

			for (idx = i; idx < event_len; idx++) {

				if (event[idx].event_mask & (AVTP_TIMESTAMP_INVALID | AVTP_TIMESTAMP_UNCERTAIN))
					continue;

				ts_offset = alsa_bytes_to_ns(event[idx].index, &avbstream->stream_params);

				if (!listener_timestamp_accept(event[idx].ts - ts_offset, gptp_time, avbstream))
				{
					ret = snd_pcm_mmap_commit(snd_handle, offset, 0);
					alsa_handle->stats.counter_stats.tx_err++;

					// Restart ALSA tx
					alsa_handle->flags &= ~AAR_ALSA_RUNNING;

					ret = alsa_tx_start(alsa_handle, avbstream);
					if (ret < 0) {
						alsa_handle->stats.counter_stats.tx_start_err++;
					}

					return ret;
				}
			}

			if (i < event_len) {
				// Calculate event time statistic
				if (avbstream->is_first_wakeup) {
					avbstream->last_event_ts = event[i].ts;
					avbstream->last_event_frame_offset = avbstream->last_exchanged_frames + event[i].index / alsa_handle->frame_size;
				} else if (is_first_event) {
					unsigned int dt_elapsed = event[i].ts - avbstream->last_event_ts;
					unsigned int event_frame_offset = avbstream->last_exchanged_frames + event[i].index / alsa_handle->frame_size;
					unsigned int frames_elapsed = event_frame_offset - avbstream->last_event_frame_offset;

					avbstream->last_event_ts = event[i].ts;
					avbstream->last_event_frame_offset = event_frame_offset;

					if ( avbstream->last_exchanged_frames && ( abs(dt_elapsed - frames_elapsed * alsa_handle->frame_duration) > alsa_handle->frame_duration)) {
						ret = snd_pcm_mmap_commit(snd_handle, offset, 0);

						alsa_handle->stats.counter_stats.rx_err++;

						// Restart ALSA tx
						alsa_handle->flags &= ~AAR_ALSA_RUNNING;

						ret = alsa_tx_start(alsa_handle, avbstream);
						if (ret < 0) {
							alsa_handle->stats.counter_stats.tx_start_err++;
						}

						return ret;
					}

					is_first_event = 0;
					stats_update(&avbstream->stats.event_2cont_wakeup, dt_elapsed);
					stats_update(&avbstream->stats.event_gptp, event[i].ts - gptp_time);
				}
			} else // Invalid time stamp
				alsa_handle->stats.counter_stats.tx_err++;
		}

		avbstream->stats.counter_stats.batch_rx ++;
		frames_committed = nbytes / alsa_handle->frame_size;

#if ALSA_EXTRA_DEBUG
		DBG(">alsa(%p)-%d\n", alsa_handle, (int)frames_committed);
#endif
		exchanged += nbytes;
		avbstream->last_exchanged_frames += nbytes / alsa_handle->frame_size;

		/* perform audio sample processing: endianness swap, padding adjust ... */
		if (alsa_handle->alsa_process_samples)
			alsa_handle->alsa_process_samples(alsa_handle, src_frame, frames_to_commit);

		ret = snd_pcm_mmap_commit(snd_handle, offset, frames_committed);
		if (ret != frames_committed) {
			ERR("alsa_tx(%p): alsa_tx MMAP commit error %s", alsa_handle, snd_strerror(ret));
			return 0;
		}

		if ((nbytes < bytes_to_read) && (event_len <  EVENT_LEN)) {
			break;
		}
		frames_remaining -= frames_committed;
	}

	alsa_handle->stats.counter_stats.period_tx += exchanged / alsa_handle->frame_size;

	if (avbstream->is_first_wakeup) {
		// First wakeup, just store the time
		avbstream->is_first_wakeup = 0;
	}

	return exchanged;
}
