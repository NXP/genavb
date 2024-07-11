/*
 * Copyright 2014 Freescale Semiconductor, Inc.
 * Copyright 2018-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Media interface handling
 @details Linux-specific code for external media stack
*/

#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "genavb/media.h"
#include "os/media.h"
#include "common/log.h"
#include "common/net.h"
#include "modules/media.h"
#include "epoll.h"
#include "shmem.h"

#define MEDIA_QUEUE_NET_FILE "/dev/media_queue_net"

static int media_init(int *fd, struct media_queue_net_params *params, int mode)
{
	*fd = open(MEDIA_QUEUE_NET_FILE, mode);
	if (*fd < 0) {
		unsigned long long *stream_id = (unsigned long long *)params->stream_id;
		os_log(LOG_ERR, "stream_id(%"PRIx64") open(%s) %s\n", ntohll(*stream_id), MEDIA_QUEUE_NET_FILE, strerror(errno));
		goto err_open;
	}

	if (ioctl(*fd, MEDIA_IOC_NET_BIND, params) < 0) {
		unsigned long long *stream_id = (unsigned long long *)params->stream_id;
		os_log(LOG_ERR, "stream_id(%"PRIx64") ioctl(MEDIA_IOC_BIND) %s\n", ntohll(*stream_id), strerror(errno));
		goto err_ioctl;
	}

	return 0;

err_ioctl:
	close(*fd);

err_open:
	return -1;
}

void media_rx_exit(struct media_rx *media)
{
	int fd = media->fd;
	os_log(LOG_DEBUG, "media->fd(%d)\n", fd);
	close(fd);
}

void media_tx_exit(struct media_tx *media)
{
	int fd = media->fd;
	os_log(LOG_DEBUG, "media->fd(%d)\n", fd);
	close(fd);
}

int media_rx_avail(struct media_rx *media)
{
	// TODO
	return 0;
}

int media_rx(struct media_rx *media, struct media_rx_desc **desc, unsigned int n)
{
	unsigned long addr[NET_TX_BATCH];
	unsigned int _read = 0;
	unsigned int n_now;
	int fd = media->fd;
	int len, i;
	int rc = 0;

	while (_read < n) {
		n_now = n - _read;
		if (n_now > NET_TX_BATCH)
			n_now = NET_TX_BATCH;

		len = n_now * sizeof(unsigned long);
		rc = read(fd, addr, len);
		if (rc <= 0)
			break;

		rc /= sizeof(unsigned long);

		for (i = 0; i < rc; i++)
			desc[_read + i] = shmem_to_virt(addr[i]);

		_read += rc;

		if (rc < n_now)
			break;
	}

	if (_read)
		return _read;
	else
		return rc;
}

int media_rx_init(struct media_rx *media, void *stream_id, unsigned long priv, unsigned int flags, unsigned int header_len, unsigned int ts_offset)
{
	int epoll_fd = priv;
	struct media_queue_net_params params;
	int ret;

	copy_64(params.stream_id, stream_id);
	params.talker.payload_offset = header_len + NET_DATA_OFFSET;

	/* Batching in the media queue driver adds a delay of up to latency (the first time the AVTP thread can check for new samples
	 * will be up to latency after the app posted samples). To be sure the media queue driver provides samples before the
	 * presentation offset is reached, it therefore needs to release samples as soon as their timestamps is less than
	 * presentation offset + latency in the future (otherwise, we would get the samples on the following wake-up, which may already
	 * be too late).
	 */
	params.talker.ts_offset = ts_offset;

	ret = media_init(&media->fd, &params, O_RDONLY);
	if (ret < 0)
		return ret;

	if (flags & MEDIA_FLAG_WAKEUP) {
		if (epoll_ctl_add(epoll_fd, media->fd, EPOLL_TYPE_MEDIA, media, &media->epoll_data, EPOLLIN) < 0) {
			os_log(LOG_ERR, "stream_id(%"PRIu64") epoll_ctl_add() failed\n", ntohll(*(uint64_t *)stream_id));
			goto err_epoll_ctl;
		}
	}

	os_log(LOG_INFO, "media_rx(%p) fd(%d)\n", media, media->fd);

	media->epoll_fd = epoll_fd;

	return 0;

err_epoll_ctl:
	media_rx_exit(media);

	return -1;
}


int media_tx(struct media_tx *media, struct media_desc **desc, unsigned int n)
{
	unsigned long addr[NET_RX_BATCH];
	unsigned int n_now, i;
	unsigned int written = 0;
	int fd = media->fd;
	int len;
	int rc = 0;

	while (written < n) {
		n_now = n - written;
		if (n_now > NET_RX_BATCH)
			n_now = NET_RX_BATCH;

		for (i = 0; i < n_now; i++)
			addr[i] = virt_to_shmem(desc[written + i]);

		len = n_now * sizeof(unsigned long);
		rc = write(fd, addr, len);
		if (rc < len) {
			if (rc < 0)
				goto err;

			written += rc / sizeof(unsigned long);
			goto err;
		}

		written += n_now;
	}

	return written;

err:
	for (i = written; i < n; i++)
		net_rx_free((struct net_rx_desc *)desc[i]);

	if (written)
		return written;
	else
		return rc;
}


int media_tx_init(struct media_tx *media, void *stream_id)
{
	struct media_queue_net_params params;
	int ret;

	copy_64(params.stream_id, stream_id);

	ret = media_init(&media->fd, &params, O_WRONLY);
	if (ret < 0)
		return ret;

	os_log(LOG_INFO, "media_tx(%p) fd(%d)\n", media, media->fd);

	return 0;
}

int  media_rx_event_enable(struct media_rx *media)
{
	os_log(LOG_DEBUG, "media_rx(%p) epoll_fd %d\n", media, media->epoll_fd);

	if (epoll_ctl_add(media->epoll_fd, media->fd, EPOLL_TYPE_MEDIA, media, &media->epoll_data, EPOLLIN) < 0) {
		os_log(LOG_ERR, "media(%p) epoll_ctl_add() failed\n", media);
		goto err_epoll_ctl;
	}

	return 0;

err_epoll_ctl:
	return -1;
}

int  media_rx_event_disable(struct media_rx *media)
{
	os_log(LOG_DEBUG, "media_rx(%p) epoll_fd %d\n", media, media->epoll_fd);

	if (epoll_ctl_del(media->epoll_fd, media->fd) < 0) {
		os_log(LOG_ERR, "media(%p) epoll_ctl_del() failed\n", media);
		goto err_epoll_ctl;
	}

	return 0;

err_epoll_ctl:
	return -1;
}
