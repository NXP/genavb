/*
* Copyright 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief HSR stack component entry points
 @details
 */

#include "genavb/init.h"

void *hsr_init(struct hsr_config *cfg, unsigned long priv);
int hsr_exit(void *hsr_h);
