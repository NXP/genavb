/*
 * Copyright 2018, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief OS specific GenAVB public control API
 \details OS specific control API definition for the GenAVB library

 \copyright Copyright 2018, 2023 NXP
*/

#ifndef _OS_GENAVB_PUBLIC_CONTROL_API_H_
#define _OS_GENAVB_PUBLIC_CONTROL_API_H_

/** Retrieve the receive file descriptor associated with a given control channel.
 * \ingroup control
 * \return 	socket/file descriptor (to be used with poll/select) for the control channel identified by handle, or negative error code.
 * \param 	handle control handle returned by ::genavb_control_open.
 */
int genavb_control_rx_fd(struct genavb_control_handle const *handle);


/** Retrieve the transmit file descriptor associated with a given control channel.
 * \ingroup control
 * \return 	socket/file descriptor (to be used with poll/select) for the control channel identified by handle, or negative error code.
 * \param 	handle	control handle returned by ::genavb_control_open.
 */
int genavb_control_tx_fd(struct genavb_control_handle const *handle);


#endif /* _OS_GENAVB_PUBLIC_CONTROL_API_H_ */
