/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <math.h>
#include <errno.h>
#include <byteswap.h>
#include <poll.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <time.h>

#include "../common/time.h"
#include "alsa.h"


#define min(a,b)  ((a)<(b)?(a):(b))

#define K		1024
#define EVENT_BUF_SZ	(K)
#define MIN_BUFFER_TO_BATCH_SIZE_RATIO	4

/** Configure actual ALSA device based on the alsa tx context information.
 * @alsa: alsa tx context to configure
 *
 * Returns 0 on success or ALSA error code otherwise.
 */
int alsa_set_format(struct alsa_common *alsa, snd_pcm_stream_t direction)
{
	snd_pcm_t *handle = alsa->handle;
	snd_pcm_hw_params_t *hwparams = alsa->hwparams;
	snd_pcm_sw_params_t *swparams = alsa->swparams;
	snd_pcm_uframes_t period_size, buffer_size_ns;
	int ret = 0, dir;
	snd_pcm_access_t access_type = SND_PCM_ACCESS_MMAP_INTERLEAVED;
	int resample = 0;			/* disable alsa-lib resampling */
	unsigned int configured_rate;

	/* choose all parameters */
	ret = snd_pcm_hw_params_any(handle, hwparams);
	if (ret < 0) {
		printf("alsa(%p) Broken configuration: no configurations available: %s\n", alsa, snd_strerror(ret));
		return ret;
	}
	/* set hardware resampling */
	ret = snd_pcm_hw_params_set_rate_resample(handle, hwparams, resample);
	if (ret < 0) {
		printf("alsa(%p) Resampling setup failed: %s\n", alsa, snd_strerror(ret));
		return ret;
	}
	/* set the interleaved read/write format */
	ret = snd_pcm_hw_params_set_access(handle, hwparams, access_type);
	if (ret < 0) {
		printf("alsa(%p) Access type not available: %s\n", alsa, snd_strerror(ret));
		return ret;
	}

	ret = snd_pcm_hw_params_set_format(handle, hwparams, alsa->format);
	if (ret < 0) {
		printf("alsa(%p) Sample format not available: %s\n", alsa, snd_strerror(ret));
		return ret;
	}

	if (alsa->channels_per_frame) {
		ret = snd_pcm_hw_params_set_channels(handle, hwparams, alsa->channels_per_frame);
		if (ret < 0) {
			printf("alsa(%p) Channels count (%i) not available: %s\n", alsa, alsa->channels_per_frame, snd_strerror(ret));
			return ret;
		}
	}

	configured_rate = alsa->rate;
	ret = snd_pcm_hw_params_set_rate_near(handle, hwparams, &configured_rate, 0);
	if (ret < 0) {
		printf("alsa(%p) Rate %iHz not available: %s\n", alsa, alsa->rate, snd_strerror(ret));
		return ret;
	}
	if (configured_rate != alsa->rate) {
		printf("alsa(%p) Rate doesn't match (requested %iHz, get %iHz)\n", alsa, alsa->rate, configured_rate);
		return -1; //FIXME define proper error return codes
	}

	if (direction == SND_PCM_STREAM_CAPTURE)
		period_size = CFG_ALSA_CAPTURE_LATENCY_NS / alsa->nsecs_per_frame;
	else
		period_size = CFG_ALSA_PERIOD_SIZE;

	/* set the period size */
	ret = snd_pcm_hw_params_set_period_size_near(handle, hwparams, &period_size, &dir);
	if (ret < 0) {
		printf("alsa(%p) Unable to set period size %u: %s\n", alsa, (unsigned int)period_size, snd_strerror(ret));
		return ret;
	}

	/* set the buffer size, round up to power of two */
	if (direction == SND_PCM_STREAM_CAPTURE)
		buffer_size_ns = CFG_ALSA_CAPTURE_BUFFER_SIZE_NS;
	else
		buffer_size_ns = CFG_ALSA_PLAYBACK_BUFFER_SIZE_NS;

	alsa->buffer_size = 1;
	while (((alsa->buffer_size * 100000) / alsa->rate) < (buffer_size_ns / 10000))
		alsa->buffer_size <<= 1;

	while (alsa->buffer_size < (MIN_BUFFER_TO_BATCH_SIZE_RATIO * alsa->batch_size))
		alsa->buffer_size <<= 1;

	ret = snd_pcm_hw_params_set_buffer_size_near(handle, hwparams, &alsa->buffer_size);
	if (ret < 0) {
		printf("alsa(%p) Unable to set buffer size %u: %s\n", alsa, (unsigned int)alsa->buffer_size, snd_strerror(ret));
		return ret;
	}

	ret = snd_pcm_hw_params_get_period_size(hwparams, &period_size, &dir);
	if (ret < 0) {
		printf("alsa(%p) Unable to get period size: %s\n", alsa, snd_strerror(ret));
		return ret;
	}

	printf("alsa(%p) buffer size %u, period size %u\n", alsa, (unsigned int)alsa->buffer_size, (unsigned int)period_size);

	/* write the parameters to device */
	ret = snd_pcm_hw_params(handle, hwparams);
	if (ret < 0) {
		printf("alsa(%p) Unable to set hw params: %s\n", alsa, snd_strerror(ret));
		return ret;
	}

	/* get the current swparams */
	ret = snd_pcm_sw_params_current(handle, swparams);
	if (ret < 0) {
		printf("alsa(%p) Unable to determine current swparams: %s\n", alsa, snd_strerror(ret));
		return ret;
	}

	ret = snd_pcm_sw_params_set_stop_threshold(handle, swparams, alsa->buffer_size);
	if (ret < 0) {
		printf("alsa(%p) Unable to set stop threshold mode: %s\n", alsa, snd_strerror(ret));
		return ret;
	}

	if ((ret = snd_pcm_sw_params_set_period_event(handle, swparams, 1)) < 0) {
		printf("Unable to enable periodic events for capture: %s\n", snd_strerror(ret));
		return ret;
	}

	ret = snd_pcm_sw_params(handle, swparams);
	if (ret < 0) {
		printf("alsa(%p) Unable to set sw params: %s\n", alsa, snd_strerror(ret));
		return ret;
	}
	return ret;
}

/* This function swap endianness then adjust padding for AAF 24/32 bits format for alsa playback:
 * As S24_LE alsa is putting the padding in the upper 8 bits (MSB padding) which will result when converting in
 * Big endian for AVTPDU to have the padding in the lower bits which contradicts AVTP IEEE 1722-2016 7.3.4
 * that imposes the unused bits to be at the upper bits inside the AVTPDU (LSB padding)
 * Apply this function on samples in LE format (e.g for talker : just after the capture from alsa and before converting
 * to BE network format and for listener: before alsa playback and after converting from BE network format)
 */
static void alsa_swap_data_32_adjust_padding_s24_le_playback(struct alsa_tx *alsa, void *src_frame, snd_pcm_uframes_t to_commit)
{
	void *dst_sample, *src_sample;
	int i;

	src_sample = dst_sample = src_frame;

	if ((alsa->common.bytes_per_sample != 4) || (alsa->common.direction != SND_PCM_STREAM_PLAYBACK))
		return;

	for (i = 0; i < to_commit * alsa->common.channels_per_frame; i++) {
		/* Do endianess conversion */
		*(unsigned int *)dst_sample = bswap_32(*(unsigned int *)src_sample);
		/* Adjust padding: move unused bits from LSB (lower bits) to MSB (upper bits)*/
		*(unsigned int *)dst_sample = (*(unsigned int *)src_sample) >> 8;

		dst_sample += 4;
		src_sample += 4;
	}
}

static void alsa_swap_data_32(struct alsa_tx *alsa, void *src_frame, snd_pcm_uframes_t to_commit)
{
	void *dst_sample, *src_sample;
	int i;

	src_sample = dst_sample = src_frame;

	if (alsa->common.bytes_per_sample != 4)
		return;

	/* Do endianess conversion */
	for (i = 0; i < to_commit * alsa->common.channels_per_frame; i++) {
		*(unsigned int *)dst_sample = bswap_32(*(unsigned int *)src_sample);

		dst_sample += 4;
		src_sample += 4;
	}
}

static void alsa_swap_data_24(struct alsa_tx *alsa, void *src_frame, snd_pcm_uframes_t to_commit)
{
	void *dst_sample, *src_sample;
	int i;
	unsigned char tmp;

	src_sample = dst_sample = src_frame;

	if (alsa->common.bytes_per_sample != 3)
		return;

	/* Do endianess conversion */
	for (i = 0; i < to_commit * alsa->common.channels_per_frame; i++) {
		tmp = *(unsigned char *)(src_sample + 2);
		*(unsigned char *)(dst_sample + 2) = *(unsigned char *)(src_sample + 0);
		*(unsigned char *)(dst_sample + 1) = *(unsigned char *)(src_sample + 1);
		*(unsigned char *)(dst_sample + 0) = tmp;

		dst_sample += 3;
		src_sample += 3;
	}
}

static void alsa_swap_data_16(struct alsa_tx *alsa, void *src_frame, snd_pcm_uframes_t to_commit)
{
	void *dst_sample, *src_sample;
	int i;

	src_sample = dst_sample = src_frame;

	if (alsa->common.bytes_per_sample != 2)
		return;

	/* Do endianess conversion */
	for (i = 0; i < to_commit * alsa->common.channels_per_frame; i++) {
		*(unsigned short *)dst_sample = bswap_16(*(unsigned short *)src_sample);

		dst_sample += 2;
		src_sample += 2;
	}
}

int alsa_common_init(struct alsa_common *alsa, struct avdecc_format *avdecc_format, snd_pcm_stream_t direction, unsigned int batch_size, const char *alsa_device)
{
	int err;

	err = snd_output_stdio_attach(&alsa->output, stdout, 0);
	if (err < 0) {
		printf("alsa(%p) Output failed: %s\n", alsa, snd_strerror(err));
		goto hwparams_malloc_err;
	}

	err = snd_pcm_hw_params_malloc(&alsa->hwparams);
	if (err < 0) {
		printf("alsa(%p) Cannot allocate ALSA HW param structure (%s)\n", alsa, snd_strerror(err));
		goto hwparams_malloc_err;
	}

	err = snd_pcm_sw_params_malloc(&alsa->swparams);
	if (err < 0) {
		printf("alsa(%p) Cannot allocate ALSA SW param structure (%s)\n", alsa, snd_strerror(err));
		goto swparams_malloc_err;
	}

	if ((err = snd_pcm_open(&alsa->handle, alsa_device, direction, 0)) < 0) {
		printf("alsa(%p) Playback open error: %s\n", alsa, snd_strerror(err));
		goto pcm_open_err;
	}

	alsa->avdecc_format = *avdecc_format;
	alsa->batch_size = batch_size;
	alsa->direction = direction;
	/* To avoid unaligned accesses and simplify code a bit, unused_bits will be handled by shifting the value and
	 * padding with zeroes.
	 */
	alsa->rate = avdecc_fmt_sample_rate(&alsa->avdecc_format);
	alsa->bytes_per_sample = avdecc_fmt_bits_per_sample(&alsa->avdecc_format) /8;
	alsa->channels_per_frame = avdecc_fmt_channels_per_sample(&alsa->avdecc_format);
	alsa->frame_size = avdecc_fmt_sample_size(&alsa->avdecc_format);
	alsa->unused_bits = avdecc_fmt_unused_bits(&alsa->avdecc_format);
	alsa->nsecs_per_frame = NSECS_PER_SEC/alsa->rate;
	if (avdecc_fmt_audio_is_float(&alsa->avdecc_format))
		alsa->format = SND_PCM_FORMAT_FLOAT_LE; // Assume float type is always 32bit
	else {
		switch (alsa->bytes_per_sample) {
		case 4:
			switch (alsa->unused_bits) {
			case 8:
				alsa->format = SND_PCM_FORMAT_S24_LE;
				break;

			case 0:
				alsa->format = SND_PCM_FORMAT_S32_LE;
				break;

			default:
				goto config_err;
				break;
			}

			break;

			case 3:
				switch (alsa->unused_bits) {
				case 0:
					alsa->format = SND_PCM_FORMAT_S24_3LE;
					break;

				default:
					goto config_err;
					break;
				}

				break;

				case 2:
					switch (alsa->unused_bits) {
					case 0:
						alsa->format = SND_PCM_FORMAT_S16_LE;
						break;

					default:
						goto config_err;
						break;
					}

					break;

					default:
						goto config_err;
						break;
		}
	}

	/* set the sample format */
	err = alsa_set_format(alsa, direction);
	if (err < 0)
		goto config_err;

	return 0;


config_err:
	snd_pcm_close(alsa->handle);
pcm_open_err:
	snd_pcm_sw_params_free(alsa->swparams);
swparams_malloc_err:
	snd_pcm_hw_params_free(alsa->hwparams);
hwparams_malloc_err:
	return err;
}

void alsa_common_exit(struct alsa_common *common)
{
	snd_pcm_close(common->handle);
	snd_pcm_sw_params_free(common->swparams);
	snd_pcm_hw_params_free(common->hwparams);
}

static void alsa_tx_stats_min_alsa(struct stats *s)
{
	struct alsa_tx *alsa = s->priv;

	stats_update(&alsa->latency, s->min);
}

static void alsa_tx_stats_alsa(struct stats *s)
{
	struct alsa_tx *alsa = s->priv;

	printf("alsa_tx(%p) frame: %8u alsa latency     %4d/%4d/%4d (us)\n", alsa, alsa->common.count,
			alsa->latency.min * USECS_PER_SEC / alsa->common.rate,
			alsa->latency.mean * USECS_PER_SEC / alsa->common.rate,
			alsa->latency.max * USECS_PER_SEC / alsa->common.rate);
}

static void alsa_tx_stats_frame_rate(struct stats *s)
{
	struct alsa_tx *alsa = s->priv;

	printf("alsa_tx(%p) frame: %8u input frame rate                     %4d/%4d/%4d (fps)\n", alsa, alsa->common.count,
			alsa->frame_rate.min, alsa->frame_rate.mean, alsa->frame_rate.max);
}

/** Opens and initialize the ALSA device.
 * @stream_id: pointer to the u64 stream ID value.
 * @avdecc_format: pointer to the AVDECC format of the stream that will be received
 * @batch_size: size of the batches coming from the GenAVB library, in bytes
 * @alsa_device: Alsa device to be used for playback.
 *
 * Returns an alsa tx context structure.
 */
void *alsa_tx_init(void *stream_id, struct avdecc_format *avdecc_format, unsigned int batch_size, const char *alsa_device)
{
	int err;
	struct alsa_tx *alsa;

	alsa = malloc(sizeof(struct alsa_tx));
	if (!alsa) {
		printf("alsa_tx_int Malloc allocation error for stream %llu.\n", *(unsigned long long *)stream_id);
		goto alsa_malloc_err;
	}

	memset(alsa, 0, sizeof(struct alsa_tx));

	err = alsa_common_init(&alsa->common, avdecc_format, SND_PCM_STREAM_PLAYBACK, batch_size, alsa_device);

	if (err < 0) {
		printf("alsa_tx(%p) Output failed: %s\n", alsa, snd_strerror(err));
		goto hwparams_malloc_err;
	}

	/*Set the right process sample function*/
	switch (alsa->common.bytes_per_sample) {
	case 4:
		if (alsa->common.format == SND_PCM_FORMAT_S24_LE && avdecc_format_is_aaf_pcm(&alsa->common.avdecc_format))
			alsa->alsa_process_samples = alsa_swap_data_32_adjust_padding_s24_le_playback;
		else
			alsa->alsa_process_samples = alsa_swap_data_32;

		break;
	case 3:
		alsa->alsa_process_samples = alsa_swap_data_24;
		break;
	case 2:
		alsa->alsa_process_samples = alsa_swap_data_16;
		break;
	default:
		printf("alsa_tx(%p) Unsupported bytes_per_sample %u\n", alsa, alsa->common.bytes_per_sample);
		goto alsa_bytes_per_sample_err;
	}

	snd_pcm_dump(alsa->common.handle, alsa->common.output);

	alsa->common.count = 0;
	alsa->common.running = 0;

	stats_init(&alsa->latency_min, 7, alsa, alsa_tx_stats_min_alsa);
	stats_init(&alsa->latency, 5, alsa, alsa_tx_stats_alsa);
	stats_init(&alsa->frame_rate, 7, alsa, alsa_tx_stats_frame_rate);

	//printf("alsa_tx(%p) done\n", alsa);

	return alsa;

alsa_bytes_per_sample_err:
	alsa_common_exit(&alsa->common);
hwparams_malloc_err:
	free(alsa);
alsa_malloc_err:
	return NULL;
}

void alsa_tx_exit(void *priv)
{
	struct alsa_tx *alsa = (struct alsa_tx *)priv;
	alsa_common_exit(&alsa->common);
	free(alsa);

	//printf("done\n");
}

/** Write frames of silence into an ALSA ring buffer
 * @alsa: alsa_tx context to write to
 * @n_frames: number of silence frames to insert
 *
 * Returns 0 on success or the alsa_lib error code otherwise.
 * Note: since this function is used to start a stream, and starting a stream is done
 * to recover from Xruns, we don't try and recover from any ALSA errors here.
 */
static int alsa_write_silence(struct alsa_tx *alsa, snd_pcm_uframes_t n_frames)
{
	snd_pcm_uframes_t offset, to_commit, size = n_frames;
	const snd_pcm_channel_area_t *areas;
	int err;

	while (size > 0) {
		to_commit = size;
		err = snd_pcm_avail_update(alsa->common.handle);
		if (err < 0) {
			printf("alsa_tx(%p) snd_pcm_avail_update error\n", alsa);
			goto alsa_err;
		}
		err = snd_pcm_mmap_begin(alsa->common.handle, &areas, &offset, &to_commit);
		if (err < 0) {
			printf("alsa_tx(%p) MMAP begin error\n", alsa);
			goto alsa_err;
		}

		//printf("alsa_tx(%p) size: %d to_commit: %d\n", alsa, size, to_commit);
		err = snd_pcm_areas_silence(areas, offset, alsa->common.channels_per_frame, to_commit, alsa->common.format);
		if (err < 0) {
			printf("alsa_tx(%p) snd_pcm_areas_silence error\n", alsa);
			goto alsa_err;
		}

		err = snd_pcm_mmap_commit(alsa->common.handle, offset, to_commit);
		if (err < 0) {
			printf("alsa_tx(%p) MMAP commit error\n", alsa);
			goto alsa_err;
		}

		size -= to_commit;
	}

	return 0;

alsa_err:
	printf("alsa_tx(%p) frame: %8u Recovering from error %s   state = %d\n", alsa, alsa->common.count, snd_strerror(err), snd_pcm_state(alsa->common.handle));
	return err;

}

int alsa_tx(void *priv, struct avb_stream_handle *stream_h, struct avb_stream_params *stream_params)
{
	struct alsa_tx *alsa = (struct alsa_tx *)priv;
	int ret = 0;
	const snd_pcm_channel_area_t *areas;
	void *src_frame;
	snd_pcm_uframes_t offset, frames_to_commit, frames_committed;
	snd_pcm_uframes_t frames_remaining;
	snd_pcm_sframes_t avail, delay;
	unsigned int bytes_to_read, bytes_written;
	int nbytes;

	/* if the stream is not started, start it now after adding some buffer padding ...*/
	if (!alsa->common.running) {
		snd_pcm_drop(alsa->common.handle);
		snd_pcm_prepare(alsa->common.handle);
		snd_pcm_reset(alsa->common.handle);
		alsa_write_silence(alsa, CFG_ALSA_PLAYBACK_LATENCY_NS / alsa->common.nsecs_per_frame);
		ret = snd_pcm_start(alsa->common.handle);
		if (ret < 0) {
			printf("alsa_tx(%p) Couldn't start stream: error = %s state = %d\n", alsa, snd_strerror(ret), snd_pcm_state(alsa->common.handle));
			goto exit;
		}
		alsa->common.running = 1;

		if (!gettime_ns(&alsa->frame_rate_last)) {
			alsa->frame_rate_count = 0;
			stats_reset(&alsa->frame_rate);
		}
		else
			printf("alsa_tx(%p) Couldn't get time for frame rate statistics\n", alsa);
	}


	ret = snd_pcm_avail_delay(alsa->common.handle, &avail, &delay);
	if (ret < 0) {
		printf("alsa_tx(%p) pcm_avail_delay error\n", alsa);
		goto alsa_err;
	}

	if ((avail * alsa->common.frame_size) < alsa->common.batch_size) {
		printf("alsa_tx(%p) frame: %8u ALSA ring buffer almost full, restarting stream\n", alsa, alsa->common.count);
		goto alsa_err;
	}

#define	ESAI_FIFO_THRESHOLD 64

	stats_update(&alsa->latency_min, delay + ESAI_FIFO_THRESHOLD / alsa->common.channels_per_frame);

	frames_remaining = avail;
	bytes_written = 0;
	do {
		frames_to_commit = frames_remaining;

		ret = snd_pcm_mmap_begin(alsa->common.handle, &areas, &offset, &frames_to_commit);
		if (ret < 0) {
			printf("alsa_tx(%p) MMAP begin error\n", alsa);
			goto alsa_err;
		}

		bytes_to_read = frames_to_commit*alsa->common.frame_size;
		src_frame = areas[0].addr + areas[0].first / 8 + offset * (areas[0].step / 8);
		nbytes = avb_stream_receive(stream_h, src_frame, bytes_to_read, NULL, NULL);
		if (nbytes <= 0) {
			if (nbytes < 0)
				printf("avb_stream_receive error stream_handle(%p) ret(%d) data(%p) len(%u) size(%u) frames_remaining(%u)\n", stream_h, nbytes, src_frame, bytes_to_read, alsa->common.frame_size, (unsigned int)frames_remaining);

			ret = snd_pcm_mmap_commit(alsa->common.handle, offset, 0);
			if (ret != 0) {
				printf("alsa_tx(%p) MMAP commit error while recovering from avb_stream_receive_error(%s)\n", alsa, avb_strerror(nbytes));
				goto alsa_err;
			}
			if (nbytes < 0)
				ret = nbytes;
			else
				ret = bytes_written;
			goto exit;
		}

		frames_committed = nbytes / alsa->common.frame_size;

		if (alsa->alsa_process_samples)
			alsa->alsa_process_samples(alsa, src_frame, frames_committed);

		ret = snd_pcm_mmap_commit(alsa->common.handle, offset, frames_committed);
		if (ret != frames_committed) {
			printf("alsa_tx(%p) MMAP commit error\n", alsa);
			goto alsa_err;
		}

		frames_remaining -= frames_committed;
		bytes_written += nbytes;
		alsa->common.count += frames_committed;
		alsa->frame_rate_count += frames_committed;
	} while (nbytes == bytes_to_read);

	ret = bytes_written;
	goto exit;

alsa_err:
	printf("alsa_tx(%p) frame: %8u Recovering from error %s   state = %d avail = %d delay = %d\n", alsa, alsa->common.count, snd_strerror(ret), snd_pcm_state(alsa->common.handle), (int)avail, (int)delay);
	alsa->common.running = 0;

exit:
	if (alsa->frame_rate_count >= (alsa->common.rate >> 4) ) {
		uint64_t now;

		if (!gettime_ns(&now)) {
			unsigned int frame_rate = ((unsigned long long)alsa->frame_rate_count * NSECS_PER_SEC) / (now - alsa->frame_rate_last);
			stats_update(&alsa->frame_rate, frame_rate);
			alsa->frame_rate_last = now;
			alsa->frame_rate_count = 0;
		}
	}

	return ret;
}
