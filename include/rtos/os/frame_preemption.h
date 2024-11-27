/*
 * Copyright 2020, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief GenAVB/TSN public API
 \details 802.1Q/802.3 Frame Preemption definitions.

 \copyright Copyright 2020, 2023-2024 NXP
*/

#ifndef _OS_GENAVB_PUBLIC_FRAME_PREEMPTION_API_H_
#define _OS_GENAVB_PUBLIC_FRAME_PREEMPTION_API_H_

/** Sets Frame Preemption configuration for a given logical port
 * \ingroup fp
 *
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param port_id	logical port id to configure
 * \param type		type of configuration being set
 * \param config	configuration to set
 */
int genavb_fp_set(unsigned int port_id, genavb_fp_config_type_t type, struct genavb_fp_config *config);

/** Gets Frame Preemption configuration for a given logical port
 * \ingroup fp
 *
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param port_id	logical port id to configure
 * \param type		type of configuration being retrieved
 * \param config	configuration retrieved
*/
int genavb_fp_get(unsigned int port_id, genavb_fp_config_type_t type, struct genavb_fp_config *config);

#endif /* _OS_GENAVB_PUBLIC_FRAME_PREEMPTION_API_H_ */
