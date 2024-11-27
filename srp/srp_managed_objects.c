/*
 * Copyright 2020-2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief SRP managed objects definition
 @details
*/

#include "srp_managed_objects.h"
#include "srp.h"

typedef struct bridge_port_params {
	unsigned int port_idx;
} bridge_port_params_t;

static bridge_port_params_t port_params;

static void port_params_reset(void)
{
	port_params.port_idx = 0;
}

static uintptr_t bridge_port_handler(void *data, uintptr_t base)
{
	struct msrp_ctx *msrp = data;

	/* if first iteration in the list, start with the first element */
	if (!base) {
		port_params_reset();
	} else {
		port_params.port_idx++;
		if (port_params.port_idx >= msrp->port_max)
			return 0;
	}

	return (uintptr_t)&msrp->port[port_params.port_idx];
}

typedef struct latency_list_params {
	unsigned int port_idx;
	unsigned int class_idx;
} latency_list_params_t;

static latency_list_params_t latency_params;

static void latency_list_reset(void)
{
	latency_params.port_idx = 0;
	latency_params.class_idx = 0;
}

static uintptr_t latency_list_handler(void *data, uintptr_t base)
{
	struct msrp_ctx *msrp = (struct msrp_ctx *)data;

	/* if first iteration in the list, start with the first element */
	if (!base) {
		latency_list_reset();
	} else {
		latency_params.class_idx++;
		if (latency_params.class_idx >= CFG_TRAFFIC_CLASS_MAX) {
			latency_params.class_idx = 0;
			latency_params.port_idx++;
			if (latency_params.port_idx >= msrp->port_max)
				return 0;
		}
	}

	return (uintptr_t)&msrp->port[latency_params.port_idx].latency[latency_params.class_idx];
}

static uintptr_t streams_list_handler(void *data, uintptr_t base)
{
	struct list_head *head = data;
	struct list_head *entry;

	/* if first iteration in the list, start with the head */
	if (!base)
		entry = head;
	else
		entry = &((struct msrp_stream *)base)->list;

	if ((entry = list_next(entry)) != head)
		return (uintptr_t)container_of(entry, struct msrp_stream, list);

	return 0;
}

typedef struct reservations_list_params {
	unsigned int port_id;
	unsigned int direction;
	struct list_head *entry;
} reservations_list_params_t;

static reservations_list_params_t reservations_params;

static void reservations_list_reset(struct msrp_ctx *msrp)
{
	reservations_params.port_id = 0;
	reservations_params.direction = 0;
	reservations_params.entry = list_next(&msrp->map[0].streams);
}

static void reservations_list_next(struct msrp_ctx *msrp)
{
	/* check next port on same entry if both direction have been parsed */
	reservations_params.direction++;
	if (reservations_params.direction > 1) {
		reservations_params.direction = 0;
		reservations_params.port_id++;
	}

	if (reservations_params.port_id >= msrp->port_max) {
		/* check next entry in the list once both direction on all ports have been parsed for the previous entry */
		reservations_params.entry = list_next(reservations_params.entry);
		reservations_params.port_id = 0;
		reservations_params.direction = 0;
	}
}

static uintptr_t reservations_list_handler(void *data, uintptr_t base)
{
	struct msrp_ctx *msrp = data;
	struct msrp_stream *stream;
	struct msrp_stream_port *stream_port;

	/* if first iteration in the list, start with the head, first port */
	if (!base)
		reservations_list_reset(msrp);
	else
		reservations_list_next(msrp);

	while (reservations_params.entry != &msrp->map[0].streams) {
		stream = (struct msrp_stream *)container_of(reservations_params.entry, struct msrp_stream, list);
		stream_port = &stream->port[reservations_params.port_id];
		if (stream_port->reservations[reservations_params.direction].valid)
			return  (uintptr_t)&stream_port->reservations[reservations_params.direction];
		else
			reservations_list_next(msrp);
	}

	return 0;
}

static void reservation_to_stream_id_handler(void *data, struct leaf *l, enum node_operation operation, uint8_t *buf, uintptr_t base)
{
	struct msrp_reservation *reservation;
	struct msrp_stream_port *stream_port;
	struct msrp_stream *stream;
	u64 val;

	switch (operation) {
	case NODE_GET:
		reservation = (struct msrp_reservation *)base;
		stream_port = container_of(reservation, struct msrp_stream_port, reservations[reservation->direction]);
		stream = container_of(stream_port, struct msrp_stream, port[reservation->port_id]);
		val = *((u64 *)((uintptr_t)stream + l->val));
		val = ntohll(val);
		os_memcpy(buf, &val, sizeof(u64));
		break;

	case NODE_SET:
		break;

	default:
		break;
	}
}

static void ntohs_param_handler(void *data, struct leaf *l, enum node_operation operation, uint8_t *buf, uintptr_t base)
{
	u16 val;

	switch (operation) {
	case NODE_GET:
		val = *((u16 *)(base + l->val));
		val = ntohs(val);
		os_memcpy(buf, &val, sizeof(u16));
		break;

	case NODE_SET:
		break;

	default:
		break;
	}
}

static void ntohl_param_handler(void *data, struct leaf *l, enum node_operation operation, uint8_t *buf, uintptr_t base)
{
	u32 val;

	switch (operation) {
	case NODE_GET:
		val = *((u32 *)(base + l->val));
		val = ntohl(val);
		os_memcpy(buf, &val, sizeof(u32));
		break;

	case NODE_SET:
		break;

	default:
		break;
	}
}

static void ntohll_param_handler(void *data, struct leaf *l, enum node_operation operation, uint8_t *buf, uintptr_t base)
{
	u64 val;

	switch (operation) {
	case NODE_GET:
		val = *((u64 *)(base + l->val));
		val = ntohll(val);
		os_memcpy(buf, &val, sizeof(u64));
		break;

	case NODE_SET:
		break;

	default:
		break;
	}
}


static void tspec_param_handler(void *data, struct leaf *l, enum node_operation operation, uint8_t *buf, uintptr_t base)
{
	u32 val;

	switch (operation) {
	case NODE_GET:
		val = ntohs(*((u16 *)(base + l->val)));
		os_memcpy(buf, &val, sizeof(u32));
		break;

	case NODE_SET:
		break;

	default:
		break;
	}
}

static void data_frame_priority_param_handler(void *data, struct leaf *l, enum node_operation operation, uint8_t *buf, uintptr_t base)
{
	struct msrp_pdu_fv_talker_advertise *fv = (struct msrp_pdu_fv_talker_advertise *)(base + l->val);
	u32 val;

	switch (operation) {
	case NODE_GET:
		val = (u32)fv->priority;
		os_memcpy(buf, &val, sizeof(u32));
		break;

	case NODE_SET:
		break;

	default:
		break;
	}
}

static void rank_param_handler(void *data, struct leaf *l, enum node_operation operation, uint8_t *buf, uintptr_t base)
{
	struct msrp_pdu_fv_talker_advertise *fv = (struct msrp_pdu_fv_talker_advertise *)(base + l->val);
	u32 val;

	switch (operation) {
	case NODE_GET:
		val = (u32)fv->rank;
		os_memcpy(buf, &val, sizeof(u32));
		break;

	case NODE_SET:
		break;

	default:
		break;
	}
}

static void msrp_port_enabled_handler(void *data, struct leaf *l, enum node_operation operation, uint8_t *buf, uintptr_t base)
{
	struct msrp_port *port = (struct msrp_port *)base;

	switch (operation) {
	case NODE_GET:
		buf[0] = *((bool *)(base + l->val));
		break;

	case NODE_SET:
		*((bool *)(base + l->val)) = buf[0];
		if (buf[0])
			msrp_port_enable(port);
		else
			msrp_port_disable(port);

		break;

	default:
		break;
	}
}

static void msrp_enabled_handler(void *data, struct leaf *l, enum node_operation operation, uint8_t *buf, uintptr_t base)
{
	struct msrp_ctx *msrp = data;

	switch (operation) {
	case NODE_GET:
		buf[0] = *((bool *)(base + l->val));
		break;

	case NODE_SET:
		if (buf[0])
			msrp_enable(msrp);
		else
			msrp_disable(msrp);

		break;

	default:
		break;
	}
}

unsigned int srp_managed_objects_get(struct srp_managed_objects *module, u8 *in, u8 *in_end, u8 *out, u8 *out_end)
{
	uint8_t *end = module_iterate(&module->node, NODE_GET, in, in_end, out, out_end);

	return end - out;
}

unsigned int srp_managed_objects_set(struct srp_managed_objects *module, u8 *in, u8 *in_end, u8 *out, u8 *out_end)
{
	uint8_t *end = module_iterate(&module->node, NODE_SET, in, in_end, out, out_end);

	return end - out;
}

__init void srp_managed_objects_init(struct srp_managed_objects *module, struct srp_ctx *srp)
{
	uint16_t keys_id[LIST_KEY_MAX];
	struct msrp_ctx *msrp = srp->msrp;

	MODULE_INIT(module, "ieee-srp");

	/*
	12.22.1 SRP Bridge Base Table
	There is a set of parameters that configure SRP operation for the entire device.
	*/
	CONTAINER_INIT(module, bridge_base_table);
	LEAF_INIT(&module->bridge_base_table, msrpEnabledStatus, LEAF_BOOL, LEAF_RW, &msrp->msrp_enabled_status, msrp_enabled_handler, msrp);
	LEAF_INIT(&module->bridge_base_table, talkerPruning, LEAF_BOOL, LEAF_R, &msrp->talker_pruning, NULL, NULL);
	LEAF_INIT(&module->bridge_base_table, msrpMaxFanInPorts, LEAF_UINT32, LEAF_R, &msrp->msrp_max_fan_in_ports, NULL, NULL);
	LEAF_INIT(&module->bridge_base_table, msrpLatencyMaxFrameSize, LEAF_UINT32, LEAF_R, &msrp->msrp_latency_max_frame_size, NULL, NULL);

	/*
	12.22.2 SRP Bridge Port Table
	There is one SRP Configuration Parameter Table per Port of a Bridge component.
	*/
	keys_id[0] = 0; /* entry key id (port) */
	LIST_DYNAMIC_INIT(module, bridge_port_table, bridge_port_handler, (void *)msrp, 1, keys_id);
	LIST_ENTRY_INIT(&module->bridge_port_table, port[0]);
	LEAF_INIT(&module->bridge_port_table.port[0], portID, LEAF_UINT16, LEAF_R, (void *)offsetof(struct msrp_port, port_id), NULL, NULL);
	LEAF_INIT(&module->bridge_port_table.port[0], msrpPortEnabledStatus, LEAF_BOOL, LEAF_RW, (void *)offsetof(struct msrp_port, msrp_port_enabled_status), msrp_port_enabled_handler, NULL);
	LEAF_INIT(&module->bridge_port_table.port[0], FailedRegistrations, LEAF_UINT64, LEAF_R, (void *)offsetof(struct msrp_port, failed_registrations), NULL, NULL);
	LEAF_INIT(&module->bridge_port_table.port[0], LastPDUOrigin, LEAF_MAC_ADDRESS, LEAF_R, (void *)offsetof(struct msrp_port, last_pdu_origin), NULL, NULL);
	LEAF_INIT(&module->bridge_port_table.port[0], SR_PVID, LEAF_UINT16, LEAF_R, (void *)offsetof(struct msrp_port, sr_pvid), ntohs_param_handler, NULL);

	/*
	12.22.3 SRP Latency Parameter Table
	There is one SRP Latency Parameter Table per Port of a Bridge component. Rows in the table can be
	created or removed dynamically in implementations that support dynamic configuration of ports and
	components.
	*/
	keys_id[0] = 0; /* 1st key leaf id  (port) */
	keys_id[1] = 1; /* 2nd key leaf id  (TrafficClass) */
	LIST_DYNAMIC_INIT(module, latency_parameter_table, latency_list_handler, (void *)msrp, 2, keys_id);
	LIST_ENTRY_INIT(&module->latency_parameter_table, latency[0]);
	LEAF_INIT(&module->latency_parameter_table.latency[0], portID, LEAF_UINT16, LEAF_R, (void *)offsetof(struct msrp_latency_parameter, port_id), NULL, NULL);
	LEAF_INIT(&module->latency_parameter_table.latency[0], TrafficClass, LEAF_UINT32, LEAF_R, (void *)offsetof(struct msrp_latency_parameter, traffic_class), NULL, NULL);
	LEAF_INIT(&module->latency_parameter_table.latency[0], portTcMaxLatency, LEAF_UINT32, LEAF_R, (void *)offsetof(struct msrp_latency_parameter, port_tc_max_latency), NULL, NULL);

	/*
	12.22.4 SRP Stream Table
	There is one SRP Stream Table per Bridge component. Rows in the table are created and
	removed dynamically as StreamIDs are registered and deregistered on the Bridge.
	*/
	keys_id[0] = 0; /* lookup key leaf's id (StreamID) */
	LIST_DYNAMIC_INIT(module, streams_table, streams_list_handler, (void *)&msrp->map[0].streams, 1, keys_id);
	LIST_ENTRY_INIT(&module->streams_table, stream[0]);
	LEAF_INIT(&module->streams_table.stream[0], StreamID, LEAF_UINT64, LEAF_R, (void *)offsetof(struct msrp_stream, fv.stream_id), ntohll_param_handler, NULL);
	LEAF_INIT(&module->streams_table.stream[0], StreamDestinationAddress, LEAF_MAC_ADDRESS, LEAF_R, (void*)offsetof(struct msrp_stream, fv.data_frame.destination_address[0]), NULL, NULL);
	LEAF_INIT(&module->streams_table.stream[0], StreamVID, LEAF_UINT16, LEAF_R, (void *)offsetof(struct msrp_stream, fv.data_frame.vlan_identifier), ntohs_param_handler, NULL);
	LEAF_INIT(&module->streams_table.stream[0], MaxFrameSize, LEAF_UINT32, LEAF_R, (void *)offsetof(struct msrp_stream, fv.tspec.max_frame_size), tspec_param_handler, NULL);
	LEAF_INIT(&module->streams_table.stream[0], MaxIntervalFrames, LEAF_UINT32, LEAF_R, (void *)offsetof(struct msrp_stream, fv.tspec.max_interval_frames), tspec_param_handler, NULL);
	LEAF_INIT(&module->streams_table.stream[0], DataFramePriority, LEAF_UINT32, LEAF_R, (void *)offsetof(struct msrp_stream, fv), data_frame_priority_param_handler, NULL);
	LEAF_INIT(&module->streams_table.stream[0], Rank, LEAF_UINT32, LEAF_R, (void *)offsetof(struct msrp_stream, fv), rank_param_handler, NULL);

	/*
	12.22.5 SRP Reservations Table
	There is one SRP Reservations Table per reservation direction per port of a Bridge component.
	Rows in the table can be created or removed dynamically as Talker and Listener declarations are
	registered and deregistered on a port of the Bridge.
	*/
	keys_id[0] = 0; /* 1st key leaf id (port) */
	keys_id[1] = 1; /* 2nd key leaf id (stream ID) */
	keys_id[2] = 2; /* 3rd key leaf id (direction) */
	LIST_DYNAMIC_INIT(module, reservations_table, reservations_list_handler, (void *)msrp, 3, keys_id);
	LIST_ENTRY_INIT(&module->reservations_table, reservation[0]);
	LEAF_INIT(&module->reservations_table.reservation[0], portID, LEAF_UINT16, LEAF_R,  (void *)offsetof(struct msrp_reservation, port_id), NULL, NULL);
	LEAF_INIT(&module->reservations_table.reservation[0], StreamID, LEAF_UINT64, LEAF_R, (void*)offsetof(struct msrp_stream, fv.stream_id), reservation_to_stream_id_handler, NULL);
	LEAF_INIT(&module->reservations_table.reservation[0], Direction, LEAF_UINT16, LEAF_R, (void *)offsetof(struct msrp_reservation, direction), NULL, NULL);
	LEAF_INIT(&module->reservations_table.reservation[0], DeclarationType, LEAF_UINT32, LEAF_R, (void *)offsetof(struct msrp_reservation, declaration_type), NULL, NULL);
	LEAF_INIT(&module->reservations_table.reservation[0], AccumulatedLatency, LEAF_UINT32, LEAF_R, (void *)offsetof(struct msrp_reservation, accumulated_latency), ntohl_param_handler, NULL);
	LEAF_INIT(&module->reservations_table.reservation[0], FailedBridgeId, LEAF_UINT64, LEAF_R, (void *)offsetof(struct msrp_reservation, failed_bridge_id), NULL, NULL);
	LEAF_INIT(&module->reservations_table.reservation[0], FailureCode, LEAF_UINT32, LEAF_R, (void *)offsetof(struct msrp_reservation, failure_code), NULL, NULL);
	LEAF_INIT(&module->reservations_table.reservation[0], DroppedFrames, LEAF_UINT64, LEAF_R, (void *)offsetof(struct msrp_reservation, dropped_frames), NULL, NULL);
	LEAF_INIT(&module->reservations_table.reservation[0], StreamAge, LEAF_UINT32, LEAF_R, (void *)offsetof(struct msrp_reservation, stream_age), NULL, NULL);
}
