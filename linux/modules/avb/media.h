/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _MEDIA_DRV_H_
#define _MEDIA_DRV_H_

#include "genavb/types.h"

struct media_queue_api_params {
	unsigned int port;
	unsigned char stream_id[8];
	unsigned int frame_stride;			/**< Space in bytes between the start of 2 media frames in the payload part of the packet (e.g. 192 bytes for 61883-4).
										 * If 0 (common case), the whole payload will be treated as media data with no holes in it. */
	unsigned int frame_size;			/**< Size of a media frame in bytes. May be less than frame_stride (e.g. 188 bytes for 61883-4) */
	unsigned int queue_size;			/** Size of the queue in ??? */
	unsigned int batch_size;			/** Size of a batch in ??? */  // TODO determine size based on what? stream bandwidth and max pkt size?
	unsigned int max_payload_size;			/**< Maximum size of the AVTP payload in bytes. Used in talker mode to split incoming stream of data into properly sized chunks. */
	unsigned int flags;				/**< Possible flags: AVTP_DGRAM when in datagram mode. */
};

struct media_queue_net_params {
	unsigned char stream_id[8];

	struct {
		unsigned int payload_offset;		/**< offset in bytes between start of buffers and start of AVTP payload, in bytes. */
		unsigned int ts_offset;			/**< offset in ns to be used when passing samples to the AVTP thread */
	} talker;
};


struct media_queue_rx {
	struct genavb_iovec const *data_iov;
	unsigned int data_iov_len;
	struct genavb_iovec const *event_iov;
	unsigned int event_iov_len;
	unsigned int data_read;			/**< return value */
	unsigned int event_read;		/**< return value */
};

struct media_queue_tx {
	struct genavb_iovec const *data_iov;
	unsigned int data_iov_len;
	struct genavb_event const *event;
	unsigned int event_len;
};

#define IOV_MAX		32

#ifdef __KERNEL__

#include <linux/cdev.h>
#include <linux/string.h>
#include <linux/atomic.h>


#define MEDIA_DRV_NAME		"media_drv"
#define MEDIA_DRV_MINOR		0
#define MEDIA_DRV_NET_MINOR	0
#define MEDIA_DRV_API_MINOR	1
#define MEDIA_DRV_MINOR_COUNT	2


/* Media queue character device instance */
struct media_queue {
	void *partial_desc;

	unsigned int port;
	u8 stream_id[8];
	atomic_t available;
	atomic_t eofs;
	unsigned int flags;
	wait_queue_head_t api_wait;
	wait_queue_head_t net_wait;

	spinlock_t lock;

	struct media_drv *drv;

	struct list_head list;

	unsigned int max_payload_size;
	unsigned int max_frame_payload_size;
	unsigned int payload_offset;
	unsigned int frame_stride;
	unsigned int frame_size;
	unsigned int batch_size;
	unsigned int offset;

	void *src;
	int src_len;
	int src_frame_len;
	unsigned int event_src_idx;

	void *dst;
	unsigned int dst_len;
	unsigned int dst_frame_len;
	unsigned int ts_dst_idx;
	unsigned int ts_dst_offset;
	unsigned int ts_dst_len;

	struct queue queue;			/* Contains pointers to media_descs */
						/* Placed last so that we can allocate a dynamic queue size */
};

#define MEDIA_QUEUE_FLAGS_TALKER		(1 << 0)
#define MEDIA_QUEUE_FLAGS_NET_BOUND		(1 << 1)
#define MEDIA_QUEUE_FLAGS_API_BOUND		(1 << 2)
#define MEDIA_QUEUE_FLAGS_BOUND_MASK		(MEDIA_QUEUE_FLAGS_NET_BOUND | MEDIA_QUEUE_FLAGS_API_BOUND)

#define MEDIA_QUEUE_FLAGS_DGRAM			(1 << 3)

static inline unsigned int media_queue_avail(struct media_queue *mqueue)
{
	return atomic_read(&mqueue->available);
}

static inline unsigned int media_queue_eofs(struct media_queue *mqueue)
{
	return atomic_read(&mqueue->eofs);
}

static inline unsigned int media_queue_remaining(struct media_queue *mqueue)
{
	unsigned int n = queue_available(&mqueue->queue);

	if (n) {
		if (mqueue->flags & MEDIA_QUEUE_FLAGS_DGRAM)
			return n;
		else
			return mqueue->max_frame_payload_size * (n - 1);
	} else
		return 0;
}



struct media_drv {
	struct list_head media_queues;
	struct cdev cdev_net;
	struct cdev cdev_api;
	struct mutex list_lock;
	dev_t devno;
};

int media_drv_init(struct media_drv *drv);
void media_drv_exit(struct media_drv *drv);

#endif /* !__KERNEL__ */

#define MEDIA_IOC_MAGIC		'm'

#define MEDIA_IOC_NET_BIND		_IOW(MEDIA_IOC_MAGIC, 1, struct media_queue_net_params)
#define MEDIA_IOC_API_BIND		_IOWR(MEDIA_IOC_MAGIC, 1, struct media_queue_api_params)
#define MEDIA_IOC_RX		_IOR(MEDIA_IOC_MAGIC, 2, struct media_queue_rx)
#define MEDIA_IOC_TX		_IOW(MEDIA_IOC_MAGIC, 3, struct media_queue_tx)

#endif /* _MEDIA_DRV_H_ */
