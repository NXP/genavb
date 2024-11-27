/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2018-2020, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief OS specific GenAVB/TSN public control API
 \details OS specific packet API definition for the GenAVB/TSN library

 \copyright Copyright 2016 Freescale Semiconductor, Inc.
 \copyright Copyright 2016, 2018-2020, 2023-2024 NXP
*/

#ifndef _OS_GENAVB_PUBLIC_SOCKET_API_H_
#define _OS_GENAVB_PUBLIC_SOCKET_API_H_


/** Set socket rx callback
 * \ingroup socket_rx
 * \return		GENAVB_SUCCESS or negative error code.
 * \param sock		Socket handler
 * \param callback	Socket rx callback
 * \param data		Socket callback data
 */
int genavb_socket_rx_set_callback(struct genavb_socket_rx *sock, void (*callback)(void *), void *data);

/** Enable socket rx callback
 * \ingroup socket_rx
 * \return		GENAVB_SUCCESS or negative error code.
 * \param sock		Socket handler
 */
int genavb_socket_rx_enable_callback(struct genavb_socket_rx *sock);

/** Set socket tx callback
 * \ingroup socket_tx
 * \return		GENAVB_SUCCESS or negative error code.
 * \param sock		Socket handler
 * \param callback	Socket tx callback
 * \param data		Socket callback data
 */
int genavb_socket_tx_set_callback(struct genavb_socket_tx *sock, void (*callback)(void *), void *data);

/** Enable socket tx callback
 * \ingroup socket_tx
 * \return		GENAVB_SUCCESS or negative error code.
 * \param sock		Socket handler
 */
int genavb_socket_tx_enable_callback(struct genavb_socket_tx *sock);

/** Get logical port's hardware MAC address
 * \ingroup socket
 * \return		GENAVB_SUCCESS or negative error code.
 * \param port_id	logical port ID
 * \param addr		address pointer to store MAC address
 */
int genavb_socket_get_hwaddr(unsigned int port_id, unsigned char *addr);

#endif /* _OS_GENAVB_PUBLIC_SOCKET_API_H_ */
