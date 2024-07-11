/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2020, 2022-2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _AVBDRV_H_
#define _AVBDRV_H_

#include "pool_dma.h"
#include "netdrv.h"
#include "queue.h"
#include "net_tx.h"
#include "ipc.h"
#include "media.h"
#include "net_port.h"
#include "media_clock.h"
#include "media_clock_drv.h"
#include "mtimer.h"
#include "mtimer_drv.h"
#include "hw_timer.h"
#include "net_logical_port.h"

#ifdef __KERNEL__


#include <linux/cdev.h>
#include <linux/fec.h>

#define AVBDRV_NAME		"avbdrv"
#define AVBDRV_MINOR		0
#define AVBDRV_MINOR_COUNT	1

void *avb_alloc_range(unsigned int size);
void avb_free_range(void *baseaddr, unsigned int size);

struct avb_drv
{
	struct cdev cdev;
	dev_t devno;
	struct pool_dma buf_pool;
	struct net_drv net_drv;
	struct media_drv media_drv;
	struct logical_port logical_port[CFG_LOGICAL_NUM_PORT];
	struct eth_avb eth[CFG_PORTS];
	struct net_qos qos;
	void *buf_baseaddr;
	struct dentry *debugfs;
	struct mclock_drv mclock_drv;
	struct mtimer_drv mtimer_drv;
	struct platform_driver epit_driver;
	struct platform_driver gpt_driver;
	struct platform_driver tpm_driver;
	struct hw_timer timer;
};

struct avb_usr
{
	struct avb_drv *avb;
};

/* These must match the limits set in the code */
#define MEDIA_QUEUE_SIZE	(QUEUE_ENTRIES_MAX + CFG_MEDIA_QUEUE_EXTRA_ENTRIES)
#define RX_QUEUE_SIZE		(QUEUE_ENTRIES_MAX + CFG_RX_EXTRA_ENTRIES)
#define TX_CLEANUP_QUEUE_SIZE	(QUEUE_ENTRIES_MAX + CFG_TX_CLEANUP_EXTRA_ENTRIES)
#define TX_BEST_EFFORT_TOTAL_QUEUE_SIZE (CFG_TX_BEST_EFFORT * QUEUE_ENTRIES_MAX)

#define TX_CLEANUP_QUEUE_MIN_AVAIL (FEC_TX_RING_SIZE + TX_BEST_EFFORT_TOTAL_QUEUE_SIZE)
#define TX_CLEANUP_QUEUE_MAX_AVAIL (TX_CLEANUP_QUEUE_MIN_AVAIL + QUEUE_ENTRIES_MAX)

#if TX_CLEANUP_QUEUE_SIZE < TX_CLEANUP_QUEUE_MAX_AVAIL
#error TX_CLEANUP_QUEUE_SIZE too small
#endif

#if RX_QUEUE_SIZE < FEC_RX_RING_SIZE
#error RX_QUEUE_SIZE too small
#endif

/* not SR traffic (best effort, msrp, ptp...) */
#define TX_BUFFERS_MAX	((CFG_TX_PROTO * (QUEUE_ENTRIES_MAX + CFG_NET_TX_EXTRA_ENTRIES) + TX_BEST_EFFORT_TOTAL_QUEUE_SIZE + FEC_TX_RING_SIZE + TX_CLEANUP_QUEUE_SIZE) * CFG_PORTS)
#define RX_BUFFERS_MAX	(CFG_RX_PROTO * QUEUE_ENTRIES_MAX + CFG_RX_BEST_EFFORT * (RX_QUEUE_SIZE + FEC_RX_RING_SIZE) * CFG_PORTS)

/* SR traffic (AVTP) */
#define BUFFERS_MAX	(TX_BUFFERS_MAX + RX_BUFFERS_MAX + CFG_STREAM_MAX * (MEDIA_QUEUE_SIZE + QUEUE_ENTRIES_MAX + CFG_NET_TX_EXTRA_ENTRIES))

#define BUF_ORDER	11
#define BUF_SIZE	(1 << BUF_ORDER)
#define BUF_POOL_PAGES	((BUFFERS_MAX * BUF_SIZE + PAGE_SIZE - 1) / PAGE_SIZE)
#define BUF_POOL_SIZE	(BUF_POOL_PAGES * PAGE_SIZE)

#endif /* __KERNEL__ */


#if CFG_SR_CLASS_HIGH_STREAM_MAX > CFG_SR_CLASS_STREAM_MAX
#error CFG_SR_CLASS_HIGH_STREAM_MAX too big
#endif

#if CFG_SR_CLASS_LOW_STREAM_MAX > CFG_SR_CLASS_STREAM_MAX
#error CFG_SR_CLASS_LOW_STREAM_MAX too big
#endif

#if (CFG_RX_STREAM_MAX > CFG_SR_CLASS_HIGH_STREAM_MAX) || (CFG_RX_STREAM_MAX > CFG_SR_CLASS_LOW_STREAM_MAX)
#error CFG_RX_STREAMS_MAX too big
#endif

#if (CFG_TX_STREAM_MAX > CFG_SR_CLASS_HIGH_STREAM_MAX) || (CFG_TX_STREAM_MAX > CFG_SR_CLASS_LOW_STREAM_MAX)
#error CFG_TX_STREAM_MAX too big
#endif

#if (CFG_STREAM_MAX > CFG_RX_STREAM_MAX) || (CFG_STREAM_MAX > CFG_TX_STREAM_MAX)
#error CFG_STREAM_MAX too big
#endif

#define AVBDRV_IOC_MAGIC		'q'
#define AVBDRV_IOC_SHMEM_SIZE		_IOR(AVBDRV_IOC_MAGIC, 0, unsigned long)

#endif /* _AVBDRV_H_ */
