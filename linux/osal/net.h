/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Linux specific NET implementation
 @details
*/

#ifndef _LINUX_OSAL_NET_H_
#define _LINUX_OSAL_NET_H_

#include "common/net_types.h"
#include "epoll.h"
#include "os/clock.h"
#include "os/string.h"
#include "common/ipc.h"

#define DEFAULT_NET_DATA_SIZE 1600

struct net_rx;
struct net_tx;

struct net_tx_ops_cb {
	int (*net_tx_init)(struct net_tx *, struct net_address *);
	void (*net_tx_exit)(struct net_tx *);

	int (*net_tx)(struct net_tx *, struct net_tx_desc *);
	int (*net_tx_multi)(struct net_tx *, struct net_tx_desc **, unsigned int);

	struct net_tx_desc * (*net_tx_alloc)(unsigned int);
	int (*net_tx_alloc_multi)(struct net_tx_desc **, unsigned int, unsigned int);
	struct net_tx_desc * (*net_tx_clone)(struct net_tx_desc *);

	int (*net_tx_ts_get)(struct net_tx *, uint64_t *, unsigned int *);
	int (*net_tx_ts_init)(struct net_tx *, struct net_address *, void (*func)(struct net_tx *, uint64_t, unsigned int), unsigned long);
	int (*net_tx_ts_exit)(struct net_tx *);

	unsigned int (*net_tx_available)(struct net_tx *);
	int (*net_port_status)(struct net_tx *, unsigned int, bool *, bool *, unsigned int *);
};

struct net_rx_ops_cb {
	int (*net_rx_init)(struct net_rx *, struct net_address *, void (*func)(struct net_rx *, struct net_rx_desc *), unsigned long);
	int (*net_rx_init_multi)(struct net_rx *, struct net_address *, void (*func)(struct net_rx *, struct net_rx_desc **, unsigned int), unsigned int, unsigned int, unsigned long);

	void (*net_rx_exit)(struct net_rx *);
	struct net_rx_desc * (*__net_rx)(struct net_rx *);
	void (*net_rx)(struct net_rx *);
	void (*net_rx_multi)(struct net_rx *);

	int (*net_add_multi)(struct net_rx *, unsigned int, const unsigned char *);
	int (*net_del_multi)(struct net_rx *, unsigned int, const unsigned char *);
};

struct net_tx {
	int fd;
	int epoll_fd;
	int port_id;
	void (*func_tx_ts)(struct net_tx *, uint64_t ts, unsigned int private);
	struct linux_epoll_data epoll_data;
	u8 eth_src[6];
	os_clock_id_t clock_domain; /* clock domain to which hw timestamps must be converted */
	void *priv;
	unsigned int pool_type;
	struct net_tx_ops_cb net_ops;
	struct ipc_tx ipc_tx; /* used for sending network buffers over IPC */
};

struct net_rx {
	int fd;
	int epoll_fd;
	int port_id;
	void (*func)(struct net_rx *, struct net_rx_desc *);
	void (*func_multi)(struct net_rx *, struct net_rx_desc **, unsigned int);
	struct linux_epoll_data epoll_data;
	unsigned int batch;
	bool is_ptp;
	os_clock_id_t clock_domain; /* clock domain to which hw timestamps must be converted */
	void *priv;
	unsigned int pool_type;
	struct net_rx_ops_cb net_ops;
	struct ipc_rx ipc_rx; /* used for sending network buffers over IPC */
};

struct net_mem_ops_cb {
	void (*net_tx_free)(struct net_tx_desc *);

	void (*net_rx_free)(struct net_rx_desc *);

	void (*net_free_multi)(void **, unsigned int);
};

#endif /* _LINUX_OSAL_NET_H_ */
