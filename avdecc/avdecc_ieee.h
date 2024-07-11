/*
 * Copyright 2021-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVDECC IEEE 1722.1 header file
 @details Definition of AVDECC IEEE 1722.1 specific stack components
*/

#ifndef _AVDECC_IEEE_H_
#define _AVDECC_IEEE_H_

#include "avdecc.h"

void avdecc_ieee_try_fast_connect(struct entity *entity, struct entity_discovery *entity_disc, unsigned int port_id);
void avdecc_ieee_discovery_update(struct avdecc_ctx *avdecc, struct adp_discovery_ctx *disc, struct entity_discovery *entity_disc, bool gptp_gmid_changed);

#endif /* _AVDECC_IEEE_H_ */
