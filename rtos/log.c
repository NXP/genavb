/*
 * Copyright 2017-2019, 2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief RTOS logging services
 @details Linux logging services implementation
*/

#include "rtos_abstraction_layer.h"

#include "common/log.h"

void _os_log_raw(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);

	rtos_vprintf(format, ap);

	va_end(ap);
}


void _os_log(const char *level, const char *func, const char *component, const char *format, ...)
{
	va_list ap;

	rtos_printf("%-4s %11u.%09u %-6s %-32.32s : ", level, (unsigned int)log_time_s, (unsigned int)log_time_ns, component, func);

	va_start(ap, format);

	rtos_vprintf(format, ap);

	va_end(ap);
}
