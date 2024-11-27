/*
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
 * Copyright 2018-2019, 2022-2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _MCLOCK_DRV_H_
#define _MCLOCK_DRV_H_

#include "media_clock.h"

#ifdef __KERNEL__

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>

#include "media_clock_gen_ptp.h"

/* Media clock minors.
 * REC and GEN are HW based media clocks dependent on HW design. They are tight
 * to an audio domain and to an ethernet port (gPTP). Hence the port is defined
 * statically and provided via the device tree. For PTP clocks it is provided
 * using the MINOR.
 */

#define MCLOCK_NAME		"mclock"
#define MCLOCK_MINOR		0

#define MCLOCK_DOMAIN_RANGE		8
#define MCLOCK_DOMAIN_PTP_RANGE		2
#define MCLOCK_DOMAIN_PTP_PORT_RANGE	1

#define MCLOCK_REC_MINOR	0  						/*  0 - 7   Audio domain (0-7) */
#define MCLOCK_GEN_MINOR	(MCLOCK_REC_MINOR + MCLOCK_DOMAIN_RANGE)	/*  8 - 15  Audio domain (0-7) */
#define MCLOCK_PTP_MINOR	(MCLOCK_GEN_MINOR + MCLOCK_DOMAIN_RANGE)	/*  16 - 17 PTP sync domain */

#define MCLOCK_MINOR_COUNT	(MCLOCK_PTP_MINOR + MCLOCK_DOMAIN_PTP_RANGE)

#define MCLOCK_TIMER_MAX 	4

struct mclock_drv {
	struct cdev cdev;
	dev_t devno;
	struct mclock_gen_ptp gen_ptp[MCLOCK_DOMAIN_PTP_RANGE];
	struct mclock_timer isr[MCLOCK_TIMER_MAX];
	raw_spinlock_t lock;
	long long users;
	struct list_head mclock_devices;
	struct dentry *mclock_dentry;
	struct list_head mclock_ext_devices;
};

struct mclock_dev *__mclock_drv_find_device(mclock_t type, int domain);

struct mclock_ext_dev * mclock_drv_bind_ext_device(struct device_node *of_node);
void mclock_drv_unbind_ext_device(struct mclock_ext_dev *ext_dev);

void mclock_drv_register_ext_device(struct mclock_ext_dev *dev);
void mclock_drv_unregister_ext_device(struct mclock_ext_dev *ext_dev);

void mclock_drv_register_device(struct mclock_dev *dev);
int mclock_drv_unregister_device(struct mclock_dev *dev);

int mclock_drv_register_timer(struct mclock_dev *dev, int (*irq_func)(struct mclock_dev *, void *, unsigned int), unsigned int period);
void mclock_drv_unregister_timer(struct mclock_dev *dev);

void mclock_drv_thread(struct mclock_drv *drv);
int mclock_drv_interrupt(struct mclock_drv *drv, void *data, unsigned int ticks);

int mclock_drv_init(struct mclock_drv *drv, struct dentry *avb_dentry);
void mclock_drv_exit(struct mclock_drv *drv);

#endif /* __KERNEL__ */

#define MCLOCK_IOC_MAGIC	'c'

#define MCLOCK_IOC_RESET	_IO(MCLOCK_IOC_MAGIC, 0)
#define MCLOCK_IOC_START	_IOW(MCLOCK_IOC_MAGIC, 1, struct mclock_start)
#define MCLOCK_IOC_STOP		_IO(MCLOCK_IOC_MAGIC, 2)
#define MCLOCK_IOC_CLEAN	_IOR(MCLOCK_IOC_MAGIC, 3, unsigned int)
#define MCLOCK_IOC_SCONFIG	_IOW(MCLOCK_IOC_MAGIC, 4, struct mclock_sconfig)
#define MCLOCK_IOC_GCONFIG	_IOR(MCLOCK_IOC_MAGIC, 5, struct mclock_gconfig)

#endif /* _MCLOCK_DRV_H_ */
