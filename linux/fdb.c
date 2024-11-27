/*
 * Copyright 2020-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Linux specific FDB service implementation
 @details
*/
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#include "os/fdb.h"
#include "common/log.h"

#include "net_logical_port.h"
#include "fdb.h"

static struct fdb_ops_cb fdb_ops;

static int default_bridge_rtnetlink(u8 *mac_addr, u16 vid, unsigned int port_id, bool add)
{
	return -1;
}

static int fdb_avb_init(struct fdb_ops_cb *fdb_ops)
{
	fdb_ops->bridge_rtnetlink = default_bridge_rtnetlink;

	return 0;
}

//TODO: remove this when we get to a unified binary for all net modes.
__attribute__((weak)) int fdb_std_init(struct fdb_ops_cb *fdb_ops)
{
	fdb_ops->bridge_rtnetlink = default_bridge_rtnetlink;

	return 0;
}

static int bridge_rtnetlink(u8 *mac_addr, u16 vid, unsigned int port_id, bool add)
{
	return fdb_ops.bridge_rtnetlink(mac_addr, vid, port_id, add);
}

int fdb_update(unsigned int port_id, u8 *mac, u16 vid, bool dynamic, genavb_fdb_port_control_t control)
{
	bool add;
	int rc = 0;

	switch (control) {
	case GENAVB_FDB_PORT_CONTROL_FORWARDING:
		add = true;
		break;
	case GENAVB_FDB_PORT_CONTROL_FILTERING:
		add = false;
		break;
	default:
		rc = -1;
		goto err;
	}

	if (bridge_rtnetlink(mac, vid, port_id, add) < 0)
		rc = -1;

err:
	return rc;
}

int fdb_delete(u8 *mac, u16 vid, bool dynamic)
{
	int i, rc = 0;

	for (i = 0; i < logical_port_max(); i++) {
		if (!logical_port_valid(i))
			continue;

		if (!logical_port_is_bridge(i))
			continue;

		if (bridge_rtnetlink(mac, vid, i, false) < 0)
			rc = -1;
	}

	return rc;
}

int fdb_read(uint8_t *address, uint16_t vid, bool *dynamic, struct genavb_fdb_port_map *map, genavb_fdb_status_t *status)
{
	return -1;
}

int fdb_dump(uint32_t *token, uint8_t *address, uint16_t *vid, bool *dynamic, struct genavb_fdb_port_map *map, genavb_fdb_status_t *status)
{
	return -1;
}

int fdb_init(struct os_net_config *config)
{
	switch (config->srp_net_mode) {
	case NET_AVB:
		if (fdb_avb_init(&fdb_ops) < 0) {
			os_log(LOG_ERR, "Could not initialize AVB FDB service implementation\n");
			goto err;
		}
		break;
	case NET_STD:
	case NET_XDP:
		if (fdb_std_init(&fdb_ops) < 0) {
			os_log(LOG_ERR, "Could not initialize STD FDB service implementation\n");
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
