/*
 * Copyright 2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVDECC stack component entry points
 @details
 */

#ifndef _AVDECC_ENTRY_H_
#define _AVDECC_ENTRY_H_

#include "genavb/init.h"

void *avdecc_init(struct avdecc_config *cfg, unsigned long priv);
int avdecc_exit(void *avdecc_h);

#endif /* _AVDECC_ENTRY_H_ */
