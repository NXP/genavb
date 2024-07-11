/*
 * Copyright 2020-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LINUX_OS_CONFIG_H_
#define _LINUX_OS_CONFIG_H_

#include "genavb/config.h"
#include <stdbool.h>

#define SYSTEM_CONF_FILENAME "/etc/genavb/system.cfg"

#define CFG_STRING_MAX_LEN	256
#define CFG_STRING_LIST_MAX_LEN	32

typedef enum {
	NET_UNSPECIFIED = 0,
	NET_AVB,
	NET_STD,
	NET_XDP,
	NET_IPC,
} network_mode_t;

#define NET_MODE_TO_FLAG(mode)			(1 << mode)
#define HAS_NET_MODE_ENABLED(flag, mode)	(flag & (1 << mode))

struct os_config {
	struct os_logical_port_config {
		char endpoint[CFG_MAX_ENDPOINTS][CFG_STRING_LIST_MAX_LEN];
		char bridge[CFG_MAX_BRIDGES][CFG_STRING_LIST_MAX_LEN];
		char bridge_ports[CFG_MAX_BRIDGES][CFG_MAX_NUM_PORT][CFG_STRING_LIST_MAX_LEN];
		char endpoint_hybrid_port[CFG_STRING_MAX_LEN];
		char bridge_hybrid_port[CFG_STRING_MAX_LEN];
	} logical_port_config;

	struct os_clock_config {
		char endpoint_gptp[CFG_MAX_GPTP_DOMAINS][CFG_MAX_ENDPOINTS][CFG_STRING_LIST_MAX_LEN];
		char endpoint_local[CFG_MAX_ENDPOINTS][CFG_STRING_LIST_MAX_LEN];
		char bridge_gptp[CFG_MAX_GPTP_DOMAINS][CFG_MAX_BRIDGES][CFG_STRING_LIST_MAX_LEN];
		char bridge_local[CFG_MAX_BRIDGES][CFG_STRING_LIST_MAX_LEN];
	} clock_config;

	struct os_net_config {
		unsigned int enabled_modes_flag;
		network_mode_t gptp_net_mode;
		network_mode_t avdecc_net_mode;
		network_mode_t avtp_net_mode;
		network_mode_t srp_net_mode;
		network_mode_t api_sockets_net_mode;
		bool enable_ipc_net_mode;
	} net_config;

	struct os_xdp_config {
		int endpoint_queue_rx[CFG_MAX_ENDPOINTS];
		int endpoint_queue_tx[CFG_MAX_ENDPOINTS];
	} xdp_config;
};

int os_config_get(struct os_config *config);
void os_net_config_parse(struct os_net_config *net_config);

#endif /* _LINUX_OS_CONFIG_H_ */
