/*
 * AVB media clock timer driver
 * Copyright 2019, 2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef _MTIMER_DRV_H_
#define _MTIMER_DRV_H_


#ifdef __KERNEL__

#include <linux/cdev.h>
#include <linux/device.h>

#define MTIMER_NAME		"mtimer"

struct mtimer_drv {
	struct cdev cdev;
	dev_t devno;
};

int mtimer_drv_init(struct mtimer_drv *drv, struct dentry *avb_dentry);
void mtimer_drv_exit(struct mtimer_drv *drv);

#endif /* __KERNEL__ */

#define MTIMER_IOC_MAGIC	't'

#define MTIMER_IOC_START	_IOW(MTIMER_IOC_MAGIC, 1, struct mtimer_start)
#define MTIMER_IOC_STOP		_IO(MTIMER_IOC_MAGIC, 2)

#endif /* _MTIMER_DRV_H_ */
