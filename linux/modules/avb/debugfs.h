/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2022-2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _DEBUGFS_AVB_H_
#define _DEBUGFS_AVB_H_

#include "media_clock_rec_pll.h"

struct dentry *avb_debugfs_init(void);
void avb_debugfs_exit(struct dentry *dentry);

void net_rx_debugfs_init(struct eth_avb *eth, struct dentry *avb_dentry);

void net_qos_debugfs_init(struct net_qos *net, struct dentry *avb_dentry);

struct dentry *mclock_debugfs_init(struct dentry *avb_dentry);

struct dentry *mclock_gen_ptp_debugfs_init(struct mclock_drv *drv, struct mclock_gen_ptp_stats *stats, int port);
void mclock_gen_ptp_debugfs_exit(struct dentry *mclock_dentry);
void mclock_rec_pll_debugfs_init(struct mclock_drv *drv, struct mclock_rec_pll *rec, int domain);

void hw_timer_debugfs_init(struct hw_timer *timer, struct dentry *avb_dentry);

#endif /* _DEBUGFS_AVB_H_ */
