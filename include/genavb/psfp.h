/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief GenAVB/TSN public API
 \details 802.1Q Per-Stream Filtering and Policing definitions.

 \copyright Copyright 2023-2024 NXP
*/
#ifndef _GENAVB_PUBLIC_PSFP_H_
#define _GENAVB_PUBLIC_PSFP_H_

#include "clock.h"
#include "types.h"

/** \cond RTOS */

/**
 * \defgroup psfp		802.1Q Per-Stream Filtering and Policing
 * \ingroup library
 *
 * This API is used to manage Per-Stream Filtering and Policing of a given network bridge or port (as defined in IEEE 802.1Q-2018 Sections 8.6 and 12.31).
 *
 * \defgroup psfp_sg	802.1Q Stream Gate
 * \ingroup psfp
 *
 * \defgroup psfp_sf	802.1Q Stream Filter
 * \ingroup psfp
 *
 * \defgroup psfp_fm	802.1Q Flow Meter
 * \ingroup psfp
 */

/**
 * \ingroup psfp
 */
#define GENAVB_STREAM_HANDLE_WILDCARD (0xFFFFFFFF)

/**
 * \ingroup psfp
 */
#define GENAVB_PRIORITY_SPEC_WILDCARD (0xFF)

/**
 * \ingroup psfp
 */
#define GENAVB_IPV_SPEC_NULL (0xFF)

/**
 * \ingroup psfp_sf
 * PSFP Stream Filter Instance
 *  IEEE 802.1Qcw-D15 - 48.2.11 ieee802-dot1q-psfp-bridge YANG module
 */

struct genavb_stream_filter_instance {
	uint32_t stream_filter_instance_id;
	uint32_t stream_handle;
	uint8_t priority_spec;
	uint32_t max_sdu_size;
	bool stream_blocked_due_to_oversize_frame_enabled;
	bool stream_blocked_due_to_oversize_frame; 
	uint32_t stream_gate_ref;
	uint32_t flow_meter_ref;
	bool flow_meter_enable;

	uint64_t matching_frames_count;
	uint64_t passing_frames_count;
	uint64_t not_passing_frames_count;
	uint64_t red_frames_count;
	uint64_t passing_sdu_count;
	uint64_t not_passing_sdu_count;
};

/**
 * \ingroup psfp_sg
 * Stream gate configuration type
 */
typedef enum {
	GENAVB_SG_OPER,
	GENAVB_SG_ADMIN
} genavb_sg_config_type_t;

/**
 * \ingroup psfp_sg
 * Stream gate operations
 * 802.1Qci-2017 - Table 8.7 Stream Gate operations
 */
typedef enum {
	GENAVB_SG_SET_GATE_AND_IPV,    /**< SetGateAndIPV */
} genavb_sg_operations_t;

/**
 * \ingroup psfp_sg
 * Stream gate control entry
 */
struct genavb_stream_gate_control_entry {
	uint8_t operation_name;
	uint8_t gate_state_value;
	uint8_t ipv_spec;
	uint32_t time_interval_value;
	uint32_t interval_octet_max;
};

/**
 * \ingroup psfp_sg
 * Stream gate instance
 */
struct genavb_stream_gate_instance {
	uint32_t stream_gate_instance_id;
	bool gate_enable;
	uint8_t admin_gate_state;
	uint8_t admin_ipv;
	uint32_t cycle_time_p;
	uint32_t cycle_time_q;
	uint32_t cycle_time_extension;
	uint64_t base_time;
	bool gate_closed_due_to_invalid_rx_enable;
	bool gate_closed_due_to_invalid_rx; /**< only used on read API, ignored on update */
	bool gate_closed_due_to_octets_exceeded_enable;
	bool gate_closed_due_to_octets_exceeded; /**< only used on read API, ignored on update */
	unsigned int list_length;
	struct genavb_stream_gate_control_entry *control_list;
};

/**
 * \ingroup psfp_fm
 * Flow meter 
 * IEEE 802.1Qcw-D15 - 48.2.11 ieee802-dot1q-psfp-bridge YANG module
 */

struct genavb_flow_meter_instance {
	uint32_t flow_meter_instance_id;
	uint64_t committed_information_rate;
	uint32_t committed_burst_size;
	uint64_t excess_information_rate;
	uint32_t excess_burst_size;
	uint8_t coupling_flag;
	uint8_t color_mode;
	bool drop_on_yellow;
	bool mark_all_frames_red_enable;
	bool mark_all_frames_red;
};

/** \endcond */

/* OS specific headers */
#include "os/psfp.h"

#endif /* _GENAVB_PUBLIC_PSFP_H_ */
