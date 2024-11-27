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

#ifndef _MD_FSM_802_H_
#define _MD_FSM_802_H_

#include "common/ptp.h"

#include "config.h"
#include "gptp.h"

#define PDELAY_EXP_FILTER_DECAY (1.0 / (1 << 6))
#define PDELAY_MEAN_FILTER_WINDOW 128

#define PICS_AVNU_PTP_5_MULT_RESPONSES_MAX	3
#define PICS_AVNU_PTP_5_PDELAY_REQ_DELAY_MS	(300 * MS_PER_S)


int md_pdelay_req_sm(struct gptp_port_common *port, ptp_pdelay_req_sm_event_t event);
int md_pdelay_resp_sm(struct gptp_port_common *port, ptp_pdelay_resp_sm_event_t event);
int md_sync_rcv_sm(struct gptp_port *port, ptp_sync_rcv_sm_event_t event);
int md_sync_send_sm(struct gptp_port *port, ptp_sync_snd_sm_event_t event);
void md_link_delay_interval_setting_sm(struct gptp_port_common *port);
int md_link_delay_sync_transmit_sm(struct gptp_port *port, ptp_link_delay_transmit_sm_event_t event);


int md_announce_transmit(struct gptp_port *port);

struct ptp_signaling_pdu *md_set_gptp_capable(struct gptp_port *port);
int md_transmit_gptp_capable(struct gptp_port *port, struct ptp_signaling_pdu *msg);


#endif /* _MD_FSM_802_H_ */
