/*
 * Copyright 2020-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief SRP managed objects definition
 @details
*/

#ifndef _SRP_MANAGED_OBJECTS_H_
#define _SRP_MANAGED_OBJECTS_H_

#include "common/managed_objects.h"

#define SRP_MAX_NODES 5

#define SRP_BRIDGE_BASE_NUM_LEAVES 4
#define SRP_BRIDGE_PORT_NUM_LEAVES 5
#define SRP_LATENCY_PARAMS_NUM_LEAVES 3
#define SRP_STREAM_NUM_LEAVES 7
#define SRP_RESERVATIONS_NUM_LEAVES 9


/* Managed object tree definition, 802.1Q-2018 */
MODULE(srp_managed_objects, SRP_MAX_NODES,
	/* Stream Reservation Protocol (SRP) entities */

	/*
	12.22.1 SRP Bridge Base Table
	There is a set of parameters that configure SRP operation for the entire device.
	*/
	CONTAINER(bridge_base_table, SRP_BRIDGE_BASE_NUM_LEAVES,
		LEAF(msrpEnabledStatus);
		LEAF(talkerPruning);
		LEAF(msrpMaxFanInPorts);
		LEAF(msrpLatencyMaxFrameSize);
	);

	/*
	12.22.2 SRP Bridge Port Table
	There is one SRP Configuration Parameter Table per Port of a Bridge component.
	*/
	LIST(bridge_port_table, 1,
		LIST_ENTRY(port, 1, SRP_BRIDGE_PORT_NUM_LEAVES,
			LEAF(portID);
			LEAF(msrpPortEnabledStatus);
			LEAF(FailedRegistrations);
			LEAF(LastPDUOrigin);
			LEAF(SR_PVID);
		);
	);

	/*
	12.22.3 SRP Latency Parameter Table
	There is one SRP Latency Parameter Table per Port of a Bridge component.
	*/
	LIST(latency_parameter_table, 1,
		LIST_ENTRY(latency, 1, SRP_LATENCY_PARAMS_NUM_LEAVES,
			LEAF(portID);
			LEAF(TrafficClass);
			LEAF(portTcMaxLatency);
		);
	);

	/*
	12.22.4 SRP Stream Table
	There is one SRP Stream Table per Bridge component.
	*/
	LIST(streams_table, 1,
		LIST_ENTRY(stream, 1, SRP_STREAM_NUM_LEAVES,
			LEAF(StreamID);
			LEAF(StreamDestinationAddress);
			LEAF(StreamVID);
			LEAF(MaxFrameSize);
			LEAF(MaxIntervalFrames);
			LEAF(DataFramePriority);
			LEAF(Rank);
		);
	);

	/*
	12.22.5 SRP Reservations Table
	There is one SRP Reservations Table per reservation direction per port of a Bridge component.
	*/
	LIST(reservations_table, 1,
		LIST_ENTRY(reservation, 1, SRP_RESERVATIONS_NUM_LEAVES,
			LEAF(portID);
			LEAF(StreamID);
			LEAF(Direction);
			LEAF(DeclarationType);
			LEAF(AccumulatedLatency);
			LEAF(FailedBridgeId);
			LEAF(FailureCode);
			LEAF(DroppedFrames);
			LEAF(StreamAge);
		);
	);
);


struct srp_ctx;

void srp_managed_objects_init(struct srp_managed_objects *module, struct srp_ctx *srp);
unsigned int srp_managed_objects_get(struct srp_managed_objects *module, u8 *in, u8 *in_end, u8 *out, u8 *out_end);
unsigned int srp_managed_objects_set(struct srp_managed_objects *module, u8 *in, u8 *in_end, u8 *out, u8 *out_end);

#endif /* _SRP_MANAGED_OBJECTS_H_ */
