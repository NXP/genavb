/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Logging services
 @details Logging services implementation
*/

#ifndef _OS_LOG_H_
#define _OS_LOG_H_

/** Print log messages to the standard output
 *
 * \return none
 * \param level	string of the current logging level ("CRIT", "ERR", "INIT", "INFO", "DEBUG")
 * \param func	string of the calling function
 * \param component	string of the calling component
 * \param format	string of the print format
 */
void _os_log(const char *level, const char *func, const char *component, const char *format, ...);


/** Print raw log messages to the standard output
 *
 * \return none
 * \param format	string of the print format
 */
void _os_log_raw(const char *format, ...);

#include "osal/log.h"

#endif /* _OS_LOG_H_ */
