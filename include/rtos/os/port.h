/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief OS specific GenAVB port API
 \details OS specific port API definition for the GenAVB library

 \copyright Copyright 2023 NXP
*/

#ifndef _OS_GENAVB_PUBLIC_PORT_API_H_
#define _OS_GENAVB_PUBLIC_PORT_API_H_

/** Get the status (up/duplex/rate) of a logical port.
 *
 * \ingroup 		generic
 * \return 		::GENAVB_SUCCESS or negative error code.
 * \param port_id	logical port number
 * \param up		true if port is up, false otherwise
 * \param duplex	true if port is full duplex, false if port is half duplex
 * \param rate		port rate in bits per second
 */
int genavb_port_status_get(unsigned int port_id, bool *up, bool *duplex, unsigned int *rate);

/** Sets the maximum frame size for a given logical port
 *
 * \ingroup		generic
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param port_id	logical port number
 * \param size		maximum frame size to set
 */
int genavb_port_set_max_frame_size(unsigned int port_id, uint16_t size);

#endif /* _OS_GENAVB_PUBLIC_PORT_API_H_ */
