/*
 * Copyright 2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Management stack component entry points
 @details
 */

#ifndef _MANAGEMENT_ENTRY_H_
#define _MANAGEMENT_ENTRY_H_

#include "genavb/init.h"

void *management_init(struct management_config *cfg, unsigned long priv);
int management_exit(void *management_ctx);

#endif /* _MANAGEMENT_ENTRY_H_ */
