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

#include "os/string.h"
#include "os/fqtss.h"
#include "os/net.h"

#include "common/log.h"

#include "fqtss.h"

/* Local FQTSS layer data to track bandwidth usage per logical port and per SR class (802.1Q-2022, 12.20) */
static struct fqtss_port_params fqtss_port_params[CFG_MAX_LOGICAL_PORTS];

/** Gets the sum of delta_bandwidth of upper non locked class bandwidth traffic classes of a given port
 * \return			sum of delta_bandwidths
 * \param fqtss_params		pointer to the fqtss_params of the concerned port
 * \param traffic_class		traffic class up to which the higher traffic classes' delta_bandwidths should be added up
 */
static unsigned int common_fqtss_get_total_delta_bandwidth(struct fqtss_port_params *fqtss_params, uint8_t traffic_class)
{
	unsigned int total_delta_bw = 0;
	int i;

	for (i = (FQTSS_MAX_TRAFFIC_CLASS - 1); i >= 0; i--) {
		if (!fqtss_params->bandwidth_availability[i].lock_class_bandwidth)
			total_delta_bw += fqtss_params->bandwidth_availability[i].delta_bandwidth;

		if (i == traffic_class)
			break;
	}

	return total_delta_bw;
}

/** Checks if there is enough bandwidth for the new idle_slope value which is to be set for the traffic class of a given port
 * \return			0 if the new idle_slope value won't exceed the delta_bandwidth, -1 otherwise
 * \param fqtss_params		pointer to the fqtss_params of the concerned port
 * \param traffic_class		traffic class of a given port on which to check whether if the new idle_slope value exceeds the delta_bandwidth or not
 * \param new_idle_slope	new idle_slope value
 */
static int common_fqtss_check_valid_oper_idle_slope(struct fqtss_port_params *fqtss_params, uint8_t traffic_class, uint64_t new_idle_slope)
{
	struct bandwidth_availability_params *bandwidth_availability = &fqtss_params->bandwidth_availability[traffic_class];
	uint64_t traffic_class_bw_limit;
	int rc = 0;

	if (bandwidth_availability->lock_class_bandwidth) {
		traffic_class_bw_limit = (fqtss_params->port_transmit_rate / 100) * bandwidth_availability->delta_bandwidth;

		if (new_idle_slope > traffic_class_bw_limit) {
			os_log(LOG_ERR, "idleslope (%" PRIu64 ") exceeds permitted bandwidth (%" PRIu64 ") of traffic class (%u)\n",
				new_idle_slope, traffic_class_bw_limit, traffic_class);
			rc = -1;
			goto exit;
		}

	} else {
		uint64_t new_total_idle_slope;

		if ((bandwidth_availability->oper_idle_slope > new_idle_slope)
		    && ((bandwidth_availability->oper_idle_slope - new_idle_slope) > fqtss_params->total_oper_idle_slope))
			new_total_idle_slope = 0;
		else
			new_total_idle_slope = fqtss_params->total_oper_idle_slope + (new_idle_slope - bandwidth_availability->oper_idle_slope);

		traffic_class_bw_limit = (fqtss_params->port_transmit_rate / 100) * common_fqtss_get_total_delta_bandwidth(fqtss_params, traffic_class);

		if (new_total_idle_slope > traffic_class_bw_limit) {
			os_log(LOG_ERR, "total idleslope (%" PRIu64 ") exceeds total permitted bandwidth (%" PRIu64 ") of non bandwidth locked traffic classes\n",
				new_total_idle_slope, traffic_class_bw_limit);
			rc = -1;
			goto exit;
		}
	}

exit:
	return rc;
}

/** Updates oper_idle_slope of a traffic class, according to the net port priority, for a given port and call the OS specific FQTSS handler used when adding a stream
 * \return			0 on success, -1 otherwise
 * \param port_id		logical port ID
 * \param stream_id		stream identifier
 * \param vlan_id		stream vlan identifier
 * \param priority		net port priority
 * \param idle_slope		idle_slope value of the new stream, which should be added to the traffic class' oper_idle_slope for the given port
 */
int common_fqtss_stream_add(unsigned int port_id, void *stream_id, uint16_t vlan_id, uint8_t priority, uint64_t idle_slope, bool is_bridge)
{
	struct bandwidth_availability_params *bandwidth_availability;
	struct fqtss_port_params *fqtss_params;
	uint64_t new_idle_slope;
	uint8_t traffic_class;
	int rc = 0;

	if (port_id >= CFG_MAX_LOGICAL_PORTS) {
		os_log(LOG_ERR, "invalid logical_port(%u), it exceeds the maximum number of ports(%u)\n", port_id, CFG_MAX_LOGICAL_PORTS);
		rc = -1;
		goto exit;
	}

	traffic_class = net_port_priority_to_traffic_class(port_id, priority);

	if (traffic_class >= FQTSS_MAX_TRAFFIC_CLASS) {
		os_log(LOG_ERR, "invalid traffic class(%u), it exceeds the maximum number of traffic classes(%u) supported by FQTSS\n", traffic_class, FQTSS_MAX_TRAFFIC_CLASS);
		rc = -1;
		goto exit;
	}

	fqtss_params = &fqtss_port_params[port_id];
	bandwidth_availability = &fqtss_params->bandwidth_availability[traffic_class];

	new_idle_slope = bandwidth_availability->oper_idle_slope + idle_slope;

	rc = common_fqtss_check_valid_oper_idle_slope(fqtss_params, traffic_class, new_idle_slope);
	if (rc < 0)
		goto exit;

	if (is_bridge) {
		if (fqtss_set_oper_idle_slope(port_id, traffic_class, new_idle_slope) < 0) {
			os_log(LOG_ERR, "logical_port(%u) traffic_class(%u) fqtss_set_oper_idle_slope failed to set oper_idle_slope(%" PRIu64 ")\n",
				port_id, traffic_class, new_idle_slope);
			rc = -1;
			goto exit;
		}

	} else {
		if (fqtss_stream_add(port_id, stream_id, vlan_id, priority, idle_slope) < 0) {
			os_log(LOG_ERR, "logical_port(%u) traffic_class(%u) fqtss_stream_add failed to add a stream using oper_idle_slope(%" PRIu64 ")\n",
				port_id, traffic_class, idle_slope);
			rc = -1;
			goto exit;
		}
	}

	bandwidth_availability->oper_idle_slope = new_idle_slope;
	fqtss_params->total_oper_idle_slope += idle_slope;

	os_log(LOG_INFO, "logical_port(%u) fqtss add (%" PRIu64 ", %" PRIu64 ", %" PRIu64 "/%" PRIu64 ")\n", port_id,
	       idle_slope, bandwidth_availability->oper_idle_slope,
	       fqtss_params->total_oper_idle_slope, fqtss_params->port_transmit_rate);

exit:
	return rc;
}

/** Updates oper_idle_slope of a traffic class, according to the net port priority, for a given port and call the OS specific FQTSS handler used when removing a stream
 * \return			0 on success, -1 otherwise
 * \param port_id		logical port ID
 * \param stream_id		stream ID
 * \param vlan_id		vlan ID
 * \param priority		net port priority
 * \param idle_slope		idle_slope value of the new stream, which should be substracted to the traffic class' oper_idle_slope for the given port
 */
int common_fqtss_stream_remove(unsigned int port_id, void *stream_id, uint16_t vlan_id, uint8_t priority, uint64_t idle_slope, bool is_bridge)
{
	struct bandwidth_availability_params *bandwidth_availability;
	uint64_t new_idle_slope, new_total_idle_slope;
	struct fqtss_port_params *fqtss_params;
	uint8_t traffic_class;
	int rc = 0;

	if (port_id >= CFG_MAX_LOGICAL_PORTS) {
		os_log(LOG_ERR, "invalid logical_port(%u), it exceeds the maximum number of ports(%u)\n", port_id, CFG_MAX_LOGICAL_PORTS);
		rc = -1;
		goto exit;
	}

	traffic_class = net_port_priority_to_traffic_class(port_id, priority);

	if (traffic_class >= FQTSS_MAX_TRAFFIC_CLASS) {
		os_log(LOG_ERR, "invalid traffic class(%u), it exceeds the maximum number of traffic classes(%u) supported by FQTSS\n", traffic_class, FQTSS_MAX_TRAFFIC_CLASS);
		rc = -1;
		goto exit;
	}

	fqtss_params = &fqtss_port_params[port_id];
	bandwidth_availability = &fqtss_params->bandwidth_availability[traffic_class];

	if (bandwidth_availability->oper_idle_slope < idle_slope)
		new_idle_slope = 0;
	else
		new_idle_slope = bandwidth_availability->oper_idle_slope - idle_slope;

	if (is_bridge) {
		if (fqtss_set_oper_idle_slope(port_id, traffic_class, new_idle_slope) < 0) {
			os_log(LOG_ERR, "logical_port(%u) traffic_class(%u) fqtss_set_oper_idle_slope failed to set oper_idle_slope(%" PRIu64 ")\n",
				port_id, traffic_class, new_idle_slope);
			rc = -1;
			goto exit;
		}

	} else {
		if (fqtss_stream_remove(port_id, stream_id, vlan_id, priority, idle_slope) < 0) {
			os_log(LOG_ERR, "logical_port(%u) traffic_class(%u) fqtss_stream_remove failed to remove the stream using oper_idle_slope(%" PRIu64 ")\n",
				port_id, traffic_class, idle_slope);
			rc = -1;
			goto exit;
		}
	}

	if ((bandwidth_availability->oper_idle_slope > new_idle_slope)
	    && ((bandwidth_availability->oper_idle_slope - new_idle_slope) > fqtss_params->total_oper_idle_slope))
		new_total_idle_slope = 0;
	else
		new_total_idle_slope = fqtss_params->total_oper_idle_slope + (new_idle_slope - bandwidth_availability->oper_idle_slope);

	bandwidth_availability->oper_idle_slope = new_idle_slope;
	fqtss_params->total_oper_idle_slope = new_total_idle_slope;

	os_log(LOG_INFO, "logical_port(%u) fqtss remove (%" PRIu64 ", %" PRIu64 ", %" PRIu64 "/%" PRIu64 ")\n", port_id,
	       idle_slope, bandwidth_availability->oper_idle_slope,
	       fqtss_params->total_oper_idle_slope, fqtss_params->port_transmit_rate);

exit:
	return rc;
}

/** Set a port's transmit_rate
 * \return		0 on success, -1 otherwise
 * \param port_id	logical port ID
 * \param rate		its new transmit_rate value
 */
int common_fqtss_set_port_transmit_rate(unsigned int port_id, uint64_t rate)
{
	if (port_id >= CFG_MAX_LOGICAL_PORTS) {
		os_log(LOG_ERR, "invalid logical_port(%u), it exceeds the maximum number of ports(%u)\n", port_id, CFG_MAX_LOGICAL_PORTS);
		return -1;
	}

	fqtss_port_params[port_id].port_transmit_rate = rate;

	return 0;
}

int common_fqtss_init()
{
	/* TODO : Complete FQTSS layer init and make it more configurable
	 * - configure traffic classes supported for the enabled SR classes
	 * - configure highest_traffic_class supported
	 * - fill the rest of the bandwidth_availability table
	 * - add the other 802.1Q-2022 12.20 tables and fill them
	 */

	uint8_t highest_traffic_class = FQTSS_MAX_TRAFFIC_CLASS - 1;
	int i;

	/* Per 802.1Q-2022 34.3 and 34.3.1, use default bandwidth parameters:
	 * lockClassBandwidth = false, deltaBandwidth(N) of the highest numbered traffic class supported = 75% and deltaBandwidth(N) of any lower numbered traffic classes = 0%
	 * So no more than 75% of the port’s available bandwidth is permitted to be reserved for all traffic class' traffic.
	 */
	os_memset(&fqtss_port_params, 0, sizeof(struct fqtss_port_params) * CFG_MAX_LOGICAL_PORTS);

	for (i = 0; i < CFG_MAX_LOGICAL_PORTS; i++)
		fqtss_port_params[i].bandwidth_availability[highest_traffic_class].delta_bandwidth = DEFAULT_HIGHEST_TRAFFIC_CLASS_DELTA_BANDWIDTH;

	return 0;
}

void common_fqtss_exit()
{
	return;
}
