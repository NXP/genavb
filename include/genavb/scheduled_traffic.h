/*
 * Copyright 2020, 2022-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief GenAVB/TSN public API
 \details 802.1Q Scheduled Traffic definitions.

 \copyright Copyright 2020, 2022-2024 NXP
*/
#ifndef _GENAVB_PUBLIC_SCHEDULED_TRAFFIC_H_
#define _GENAVB_PUBLIC_SCHEDULED_TRAFFIC_H_

#include "clock.h"
#include "types.h"

/** \cond RTOS */

/**
 * \defgroup st	            802.1Q Scheduled Traffic
 * \ingroup library
 *
 * This API is used to configure the scheduled traffic feature of a given network port (as defined in IEEE 802.1Q-2018 Section 8.6.9).
 * Scheduled traffic allows the configuration of a sequence of gate states per port which determine the set of traffic classes that are allowed to transmit at any given time.
 *
 * This feature requires specific hardware support and returns an error if the corresponding network port doesn't support it.
 *
 * The administrative configuration is set using ::genavb_st_set_admin_config.
 * Parameters:
 * * port_id: the logical port ID.
 * * clk_id: the ::genavb_clock_id_t clock ID domain. The times provided in the configuration are based on this clock reference.
 * * config: the ::genavb_st_get_config configuration
 *   + enable: 0 or 1, if 0 scheduled traffic is disabled
 *   + base_time (nanoseconds): the instant defining the time when the scheduling starts. If base_time is in the past, the scheduling will start when base_time + (N * cycle_time_p / cycle_time_q) is greater than "now". (N being the smallest integer making the equation true)
 *   + cycle_time_p and cycle_time_q (seconds): the scheduling cycle time in rational format. It is the time when the list should be repeated. If the provided list is longer than the cycle time, the list will be truncated.
 *   + cycle_time_ext (nanoseconds): the amount of time that the current gating cycle can be extended when a new cycle configuration is configured.
 *   + control_list: the ::genavb_st_gate_control_entry control list (see description below)
 *
 * Control list description:
 * * it is an array of ::genavb_st_gate_control_entry elements of list_length length.
 * * operation: the gate operation ::genavb_st_operations_t. (Note: only ::GENAVB_ST_SET_GATE_STATES is supported)
 * * gate_states: a bit mask in which the bit in position N refers to the traffic class N. If the bit is set, the traffic class is allowed to transmit, if the bit is not set the traffic class is not allowed to transmit.
 * * time_interval (nanoseconds): the duration of the state defined by operation and gate_states before moving to the next entry.
 *
 */

#define QOS_MAX_PDU_BYTES 0x600U
#define QOS_MAX_SDU_BYTES (QOS_MAX_PDU_BYTES - 12 - 4)

/**
 * \ingroup st
 * Scheduled traffic configuration type
 */
typedef enum {
	GENAVB_ST_OPER,
	GENAVB_ST_ADMIN
} genavb_st_config_type_t;

/**
 * \ingroup st
 * Scheduled traffic gate operations
 * 802.1Q-2018 - Table 8.7 Gate operations
 */
typedef enum {
	GENAVB_ST_SET_GATE_STATES,    /**< SetGateStates */
	GENAVB_ST_SET_AND_HOLD_MAC,   /**< Set-And-Hold-MAC */
	GENAVB_ST_SET_AND_RELEASE_MAC /**< Set-And-Release-MAC */
} genavb_st_operations_t;

/**
 * \ingroup st
 * Scheduled traffic gate control entry
 * 802.1Q-2018 - 12.29.1.2.1 GateControlEntry
 */
struct genavb_st_gate_control_entry {
	uint8_t operation;	/**< Table 8.7 Operation */
	uint8_t gate_states;	/**< 12.29.1.2.2 gateStatesValue */
	uint32_t time_interval; /**< 12.29.1.2.3 timeIntervalValue */
};

/**
 * \ingroup st
 * Scheduled traffic configuration
 * 802.1Q-2018 - Table 12-29
 */
struct genavb_st_config {
	int enable;					   /**< GateEnabled */
	uint64_t base_time;				   /**< AdminBaseTime or OperBaseTime in nanoseconds */
	uint32_t cycle_time_p;				   /**< AdminCycleTime or OperCycleTime (numerator) in seconds */
	uint32_t cycle_time_q;				   /**< AdminCycleTime or OperCycleTime (denominator) in seconds */
	uint32_t cycle_time_ext;			   /**< AdminCycleTimeExtension or OperCycleTimeExtension in nanoseconds */
	unsigned int list_length;			   /**< AdminControlListLength or OperControlListLength */
	struct genavb_st_gate_control_entry *control_list; /**< AdminControlList or OperControlList */
};

/**
 * \ingroup st
 * Scheduled traffic queue max SDU
 * 802.1Q-2018 - Table 12-29
 */
struct genavb_st_max_sdu {
	uint8_t traffic_class; /* port traffic class*/
	uint32_t queue_max_sdu; /* max SDU size*/
	uint64_t transmission_overrun; /* transmission overrun counter */
};

/** \endcond */

/* OS specific headers */
#include "os/scheduled_traffic.h"

#endif /* _GENAVB_PUBLIC_SCHEDULED_TRAFFIC_H_ */
