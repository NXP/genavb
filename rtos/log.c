/*
 * Copyright 2017-2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief RTOS logging services
 @details Linux logging services implementation
*/

#include "common/log.h"
#include "config.h"

void _os_log_raw(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);

	sdk_vprintf(format, ap);

	va_end(ap);
}


void _os_log(const char *level, const char *func, const char *component, const char *format, ...)
{
	va_list ap;

	sdk_printf("%-4s %11u.%09u %-6s %-32.32s : ", level, (unsigned int)log_time_s, (unsigned int)log_time_ns, component, func);

	va_start(ap, format);

	sdk_vprintf(format, ap);

	va_end(ap);
}
