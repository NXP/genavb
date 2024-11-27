/*
 * Copyright 2020, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief GenAVB/TSN public API
 \details 802.1Q QoS definitions.

 \copyright Copyright 2020, 2023-2024 NXP
*/

#ifndef _OS_GENAVB_PUBLIC_QOS_API_H_
#define _OS_GENAVB_PUBLIC_QOS_API_H_

/** Maps priority to traffic class, based on port QoS settings
 * \ingroup qos
 *
 * \return		traffic class
 * \param port_id	logical port id
 * \param priority	priority
*/
unsigned int genavb_priority_to_traffic_class(unsigned int port_id, uint8_t priority);

/** Set traffic class configuration on the port
 * \ingroup qos
 * \return		::GENAVB_SUCCESS or negative error code
 * \param port_id	logical port number
 * \param config	configuration of traffic class
 */
int genavb_traffic_class_set(unsigned int port_id, struct genavb_traffic_class_config *config);

#endif /* _OS_GENAVB_PUBLIC_QOS_API_H_ */
