/*
 * Copyright 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2020-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief 802.1AS State Machine header file
 @details Definition of 802.1AS Site entity state machines functions and data structures.
*/

#ifndef _SITE_FSM_H_
#define _SITE_FSM_H_

#include "common/ptp.h"

#include "config.h"
#include "gptp.h"

void port_state_selection_sm(struct gptp_instance *instance);
int site_sync_sync_sm(struct gptp_instance *instance);


#endif /* _SITE_FSM_H_ */
