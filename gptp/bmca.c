/*
 * Copyright 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Best Master Clock Algorithm functions
 @details
*/

#include "bmca.h"

void copy_system_identity_from_message(struct ptp_system_identity *sid, struct ptp_announce_pdu *announce)
{
	sid->u.s.priority_1 = announce->grandmaster_priority1;
	os_memcpy(&sid->u.s.clock_quality, &announce->grandmaster_clock_quality, sizeof(struct ptp_clock_quality));
	sid->u.s.priority_2 = announce->grandmaster_priority2;
	os_memcpy(&sid->u.s.clock_identity, &announce->grandmaster_identity, sizeof(struct ptp_clock_identity));
}

void copy_port_identity_from_message(struct ptp_port_identity *pid, struct ptp_announce_pdu *announce)
{
	os_memcpy(pid, &announce->header.source_port_id, sizeof(struct ptp_port_identity));
}

void copy_priority_vector_from_message(struct gptp_port *port, struct ptp_priority_vector *p, struct ptp_announce_pdu *announce)
{
	/* 10.3.5 - messagePriorityVector*/
	copy_system_identity_from_message(&p->u.s.root_system_identity, announce);
	p->u.s.steps_removed = announce->steps_removed;
	copy_port_identity_from_message(&p->u.s.source_port_identity, announce);
	p->u.s.port_number = htons(get_port_identity_number(port));
}

int compare_clock_identity(struct ptp_clock_identity *ida, struct ptp_clock_identity *idb)
{
	return os_memcmp(ida, idb, sizeof(struct ptp_clock_identity));
}


bmca_vector_cmp_t compare_system_identity(struct ptp_priority_vector *Pa, struct ptp_priority_vector *Pb)
{
	int cmp = os_memcmp(&Pa->u.s.root_system_identity, &Pb->u.s.root_system_identity, sizeof(struct ptp_system_identity));

	if (cmp > 0)
		return BMCA_VECTOR_B_BETTER;
	else if (cmp < 0)
		return BMCA_VECTOR_A_BETTER;
	else
		return BMCA_VECTOR_A_B_SAME;
}

bmca_vector_cmp_t compare_priority_vector(struct ptp_priority_vector *Pa, struct ptp_priority_vector *Pb)
{
	int cmp = os_memcmp(Pa, Pb, sizeof(struct ptp_priority_vector));

	if (cmp > 0)
		return BMCA_VECTOR_B_BETTER;
	else if (cmp < 0)
		return BMCA_VECTOR_A_BETTER;
	else
		return BMCA_VECTOR_A_B_SAME;
}

/** 802.1AS -10.3.5
 * This messagePriorityVector is superior to the portPriorityVector and will replace it if, and only if, the
 * messagePriorityVector is better than the portPriorityVector, or the Announce message has been transmitted
 * from the same master time-aware system and MasterPort as the portPriorityVector, i.e., if the following is
 * true.
 *
 */
bmca_vector_cmp_t compare_msg_priority_vector(struct ptp_priority_vector *msgP, struct ptp_priority_vector *portP)
{
	int cmp;

	/* Message is from the same master time-aware system and MasterPort as the portPriorityVector */
	cmp = os_memcmp(&msgP->u.s.source_port_identity, &portP->u.s.source_port_identity, sizeof(struct ptp_port_identity));
	if (!cmp)
		return BMCA_VECTOR_A_BETTER;

	return compare_priority_vector(msgP, portP);
}

void dump_priority_vector(struct ptp_priority_vector *p, u8 domain_index, u8 domain, char *p_name, log_level_t lvl)
{
	u64 cid = get_64(p->u.s.source_port_identity.clock_identity);

	os_log(lvl, "domain(%u, %d) %s: root identity %016"PRIx64"\n", domain_index, (s8)domain, p_name,
		get_ntohll(p->u.s.root_system_identity.u.s.clock_identity.identity));
	os_log(lvl, "%s: priority1 %u\tpriority2 %u\n",
		p_name, p->u.s.root_system_identity.u.s.priority_1,  p->u.s.root_system_identity.u.s.priority_2);
	os_log(lvl, "%s: class %u\taccuracy %u\n",
		p_name, p->u.s.root_system_identity.u.s.clock_quality.clock_class, p->u.s.root_system_identity.u.s.clock_quality.clock_accuracy);
	os_log(lvl, "%s: variance %u\n",
			p_name, ntohs(p->u.s.root_system_identity.u.s.clock_quality.offset_scaled_log_variance));
	os_log(lvl, "%s: source port identity %016"PRIx64", port number %u\n",
		p_name, ntohll(cid), ntohs(p->u.s.source_port_identity.port_number));
	os_log(lvl, "%s: port number %u steps removed %u\n",
			p_name, ntohs(p->u.s.port_number), ntohs(p->u.s.steps_removed));
}
