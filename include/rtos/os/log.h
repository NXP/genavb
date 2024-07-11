/*
 * Copyright 2020, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief OS specific GenAVB logging API
 \details OS specific logging API definition for the GenAVB library

 \copyright Copyright 2020, 2023 NXP
*/

#ifndef _OS_GENAVB_PUBLIC_LOG_API_H_
#define _OS_GENAVB_PUBLIC_LOG_API_H_

/** Set the logging level for a component
 * \ingroup generic
 * \return ::GENAVB_SUCCESS or negative error code.
 * \param id	component id
 * \param level	logging level to apply
 */
int genavb_log_level_set(genavb_log_component_id_t id, genavb_log_level_t level);

#endif /* _OS_GENAVB_PUBLIC_LOG_API_H_ */
