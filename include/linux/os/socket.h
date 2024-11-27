/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2018, 2020, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief OS specific GenAVB/TSN public control API
 \details OS specific qos API definition for the GenAVB/TSN library

 \copyright Copyright 2016 Freescale Semiconductor, Inc.
 \copyright Copyright 2016, 2018, 2020, 2023-2024 NXP
*/

#ifndef _OS_GENAVB_PUBLIC_SOCKET_API_H_
#define _OS_GENAVB_PUBLIC_SOCKET_API_H_

/** Retrieve the file descriptor associated with a given socket.
 * \ingroup socket
 * \return		GENAVB_SUCCESS or negative error code.
 * \param sock		Socket handler
 */
int genavb_socket_rx_fd(struct genavb_socket_rx *sock);

/** Retrieve the file descriptor associated with a given socket.
 * \ingroup socket
 * \return		GENAVB_SUCCESS or negative error code.
 * \param sock		Socket handler
 */
int genavb_socket_tx_fd(struct genavb_socket_tx *sock);

#endif /* _OS_GENAVB_PUBLIC_SOCKET_API_H_ */
