/*
 * Copyright 2020-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Linux specific FQTSS service implementation
 @details
*/

#include "common/log.h"
#include "os_config.h"
#include "fqtss.h"

__attribute__((weak)) int fqtss_avb_init(struct fqtss_ops_cb *fqtss_ops) { return -1; };
__attribute__((weak)) int fqtss_std_init(struct fqtss_ops_cb *fqtss_ops) { return -1; };

static struct fqtss_ops_cb fqtss_ops;

int fqtss_set_oper_idle_slope(unsigned int port_id, uint8_t traffic_class, unsigned int idle_slope)
{
	return fqtss_ops.fqtss_set_oper_idle_slope(port_id, traffic_class, idle_slope);
}

int fqtss_stream_add(unsigned int port_id, void *stream_id, uint16_t vlan_id, uint8_t priority, unsigned int idle_slope)
{
	return fqtss_ops.fqtss_stream_add(port_id, stream_id, vlan_id, priority, idle_slope);
}

int fqtss_stream_remove(unsigned int port_id, void *stream_id, uint16_t vlan_id, uint8_t priority, unsigned int idle_slope)
{
	return fqtss_ops.fqtss_stream_remove(port_id, stream_id, vlan_id, priority, idle_slope);
}

int fqtss_init(struct os_net_config *config)
{
	switch (config->srp_net_mode) {
	case NET_AVB:
		if (fqtss_avb_init(&fqtss_ops) < 0) {
			os_log(LOG_ERR, "Could not initialize AVB network service implementation\n");
			goto err;
		}
		break;
	case NET_STD:
	case NET_XDP:
		if (fqtss_std_init(&fqtss_ops) < 0) {
			os_log(LOG_ERR, "Could not initialize STD network service implementation\n");
			goto err;
		}
		break;
	default:
		goto err;
	}

	os_log(LOG_INIT, "done\n");

	return 0;

err:
	return -1;
}

void fqtss_exit(void)
{
	return fqtss_ops.fqtss_exit();
}
