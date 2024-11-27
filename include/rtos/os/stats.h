/*
 * Copyright 2020, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief OS specific GenAVB/TSN stats API
 \details OS specific stats API definition for the GenAVB/TSN library

 \copyright Copyright 2020, 2023-2024 NXP
*/

#ifndef _OS_GENAVB_PUBLIC_STATS_API_H_
#define _OS_GENAVB_PUBLIC_STATS_API_H_

/** Get the number of available stats
 *
 * \ingroup		port
 * \return 		number of available stats or negative error code.
 * \param port_id	logical port number
 */
int genavb_port_stats_get_number(unsigned int port_id);

/** Get the statistics names
 *
 * \ingroup		port
 * \return 		::GENAVB_SUCCESS or negative error code.
 * \param port_id	logical port number
 * \param buf		pointer to buffer used to return pointers to statistics
 * 			names strings (array of const char *)
 * \param buf_len	buffer length. At least sizeof(char *) * (stats number) bytes.
 */
int genavb_port_stats_get_strings(unsigned int port_id, const char **buf, unsigned int buf_len);

/** Get the statistics values
 *
 * \ingroup 		port
 * \return 		::GENAVB_SUCCESS or negative error code.
 * \param port_id	logical port number
 * \param buf		pointer to buffer which will be used to write statistics
 * 			values (array of uint64_t values)
 * \param buf_len	buffer length. At least sizeof(uint64_t) * (stats number) bytes.
 */
int genavb_port_stats_get(unsigned int port_id, uint64_t *buf, unsigned int buf_len);

#endif /* _OS_GENAVB_PUBLIC_STATS_API_H_ */
