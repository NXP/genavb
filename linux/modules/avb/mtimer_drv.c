/*
 * AVB media clock timer driver
 * Copyright 2019, 2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/ioctl.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <asm/uaccess.h>

#include "avbdrv.h"
#include "mtimer_drv.h"
#include "mtimer.h"

static long mtimer_drv_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct mtimer_dev *dev = file->private_data;
	struct mtimer_start start;
	int rc = 0;

	switch (cmd) {
	case MTIMER_IOC_START:
		if (copy_from_user(&start, (void __user *)arg, sizeof(start))) {
			rc = -EFAULT;
			break;
		}

		rc = mclock_timer_start(dev, &start);

		break;

	case MTIMER_IOC_STOP:
		mclock_timer_stop(dev);

		break;

	default:
		rc = -EINVAL;
		break;
	}

	return rc;
}

static unsigned int mtimer_drv_poll(struct file *file, poll_table *poll)
{
	struct mtimer_dev *dev = file->private_data;
	unsigned int mask = 0;

	poll_wait(file, &dev->wait, poll);

	if (atomic_read(&dev->wake_up)) {
		atomic_dec(&dev->wake_up);
		mask |= (POLLIN | POLLRDNORM);
	}

	return mask;
}

static int mtimer_drv_open(struct inode *in, struct file *file)
{
	struct mtimer_dev *dev;
	mclock_t type;
	int minor = MINOR(in->i_rdev);
	int port = 0;
	int domain;
	int rc = 0;

	//pr_info("%s: in: %p, file: %p, minor: %d\n", __func__, in, file, minor);

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
		goto err_minor;
	}

	dev = mtimer_open(type, domain);
	if (!dev) {
		rc = -ENOMEM;
		goto err_open;
	}

	file->private_data = dev;

	return rc;

err_open:
err_minor:
	return rc;
}

static int mtimer_drv_release(struct inode *in, struct file *file)
{
	struct mtimer_dev *dev = file->private_data;

	//pr_err("%s: in: %p, file: %p\n", __func__, in, file);

	mtimer_release(dev);

	file->private_data = NULL;

	return 0;
}


static const struct file_operations mtimer_fops = {
	.owner = THIS_MODULE,
	.open = mtimer_drv_open,
	.release = mtimer_drv_release,
	.unlocked_ioctl = mtimer_drv_ioctl,
	.poll = mtimer_drv_poll,
};

int mtimer_drv_init(struct mtimer_drv *drv, struct dentry *avb_dentry)
{
	int ret;

	ret = alloc_chrdev_region(&drv->devno, MCLOCK_MINOR, MCLOCK_MINOR_COUNT, MTIMER_NAME);
	if (ret < 0) {
		pr_err("%s: alloc_chrdev_region() failed\n", __func__);
		goto err_alloc_chrdev;
	}

	cdev_init(&drv->cdev, &mtimer_fops);
	drv->cdev.owner = THIS_MODULE;

	ret = cdev_add(&drv->cdev, drv->devno, MCLOCK_MINOR_COUNT);
	if (ret < 0) {
		pr_err("%s: cdev_add() failed\n", __func__);
		goto err_cdev_add;
	}

//	drv->mclock_dentry = mclock_debugfs_init(avb_dentry);

	pr_info("%s: %p\n", __func__, drv);

	return ret;

err_cdev_add:
	unregister_chrdev_region(drv->devno, MCLOCK_MINOR_COUNT);

err_alloc_chrdev:
	return ret;
}

void mtimer_drv_exit(struct mtimer_drv *drv)
{
	cdev_del(&drv->cdev);
	unregister_chrdev_region(drv->devno, MCLOCK_MINOR_COUNT);
}
