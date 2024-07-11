/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Linux specific monotonic time logging implementation
 @details
*/


#ifndef _LINUX_LOG_H_
#define _LINUX_LOG_H_


/** Enable the monotonic time ouput in log messages.
 * \return none
 */
void log_enable_monotonic(void);


/** Update the monotonic time that will be displayed in log messages.
 * May be called at any time to provide the desired log timestamping accuracy.
 * \return none
 */
int log_update_monotonic(void);

#endif /* _LINUX_LOG_H_ */
