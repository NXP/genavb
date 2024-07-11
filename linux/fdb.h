/*
* Copyright 2021, 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief Linux specific FDB service implementation
 @details
*/

#ifndef _LINUX_FDB_H_
#define _LINUX_FDB_H_

#include "os/sys_types.h"

struct fdb_ops_cb {
	int (*bridge_rtnetlink)(u8 *, u16, unsigned int, bool);
};

#endif /* _LINUX_FDB_H_ */
