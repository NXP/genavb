/*
 * Copyright 2020, 2022-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
  @file 	 helpers.h
  @brief	 Helper functions
  @details	 Various common helpers for Linux
 */

#ifndef _GENAVB_PUBLIC_HELPERS_H_
#define _GENAVB_PUBLIC_HELPERS_H_

#include <stdlib.h>
#include <string.h>
#include <errno.h>

void h_strncpy(char *dst, const char *src, int dst_len);
int h_strncpy_strict(char *dst, const char *src, int dst_len);
int h_strtoul(unsigned long *output, const char *nptr, char **endptr, int base);
int h_strtoull(unsigned long long *output, const char *nptr, char **endptr, int base);
int h_strtol(long *output, const char *nptr, char **endptr, int base);
int h_snprintf_strict(char *str, unsigned int len, const char *format, ...);
unsigned int h_snprintf(char *str, unsigned int len, const char *format, ...);

#endif /* _GENAVB_PUBLIC_HELPERS_H_ */
