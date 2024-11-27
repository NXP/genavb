/*
* Copyright 2021, 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief Linux Standard BPF socket filters definitions
 @details
*/
#ifndef _LINUX_SOCKET_FILTERS_H_
#define _LINUX_SOCKET_FILTERS_H_

#include "genavb/net_types.h"

#define BPF_FILTER_MAX_ARRAY_SIZE			16 /* Keep it greater or equal to the max of the filter arrays size */

int sock_filter_get_bpf_code(struct net_address *addr, void *buf, unsigned int *inst_count);

#endif /* _LINUX_SOCKET_FILTERS_H_ */
