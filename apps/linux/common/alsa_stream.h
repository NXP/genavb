/*
 * Copyright 2017, 2020, 2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __ALSA_STREAM_H__
#define __ALSA_STREAM_H__

#include <alsa/asoundlib.h>
#include "thread.h"
#include "alsa2.h"
#include "avb_stream_config.h"
#include "stats.h"
#include "stream_stats.h"

#define APP_FLAG_FIRST_HANDLE	(1 << 0)

struct alsa_stream_stats {
	struct stream_stats gen_stats;
	unsigned long alsa_handle_ptr;    /**< ALSA handle pointer value, just for printout */
	int alsa_device;        /**< ALSA device value, just for printout */
	int alsa_direction;     /**< ALSA device direction, just for printout */

	aar_alsa_stats_t alsa_stats; /**< ALSA statistic structure */
};

struct alsa_stream {
	unsigned int created;
	unsigned int index;
	thr_thread_slot_t *thread_slot;
	aar_alsa_handle_t alsa_handle;
	aar_avb_stream_t *avb_stream;
	char *alsa_device;

	struct alsa_stream_stats stats;
};

void alsa_stats_dump(struct alsa_stream_stats *stats);
int talker_alsa_handler(void *data, unsigned int events);
int talker_alsa_connect(struct alsa_stream *talker, struct avb_stream_params *params);
void talker_alsa_disconnect(struct alsa_stream *talker);
int listener_alsa_handler(void *data, unsigned int events);
int listener_alsa_connect(struct alsa_stream *listener, struct avb_stream_params *params);
void listener_alsa_disconnect(struct alsa_stream *listener);


#endif /* __ALSA_STREAM_H__ */
