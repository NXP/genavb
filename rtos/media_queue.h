/*
 * Copyright 2018-2019, 2021-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief RTOS specific media interface implementation
 @details
*/

#ifndef _RTOS_MEDIA_QUEUE_H_
#define _RTOS_MEDIA_QUEUE_H_

#include "rtos_abstraction_layer.h"

#include "avb_queue.h"

#include "os/clock.h"

#include "genavb/media.h"

#define MEDIA_QUEUE_LENGTH	32

struct media_queue_api_params {
	unsigned int port;
	os_clock_id_t clock_gptp;
	uint8_t stream_id[8];
	unsigned int frame_stride;			/**< Space in bytes between the start of 2 media frames in the payload part of the packet (e.g. 192 bytes for 61883-4).
										 * If 0 (common case), the whole payload will be treated as media data with no holes in it. */
	unsigned int frame_size;			/**< Size of a media frame in bytes. May be less than frame_stride (e.g. 188 bytes for 61883-4) */
	unsigned int queue_size;			/** Size of the queue in ??? */
	unsigned int batch_size;			/** Size of a batch in ??? */  // TODO determine size based on what? stream bandwidth and max pkt size?
	unsigned int max_payload_size;			/**< Maximum size of the AVTP payload in bytes. Used in talker mode to split incoming stream of data into properly sized chunks. */
};

struct media_queue_net_params {
	uint8_t stream_id[8];

	struct {
		unsigned int payload_offset;		/**< offset in bytes between start of buffers and start of AVTP payload, in bytes. */
		unsigned int ts_offset;			/**< offset in ns to be used when passing samples to the AVTP thread */
	} talker;
};

struct media_queue {
	void *partial_desc;

	unsigned int port;
	rtos_atomic_t available;
	rtos_atomic_t eofs;
	unsigned int flags;

	os_clock_id_t clock_gptp;

	int (*callback)(void *data);
	void *callback_data;
	bool callback_enabled;

	void *id;

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

	uint8_t *dst;
	unsigned int dst_len;
	unsigned int dst_frame_len;
	unsigned int ts_dst_idx;
	unsigned int ts_dst_offset;
	unsigned int ts_dst_len;

	struct queue queue;
};

#if defined(CONFIG_AVTP)
int media_api_open(struct media_queue *mqueue, struct media_queue_api_params *params, unsigned int talker);
void media_api_close(struct media_queue *mqueue);
int media_api_read(struct media_queue *mqueue, struct genavb_iovec const *data_iov, unsigned int data_iov_len,
				struct genavb_iovec const *event_iov, unsigned int event_iov_len, unsigned int *event_len);
int media_api_write(struct media_queue *mqueue, struct genavb_iovec const *data_iov, unsigned int data_iov_len,
				struct genavb_event const *event, unsigned int event_len);
void media_api_set_callback(struct media_queue *mqueue, int (*callback)(void *), void *data);
int media_api_enable_callback(struct media_queue *mqueue);

int media_net_open(struct media_queue_net_params *params, unsigned int talker);
void media_net_close(int id);
int media_net_write(int id, struct media_desc **desc, unsigned int n);
int media_net_read(int id, struct media_rx_desc **desc, unsigned int n);

int media_queue_init(void);
void media_queue_exit(void);
#else
static inline int media_queue_init(void)
{
	return 0;
}
static inline void media_queue_exit(void)
{
	return;
}
#endif

#endif /* _RTOS_MEDIA_QUEUE_H_ */
