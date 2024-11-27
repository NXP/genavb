/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/poll.h>
#include <linux/version.h>
#include <linux/of_platform.h>

#include "avbdrv.h"
#include "queue.h"
#include "pool.h"
#include "net_port.h"
#include "epit.h"
#include "debugfs.h"
#include "hw_timer.h"
#include "gpt.h"
#include "tpm.h"

#define AVB_READ_MAX_BUFFERS	32
#define AVB_WRITE_BATCH		16


/* TODO Move all the shared memory related code to a new shmem.c */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,11,0)
static int avbdrv_vma_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
#elif LINUX_VERSION_CODE < KERNEL_VERSION(4,17,0)
static int avbdrv_vma_fault(struct vm_fault *vmf)
#else
static vm_fault_t avbdrv_vma_fault(struct vm_fault *vmf)
#endif
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,11,0)
	struct avb_usr *usr = vma->vm_private_data;
#else
	struct avb_usr *usr = vmf->vma->vm_private_data;
#endif
	struct page *page;
	unsigned long offset;
	void *addr;

//	pr_info("%s: addr: %p, pgoff: %lx, flags: %x\n", __func__, vmf->virtual_address, vmf->pgoff, vmf->flags);

	offset = vmf->pgoff << PAGE_SHIFT;

	if (offset < BUF_POOL_SIZE)
		addr = pool_dma_shmem_to_virt(&usr->avb->buf_pool, offset);
	else
	/* IO memory is mapped statically, so this should not happen */
		return VM_FAULT_SIGBUS;

	page = virt_to_page(addr);
	get_page(page);

	vmf->page = page;

	return 0;
}

static void avbdrv_vma_open(struct vm_area_struct *vma)
{
	//pr_info("%s: start: %lx, end: %lx, offset: %lx, flags: %lx\n", __func__, vma->vm_start, vma->vm_end, vma->vm_pgoff, vma->vm_flags);
}

static void avbdrv_vma_close(struct vm_area_struct *vma)
{
	//pr_info("%s: start: %lx, end: %lx, offset: %lx, flags: %lx\n", __func__, vma->vm_start, vma->vm_end, vma->vm_pgoff, vma->vm_flags);
}

static const struct vm_operations_struct avbdrv_mem_ops = {
	.open = avbdrv_vma_open,
	.close = avbdrv_vma_close,
	.fault = avbdrv_vma_fault,
};

static int avbdrv_mmap(struct file *file, struct vm_area_struct *vma)
{
	unsigned long size, offset;

	//pr_info("%s: start: %lx, end: %lx, offset: %lx, flags: %lx\n", __func__, vma->vm_start, vma->vm_end, vma->vm_pgoff, vma->vm_flags);

	if (vma->vm_end < vma->vm_start)
		return -EINVAL;

	size = vma->vm_end - vma->vm_start;
	offset = vma->vm_pgoff << PAGE_SHIFT;

	/* Each area with different cache settings must use a different VMA */

	if (offset < BUF_POOL_SIZE) {
		//pr_info("%s: mapping ddr [%lx:%lx]\n", __func__, offset, offset + size - 1);

		if ((offset + size) > BUF_POOL_SIZE) {
			pr_err("%s: invalid range [%lx:%lx]\n", __func__, offset, offset + size -1);
			return -EINVAL;
		}

		/* Mapping is done dynamically in fault handler */

	}
#if 0
	else if (offset < (SHMEM_MEMORY_SIZE + SHMEM_IO_SIZE)) {
		pr_info("%s: mapping io [%lx:%lx]\n", __func__, offset, offset + size - 1);

		if ((offset + size) > (SHMEM_MEMORY_SIZE + SHMEM_IO_SIZE)) {
			pr_err("%s: invalid range [%lx:%lx]\n", __func__, offset, offset + size -1);
			return -EINVAL;
		}

		offset -= SHMEM_MEMORY_SIZE;

		vma->vm_flags |= VM_IO;
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

		/* Map IO (using a fixed mapping) above shared DDR */
		rc = remap_pfn_range(vma, vma->vm_start, (TIMER_BASEADDR + offset) >> PAGE_SHIFT, size, vma->vm_page_prot);
		if (rc < 0) {
			pr_err("%s: remap_pfn_range() failed\n", __func__);
			return -EINVAL;
		}
	}
#endif
	else {
		pr_err("%s: invalid range [%lx:%lx]\n", __func__, offset, offset + size -1);
		return -EINVAL;
	}

	vma->vm_ops = &avbdrv_mem_ops;
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
	vma->vm_flags |= VM_RESERVED;
#endif
	vma->vm_private_data = file->private_data;

	return 0;
}

/**
 * avbdrv_write() - frees an array of shared memory buffers
 * @buf: pointer to array containing shared memory buffer addresses
 * @len: length of the array (in bytes)
 * @off: pointer to file offset
 *
 * The buffer addresses in the array are shared memory relative addresses that need
 * to be translated to kernel virtual addresses.
 * The file offset (pointed to by @off) must always be zero (no support for lseek in the character device)
 *
 * Return: number of buffers freed times the size of a pointer
 */
static ssize_t avbdrv_write(struct file *file, const char __user *buf, size_t len, loff_t *off)
{
	struct avb_usr *usr = file->private_data;
	unsigned int n;
	unsigned long addr[AVB_WRITE_BATCH];
	unsigned int free = 0, n_now;

	if (*off)
		return -EPIPE;

	n = len / sizeof(unsigned long);

	while (free < n) {
		n_now = n - free;
		if (n_now > AVB_WRITE_BATCH)
			n_now = AVB_WRITE_BATCH;

		if (copy_from_user(addr, &((unsigned long *)buf)[free], n_now * sizeof(unsigned long)))
			return -EFAULT;

		pool_dma_free_array_shmem(&usr->avb->buf_pool, addr, n_now);

		free += n_now;
	}

	return free * sizeof(unsigned long);
}

/**
 * avbdrv_read() - allocates an array of shared memory buffers
 * @buf: pointer to array where allocated shared memory buffer addresses will be stored
 * @len: length of the array (in bytes)
 * @off: pointer to file offset
 *
 * The buffer addresses in the array are shared memory relative addresses that need
 * to be translated to user virtual addresses.
 * The file offset (pointed to by @off) must always be zero (no support for lseek in the character device)
 *
 * Return: number of buffers allocated times the size of a pointer
 */
static ssize_t avbdrv_read(struct file *file, char __user *buf, size_t len, loff_t *off)
{
	struct avb_usr *usr = file->private_data;
	ssize_t rc = 0;
	int n;
	unsigned long addr[AVB_READ_MAX_BUFFERS];

	if (*off)
		return -EPIPE;

	n = len / sizeof(unsigned long);

	/* The maximum number of buffers per read is capped to AVB_READ_MAX_BUFFERS*/
	if (n > AVB_READ_MAX_BUFFERS)
		n = AVB_READ_MAX_BUFFERS;

	rc = pool_dma_alloc_array_shmem(&usr->avb->buf_pool, addr, n);
	if (rc <= 0)
		return rc;

	if (copy_to_user(buf, addr, rc * sizeof(unsigned long))) {
		pool_dma_free_array_shmem(&usr->avb->buf_pool, addr, rc);
		return -EFAULT;
	}

	return rc * sizeof(unsigned long);

}

static int avbdrv_open(struct inode *in, struct file *file)
{
	struct avb_drv *avb = container_of(in->i_cdev, struct avb_drv, cdev);
	struct avb_usr *usr;
	int rc;

	//pr_info("%s: in: %p, file: %p\n", __func__, in, file);

	usr = kzalloc(sizeof(*usr), GFP_KERNEL);
	if (!usr) {
		pr_err("%s: kmalloc() failed\n", __func__);
		rc = -ENOMEM;
		goto err_kmalloc;
	}

	usr->avb = avb;
	file->private_data = usr;

	return 0;

err_kmalloc:
	return rc;
}

static int avbdrv_release(struct inode *in, struct file *file)
{
	struct avb_usr *usr = file->private_data;

	//pr_info("%s: in: %p, file: %p\n", __func__, in, file);

	kfree(usr);

	file->private_data = NULL;

	return 0;
}

static long avbdrv_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	unsigned long shmem_size;
	int rc = 0;

	switch (cmd) {

	case AVBDRV_IOC_SHMEM_SIZE:
		shmem_size = BUF_POOL_SIZE;

		rc = put_user(shmem_size, (unsigned long *)arg);

		break;

	default:
		rc = -EINVAL;
		break;
	}

	return rc;
}

/** shared memory implementation:
 * 1 buffer pool initialized on module init
 * user-space can access the pool with:
 * * read: allocate objects from the pool and retrieve object pointers
 * * write: release objects by writing object pointers to the file
 * * mmap: access the memory pointed to by the object pointers
 */
static const struct file_operations avbdrv_fops = {
	.owner = THIS_MODULE,
	.open = avbdrv_open,
	.release = avbdrv_release,
	.read = avbdrv_read,
	.write = avbdrv_write,
	.mmap = avbdrv_mmap,
	.unlocked_ioctl = avbdrv_ioctl,
};

static struct avb_drv *avb;

/**
 * avb_alloc_range() - allocates a physically continuous range of memory
 * @size: size of range to allocate in bytes
 *
 * The function creates a separate mapping for each page in the range, so
 * that they can be mmaped to userspace one page at a time.
 *
 * Return: pointer to memory range, or NULL in case of error.
 */
void *avb_alloc_range(unsigned int size)
{
	unsigned int order = get_order(size);
	void *baseaddr;
	struct page *page;
	int i;

	baseaddr = (void *)__get_free_pages(GFP_KERNEL | GFP_DMA32, order);
	if (!baseaddr) {
		pr_err("%s: get_free_pages() failed\n", __func__);
		return NULL;
	}

	/* Workaround to make mmap work with higher order pages, need to free one by one */
	page = virt_to_page(baseaddr);
	split_page(page, order);

	/* Now free excess pages */
	for (i = size / PAGE_SIZE; i < (1 << order); i++)
		free_page((unsigned long)baseaddr + i * PAGE_SIZE);

	return baseaddr;
}

/**
 * avb_free_range() - frees range of memory
 * @baseaddr: pointer to memory range (returned by @avb_alloc_range)
 * @size: size of allocated range in bytes
 *
 * The function frees a range of memory previously allocated by @avb_alloc_range.
 *
 */
void avb_free_range(void *baseaddr, unsigned int size)
{
	int i;

	for (i = 0; i < size / PAGE_SIZE; i++)
		free_page((unsigned long)baseaddr + i * PAGE_SIZE);
}

static int avbdrv_init(void)
{
	int rc;

	if (!is_gpt_hw_timer_available()
		&& !is_tpm_hw_timer_available()
		&& !is_epit_hw_timer_available()) {

		pr_err("%s: No HW timer is available in device tree\n", __func__);
		rc = -ENODEV;
		goto err_hw_timer;
	}

	avb = kzalloc(sizeof(struct avb_drv), GFP_KERNEL);
	if (!avb) {
		pr_err("%s: kmalloc() failed\n", __func__);
		rc = -ENOMEM;
		goto err_kmalloc;
	}

	avb->debugfs = avb_debugfs_init();
	if (!avb->debugfs) {
		pr_err("%s: avb_debugfs_init() failed\n", __func__);
		rc = -ENOMEM;
		goto err_debugfs;
	}

	avb->buf_baseaddr = avb_alloc_range(BUF_POOL_SIZE);
	if (!avb->buf_baseaddr) {
		pr_err("%s: avb_alloc_range() failed\n", __func__);
		rc = -ENOMEM;
		goto err_buf;
	}

	/* Network buffer memory pool */
	rc = pool_dma_init(&avb->buf_pool, avb->buf_baseaddr, BUF_POOL_SIZE, BUF_ORDER);
	if (rc < 0) {
		pr_err("%s: pool_init() failed\n", __func__);
		goto err_buf_pool;
	}

	rc = alloc_chrdev_region(&avb->devno, AVBDRV_MINOR, AVBDRV_MINOR_COUNT, AVBDRV_NAME);
	if (rc < 0) {
		pr_err("%s: alloc_chrdev_region() failed\n", __func__);
		goto err_alloc_chrdev;
	}

	cdev_init(&avb->cdev, &avbdrv_fops);
	avb->cdev.owner = THIS_MODULE;

	rc = cdev_add(&avb->cdev, avb->devno, AVBDRV_MINOR_COUNT);
	if (rc < 0) {
		pr_err("%s: cdev_add() failed\n", __func__);
		goto err_cdev_add;
	}

	rc = logical_port_init(avb);
	if (rc < 0) {
		pr_err("%s: logical_port_init() failed\n", __func__);
		goto err_logical_port;
	}

	rc = net_qos_init(&avb->qos, avb->debugfs);
	if (rc < 0) {
		pr_err("%s: net_qos_init() failed\n", __func__);
		goto err_net_qos;
	}

	rc = eth_avb_init(avb->eth, &avb->buf_pool, &avb->qos, avb->debugfs);
	if (rc < 0) {
		pr_err("%s: eth_avb_init() failed\n", __func__);
		goto err_eth_avb;
	}

	rc = netdrv_init(&avb->net_drv);
	if (rc < 0) {
		pr_err("%s: netdrv_init() failed\n", __func__);
		goto err_net_drv;
	}

	rc = media_drv_init(&avb->media_drv);
	if (rc < 0) {
		pr_err("%s: media_drv_init() failed\n", __func__);
		goto err_media_drv;
	}

	rc = mclock_drv_init(&avb->mclock_drv, avb->debugfs);
	if (rc < 0) {
		pr_err("%s: mclock_drv_init() failed\n", __func__);
		goto err_mclock_drv;
	}

	rc = mtimer_drv_init(&avb->mtimer_drv, avb->debugfs);
	if (rc < 0) {
		pr_err("%s: mtimer_drv_init() failed\n", __func__);
		goto err_mtimer_drv;
	}

	rc = hw_timer_init(&avb->timer, HW_TIMER_PERIOD_US, avb->debugfs);
	if (rc < 0) {
		pr_err("%s: hw_timer_init() failed\n", __func__);
		goto err_timer;
	}

	rc = epit_init(&avb->epit_driver);
	if (rc < 0) {
		pr_err("%s: epit_init() failed\n", __func__);
		goto err_epit;
	}

	rc = gpt_init(&avb->gpt_driver);
	if (rc < 0) {
		pr_err("%s: gpt_init() failed\n", __func__);
		goto err_gpt;
	}

	rc = tpm_init(&avb->tpm_driver);
	if (rc < 0) {
		pr_err("%s: tpm_init() failed\n", __func__);
		goto err_tpm;
	}

	return 0;

err_tpm:
	gpt_exit(&avb->gpt_driver);

err_gpt:
	epit_exit(&avb->epit_driver);

err_epit:
	hw_timer_exit(&avb->timer);

err_timer:
	mtimer_drv_exit(&avb->mtimer_drv);

err_mtimer_drv:
	mclock_drv_exit(&avb->mclock_drv);

err_mclock_drv:
	media_drv_exit(&avb->media_drv);

err_media_drv:
	netdrv_exit(&avb->net_drv);

err_net_drv:
	eth_avb_exit(avb->eth);

err_eth_avb:
	net_qos_exit(&avb->qos);

err_net_qos:
err_logical_port:
	cdev_del(&avb->cdev);

err_cdev_add:
	unregister_chrdev_region(avb->devno, AVBDRV_MINOR_COUNT);

err_alloc_chrdev:
	pool_dma_exit(&avb->buf_pool);

err_buf_pool:
	avb_free_range(avb->buf_baseaddr, BUF_POOL_SIZE);

err_buf:
	avb_debugfs_exit(avb->debugfs);

err_debugfs:
	kfree(avb);

err_kmalloc:
err_hw_timer:
	return rc;
}

static void avbdrv_exit(void)
{
	avb_debugfs_exit(avb->debugfs);

	tpm_exit(&avb->tpm_driver);

	gpt_exit(&avb->gpt_driver);

	epit_exit(&avb->epit_driver);

	hw_timer_exit(&avb->timer);

	mtimer_drv_exit(&avb->mtimer_drv);

	mclock_drv_exit(&avb->mclock_drv);

	media_drv_exit(&avb->media_drv);

	netdrv_exit(&avb->net_drv);

	eth_avb_exit(avb->eth);

	net_qos_exit(&avb->qos);

	cdev_del(&avb->cdev);
	unregister_chrdev_region(avb->devno, AVBDRV_MINOR_COUNT);

	pool_dma_exit(&avb->buf_pool);

	avb_free_range(avb->buf_baseaddr, BUF_POOL_SIZE);

	kfree(avb);
}

module_init(avbdrv_init);
module_exit(avbdrv_exit);

MODULE_LICENSE("GPL");
