/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief OS specific GenAVB/TSN port API
 \details OS specific port API definition for the GenAVB/TSN library

 \copyright Copyright 2023-2024 NXP
*/

#ifndef _OS_GENAVB_PUBLIC_PORT_API_H_
#define _OS_GENAVB_PUBLIC_PORT_API_H_

/**
 * \defgroup port		Network Port
 * \ingroup library
 */

/** Get the status (up/duplex/rate) of a logical port.
 *
 * \ingroup 		port
 * \return 		::GENAVB_SUCCESS or negative error code.
 * \param port_id	logical port number
 * \param up		true if port is up, false otherwise
 * \param duplex	true if port is full duplex, false if port is half duplex
 * \param rate		port rate in bits per second
 */
int genavb_port_status_get(unsigned int port_id, bool *up, bool *duplex, uint64_t *rate);

/** Sets the maximum frame size for a given logical port
 *
 * \ingroup		port
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param port_id	logical port number
 * \param size		maximum frame size to set
 */
int genavb_port_set_max_frame_size(unsigned int port_id, uint16_t size);

#endif /* _OS_GENAVB_PUBLIC_PORT_API_H_ */
