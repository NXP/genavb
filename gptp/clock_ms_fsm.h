/*
 * Copyright 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2020-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief 802.1AS Clock Master State Machine header file
 @details Definition of 802.1AS state machines functions and data structures.
*/

#ifndef _CLOCK_MS_FSM_H_
#define _CLOCK_MS_FSM_H_

#include "common/ptp.h"

#include "gptp.h"

int clock_master_sync_send_sm(struct gptp_instance *instance, ptp_clock_master_sync_send_sm_event_t event);
void clock_master_sync_offset_sm(struct gptp_instance *instance);
void clock_master_sync_receive_sm(struct gptp_instance *instance);

#endif /* _CLOCK_MS_FSM_H_ */
