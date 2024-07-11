/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2024 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/sched.h>

#include "genavb/media.h"
#include "genavb/types.h"
#include "genavb/avtp.h"
#include "genavb/streaming.h"
#include "avtp.h"
#include "queue.h"
#include "pool.h"
#include "avbdrv.h"
#include "media.h"


/**
 * DOC: Media interface driver
 *
 * Provides interface between AVB network stack and media application/stack using a character device driver.
 * The interface provides a dedicated queue per AVB stream, with read/write/poll/ioctl api's.
 * The media queues are indentified by a stream id and have two ends, one bound to the AVB stack
 * the other to the media stack.
 * Both the AVB network stack (avtp thread) and media stack open a media queue and then try to bind
 * to opposite ends, to exchange data and control information.
 * A data copy is always done in the driver, which allows data to be moved between media stack buffers
 * and AVB network stack buffers. This means AVB network buffers are never shared with media application/stack.
 *
 */

#define FRAME_STRIDE_MAX	1024
#define PAYLOAD_OFFSET_MIN	NET_DATA_OFFSET
#define PAYLOAD_OFFSET_MAX	(2 * NET_DATA_OFFSET)

#if (NET_PAYLOAD_SIZE_MAX > (BUF_SIZE - NET_DATA_OFFSET))
#error "Invalid NET_PAYLOAD_SIZE_MAX"
#endif

static inline int need_to_wake_up_listener_queue(struct media_queue * mqueue)
{
	return (media_queue_avail(mqueue) >= mqueue->batch_size) || queue_full(&mqueue->queue) || media_queue_eofs(mqueue);
}

/**
 * media_queue_alloc() - allocates media queue
 * @drv - media driver pointer
 * @flags - media queue flags
 *
 */
static struct media_queue *media_queue_alloc(struct media_drv *drv, unsigned int flags)
{
	struct media_queue *mqueue;

	mqueue = kzalloc(sizeof(*mqueue) + sizeof(unsigned long) * CFG_MEDIA_QUEUE_EXTRA_ENTRIES, GFP_KERNEL);
	if (mqueue) {
		mqueue->drv = drv;
		mqueue->flags = flags;

		init_waitqueue_head(&mqueue->api_wait);
		init_waitqueue_head(&mqueue->net_wait);

		/* Account for extra entries */
		/* TODO add queue size param coming from api */
		queue_init(&mqueue->queue, pool_dma_free_virt, CFG_MEDIA_QUEUE_EXTRA_ENTRIES);

		spin_lock_init(&mqueue->lock); // FIXME do we need it?
		atomic_set(&mqueue->available, 0);
		atomic_set(&mqueue->eofs, 0);
	}

	return mqueue;
}

static void media_queue_flush_partial(struct media_queue *mqueue)
{
	struct avb_drv *avb = container_of(mqueue->drv, struct avb_drv, media_drv);

	if (mqueue->partial_desc) {
		if (mqueue->flags & MEDIA_QUEUE_FLAGS_TALKER) {
			((struct avb_desc *)mqueue->partial_desc)->len = (mqueue->dst - mqueue->partial_desc) - mqueue->payload_offset;
			((struct avb_desc *)mqueue->partial_desc)->offset = mqueue->payload_offset;
		}

		pool_dma_free(&avb->buf_pool, mqueue->partial_desc);

		mqueue->partial_desc = NULL;
	}
}

static void media_queue_flush(struct media_queue *mqueue)
{
	struct avb_drv *avb = container_of(mqueue->drv, struct avb_drv, media_drv);

	queue_flush(&mqueue->queue, &avb->buf_pool);

	media_queue_flush_partial(mqueue);

	atomic_set(&mqueue->available, 0);
	atomic_set(&mqueue->eofs, 0);
}

static void media_queue_free(struct media_queue *mqueue)
{
	// if deduplicate added the queue to the list, then at least one bound flag is set
	if (mqueue->flags & MEDIA_QUEUE_FLAGS_BOUND_MASK)
		list_del(&mqueue->list);

	kfree(mqueue);
}

/**
 * media_queue_bind_start() - starts process of binding a media queue to a stream id
 * @drv - media driver pointer
 * @mqueue - media queue pointer
 * @stream_id - stream id
 *
 * If a queue exists that is already bound to the same stream_id and queue end an error is returned.
 * If a queue exists that is already bound to the same stream_id but opposite queue end, that queue is returned.
 * Otherwise the original queue is returned.
 *
 */
static int media_queue_bind_start(struct media_drv *drv, struct media_queue **mqueue, void *stream_id, unsigned int bound_flag)
{
	struct list_head *item;
	struct media_queue *mqueue_item;

	// TODO Implement hash for better search performance
	list_for_each(item, &drv->media_queues) {
		mqueue_item = list_entry(item, struct media_queue, list);

		if (stream_id_match(stream_id, mqueue_item->stream_id) && (((*mqueue)->flags & MEDIA_QUEUE_FLAGS_TALKER) == (mqueue_item->flags & MEDIA_QUEUE_FLAGS_TALKER))) {
			// We found an existing entry
			if ((bound_flag & mqueue_item->flags & MEDIA_QUEUE_FLAGS_BOUND_MASK) != 0) {
				// We're trying to bind a queue in a direction that is already bound
				return -EINVAL;
			}

			*mqueue = mqueue_item;

			return 0;
		}
	}

	stream_id_copy((*mqueue)->stream_id, stream_id);

	return 0;
}

/**
 * media_queue_bind_finish() - finishes process of binding a media queue to a stream id
 * @drv - media driver pointer
 * @mqueue - media queue pointer
 *
 */
static void media_queue_bind_finish(struct media_drv *drv, void **private_data, struct media_queue *mqueue, unsigned int bound_flag)
{
	// Deduplicate may have changed the mqueue we use, so we need to update the private data
	if (*private_data != mqueue)
		*private_data = mqueue;

	if (!(mqueue->flags & MEDIA_QUEUE_FLAGS_BOUND_MASK))
		list_add(&mqueue->list, &drv->media_queues);

	mqueue->flags |= bound_flag;
}


/**
 * media_drv_net_write() - AVB stack listener stream write
 * @buf - array of avb media descriptors pointers
 * @len - length of array (in bytes)
 * @off - file offset pointer
 *
 * AVB stack calls this function to write received network data to media stack (i.e, listener stream receive).
 *
 */
static ssize_t media_drv_net_write(struct file *file, const char __user *buf, size_t len, loff_t *off)
{
	struct media_queue *mqueue = file->private_data;
	struct avb_drv *avb = container_of(mqueue->drv, struct avb_drv, media_drv);
	struct media_desc *desc;
	unsigned long addr_shmem;
	int i, rc = 0;
	unsigned int desc_len, written = 0;
	unsigned int qa, write;


	if (*off)
		return -ESPIPE;

	if (mqueue->flags & MEDIA_QUEUE_FLAGS_TALKER)
		return -EPERM;

	if ((mqueue->flags & MEDIA_QUEUE_FLAGS_BOUND_MASK) != MEDIA_QUEUE_FLAGS_BOUND_MASK)
		return -EPIPE;

	len /= sizeof(unsigned long);

	qa = queue_available(&mqueue->queue);
	if (len > qa)
		len = qa;

	queue_enqueue_init(&mqueue->queue, &write);

	for (i = 0; i < len; i++) {
		if (get_user(addr_shmem, &((unsigned long *)buf)[i]) < 0) {
			rc = -EFAULT;
			break;
		}

		if (addr_shmem >= BUF_POOL_SIZE) {
			pr_err("%s: desc(%lx) outside of pool range\n", __func__, addr_shmem);
			rc = -EFAULT;
			break;
		}

		desc = pool_dma_shmem_to_virt(&avb->buf_pool, addr_shmem);

		if (desc->len > NET_PAYLOAD_SIZE_MAX) {
			pr_err("%s: desc(%lx), len(%u) too big\n", __func__, addr_shmem, desc->len);
			rc = -EINVAL;
			break;
		}

		if ((desc->l2_offset + desc->len) > BUF_SIZE) {
			pr_err("%s: desc(%lx), offset(%u) too big\n", __func__, addr_shmem, desc->l2_offset);
			rc = -EINVAL;
			break;
		}

		if (desc->n_ts > MEDIA_TS_PER_PACKET) {
			pr_err("%s: desc(%lx), n_ts(%u) is not valid\n", __func__, addr_shmem, desc->n_ts);
			rc = -EINVAL;
			break;
		}

		if (mqueue->frame_stride != mqueue->frame_size)
			desc_len = (desc->len * mqueue->frame_size) / mqueue->frame_stride;
		else
			desc_len = desc->len;

		queue_enqueue_next(&mqueue->queue, &write, (unsigned long) desc);

		/* The End-of-Frame marker is assumed always to be at the end of a packet, or at least always to be the last event in a packet. */
		if (desc->n_ts && (desc->avtp_ts[desc->n_ts - 1].flags & AVTP_FLAGS_TO_MEDIA_DESC(AVTP_END_OF_FRAME)))
			atomic_inc(&mqueue->eofs);

		written += desc_len;
	}

	queue_enqueue_done(&mqueue->queue, write);

	if (written) {
		atomic_add(written, &mqueue->available);

		if (need_to_wake_up_listener_queue(mqueue)) {
			if (waitqueue_active(&mqueue->api_wait))
				wake_up(&mqueue->api_wait);
		}
	}

	if (i > 0)
		return i * sizeof(unsigned long);
	else
		return rc;
}

/**
 * media_drv_net_read() - AVB stack talker stream read
 * @buf - array of avb media descriptors pointers
 * @len - length of array (in bytes)
 *
 * AVB stack calls this function to read network data to transmit from media stack (i.e, talker stream transmit).
 *
 */
static ssize_t media_drv_net_read(struct file *file, char __user *buf, size_t len, loff_t *off)
{
	struct media_queue *mqueue = file->private_data;
	struct avb_drv *avb = container_of(mqueue->drv, struct avb_drv, media_drv);
	struct media_rx_desc *desc;
	unsigned long addr_shmem;
	void *addr;
	unsigned int ptp_now, ptp_read = 0;
	unsigned int n = 0;
	unsigned int read, _read, qp;
	int rc = 0;

	if (*off)
		return -ESPIPE;

	if ((mqueue->flags & MEDIA_QUEUE_FLAGS_TALKER) == 0)
		return -EPERM;

	len /= sizeof(unsigned long);

	qp = queue_pending(&mqueue->queue);
	if (len > qp)
		len = qp;

	queue_dequeue_init(&mqueue->queue, &read);

	_read = read;
	while (len) {
		addr = (void *)queue_dequeue_next(&mqueue->queue, &_read);
		desc = (struct media_rx_desc *) addr;

		if (desc->ts_n) {
			if (!ptp_read)
				if (!eth_avb_read_ptp(&avb->eth[mqueue->port], &ptp_now))
					ptp_read = 1;

//			pr_info("%p %u %u %u %u\n", desc, desc->ts_n, ptp_read, ptp_now, desc->avtp_ts[0].val);

			if (ptp_read && avtp_before(ptp_now + mqueue->offset, desc->avtp_ts[0].val))
				break;
		}

		addr_shmem = pool_dma_virt_to_shmem(&avb->buf_pool, addr);

		if (put_user(addr_shmem, &((unsigned long *)buf)[n]) < 0) {
			rc = -EFAULT;
			break;
		}

		read = _read;
		len--;
		n++;
	}

	queue_dequeue_done(&mqueue->queue, read);

	if ((media_queue_remaining(mqueue) >= mqueue->batch_size) || queue_empty(&mqueue->queue)) {
		if (waitqueue_active(&mqueue->api_wait))
			wake_up(&mqueue->api_wait);
	}

	if (n)
		return n * sizeof(unsigned long);
	else
		return rc;
}



static int media_params_check_listener(struct media_queue *mqueue, unsigned int flags)
{
	int rc = 0;

	if (flags & MEDIA_QUEUE_FLAGS_API_BOUND) {
		if (mqueue->port >= CFG_PORTS) {
			rc = -EINVAL;
			goto out;
		}

		if (!mqueue->frame_stride || !mqueue->frame_size) {
			rc = -EINVAL;
			goto out;
		}

		if (mqueue->frame_stride < mqueue->frame_size) {
			rc = -EINVAL;
			goto out;
		}

		if (mqueue->frame_stride > FRAME_STRIDE_MAX) {
			rc = -EINVAL;
			goto out;
		}

		if (!mqueue->batch_size) {
			rc = -EINVAL;
			goto out;
		}

		if (mqueue->flags & MEDIA_QUEUE_FLAGS_DGRAM) {
			if (mqueue->batch_size > queue_available(&mqueue->queue) / 4)
				mqueue->batch_size = queue_available(&mqueue->queue) / 4;
		} else {
			if (!mqueue->max_payload_size) {
				rc = -EINVAL;
				goto out;
			}

			if (mqueue->frame_stride > mqueue->max_payload_size) {
				rc = -EINVAL;
				goto out;
			}

			mqueue->max_frame_payload_size = (mqueue->max_payload_size / mqueue->frame_stride) * mqueue->frame_size;

			if (mqueue->batch_size < mqueue->max_frame_payload_size) {
				rc = -EINVAL;
				goto out;
			}

			if (mqueue->batch_size > media_queue_remaining(mqueue) / 4)
				mqueue->batch_size = media_queue_remaining(mqueue) / 4;

			mqueue->batch_size = (mqueue->batch_size / mqueue->max_frame_payload_size) * mqueue->max_frame_payload_size;
		}
	}

out:
	return rc;
}


static int media_params_check_talker(struct media_queue *mqueue, unsigned int flags)
{
	int rc = 0;

	if (flags & MEDIA_QUEUE_FLAGS_NET_BOUND) {
		if ((mqueue->payload_offset < PAYLOAD_OFFSET_MIN) || (mqueue->payload_offset > PAYLOAD_OFFSET_MAX)) {
			rc = -EINVAL;
			goto out;
		}
	}

	if (flags & MEDIA_QUEUE_FLAGS_API_BOUND) {
		if (mqueue->port >= CFG_PORTS) {
			rc = -EINVAL;
			goto out;
		}

		if (!mqueue->frame_stride || !mqueue->frame_size || !mqueue->max_payload_size) {
			rc = -EINVAL;
			goto out;
		}

		if (mqueue->frame_stride < mqueue->frame_size) {
			rc = -EINVAL;
			goto out;
		}

		if (mqueue->frame_stride > FRAME_STRIDE_MAX) {
			rc = -EINVAL;
			goto out;
		}

		if ((PAYLOAD_OFFSET_MIN + mqueue->max_payload_size) > NET_PAYLOAD_SIZE_MAX) {
			rc = -EINVAL;
			goto out;
		}

		if (!mqueue->batch_size) {
			rc = -EINVAL;
			goto out;
		}

		if (mqueue->frame_stride > mqueue->max_payload_size) {
			rc = -EINVAL;
			goto out;
		}

		if ((mqueue->max_payload_size / mqueue->frame_stride) * mqueue->frame_stride != mqueue->max_payload_size)  {
			rc = -EINVAL;
			goto out;
		}

		mqueue->max_frame_payload_size = (mqueue->max_payload_size / mqueue->frame_stride) * mqueue->frame_size;

		if (mqueue->flags & MEDIA_QUEUE_FLAGS_DGRAM) {
			if (mqueue->batch_size > queue_available(&mqueue->queue) / 4)
				mqueue->batch_size = queue_available(&mqueue->queue) / 4;
		} else {
			if (mqueue->batch_size < mqueue->max_frame_payload_size) {
				rc = -EINVAL;
				goto out;
			}

			if (mqueue->batch_size > media_queue_remaining(mqueue) / 4)
				mqueue->batch_size = media_queue_remaining(mqueue) / 4;

			mqueue->batch_size = (mqueue->batch_size / mqueue->max_frame_payload_size) * mqueue->max_frame_payload_size;
		}
	}

	if ((flags & (MEDIA_QUEUE_FLAGS_BOUND_MASK | MEDIA_QUEUE_FLAGS_TALKER)) == (MEDIA_QUEUE_FLAGS_BOUND_MASK | MEDIA_QUEUE_FLAGS_TALKER)) {
		if ((mqueue->payload_offset + mqueue->max_payload_size) > NET_PAYLOAD_SIZE_MAX) {
			rc = -EINVAL;
			goto out;
		}
	}

out:
	return rc;
}


static int media_params_check(struct media_queue *mqueue, unsigned int flags)
{
	int rc = 0;

	if (flags & MEDIA_QUEUE_FLAGS_TALKER)
		rc = media_params_check_talker(mqueue, flags);
	else
		rc = media_params_check_listener(mqueue, flags);

	//if (rc < 0)
	//	pr_err("stream_id 0x%llx rc %d batch_size %u frame_size %u frame_stride %u max_payload_size %u max_frame_payload_size %u\n",
	//		*(unsigned long long *)mqueue->stream_id, rc, mqueue->batch_size, mqueue->frame_size, mqueue->frame_stride, mqueue->max_payload_size, mqueue->max_frame_payload_size);

	return rc;
}


/* Queue character device callbacks */
static long media_drv_net_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct media_queue *mqueue_orig, *mqueue = file->private_data;
	struct media_drv *drv = mqueue->drv;
	struct media_queue_net_params params;
	int rc = 0;

	//	pr_info("%s: file: %p cmd: %u, arg: %lu\n", __func__, file, cmd, arg);

	switch (cmd) {
	case MEDIA_IOC_NET_BIND:

		if ((mqueue->flags & MEDIA_QUEUE_FLAGS_BOUND_MASK) == MEDIA_QUEUE_FLAGS_BOUND_MASK) {
			rc = -EPIPE;
			break;
		}

		if (copy_from_user(&params, (void *)arg, sizeof(struct media_queue_net_params))) {
			rc = -EFAULT;
			break;
		}

		mutex_lock(&drv->list_lock);

		mqueue_orig = mqueue;

		rc = media_queue_bind_start(drv, &mqueue, params.stream_id, MEDIA_QUEUE_FLAGS_NET_BOUND);
		if (rc != 0) {
			goto unlock;
		}

		if (mqueue->flags & MEDIA_QUEUE_FLAGS_TALKER) {
			mqueue->payload_offset = params.talker.payload_offset;
			mqueue->offset = params.talker.ts_offset;
		}

		rc = media_params_check(mqueue, mqueue->flags | MEDIA_QUEUE_FLAGS_NET_BOUND);
		if (rc < 0)
			goto unlock;

		media_queue_bind_finish(drv, &file->private_data, mqueue, MEDIA_QUEUE_FLAGS_NET_BOUND);

		if (mqueue_orig != mqueue)
			media_queue_free(mqueue_orig);

		if (waitqueue_active(&mqueue->api_wait))
			wake_up(&mqueue->api_wait);

	unlock:
		mutex_unlock(&drv->list_lock);

		break;

	default:
		rc = -EINVAL;
		break;
	}

	return rc;
}

static unsigned int media_drv_net_poll(struct file *file, poll_table *poll)
{
	struct media_queue *mqueue = file->private_data;
	unsigned int mask = 0;

//	pr_info("%s: file: %p\n", __func__, file);

	poll_wait(file, &mqueue->net_wait, poll);

	if ((mqueue->flags & MEDIA_QUEUE_FLAGS_BOUND_MASK) != MEDIA_QUEUE_FLAGS_BOUND_MASK)
		goto exit;

	if (mqueue->flags & MEDIA_QUEUE_FLAGS_TALKER) {
		if (queue_pending(&mqueue->queue) >= mqueue->batch_size)
			mask |= POLLIN | POLLRDNORM;
	} else
		mask |= POLLERR;

exit:
//	pr_info("%s: %x\n", __func__, mask);

	return mask;
}

static int media_drv_net_release(struct inode *in, struct file *file)
{
	struct media_queue *mqueue = file->private_data;
	struct media_drv *drv = mqueue->drv;

//	pr_info("%s %p\n", __func__, mqueue);

	mutex_lock(&drv->list_lock);

	if ((mqueue->flags & ~MEDIA_QUEUE_FLAGS_NET_BOUND & MEDIA_QUEUE_FLAGS_BOUND_MASK) == 0) {
		media_queue_flush(mqueue);
		media_queue_free(mqueue);
	} else
		mqueue->flags &= ~MEDIA_QUEUE_FLAGS_NET_BOUND;

	file->private_data = NULL;

	mutex_unlock(&drv->list_lock);

	return 0;
}

static int media_drv_net_open(struct inode *in, struct file *file)
{
	struct media_drv *drv = container_of(in->i_cdev, struct media_drv, cdev_net);
	struct media_queue *mqueue;
	unsigned int flags = 0;
	int rc = 0;

	if (file->f_mode & FMODE_READ)
		flags |= MEDIA_QUEUE_FLAGS_TALKER;
	else if ((file->f_mode & FMODE_WRITE) == 0) {
		rc = -EINVAL;
		goto err;
	}

	mqueue = media_queue_alloc(drv, flags);
	if (!mqueue) {
		rc = -ENOMEM;
		goto err;
	}

	file->private_data = mqueue;

//	pr_info("%s %p\n", __func__, mqueue);

	return 0;

err:
	return rc;
}

struct event_info {
	unsigned int src_offset;
	unsigned int src_idx;
	unsigned int src_len;

	int dst_offset;
	unsigned int dst_idx;
	unsigned int dst_len;

	unsigned int dst_iov_idx;
	unsigned int total_dst_len;
	unsigned int total_read;
};

static void tx_read_event(struct media_rx_desc *desc, struct genavb_event *event_array, struct event_info *event_info, unsigned int dst_len, unsigned int src_len)
{
	struct genavb_event *event;

	//      pr_info("%p %u %u %u %u %u %u %u %u %u\n",
	//              desc, event_info->src_idx, event_info->dst_idx, event_info->src_offset, event_info->src_len, event_info->dst_offset, dst_len,
	//              event_array[event_info->src_idx].event_mask, event_array[event_info->src_idx].index, event_array[event_info->src_idx].ts);

	/* Search for an event in the correct range */
	for (event = &event_array[event_info->src_idx]; event_info->src_idx < event_info->src_len; event_info->src_idx++) {

		if (event->index < event_info->src_offset)
			continue;

		if (event->index >= (event_info->src_offset + src_len))
			break;

		if (!(event->event_mask & AVTP_SYNC))
			continue;

		/* Only use the first event for a given range */
		if (event_info->dst_idx < event_info->dst_len) {
			desc->avtp_ts[event_info->dst_idx].val = event->ts;
			desc->avtp_ts[event_info->dst_idx].offset = event_info->dst_offset;

			desc->ts_n++;
			event_info->dst_offset += dst_len;
			event_info->dst_idx++;
			event_info->src_idx++;
			break;
		}
	}

	event_info->src_offset += src_len;
}

static int media_drv_desc_alloc_array(struct avb_drv *avb, struct media_rx_desc **desc, unsigned int n)
{
	int i, rc;

	rc = pool_dma_alloc_array(&avb->buf_pool, (void **)desc, n);
	if (rc < 0)
		goto exit;

	for (i = 0; i < rc; i++)
		desc[i]->net.pool_type = POOL_TYPE_AVB;

exit:
	return rc;
}

#define DESC_MAX	32
#define EVENT_MAX	32

/**
 * media_drv_api_tx() - media talker stream write
 * @mqueue - media queue pointer
 * @tx - media transmit context
 *
 * Media application calls this function to write media data to avb stack (i.e, talker stream transmit).
 *
 */
static int media_drv_api_tx(struct media_queue *mqueue, struct media_queue_tx *tx)
{
	struct avb_drv *avb = container_of(mqueue->drv, struct avb_drv, media_drv);
	struct genavb_iovec data_iov[IOV_MAX + 1];
	struct media_rx_desc *desc[DESC_MAX + 1];
	struct genavb_event event[EVENT_MAX];
	unsigned int used_len, total_src_len, total_dst_len, total_len, len_now;
	unsigned int n, n_now;
	void *src, *dst = NULL;
	unsigned int src_len, src_len_now, dst_len = 0, dst_frame_len = 0, write_total = 0, frame_size, stride_overhead;
	unsigned int write = 0;
	struct event_info event_info;
	int i, iov_idx;
	int rc = 0;

	if (tx->data_iov_len > IOV_MAX) {
		rc = -EINVAL;
		goto exit;
	}

	if (copy_from_user(data_iov, tx->data_iov, tx->data_iov_len * sizeof(struct genavb_iovec))) {
		rc = -EFAULT;
		goto exit;
	}

	if (tx->event_len) {
		if (tx->event_len > EVENT_MAX) {
			rc = -EINVAL;
			goto exit;
		}

		if (copy_from_user(event, tx->event, tx->event_len * sizeof(struct genavb_event))) {
			rc = -EFAULT;
			goto exit;
		}
	}

	total_src_len = 0;
	for (i = 0; i < tx->data_iov_len; i++) {
		if (total_src_len + data_iov[i].iov_len < total_src_len) {
			rc = -EINVAL;
			goto exit;
		}

		total_src_len += data_iov[i].iov_len;
	}

	if (mqueue->partial_desc) {
		used_len = mqueue->max_frame_payload_size - mqueue->dst_len;
		dst_len = mqueue->dst_len;
	} else {
		used_len = 0;
		dst_len = 0;
	}

	n = queue_available(&mqueue->queue);
	if (!n) {
		rc = -EAGAIN;
		goto exit;
	}

	total_dst_len = n * mqueue->max_frame_payload_size - used_len;

	if (total_dst_len > total_src_len) {
		if (total_src_len > dst_len)
			n = ((total_src_len - dst_len) + mqueue->max_frame_payload_size - 1) / mqueue->max_frame_payload_size;
		else
			n = 0;

		if (used_len)
			n += 1;

		total_len = total_src_len;
	} else
		total_len = total_dst_len;

	if (mqueue->frame_size == mqueue->frame_stride) {
		frame_size = mqueue->max_frame_payload_size;
		stride_overhead = 0;
	} else {
		frame_size = mqueue->frame_size;
		stride_overhead = mqueue->frame_stride - mqueue->frame_size;
	}

	iov_idx = 0;
	src = data_iov[iov_idx].iov_base;
	src_len = data_iov[iov_idx].iov_len;

	event_info.src_len = tx->event_len;
	event_info.src_idx = 0;
	event_info.src_offset = 0;

	event_info.dst_idx = 0;
	event_info.dst_len = 0;
	event_info.dst_offset = 0;

	queue_enqueue_init(&mqueue->queue, &write);

	while (n) {
		n_now = n;
		if (n_now > DESC_MAX)
			n_now = DESC_MAX;

		i = 0;
		if (mqueue->partial_desc) {
			desc[0] = mqueue->partial_desc;
			dst = mqueue->dst;
			dst_len = mqueue->dst_len;
			dst_frame_len = mqueue->dst_frame_len;
			mqueue->partial_desc = NULL;

			rc = media_drv_desc_alloc_array(avb, &desc[1], n_now - 1);
			if (rc < 0)
				break;

			n_now = rc + 1;

			len_now = (n_now - 1) * mqueue->max_frame_payload_size + dst_len;

			event_info.dst_idx = mqueue->ts_dst_idx;
			event_info.dst_offset = mqueue->ts_dst_offset;
			event_info.dst_len = mqueue->ts_dst_len;

		} else {
			rc = media_drv_desc_alloc_array(avb, &desc[0], n_now);
			if (rc <= 0)
				break;

			n_now = rc;

			dst = (void *)desc[0] + mqueue->payload_offset + stride_overhead;
			dst_len = mqueue->max_frame_payload_size;
			dst_frame_len = frame_size;

			len_now = n_now * mqueue->max_frame_payload_size;

			event_info.dst_idx = 0;
			event_info.dst_len = MEDIA_TS_PER_PACKET;
			event_info.dst_offset = 0;

			desc[0]->ts_n = 0;
		}

		if (len_now > total_len)
			len_now = total_len;

		while (len_now) {

			/* We may not copy the entire source so adjust the source length */
			src_len_now = src_len;
			if (src_len_now > len_now)
				src_len_now = len_now;

			src_len -= src_len_now;

			/* Copy full frames, knowing packets are stride aligned */
			while (src_len_now >= dst_frame_len) {

				if (copy_from_user(dst, src, dst_frame_len)) {
					rc = -EFAULT;
					goto fault;
				}

				dst += dst_frame_len + stride_overhead;
				dst_len -= dst_frame_len;
				len_now -= dst_frame_len;
				total_len -= dst_frame_len;

				src += dst_frame_len;
				src_len_now -= dst_frame_len;
				write_total += dst_frame_len;

				dst_frame_len = frame_size;

				/* End-of-Frame events shall appear only at the end of an iovec, so there is no
				 * need to check for one here:
				 * . If the iovec ends on a max_payload_size boundary, the packet will be enqueued
				 *   by the "if (!dst_len)" check below anyway (whether there is an EOF event or not).
				 * . If the iovec doesn't end on a max_payload_size boundary, the EOF event will be handled
				 *   after copying the last bytes of iovec below, outside of the inner loop.
				 */
				tx_read_event(desc[i], event, &event_info, dst_frame_len + stride_overhead, dst_frame_len);

				if (!dst_len) {
					desc[i]->net.len = mqueue->max_payload_size;
					desc[i]->net.l2_offset = mqueue->payload_offset;
					desc[i]->net.flags = 0;
					/*This is the last packet , mark it as end of frame*/
					if ((iov_idx >= tx->data_iov_len - 1) && ((tx->event_len &&  (event[0].event_mask & AVTP_FRAME_END))) && !src_len)
						desc[i]->net.flags |= NET_TX_FLAGS_END_FRAME;

					queue_enqueue_next(&mqueue->queue, &write, (unsigned long)desc[i]);

					i++;

					if (i < n_now) {
						dst = (void *)desc[i] + mqueue->payload_offset + stride_overhead;
						dst_len = mqueue->max_frame_payload_size;
						desc[i]->ts_n = 0;

						event_info.dst_idx = 0;
						event_info.dst_len = MEDIA_TS_PER_PACKET;
						event_info.dst_offset = 0;
					}
				}
			}

			/* Copy last bytes of iovec */
			if (src_len_now) {

				/* Copy partial frame */
				if (copy_from_user(dst, src, src_len_now)) {
					rc = -EFAULT;
					goto fault;
				}

				dst += src_len_now;
				dst_len -= src_len_now;
				len_now -= src_len_now;
				total_len -= src_len_now;

				write_total += src_len_now;

				dst_frame_len -= src_len_now;

				tx_read_event(desc[i], event, &event_info, src_len_now, src_len_now);
			}

			if (!src_len) {
				iov_idx++;

				/* This is safe since we always leave a safeguard entry at the end */
				src = data_iov[iov_idx].iov_base;
				src_len = data_iov[iov_idx].iov_len;
			}
		}

		n -= n_now;
	}

	/* Keep track of the last partial descriptor */
	if (dst_len && (dst_len != mqueue->max_frame_payload_size)) {
		if ((mqueue->flags & MEDIA_QUEUE_FLAGS_DGRAM) || (tx->event_len && ((event[0].event_mask & AVTP_FLUSH) || (event[0].event_mask & AVTP_FRAME_END)))) {
			used_len = mqueue->max_frame_payload_size - dst_len;
			desc[i]->net.len = used_len - (used_len % mqueue->frame_size); //partial desc size rounded down to nearest multiple of frame size
			if (stride_overhead)
				desc[i]->net.len += (desc[i]->net.len / mqueue->frame_size) * stride_overhead;
			desc[i]->net.l2_offset = mqueue->payload_offset;
			desc[i]->net.flags = NET_TX_FLAGS_PARTIAL;
			if (event[0].event_mask & AVTP_FRAME_END)
				desc[i]->net.flags |= NET_TX_FLAGS_END_FRAME;

			queue_enqueue_next(&mqueue->queue, &write, (unsigned long)desc[i]);
		} else {
			mqueue->partial_desc = desc[i];
			mqueue->dst_len = dst_len;
			mqueue->dst_frame_len = dst_frame_len;
			mqueue->dst = dst;

			mqueue->ts_dst_idx = event_info.dst_idx;
			mqueue->ts_dst_offset = event_info.dst_offset;
			mqueue->ts_dst_len = event_info.dst_len;
		}
	}
	queue_enqueue_done(&mqueue->queue, write);

	if (write_total) {
		rc = write_total;

		if (waitqueue_active(&mqueue->net_wait))
			wake_up(&mqueue->net_wait);
	}


exit:
	return rc;

fault:
	/* Don't revert packets already queueed, but need to free all others */

	pool_dma_free_array(&avb->buf_pool, (void **)&desc[i], n_now - i);

	if (write_total) {
		queue_enqueue_done(&mqueue->queue, write);
		rc = write_total;

		if (waitqueue_active(&mqueue->net_wait))
			wake_up(&mqueue->net_wait);
	}

	return rc;
}


static void rx_write_event(struct genavb_event *avb_event, struct media_desc *desc, struct event_info *event_info)
{
	struct genavb_event *event;
	struct genavb_event event_local;
	unsigned int len_now;
	unsigned int i, rc;

	/* Determine number of events to copy */
	len_now = min(event_info->dst_len, event_info->src_len);

	event = &avb_event[event_info->dst_idx];

	for (i = event_info->src_idx; i < event_info->src_idx + len_now; i++) {
		event_local.index = event_info->dst_offset + (int)desc->avtp_ts[i].offset;

		event_local.event_mask = MEDIA_DESC_FLAGS_TO_AVTP(desc->avtp_ts[i].flags);
		if (!(event_local.event_mask & AVTP_TIMESTAMP_INVALID))
			event_local.ts = desc->avtp_ts[i].val;
		else
			event_local.ts = 0;

		/* Copy packet-level flags for the first event of the packet */
		if (i == 0) {
			event_local.event_mask |= desc->flags;
			event_local.event_data = desc->bytes_lost;
		} else
			event_local.event_data = 0;

		rc = copy_to_user(event, &event_local, sizeof(struct genavb_event));
		if (rc)
			break;
		event++;
	}


	/* Update event source */
	event_info->src_len -= len_now;
	event_info->src_idx += len_now;

	event_info->dst_idx += len_now;
	event_info->dst_len -= len_now;

	/* Update global counters */
	event_info->total_dst_len -= len_now;
	event_info->total_read += len_now;
}

static void media_drv_desc_free(struct avb_drv *avb, void *desc_array[], unsigned int *count, void *desc)
{
	desc_array[*count] = desc;
	(*count)++;
	if (unlikely(*(count) >= DESC_MAX)) {
		pool_dma_free_array(&avb->buf_pool, desc_array, DESC_MAX);
		*count = 0;
	}
}

/**
 * media_drv_api_rx() - media listener stream read
 * @mqueue - media queue pointer
 * @rx - media receive context
 *
 * Media application calls this function to read media data from avb stack (i.e, listener stream receive).
 *
 */
static int media_drv_api_rx(struct media_queue *mqueue, struct media_queue_rx *rx)
{
	struct avb_drv *avb = container_of(mqueue->drv, struct avb_drv, media_drv);
	struct genavb_iovec data_iov[IOV_MAX + 1];
	struct genavb_iovec event_iov[IOV_MAX + 1];
	int total_dst_len, dst_len, total_read, len_now;  // Those variables are in number of bytes
	struct event_info event_info;
	int iov_idx; /* Iovec index */
	struct genavb_event *event = NULL;
	void *dst, *src;
	int src_len, src_frame_len;
	void *desc_array[DESC_MAX];
	unsigned int desc_count = 0;
	struct media_desc *desc;
	int i, skip_event = 0;
	int finished, rc = 0;
	const int stride_overhead = mqueue->frame_stride - mqueue->frame_size;

	total_read = 0;
	event_info.total_read = 0;

	if (rx->data_iov_len > IOV_MAX) {
		rc = -EINVAL;
		goto early_exit;
	}

	if (copy_from_user(data_iov, rx->data_iov, rx->data_iov_len * sizeof(struct genavb_iovec))) {
		rc = -EFAULT;
		goto early_exit;
	}

	if (rx->event_iov_len == 0)
		skip_event = 1;
	else {
		if (rx->event_iov_len > IOV_MAX) {
			rc = -EINVAL;
			goto early_exit;
		}

		if (copy_from_user(event_iov, rx->event_iov, rx->event_iov_len * sizeof(struct genavb_iovec))) {
			rc = -EFAULT;
			goto early_exit;
		}
	}


	total_dst_len = 0;
	for (i = 0; i < rx->data_iov_len; i++) {
		if (total_dst_len + data_iov[i].iov_len < total_dst_len) {
			rc = -EINVAL;
			goto early_exit;
		}

		total_dst_len += data_iov[i].iov_len;
	}

	if (total_dst_len == 0) {
		goto early_exit;
	}


	event_info.total_dst_len = 0;
	if (!skip_event) {
		for (i = 0; i < rx->event_iov_len; i++) {
			if ((event_info.total_dst_len + event_iov[i].iov_len) < event_info.total_dst_len) {
				rc = -EINVAL;
				goto early_exit;
			}

			event_info.total_dst_len += event_iov[i].iov_len;
		}

		if (event_info.total_dst_len == 0) {
			goto early_exit;
		}
	}


	/* There is space available to copy data to */

	/* Keep a partial desc across ioctl calls */
	if (likely(mqueue->partial_desc)) {
		desc = mqueue->partial_desc;

		src = mqueue->src;
		src_len = mqueue->src_len;
		src_frame_len = mqueue->src_frame_len;

		event_info.dst_offset = src - ((void *)desc + desc->l2_offset) - stride_overhead;

		if (stride_overhead)
			event_info.dst_offset -= (event_info.dst_offset / mqueue->frame_stride) * stride_overhead;

		event_info.dst_offset = -event_info.dst_offset;
	} else {
		desc = (struct media_desc *)queue_dequeue(&mqueue->queue);
		if ((unsigned long)desc == (unsigned long)-1) {
			goto early_exit;
		}

		src_len = desc->len;
		src = (void *)desc + desc->l2_offset;

		event_info.dst_offset = 0;

		if (stride_overhead) {
			/* Assume hole is at start of frame (for 61883-4, skip first timestamp (SP Header)) */
			src += stride_overhead;
			src_len -= (src_len / mqueue->frame_stride) * stride_overhead;

			src_frame_len = mqueue->frame_size;
		} else
			src_frame_len = 0;
	}

	/* Init data dest */
	iov_idx = 0;
	dst_len = data_iov[iov_idx].iov_len;
	dst = data_iov[iov_idx].iov_base;


	event_info.dst_len = 0;
	if (!skip_event) {
		/* Init event source */
		event_info.src_idx = mqueue->event_src_idx;
		event_info.src_len = desc->n_ts - mqueue->event_src_idx;

		/* Init event dest */
		event_info.dst_idx = 0;
		event_info.dst_iov_idx = 0;
		event_info.dst_len = event_iov[event_info.dst_iov_idx].iov_len;
		event = event_iov[event_info.dst_iov_idx].iov_base;

	} else {
		event_info.src_idx = 0;
		event_info.src_len = 0;
	}

	finished = 0;
	while (!finished) {
		/* Make sure data length and event length match (don't copy more of one than the other) */
		if (!skip_event) {
			if (total_dst_len < src_len) {
				/* Not enough space to copy the remaining payload, need to determine which event will be the last to be copied. */
				i = 0;
				while ((i < event_info.src_len) && ((total_read + total_dst_len) > (event_info.dst_offset + desc->avtp_ts[event_info.src_idx + i].offset)))
					i++;

				/* We cannot simply set event length, as we may still need several passes on the event_iov. */
				event_info.src_len = i;
				event_info.dst_len = min(event_info.src_len, event_info.dst_len);
				event_info.total_dst_len = i;
			}

			if (event_info.total_dst_len < event_info.src_len) {
				/* Not enough space to copy all the events in the current desc, need to determine number of data bytes to be copied.
				As above, we cannot simply set data length. */
				src_len = event_info.dst_offset + desc->avtp_ts[event_info.src_idx + event_info.total_dst_len].offset - total_read;
				dst_len = min(src_len, dst_len);
				total_dst_len = src_len;
			}
		}

		/* Determine size of data copy */
		len_now = min(dst_len, src_len);

		/* Do data copy */
		if (len_now) {
			int src_len_now;

			if (stride_overhead)
				src_len_now = min(len_now, src_frame_len);
			else
				src_len_now = len_now;

			rc = copy_to_user(dst, src, src_len_now);
			if (rc)
				goto exit;

			/* Update data source and destination */
			src_len -= src_len_now;
			src += src_len_now;
			dst += src_len_now;

			/* Update global counters */
			total_read += src_len_now;
			total_dst_len -= src_len_now;

			if (stride_overhead) {
				src_frame_len -= src_len_now;
				if (src_frame_len == 0) {
					src += stride_overhead;
					src_frame_len = mqueue->frame_size;
				}

				src_len_now = len_now - src_len_now;
				while (src_len_now >= mqueue->frame_size) {
					rc = copy_to_user(dst, src, mqueue->frame_size);
					if (rc)
						goto exit;

					/* Update data source and destination */
					src_len -= mqueue->frame_size;
					src += mqueue->frame_stride;
					dst += mqueue->frame_size;

					/* Update global counters */
					total_read += mqueue->frame_size;
					total_dst_len -= mqueue->frame_size;

					src_len_now -= mqueue->frame_size;
				}

				if (src_len_now) {
					rc = copy_to_user(dst, src, src_len_now);
					if (rc)
						goto exit;

					/* Update data source and destination */
					src_len -= src_len_now;
					src_frame_len -= src_len_now;
					src += src_len_now;
					dst += src_len_now;

					/* Update global counters */
					total_read += src_len_now;
					total_dst_len -= src_len_now;
				}
			}
		}

		/* Do event copy */
		if (!skip_event)
			rx_write_event(event, desc, &event_info);

		/* Quick exit. This is just an optimization (to avoid going through the update dest blocks below at the end of the loop) and could be removed. */
		if ((total_dst_len == 0)
			|| ((event_info.total_dst_len == 0) && !skip_event)
			|| (desc->n_ts && (desc->avtp_ts[desc->n_ts - 1].flags & AVTP_FLAGS_TO_MEDIA_DESC(AVTP_END_OF_FRAME))))
			goto exit;

		/* Update data_iov/data dest */
		dst_len -= len_now;
		if (dst_len == 0) {
			iov_idx++;
			if (iov_idx < rx->data_iov_len) {
				dst_len = data_iov[iov_idx].iov_len;
				dst = data_iov[iov_idx].iov_base;
			} else {
				goto exit;
			}
		}

		/* Update event dest */
		if (!skip_event) {
			if (event_info.dst_len == 0) {
				event_info.dst_iov_idx++;
				if (event_info.dst_iov_idx < rx->event_iov_len) {
					event_info.dst_idx = 0;
					event_info.dst_len = event_iov[event_info.dst_iov_idx].iov_len;
					event = event_iov[event_info.dst_iov_idx].iov_base;
				} else {
					goto exit;
				}
			}
		}

		/* Update desc: if we got to there we still have space available in the iovecs, so we can try dequeuing another desc */
		if ((src_len == 0) && ((event_info.src_len == 0) || skip_event)) {
			/* The End-of-Frame marker is assumed always to be at the end of a packet, or at least always to be the last event in a packet.
			   Note: If not in DATAGRAM mode this test should never trigger, since the EOF should be caught by the quick exit test above. */
			if (desc->n_ts && (desc->avtp_ts[desc->n_ts - 1].flags & AVTP_FLAGS_TO_MEDIA_DESC(AVTP_END_OF_FRAME)))
				atomic_dec(&mqueue->eofs);

			media_drv_desc_free(avb, desc_array, &desc_count, desc);

			if (mqueue->flags & MEDIA_QUEUE_FLAGS_DGRAM) {
				/*  DATAGRAM implies that we read one packet at a time */
				desc = (struct media_desc *) -1;
				goto exit;
			}

			desc = (struct media_desc *) queue_dequeue(&mqueue->queue);
			if ((unsigned long)desc == (unsigned long)-1) {
				goto exit;
			}

			src_len = desc->len;
			src = (void *)desc + desc->l2_offset;

			/* Init data source */
			if (stride_overhead) {
				/* Assume hole is at start of frame (for 61883-4, skip first timestamp (SP Header)) */
				src += stride_overhead;
				src_len -= (src_len / mqueue->frame_stride) * stride_overhead;
				src_frame_len = mqueue->frame_size;
			}

			event_info.dst_offset = total_read;

			/* Init event source */
			event_info.src_idx = 0;
			event_info.src_len = desc->n_ts;

			/* If the new descriptor mentions an error, exit now so the next batch will contain the error flags on the first genavb_event */
			if (desc->flags)
				goto exit;
		}
	}

exit:
	/* Update partial desc if needed */
	if (src_len || (event_info.src_len && !skip_event)) {
		u16 new_offset = src - (void *)desc;

		/* For datagram we always stop rx process at packets boundaries, i.e.
		not partial descriptor are maintained */
		if (mqueue->flags & MEDIA_QUEUE_FLAGS_DGRAM) {
			media_drv_desc_free(avb, desc_array, &desc_count, desc);

			mqueue->partial_desc = NULL;
			mqueue->event_src_idx = 0;
			goto early_exit;
		}

		mqueue->partial_desc = desc;
		mqueue->src_len = desc->len - (new_offset - desc->l2_offset);

		if (stride_overhead)
			mqueue->src_len -= (mqueue->src_len / mqueue->frame_stride) * stride_overhead;

		mqueue->src = src;
		mqueue->src_frame_len = src_frame_len;

		mqueue->event_src_idx = event_info.src_idx;
	} else {
		if ((unsigned long)desc != (unsigned long)-1) {
			/* The End-of-Frame marker is assumed always to be at the end of a packet, or at least always to be the last event in a packet. */
			if (desc->n_ts && (desc->avtp_ts[desc->n_ts - 1].flags & AVTP_FLAGS_TO_MEDIA_DESC(AVTP_END_OF_FRAME)))
				atomic_dec(&mqueue->eofs);

			media_drv_desc_free(avb, desc_array, &desc_count, desc);
		}

		mqueue->partial_desc = NULL;

		mqueue->event_src_idx = 0;
	}

early_exit:
	if (desc_count)
		pool_dma_free_array(&avb->buf_pool, desc_array, desc_count);

	rx->data_read = total_read;
	atomic_sub(total_read, &mqueue->available);

	rx->event_read = event_info.total_read;

	return rc;
}


static long media_drv_api_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct media_queue *mqueue_orig, *mqueue = file->private_data;
	struct avb_drv *avb = container_of(mqueue->drv, struct avb_drv, media_drv);
	struct media_drv *drv = mqueue->drv;
	struct media_queue_api_params params;
	struct media_queue_rx rx;
	struct media_queue_tx tx;
	struct logical_port *port;
	int rc = 0;

	//	pr_info("%s: file: %p cmd: %u, arg: %lu\n", __func__, file, cmd, arg);

	switch (cmd) {
	case MEDIA_IOC_API_BIND:

		if ((mqueue->flags & MEDIA_QUEUE_FLAGS_BOUND_MASK) == MEDIA_QUEUE_FLAGS_BOUND_MASK) {
			rc = -EPIPE;
			break;
		}

		if (copy_from_user(&params, (void *)arg, sizeof(struct media_queue_api_params))) {
			rc = -EFAULT;
			break;
		}

		mutex_lock(&drv->list_lock);

		mqueue_orig = mqueue;

		rc = media_queue_bind_start(drv, &mqueue, params.stream_id, MEDIA_QUEUE_FLAGS_API_BOUND);
		if (rc != 0) {
			goto unlock;
		}

		port = logical_port_get(avb, params.port);
		if (!port) {
			rc = -EINVAL;
			goto unlock;
		}

		mqueue->port = port->eth->port;
		mqueue->batch_size = params.batch_size;
		mqueue->frame_stride = params.frame_stride;
		mqueue->frame_size = params.frame_size;
		mqueue->max_payload_size = params.max_payload_size;
		if (params.flags & AVTP_DGRAM)
			mqueue->flags |= MEDIA_QUEUE_FLAGS_DGRAM;

		rc = media_params_check(mqueue, mqueue->flags | MEDIA_QUEUE_FLAGS_API_BOUND);
		if (rc < 0)
			goto unlock;

		/* Writeback final batch size */
		rc = put_user(mqueue->batch_size, &(((struct media_queue_api_params *)arg)->batch_size));
		if (rc)
			goto unlock;

		media_queue_bind_finish(drv, &file->private_data, mqueue, MEDIA_QUEUE_FLAGS_API_BOUND);

		if (mqueue_orig != mqueue)
			media_queue_free(mqueue_orig);

		if (waitqueue_active(&mqueue->api_wait))
			wake_up(&mqueue->api_wait);

	unlock:
		mutex_unlock(&drv->list_lock);

		break;

	case MEDIA_IOC_RX:
		if (mqueue->flags & MEDIA_QUEUE_FLAGS_TALKER) {
			rc = -EPERM;
			break;
		}

		if ((mqueue->flags & MEDIA_QUEUE_FLAGS_BOUND_MASK) != MEDIA_QUEUE_FLAGS_BOUND_MASK) {
			rc = -EPIPE;
			break;
		}

		if (copy_from_user(&rx, (void *)arg, sizeof(struct media_queue_rx))) {
			rc = -EFAULT;
			break;
		}

		//TODO handle file->f_flags & O_NONBLOCK
		rc = media_drv_api_rx(mqueue, &rx);

		if (copy_to_user((void *)arg, &rx, sizeof(struct media_queue_rx))) {
			rc = -EFAULT;
			break;
		}

		break;

	case MEDIA_IOC_TX:
		if (!(mqueue->flags & MEDIA_QUEUE_FLAGS_TALKER)) {
			rc = -EPERM;
			break;
		}

		if ((mqueue->flags & MEDIA_QUEUE_FLAGS_BOUND_MASK) != MEDIA_QUEUE_FLAGS_BOUND_MASK) {
			rc = -EPIPE;
			break;
		}

		if (copy_from_user(&tx, (void *)arg, sizeof(struct media_queue_tx))) {
			rc = -EFAULT;
			break;
		}

		rc = media_drv_api_tx(mqueue, &tx);

		break;

	default:
		rc = -EINVAL;
		break;
	}

	return rc;
}


static unsigned int media_drv_api_poll(struct file *file, poll_table *poll)
{
	struct media_queue *mqueue = file->private_data;
	unsigned int mask = 0;

//	pr_info("%s: file: %p\n", __func__, file);

	poll_wait(file, &mqueue->api_wait, poll);

	if ((mqueue->flags & MEDIA_QUEUE_FLAGS_BOUND_MASK) != MEDIA_QUEUE_FLAGS_BOUND_MASK)
		goto exit;

	if (mqueue->flags & MEDIA_QUEUE_FLAGS_TALKER) {
		if ((media_queue_remaining(mqueue) >= mqueue->batch_size) || queue_empty(&mqueue->queue))
			mask |= POLLOUT | POLLWRNORM;
	} else {
		if (need_to_wake_up_listener_queue(mqueue))
			mask |= POLLIN | POLLRDNORM;
	}

exit:
//	pr_info("%s: %x\n", __func__, mask);

	return mask;
}

static int media_drv_api_open(struct inode *in, struct file *file)
{
	struct media_drv *drv = container_of(in->i_cdev, struct media_drv, cdev_api);
	struct media_queue *mqueue;
	unsigned int flags = 0;
	int rc = 0;

	if (file->f_mode & FMODE_WRITE)
		flags |= MEDIA_QUEUE_FLAGS_TALKER;
	else if ((file->f_mode & FMODE_READ) == 0) {
		rc = -EINVAL;
		goto err;
	}

	mqueue = media_queue_alloc(drv, flags);
	if (!mqueue) {
		rc = -ENOMEM;
		goto err;
	}

	file->private_data = mqueue;

//	pr_info("%s %p\n", __func__, mqueue);

	return 0;

err:
	return rc;
}

static int media_drv_api_release(struct inode *in, struct file *file)
{
	struct media_queue *mqueue = file->private_data;
	struct media_drv *drv = mqueue->drv;

//	pr_info("%s %p\n", __func__, mqueue);

	mutex_lock(&drv->list_lock);

	if ((mqueue->flags & ~MEDIA_QUEUE_FLAGS_API_BOUND & MEDIA_QUEUE_FLAGS_BOUND_MASK) == 0) {
		media_queue_flush(mqueue);
		media_queue_free(mqueue);
	} else {
		mqueue->flags &= ~MEDIA_QUEUE_FLAGS_API_BOUND;
		if ((mqueue->flags & MEDIA_QUEUE_FLAGS_TALKER) == 0)	// Try and quickly free up memory used by pending buffers, in case of an application crash for example.
			media_queue_flush(mqueue);
	}

	file->private_data = NULL;

	mutex_unlock(&drv->list_lock);

	return 0;
}


static const struct file_operations media_drv_net_fops = {
	.owner = THIS_MODULE,
	.open = media_drv_net_open,
	.release = media_drv_net_release,
	.write = media_drv_net_write,
	.read = media_drv_net_read,
	.unlocked_ioctl = media_drv_net_ioctl,
	.poll = media_drv_net_poll,
};

static const struct file_operations media_drv_api_fops = {
	.owner = THIS_MODULE,
	.open = media_drv_api_open,
	.release = media_drv_api_release,
	.unlocked_ioctl = media_drv_api_ioctl,
	.poll = media_drv_api_poll,
};


int media_drv_init(struct media_drv *drv)
{
	int rc;

	pr_info("%s: %p\n", __func__, drv);

	INIT_LIST_HEAD(&drv->media_queues);

	mutex_init(&drv->list_lock);

	rc = alloc_chrdev_region(&drv->devno, MEDIA_DRV_MINOR, MEDIA_DRV_MINOR_COUNT, MEDIA_DRV_NAME);
	if (rc < 0) {
		pr_err("%s: alloc_chrdev_region() failed\n", __func__);
		goto err_alloc_chrdev;
	}

	cdev_init(&drv->cdev_net, &media_drv_net_fops);
	drv->cdev_net.owner = THIS_MODULE;

	rc = cdev_add(&drv->cdev_net, drv->devno, 1);
	if (rc < 0) {
		pr_err("%s: cdev_add() failed\n", __func__);
		goto err_cdev_net_add;
	}

	cdev_init(&drv->cdev_api, &media_drv_api_fops);
	drv->cdev_api.owner = THIS_MODULE;

	rc = cdev_add(&drv->cdev_api, MKDEV(MAJOR(drv->devno), MEDIA_DRV_API_MINOR), 1);
	if (rc < 0) {
		pr_err("%s: cdev_add() failed\n", __func__);
		goto err_cdev_api_add;
	}


	return 0;

err_cdev_api_add:
	cdev_del(&drv->cdev_net);
err_cdev_net_add:
	unregister_chrdev_region(drv->devno, MEDIA_DRV_MINOR_COUNT);

err_alloc_chrdev:
	return rc;
}

void media_drv_exit(struct media_drv *drv)
{
	pr_info("%s: %p\n", __func__, drv);

	cdev_del(&drv->cdev_net);
	cdev_del(&drv->cdev_api);
	unregister_chrdev_region(drv->devno, MEDIA_DRV_MINOR_COUNT);
}
