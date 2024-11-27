/*
 * Copyright 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2020-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief  Best Master Clock definitions
 @details
*/


#ifndef _BMCA_H_
#define _BMCA_H_

#include "gptp.h"
#include "common/ptp.h"


typedef enum {
	BMCA_VECTOR_OTHER = 0,
	BMCA_VECTOR_A_BETTER,
	BMCA_VECTOR_B_BETTER,
	BMCA_VECTOR_A_B_SAME
}bmca_vector_cmp_t;


void copy_system_identity_from_message(struct ptp_system_identity *sid, struct ptp_announce_pdu *announce);
void copy_port_identity_from_message(struct ptp_port_identity *pid, struct ptp_announce_pdu *announce);
void copy_priority_vector_from_message(struct gptp_port *port, struct ptp_priority_vector * p, struct ptp_announce_pdu * announce);
int compare_clock_identity(struct ptp_clock_identity *ida, struct ptp_clock_identity *idb);
bmca_vector_cmp_t compare_system_identity(struct ptp_priority_vector *p1, struct ptp_priority_vector *p2);
bmca_vector_cmp_t compare_priority_vector(struct ptp_priority_vector *p1, struct ptp_priority_vector *p2);

bmca_vector_cmp_t compare_msg_priority_vector(struct ptp_priority_vector *msgP, struct ptp_priority_vector *portP);

void dump_priority_vector(struct ptp_priority_vector *p, u8 domain_index, u8 domain, char *p_name, log_level_t lvl);

#endif
