/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2018, 2020, 2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _IPCDRV_H_
#define _IPCDRV_H_

#ifdef __KERNEL__

#include <linux/cdev.h>
#include <linux/string.h>

#include "pool.h"
#include "queue.h"

#define IPCDRV_NAME		"ipcdrv"
#define IPCDRV_MINOR		0
#define IPCDRV_MINOR_COUNT	256
#define IPCDRV_IPC_MAX		((IPC_SINGLE_READER_WRITER_MAX + IPC_MANY_READERS_MAX + IPC_MANY_WRITERS_MAX) / 2)

#define IPC_MANY_WRITERS_MINOR_BASE		0	/* 0 - 89 */
#define IPC_MANY_WRITERS_INDEX_BASE		0	/* 0 - 89 */
#define IPC_MANY_WRITERS_MAX			90

#define IPC_MANY_READERS_MINOR_BASE		100	/* 100 - 189 */
#define IPC_MANY_READERS_INDEX_BASE		90	/* 90 - 179 */
#define IPC_MANY_READERS_MAX			90

#define IPC_SINGLE_READER_WRITER_MINOR_BASE	200	/* 200 - 231 */
#define IPC_SINGLE_READER_WRITER_INDEX_BASE	180	/* 180 - 211 */
#define IPC_SINGLE_READER_WRITER_MAX		32

struct ipc_slot {
	struct queue queue;

	wait_queue_head_t wait;

	struct pool buf_pool;

	void *mmap_base;
	unsigned long mmap_size;

	struct ipc_channel *ipc;

	unsigned int index;
};

/* Queue kernel character device instance */
struct ipc_dev {
	unsigned int flags;

	struct ipc_slot slot;
};

#define IPC_MAX_READER_WRITERS	8
#define IPCDEV_FLAGS_RX	(1 << 0)

struct ipc_dst_map {
	unsigned int dst;
	struct ipc_slot *dst_slot;
};

struct ipc_channel {
	unsigned int index;

	unsigned int type;

	struct mutex lock;

	struct ipc_slot *slot[IPC_MAX_READER_WRITERS + 1];

	struct ipc_dst_map dst_map[IPC_MAX_READER_WRITERS + 1];

	unsigned int last;
};


struct ipc_drv {
	struct ipc_channel ipc[IPCDRV_IPC_MAX];

	struct cdev cdev;
	dev_t devno;
};

int ipcdrv_init(struct ipc_drv *drv);
void ipcdrv_exit(struct ipc_drv *drv);

#define IPC_BUF_COUNT		QUEUE_ENTRIES_MAX
#define IPC_BUF_POOL_PAGES	((IPC_BUF_COUNT * IPC_BUF_SIZE + PAGE_SIZE - 1) / PAGE_SIZE)
#define IPC_BUF_POOL_SIZE	(IPC_BUF_POOL_PAGES * PAGE_SIZE)

#endif /* !__KERNEL__ */

#define IPC_BUF_ORDER		10
#define IPC_BUF_SIZE		(1 << IPC_BUF_ORDER)

struct ipc_tx_data {
	unsigned long addr_shmem;
	unsigned int dst;
	unsigned int len;
};

struct ipc_rx_data {
	unsigned long addr_shmem;
	unsigned int src;
};

#define IPC_IOC_MAGIC		'i'

#define IPC_IOC_ALLOC		_IOR(IPC_IOC_MAGIC, 0, unsigned long)
#define IPC_IOC_FREE		_IOW(IPC_IOC_MAGIC, 1, unsigned long)
#define IPC_IOC_RX		_IOR(IPC_IOC_MAGIC, 2, struct ipc_rx_data)
#define IPC_IOC_TX		_IOW(IPC_IOC_MAGIC, 3, struct ipc_tx_data)
#define IPC_IOC_POOL_SIZE	_IOR(IPC_IOC_MAGIC, 4, unsigned long)
#define IPC_IOC_CONNECT_TX	_IOW(IPC_IOC_MAGIC, 5, unsigned long)

#endif /* _IPCDRV_H_ */
