/*
 * Copyright 2018-2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Media interface handling
 @details RTOS-specific code for external media stack
*/

#include "genavb/media.h"
#include "common/log.h"
#include "common/types.h"
#include "os/media.h"
#include "os/net.h"

#include "net_port.h"
#include "media_queue.h"

int media_rx(struct media_rx *media, struct media_rx_desc **desc, unsigned int n)
{
	int rc;

	rc = media_net_read(media->id, desc, n);
	if (rc < 0)
		return -1;

	return rc;
}

int media_rx_avail(struct media_rx *media)
{
	return 0;
}

int media_tx(struct media_tx *media, struct media_desc **desc, unsigned int n)
{
	unsigned int i, written = 0;
	int rc;

	rc = media_net_write(media->id, desc, n);
	if (unlikely(rc < (int)n)) {
		if (rc >= 0)
			written = rc;
	} else
		return n;

	for (i = written; i < n; i++)
		net_rx_free((struct net_rx_desc *)desc[i]);

	if (rc < 0)
		return -1;
	else
		return written;
}

void media_rx_exit(struct media_rx *media)
{
	media_net_close(media->id);
	media->id = -1;
}

void media_tx_exit(struct media_tx *media)
{
	media_net_close(media->id);
	media->id = -1;
}

int media_rx_init(struct media_rx *media, void *stream_id, unsigned long priv, unsigned int flags, unsigned int header_len, unsigned int ts_offset)
{
	struct media_queue_net_params params;

	copy_64(params.stream_id, stream_id);
	params.talker.payload_offset = header_len + NET_DATA_OFFSET;

	/* Batching in the media queue driver adds a delay of up to latency (the first time the AVTP thread can check for new samples
	 * will be up to latency after the app posted samples). To be sure the media queue driver provides samples before the
	 * presentation offset is reached, it therefore needs to release samples as soon as their timestamps is less than
	 * presentation offset + latency in the future (otherwise, we would get the samples on the following wake-up, which may already
	 * be too late).
	 */
	params.talker.ts_offset = ts_offset;

	media->id = media_net_open(&params, 1);
	if (media->id < 0)
		goto err;

	if (flags & MEDIA_FLAG_WAKEUP) {
		/* FIXME need to add callback, event queue send? */
	}

	os_log(LOG_INFO, "media_rx(%p) id(%d)\n", media, media->id);

	return 0;

err:
	return -1;
}

int media_tx_init(struct media_tx *media, void *stream_id)
{
	struct media_queue_net_params params;

	copy_64(params.stream_id, stream_id);

	media->id = media_net_open(&params, 0);
	if (media->id < 0)
		goto err;

	os_log(LOG_INFO, "media_tx(%p) id(%d)\n", media, media->id);

	return 0;

err:
	return -1;
}

int media_rx_event_enable(struct media_rx *media)
{
	os_log(LOG_INFO, "media_rx(%p)\n", media);

	return 0;
}

int media_rx_event_disable(struct media_rx *media)
{
	os_log(LOG_INFO, "media_rx(%p)\n", media);

	return 0;
}
