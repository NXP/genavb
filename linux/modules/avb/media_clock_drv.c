/*
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
 * Copyright 2018-2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/ioctl.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <asm/uaccess.h>

#include "avbdrv.h"
#include "hw_timer.h"
#include "debugfs.h"

#include "media_clock_drv.h"

struct mclock_drv *mclk_drv;

struct mclock_ext_dev * mclock_drv_bind_ext_device(struct device_node *of_node)
{
	struct mclock_drv *drv = mclk_drv;
	struct mclock_ext_dev *ext_dev;
	unsigned long flags;

	raw_spin_lock_irqsave(&drv->lock, flags);

	list_for_each_entry(ext_dev, &drv->mclock_ext_devices, list) {
		if ((of_node == ext_dev->dev->of_node)
		&& (ext_dev->flags & MCLOCK_FLAGS_FREE)) {
			ext_dev->flags &= ~MCLOCK_FLAGS_FREE;
			goto out;
		}
	}

	/* Not found */
	ext_dev = NULL;

out:
	raw_spin_unlock_irqrestore(&drv->lock, flags);

	return ext_dev;
}

void mclock_drv_unbind_ext_device(struct mclock_ext_dev *ext_dev)
{
	struct mclock_drv *drv = mclk_drv;
	unsigned long flags;

	raw_spin_lock_irqsave(&drv->lock, flags);

	ext_dev->flags |= MCLOCK_FLAGS_FREE;

	raw_spin_unlock_irqrestore(&drv->lock, flags);
}

void mclock_drv_register_ext_device(struct mclock_ext_dev *ext_dev)
{
	struct mclock_drv *drv = mclk_drv;
	unsigned long flags;

	raw_spin_lock_irqsave(&drv->lock, flags);

	list_add(&ext_dev->list, &drv->mclock_ext_devices);
	ext_dev->flags = MCLOCK_FLAGS_FREE | MCLOCK_FLAGS_REGISTERED;

	raw_spin_unlock_irqrestore(&drv->lock, flags);

	pr_info("%s: dev(%p)\n", __func__, ext_dev);
}

void mclock_drv_unregister_ext_device(struct mclock_ext_dev *ext_dev)
{
	struct mclock_drv *drv = mclk_drv;
	unsigned long flags;

	raw_spin_lock_irqsave(&drv->lock, flags);

	if (ext_dev->flags & MCLOCK_FLAGS_REGISTERED) {
		list_del(&ext_dev->list);
		ext_dev->flags = 0;
	}

	raw_spin_unlock_irqrestore(&drv->lock, flags);

	pr_info("%s: dev(%p)\n", __func__, ext_dev);
}

void mclock_drv_register_device(struct mclock_dev *dev)
{
	struct mclock_drv *drv = mclk_drv;
	unsigned long flags;

	raw_spin_lock_irqsave(&drv->lock, flags);

	list_add(&dev->list, &drv->mclock_devices);

	INIT_LIST_HEAD(&dev->mtimer_devices);

	dev->drv = drv;
	dev->flags = MCLOCK_FLAGS_FREE | MCLOCK_FLAGS_REGISTERED;

	raw_spin_unlock_irqrestore(&drv->lock, flags);

	pr_info("%s: dev(%p)\n", __func__, dev);
}

int mclock_drv_unregister_device(struct mclock_dev *dev)
{
	struct mclock_drv *drv = mclk_drv;
	unsigned long flags;
	int rc = -ENODEV;

	/* Make sure the device is no longer used.
	 * For now the init/exit order of the different drivers guarantees it. */

	raw_spin_lock_irqsave(&drv->lock, flags);

	if (dev->flags & MCLOCK_FLAGS_REGISTERED) {
		list_del(&dev->list);
		dev->flags = 0;
		rc = 0;
	}

	raw_spin_unlock_irqrestore(&drv->lock, flags);

	return rc;
}

int mclock_drv_register_timer(struct mclock_dev *dev, int (*irq_func)(struct mclock_dev *dev, void *, unsigned int), unsigned int period)
{
	int i, rc = -ENOMEM;
	unsigned long flags;
	struct mclock_drv *drv = dev->drv;

	raw_spin_lock_irqsave(&drv->lock, flags);

	for (i = 0; i < MCLOCK_TIMER_MAX; i++) {
		if (drv->isr[i].flags & MCLOCK_TIMER_ACTIVE)
			continue;
		else {
			drv->isr[i].irq_func = irq_func;
			drv->isr[i].dev = dev;
			drv->isr[i].flags |= MCLOCK_TIMER_ACTIVE;
			drv->isr[i].period = period;
			drv->isr[i].count = period;
			dev->id = i;
			rc = 0;
			goto exit;
		}
	}
exit:
	raw_spin_unlock_irqrestore(&drv->lock, flags);

	return rc;
}

void mclock_drv_unregister_timer(struct mclock_dev *dev)
{
	unsigned long flags;
	struct mclock_drv *drv = dev->drv;

	raw_spin_lock_irqsave(&drv->lock, flags);

	drv->isr[dev->id].flags &= (~MCLOCK_TIMER_ACTIVE);

	raw_spin_unlock_irqrestore(&drv->lock, flags);
}

static void mclock_drv_vma_open(struct vm_area_struct *vma)
{
	//pr_err("%s: start: %lx, end: %lx, offset: %lx, flags: %lx\n", __func__,
	//	vma->vm_start, vma->vm_end, vma->vm_pgoff, vma->vm_flags);
}

static void mclock_drv_vma_close(struct vm_area_struct *vma)
{
	//pr_err("%s: start: %lx, end: %lx, offset: %lx, flags: %lx\n", __func__,
	//	vma->vm_start, vma->vm_end, vma->vm_pgoff, vma->vm_flags);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,11,0)
static int mclock_drv_vma_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
#elif LINUX_VERSION_CODE < KERNEL_VERSION(4,17,0)
static int mclock_drv_vma_fault(struct vm_fault *vmf)
#else
static vm_fault_t mclock_drv_vma_fault(struct vm_fault *vmf)
#endif
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,11,0)
	struct mclock_dev *dev = vma->vm_private_data;
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
#else
	struct mclock_dev *dev = vmf->vma->vm_private_data;
	unsigned long offset = vmf->vma->vm_pgoff << PAGE_SHIFT;
#endif
	void *addr = NULL;
	struct page *page;

	//pr_err("%s: addr: %p, pgoff: %lx, flags: %x\n", __func__, vmf->virtual_address,
	//	vmf->pgoff, vmf->flags);

	if (offset >= PAGE_SIZE)
		return VM_FAULT_SIGBUS;

	if (!dev->sh_mem)
		return VM_FAULT_SIGBUS;

	/*
	 * If the shared memory has been allocated in the DMA zone, it should
	 * have entirely remmaped in mmap and no fault should happen.
	 */
	if (!dev->sh_mem_dma)
		addr = dev->sh_mem + offset;
	else
		return VM_FAULT_SIGBUS;

	page = virt_to_page(addr);
	get_page(page);
	vmf->page = page;

	return 0;
}

static const struct vm_operations_struct mclock_mem_ops = {
	.open = mclock_drv_vma_open,
	.close = mclock_drv_vma_close,
	.fault = mclock_drv_vma_fault,
};

struct mclock_dev *__mclock_drv_find_device(mclock_t type, int domain)
{
	struct mclock_drv *drv = mclk_drv;
	struct mclock_dev *dev;

	list_for_each_entry(dev, &drv->mclock_devices, list) {
		if (!(dev->flags & MCLOCK_FLAGS_FREE) && (dev->type == type) && (dev->domain == domain))
			goto found;
	}

	return NULL;

found:
	return dev;
}

static struct mclock_dev * __mclock_drv_get_device(mclock_t type, int domain)
{
	struct mclock_drv *drv = mclk_drv;
	struct mclock_dev *dev;

	list_for_each_entry(dev, &drv->mclock_devices, list) {
		if ((dev->flags & MCLOCK_FLAGS_FREE) && (dev->type == type) && (dev->domain == domain)) {
			dev->flags &= ~MCLOCK_FLAGS_FREE;
			return dev;
		}
	}
	return NULL;
}

static void __mclock_drv_put_device(struct mclock_dev *dev)
{
	dev->flags |= MCLOCK_FLAGS_FREE;
}

static int mclock_drv_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct mclock_dev *dev = file->private_data;
	unsigned long size, offset;
	int rc = 0;

	//pr_err("%s: start: %lx, end: %lx, offset: %lx, flags: %lx, priv: %p\n",
	//	__func__, vma->vm_start, vma->vm_end, vma->vm_pgoff, vma->vm_flags, dev);

	if (vma->vm_end < vma->vm_start)
		return -EINVAL;

	size = vma->vm_end - vma->vm_start;
	offset = vma->vm_pgoff << PAGE_SHIFT;

	if (!dev->sh_mem)
		return -EINVAL;

	if (dev->sh_mem_dma) {
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

		/* Remap buffer allocated in DMA zone */
		rc = remap_pfn_range(vma, vma->vm_start, (dev->sh_mem_dma + offset) >> PAGE_SHIFT, size, vma->vm_page_prot);
		if (rc < 0) {
			pr_err("%s: remap_pfn_range() failed\n", __func__);
			goto exit;
		}
	}
	else {
		//vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
		vma->vm_ops = &mclock_mem_ops;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
	vma->vm_flags |= VM_RESERVED;
#endif
	vma->vm_private_data = dev;
exit:
	return rc;
}

static long mclock_drv_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct mclock_dev *dev = file->private_data;
	int rc = 0;

	switch (cmd) {
	case MCLOCK_IOC_RESET:

		rc = mclock_reset(dev);

		break;

	case MCLOCK_IOC_START:
		{
			struct mclock_start start;

			if (copy_from_user(&start, (void __user *)arg, sizeof(start))) {
				rc = -EFAULT;
				break;
			}

			rc = mclock_start(dev, &start);
		}

		break;

	case MCLOCK_IOC_STOP:

		rc = mclock_stop(dev);

		break;

	case MCLOCK_IOC_CLEAN:
		{
			struct mclock_clean clean;

			rc = mclock_clean(dev, &clean);

			if (rc < 0)
				break;

			rc = copy_to_user((void __user *)arg, &clean, sizeof(clean));

		}
		break;

	case MCLOCK_IOC_SCONFIG:
		{
			struct mclock_sconfig cfg;

			if (copy_from_user(&cfg, (void __user *)arg, sizeof(cfg))) {
				rc = -EFAULT;
				break;
			}

			rc = mclock_sconfig(dev, &cfg);

		}

		break;

	case MCLOCK_IOC_GCONFIG:
		{
			struct mclock_gconfig cfg;

			mclock_gconfig(dev, &cfg);

			rc = copy_to_user((void __user *)arg, &cfg, sizeof(cfg));
		}

		break;

	default:
		rc = -EINVAL;
		break;
	}

	return rc;
}

static int mclock_drv_open(struct inode *in, struct file *file)
{
	struct mclock_drv *drv = container_of(in->i_cdev, struct mclock_drv, cdev);
	struct mclock_dev *dev;
	int rc = 0;
	mclock_t type;
	int minor = MINOR(in->i_rdev);
	unsigned long flags;
	int port = 0;
	int domain;

	//pr_info("%s: in: %p, file: %p, minor: %d\n", __func__, in, file, minor);

	raw_spin_lock_irqsave(&drv->lock, flags);

	if ((1 << minor) & drv->users) {
		rc = -EBUSY;
		goto err_unlock;
	}

	if (minor < (MCLOCK_REC_MINOR + MCLOCK_DOMAIN_RANGE)) {
		type = REC;
		domain = minor;
	}
	else if (minor < (MCLOCK_GEN_MINOR + MCLOCK_DOMAIN_RANGE)) {
		type = GEN;
		domain = minor - MCLOCK_GEN_MINOR;
	}
	else if (minor < (MCLOCK_PTP_MINOR + MCLOCK_DOMAIN_PTP_RANGE)) {
		if (minor < (MCLOCK_PTP_MINOR + MCLOCK_DOMAIN_PTP_PORT_RANGE))
			port = 0;
		else
			port = 1;

		type = PTP;
		domain = 0; //not attached to any physical domain
	}
	/* Invalid minor */
	else {
		rc = -EINVAL;
		goto err_unlock;
	}

	dev = __mclock_drv_get_device(type, domain);
	if (!dev) {
		rc = -ENODEV;
		goto err_unlock;
	}

	file->private_data = dev;
	drv->users |= (1 << minor);

	raw_spin_unlock_irqrestore(&drv->lock, flags);

	rc = mclock_open(dev, port);
	if (rc < 0)
		goto err_open;

	return rc;

err_open:
	raw_spin_lock_irqsave(&drv->lock, flags);
	file->private_data = NULL;
	__mclock_drv_put_device(dev);
	drv->users &= ~(1 << minor);
err_unlock:
	raw_spin_unlock_irqrestore(&drv->lock, flags);
	return rc;
}

static int mclock_drv_release(struct inode *in, struct file *file)
{
	struct mclock_drv *drv = container_of(in->i_cdev, struct mclock_drv, cdev);
	struct mclock_dev *dev = file->private_data;
	int minor = MINOR(in->i_rdev);
	unsigned long flags;

	//pr_err("%s: in: %p, file: %p\n", __func__, in, file);

	mclock_release(dev);

	file->private_data = NULL;

	raw_spin_lock_irqsave(&drv->lock, flags);

	__mclock_drv_put_device(dev);
	drv->users &= ~(1 << minor);

	raw_spin_unlock_irqrestore(&drv->lock, flags);

	return 0;
}

static int mclock_drv_register_gen_ptp(struct mclock_drv *drv)
{
	int ret, i, j;

	for (i = 0; i < MCLOCK_DOMAIN_PTP_RANGE; i++) {
		ret = mclock_gen_ptp_init(&drv->gen_ptp[i]);
		if (ret < 0) {
			pr_err("%s: mclock_gen_ptp_init() failed for PTP domain %d \n", __func__, i);
			goto err;
		}
	}

	return 0;
err:
	for (j = 0; j < i; j++)
		mclock_gen_ptp_exit(&drv->gen_ptp[j]);

	return ret;
}

static const struct file_operations mclock_fops = {
	.owner = THIS_MODULE,
	.open = mclock_drv_open,
	.release = mclock_drv_release,
	.mmap = mclock_drv_mmap,
	.unlocked_ioctl = mclock_drv_ioctl,
};

void mclock_drv_thread(struct mclock_drv *drv)
{
	struct mclock_dev *dev;
	struct mtimer_dev *mtimer_dev[MTIMER_MAX];
	unsigned int i, n = 0;
	unsigned long flags;

	raw_spin_lock_irqsave(&drv->lock, flags);

	list_for_each_entry(dev, &drv->mclock_devices, list) {
		mclock_wake_up_thread(dev, mtimer_dev, &n);
	}

	raw_spin_unlock_irqrestore(&drv->lock, flags);

	for (i = 0; i < n; i++) {
		wake_up(&mtimer_dev[i]->wait);
		clear_bit(MTIMER_ATOMIC_FLAGS_BUSY, &mtimer_dev[i]->atomic_flags);
	}
}

int mclock_drv_interrupt(struct mclock_drv *drv, void *data, unsigned int hw_ticks)
{
	int i, rc = 0;
	struct mclock_timer *timer;
	struct mclock_dev *dev;
	unsigned int timer_ticks, hw_ticks_now;
	unsigned long flags;

	raw_spin_lock_irqsave(&drv->lock, flags);

	for (i = 0; i < MCLOCK_TIMER_MAX; i++) {
		timer = &drv->isr[i];

		if (!(timer->flags & MCLOCK_TIMER_ACTIVE))
			continue;

		dev = timer->dev;

		timer_ticks = 0;
		hw_ticks_now = hw_ticks;

		/* Determine how many timer ticks have elapsed, based on hw_ticks and timer period */
		while (hw_ticks_now--) {
			if (!(--timer->count)) {
				timer_ticks++;
				timer->count = timer->period;
			}
		}

		if (timer_ticks) {
			timer->irq_func(dev, data, timer_ticks);

			if (dev->flags & MCLOCK_FLAGS_WAKE_UP)
				rc = mclock_wake_up(dev, dev->clk_timer);
		}
	}

	raw_spin_unlock_irqrestore(&drv->lock, flags);

	return rc;
}

int mclock_drv_init(struct mclock_drv *drv, struct dentry *avb_dentry)
{
	int ret;

	mclk_drv = drv;
	raw_spin_lock_init(&drv->lock);
	INIT_LIST_HEAD(&drv->mclock_devices);
	INIT_LIST_HEAD(&drv->mclock_ext_devices);

	/* Register PTP generations */
	ret = mclock_drv_register_gen_ptp(drv);
	if (ret < 0)
		goto err_gen_ptp_init;

	ret = alloc_chrdev_region(&drv->devno, MCLOCK_MINOR, MCLOCK_MINOR_COUNT, MCLOCK_NAME);
	if (ret < 0) {
		pr_err("%s: alloc_chrdev_region() failed\n", __func__);
		goto err_alloc_chrdev;
	}

	cdev_init(&drv->cdev, &mclock_fops);
	drv->cdev.owner = THIS_MODULE;

	ret = cdev_add(&drv->cdev, drv->devno, MCLOCK_MINOR_COUNT);
	if (ret < 0) {
		pr_err("%s: cdev_add() failed\n", __func__);
		goto err_cdev_add;
	}

	drv->mclock_dentry = mclock_debugfs_init(avb_dentry);

	pr_info("%s: %p\n", __func__, drv);

	return ret;

err_cdev_add:
	unregister_chrdev_region(drv->devno, MCLOCK_MINOR_COUNT);

err_alloc_chrdev:
err_gen_ptp_init:
	return ret;
}

void mclock_drv_exit(struct mclock_drv *drv)
{
	int i;

	cdev_del(&drv->cdev);
	unregister_chrdev_region(drv->devno, MCLOCK_MINOR_COUNT);

	for (i = 0; i < MCLOCK_DOMAIN_PTP_RANGE; i++)
		mclock_gen_ptp_exit(&drv->gen_ptp[i]);

	mclk_drv = NULL;
}
