/*
 * Copyright 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2020-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief 802.1AS Clock Slave State Machine header file
 @details Definition of 802.1AS state machines functions and data structures.
*/

#ifndef _CLOCK_SL_FSM_H_
#define _CLOCK_SL_FSM_H_

#include "common/ptp.h"

#include "gptp.h"

int clock_slave_sync_sm(struct gptp_port *port);

#endif /* _CLOCK_SL_FSM_H_ */
