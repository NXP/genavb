/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2018, 2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief OS specific GenAVB/TSN public control API
 \details OS specific control API definition for the GenAVB/TSN library

 \copyright Copyright 2016 Freescale Semiconductor, Inc.
 \copyright Copyright 2016, 2018, 2021, 2023-2024 NXP
*/

#ifndef _OS_GENAVB_PUBLIC_CONTROL_API_H_
#define _OS_GENAVB_PUBLIC_CONTROL_API_H_

/** Sets notification callback for a given control channel.
 * The callback must not block and should do minimal processing of events, typically just schedule
 * another task to do the actual event handling. The callback is disabled after each call and must
 * be explicitly re-enabled by calling (::genavb_control_enable_callback).
 *
 * GenAVB/TSN stack will call the callback function when:
 * A control receive message is available, the application should call ::genavb_control_receive()
 *
 * \ingroup control
 * \return 	::GENAVB_SUCCESS, or negative error code.
 * \param 	handle control handle returned by ::genavb_control_open.
 * \param 	callback application callback function.
 * \param 	data application private data.
 */
int genavb_control_set_callback(struct genavb_control_handle *handle, int (*callback)(void *), void *data);

/** Enables notification callback for a given control channel.
 * Enabling callback is valid only for one event, so the application needs to re-enable it after each event.
 * In addition if a message is available this function will call directly the control callback.
 * \ingroup control
 * \return 	::GENAVB_SUCCESS, or negative error code.
 * \param 	handle control handle returned by ::genavb_control_open.
 */
int genavb_control_enable_callback(struct genavb_control_handle *handle);

#endif /* _OS_GENAVB_PUBLIC_CONTROL_API_H_ */
