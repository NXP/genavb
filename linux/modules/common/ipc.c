/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2020, 2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/file.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <linux/vmalloc.h>

#include "ipc.h"

#define IPC_TYPE_MANY_READERS	0
#define IPC_TYPE_MANY_WRITERS	1
#define IPC_TYPE_SINGLE_READER_WRITER	2

#define IPC_DST_ALL	0xffff

static const struct file_operations ipcdrv_fops;

/**
 * DOC: IPC service
 *
 * Simple IPC service used by AVB userspace stack components.
 * A fixed number of unidirectional IPC channels is defined. For now a single reader/writer is supported
 * per IPC channel. On open the minor number of the device is used to map the read or write side of an IPC channel.
 * The users of the API must agree one a shared minor number to use to be able to communicate.
 * Each IPC channel uses a dedicated poll of buffers (that is mmaped in userspace).
 *
 */

static void *ipc_shmem_to_virt(struct ipc_slot *slot, unsigned long addr_shmem)
{
	return pool_user_shmem_to_virt(&slot->buf_pool, addr_shmem);
}

static unsigned long ipc_virt_to_shmem(struct ipc_slot *slot, void *addr)
{
	return pool_virt_to_shmem(&slot->buf_pool, addr);
}

static void ipc_flush_queue(struct ipc_slot *slot)
{
	queue_flush(&slot->queue, &slot->buf_pool);
}

static void ipc_free(struct ipc_slot *slot, void *addr)
{
	pool_free(&slot->buf_pool, addr);
}


static int ipc_alloc_user(struct ipc_slot *slot, unsigned long arg)
{
	unsigned long addr_shmem;
	int rc;

	rc = pool_alloc_shmem(&slot->buf_pool, &addr_shmem);
	if (rc < 0)
		goto err;

	return put_user(addr_shmem, (unsigned long *)arg);

err:
	return rc;
}

static int ipc_free_user(struct ipc_slot *slot, unsigned long arg)
{
	unsigned long addr_shmem;
	int rc;

	rc = get_user(addr_shmem, (unsigned long *)arg);
	if (rc < 0)
		goto err;

	pool_free_shmem(&slot->buf_pool, addr_shmem);

	return 0;

err:
	return rc;
}

static int ipc_is_free_slot(struct ipc_channel *ipc, unsigned int i)
{
	if (ipc->slot[i])
		return 0;

	return 1;
}

static int ipc_is_disabled_slot(struct ipc_slot *slot)
{
	return !slot;
}

static int ipc_find_slot(struct ipc_channel *ipc)
{
	int i;

	for (i = 1; i <= IPC_MAX_READER_WRITERS; i++) {
		if (ipc_is_free_slot(ipc, i))
			return i;
	}

	return -1;
}

static void ipc_alloc_slot(struct ipc_channel *ipc, unsigned int index, struct ipc_slot *slot)
{
	ipc->slot[index] = slot;

	slot->ipc = ipc;
	slot->index = index;
}

static void ipc_free_slot(struct ipc_slot *slot)
{
	slot->ipc->slot[slot->index] = NULL;
}

static void *ipc_copy(struct ipc_slot *slot_dst, struct ipc_slot *slot_src, void *addr_src, unsigned int len)
{
	void *addr_dst;

	if (ipc_is_disabled_slot(slot_dst))
		goto err;

	addr_dst = pool_alloc(&slot_dst->buf_pool);
	if (!addr_dst)
		goto err;

	memcpy(addr_dst, addr_src, len);

	return addr_dst;

err:
	return NULL;
}

static int ipc_tx_slot(struct ipc_slot *queue_slot, struct ipc_slot *wake_slot, void *addr)
{
	if (queue_enqueue(&queue_slot->queue, (unsigned long)addr) < 0)
		goto err;

	wake_up(&wake_slot->wait);

	return 0;

err:
	return -1;
}

static int ipc_tx(struct ipc_slot *slot, void *addr, unsigned int len, unsigned int dst)
{
	struct ipc_channel *ipc = slot->ipc;
	struct ipc_slot *rx_slot;
	void *addr_rx;
	int rc;
	int i;

	mutex_lock(&ipc->lock);

	switch (ipc->type) {
	case IPC_TYPE_MANY_READERS:
		if (dst == IPC_DST_ALL) {
			for (i = 1; i <= IPC_MAX_READER_WRITERS; i++) {
				rx_slot = ipc->slot[i];

				addr_rx = ipc_copy(rx_slot, slot, addr, len);
				if (!addr_rx)
					continue;

				rc = ipc_tx_slot(rx_slot, rx_slot, addr_rx);
				if (rc < 0)
					ipc_free(rx_slot, addr_rx);

			}

			ipc_free(slot, addr);

		} else {

			unsigned int dst_index = dst & 0xff;

			/* send message to specific reader */
			if (!dst_index || (dst_index > IPC_MAX_READER_WRITERS))
				goto err_unlock;

			if (ipc->dst_map[dst_index].dst != dst)
				goto err_unlock;

			rx_slot = ipc->dst_map[dst_index].dst_slot;

			addr_rx = ipc_copy(rx_slot, slot, addr, len);
			if (!addr_rx)
				goto err_unlock;

			rc = ipc_tx_slot(rx_slot, rx_slot, addr_rx);
			if (rc < 0) {
				ipc_free(rx_slot, addr_rx);
				goto err_unlock;
			}

			ipc_free(slot, addr);
		}

		break;

	case IPC_TYPE_SINGLE_READER_WRITER:
		rx_slot = ipc->slot[0];

		addr_rx = ipc_copy(rx_slot, slot, addr, len);
		if (!addr_rx)
			goto err_unlock;

		rc = ipc_tx_slot(rx_slot, rx_slot, addr_rx);
		if (rc < 0) {
			ipc_free(rx_slot, addr_rx);
			goto err_unlock;
		}

		ipc_free(slot, addr);

		break;

	case IPC_TYPE_MANY_WRITERS:
		if (ipc_is_disabled_slot(ipc->slot[0]))
			goto err_unlock;

		rc = ipc_tx_slot(slot, ipc->slot[0], addr);
		if (rc < 0)
			goto err_unlock;

		break;

	default:
		goto err_unlock;
		break;
	}

	mutex_unlock(&ipc->lock);

	return 0;

err_unlock:
	mutex_unlock(&ipc->lock);

	return -1;
}

static int ipc_dequeue(struct ipc_slot *slot, void **addr)
{
	if (ipc_is_disabled_slot(slot))
		goto err;

	*addr = (void *)queue_dequeue(&slot->queue);
	if (*addr == (void *) - 1)
		goto err;

	return 0;

err:
	return -1;
}

static unsigned int ipc_slot_address(struct ipc_slot *slot)
{
	return (slot->index | (slot->ipc->index << 8));
}

static void *ipc_rx(struct ipc_slot *slot, unsigned int *src)
{
	struct ipc_channel *ipc = slot->ipc;
	struct ipc_slot *tx_slot;
	void *addr, *addr_tx;
	int i;

	mutex_lock(&ipc->lock);

	switch (ipc->type) {
	case IPC_TYPE_SINGLE_READER_WRITER:
	case IPC_TYPE_MANY_READERS:
		if (ipc_dequeue(slot, &addr) < 0)
			goto err_unlock;

		*src = 0;

		break;

	case IPC_TYPE_MANY_WRITERS:

		for (i = 1; i <= IPC_MAX_READER_WRITERS; i++) {

			ipc->last++;
			if (ipc->last > IPC_MAX_READER_WRITERS)
				ipc->last = 1;

			tx_slot = ipc->slot[ipc->last];

			if (ipc_dequeue(tx_slot, &addr_tx) < 0)
				continue;

			addr = ipc_copy(slot, tx_slot, addr_tx, 1 << IPC_BUF_ORDER);
			if (!addr) {
				ipc_free(tx_slot, addr_tx);
				continue;
			}

			*src = ipc_slot_address(tx_slot);

			ipc_free(tx_slot, addr_tx);

			goto out;
		}

		goto err_unlock;

		break;

	default:
		goto err_unlock;
		break;
	}

out:
	mutex_unlock(&ipc->lock);

	return addr;

err_unlock:
	mutex_unlock(&ipc->lock);
	return NULL;
}

static int ipc_rx_init(struct ipc_channel *ipc, struct ipc_slot *slot)
{
	int slot_i;
	int rc;

	mutex_lock(&ipc->lock);

	switch (ipc->type) {
	case IPC_TYPE_MANY_READERS:
		slot_i = ipc_find_slot(ipc);
		if (slot_i < 0) {
			rc = -EBUSY;
			goto err_unlock;
		}

		break;

	case IPC_TYPE_MANY_WRITERS:
	case IPC_TYPE_SINGLE_READER_WRITER:
		slot_i = 0;

		if (!ipc_is_free_slot(ipc, slot_i)) {
			rc = -EBUSY;
			goto  err_unlock;
		}

		break;

	default:
		rc = -EINVAL;
		goto err_unlock;
		break;
	}

	slot->mmap_size = IPC_BUF_POOL_SIZE;
	slot->mmap_base = vmalloc(IPC_BUF_POOL_SIZE);
	if (!slot->mmap_base) {
		rc = -ENOMEM;
		goto err_vmalloc;
	}

	if (pool_init(&slot->buf_pool, slot->mmap_base, slot->mmap_size, IPC_BUF_ORDER) < 0) {
		pr_err("%s: pool_init() failed\n", __func__);
		rc = -ENOMEM;
		goto err_pool_init;
	}

	ipc_alloc_slot(ipc, slot_i, slot);

	if (ipc->type != IPC_TYPE_MANY_WRITERS)
		queue_init(&slot->queue, pool_free_virt, 0);

	init_waitqueue_head(&slot->wait);

	mutex_unlock(&ipc->lock);

	return 0;

err_pool_init:
	vfree(slot->mmap_base);

err_vmalloc:
err_unlock:
	mutex_unlock(&ipc->lock);

	return rc;
}

static void ipc_rx_exit(struct ipc_slot *slot)
{
	struct ipc_channel *ipc = slot->ipc;
	int i;

	mutex_lock(&ipc->lock);

	switch (ipc->type) {
	case IPC_TYPE_MANY_READERS:
		ipc_flush_queue(slot);

		/* Clear mapping, if any */
		for (i = 0; i <= IPC_MAX_READER_WRITERS; i++)
			if (ipc->dst_map[i].dst_slot == slot) {
				ipc->dst_map[i].dst_slot = NULL;
				ipc->dst_map[i].dst = 0;
				break;
			}

		break;
	case IPC_TYPE_SINGLE_READER_WRITER:
		ipc_flush_queue(slot);

		break;

	case IPC_TYPE_MANY_WRITERS:
		break;

	default:
		break;
	}

	pool_exit(&slot->buf_pool);

	vfree(slot->mmap_base);

	ipc_free_slot(slot);

	mutex_unlock(&ipc->lock);

	return;
}

static int ipc_tx_init(struct ipc_channel *ipc, struct ipc_slot *slot)
{
	int slot_i;
	int rc;

	mutex_lock(&ipc->lock);

	switch (ipc->type) {
	case IPC_TYPE_MANY_READERS:
		slot_i = 0;

		if (!ipc_is_free_slot(ipc, slot_i)) {
			rc = -EBUSY;
			goto  err_unlock;
		}

		break;

	case IPC_TYPE_SINGLE_READER_WRITER:
		slot_i = 1;

		if (!ipc_is_free_slot(ipc, slot_i)) {
			rc = -EBUSY;
			goto  err_unlock;
		}

		break;

	case IPC_TYPE_MANY_WRITERS:
		slot_i = ipc_find_slot(ipc);
		if (slot_i < 0) {
			rc = -EBUSY;
			goto err_unlock;
		}

		break;

	default:
		rc = -EINVAL;
		goto err_unlock;
		break;
	}

	slot->mmap_size = IPC_BUF_POOL_SIZE;
	slot->mmap_base = vmalloc(IPC_BUF_POOL_SIZE);
	if (!slot->mmap_base) {
		rc = -ENOMEM;
		goto err_vmalloc;
	}

	if (pool_init(&slot->buf_pool, slot->mmap_base, slot->mmap_size, IPC_BUF_ORDER) < 0) {
		pr_err("%s: pool_init() failed\n", __func__);
		rc = -ENOMEM;
		goto err_pool_init;
	}

	if (ipc->type == IPC_TYPE_MANY_WRITERS)
		queue_init(&slot->queue, pool_free_virt, 0);

	ipc_alloc_slot(ipc, slot_i, slot);

	mutex_unlock(&ipc->lock);

	return 0;

err_pool_init:
	vfree(slot->mmap_base);

err_vmalloc:
err_unlock:
	mutex_unlock(&ipc->lock);

	return rc;
}

static void ipc_tx_exit(struct ipc_slot *slot)
{
	struct ipc_channel *ipc = slot->ipc;

	mutex_lock(&ipc->lock);

	switch (ipc->type) {
	case IPC_TYPE_MANY_READERS:
	case IPC_TYPE_SINGLE_READER_WRITER:
		break;

	case IPC_TYPE_MANY_WRITERS:
		ipc_flush_queue(slot);

		break;

	default:
		break;
	}

	pool_exit(&slot->buf_pool);

	vfree(slot->mmap_base);

	ipc_free_slot(slot);

	mutex_unlock(&ipc->lock);

	return;
}


/* Move all the shared memory related code to a new shmem.c */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,11,0)
static int ipcdrv_vma_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
#elif LINUX_VERSION_CODE <= KERNEL_VERSION(4,17,0)
static int ipcdrv_vma_fault(struct vm_fault *vmf)
#else
static vm_fault_t ipcdrv_vma_fault(struct vm_fault *vmf)
#endif
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,11,0)
	struct ipc_dev *dev = vma->vm_private_data;
#else
	struct ipc_dev *dev = vmf->vma->vm_private_data;
#endif
	struct ipc_slot *slot = &dev->slot;
	struct page *page;
	unsigned long offset;
	void *addr;

//	pr_info("%s: addr: %p, pgoff: %lx, flags: %x\n", __func__, vmf->virtual_address, vmf->pgoff, vmf->flags);

	offset = vmf->pgoff << PAGE_SHIFT;

	if (offset >= slot->mmap_size)
		return VM_FAULT_SIGBUS;

	addr = slot->mmap_base + offset;

	page = vmalloc_to_page(addr);
	get_page(page);

	vmf->page = page;

	return 0;
}

static void ipcdrv_vma_open(struct vm_area_struct *vma)
{
	//pr_info("%s: start: %lx, end: %lx, offset: %lx, flags: %lx\n", __func__, vma->vm_start, vma->vm_end, vma->vm_pgoff, vma->vm_flags);
}

static void ipcdrv_vma_close(struct vm_area_struct *vma)
{
	//pr_info("%s: start: %lx, end: %lx, offset: %lx, flags: %lx\n", __func__, vma->vm_start, vma->vm_end, vma->vm_pgoff, vma->vm_flags);
}

static const struct vm_operations_struct ipcdrv_mem_ops = {
	.open = ipcdrv_vma_open,
	.close = ipcdrv_vma_close,
	.fault = ipcdrv_vma_fault,
};

static int ipcdrv_mmap(struct file *file, struct vm_area_struct *vma)
{
	//pr_info("%s: start: %lx, end: %lx, offset: %lx, flags: %lx\n", __func__, vma->vm_start, vma->vm_end, vma->vm_pgoff, vma->vm_flags);

	if (vma->vm_end < vma->vm_start)
		return -EINVAL;

	vma->vm_ops = &ipcdrv_mem_ops;
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
	vma->vm_flags |= VM_RESERVED;
#endif

	vma->vm_private_data = file->private_data;

	return 0;
}

static long ipcdrv_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct ipc_dev *dev = file->private_data;
	struct ipc_slot *slot = &dev->slot;
	struct ipc_tx_data tx_data;
	struct ipc_rx_data rx_data;
	int user_fd;
	struct file *rx_file;
	struct ipc_dev *rx_dev;
	struct ipc_channel *rx_ipc;
	struct ipc_slot *rx_slot;
	void *addr;
	unsigned int src;
	int rc = 0;

	//pr_info("%s: file: %p cmd: %u, arg: %lu\n", __func__, file, cmd, arg);

	if (dev->flags & IPCDEV_FLAGS_RX) {
		switch (cmd) {

		case IPC_IOC_FREE:
			rc = ipc_free_user(slot, arg);

			break;

		case IPC_IOC_RX:

			addr = ipc_rx(slot, &src);
			if (!addr) {
				rc = -EAGAIN;
				break;
			}

			rx_data.addr_shmem = ipc_virt_to_shmem(slot, addr);
			rx_data.src = src;

			if (copy_to_user((void *)arg, &rx_data, sizeof(struct ipc_rx_data)))
				rc = -EFAULT;

			break;

		case IPC_IOC_POOL_SIZE:
			rc = put_user(slot->mmap_size, (unsigned long *)arg);

			break;

		default:
			rc = -EINVAL;
			break;
		}

	} else {
		switch (cmd) {
		case IPC_IOC_ALLOC:
			rc = ipc_alloc_user(slot, arg);
			break;

		case IPC_IOC_FREE:
			rc = ipc_free_user(slot, arg);
			break;

		case IPC_IOC_TX:
			if (copy_from_user(&tx_data, (void *)arg, sizeof(struct ipc_tx_data))) {
				rc = -EFAULT;
				break;
			}

			if (tx_data.len > (1 << IPC_BUF_ORDER)) {
				rc = -EFAULT;
				break;
			}

			addr = ipc_shmem_to_virt(slot, tx_data.addr_shmem);
			if (!addr) {
				rc = -EFAULT;
				break;
			}

			rc = ipc_tx(slot, addr, tx_data.len, tx_data.dst);

			break;

		case IPC_IOC_CONNECT_TX:
			rc = get_user(user_fd, (unsigned long *)arg);
			if (rc < 0)
				break;

			if (slot->ipc->type != IPC_TYPE_MANY_WRITERS) {
				rc = -EBADF;
				break;
			}

			rx_file = fget(user_fd);
			if (!rx_file) {
				rc = -EBADF;
				break;
			}

			if (rx_file->f_op != &ipcdrv_fops) {
				rc = -EBADF;
				goto put;
			}

			rx_dev = rx_file->private_data;

			if (!(rx_dev->flags & IPCDEV_FLAGS_RX)) {
				rc = -EBADF;
				goto put;
			}

			rx_slot = &rx_dev->slot;
			rx_ipc = rx_slot->ipc;

			if (rx_ipc->type != IPC_TYPE_MANY_READERS) {
				rc = -EBADF;
				goto put;
			}

			mutex_lock(&rx_ipc->lock);

			rx_ipc->dst_map[slot->index].dst_slot = rx_slot;
			rx_ipc->dst_map[slot->index].dst = ipc_slot_address(slot);

			mutex_unlock(&rx_ipc->lock);

		put:
			fput(rx_file);

			break;

		case IPC_IOC_POOL_SIZE:
			rc = put_user(slot->mmap_size, (unsigned long *)arg);

			break;

		default:
			rc = -EINVAL;
			break;
		}
	}

	return rc;
}

static unsigned int ipcdrv_poll(struct file *file, poll_table *poll)
{
	struct ipc_dev *dev = file->private_data;
	unsigned int mask = 0;
	int i;

//	pr_info("%s: file: %p\n", __func__, file);

	if (dev->flags & IPCDEV_FLAGS_RX) {
		struct ipc_slot *rx_slot = &dev->slot;
		struct ipc_channel *ipc = rx_slot->ipc;
		struct ipc_slot *tx_slot;

		poll_wait(file, &rx_slot->wait, poll);

		mutex_lock(&ipc->lock);

		switch (ipc->type) {
		case IPC_TYPE_MANY_READERS:
		case IPC_TYPE_SINGLE_READER_WRITER:

			if (queue_pending(&rx_slot->queue))
				mask |= POLLIN | POLLRDNORM;

			break;

		case IPC_TYPE_MANY_WRITERS:
			for (i = 1; i <= IPC_MAX_READER_WRITERS; i++) {
				tx_slot = ipc->slot[i];

				if (ipc_is_disabled_slot(tx_slot))
					continue;

				if (queue_pending(&tx_slot->queue)) {
					mask |= POLLIN | POLLRDNORM;
					break;
				}
			}

			break;

		default:
			break;
		}

		mutex_unlock(&ipc->lock);
	} else {
//		if (!queue_empty(dev->queue))
//			mask |= POLLOUT | POLLWRNORM;
	}

//	pr_info("%s: %x\n", __func__, mask);

	return mask;
}


static int ipcdrv_release(struct inode *in, struct file *file)
{
	struct ipc_dev *dev = file->private_data;

	if (dev->flags & IPCDEV_FLAGS_RX) {
		ipc_rx_exit(&dev->slot);

	} else {
		ipc_tx_exit(&dev->slot);
	}

	kfree(dev);

	file->private_data = NULL;

	return 0;
}

static int ipcdrv_open(struct inode *in, struct file *file)
{
	struct ipc_drv *drv = container_of(in->i_cdev, struct ipc_drv, cdev);
	struct ipc_dev *dev;
	struct ipc_channel *ipc;
	int rc;
	int minor = MINOR(in->i_rdev);
	unsigned int index;

	if ((minor >= IPC_MANY_WRITERS_MINOR_BASE) && (minor < (IPC_MANY_WRITERS_MINOR_BASE + IPC_MANY_WRITERS_MAX))) {
		index = (minor - IPC_MANY_WRITERS_MINOR_BASE) + IPC_MANY_WRITERS_INDEX_BASE;
	} else if (minor >= IPC_MANY_READERS_MINOR_BASE && (minor < (IPC_MANY_READERS_MINOR_BASE + IPC_MANY_READERS_MAX))) {
		index = (minor - IPC_MANY_READERS_MINOR_BASE) + IPC_MANY_READERS_INDEX_BASE;
	} else if (minor >= IPC_SINGLE_READER_WRITER_MINOR_BASE && (minor < (IPC_SINGLE_READER_WRITER_MINOR_BASE + IPC_SINGLE_READER_WRITER_MAX))){
		index = (minor - IPC_SINGLE_READER_WRITER_MINOR_BASE) + IPC_SINGLE_READER_WRITER_INDEX_BASE;
	} else {
		rc = -ENODEV;
		goto err;
	}

	ipc = &drv->ipc[index / 2];

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		rc = -ENOMEM;
		goto err;
	}

	if (index & 1) {
		rc = ipc_tx_init(ipc, &dev->slot);
	} else {
		dev->flags = IPCDEV_FLAGS_RX;

		rc = ipc_rx_init(ipc, &dev->slot);
	}

	if (rc < 0)
		goto err_init;

	file->private_data = dev;

	return 0;

err_init:
	kfree(dev);

err:
	return rc;
}

static const struct file_operations ipcdrv_fops = {
	.owner = THIS_MODULE,
	.open = ipcdrv_open,
	.release = ipcdrv_release,
	.unlocked_ioctl = ipcdrv_ioctl,
	.poll = ipcdrv_poll,
	.mmap = ipcdrv_mmap,
};

static int ipc_channel_init(struct ipc_channel *ipc, unsigned int index, struct ipc_drv *drv)
{
	int rc;

	mutex_init(&ipc->lock);
	ipc->last = 1;
	ipc->index = index;

	index *=2;

	if ((index >= IPC_MANY_WRITERS_INDEX_BASE) && (index < (IPC_MANY_WRITERS_INDEX_BASE + IPC_MANY_WRITERS_MAX))) {
		ipc->type = IPC_TYPE_MANY_WRITERS;
	} else if (index >= IPC_MANY_READERS_INDEX_BASE && (index < (IPC_MANY_READERS_INDEX_BASE + IPC_MANY_READERS_MAX))) {
		ipc->type = IPC_TYPE_MANY_READERS;
	} else if (index >= IPC_SINGLE_READER_WRITER_INDEX_BASE && (index < (IPC_SINGLE_READER_WRITER_INDEX_BASE + IPC_SINGLE_READER_WRITER_MAX))){
		ipc->type = IPC_TYPE_SINGLE_READER_WRITER;
	} else {
		rc = -ENODEV;
		goto err;
	}

	return 0;

err:
	return rc;
}

static void ipc_channel_exit(struct ipc_channel *ipc)
{
}

int ipcdrv_init(struct ipc_drv *drv)
{
	int rc;
	int i;

	pr_info("%s: %p\n", __func__, drv);

	for (i = 0; i < IPCDRV_IPC_MAX; i++) {
		struct ipc_channel *ipc = &drv->ipc[i];

		rc = ipc_channel_init(ipc, i, drv);
		if (rc < 0)
			goto err_ipc_init;
	}

	rc = alloc_chrdev_region(&drv->devno, IPCDRV_MINOR, IPCDRV_MINOR_COUNT, IPCDRV_NAME);
	if (rc < 0) {
		pr_err("%s: alloc_chrdev_region() failed\n", __func__);
		goto err_alloc_chrdev;
	}

	cdev_init(&drv->cdev, &ipcdrv_fops);
	drv->cdev.owner = THIS_MODULE;

	rc = cdev_add(&drv->cdev, drv->devno, IPCDRV_MINOR_COUNT);
	if (rc < 0) {
		pr_err("%s: cdev_add() failed\n", __func__);
		goto err_cdev_add;
	}

	return 0;

err_cdev_add:
	unregister_chrdev_region(drv->devno, IPCDRV_MINOR_COUNT);

err_alloc_chrdev:
err_ipc_init:
	while (i--) {
		struct ipc_channel *ipc = &drv->ipc[i];

		ipc_channel_exit(ipc);
	}

	return rc;
}

void ipcdrv_exit(struct ipc_drv *drv)
{
	int i;

	pr_info("%s: %p\n", __func__, drv);

	cdev_del(&drv->cdev);
	unregister_chrdev_region(drv->devno, IPCDRV_MINOR_COUNT);

	for (i = 0; i < IPCDRV_IPC_MAX; i++) {
		struct ipc_channel *ipc = &drv->ipc[i];

		ipc_channel_exit(ipc);
	}
}
