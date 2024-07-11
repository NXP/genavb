/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2019-2020, 2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __ALSA2_H__
#define __ALSA2_H__

#include <alsa/asoundlib.h>
#include "alsa_config.h"
#include "avb_stream_config.h"
#include "stats.h"

typedef enum _DATA_STREAM_DIRECTION {
        AAR_DATA_DIR_INPUT = 1,         /**< Input direction */
        AAR_DATA_DIR_OUTPUT             /**< Output direction */
} aar_device_direction_t;


/**
 * @addtogroup alsa
 * @{
 */
typedef enum {
	AAR_ALSA_RUNNING =    0x01,
	AAR_ALSA_FIRST_READ = 0x02,
} aar_alsa_flags_t;

/** ALSA counter statistic */
typedef struct {
	unsigned int tx_err;     /**< ALSA write error counter */
	unsigned int rx_err;     /**< ALSA read error counter */

	unsigned int period_rx;  /**< ALSA period received counter */
	unsigned int period_tx;  /**< ALSA period transmitted counter */

	unsigned int tx_start;
	unsigned int tx_start_drop;
	unsigned int tx_start_err;
	unsigned int tx_start_no_data;
} aar_alsa_counter_stats_t;

typedef struct {
	struct stats alsa_latency;
	struct stats min_alsa_lat;

	struct stats alsa_avail_samples;         /**< ALSA available samples */
	aar_alsa_counter_stats_t counter_stats;  /**< ALSA simple counter statistic */
} aar_alsa_stats_t;

/** ALSA wrapper handle structure */
typedef struct _ALSA_HANDLE_STRUCTURE {
	snd_pcm_t *handle;                /**< ALSA handle pointer */
	unsigned int device;      /**< ALSA input/output device */
	aar_device_direction_t direction; /**< ALSA device direction */
	int flags;                        /**< ALSA flag, mixing from aar_alsa_flags_t */
	snd_pcm_uframes_t buffer_size;    /**< ALSA buffer size of this handle */
	snd_pcm_uframes_t period_size;    /**< ALSA period size of this handle */
	unsigned int frame_size;          /**< Size in bytes of each ALSA frame */
	unsigned int frame_duration;      /**< Duration in ns of each ALSA frame */
	unsigned int rate;
	unsigned int channels;
	unsigned int start_time;          /**< gPTP time of snd_pcm_start (used for talkers only) */
	aar_alsa_stats_t stats;           /**< ALSA statistics */
	void (*alsa_process_samples)(struct _ALSA_HANDLE_STRUCTURE *handle, void *src_frame, snd_pcm_uframes_t to_commit);	/**< Custom function per stream to do sample processing
														  	once for all to enhance performance:
															endianness swap, label adding, padding adjust ...*/
} aar_alsa_handle_t;

extern const aar_alsa_param_t g_alsa_playback_params[MAX_ALSA_PLAYBACK];
extern const aar_alsa_param_t g_alsa_capture_params[MAX_ALSA_CAPTURE];

/** Open and initialize an ALSA PCM playback handle
 *
 * @details    This function will open a ALSA audio device, initialize and configure this device.
 *             Information of the handle must be set before call this function.
 *
 * @param      handle            Pointer of ALSA handle
 * @param      stream_params     AVB stream parameters
 * @param      alsa_device       ALSA device
 *
 * @return     0 if success or negative error code
 */
int alsa_tx_init(aar_alsa_handle_t *handle, struct avb_stream_params *stream_params, const char *alsa_device);

/** Open and initialize an ALSA PCM capture handle
 *
 * @param      handle        ALSA handle pointer
 * @param      stream_params     AVB stream parameters
 * @param      alsa_device       ALSA device
 *
 * @return     0 if success or negative error code
 */
int alsa_rx_init(aar_alsa_handle_t *handle, struct avb_stream_params *stream_params, const char *alsa_device);

/** Close and free an ALSA PCM playback handle
 *
 * @param      handle  The handle
 *
 * @return     0 if success or negative error code
 */
int alsa_tx_exit(aar_alsa_handle_t *handle);

/** Close and free an ALSA PCM capture handle
 *
 * @param      handle  The handle
 *
 * @return     0 if success or negative error code
 */
int alsa_rx_exit(aar_alsa_handle_t *handle);

/**
 * @brief      Get file description from ALSA handle
 *
 * @param[in]  handle  Pointer of ALSA handle
 * @param[out] fd      Pointer of file descriptor var
 *
 * @return     0 if success or negative error code
 */
int alsa_get_fd_from_handle(aar_alsa_handle_t *handle, int *fd);

/** Write data into ALSA playback handle
 *
 * @param[in]  handle     The ALSA handle
 * @param[in]  avbstream  The AVB stream
 *
 * @return     Number of written data (in bytes) or negative error code
 */
int alsa_tx(aar_alsa_handle_t *handle, aar_avb_stream_t *avbstream);

/** Read data from ALSA capture handle
 *
 * @param[in]  handle     The ALSA handle
 * @param[in]  avbstream  The AVB stream
 *
 * @return     Number of received data (in bytes) or negative error code
 */
int alsa_rx(aar_alsa_handle_t *handle, aar_avb_stream_t *avbstream);

/** Handles synchronized start of playback (based on AVTP presentation time)
 *
 * @param[in]  handle     The ALSA handle
 * @param[in]  avbstream  The AVB stream
 *
 * @return     0 if success or negative error code
 *
 * This function will add silence frames at the beginning of the ring buffer before starting playback so
 * that the next frame to be posted to ALSA will be played out at the requested tstamp + a fixed value.
 * Since tstamp is a u32, it is assumed that tstamp will not be more than 2^32-1 ns in the future, or about 4.2 seconds.
 *
 * The delay to be added is therefore given by:
 *  delay = tstamp + PLAYBACK_LATENCY_NS - current_time - current_alsa_delay
 *  where:
 *  	. current_time is the current gPTP time in ns modulo 2^32
 *  	. current_alsa_delay is the current ALSA playout delay (based on the number of frames already queued)
 *  	. PLAYBACK_LATENCY_NS is the fixed value to be added (in ns), to account for the per frame media stack processing
 */
int alsa_tx_start(aar_alsa_handle_t *handle, aar_avb_stream_t *avbstream);

/** Handles synchronized start of capture and is able to assign a gptp time to audio samples (used
 * with AVTP_SYNC event)
 *
 * @param      handle  The ALSA handle
 *
 * @return     0 if success or negative error code
 */
int alsa_rx_start(aar_alsa_handle_t *handle);

/** @} */

#endif /* __ALSA2_H__ */
