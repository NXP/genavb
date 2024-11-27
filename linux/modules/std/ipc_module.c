/*
 * AVB ipc service driver
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
 * Copyright 2018-2020, 2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <linux/module.h>
#include <linux/slab.h>

#include "ipc.h"

struct ipc_module {
	struct ipc_drv ipc_drv;
};

static struct ipc_module *module;

static int ipc_module_init(void)
{
	int rc;

	module = kzalloc(sizeof(struct ipc_module), GFP_KERNEL);
	if (!module) {
		pr_err("%s: kmalloc() failed\n", __func__);
		rc = -ENOMEM;
		goto err_kmalloc;
	}

	rc = ipcdrv_init(&module->ipc_drv);
	if (rc < 0) {
		pr_err("%s: ipcdrv_init() failed\n", __func__);
		goto err_ipc_drv;
	}

	return 0;

err_ipc_drv:
	kfree(module);
	module = NULL;

err_kmalloc:
	return -1;
}

static void ipc_module_exit(void)
{
	ipcdrv_exit(&module->ipc_drv);

	kfree(module);
	module = NULL;
}

module_init(ipc_module_init);
module_exit(ipc_module_exit);

MODULE_LICENSE("GPL");
