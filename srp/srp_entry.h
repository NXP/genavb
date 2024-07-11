/*
 * Copyright 2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief SRP stack component entry points
 @details
 */

#ifndef _SRP_ENTRY_H_
#define _SRP_ENTRY_H_

#include "genavb/init.h"

void *srp_init(struct srp_config *cfg, unsigned long priv);
int srp_exit(void *srp_h);

#endif /* _SRP_ENTRY_H_ */
