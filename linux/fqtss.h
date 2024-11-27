/*
* Copyright 2021, 2023-2024 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief Linux specific FQTSS service implementation
 @details
*/

#ifndef _LINUX_FQTSS_H_
#define _LINUX_FQTSS_H_

#include "os/sys_types.h"

struct fqtss_ops_cb {
	void (*fqtss_exit)(void);
	int (*fqtss_set_oper_idle_slope)(unsigned int, uint8_t, uint64_t);
	int (*fqtss_stream_add)(unsigned int, void *, uint16_t, uint8_t, uint64_t);
	int (*fqtss_stream_remove)(unsigned int, void *, uint16_t, uint8_t, uint64_t);
};

#endif /* _LINUX_FQTSS_H_ */
