/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Common FQTSS service implementation
 @details
*/

#ifndef _COMMON_FQTSS_H_
#define _COMMON_FQTSS_H_

#include "common/types.h"
#include "config.h"

/* TODO : 802.1Q-2022
 * - Tranmission Selection Algorithm Table 12-5
 * - Priority Regeneration Override Table 12-6
 * - SR Class to Priority Mapping Table 12-7
 */

#define FQTSS_MAX_TRAFFIC_CLASS 8
#define DEFAULT_HIGHEST_TRAFFIC_CLASS_DELTA_BANDWIDTH 75

/* As per 802.1Q-2022 Table 12-4 Bandwidth Availability Parameter Table */
struct bandwidth_availability_params {
	uint8_t traffic_class;
	unsigned int delta_bandwidth;
	uint64_t admin_idle_slope;
	uint64_t oper_idle_slope;
	unsigned int class_measurement_interval;
	bool lock_class_bandwidth;
};

/* As per 802.1Q-2022 12.20 Management Entities for FQTSS and 34.3 Bandwidth availability parameters */
struct fqtss_port_params {
	struct bandwidth_availability_params bandwidth_availability[FQTSS_MAX_TRAFFIC_CLASS];
	uint64_t total_oper_idle_slope;	/* operIdleSlope for all enabled SR classes. */
	uint64_t port_transmit_rate;	/* portTransmitRate as reported by MAC layer. */
};

int common_fqtss_init(void);
void common_fqtss_exit(void);
int common_fqtss_stream_add(unsigned int port_id, void *stream_id, uint16_t vlan_id, uint8_t priority, uint64_t idle_slope, bool is_bridge);
int common_fqtss_stream_remove(unsigned int port_id, void *stream_id, uint16_t vlan_id, uint8_t priority, uint64_t idle_slope, bool is_bridge);
int common_fqtss_set_port_transmit_rate(unsigned int port_id, uint64_t rate);

#endif /* _COMMON_FQTSS_H_ */
