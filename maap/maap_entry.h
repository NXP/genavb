/*
* Copyright 2021, 2023 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief MAAP stack component entry points
 @details
*/

#ifndef _MAAP_ENTRY_H_
#define _MAAP_ENTRY_H_

void *maap_init(struct maap_config *cfg, unsigned long priv);
int maap_exit(void *maap_ctx);

#endif /* _MAAP_ENTRY_H_ */
