/*
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
 * Copyright 2017-2020, 2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _NETDRV_H_
#define _NETDRV_H_

#ifdef __KERNEL__

#include <linux/cdev.h>

#define NETDRV_NAME		"netdrv"
#define NETDRV_MINOR		0
#define NETDRV_NET_RX_MINOR	0
#define NETDRV_NET_TX_MINOR	1
#define NETDRV_MINOR_COUNT	2

#define NET_PAYLOAD_SIZE_MAX		1600 /* Must be smaller than BUF_SIZE - NET_DATA_OFFSET */

struct net_drv {
	struct list_head batching_sync_list; /*Protected by ptype_lock*/
	struct list_head batching_async_list; /*Protected by ptype_lock*/
	struct list_head no_batching_list; /*Protected by ptype_lock*/
	struct cdev cdev;
	dev_t devno;
};

int netdrv_init(struct net_drv *drv);
void netdrv_exit(struct net_drv *drv);

#endif /* !__KERNEL__ */

struct net_set_option {
	unsigned int type;
	unsigned long val;
};

#define SOCKET_OPTION_BUFFER_PACKETS	0
#define SOCKET_OPTION_BUFFER_LATENCY	1

#define NET_IOC_MAGIC		'n'

#define NET_IOC_BIND		_IOW(NET_IOC_MAGIC, 1, struct net_address)
#define NET_IOC_CONNECT		_IOW(NET_IOC_MAGIC, 3, struct net_address)
#define NET_IOC_SR_CONFIG	_IOW(NET_IOC_MAGIC, 4, struct net_sr_config)
#define NET_IOC_TS_GET		_IOR(NET_IOC_MAGIC, 5, struct net_ts_desc)
#define NET_IOC_ADDMULTI	_IOW(NET_IOC_MAGIC, 6, struct net_mc_address)
#define NET_IOC_DELMULTI	_IOW(NET_IOC_MAGIC, 7, struct net_mc_address)
#define NET_IOC_GET_TX_AVAILABLE _IOR(NET_IOC_MAGIC, 8, unsigned int)
#define NET_IOC_SR_CLASS_CONFIG	_IOW(NET_IOC_MAGIC, 9, struct net_sr_class_cfg)
#define NET_IOC_PORT_STATUS	_IOWR(NET_IOC_MAGIC, 10, struct net_port_status)
#define NET_IOC_SET_OPTION	_IOW(NET_IOC_MAGIC, 11, struct net_set_option)


#endif /* _NETDRV_H_ */
