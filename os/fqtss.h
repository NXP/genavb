/*
 * Copyright 2020-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief FQTSS OS abstraction
 @details
*/

#ifndef _OS_FQTSS_H_
#define _OS_FQTSS_H_

/** Sets FQTSS operIdleSlope for a given logical port and traffic class
 *
 * \return 0 on success, -1 on error
 * \param port_id	logical port id to configure
 * \param traffic_class	traffic class to configure
 * \param idle_slope	idle slope in bits/s (802.1Q-2018, section 34.3 d))
 */
int fqtss_set_oper_idle_slope(unsigned int port_id, uint8_t traffic_class, unsigned int idle_slope);

/** Increases FQTSS operIdleSlope for a given logical port and traffic class
 *
 * For an endpoint it also enables per stream shapping
 *
 * \return 0 on success, -1 on error
 * \param port_id	logical port id to configure
 * \param stream_id	stream id to configure
 * \param vlan_id	stream vlan id
 * \param priority	stream priority to configure
 * \param idle_slope	idle slope in bits/s (802.1Q-2018, section 34.3 d))
 */
int fqtss_stream_add(unsigned int port_id, void *stream_id, uint16_t vlan_id, uint8_t priority, unsigned int idle_slope);

/** Decreases FQTSS operIdleSlope for a given logical port and traffic class
 *
 * For an endpoint it also disables per stream shapping
 *
 * \return 0 on success, -1 on error
 * \param port_id	logical port id to configure
 * \param stream_id	stream id to configure
 * \param vlan_id	stream vlan id
 * \param priority	stream priority to configure
 * \param idle_slope	idle slope in bits/s (802.1Q-2018, section 34.3 d))
 */
int fqtss_stream_remove(unsigned int port_id, void *stream_id, uint16_t vlan_id, uint8_t priority, unsigned int idle_slope);

#endif /* _OS_FQTSS_H_ */
