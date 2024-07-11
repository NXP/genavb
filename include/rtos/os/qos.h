/*
 * Copyright 2020, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief GenAVB public API
 \details 802.1Q QoS definitions.

 \copyright Copyright 2020, 2023 NXP
*/

#ifndef _OS_GENAVB_PUBLIC_QOS_API_H_
#define _OS_GENAVB_PUBLIC_QOS_API_H_

/** Sets Scheduled Traffic admin configuration for a given logical port
 * \ingroup qos
 *
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param port_id	logical port id to configure
 * \param clk_id	clock id
 * \param config	EST configuration
 */
int genavb_st_set_admin_config(unsigned int port_id, genavb_clock_id_t clk_id,
			       struct genavb_st_config *config);

/** Gets Scheduled Traffic configuration for a given logical port
 * \ingroup qos
 *
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param port_id	logical port id to configure
 * \param type		config type to read (administrative or operational)
 * \param config	EST configuration that will be written
 * \param list_length   the maximum length of the list provided in the config
 */
int genavb_st_get_config(unsigned int port_id, genavb_st_config_type_t type,
			 struct genavb_st_config *config, unsigned int list_length);

/** Sets Scheduled Traffic maximum service data unit configuration for a given logical port and set of queues
 * \ingroup qos
 *
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param port_id	logical port id to configure
 * \param queue_max_sdu	EST array of max SDU values that will be written. One entry per queue to be configured.
 * \param n   size of queue_max_sdu array
 */
int genavb_st_set_max_sdu(unsigned int port_id, struct genavb_st_max_sdu *queue_max_sdu, unsigned int n);

/** Gets Scheduled Traffic maximum service data unit configuration for a give logical port
 * \ingroup qos
 *
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param port_id	logical port id to read configuration from
 * \param queue_max_sdu	EST array of max SDU values that that will be retrieved. The array must have at least 8 entries.
 */
int genavb_st_get_max_sdu(unsigned int port_id, struct genavb_st_max_sdu *queue_max_sdu);

/** Sets Frame Preemption configuration for a given logical port
 * \ingroup qos
 *
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param port_id	logical port id to configure
 * \param type		type of configuration being set
 * \param config	configuration to set
 */
int genavb_fp_set(unsigned int port_id, genavb_fp_config_type_t type, struct genavb_fp_config *config);

/** Gets Frame Preemption configuration for a given logical port
 * \ingroup qos
 *
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param port_id	logical port id to configure
 * \param type		type of configuration being retrieved
 * \param config	configuration retrieved
*/
int genavb_fp_get(unsigned int port_id, genavb_fp_config_type_t type, struct genavb_fp_config *config);

/** Maps priority to traffic class, based on port QoS settings
 * \ingroup qos
 *
 * \return		traffic class
 * \param port_id	logical port id
 * \param priority	priority
*/
unsigned int genavb_priority_to_traffic_class(unsigned int port_id, uint8_t priority);

#endif /* _OS_GENAVB_PUBLIC_QOS_API_H_ */
