/*
* Copyright 2023-2024 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief rtos specific FRER service implementation
 @details
*/

#include "genavb/error.h"

#include "os/frer.h"
#include "common/log.h"
#include "net_bridge.h"
#include "net_logical_port.h"

#if CFG_BRIDGE_NUM

int sequence_generation_update(uint32_t index, struct genavb_sequence_generation *entry, unsigned int option)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -GENAVB_ERR_FRER_NOT_SUPPORTED;

	/* 802.1CB-2017, Table 8.1 */
	if (entry) {
		if (entry->direction_out_facing) {
			rc = -GENAVB_ERR_FRER_PARAMS;
			goto out;
		}

		if (entry->stream_n < 1) {
			rc = -GENAVB_ERR_FRER_PARAMS;
			goto out;
		}
	} else {
		if (option != SEQG_UPDATE_OPTION_RESET) {
			rc = -GENAVB_ERR_FRER_NOT_SUPPORTED;
			goto out;
		}
	}

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (bridge->drv_ops.seqg_update)
		rc = bridge->drv_ops.seqg_update(bridge, index, entry, option);

	rtos_mutex_unlock(&bridge->mutex);

out:
	return rc;
}

int sequence_generation_delete(uint32_t index)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -GENAVB_ERR_FRER_NOT_SUPPORTED;

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (bridge->drv_ops.seqg_delete)
		rc = bridge->drv_ops.seqg_delete(bridge, index);

	rtos_mutex_unlock(&bridge->mutex);

	return rc;
}

int sequence_generation_read(uint32_t index, struct genavb_sequence_generation *entry)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -GENAVB_ERR_FRER_NOT_SUPPORTED;

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (bridge->drv_ops.seqg_read)
		rc = bridge->drv_ops.seqg_read(bridge, index, entry);

	rtos_mutex_unlock(&bridge->mutex);

	return rc;
}

int sequence_recovery_update(uint32_t index, struct genavb_sequence_recovery *entry)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -GENAVB_ERR_FRER_NOT_SUPPORTED;
	int i;

	if (entry->stream_n < 1) {
		rc = -GENAVB_ERR_FRER_PARAMS;
		goto out;
	}

	if (entry->port_n < 1) {
		rc = -GENAVB_ERR_FRER_PARAMS;
		goto out;
	}

	/* 802.1CB-2017, Table 8.1 - !!! seems to be reversed in standard */
	if (!entry->direction_out_facing) {
		rc = -GENAVB_ERR_FRER_PARAMS;
		goto out;
	}

	if ((entry->algorithm != GENAVB_SEQR_VECTOR) &&
	    (entry->algorithm != GENAVB_SEQR_MATCH)) {
		rc = -GENAVB_ERR_FRER_PARAMS;
		goto out;
	}

	if (entry->individual_recovery && entry->latent_error_detection) {
		rc = -GENAVB_ERR_FRER_PARAMS;
		goto out;
	}

	/* Not supported for now (requires software support) */
	if (entry->latent_error_detection) {
		rc = -GENAVB_ERR_FRER_PARAMS;
		goto out;
	}

	for (i = 0; i < entry->port_n; i++) {
		struct logical_port *port = logical_port_get(entry->port[i]);

		if (!port || !logical_port_is_bridge(entry->port[i]) ||
		    (logical_port_bridge_id(entry->port[i]) != DEFAULT_BRIDGE_ID)) {
			rc = -GENAVB_ERR_INVALID_PORT;
			goto out;
		}
	}

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (bridge->drv_ops.seqr_update)
		rc = bridge->drv_ops.seqr_update(bridge, index, entry);

	rtos_mutex_unlock(&bridge->mutex);

out:
	return rc;
}

int sequence_recovery_delete(uint32_t index)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -GENAVB_ERR_FRER_NOT_SUPPORTED;

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (bridge->drv_ops.seqr_delete)
		rc = bridge->drv_ops.seqr_delete(bridge, index);

	rtos_mutex_unlock(&bridge->mutex);

	return rc;
}

int sequence_recovery_read(uint32_t index, struct genavb_sequence_recovery *entry)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	int rc = -GENAVB_ERR_FRER_NOT_SUPPORTED;

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (bridge->drv_ops.seqr_read)
		rc = bridge->drv_ops.seqr_read(bridge, index, entry);

	rtos_mutex_unlock(&bridge->mutex);

	return rc;
}

int sequence_identification_update(unsigned int port_id, bool direction_out_facing, struct genavb_sequence_identification *entry)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	struct logical_port *port = logical_port_get(port_id);
	int rc = -GENAVB_ERR_FRER_NOT_SUPPORTED;

	if (!port || !logical_port_is_bridge(port_id) ||
	    (logical_port_bridge_id(port_id) != DEFAULT_BRIDGE_ID)) {
		rc = -GENAVB_ERR_INVALID_PORT;
		goto out;
	}

	if (!direction_out_facing) {
		rc = -GENAVB_ERR_FRER_PARAMS;
		goto out;
	}

	if (entry->stream_n < 1) {
		rc = -GENAVB_ERR_FRER_PARAMS;
		goto out;
	}

	/* PRP not supported */
	if ((entry->encapsulation != GENAVB_SEQI_RTAG) &&
	    (entry->encapsulation != GENAVB_SEQI_HSR_SEQ_TAG)) {
		rc = -GENAVB_ERR_FRER_PARAMS;
		goto out;
	}

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (bridge->drv_ops.seqi_update)
		rc = bridge->drv_ops.seqi_update(bridge, port->phys, entry);

	rtos_mutex_unlock(&bridge->mutex);

out:
	return rc;
}

int sequence_identification_delete(unsigned int port_id, bool direction_out_facing)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	struct logical_port *port = logical_port_get(port_id);
	int rc = -GENAVB_ERR_FRER_NOT_SUPPORTED;

	if (!port || !logical_port_is_bridge(port_id) ||
	    (logical_port_bridge_id(port_id) != DEFAULT_BRIDGE_ID)) {
		rc = -GENAVB_ERR_INVALID_PORT;
		goto out;
	}

	if (!direction_out_facing) {
		rc = -GENAVB_ERR_FRER_PARAMS;
		goto out;
	}

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (bridge->drv_ops.seqi_delete)
		rc = bridge->drv_ops.seqi_delete(bridge, port->phys);

	rtos_mutex_unlock(&bridge->mutex);

out:
	return rc;
}

int sequence_identification_read(unsigned int port_id, bool direction_out_facing, struct genavb_sequence_identification *entry)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	struct logical_port *port = logical_port_get(port_id);
	int rc = -GENAVB_ERR_FRER_NOT_SUPPORTED;

	if (!port || !logical_port_is_bridge(port_id) ||
	    (logical_port_bridge_id(port_id) != DEFAULT_BRIDGE_ID)) {
		rc = -GENAVB_ERR_INVALID_PORT;
		goto out;
	}

	if (!direction_out_facing) {
		rc = -GENAVB_ERR_FRER_PARAMS;
		goto out;
	}

	rtos_mutex_lock(&bridge->mutex, RTOS_WAIT_FOREVER);

	if (bridge->drv_ops.seqi_read)
		rc = bridge->drv_ops.seqi_read(bridge, port->phys, entry);

	rtos_mutex_unlock(&bridge->mutex);

out:
	return rc;
}
#else
int sequence_generation_update(uint32_t index, struct genavb_sequence_generation *entry, unsigned int option) { return -1; }
int sequence_generation_delete(uint32_t index) { return -1; }
int sequence_generation_read(uint32_t index, struct genavb_sequence_generation *entry) { return -1; }

int sequence_recovery_update(uint32_t index, struct genavb_sequence_recovery *entry) { return -1; }
int sequence_recovery_delete(uint32_t index) { return -1; }
int sequence_recovery_read(uint32_t index, struct genavb_sequence_recovery *entry) { return -1; }

int sequence_identification_update(unsigned int port_id, bool direction_out_facing, struct genavb_sequence_identification *entry) { return -1; }
int sequence_identification_delete(unsigned int port_id, bool direction_out_facing) { return -1; }
int sequence_identification_read(unsigned int port_id, bool direction_out_facing, struct genavb_sequence_identification *entry) { return -1; }
#endif
