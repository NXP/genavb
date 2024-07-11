/*
 * Copyright 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2020-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief 802.1AS State Machine header file
 @details Definition of 802.1AS state machines functions and data structures.
*/

#ifndef _PORT_FSM_H_
#define _PORT_FSM_H_

#include "common/ptp.h"

#include "config.h"
#include "gptp.h"

int port_sync_sync_rcv_sm(struct gptp_port *port);
int port_sync_sync_send_sm(struct gptp_port *port, ptp_port_sync_sync_send_sm_event_t event);
int port_announce_rcv_sm(struct gptp_port *port);
int port_announce_transmit_sm(struct gptp_port *port, ptp_port_announce_transmit_sm_event_t event);
int port_announce_info_sm(struct gptp_port *port, ptp_port_announce_info_sm_event_t event);
void port_announce_interval_setting_sm(struct gptp_port *port);
void port_sync_interval_setting_sm(struct gptp_port *port);
void port_gptp_capable_receive_sm(struct gptp_port *port, ptp_gptp_capable_receive_sm_event_t event);
void port_gptp_capable_transmit_sm(struct gptp_port *port, ptp_gptp_capable_transmit_sm_event_t event);
void port_gptp_capable_interval_setting_sm(struct gptp_port *port);

#endif /* _PORT_FSM_H_ */
