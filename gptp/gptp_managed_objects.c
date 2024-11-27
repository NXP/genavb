/*
 * Copyright 2018-2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief GPTP managed objects definition
 @details
*/

#include "gptp_managed_objects.h"
#include "common/ptp_time_ops.h"
#include "gptp.h"

extern const unsigned int leaf_object_size[];

typedef struct port_params {
	unsigned int port_idx;
} port_params_t;

static port_params_t port_params;

static void port_params_reset(void)
{
	port_params.port_idx = 0;
}

static uintptr_t port_handler(void *data, uintptr_t base)
{
	struct gptp_instance *instance = data;

	/* if first iteration in the list, start with the first element */
	if (!base) {
		port_params_reset();
	} else {
		port_params.port_idx++;
		if (port_params.port_idx >= instance->numberPorts)
			return 0;
	}

	return (uintptr_t)&instance->ports[port_params.port_idx];
}

static void selected_role_handler(void *data, struct leaf *l, enum node_operation operation, uint8_t *buf, uintptr_t base)
{
	struct gptp_instance *instance = data;
	struct gptp_port *port = (struct gptp_port *)base;

	switch (operation) {
	case NODE_GET:
		buf[0] = instance->params.selected_role[port->port_id + 1];
		break;

	default:
		break;
	}
}

static void version_number_handler(void *data, struct leaf *l, enum node_operation operation, uint8_t *buf, uintptr_t base)
{
	struct gptp_instance *instance = data;

	switch (operation) {
	case NODE_GET:
		buf[0] = instance->versionNumber;
		break;

	default:
		break;
	}
}

static void priority_handler(void *data, struct leaf *l, enum node_operation operation, uint8_t *buf, uintptr_t base)
{
	switch (operation) {
	case NODE_GET:
		buf[0] = *((u8 *)(base + l->val));
		break;

	case NODE_SET:
		*((u8 *)(base + l->val)) = buf[0];
		gptp_instance_priority_vector_update(data);
		break;

	default:
		break;
	}
}

static void offset_scaled_log_variance_handler(void *data, struct leaf *l, enum node_operation operation, uint8_t *buf, uintptr_t base)
{
	uint16_t val;

	switch (operation) {
	case NODE_GET:
		val = *((u16 *)(base + l->val));
		val = ntohs(val);
		os_memcpy(buf, &val, sizeof(u16));
		break;

	default:
		break;
	}
}

static void neighbor_rate_ratio_handler(void *data, struct leaf *l, enum node_operation operation, uint8_t *buf, uintptr_t base)
{
	struct gptp_port *port = (struct gptp_port *)base;
	double ratio;
	s32 ratio_s32;

	switch (operation) {
	case NODE_GET:
		ratio = *((double *)((uintptr_t)port->c + (uintptr_t)l->val));
		ratio = (ratio - 1.0) * POW_2_41;
		ratio_s32 = (s32)ratio;
		os_memcpy(buf, &ratio_s32, sizeof(s32));
		break;

	default:
		break;
	}
}

static void ptp_port_enabled_handler(void *data, struct leaf *l, enum node_operation operation, uint8_t *buf, uintptr_t base)
{
	struct gptp_port *port = (struct gptp_port *)base;

	switch (operation) {
	case NODE_GET:
		buf[0] = *((bool *)(base + l->val));
		break;

	case NODE_SET:
		*((bool *)(base + l->val)) = buf[0];
		gptp_port_update_fsm(port);
		break;

	default:
		break;
	}
}

static void ptp_port_common_handler(void *data, struct leaf *l, enum node_operation operation, uint8_t *buf, uintptr_t base)
{
	struct gptp_port *port = (struct gptp_port *)base;
	uintptr_t object_base;
	unsigned int object_size;

	switch (operation) {
	case NODE_GET:
		object_base = (uintptr_t)port->c + (uintptr_t)l->val;
		object_size = leaf_object_size[l->type];

		if (l->type == LEAF_BOOL)
			buf[0] = *((bool *)(object_base + l->val));
		else
			os_memcpy(buf, (object_base + l->val), object_size);
		break;

	default:
		break;
	}
}


unsigned int gptp_managed_objects_get(struct gptp_managed_objects *module, u8 *in, u8 *in_end, u8 *out, u8 *out_end)
{
	uint8_t *end = module_iterate(&module->node, NODE_GET, in, in_end, out, out_end);

	return end - out;
}

unsigned int gptp_managed_objects_set(struct gptp_managed_objects *module, u8 *in, u8 *in_end, u8 *out, u8 *out_end)
{
	uint8_t *end = module_iterate(&module->node, NODE_SET, in, in_end, out, out_end);

	return end - out;
}

__init void gptp_managed_objects_init(struct gptp_managed_objects *module, struct gptp_ctx *gptp)
{
	struct gptp_instance *instance = gptp->instances[0];
	uint16_t key_id;

	MODULE_INIT(module, "ietf-gptp");

	CONTAINER_INIT(module, default_parameter_data_set);
	CONTAINER_INIT(module, current_parameter_data_set);
	CONTAINER_INIT(module, parent_parameter_data_set);
	CONTAINER_INIT(module, time_properties_parameter_data_set);

	key_id = 0;
	LIST_DYNAMIC_INIT(module, port_parameter_data_set, port_handler, instance, 1, &key_id);
	LIST_DYNAMIC_INIT(module, port_parameter_statistics, port_handler, instance, 1, &key_id);

	LEAF_INIT(&module->default_parameter_data_set, clockIdentity, LEAF_CLOCK_IDENTITY, LEAF_R, &instance->params.this_clock, NULL, NULL);
	LEAF_INIT(&module->default_parameter_data_set, numberPorts, LEAF_UINT8, LEAF_R, &instance->numberPorts, NULL, NULL);
	LEAF_INIT(&module->default_parameter_data_set, clockClass, LEAF_UINT8, LEAF_R, &instance->params.system_priority.u.s.root_system_identity.u.s.clock_quality.clock_class, NULL, NULL);
	LEAF_INIT(&module->default_parameter_data_set, clockAccuracy, LEAF_UINT8, LEAF_R, &instance->params.system_priority.u.s.root_system_identity.u.s.clock_quality.clock_accuracy, NULL, NULL);
	LEAF_INIT(&module->default_parameter_data_set, offsetScaledLogVariance, LEAF_UINT16, LEAF_R, &instance->params.system_priority.u.s.root_system_identity.u.s.clock_quality.offset_scaled_log_variance, offset_scaled_log_variance_handler, NULL);
	LEAF_INIT(&module->default_parameter_data_set, priority1, LEAF_UINT8, LEAF_RW, &instance->params.system_priority.u.s.root_system_identity.u.s.priority_1, priority_handler, instance);
	LEAF_INIT(&module->default_parameter_data_set, priority2, LEAF_UINT8, LEAF_RW, &instance->params.system_priority.u.s.root_system_identity.u.s.priority_2, priority_handler, instance);
	LEAF_INIT(&module->default_parameter_data_set, gmCapable, LEAF_BOOL, LEAF_R, &instance->gmCapable, NULL, NULL);
	LEAF_INIT(&module->default_parameter_data_set, currentUtcOffset, LEAF_UINT16, LEAF_RW, &instance->params.sys_current_utc_offset, NULL, NULL);
	LEAF_INIT(&module->default_parameter_data_set, currentUtcOffetValid, LEAF_BOOL, LEAF_RW, &instance->params.sys_current_utc_offset_valid, NULL, NULL);
	LEAF_INIT(&module->default_parameter_data_set, leap59, LEAF_BOOL, LEAF_RW, &instance->params.sys_leap59, NULL, NULL);
	LEAF_INIT(&module->default_parameter_data_set, leap61, LEAF_BOOL, LEAF_RW, &instance->params.sys_leap61, NULL, NULL);
	LEAF_INIT(&module->default_parameter_data_set, timeTraceable, LEAF_BOOL, LEAF_R, &instance->params.sys_time_traceable, NULL, NULL);
	LEAF_INIT(&module->default_parameter_data_set, frequencyTraceable, LEAF_BOOL, LEAF_R, &instance->params.sys_frequency_traceable, NULL, NULL);
	LEAF_INIT(&module->default_parameter_data_set, timeSource, LEAF_UINT8, LEAF_R, &instance->params.sys_time_source, NULL, NULL);

	LEAF_INIT(&module->current_parameter_data_set, stepsRemoved, LEAF_UINT16, LEAF_R, &instance->params.master_steps_removed, NULL, NULL);
	LEAF_INIT(&module->current_parameter_data_set, offsetFromMaster, LEAF_SCALED_NS, LEAF_R, NULL, NULL, NULL); /* FIXME Implementation defined, should be provided by the ClockMasterSyncOffset SM, 14.3.2 */
	LEAF_INIT(&module->current_parameter_data_set, lastGmPhaseChange, LEAF_SCALED_NS, LEAF_R, &instance->params.last_gm_phase_change, NULL, NULL);
	LEAF_INIT(&module->current_parameter_data_set, lastGmFreqChange, LEAF_DOUBLE, LEAF_R, &instance->params.last_gm_freq_change, NULL, NULL);
	LEAF_INIT(&module->current_parameter_data_set, gmTimebaseIndicator, LEAF_UINT16, LEAF_R, &instance->params.gm_time_base_indicator, NULL, NULL);
	LEAF_INIT(&module->current_parameter_data_set, gmChangeCount, LEAF_UINT32, LEAF_R, &instance->gmChangeCount, NULL, NULL);
	LEAF_INIT(&module->current_parameter_data_set, timeOfLastGmChangeEvent, LEAF_UINT32, LEAF_R, &instance->timeOfLastGmChangeEvent, NULL, NULL);
	LEAF_INIT(&module->current_parameter_data_set, timeOfLastGmPhaseChangeEvent, LEAF_UINT32, LEAF_R, &instance->timeOfLastGmPhaseChangeEvent, NULL, NULL);
	LEAF_INIT(&module->current_parameter_data_set, timeOfLastGmFreqChangeEvent, LEAF_UINT32, LEAF_R, &instance->timeOfLastGmFreqChangeEvent, NULL, NULL);

	LEAF_INIT(&module->parent_parameter_data_set, parentPortIdentity, LEAF_PORT_IDENTITY, LEAF_R, NULL, NULL, NULL); /* FIXME implement information, 14.4.1 */
	LEAF_INIT(&module->parent_parameter_data_set, cumulativeRateRatio, LEAF_INT32, LEAF_R, &instance->cumulativeRateRatio, NULL, NULL);
	LEAF_INIT(&module->parent_parameter_data_set, grandMasterIdentity, LEAF_CLOCK_IDENTITY, LEAF_R, &instance->params.gm_priority.u.s.root_system_identity.u.s.clock_identity, NULL, NULL);
	LEAF_INIT(&module->parent_parameter_data_set, grandMasterClockClass, LEAF_UINT8, LEAF_R, &instance->params.gm_priority.u.s.root_system_identity.u.s.clock_quality.clock_class, NULL, NULL);
	LEAF_INIT(&module->parent_parameter_data_set, grandMasterClockAccuracy, LEAF_UINT8, LEAF_R, &instance->params.gm_priority.u.s.root_system_identity.u.s.clock_quality.clock_accuracy, NULL, NULL);
	LEAF_INIT(&module->parent_parameter_data_set, grandMasterOffsetScaledLogVariance, LEAF_UINT16, LEAF_R, &instance->params.gm_priority.u.s.root_system_identity.u.s.clock_quality.offset_scaled_log_variance, offset_scaled_log_variance_handler, NULL);
	LEAF_INIT(&module->parent_parameter_data_set, grandMasterPriority1, LEAF_UINT8, LEAF_R, &instance->params.gm_priority.u.s.root_system_identity.u.s.priority_1, NULL, NULL);
	LEAF_INIT(&module->parent_parameter_data_set, grandMasterPriority2, LEAF_UINT8, LEAF_R, &instance->params.gm_priority.u.s.root_system_identity.u.s.priority_2, NULL, NULL);

	LEAF_INIT(&module->time_properties_parameter_data_set, currentUtcOffset, LEAF_INT16, LEAF_R, &instance->params.current_utc_offset, NULL, NULL);
	LEAF_INIT(&module->time_properties_parameter_data_set, currentUtcOffsetValid, LEAF_BOOL, LEAF_R, &instance->params.current_utc_offset_valid, NULL, NULL);
	LEAF_INIT(&module->time_properties_parameter_data_set, leap59, LEAF_BOOL, LEAF_R, &instance->params.leap59, NULL, NULL);
	LEAF_INIT(&module->time_properties_parameter_data_set, leap61, LEAF_BOOL, LEAF_R, &instance->params.leap61, NULL, NULL);
	LEAF_INIT(&module->time_properties_parameter_data_set, timeTraceable, LEAF_BOOL, LEAF_R, &instance->params.time_traceable, NULL, NULL);
	LEAF_INIT(&module->time_properties_parameter_data_set, frequencyTraceable, LEAF_BOOL, LEAF_R, &instance->params.frequency_traceable, NULL, NULL);
	LEAF_INIT(&module->time_properties_parameter_data_set, timeSource, LEAF_UINT8, LEAF_R, &instance->params.time_source, NULL, NULL);

	LIST_ENTRY_INIT(&module->port_parameter_data_set, port[0]);
	LIST_ENTRY_INIT(&module->port_parameter_statistics, port[0]);

	LEAF_INIT(&module->port_parameter_data_set.port[0], portID, LEAF_UINT16, LEAF_R, (void *)offsetof(struct gptp_port, port_id), NULL, NULL);
	LEAF_INIT(&module->port_parameter_data_set.port[0], portIdentity, LEAF_PORT_IDENTITY, LEAF_R, (void *)offsetof(struct gptp_port, params.port_priority.u.s.source_port_identity), NULL, NULL);
	LEAF_INIT(&module->port_parameter_data_set.port[0], portRole, LEAF_UINT8, LEAF_R, NULL, selected_role_handler, instance);
	LEAF_INIT(&module->port_parameter_data_set.port[0], pttPortEnabled, LEAF_BOOL, LEAF_RW, (void *)offsetof(struct gptp_port, params.ptp_port_enabled), ptp_port_enabled_handler, NULL);
	LEAF_INIT(&module->port_parameter_data_set.port[0], isMeasuringDleay, LEAF_BOOL, LEAF_R, (void *)offsetof(struct gptp_port_common, md.globals.isMeasuringDelay), ptp_port_common_handler, NULL);
	LEAF_INIT(&module->port_parameter_data_set.port[0], asCapable, LEAF_BOOL, LEAF_R, (void *)offsetof(struct gptp_port, params.as_capable), NULL, NULL);
	LEAF_INIT(&module->port_parameter_data_set.port[0], neighborPropDelay, LEAF_USCALED_NS, LEAF_R, (void *)offsetof(struct gptp_port_common, params.mean_link_delay), ptp_port_common_handler, NULL);
	LEAF_INIT(&module->port_parameter_data_set.port[0], neighborPropDelayThresh, LEAF_USCALED_NS, LEAF_RW, (void *)offsetof(struct gptp_port_common, md.globals.meanLinkDelayThresh), ptp_port_common_handler, NULL);
	LEAF_INIT(&module->port_parameter_data_set.port[0], delayAsymmetry, LEAF_SCALED_NS, LEAF_RW, (void *)offsetof(struct gptp_port_common, params.delay_asymmetry), ptp_port_common_handler, NULL);
	LEAF_INIT(&module->port_parameter_data_set.port[0], neighborRateRatio, LEAF_INT32, LEAF_R, (void *)offsetof(struct gptp_port_common, params.neighbor_rate_ratio), neighbor_rate_ratio_handler, NULL);
	LEAF_INIT(&module->port_parameter_data_set.port[0], initialLogAnnounceInterval, LEAF_INT8, LEAF_RW, (void *)offsetof(struct gptp_port, params.initial_log_announce_interval), NULL, NULL);
	LEAF_INIT(&module->port_parameter_data_set.port[0], currentLogAnnounceInterval, LEAF_INT8, LEAF_R, (void *)offsetof(struct gptp_port, params.current_log_announce_interval), NULL, NULL);
	LEAF_INIT(&module->port_parameter_data_set.port[0], announceReceiptTimeout, LEAF_UINT8, LEAF_RW, (void *)offsetof(struct gptp_port, announce_receipt_timeout), NULL, NULL); /* FIXME implement interval update, or just wait for next announce? */
	LEAF_INIT(&module->port_parameter_data_set.port[0], initialLogSyncInterval, LEAF_INT8, LEAF_RW, (void *)offsetof(struct gptp_port, params.initial_log_sync_interval), NULL, NULL);
	LEAF_INIT(&module->port_parameter_data_set.port[0], currentLogSyncInterval, LEAF_INT8, LEAF_R, (void *)offsetof(struct gptp_port, params.current_log_sync_interval), NULL, NULL);
	LEAF_INIT(&module->port_parameter_data_set.port[0], syncReceiptTimeout, LEAF_UINT8, LEAF_RW, (void *)offsetof(struct gptp_port, sync_receipt_timeout), NULL, NULL); /* FIXME implement interval update, or just wait for next sync? */
	LEAF_INIT(&module->port_parameter_data_set.port[0], syncReceiptTimeoutTimeInterval, LEAF_USCALED_NS, LEAF_R, (void *)offsetof(struct gptp_port, params.sync_receipt_timeout_time_interval), NULL, NULL);
	LEAF_INIT(&module->port_parameter_data_set.port[0], initialLogPdelayReqInterval, LEAF_INT8, LEAF_RW, (void *)offsetof(struct gptp_port_common, md.globals.initialLogPdelayReqInterval), ptp_port_common_handler, NULL);
	LEAF_INIT(&module->port_parameter_data_set.port[0], currentLogPdelayReqInterval, LEAF_INT8, LEAF_R, (void *)offsetof(struct gptp_port_common, md.globals.currentLogPdelayReqInterval), ptp_port_common_handler, NULL);
	LEAF_INIT(&module->port_parameter_data_set.port[0], allowedLostResponses, LEAF_UINT16, LEAF_RW, (void *)offsetof(struct gptp_port_common, md.globals.allowedLostResponses), ptp_port_common_handler, NULL);
	LEAF_INIT(&module->port_parameter_data_set.port[0], allowedFaults, LEAF_UINT16, LEAF_RW, (void *)offsetof(struct gptp_port_common, md.globals.allowedFaults), ptp_port_common_handler, NULL);
	LEAF_INIT(&module->port_parameter_data_set.port[0], versionNumber, LEAF_UINT8, LEAF_R, NULL, version_number_handler, instance);

	LEAF_INIT(&module->port_parameter_statistics.port[0], portID, LEAF_UINT16, LEAF_R, (void *)offsetof(struct gptp_port, port_id), NULL, NULL);
	LEAF_INIT(&module->port_parameter_statistics.port[0], rxSyncCount, LEAF_UINT32, LEAF_R, (void *)offsetof(struct gptp_port, stats.num_rx_sync), NULL, NULL);
	LEAF_INIT(&module->port_parameter_statistics.port[0], rxFollowUpCount, LEAF_UINT32, LEAF_R, (void *)offsetof(struct gptp_port, stats.num_rx_fup), NULL, NULL);
	LEAF_INIT(&module->port_parameter_statistics.port[0], rxPdelayRequestCount, LEAF_UINT32, LEAF_R, (void *)offsetof(struct gptp_port_common, stats.num_rx_pdelayreq), ptp_port_common_handler, NULL);
	LEAF_INIT(&module->port_parameter_statistics.port[0], rxPdelayResponseCount, LEAF_UINT32, LEAF_R, (void *)offsetof(struct gptp_port_common, stats.num_rx_pdelayresp), ptp_port_common_handler, NULL);
	LEAF_INIT(&module->port_parameter_statistics.port[0], rxPdelayResponseFollowUpCount, LEAF_UINT32, LEAF_R,(void *)offsetof(struct gptp_port_common, stats.num_rx_pdelayrespfup), ptp_port_common_handler, NULL);
	LEAF_INIT(&module->port_parameter_statistics.port[0], rxAnnounceCount, LEAF_UINT32, LEAF_R, (void *)offsetof(struct gptp_port, stats.num_rx_announce), NULL, NULL);

	LEAF_INIT(&module->port_parameter_statistics.port[0], rxPTPPacketDiscardCount, LEAF_UINT32, LEAF_R, (void *)offsetof(struct gptp_port_common, stats.num_rx_ptp_packet_discard), ptp_port_common_handler, NULL);
	LEAF_INIT(&module->port_parameter_statistics.port[0], syncReceiptTimeoutCount, LEAF_UINT32, LEAF_R, (void *)offsetof(struct gptp_port, stats.num_rx_sync_timeout), NULL, NULL);
	LEAF_INIT(&module->port_parameter_statistics.port[0], announceReceiptTimeoutCount, LEAF_UINT32, LEAF_R, (void *)offsetof(struct gptp_port, stats.num_rx_announce_timeout), NULL, NULL);
	LEAF_INIT(&module->port_parameter_statistics.port[0], pdelayAllowedLostResponsesExceededCount, LEAF_UINT32, LEAF_R, (void *)offsetof(struct gptp_port_common, stats.num_rx_pdelayresp_lost_exceeded), ptp_port_common_handler, NULL);
	LEAF_INIT(&module->port_parameter_statistics.port[0], txSyncCount, LEAF_UINT32, LEAF_R, (void *)offsetof(struct gptp_port, stats.num_tx_sync), NULL, NULL);
	LEAF_INIT(&module->port_parameter_statistics.port[0], txFollowUpCount, LEAF_UINT32, LEAF_R, (void *)offsetof(struct gptp_port, stats.num_tx_fup), NULL, NULL);
	LEAF_INIT(&module->port_parameter_statistics.port[0], txPdelayRequestCount, LEAF_UINT32, LEAF_R, (void *)offsetof(struct gptp_port_common, stats.num_tx_pdelayreq), ptp_port_common_handler, NULL);
	LEAF_INIT(&module->port_parameter_statistics.port[0], txPdelayResponseCount, LEAF_UINT32, LEAF_R, (void *)offsetof(struct gptp_port_common, stats.num_tx_pdelayresp), ptp_port_common_handler, NULL);
	LEAF_INIT(&module->port_parameter_statistics.port[0], txPdelayResponseFollowUpCount, LEAF_UINT32, LEAF_R, (void *)offsetof(struct gptp_port_common, stats.num_tx_pdelayrespfup), ptp_port_common_handler, NULL);
	LEAF_INIT(&module->port_parameter_statistics.port[0], txAnnounceCount, LEAF_UINT32, LEAF_R, (void *)offsetof(struct gptp_port, stats.num_tx_announce), NULL, NULL);
}
