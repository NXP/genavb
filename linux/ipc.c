/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Linux specific IPC service implementation
 @details
*/

#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "common/log.h"
#include "common/ipc.h"

#include "epoll.h"

#include "modules/ipc.h"


#define static_assert(condition) extern char __CHECK__[1/(condition)];

static_assert(sizeof(struct ipc_desc) < IPC_BUF_SIZE);

/* Each IPC channel has two ends, each end is mapped to a specific device file */

#define IPC_RX	0
#define IPC_TX	1

static const char ipc_device[IPC_ID_MAX][2][64] = {
	[IPC_MEDIA_STACK_MSRP] = {
		[IPC_RX] = {"/dev/ipc_media_stack_msrp_rx"},
		[IPC_TX] = {"/dev/ipc_media_stack_msrp_tx"}
	},

	[IPC_MSRP_MEDIA_STACK] = {
		[IPC_RX] = {"/dev/ipc_msrp_media_stack_rx"},
		[IPC_TX] = {"/dev/ipc_msrp_media_stack_tx"}
	},

	[IPC_MSRP_MEDIA_STACK_SYNC] = {
		[IPC_RX] = {"/dev/ipc_msrp_media_stack_sync_rx"},
		[IPC_TX] = {"/dev/ipc_msrp_media_stack_sync_tx"}
	},

	[IPC_MEDIA_STACK_MSRP_1] = {
		[IPC_RX] = {"/dev/ipc_media_stack_msrp_1_rx"},
		[IPC_TX] = {"/dev/ipc_media_stack_msrp_1_tx"}
	},

	[IPC_MSRP_1_MEDIA_STACK] = {
		[IPC_RX] = {"/dev/ipc_msrp_1_media_stack_rx"},
		[IPC_TX] = {"/dev/ipc_msrp_1_media_stack_tx"}
	},

	[IPC_MSRP_1_MEDIA_STACK_SYNC] = {
		[IPC_RX] = {"/dev/ipc_msrp_1_media_stack_sync_rx"},
		[IPC_TX] = {"/dev/ipc_msrp_1_media_stack_sync_tx"}
	},

	[IPC_MEDIA_STACK_MVRP] = {
		[IPC_RX] = {"/dev/ipc_media_stack_mvrp_rx"},
		[IPC_TX] = {"/dev/ipc_media_stack_mvrp_tx"}
	},

	[IPC_MVRP_MEDIA_STACK] = {
		[IPC_RX] = {"/dev/ipc_mvrp_media_stack_rx"},
		[IPC_TX] = {"/dev/ipc_mvrp_media_stack_tx"}
	},

	[IPC_MVRP_MEDIA_STACK_SYNC] = {
		[IPC_RX] = {"/dev/ipc_mvrp_media_stack_sync_rx"},
		[IPC_TX] = {"/dev/ipc_mvrp_media_stack_sync_tx"}
	},

	[IPC_MEDIA_STACK_MVRP_1] = {
		[IPC_RX] = {"/dev/ipc_media_stack_mvrp_1_rx"},
		[IPC_TX] = {"/dev/ipc_media_stack_mvrp_1_tx"}
	},

	[IPC_MVRP_1_MEDIA_STACK] = {
		[IPC_RX] = {"/dev/ipc_mvrp_1_media_stack_rx"},
		[IPC_TX] = {"/dev/ipc_mvrp_1_media_stack_tx"}
	},

	[IPC_MVRP_1_MEDIA_STACK_SYNC] = {
		[IPC_RX] = {"/dev/ipc_mvrp_1_media_stack_sync_rx"},
		[IPC_TX] = {"/dev/ipc_mvrp_1_media_stack_sync_tx"}
	},

	[IPC_MEDIA_STACK_CLOCK_DOMAIN] = {
		[IPC_RX] = {"/dev/ipc_media_stack_clock_domain_rx"},
		[IPC_TX] = {"/dev/ipc_media_stack_clock_domain_tx"}
	},

	[IPC_CLOCK_DOMAIN_MEDIA_STACK] = {
		[IPC_RX] = {"/dev/ipc_clock_domain_media_stack_rx"},
		[IPC_TX] = {"/dev/ipc_clock_domain_media_stack_tx"}
	},

	[IPC_CLOCK_DOMAIN_MEDIA_STACK_SYNC] = {
		[IPC_RX] = {"/dev/ipc_clock_domain_media_stack_sync_rx"},
		[IPC_TX] = {"/dev/ipc_clock_domain_media_stack_sync_tx"}
	},

	[IPC_AVDECC_MEDIA_STACK] = {
		[IPC_RX] = {"/dev/ipc_avdecc_media_stack_rx"},
		[IPC_TX] = {"/dev/ipc_avdecc_media_stack_tx"}
	},

	[IPC_MEDIA_STACK_AVDECC] = {
		[IPC_RX] = {"/dev/ipc_media_stack_avdecc_rx"},
		[IPC_TX] = {"/dev/ipc_media_stack_avdecc_tx"}
	},

	[IPC_MEDIA_STACK_MAAP] = {
		[IPC_RX] = {"/dev/ipc_media_stack_maap_rx"},
		[IPC_TX] = {"/dev/ipc_media_stack_maap_tx"}
	},

	[IPC_MAAP_MEDIA_STACK] = {
		[IPC_RX] = {"/dev/ipc_maap_media_stack_rx"},
		[IPC_TX] = {"/dev/ipc_maap_media_stack_tx"}
	},

	[IPC_MAAP_MEDIA_STACK_SYNC] = {
		[IPC_RX] = {"/dev/ipc_maap_media_stack_sync_rx"},
		[IPC_TX] = {"/dev/ipc_maap_media_stack_sync_tx"}
	},

	[IPC_AVDECC_CONTROLLED] = {
		[IPC_RX] = {"/dev/ipc_avdecc_controlled_rx"},
		[IPC_TX] = {"/dev/ipc_avdecc_controlled_tx"}
	},

	[IPC_CONTROLLED_AVDECC] = {
		[IPC_RX] = {"/dev/ipc_controlled_avdecc_rx"},
		[IPC_TX] = {"/dev/ipc_controlled_avdecc_tx"}
	},

	[IPC_AVDECC_CONTROLLER] = {
		[IPC_RX] = {"/dev/ipc_avdecc_controller_rx"},
		[IPC_TX] = {"/dev/ipc_avdecc_controller_tx"}
	},

	[IPC_CONTROLLER_AVDECC] = {
		[IPC_RX] = {"/dev/ipc_controller_avdecc_rx"},
		[IPC_TX] = {"/dev/ipc_controller_avdecc_tx"},
	},

	[IPC_AVDECC_CONTROLLER_SYNC] = {
		[IPC_RX] = {"/dev/ipc_avdecc_controller_sync_rx"},
		[IPC_TX] = {"/dev/ipc_avdecc_controller_sync_tx"}
	},

	[IPC_MEDIA_STACK_GPTP] = {
		[IPC_RX] = {"/dev/ipc_media_stack_gptp_rx"},
		[IPC_TX] = {"/dev/ipc_media_stack_gptp_tx"}
	},

	[IPC_GPTP_MEDIA_STACK] = {
		[IPC_RX] = {"/dev/ipc_gptp_media_stack_rx"},
		[IPC_TX] = {"/dev/ipc_gptp_media_stack_tx"}
	},

	[IPC_GPTP_MEDIA_STACK_SYNC] = {
		[IPC_RX] = {"/dev/ipc_gptp_media_stack_sync_rx"},
		[IPC_TX] = {"/dev/ipc_gptp_media_stack_sync_tx"}
	},

	[IPC_MEDIA_STACK_GPTP_1] = {
		[IPC_RX] = {"/dev/ipc_media_stack_gptp_1_rx"},
		[IPC_TX] = {"/dev/ipc_media_stack_gptp_1_tx"}
	},

	[IPC_GPTP_1_MEDIA_STACK] = {
		[IPC_RX] = {"/dev/ipc_gptp_1_media_stack_rx"},
		[IPC_TX] = {"/dev/ipc_gptp_1_media_stack_tx"}
	},

	[IPC_GPTP_1_MEDIA_STACK_SYNC] = {
		[IPC_RX] = {"/dev/ipc_gptp_1_media_stack_sync_rx"},
		[IPC_TX] = {"/dev/ipc_gptp_1_media_stack_sync_tx"}
	},

	[IPC_MEDIA_STACK_GPTP_BRIDGE] = {
		[IPC_RX] = {"/dev/ipc_media_stack_gptp_bridge_rx"},
		[IPC_TX] = {"/dev/ipc_media_stack_gptp_bridge_tx"}
	},

	[IPC_GPTP_BRIDGE_MEDIA_STACK] = {
		[IPC_RX] = {"/dev/ipc_gptp_bridge_media_stack_rx"},
		[IPC_TX] = {"/dev/ipc_gptp_bridge_media_stack_tx"}
	},

	[IPC_GPTP_BRIDGE_MEDIA_STACK_SYNC] = {
		[IPC_RX] = {"/dev/ipc_gptp_bridge_media_stack_sync_rx"},
		[IPC_TX] = {"/dev/ipc_gptp_bridge_media_stack_sync_tx"}
	},

	[IPC_MEDIA_STACK_AVTP] = {
		[IPC_RX] = {"/dev/ipc_media_stack_avtp_rx"},
		[IPC_TX] = {"/dev/ipc_media_stack_avtp_tx"}
	},

	[IPC_AVTP_MEDIA_STACK] = {
		[IPC_RX] = {"/dev/ipc_avtp_media_stack_rx"},
		[IPC_TX] = {"/dev/ipc_avtp_media_stack_tx"}
	},

	[IPC_AVTP_STATS] = {
		[IPC_RX] = {"/dev/ipc_avtp_stats_rx"},
		[IPC_TX] = {"/dev/ipc_avtp_stats_tx"}
	},

	[IPC_AVDECC_AVTP] = {
		[IPC_RX] = {"/dev/ipc_avdecc_avtp_rx"},
		[IPC_TX] = {"/dev/ipc_avdecc_avtp_tx"}
	},

	[IPC_AVTP_AVDECC] = {
		[IPC_RX] = {"/dev/ipc_avtp_avdecc_rx"},
		[IPC_TX] = {"/dev/ipc_avtp_avdecc_tx"}
	},

	[IPC_MEDIA_STACK_MAC_SERVICE] = {
		[IPC_RX] = {"/dev/ipc_media_stack_mac_service_rx"},
		[IPC_TX] = {"/dev/ipc_media_stack_mac_service_tx"}
	},

	[IPC_MAC_SERVICE_MEDIA_STACK] = {
		[IPC_RX] = {"/dev/ipc_mac_service_media_stack_rx"},
		[IPC_TX] = {"/dev/ipc_mac_service_media_stack_tx"}
	},

	[IPC_MAC_SERVICE_MEDIA_STACK_SYNC] = {
		[IPC_RX] = {"/dev/ipc_mac_service_media_stack_sync_rx"},
		[IPC_TX] = {"/dev/ipc_mac_service_media_stack_sync_tx"}
	},

	[IPC_MEDIA_STACK_MAC_SERVICE_1] = {
		[IPC_RX] = {"/dev/ipc_media_stack_mac_service_1_rx"},
		[IPC_TX] = {"/dev/ipc_media_stack_mac_service_1_tx"}
	},

	[IPC_MAC_SERVICE_1_MEDIA_STACK] = {
		[IPC_RX] = {"/dev/ipc_mac_service_1_media_stack_rx"},
		[IPC_TX] = {"/dev/ipc_mac_service_1_media_stack_tx"}
	},

	[IPC_MAC_SERVICE_1_MEDIA_STACK_SYNC] = {
		[IPC_RX] = {"/dev/ipc_mac_service_1_media_stack_sync_rx"},
		[IPC_TX] = {"/dev/ipc_mac_service_1_media_stack_sync_tx"}
	},

	[IPC_MEDIA_STACK_MAC_SERVICE_BRIDGE] = {
		[IPC_RX] = {"/dev/ipc_media_stack_mac_service_bridge_rx"},
		[IPC_TX] = {"/dev/ipc_media_stack_mac_service_bridge_tx"}
	},

	[IPC_MAC_SERVICE_BRIDGE_MEDIA_STACK] = {
		[IPC_RX] = {"/dev/ipc_mac_service_bridge_media_stack_rx"},
		[IPC_TX] = {"/dev/ipc_mac_service_bridge_media_stack_tx"}
	},

	[IPC_MAC_SERVICE_BRIDGE_MEDIA_STACK_SYNC] = {
		[IPC_RX] = {"/dev/ipc_mac_service_bridge_media_stack_sync_rx"},
		[IPC_TX] = {"/dev/ipc_mac_service_bridge_media_stack_sync_tx"}
	},

	[IPC_MEDIA_STACK_MSRP_BRIDGE] = {
		[IPC_RX] = {"/dev/ipc_media_stack_msrp_bridge_rx"},
		[IPC_TX] = {"/dev/ipc_media_stack_msrp_bridge_tx"}
	},

	[IPC_MSRP_BRIDGE_MEDIA_STACK] = {
		[IPC_RX] = {"/dev/ipc_msrp_bridge_media_stack_rx"},
		[IPC_TX] = {"/dev/ipc_msrp_bridge_media_stack_tx"}
	},

	[IPC_MSRP_BRIDGE_MEDIA_STACK_SYNC] = {
		[IPC_RX] = {"/dev/ipc_msrp_bridge_media_stack_sync_rx"},
		[IPC_TX] = {"/dev/ipc_msrp_bridge_media_stack_sync_tx"}
	},

	[IPC_MEDIA_STACK_MVRP_BRIDGE] = {
		[IPC_RX] = {"/dev/ipc_media_stack_mvrp_bridge_rx"},
		[IPC_TX] = {"/dev/ipc_media_stack_mvrp_bridge_tx"}
	},

	[IPC_MVRP_BRIDGE_MEDIA_STACK] = {
		[IPC_RX] = {"/dev/ipc_mvrp_bridge_media_stack_rx"},
		[IPC_TX] = {"/dev/ipc_mvrp_bridge_media_stack_tx"}
	},

	[IPC_MVRP_BRIDGE_MEDIA_STACK_SYNC] = {
		[IPC_RX] = {"/dev/ipc_mvrp_bridge_media_stack_sync_rx"},
		[IPC_TX] = {"/dev/ipc_mvrp_bridge_media_stack_sync_tx"}
	},

	[IPC_SRP_BRIDGE_ENDPOINT] = {
		[IPC_RX] = {"/dev/ipc_srp_bridge_endpoint_rx"},
		[IPC_TX] = {"/dev/ipc_srp_bridge_endpoint_tx"}
	},

	[IPC_SRP_ENDPOINT_BRIDGE] = {
		[IPC_RX] = {"/dev/ipc_srp_endpoint_bridge_rx"},
		[IPC_TX] = {"/dev/ipc_srp_endpoint_bridge_tx"}
	},
};

static void *ipc_shmem_to_virt(void *mmap_baseaddr, unsigned long addr)
{
	return (char *)mmap_baseaddr + addr;
}

static unsigned long ipc_virt_to_shmem(void *mmap_baseaddr, void *addr)
{
	return (char *)addr - (char *)mmap_baseaddr;
}

struct ipc_desc *ipc_alloc(struct ipc_tx const *tx, unsigned int size)
{
	unsigned long addr;

	if (size > IPC_BUF_SIZE)
		return NULL;

	if (ioctl(tx->fd, IPC_IOC_ALLOC, &addr) < 0) {
		os_log(LOG_ERR, "ioctl() %s ipc_tx(%p)\n", strerror(errno), tx);
		return NULL;
	}

	return ipc_shmem_to_virt(tx->mmap_baseaddr, addr);
}


void ipc_free(void const *ipc, struct ipc_desc *desc)
{
	unsigned long addr = ipc_virt_to_shmem(((struct ipc_tx const *)ipc)->mmap_baseaddr, desc);

	if (ioctl(((struct ipc_tx const *)ipc)->fd, IPC_IOC_FREE, &addr) < 0)
		os_log(LOG_ERR, "ioctl() %s\n", strerror(errno));
}


int ipc_rx_init_no_notify(struct ipc_rx *rx, ipc_id_t id)
{

	rx->fd = open(ipc_device[id][IPC_RX], O_RDWR | O_CLOEXEC);
	if (rx->fd < 0) {
		os_log(LOG_ERR, "open(%s) %s\n", ipc_device[id][IPC_RX], strerror(errno));
		goto err_open;
	}

	if (ioctl(rx->fd, IPC_IOC_POOL_SIZE, &rx->pool_size) < 0) {
		os_log(LOG_ERR, "ioctl() %s\n", strerror(errno));
		goto err_ioctl;
	}

	rx->mmap_baseaddr = mmap(NULL, rx->pool_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, rx->fd, 0);
	if (rx->mmap_baseaddr == MAP_FAILED) {
		os_log(LOG_ERR, "mmap() %s\n", strerror(errno));
		goto err_mmap;
	}

	if (madvise(rx->mmap_baseaddr, rx->pool_size, MADV_DONTFORK) < 0)
		os_log(LOG_ERR, "madvise() %s\n", strerror(errno));

	os_log(LOG_INFO, "ipc_rx(%p) id(%d) fd(%d) baseaddr(%p) size : %lu\n", rx, id, rx->fd, rx->mmap_baseaddr, rx->pool_size);

	return 0;

err_ioctl:
err_mmap:
	close(rx->fd);
	rx->fd = -1;

err_open:
	return -1;
}

int ipc_rx_init(struct ipc_rx *rx, ipc_id_t id, void (*func)(struct ipc_rx const *, struct ipc_desc *), unsigned long priv)
{
	int epoll_fd = (int)priv;

	if (ipc_rx_init_no_notify(rx, id) < 0)
		goto err_init;

	if (epoll_fd >= 0) {
		if (epoll_ctl_add(epoll_fd, rx->fd, EPOLL_TYPE_IPC, rx, &rx->epoll_data, EPOLLIN) < 0) {
			os_log(LOG_ERR, "ipc_rx(%p) epoll_ctl_add() failed for ipc id(%d)\n", rx, id);
			goto err_epoll_ctl;
		}
	}

	rx->func = func;

	os_log(LOG_INFO, "ipc_rx(%p) id(%d) fd(%d) baseaddr(%p)\n", rx, id, rx->fd, rx->mmap_baseaddr);

	return 0;

err_epoll_ctl:
	munmap(rx->mmap_baseaddr, rx->pool_size);
	close(rx->fd);
	rx->fd = -1;

err_init:
	return -1;
}


int ipc_tx_init(struct ipc_tx *tx, ipc_id_t id)
{
	os_log(LOG_DEBUG, "ipc_tx(%p, %d)\n", tx, id);

	tx->fd = open(ipc_device[id][IPC_TX], O_RDWR | O_CLOEXEC);
	if (tx->fd < 0) {
		os_log(LOG_ERR, "open(%s) %s\n", ipc_device[id][IPC_TX], strerror(errno));
		goto err_open;
	}

	if (ioctl(tx->fd, IPC_IOC_POOL_SIZE, &tx->pool_size) < 0) {
		os_log(LOG_ERR, "ioctl() %s\n", strerror(errno));
		goto err_ioctl;
	}

	tx->mmap_baseaddr = mmap(NULL, tx->pool_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, tx->fd, 0);
	if (tx->mmap_baseaddr == MAP_FAILED) {
		os_log(LOG_ERR, "mmap() %s\n", strerror(errno));
		goto err_mmap;
	}

	if (madvise(tx->mmap_baseaddr, tx->pool_size, MADV_DONTFORK) < 0)
		os_log(LOG_ERR, "madvise() %s\n", strerror(errno));

	os_log(LOG_INFO, "ipc_tx(%p) id(%d) fd(%d) baseaddr(%p) size : %lu\n", tx, id, tx->fd, tx->mmap_baseaddr, tx->pool_size);

	return 0;

err_ioctl:
err_mmap:
	close(tx->fd);
	tx->fd = -1;

err_open:
	return -1;
}

int ipc_tx_connect(struct ipc_tx *tx, struct ipc_rx *rx)
{
	if (ioctl(tx->fd, IPC_IOC_CONNECT_TX, &rx->fd) < 0) {
		os_log(LOG_ERR, "ioctl() %s\n", strerror(errno));
		goto err_ioctl;
	}

	return 0;

err_ioctl:
	return -1;
}

void ipc_rx_exit(struct ipc_rx *rx)
{
	os_log(LOG_DEBUG, "ipc_rx(%p)\n", rx);

	if (rx->fd >= 0) {
		munmap(rx->mmap_baseaddr, rx->pool_size);
		close(rx->fd);
		rx->fd = -1;
	}
}

void ipc_tx_exit(struct ipc_tx *tx)
{
	os_log(LOG_DEBUG, "ipc_tx(%p)\n", tx);

	if (tx->fd >= 0) {
		munmap(tx->mmap_baseaddr, tx->pool_size);
		close(tx->fd);
		tx->fd = -1;
	}
}

static inline unsigned int ipc_header_len(void)
{
	struct ipc_desc *tmp = (struct ipc_desc *)0;

	/* Length of all structure members, up to the union */
	return (unsigned long)&tmp->u;
}

int ipc_tx(struct ipc_tx const *tx, struct ipc_desc *desc)
{
	struct ipc_tx_data data;
	int rc;

	data.addr_shmem = ipc_virt_to_shmem(tx->mmap_baseaddr, desc);
	data.len = desc->len + ipc_header_len();
	data.dst = desc->dst;

	rc = ioctl(tx->fd, IPC_IOC_TX, &data);
	if (rc < 0) {
		os_log(LOG_DEBUG, "ipc_tx(%p) ioctl() %s(%d)\n", tx, strerror(errno), errno);
		switch (errno) {
		case EPIPE:
			rc = -IPC_TX_ERR_NO_READER;
			break;
		case EAGAIN:
			rc = -IPC_TX_ERR_QUEUE_FULL;
			break;
		default:
			rc = -IPC_TX_ERR_UNKNOWN;
			break;
		}
	}

	return rc;
}

struct ipc_desc * __ipc_rx(struct ipc_rx const *rx)
{
	struct ipc_rx_data data;
	struct ipc_desc *desc = NULL;

	if (ioctl(rx->fd, IPC_IOC_RX, &data) < 0)
		goto err;

	desc = ipc_shmem_to_virt(rx->mmap_baseaddr, data.addr_shmem);
	desc->src = data.src;

err:
	return desc;
}

void ipc_rx(struct ipc_rx const *rx)
{
	struct ipc_desc *desc;

	while (1) {
		desc = __ipc_rx(rx);
		if (!desc)
			break;

		rx->func(rx, desc);
	}
}
