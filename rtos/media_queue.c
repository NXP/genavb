/*
 * Copyright 2018-2019, 2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief RTOS specific media interface implementation
 @details
*/
#include "media_queue.h"

#include "rtos_abstraction_layer.h"

#include "genavb/media.h"
#include "genavb/types.h"
#include "genavb/avtp.h"

#include "common/log.h"
#include "common/types.h"

#include "os/net.h"
#include "os/clock.h"

#include "net.h"
#include "net_port.h"
#include "net_logical_port.h"

#define MEDIA_QUEUE_FLAGS_TALKER		(1 << 0)
#define MEDIA_QUEUE_FLAGS_NET_BOUND		(1 << 1)
#define MEDIA_QUEUE_FLAGS_API_BOUND		(1 << 2)
#define MEDIA_QUEUE_FLAGS_BOUND_MASK		(MEDIA_QUEUE_FLAGS_NET_BOUND | MEDIA_QUEUE_FLAGS_API_BOUND)

#define FRAME_STRIDE_MAX	1024
#define PAYLOAD_OFFSET_MIN	NET_DATA_OFFSET
#define PAYLOAD_OFFSET_MAX	(2 * NET_DATA_OFFSET)

#define MEDIA_QUEUE_MAX	12

struct media_queue_table {
	struct media_queue *mqueue;
	uint8_t	stream_id[8];
	unsigned int flags;
	unsigned int payload_offset;
	unsigned int offset;
};

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

static rtos_mutex_t table_mutex;

static struct media_queue_table mqueue_table[MEDIA_QUEUE_MAX];

static inline unsigned int media_queue_remaining(struct media_queue *mqueue)
{
	unsigned int n = queue_available(&mqueue->queue);

	if (n)
		return mqueue->max_frame_payload_size * (n - 1);
	else
		return 0;
}

static inline unsigned int media_queue_avail(struct media_queue *mqueue)
{
	return rtos_atomic_read(&mqueue->available);
}

static inline unsigned int media_queue_eofs(struct media_queue *mqueue)
{
	return rtos_atomic_read(&mqueue->eofs);
}

static inline int need_to_wake_up_listener_queue(struct media_queue *mqueue)
{
	return (media_queue_avail(mqueue) >= mqueue->batch_size) || queue_full(&mqueue->queue) || media_queue_eofs(mqueue);
}

static void mqueue_flush_partial(struct media_queue *mqueue)
{
	if (mqueue->partial_desc) {
		if (mqueue->flags & MEDIA_QUEUE_FLAGS_TALKER) {
			((struct media_desc *)mqueue->partial_desc)->len = (mqueue->dst - (uint8_t *)mqueue->partial_desc) - mqueue->payload_offset;
			((struct media_desc *)mqueue->partial_desc)->l2_offset = mqueue->payload_offset;
		}

		net_rx_free(mqueue->partial_desc);

		mqueue->partial_desc = NULL;
	}
}

static void mqueue_flush(struct media_queue *mqueue)
{
	queue_flush(&mqueue->queue, NULL);

	mqueue_flush_partial(mqueue);

	rtos_atomic_set(&mqueue->available, 0);
	rtos_atomic_set(&mqueue->eofs, 0);
}

static struct media_queue_table *mqueue_table_find(uint8_t *stream_id, unsigned int flags)
{
	int i;

	for (i = 0; i < MEDIA_QUEUE_MAX; i++) {
		struct media_queue_table *entry = &mqueue_table[i];

		if ((entry->flags & MEDIA_QUEUE_FLAGS_BOUND_MASK) &&
			((entry->flags & MEDIA_QUEUE_FLAGS_TALKER) == flags) &&
			!memcmp(entry->stream_id, stream_id, 8))
			return entry;
	}

	return NULL;
}

static struct media_queue *mqueue_get(int id, unsigned int flags)
{
	struct media_queue_table *entry;

	if (id >= MEDIA_QUEUE_MAX)
		goto err;

	entry = &mqueue_table[id];

	if ((entry->flags & MEDIA_QUEUE_FLAGS_TALKER) != flags)
		goto err;

	if ((entry->flags & MEDIA_QUEUE_FLAGS_BOUND_MASK) != MEDIA_QUEUE_FLAGS_BOUND_MASK)
		goto err;

	return entry->mqueue;

err:
	return NULL;
}

static struct media_queue_table *mqueue_table_add(uint8_t *stream_id, unsigned int flags)
{
	int i;

	for (i = 0; i < MEDIA_QUEUE_MAX; i++) {
		struct media_queue_table *entry = &mqueue_table[i];

		if (!(entry->flags & MEDIA_QUEUE_FLAGS_BOUND_MASK)) {
			entry->flags |= flags;

			memcpy(entry->stream_id, stream_id, 8);

			return entry;
		}
	}

	return NULL;
}

static int mqueue_table_bind(struct media_queue_table *entry, unsigned int flags)
{
	if (entry->flags & flags)
		goto err;

	entry->flags |= flags;

	return 0;

err:
	return -1;
}

static void mqueue_table_free(struct media_queue_table *entry)
{
	entry->flags = 0;
}

static void mqueue_table_unbind(struct media_queue_table *entry, unsigned int flags)
{
	entry->flags &= ~flags;

	if (!(entry->flags & MEDIA_QUEUE_FLAGS_BOUND_MASK))
		mqueue_table_free(entry);

	return;
}

static int mqueue_table_id(struct media_queue_table *entry)
{
	return (entry - &mqueue_table[0]);
}


static int media_params_check(struct media_queue_table *entry, unsigned int flags)
{
	struct media_queue *mqueue = entry->mqueue;
	int rc = 0;

	if (flags & MEDIA_QUEUE_FLAGS_NET_BOUND) {

		if (flags & MEDIA_QUEUE_FLAGS_TALKER) {
			if ((entry->payload_offset < PAYLOAD_OFFSET_MIN) || (entry->payload_offset > PAYLOAD_OFFSET_MAX)) {
				rc = -1;
				goto out;
			}
		}
	}

	if (flags & MEDIA_QUEUE_FLAGS_API_BOUND) {

		if (!logical_port_valid(mqueue->port)) {
			rc = -1;
			goto out;
		}

		if (!mqueue->frame_stride || !mqueue->frame_size || !mqueue->max_payload_size) {
			rc = -1;
			goto out;
		}

		if (mqueue->frame_stride < mqueue->frame_size) {
			rc = -1;
			goto out;
		}

		if (mqueue->frame_stride > FRAME_STRIDE_MAX) {
			rc = -1;
			goto out;
		}

		if (flags & MEDIA_QUEUE_FLAGS_TALKER) {
			if ((PAYLOAD_OFFSET_MIN + mqueue->max_payload_size) > NET_PAYLOAD_SIZE_MAX) {
				rc = -1;
				goto out;
			}
		}

		if (mqueue->batch_size < mqueue->frame_size) {
			rc = -1;
			goto out;
		}

		if (mqueue->frame_stride > mqueue->max_payload_size) {
			rc = -1;
			goto out;
		}

		if ((mqueue->max_payload_size / mqueue->frame_stride) * mqueue->frame_stride != mqueue->max_payload_size)  {
			rc = -1;
			goto out;
		}

		mqueue->max_frame_payload_size = (mqueue->max_payload_size / mqueue->frame_stride) * mqueue->frame_size;

		if (mqueue->batch_size < mqueue->max_frame_payload_size) {
			rc = -1;
			goto out;
		}

		if (mqueue->batch_size > media_queue_remaining(mqueue) / 4)
			mqueue->batch_size = media_queue_remaining(mqueue) / 4;

		mqueue->batch_size = (mqueue->batch_size / mqueue->max_frame_payload_size) * mqueue->max_frame_payload_size;
	}

	if ((flags & (MEDIA_QUEUE_FLAGS_BOUND_MASK | MEDIA_QUEUE_FLAGS_TALKER)) == (MEDIA_QUEUE_FLAGS_BOUND_MASK | MEDIA_QUEUE_FLAGS_TALKER)) {
		if ((entry->payload_offset + mqueue->max_payload_size) > NET_PAYLOAD_SIZE_MAX) {
			rc = -1;
			goto out;
		}

		mqueue->payload_offset = entry->payload_offset;
		mqueue->offset = entry->offset;
	}

out:
	return rc;
}

int media_api_open(struct media_queue *mqueue, struct media_queue_api_params *params, unsigned int talker)
{
	struct media_queue_table *entry;
	unsigned int flags;

	rtos_mutex_lock(&table_mutex, RTOS_WAIT_FOREVER);

	if (talker)
		flags = MEDIA_QUEUE_FLAGS_TALKER;
	else
		flags = 0;

	entry = mqueue_table_find(params->stream_id, flags);
	if (!entry) {
		entry = mqueue_table_add(params->stream_id, flags);
		if (!entry)
			goto err_unlock;
	}

	if (mqueue_table_bind(entry, MEDIA_QUEUE_FLAGS_API_BOUND) < 0)
		goto err_unlock;

	mqueue->port = params->port; /* FIXME logical to physical? */
	mqueue->clock_gptp = params->clock_gptp;
	mqueue->batch_size = params->batch_size;
	mqueue->frame_stride = params->frame_stride;
	mqueue->frame_size = params->frame_size;
	mqueue->max_payload_size = params->max_payload_size;

	if (talker)
		queue_init(&mqueue->queue, net_tx_desc_free, 0);
	else
		queue_init(&mqueue->queue, net_rx_desc_free, 0);

	entry->mqueue = mqueue;

	if (media_params_check(entry, entry->flags) < 0)
		goto err_check;

	params->batch_size = mqueue->batch_size;

	rtos_atomic_set(&mqueue->available, 0);
	rtos_atomic_set(&mqueue->eofs, 0);

	mqueue->id = entry;

	rtos_mutex_unlock(&table_mutex);

	return 0;

err_check:
	entry->mqueue = NULL;
	mqueue_table_unbind(entry, MEDIA_QUEUE_FLAGS_API_BOUND);

err_unlock:
	rtos_mutex_unlock(&table_mutex);
	mqueue->id = NULL;

	return -1;
}

void media_api_close(struct media_queue *mqueue)
{
	struct media_queue_table *entry;

	rtos_mutex_lock(&table_mutex, RTOS_WAIT_FOREVER);

	mqueue_flush(mqueue);

	entry = mqueue->id;

	mqueue_table_unbind(entry, MEDIA_QUEUE_FLAGS_API_BOUND);
	entry->mqueue = NULL;
	mqueue->id = NULL;

	rtos_mutex_unlock(&table_mutex);

	return;
}


static void rx_write_event(struct genavb_event *avb_event, struct media_desc *desc, struct event_info *event_info)
{
	struct genavb_event *event;
	struct genavb_event event_local;
	unsigned int len_now;
	unsigned int i;

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

		memcpy(event, &event_local, sizeof(struct genavb_event));

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


int media_api_read(struct media_queue *mqueue, struct genavb_iovec const *data_iov, unsigned int data_iov_len,
				struct genavb_iovec const *event_iov, unsigned int event_iov_len, unsigned int *event_len)
{
	int total_dst_len, dst_len, total_read, len_now;  // Those variables are in number of bytes
	struct event_info event_info;
	int iov_idx; /* Iovec index */
	struct genavb_event *event = NULL;
	uint8_t *dst, *src;
	int src_len, src_frame_len;
	struct media_desc *desc;
	int i, skip_event = 0;
	int finished, rc = 0;
	const int stride_overhead = mqueue->frame_stride - mqueue->frame_size;

	total_read = 0;
	event_info.total_read = 0;

	if (event_iov_len == 0)
		skip_event = 1;

	total_dst_len = 0;
	for (i = 0; i < data_iov_len; i++) {
		if (total_dst_len + data_iov[i].iov_len < total_dst_len) {
			rc = -1;
			goto early_exit;
		}

		total_dst_len += data_iov[i].iov_len;
	}

	if (total_dst_len == 0) {
		goto early_exit;
	}


	event_info.total_dst_len = 0;
	if (!skip_event) {
		for (i = 0; i < event_iov_len; i++) {
			if ((event_info.total_dst_len + event_iov[i].iov_len) < event_info.total_dst_len) {
				rc = -1;
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
	if (likely(mqueue->partial_desc != NULL)) {
		desc = mqueue->partial_desc;

		src = mqueue->src;
		src_len = mqueue->src_len;
		src_frame_len = mqueue->src_frame_len;

		event_info.dst_offset = src - ((uint8_t *)desc + desc->l2_offset) - stride_overhead;

		if (stride_overhead)
			event_info.dst_offset -= (event_info.dst_offset / mqueue->frame_stride) * stride_overhead;

		event_info.dst_offset = -event_info.dst_offset;
	} else {
		desc = (struct media_desc *)queue_dequeue(&mqueue->queue);
		if ((unsigned long)desc == (unsigned long)-1) {
			desc = NULL;
			goto early_exit;
		}

		src_len = desc->len;
		src = (uint8_t *)desc + desc->l2_offset;

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

			memcpy(dst, src, src_len_now);

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
					memcpy(dst, src, mqueue->frame_size);

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
					memcpy(dst, src, src_len_now);

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
		if ( (total_dst_len == 0)
		   || ((event_info.total_dst_len == 0) && !skip_event)
		   || (desc->n_ts && (desc->avtp_ts[desc->n_ts - 1].flags & AVTP_FLAGS_TO_MEDIA_DESC(AVTP_END_OF_FRAME))))
			goto exit;

		/* Update data_iov/data dest */
		dst_len -= len_now;
		if (dst_len == 0) {
			iov_idx++;
			if (iov_idx < data_iov_len) {
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
				if (event_info.dst_iov_idx < event_iov_len) {
					event_info.dst_idx = 0;
					event_info.dst_len = event_iov[event_info.dst_iov_idx].iov_len;
					event = event_iov[event_info.dst_iov_idx].iov_base;
				} else {
					goto exit;
				}
			}
		}

		/* Update desc: if we got to there we still have space available in the iovecs, so it is safe to dequeue another desc */
		if ((src_len == 0) && ((event_info.src_len == 0) || skip_event)) {
			/* The End-of-Frame marker is assumed always to be at the end of a packet, or at least always to be the last event in a packet.
			   Note: this test should never trigger, since the EOF should be caught by the quick exit test above. */
			if (desc->n_ts && (desc->avtp_ts[desc->n_ts - 1].flags & AVTP_FLAGS_TO_MEDIA_DESC(AVTP_END_OF_FRAME)))
				rtos_atomic_dec(&mqueue->eofs);

			net_rx_free((struct net_rx_desc *)desc);

			desc = (struct media_desc *)queue_dequeue(&mqueue->queue);
			if ((unsigned long)desc == (unsigned long)-1) {
				desc = NULL;
				goto exit;
			}

			src_len = desc->len;
			src = (uint8_t *)desc + desc->l2_offset;

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
		u16 new_offset = src - (uint8_t *)desc;

		mqueue->partial_desc = desc;
		mqueue->src_len = desc->len - (new_offset - desc->l2_offset);

		if (stride_overhead)
			mqueue->src_len -= (mqueue->src_len / mqueue->frame_stride) * stride_overhead;

		mqueue->src = src;
		mqueue->src_frame_len = src_frame_len;

		mqueue->event_src_idx = event_info.src_idx;
	} else {
		if (desc) {
			/* The End-of-Frame marker is assumed always to be at the end of a packet, or at least always to be the last event in a packet. */
			if (desc->n_ts && (desc->avtp_ts[desc->n_ts - 1].flags & AVTP_FLAGS_TO_MEDIA_DESC(AVTP_END_OF_FRAME)))
				rtos_atomic_dec(&mqueue->eofs);

			net_rx_free((struct net_rx_desc *)desc);
		}

		mqueue->partial_desc = NULL;

		mqueue->event_src_idx = 0;
	}

early_exit:
	rtos_atomic_sub(total_read, &mqueue->available);

	if (event_len)
		*event_len = event_info.total_read;

	if (rc < 0)
		return rc;
	else
		return total_read;
}

static void tx_read_event(struct media_rx_desc *desc, const struct genavb_event *event_array, struct event_info *event_info, unsigned int dst_len, unsigned int src_len)
{
	const struct genavb_event *event;

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

#define DESC_MAX	32

int media_api_write(struct media_queue *mqueue, struct genavb_iovec const *data_iov, unsigned int data_iov_len,
				struct genavb_event const *event, unsigned int event_len)
{
	struct media_rx_desc *desc[DESC_MAX + 1];
	unsigned int used_len, total_src_len, total_dst_len, total_len, len_now;
	unsigned int n, n_now;
	uint8_t *src, *dst = NULL;
	uint32_t write;
	unsigned int src_len, src_len_now, dst_len = 0, dst_frame_len = 0, write_total = 0, frame_size, stride_overhead;
	struct event_info event_info;
	int i, iov_idx;
	int rc = 0;

	total_src_len = 0;
	for (i = 0; i < data_iov_len; i++) {
		if (total_src_len + data_iov[i].iov_len < total_src_len) {
			rc = -1;
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
		rc = -1;
		goto exit;
	}

	total_dst_len = n * mqueue->max_frame_payload_size - used_len;

	if (mqueue->frame_size == mqueue->frame_stride) {
		frame_size = mqueue->max_frame_payload_size;
		stride_overhead = 0;
	} else {
		frame_size = mqueue->frame_size;
		stride_overhead = mqueue->frame_stride - mqueue->frame_size;
	}

	if (total_dst_len > total_src_len) {
		if (total_src_len > dst_len)
			n = ((total_src_len - dst_len) + mqueue->max_frame_payload_size - 1) / mqueue->max_frame_payload_size;
		else
			n = 0;

		if (dst_len)
			n += 1;

		total_len = total_src_len;
	} else
		total_len = total_dst_len;

	iov_idx = 0;
	src = data_iov[iov_idx].iov_base;
	src_len = data_iov[iov_idx].iov_len;

	event_info.src_len = event_len;
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

			rc = net_pool_tx_alloc_multi((struct net_tx_desc **)&desc[1], n_now - 1,
						mqueue->max_frame_payload_size + mqueue->payload_offset - NET_DATA_OFFSET);
			if (rc < 0)
				break;

			n_now = rc + 1;

			len_now = (n_now - 1) * mqueue->max_frame_payload_size + dst_len;

			event_info.dst_idx = mqueue->ts_dst_idx;
			event_info.dst_offset = mqueue->ts_dst_offset;
			event_info.dst_len = mqueue->ts_dst_len;

		} else {
			rc = net_pool_tx_alloc_multi((struct net_tx_desc **)&desc[0], n_now,
						mqueue->max_frame_payload_size + mqueue->payload_offset - NET_DATA_OFFSET);
			if (rc <= 0)
				break;

			n_now = rc;

			dst = (uint8_t *)desc[0] + mqueue->payload_offset + stride_overhead;
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

				memcpy(dst, src, dst_frame_len);

				dst += dst_frame_len + stride_overhead;
				dst_len -= dst_frame_len;
				len_now -= dst_frame_len;
				total_len -= dst_frame_len;

				src += dst_frame_len;
				src_len_now -= dst_frame_len;
				write_total += dst_frame_len;

				dst_frame_len = frame_size;

				tx_read_event(desc[i], event, &event_info, dst_frame_len + stride_overhead, dst_frame_len);

				if (!dst_len) {
					desc[i]->net.len = mqueue->max_payload_size;
					desc[i]->net.l2_offset = mqueue->payload_offset;
					desc[i]->net.flags = 0;
					/*This is the last packet , mark it as end of frame*/
					if ((iov_idx >= data_iov_len - 1) && ((event_len &&  (event[0].event_mask & AVTP_FRAME_END))) && !src_len)
						desc[i]->net.flags |= NET_TX_FLAGS_END_FRAME;

					queue_enqueue_next(&mqueue->queue, &write, (unsigned long)desc[i]);

					i++;

					if (i < n_now) {
						dst = (uint8_t *)desc[i] + mqueue->payload_offset + stride_overhead;
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
				memcpy(dst, src, src_len_now);

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
		if ((event_len && ((event[0].event_mask & AVTP_FLUSH) || (event[0].event_mask & AVTP_FRAME_END)))) {
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

	if (write_total)
		rc = write_total;

exit:
	return rc;
}

void media_api_set_callback(struct media_queue *mqueue, int (*callback)(void *), void *data)
{
	rtos_mutex_lock(&table_mutex, RTOS_WAIT_FOREVER);

	mqueue->callback = callback;
	mqueue->callback_data = data;

	if (mqueue->callback)
		mqueue->callback_enabled = true;
	else
		mqueue->callback_enabled = false;

	rtos_mutex_unlock(&table_mutex);
}

int media_api_enable_callback(struct media_queue *mqueue)
{
	int rc = 0;

	rtos_mutex_lock(&table_mutex, RTOS_WAIT_FOREVER);

	if (!mqueue->callback) {
		rc = -1;
		goto out;
	}

	if (mqueue->callback_enabled) {
		goto out;
	} else {
		if (mqueue->flags & MEDIA_QUEUE_FLAGS_TALKER) {
			if (media_queue_remaining(mqueue) >= mqueue->batch_size)
				mqueue->callback(mqueue->callback_data);
		} else {
			if (need_to_wake_up_listener_queue(mqueue))
				mqueue->callback(mqueue->callback_data);
		}
	}

	mqueue->callback_enabled = true;

out:
	rtos_mutex_unlock(&table_mutex);
	return rc;
}

int media_net_open(struct media_queue_net_params *params, unsigned int talker)
{
	struct media_queue_table *entry;
	unsigned int flags;

	rtos_mutex_lock(&table_mutex, RTOS_WAIT_FOREVER);

	if (talker)
		flags = MEDIA_QUEUE_FLAGS_TALKER;
	else
		flags = 0;

	entry = mqueue_table_find(params->stream_id, flags);
	if (!entry) {
		entry = mqueue_table_add(params->stream_id, flags);
		if (!entry)
			goto err_unlock;
	}

	if (mqueue_table_bind(entry, MEDIA_QUEUE_FLAGS_NET_BOUND) < 0)
		goto err_unlock;

	if (flags & MEDIA_QUEUE_FLAGS_TALKER) {
		entry->payload_offset = params->talker.payload_offset;
		entry->offset = params->talker.ts_offset;
	}

	if (media_params_check(entry, entry->flags) < 0)
		goto err_check;

	rtos_mutex_unlock(&table_mutex);

	return mqueue_table_id(entry);

err_check:
	mqueue_table_unbind(entry, MEDIA_QUEUE_FLAGS_NET_BOUND);

err_unlock:
	rtos_mutex_unlock(&table_mutex);

	return -1;
}

void media_net_close(int id)
{
	struct media_queue_table *entry;

	rtos_mutex_lock(&table_mutex, RTOS_WAIT_FOREVER);

	entry = &mqueue_table[id];

	mqueue_table_unbind(entry, MEDIA_QUEUE_FLAGS_NET_BOUND);

	rtos_mutex_unlock(&table_mutex);
}


int media_net_write(int id, struct media_desc **desc_array, unsigned int n)
{
	struct media_queue *mqueue;
	struct media_desc *desc;
	unsigned int desc_len, written = 0;
	uint32_t qa, write;
	int i;

	rtos_mutex_lock(&table_mutex, RTOS_WAIT_FOREVER);

	mqueue = mqueue_get(id, 0);
	if (!mqueue)
		goto err_unlock;

	qa = queue_available(&mqueue->queue);
	if (n > qa)
		n = qa;

	queue_enqueue_init(&mqueue->queue, &write);

	for (i = 0; i < n; i++) {
		desc = desc_array[i];

		if (desc->len > NET_PAYLOAD_SIZE_MAX) {
			os_log(LOG_ERR, "desc(%lx), len(%u) too big\n", desc, desc->len);
			break;
		}

		if (desc->l2_offset > (NET_DATA_SIZE - desc->len)) {
			os_log(LOG_ERR, "desc(%lx), offset(%u) too big\n", desc, desc->l2_offset);
			break;
		}

		if (mqueue->frame_stride != mqueue->frame_size)
			desc_len = (desc->len * mqueue->frame_size) / mqueue->frame_stride;
		else
			desc_len = desc->len;

		queue_enqueue_next(&mqueue->queue, &write, (unsigned long)desc);

		/* The End-of-Frame marker is assumed always to be at the end of a packet, or at least always to be the last event in a packet. */
		if (desc->n_ts && (desc->avtp_ts[desc->n_ts - 1].flags & AVTP_FLAGS_TO_MEDIA_DESC(AVTP_END_OF_FRAME)))
			rtos_atomic_inc(&mqueue->eofs);

		written += desc_len;
	}

	queue_enqueue_done(&mqueue->queue, write);

	if (written) {
		rtos_atomic_add(written, &mqueue->available);

		if (mqueue->callback_enabled && need_to_wake_up_listener_queue(mqueue)) {
			mqueue->callback(mqueue->callback_data);
			mqueue->callback_enabled = false;
		}
	}

	rtos_mutex_unlock(&table_mutex);

	return i;

err_unlock:
	rtos_mutex_unlock(&table_mutex);

	return -1;
}

int media_net_read(int id, struct media_rx_desc **desc_array, unsigned int n)
{
	struct media_queue *mqueue;
	struct media_rx_desc *desc;
	uint32_t ptp_now;
	unsigned int ptp_read = 0;
	uint32_t read, _read, qp;
	int i = 0;

	rtos_mutex_lock(&table_mutex, RTOS_WAIT_FOREVER);

	mqueue = mqueue_get(id, MEDIA_QUEUE_FLAGS_TALKER);
	if (!mqueue)
		goto err_unlock;

	qp = queue_pending(&mqueue->queue);
	if (n > qp)
		n = qp;

	queue_dequeue_init(&mqueue->queue, &read);

	_read = read;
	while (n) {
		desc = (struct media_rx_desc *)queue_dequeue_next(&mqueue->queue, &_read);

		if (desc->ts_n) {
			if (!ptp_read)
				if (os_clock_gettime32(mqueue->clock_gptp, &ptp_now) >= 0)
					ptp_read = 1;

			if (ptp_read && avtp_before(ptp_now + mqueue->offset, desc->avtp_ts[0].val))
				break;
		}

		desc = (struct media_rx_desc *)queue_dequeue(&mqueue->queue);

		desc_array[i] = desc;
		read = _read;
		i++;
		n--;
	}

	queue_dequeue_done(&mqueue->queue, read);

	if (mqueue->callback_enabled && (media_queue_remaining(mqueue) >= mqueue->batch_size) /* || queue_empty(&mqueue->queue) */) {
		mqueue->callback(mqueue->callback_data);
		mqueue->callback_enabled = false;
	}

	rtos_mutex_unlock(&table_mutex);

	return i;

err_unlock:
	rtos_mutex_unlock(&table_mutex);

	return -1;
}

__init int media_queue_init(void)
{
	if (rtos_mutex_init(&table_mutex) < 0)
		return -1;
	else
		return 0;
}

__exit void media_queue_exit(void)
{
}
