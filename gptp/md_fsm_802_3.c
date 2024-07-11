/*
 * Copyright 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Implementation of 802.1AS 802.3 Media dependent State Machines
 @details Implementation of pdelay_req, pdelay_resp and sync_rcv state machines
*/

#include "md_fsm_802_3.h"
#include "port_fsm.h"
#include "target_clock_adj.h"
#include "common/ptp_time_ops.h"

static const char *pdelay_req_state_str[] = {
	[PDELAY_REQ_SM_STATE_NOT_ENABLED]= "NOT_ENABLED",
	[PDELAY_REQ_SM_STATE_INITIAL_SEND_PDELAY_REQ]= "INITIAL_SEND_PDELAY_REQ",
	[PDELAY_REQ_SM_STATE_RESET]= "RESET",
	[PDELAY_REQ_SM_STATE_SEND_PDELAY_REQ]= "SEND_PDELAY_REQ",
	[PDELAY_REQ_SM_STATE_WAITING_FOR_PDELAY_RESP]= "WAITING_FOR_PDELAY_RESP",
	[PDELAY_REQ_SM_STATE_WAITING_FOR_PDELAY_RESP_FOLLOW_UP]= "WAITING_FOR_PDELAY_RESP_FOLLOW_UP",
	[PDELAY_REQ_SM_STATE_WAITING_FOR_PDELAY_INTERVAL_TIMER]= "WAITING_FOR_PDELAY_INTERVAL_TIMER"
};

static const char *get_pdelay_req_state_str(ptp_pdelay_req_sm_state_t state)
{
	if (state > PDELAY_REQ_SM_STATE_MAX_VALUE || state < PDELAY_REQ_SM_STATE_MIN_VALUE)
		return "Unknown PTP pdelay req state";
	else
		return pdelay_req_state_str[state];
}

static const char *pdelay_req_event_str[] = {
	[PDELAY_REQ_SM_EVENT_REQ_INTERVAL]= "REQ_INTERVAL",
	[PDELAY_REQ_SM_EVENT_RUN]= "RUN"
};

static const char *get_pdelay_req_event_str(ptp_pdelay_req_sm_event_t event)
{
	if (event > PDELAY_REQ_SM_EVENT_MAX_VALUE || event < PDELAY_REQ_SM_EVENT_MIN_VALUE)
		return "Unknown PTP pdelay req event";
	else
		return pdelay_req_event_str[event];
}

static const char *pdelay_resp_state_str[] = {
	[PDELAY_RESP_SM_STATE_NOT_ENABLED]= "NOT_ENABLED",
	[PDELAY_RESP_SM_STATE_WAITING_FOR_PDELAY_REQ]= "WAITING_FOR_PDELAY_REQ",
	[PDELAY_RESP_SM_STATE_INITIAL_WAITING_FOR_PDELAY_REQ]= "INITIAL_WAITING_FOR_PDELAY_REQ",
	[PDELAY_RESP_SM_STATE_SENT_PDELAY_RESP_WAITING_FOR_TIMESTAMP]= "SENT_PDELAY_RESP_WAITING_FOR_TIMESTAMP"
};

static const char *get_pdelay_resp_state_str(ptp_pdelay_resp_sm_state_t state)
{
	if (state > PDELAY_RESP_SM_STATE_MAX_VALUE || state < PDELAY_RESP_SM_STATE_MIN_VALUE)
		return "Unknown PTP pdelay resp state";
	else
		return pdelay_resp_state_str[state];
}

static const char *pdelay_resp_event_str[] = {
	[PDELAY_RESP_SM_EVENT_REQ_RECEIVED]= "REQ_RECEIVED",
	[PDELAY_RESP_SM_EVENT_RUN]= "RUN"
};

static const char *get_pdelay_resp_event_str(ptp_pdelay_resp_sm_event_t event)
{
	if (event > PDELAY_RESP_SM_EVENT_MAX_VALUE || event < PDELAY_RESP_SM_EVENT_MIN_VALUE)
		return "Unknown pdelay resp event";
	else
		return pdelay_resp_event_str[event];
}

static const char *sync_rcv_state_str[] = {
	[SYNC_RCV_SM_STATE_DISCARD]= "DISCARD",
	[SYNC_RCV_SM_STATE_WAITING_FOR_FOLLOW_UP]= "WAITING_FOR_FOLLOW_UP",
	[SYNC_RCV_SM_STATE_WAITING_FOR_SYNC]= "WAITING_FOR_SYNC"
};

static const char *get_sync_rcv_state_str(ptp_sync_rcv_sm_state_t state)
{
	if (state > SYNC_RCV_SM_STATE_MAX_VALUE || state < SYNC_RCV_SM_STATE_MIN_VALUE)
		return "Unknown PTP sync receive state";
	else
		return sync_rcv_state_str[state];
}

static const char *sync_rcv_event_str[] = {
	[SYNC_RCV_SM_EVENT_FUP_TIMEOUT]= "FUP_TIMEOUT",
	[SYNC_RCV_SM_EVENT_RUN]= "RUN"
};

static const char *get_sync_rcv_event_str(ptp_sync_rcv_sm_event_t event)
{
	if (event > SYNC_RCV_SM_EVENT_MAX_VALUE || event < SYNC_RCV_SM_EVENT_MIN_VALUE)
		return "Unknown PTP sync rcv event";
	else
		return sync_rcv_event_str[event];
}

static const char *sync_snd_state_str[] = {
	[SYNC_SND_SM_STATE_INITIALIZING]= "INITIALIZING",
	[SYNC_SND_SM_STATE_SEND_SYNC]= "SEND SYNC",
	[SYNC_SND_SM_STATE_SEND_FOLLOW_UP]= "SEND FOLLOW UP",
};

static const char *get_sync_snd_state_str(ptp_sync_snd_sm_state_t state)
{
	if (state > SYNC_SND_SM_STATE_MAX_VALUE || state < SYNC_SND_SM_STATE_MIN_VALUE)
		return "Unknown PTP sync snd state";
	else
		return sync_snd_state_str[state];
}

static const char *sync_snd_event_str[] = {
	[SYNC_SND_SM_EVENT_RUN]= "RUN",
};

static const char *get_sync_snd_event_str(ptp_sync_snd_sm_event_t event)
{
	if (event > SYNC_SND_SM_EVENT_MAX_VALUE || event < SYNC_SND_SM_EVENT_MIN_VALUE)
		return "Unknown PTP sync snd event";
	else
		return sync_snd_event_str[event];
}


static const char *link_interval_setting_state_str[] = {
	[LINK_INTERVAL_SETTING_SM_STATE_NOT_ENABLED]= "NOT_ENABLED",
	[LINK_INTERVAL_SETTING_SM_STATE_INITIALIZE]= "INITIALIZE",
	[LINK_INTERVAL_SETTING_SM_STATE_SET_INTERVAL]= "SET_INTERVAL",
};

static const char *get_link_interval_setting_state_str(ptp_link_interval_setting_sm_state_t state)
{
	if (state > LINK_INTERVAL_SETTING_SM_STATE_MAX_VALUE || state < LINK_INTERVAL_SETTING_SM_STATE_MIN_VALUE)
		return "Unknown PTP link delay interval state";
	else
		return link_interval_setting_state_str[state];
}


static void get_static_rate_ratio(struct gptp_port_common *port)
{
	struct ptp_port_params *params = &port->params;

	params->neighbor_rate_ratio = 1.00;
}


static void get_static_pdelay(struct gptp_port_common *port)
{
	struct ptp_port_params *params = &port->params;

	ptp_double_to_u_scaled_ns(&params->mean_link_delay, port->cfg.initial_neighborPropDelay);
	os_log(LOG_DEBUG, "Port(%u) static PDelay %4.2f ns\n", port->port_id, port->cfg.initial_neighborPropDelay);
}



/* see 802.1-AS specs at section 10.2.5.1
*/
static u16 sequence_id_random(void)
{
	return (u16)os_random();
}


static void md_set_common_header(void *msg, u8 domain_number)
{
	struct ptp_hdr *header = (struct ptp_hdr *)(msg);

	os_memset(header, 0, sizeof(struct ptp_hdr));

	/* fill in parameters common to all messages types */
	header->transport_specific = PTP_DOMAIN_MAJOR_SDOID;
	header->version_ptp = PTP_VERSION;
	header->domain_number = domain_number;
}

/* 11.2.14.2.3 - creates a structure whose parameters contain the fields (see 11.4 and its
subclauses) of a Follow_Up message to be transmitted, and returns a pointer to this structure.*/
static struct ptp_follow_up_pdu * md_set_follow_up(struct gptp_port *port)
{
	struct ptp_follow_up_pdu *msg = &port->follow_up_tx;
	struct ptp_instance_md_entity *md = &port->md;
	u32 scaled_last_gm_freq_change;
	struct ptp_u_scaled_ns fup_correction_field_u_scaled_ns;
	struct ptp_u_scaled_ns residence_and_delay_u_scaled_ns;
	struct ptp_u_scaled_ns tmp_u_scaled_ns, sync_tx_ts_u_scaled_ns;
	struct ptp_scaled_ns tmp_scaled_ns;
	struct ptp_timestamp tmp_ptp_ts;
	struct ptp_extended_timestamp tmp_ptp_extended_ts;

	md_set_common_header(msg, port->instance->domain.domain_number);

	msg->header.msg_type = PTP_MSG_TYPE_FOLLOW_UP;
	msg->header.msg_length = htons(sizeof(struct ptp_follow_up_pdu));
	if (port->instance->gptp->force_2011) {
		msg->header.flags |= htons(PTP_FLAG_PTP_TIMESCALE);
		msg->header.control = PTP_CONTROL_FOLLOW_UP_2011;
	} else {
		msg->header.minor_version_ptp = PTP_MINOR_VERSION;
		msg->header.control = PTP_CONTROL_FOLLOW_UP;
	}

	/* 11.2.14.2.3 c) */
	/* Note: follow-up must use previous sync sequence number */
	msg->header.sequence_id = htons((port->sync_tx_seqnum - 1));

	/* 11.2.14.2.3 d) */
	msg->header.log_msg_interval = md->sync_snd.logMessageInterval;

	/* 11.2.14.2.3 a) The followUpCorrectionField is set equal to the sum of:
	1) the followUpCorrectionField member of the most recently received MDSyncSend structure
	(see 10.2.2.1 and 11.2.11)
	2) the quantity
	rateRatio x (<syncEventEgressTimestamp> - upstreamTxTime),
	where rateRatio is the rateRatio member of the most recently received MDSyncSend structure
	(see 10.2.2.1 and 11.2.11), upstreamTxTime is the upstreamTxTime member of the most
	recently received MDSyncSend structure (see 10.2.2.1 and 11.2.11), and
	<syncEventEgressTimestamp> is the timestamp pointed to by rcvdMDTimestampReceivePtr,
	corrected for egressLatency (see 8.4.3).
	*/
	u64_to_u_scaled_ns(&sync_tx_ts_u_scaled_ns, port->sync_tx_ts);
	u_scaled_ns_sub(&residence_and_delay_u_scaled_ns, &sync_tx_ts_u_scaled_ns, &md->sync_snd.upstreamTxTime);
	u_scaled_ns_to_scaled_ns(&tmp_scaled_ns, &residence_and_delay_u_scaled_ns);
	double_scaled_ns_mul(&tmp_scaled_ns, md->sync_snd.rateRatio, tmp_scaled_ns);
	scaled_ns_to_u_scaled_ns(&residence_and_delay_u_scaled_ns, &tmp_scaled_ns);

	scaled_ns_to_u_scaled_ns(&tmp_u_scaled_ns, &md->sync_snd.followUpCorrectionField);
	u_scaled_ns_add(&fup_correction_field_u_scaled_ns, &tmp_u_scaled_ns, &residence_and_delay_u_scaled_ns);

	os_log(LOG_DEBUG, "Port(%u): upstreamTxTime=%"PRIu64":%u sync_tx_ts=%"PRIu64" correction=%"PRIu64":%u\n", port->port_id,
		md->sync_snd.upstreamTxTime.u.s.nanoseconds,
		md->sync_snd.upstreamTxTime.u.s.fractional_nanoseconds,
		port->sync_tx_ts,
		fup_correction_field_u_scaled_ns.u.s.nanoseconds,
		fup_correction_field_u_scaled_ns.u.s.fractional_nanoseconds);

	os_log(LOG_DEBUG, "Port(%u): isGM %d lastRcvdPortNum %u\n", port->port_id, port->instance->is_grandmaster, md->sync_snd.lastRcvdPortNum);

	/* txPSSync - 10.2.6.2.2: PortSyncSync's lastRcvdPortNum is 0 if sync/follow-up are generated by the local master clock (ie. this node is the grand master) */
	if (!md->sync_snd.lastRcvdPortNum) {
		/*
		AVnu test gPTP.com.c.14.2 MDSyncSendSM: correctionField of Follow_Up Messages

		When a grandmaster sets the Correction field in a Follow_Up message, it may include any fractional
		nanoseconds of the masterTime added to the gmRateRatio x (currentTime - localTime), it will
		also have added to it the gmRateRatio x (the TimeStamp of the Sync messages egress minus the localTime).

		While it is conceivable that an implementation may track fractional nanoseconds, common implementations will
		not. This test validates that such implementations populate the Follow_Up messages correctionField with zero
		*/

		/* force the correction field to zero only if this node is the grand master*/
		msg->header.correction_field = htonll(0);

		/* add computed correction (residence and delay) to the message's pot field */
		ptp_timestamp_to_u_scaled_ns(&tmp_u_scaled_ns, &md->sync_snd.preciseOriginTimestamp);
		u_scaled_ns_add(&tmp_u_scaled_ns, &tmp_u_scaled_ns, &fup_correction_field_u_scaled_ns);
		u_scaled_ns_to_ptp_extended_timestamp(&tmp_ptp_extended_ts, &tmp_u_scaled_ns);
		ptp_extended_timestamp_to_ptp_timestamp(&tmp_ptp_ts, &tmp_ptp_extended_ts);
		hton_ptp_timestamp(&msg->precise_origin_timestamp, &tmp_ptp_ts);
	} else {
		s64 correction_field;

		/* Acting as a bridge and forwarding sync/follow-up messages from a remote grand master */
		u_scaled_ns_to_scaled_ns(&tmp_scaled_ns, &fup_correction_field_u_scaled_ns);
		scaled_ns_to_pdu_correction_field(&correction_field, &tmp_scaled_ns);
		copy_64(&msg->header.correction_field, &correction_field);

		hton_ptp_timestamp(&msg->precise_origin_timestamp, &md->sync_snd.preciseOriginTimestamp);
	}

	os_log(LOG_DEBUG, "Port(%u): message pot=%u:%u:%u correction=%"PRIx64":%"PRIx64" (%"PRIx64":%u))\n\n", port->port_id,
		htons(msg->precise_origin_timestamp.seconds_msb),
		htonl(msg->precise_origin_timestamp.seconds_lsb),
		htonl(msg->precise_origin_timestamp.nanoseconds),
		ntohll(msg->header.correction_field) >> 16,
		ntohll(msg->header.correction_field) & 0xFFFF,
		tmp_scaled_ns.u.s.nanoseconds,
		tmp_scaled_ns.u.s.fractional_nanoseconds);

	/* 11.2.14.2.3 b) */
	msg->header.source_port_id.port_number = htons(md->sync_snd.sourcePortIdentity.port_number);
	copy_64(msg->header.source_port_id.clock_identity, md->sync_snd.sourcePortIdentity.clock_identity);

	/* 11.4.4.3 */
	os_memset(&msg->tlv, 0, sizeof(struct ptp_follow_up_tlv));
	msg->tlv.tlv_type = htons(PTP_TLV_TYPE_ORGANIZATION_EXTENSION);
	msg->tlv.length_field = htons(28);
	msg->tlv.organization_id[0] = 0x00;
	msg->tlv.organization_id[1] = 0x80;
	msg->tlv.organization_id[2] = 0xC2;
	msg->tlv.organization_sub_type[0] = 0x00;
	msg->tlv.organization_sub_type[1] = 0x00;
	msg->tlv.organization_sub_type[2] = 0x01;

	/* 11.2.14.2.3 f) */
	/* 11.4.4.3.6 - The value of cumulativeScaledRateOffset is equal to (rateRatio - 1.0) x (2e41 ), truncated to the next smaller
    signed integer, where rateRatio is the ratio of the frequency of the grandMaster to the frequency of the
    LocalClock entity in the time-aware system that sends the message */
	msg->tlv.cumulative_scaled_rate_offset = htonl((s32)((md->sync_snd.rateRatio - 1.0) * (1ULL << 41)));

	/* 11.2.14.2.3 g) */
	/* 11.4.4.3.7 - The value of gmTimeBaseIndicator is the timeBaseIndicator of the ClockSource entity for the current
    grandmaster */
	msg->tlv.gm_time_base_indicator = htons(md->sync_snd.gmTimeBaseIndicator);

	/* 11.2.14.2.3 h) */
	/* 11.4.4.3.8 - The value of lastGmPhaseChange is the time of the current grandmaster minus the time of the previous
    grandmaster, at the time that the current grandmaster became grandmaster. The value is copied from the
    lastGmPhaseChange member of the MDSyncSend structure whose receipt causes the MD entity to send the
    Follow_Up message */
	hton_scaled_ns(&msg->tlv.last_gm_phase_change, &md->sync_snd.lastGmPhaseChange);

	/* 11.2.14.2.3 i) */
	/* 11.4.4.3.9 - The value of scaledLastGmFreqChange is the fractional frequency offset of the current grandmaster relative
	to the previous grandmaster, at the time that the current grandmaster became grandmaster, or relative to
	itself prior to the last change in gmTimeBaseIndicator, multiplied by 2e41 and truncated to the next smaller
	signed integer. The value is obtained by multiplying the lastGmFreqChange member of MDSyncSend
	whose receipt causes the MD entity to send the Follow_Up message (see 11.2.11) by 2e41 , and truncating to
	the next smaller signed integer */
	scaled_last_gm_freq_change = (u32)(s32)(md->sync_snd.lastGmFreqChange * (1ULL << 41));
	msg->tlv.scaled_last_gm_freq_change = htonl(scaled_last_gm_freq_change);

	return msg;
}


/* 11.2.14.2.1 - creates a structure whose parameters contain the fields (see 11.4 and its
subclauses) of a Sync message to be transmitted, and returns a pointer to this structure.*/
static struct ptp_sync_pdu * md_set_sync(struct gptp_port *port)
{
	struct ptp_instance_md_entity *md = &port->md;
	struct ptp_sync_pdu *msg = &port->sync_tx;

	os_log(LOG_DEBUG, "Port(%u) domain(%u, %u)\n", port->port_id, port->instance->index, port->instance->domain.domain_number);

	md_set_common_header(msg, port->instance->domain.domain_number);

	msg->header.source_port_id.port_number = htons(md->sync_snd.sourcePortIdentity.port_number);
	copy_64(msg->header.source_port_id.clock_identity, md->sync_snd.sourcePortIdentity.clock_identity);

	msg->header.msg_type = PTP_MSG_TYPE_SYNC;
	msg->header.msg_length = htons(sizeof(struct ptp_sync_pdu));
	msg->header.flags = htons((PTP_FLAG_TWO_STEP << 8));
	msg->header.sequence_id = htons(port->sync_tx_seqnum);
	msg->header.control = PTP_CONTROL_SYNC;
	msg->header.log_msg_interval = md->sync_snd.logMessageInterval;

	if (port->instance->gptp->force_2011)
		msg->header.flags |= htons(PTP_FLAG_PTP_TIMESCALE);
	else
		msg->header.minor_version_ptp = PTP_MINOR_VERSION;

	os_memset(&msg->timestamp, 0, sizeof(struct ptp_timestamp));

	return msg;
}


static struct ptp_announce_pdu * md_set_announce(struct gptp_port *port)
{
	struct ptp_announce_pdu *msg = &port->announce_tx;
	struct gptp_instance *instance = port->instance;
	u16 flags = 0;
	unsigned int num_ptlv_entries;

	os_log(LOG_DEBUG, "Port(%u) domain(%u, %u)\n", port->port_id, port->instance->index, port->instance->domain.domain_number);

	md_set_common_header(msg, port->instance->domain.domain_number);

	msg->header.msg_type = PTP_MSG_TYPE_ANNOUNCE;

	if ((instance->params.master_steps_removed + 1) > MAX_PTLV_ENTRIES) {
		os_log(LOG_ERR, "Port(%u): announce: StepsRemoved(%u) exceeds maximum supported path sequence entries\n", port->port_id, instance->params.master_steps_removed);
		num_ptlv_entries = MAX_PTLV_ENTRIES;
	} else {
		num_ptlv_entries = instance->params.master_steps_removed + 1;
	}

	msg->header.msg_length = htons(sizeof(struct ptp_announce_pdu) - ((MAX_PTLV_ENTRIES - num_ptlv_entries) * sizeof(struct ptp_clock_identity)));

	if (port->instance->domain.domain_number == PTP_DOMAIN_0)
		flags = PTP_FLAG_PTP_TIMESCALE;
	if (instance->params.leap61)
		flags |= PTP_FLAG_LEAP_61;
	if (instance->params.leap59)
		flags |= PTP_FLAG_LEAP_59;
	if (instance->params.current_utc_offset_valid)
		flags |= PTP_FLAG_CURRENT_UTC_OFF_VALID;
	if (instance->params.time_traceable)
		flags |= PTP_FLAG_TIME_TRACEABLE;
	if (instance->params.frequency_traceable)
		flags |= PTP_FLAG_FREQUENCY_TRACEABLE;
	msg->header.flags = htons(flags);

	msg->header.source_port_id.port_number = htons(get_port_identity_number(port));
	copy_64(msg->header.source_port_id.clock_identity, get_port_identity_clock_id(port));
	msg->header.sequence_id = htons(port->announce_tx_seqnum);
	port->announce_tx_seqnum++;
	if (port->instance->gptp->force_2011) {
		msg->header.control = PTP_CONTROL_ANNOUNCE_2011;
	} else {
		msg->header.minor_version_ptp = PTP_MINOR_VERSION;
		msg->header.control = PTP_CONTROL_ANNOUNCE;
	}
	msg->header.log_msg_interval = port->params.current_log_announce_interval;

	msg->current_utc_offset = htons(instance->params.current_utc_offset);
	msg->grandmaster_priority1 = instance->params.gm_priority.u.s.root_system_identity.u.s.priority_1;
	os_memcpy(&msg->grandmaster_clock_quality, &instance->params.gm_priority.u.s.root_system_identity.u.s.clock_quality, sizeof(struct ptp_clock_quality));
	msg->grandmaster_priority2 = instance->params.gm_priority.u.s.root_system_identity.u.s.priority_2;
	os_memcpy(&msg->grandmaster_identity, &instance->params.gm_priority.u.s.root_system_identity.u.s.clock_identity, sizeof(struct ptp_clock_identity));
	msg->steps_removed = htons(instance->params.master_steps_removed);
	msg->time_source = instance->params.time_source;
	msg->ptlv.header.tlv_type = htons(PTP_TLV_TYPE_PATH_TRACE);
	msg->ptlv.header.length_field = htons(sizeof(struct ptp_clock_identity) * num_ptlv_entries);
	os_memcpy(&msg->ptlv.path_sequence[0], &instance->params.path_trace[0], ntohs(msg->ptlv.header.length_field));

	return msg;
}


int md_announce_transmit(struct gptp_port *port)
{
	struct ptp_announce_pdu *msg = md_set_announce(port);

	port->stats.num_tx_announce++;

	return gptp_net_tx(net_port_from_gptp(port), msg, ntohs(msg->header.msg_length), 0, port->instance->domain.domain_number, PTP_DOMAIN_MAJOR_SDOID);
}

/* 11.2.15.2.1 - creates a structure containing the parameters of a PDelay_Req message
to be transmitted, and returns a pointer to this structure */
static struct ptp_pdelay_req_pdu * md_set_pdelay_req(struct gptp_port_common *port)
{
	struct ptp_md_entity_pdelay_req_sm *sm = &port->md.pdelay_req_sm;
	struct ptp_pdelay_req_pdu *msg = &sm->req_tx;

	os_log(LOG_DEBUG, "Port(%u)\n", port->port_id);

	md_set_common_header(msg, PTP_DOMAIN_0);

	if (sm->s < 0)
		msg->header.transport_specific = PTP_CMLDS_MAJOR_SDOID;
	else if (port->gptp->force_2011)
		msg->header.flags = htons(PTP_FLAG_PTP_TIMESCALE);

	msg->header.msg_type = PTP_MSG_TYPE_PDELAY_REQ;
	msg->header.msg_length = htons(sizeof(struct ptp_pdelay_req_pdu));
	msg->header.source_port_id.port_number = htons(port->identity.port_number);
	copy_64(msg->header.source_port_id.clock_identity, port->identity.clock_identity);
	msg->header.sequence_id = htons(sm->pdelayReqSequenceId);
	if (port->gptp->force_2011) {
		msg->header.control = PTP_CONTROL_PDELAY_REQ_2011;
	} else {
		msg->header.minor_version_ptp = PTP_MINOR_VERSION;
		msg->header.control = PTP_CONTROL_PDELAY_REQ;
	}
	msg->header.log_msg_interval = 0;
	os_memset(&msg->reserved[0], 0, 20);

	return msg;
}

/* 11.2.16.2.1 - creates a structure containing the parameters of a PDelay_Resp message
to be transmitted, and returns a pointer to this structure */
static struct ptp_pdelay_resp_pdu * md_set_pdelay_resp(struct gptp_port_common *port)
{
	struct ptp_md_entity_pdelay_resp_sm *sm = &port->md.pdelay_resp_sm;
	struct ptp_pdelay_resp_pdu *msg = &sm->resp_tx;

	os_log(LOG_DEBUG, "Port(%u)\n", port->port_id);

	md_set_common_header(msg, PTP_DOMAIN_0);

	if (sm->s < 0)
		msg->header.transport_specific = PTP_CMLDS_MAJOR_SDOID;
	else if (port->gptp->force_2011)
		msg->header.flags = htons(PTP_FLAG_PTP_TIMESCALE);

	msg->header.msg_type = PTP_MSG_TYPE_PDELAY_RESP;
	msg->header.msg_length = htons(sizeof(struct ptp_pdelay_resp_pdu));
	msg->header.flags |= htons((PTP_FLAG_TWO_STEP << 8));
	msg->header.source_port_id.port_number = htons(port->identity.port_number);
	copy_64(msg->header.source_port_id.clock_identity, port->identity.clock_identity);
	msg->header.sequence_id = sm->req_rx.header.sequence_id;
	if (port->gptp->force_2011) {
		msg->header.control = PTP_CONTROL_PDELAY_RESP_2011;
	} else {
		msg->header.minor_version_ptp = PTP_MINOR_VERSION;
		msg->header.control = PTP_CONTROL_PDELAY_RESP;
	}

	msg->header.log_msg_interval = PTP_LOG_MSG_PDELAY_RESP;

	/* fill in pdelay response message */
	u64_to_pdu_ptp_timestamp(&msg->request_receipt_timestamp, sm->req_rx_ts);
	os_memcpy(&msg->requesting_port_identity, &sm->req_rx.header.source_port_id, sizeof(struct ptp_port_identity));

	return msg;
}


static struct ptp_pdelay_resp_follow_up_pdu *md_set_pdelay_resp_fup(struct gptp_port_common *port)
{
	struct ptp_md_entity_pdelay_resp_sm *sm = &port->md.pdelay_resp_sm;
	struct ptp_pdelay_resp_follow_up_pdu *msg = &sm->resp_fup_tx;

	os_log(LOG_DEBUG, "Port(%u): ts %"PRIu64"\n", port->port_id, sm->resp_tx_ts);

	md_set_common_header(msg, PTP_DOMAIN_0);

	if (sm->s < 0)
		msg->header.transport_specific = PTP_CMLDS_MAJOR_SDOID;
	else if (port->gptp->force_2011)
		msg->header.flags = htons(PTP_FLAG_PTP_TIMESCALE);

	msg->header.msg_type = PTP_MSG_TYPE_PDELAY_RESP_FUP;
	msg->header.msg_length = htons(sizeof(struct ptp_pdelay_resp_follow_up_pdu));
	msg->header.source_port_id.port_number = htons(port->identity.port_number);
	copy_64(msg->header.source_port_id.clock_identity, port->identity.clock_identity);
	msg->header.sequence_id = sm->req_rx.header.sequence_id;
	if (port->gptp->force_2011) {
		msg->header.control = PTP_CONTROL_PDELAY_RESP_FUP_2011;
	} else {
		msg->header.minor_version_ptp = PTP_MINOR_VERSION;
		msg->header.control = PTP_CONTROL_PDELAY_RESP_FUP;
	}
	msg->header.log_msg_interval = PTP_LOG_MSG_PDELAY_RESP_FUP;

	/* fill in pdelay response follow up message */
	u64_to_pdu_ptp_timestamp(&msg->response_origin_timestamp, sm->resp_tx_ts);
	os_memcpy(&msg->requesting_port_identity, &sm->req_rx.header.source_port_id, sizeof(struct ptp_port_identity));

	/* FIXME: optionnaly add correction to response origin timestamp to compensate for phy/mac latency */

	return msg;
}


struct ptp_signaling_pdu *md_set_gptp_capable(struct gptp_port *port)
{
	struct ptp_signaling_pdu *msg = &port->gptp_capable_transmit_sm.msg;

	os_log(LOG_DEBUG, "Port(%u) domain(%u, %u)\n", port->port_id, port->instance->index, port->instance->domain.domain_number);

	md_set_common_header(msg, port->instance->domain.domain_number);

	/* IEEE 802.1AS-2020 section 10.6.4.4 */
	msg->header.msg_type = PTP_MSG_TYPE_SIGNALING;
	msg->header.msg_length = htons(sizeof(struct ptp_signaling_pdu));
	msg->header.flags = htons(0);
	msg->header.source_port_id.port_number = htons(get_port_identity_number(port));
	copy_64(msg->header.source_port_id.clock_identity, get_port_identity_clock_id(port));
	msg->header.sequence_id = htons(port->signaling_tx_seqnum);
	port->signaling_tx_seqnum++;
	msg->header.control = 0;
	msg->header.log_msg_interval = PTP_LOG_MSG_SIGNALING;
	if (!port->instance->gptp->force_2011)
		msg->header.minor_version_ptp = PTP_MINOR_VERSION;
	os_memset(&msg->target_port_identity, 0xFF, sizeof(struct ptp_port_identity));

	/* IEEE 802.1AS-2020 section 10.6.4.5 */
	os_memset(&msg->u.ctlv, 0, sizeof(struct ptp_gptp_capable_tlv));
	msg->tlv_type = htons(PTP_TLV_TYPE_ORGANIZATION_EXTENSION_DO_NOT_PROPAGATE);
	msg->length_field = htons(12);
	msg->organization_id[0] = 0x00;
	msg->organization_id[1] = 0x80;
	msg->organization_id[2] = 0xC2;
	msg->organization_sub_type[0] = 0x00;
	msg->organization_sub_type[1] = 0x00;
	msg->organization_sub_type[2] = PTP_TLV_SUBTYPE_GPTP_CAPABLE_MESSAGE;
	msg->u.ctlv.log_gptp_capable_message_interval = port->params.current_log_gptp_capable_message_interval;

	return msg;
}

int md_transmit_gptp_capable(struct gptp_port *port, struct ptp_signaling_pdu *msg)
{
	port->stats.num_tx_sig++;

	return gptp_net_tx(net_port_from_gptp(port), msg, sizeof(struct ptp_signaling_pdu), 0, port->instance->domain.domain_number, PTP_DOMAIN_MAJOR_SDOID);
}


/** Checks if the received pdelay response matches a pdelay request
 *
 */
static bool md_pdelay_req_is_resp_valid(struct ptp_md_entity_pdelay_req_sm *sm)
{
	if (os_memcmp(sm->rcvdPdelayRespPtr->requesting_port_identity.clock_identity, sm->txPdelayReqPtr->header.source_port_id.clock_identity, sizeof(struct ptp_clock_identity)) ||
		(sm->rcvdPdelayRespPtr->requesting_port_identity.port_number != sm->txPdelayReqPtr->header.source_port_id.port_number) ||
		(sm->rcvdPdelayRespPtr->header.sequence_id != sm->txPdelayReqPtr->header.sequence_id))
		return false;
	else
		return true;
}

/*
 * FIXME: Differs from the spec 802.1AS 11.2.20
 * 802.1AS-2020 is also accepting pdelay follow up if previous pdelay response matches our requesting port,
 * but this is always true a this point (checked in PDELAY_REQ_SM_STATE_WAITING_FOR_PDELAY_RESP).
 * Most probably the standard wanted to check if the pdelay follow up also matches our requesting port.
 */
static bool md_pdelay_req_is_resp_fup_valid(struct ptp_md_entity_pdelay_req_sm *sm)
{
	if ((sm->rcvdPdelayRespFollowUpPtr->header.sequence_id != sm->txPdelayReqPtr->header.sequence_id) ||
	os_memcmp(sm->rcvdPdelayRespFollowUpPtr->header.source_port_id.clock_identity, sm->rcvdPdelayRespPtr->header.source_port_id.clock_identity, sizeof(struct ptp_clock_identity)) ||
	(sm->rcvdPdelayRespFollowUpPtr->header.source_port_id.port_number != sm->rcvdPdelayRespPtr->header.source_port_id.port_number))
		return false;
	else
		return true;
}


/*
* MDPdelayReqSM state machine and functions
*/

/** Computes the rate ratio between this node and the remote end of the link (IEEE 802.1AS-2020 section 11.2.19.3.3)
 */

#define MAX_RATE_RATIO_DEVIATION	0.01

static int md_pdelay_req_compute_pdelay_rate_ratio(struct gptp_port_common *port, ptp_double *rate_ratio)
{
	struct ptp_md_entity_pdelay_req_sm *sm = &port->md.pdelay_req_sm;
	struct gptp_ctx *gptp = port->gptp;
	u64 corrected_responder_event_timestamp, pdelay_response_event_ingress_timestamp;
	int phase_discont = 0;

	os_log(LOG_DEBUG, "Port(%u)\n", port->port_id);

	/*
	 * Get current timings
	 * t3 = corrected_responder_event_timestamp
	 * t4 = pdelay_response_event_ingress_timestamp
	 */
	/* FIXME take correctionField into account (sub nanosecond precision) */
	corrected_responder_event_timestamp = pdu_ptp_timestamp_to_u64(sm->rcvdPdelayRespFollowUpPtr->response_origin_timestamp);
	pdelay_response_event_ingress_timestamp = sm->resp_rx_ts;

	if (sm->prev_corrected_responder_event_timestamp != PTP_TS_UNSET_U64_VALUE) {

		/* Check if a phase discontinuity happened on the HW clock between this pDelay set
		 * and the last one.
		 * If it has, exit and report it.
		 */
		if (sm->prev_rate_ratio_local_clk_phase_discont != gptp->local_clock.phase_discont) {
			os_log(LOG_INFO, "local clock phase discontinuity, skip rate ratio computation\n");
			phase_discont = 1;
			goto exit;
		}

		/* Check that values used for rate_ratio are valid */
		if ((pdelay_response_event_ingress_timestamp > sm->prev_pdelay_response_event_ingress_timestamp)
		&& (corrected_responder_event_timestamp > sm->prev_corrected_responder_event_timestamp)) {

			/*
			 * r = (t3 - t3') / (t4 - t4')
			 */
			*rate_ratio = ((ptp_double)corrected_responder_event_timestamp - sm->prev_corrected_responder_event_timestamp)
					/ (pdelay_response_event_ingress_timestamp - sm->prev_pdelay_response_event_ingress_timestamp);

			if (os_fabs(1 - *rate_ratio) > MAX_RATE_RATIO_DEVIATION) {
				os_log(LOG_ERR, "Port(%u): NeighborRateRatio %1.16f ignored\n", port->port_id, *rate_ratio);
				*rate_ratio = 1;
			}
			sm->neighborRateRatioValid = true;
		}
		else
			sm->neighborRateRatioValid = false;
	}

	os_log(LOG_DEBUG, "Port(%u): prev_pdelay_response_event_ingress_timestamp(%"PRIu64") prev_corrected_responder_event_timestamp(%"PRIu64")\n",
			port->port_id, sm->prev_pdelay_response_event_ingress_timestamp, sm->prev_corrected_responder_event_timestamp);
	os_log(LOG_DEBUG, "Port(%u): pdelay_response_event_ingress_timestamp(%"PRIu64") corrected_responder_event_timestamp(%"PRIu64")\n",
			port->port_id, pdelay_response_event_ingress_timestamp, corrected_responder_event_timestamp);
	os_log(LOG_DEBUG, "Port(%u): NeighborRateRatio %1.16f (%f ppb)\n", port->port_id, *rate_ratio, (*rate_ratio - 1)*1000000000);

exit:
	/*
	 * Save timings.
	 * t3' = sm->prev_pdelay_response_event_ingress_timestamp
	 * t4' = sm->prev_corrected_responder_event_timestamp
	 */
	sm->prev_pdelay_response_event_ingress_timestamp = pdelay_response_event_ingress_timestamp;
	sm->prev_corrected_responder_event_timestamp = corrected_responder_event_timestamp;
	sm->prev_rate_ratio_local_clk_phase_discont = gptp->local_clock.phase_discont;

	return phase_discont;
}


/** Computes the mean propagation delay on the link attached to MD entity (11.2.15.2.4)
 *
 */
static void md_pdelay_req_compute_prop_time(struct gptp_port_common *port, ptp_double r, struct ptp_u_scaled_ns *d)
{
	struct ptp_md_entity_pdelay_req_sm *sm = &port->md.pdelay_req_sm;
	struct gptp_ctx *gptp = port->gptp;
	struct ptp_timestamp ptp_ts;
	ptp_double delay, raw_delay;
	ptp_double current_delay_ns, previous_delay_ns;

	os_log(LOG_DEBUG, "Port(%u)\n", port->port_id);

	/* Check if we adjusted (in offset) the HW clock between this pDelay set.
	 * If we did, silently exit as it's caused by our adjustment.
	 */
	if (sm->prev_pdelay_local_clk_phase_discont != gptp->local_clock.phase_discont) {
		os_log(LOG_INFO, "local clock phase discontinuity, skip prop time computation\n");
		return;
	}

	if (sm->neighborRateRatioValid) {
		u64 t1 = sm->req_tx_ts;
		u64 t2 = pdu_ptp_timestamp_to_u64(sm->rcvdPdelayRespPtr->request_receipt_timestamp);		//FIXME take correctionField into account
		u64 t3 = pdu_ptp_timestamp_to_u64(sm->rcvdPdelayRespFollowUpPtr->response_origin_timestamp);	//FIXME take correctionField into account
		u64 t4 = sm->resp_rx_ts;

		ptp_ts = sm->rcvdPdelayRespPtr->request_receipt_timestamp;
		os_log(LOG_DEBUG, "Port(%u): PDelay request_receipt_timestamp: secs msb(%u) secs lsb(%u) nsecs(%u)\n", port->port_id, ntohs(ptp_ts.seconds_msb), ntohl(ptp_ts.seconds_lsb), ntohl(ptp_ts.nanoseconds));
		ptp_ts = sm->rcvdPdelayRespFollowUpPtr->response_origin_timestamp;
		os_log(LOG_DEBUG, "Port(%u): PDelay response_origin_timestamp: secs msb(%u) secs lsb(%u) nsecs(%u)\n", port->port_id, ntohs(ptp_ts.seconds_msb), ntohl(ptp_ts.seconds_lsb), ntohl(ptp_ts.nanoseconds));
		os_log(LOG_DEBUG, "Port(%u): r %1.16f t1(%"PRIu64") t2(%"PRIu64") t3(%"PRIu64") t4(%"PRIu64") t4-t1(%u) t3-t2(%u)\n", port->port_id, r, t1, t2, t3, t4, (u32)(t4-t1), (u32)(t3-t2));

		raw_delay = (r*(t4 - t1) - (t3 - t2)) / 2;

		stats_update(&port->pdelay_stats, (s32)raw_delay);

		/* AVnu test gPTP.com 15.6.g
		 * (t4 - t1) < (t3 - t2)
		 * Invalid values, d (u_scaled_ns) is implicitely supposed to overflow in the conversion.
		 * Overflow it by a large enough value to make delay mean above pDelay treshold.
		 */
		if (raw_delay < 0) {
			/*
			* Fix against switching to AS_Capable=FALSE due to measurements error (local clock granularity).
			* If the measured delay is negative but still within a given range it is considered as valid.
			*
			* By experiment, on iMX6Q Slave endpoint, it appears that the pdelay average with a short ethernet
			* cable (< 0.5m) is ~35ns. The corresponding standard devication is ~16ns and the variance ~256ns.
			* The check below (MAX_MEASURE_ERROR_NS) rejects any sample that is below the theorical pdelay minimal
			* value (i.e. 0) minus 3 times the standard deviation.
			*/
			#define MAX_MEASURE_ERROR_NS (16*3)
			if (raw_delay < (-MAX_MEASURE_ERROR_NS)) {
				os_log(LOG_DEBUG, "invalid pdelay %4.2f\n", raw_delay);
				raw_delay += 0xFFFFFFFF;
			}
		}

		delay = filter(&sm->pdelay_filter, raw_delay);
		ptp_double_to_u_scaled_ns(d, delay);
		os_log(LOG_DEBUG, "Port(%u): PDelay %4.2f ns (%4.2f ns)\n", port->port_id, delay, raw_delay);

		/* notify upper layer if a new pdelay has been computed. Can be used
		 later as static pdelay value if needed. Only used in automotive profile */
		if (port->cfg.neighborPropDelay_mode == CFG_GPTP_PDELAY_MODE_STATIC) {
			previous_delay_ns = port->pdelay_info.pdelay;
			current_delay_ns = delay;
			os_log(LOG_DEBUG, "Port(%u): previous_delay_ns %4.2f / current_delay_ns %4.2f (diff %4.2f)\n", port->port_id, previous_delay_ns, current_delay_ns, os_fabs(current_delay_ns - previous_delay_ns));
			if (gptp->pdelay_indication && (os_fabs(current_delay_ns - previous_delay_ns) > port->cfg.neighborPropDelay_sensitivity)) {
				port->pdelay_info.port_id = port->port_id;
				port->pdelay_info.pdelay = delay;
				gptp->pdelay_indication(&port->pdelay_info);
			}
		}
	}
}


static void md_pdelay_req_sm_reset(struct gptp_port_common *port)
{
	struct ptp_md_entity_pdelay_req_sm *sm = &port->md.pdelay_req_sm;
	struct ptp_md_entity_globals *globals = &port->md.globals;

	port->stats.num_md_pdelay_req_sm_reset++;

	sm->rcvdPdelayResp = false; /* 802.1AS 2011 Cor1-2013 */

	/*
	 * Actually if the spec is followed litterally we end-up accepting
	 * lostResponses + 1 before declaring not AS_CAPABLE.
	 */
	if (sm->lostResponses < globals->allowedLostResponses)
		sm->lostResponses += 1;
	else {
		port->stats.num_rx_pdelayresp_lost_exceeded++;
		globals->isMeasuringDelay = false;
		gptp_as_capable_across_domains_down(port);

		/* Non standard */
		/* Reset pdelay measurements */
		sm->prev_corrected_responder_event_timestamp = PTP_TS_UNSET_U64_VALUE;
		sm->neighborRateRatioValid = false;
	}

	/* PICS AVnu_PTP-5
	 *
	 * Cease pDelay_Req transmissions if more than
	 * one pDelay_Resp messages have been received
	 * from multiple clock identities for each of three
	 * successive pDelay_Req messages. After 5 minutes, or a link state toggle or portEnabled toggled
	 * or pttPortEnabled, the pDelay_Req transmission resumes.
	 */
	if (sm->multipleResponses >= PICS_AVNU_PTP_5_MULT_RESPONSES_MAX) {
		timer_restart(&sm->req_timer, PICS_AVNU_PTP_5_PDELAY_REQ_DELAY_MS);
		sm->multipleResponses = 0;
	}

	sm->state = PDELAY_REQ_SM_STATE_RESET;
}


static void md_pdelay_req_sm_silent_pdelay(struct gptp_port_common *port)
{
	struct ptp_port_params *params = &port->params;
	struct gptp_ctx *gptp = port->gptp;

	get_static_rate_ratio(port);
	get_static_pdelay(port);
	if (gptp->pdelay_indication) {
		port->pdelay_info.port_id = port->port_id;
		u_scaled_ns_to_ptp_double(&port->pdelay_info.pdelay, &params->mean_link_delay);
		gptp->pdelay_indication(&port->pdelay_info);
	}
}


static void md_pdelay_req_sm_initial_send(struct gptp_port_common *port)
{
	struct gptp_ctx *gptp = port->gptp;
	struct ptp_md_entity_pdelay_req_sm *sm = &port->md.pdelay_req_sm;
	struct ptp_md_entity_globals *globals = &port->md.globals;
	struct ptp_port_params *params = &port->params;
	u64 pdelay_req_interval_ns;

	sm->rcvdPdelayResp = false;
	sm->rcvdPdelayRespFollowUp = false;
	params->neighbor_rate_ratio = 1.0;
	sm->rcvdMDTimestampReceive = false;
	sm->lostResponses = 0;
	sm->detectedFaults = 0;
	globals->isMeasuringDelay = false;
	gptp_as_capable_across_domains_down(port);

	/* Non standard */
	sm->prev_corrected_responder_event_timestamp = PTP_TS_UNSET_U64_VALUE;
	sm->neighborRateRatioValid = false;
	filter_exp_decay_init(&sm->pdelay_filter, PDELAY_EXP_FILTER_DECAY);

	sm->pdelayReqSequenceId = sequence_id_random();
	sm->txPdelayReqPtr = md_set_pdelay_req(port);

	gptp_net_tx(net_port_from_gptp_common(port), sm->txPdelayReqPtr, sizeof(struct ptp_pdelay_req_pdu), 1, PTP_DOMAIN_0, (sm->s < 0)? PTP_CMLDS_MAJOR_SDOID : PTP_DOMAIN_MAJOR_SDOID);

	port->stats.num_tx_pdelayreq++;

	os_clock_gettime64(gptp->clock_monotonic, &sm->req_time);

	/* re-schedule next pdelay request transmit */
	u_scaled_ns_to_u64(&pdelay_req_interval_ns, &globals->pdelayReqInterval);
	timer_restart(&sm->req_timer, pdelay_req_interval_ns/NSECS_PER_MS);

	os_log(LOG_INFO, "Port(%u): s %d pdelay_req_interval (ms) %"PRIu64"\n", port->port_id, sm->s, pdelay_req_interval_ns/NSECS_PER_MS);

	sm->state = PDELAY_REQ_SM_STATE_INITIAL_SEND_PDELAY_REQ;
}


static void md_pdelay_req_sm_send(struct gptp_port_common *port)
{
	struct ptp_md_entity_pdelay_req_sm *sm = &port->md.pdelay_req_sm;
	struct ptp_md_entity_globals *globals = &port->md.globals;
	struct gptp_ctx *gptp = port->gptp;
	u64 pdelay_req_interval_ns;

	sm->pdelayReqSequenceId += 1;
	sm->txPdelayReqPtr = md_set_pdelay_req(port);

	sm->prev_pdelay_local_clk_phase_discont = gptp->local_clock.phase_discont;

	gptp_net_tx(net_port_from_gptp_common(port), sm->txPdelayReqPtr, sizeof(struct ptp_pdelay_req_pdu), 1, PTP_DOMAIN_0,  (sm->s < 0)? PTP_CMLDS_MAJOR_SDOID : PTP_DOMAIN_MAJOR_SDOID);

	port->stats.num_tx_pdelayreq++;

	os_clock_gettime64(gptp->clock_monotonic, &sm->req_time);

	/* re-schedule next pdelay request transmit */
	u_scaled_ns_to_u64(&pdelay_req_interval_ns, &globals->pdelayReqInterval);
	timer_start(&sm->req_timer, pdelay_req_interval_ns/NSECS_PER_MS);

	sm->state = PDELAY_REQ_SM_STATE_SEND_PDELAY_REQ;
}

static void md_pdelay_req_sm_waiting_fup(struct ptp_md_entity_pdelay_req_sm *sm)
{
	sm->rcvdPdelayResp = false;
	sm->state = PDELAY_REQ_SM_STATE_WAITING_FOR_PDELAY_RESP_FOLLOW_UP;
}


static void md_pdelay_req_sm_waiting_interval(struct gptp_port_common*port)
{
	struct ptp_md_entity_pdelay_req_sm *sm = &port->md.pdelay_req_sm;
	struct ptp_md_entity_globals *globals = &port->md.globals;
	struct ptp_port_params *params = &port->params;
	u64 cid_req, cid_src;
	ptp_double neigh_delay, neigh_delay_threshold;
	int phase_discont = 0;

	sm->rcvdPdelayRespFollowUp = false;
	sm->lostResponses = 0;
	sm->multipleResponses = 0;

	if (params->compute_neighbor_rate_ratio) {
		/*
		 * Skip propagation delay computation without having neighborRateRatioValid being false.
		 * Only happens when the HW clock has been adjusted making the current ratio
		 * not usable for propagation delay computation. As this was done on purpose and because
		 * of our own adjustment this shouldn't make the link !asCapable.
		 */
		phase_discont = md_pdelay_req_compute_pdelay_rate_ratio(port, &params->neighbor_rate_ratio);
	} else {
		/* Non standard */
		/* Don't use previous timestamps when compute_neighbor_rate_ratio is set back to true,
		  but avoid resetting asCapable.
		  This can be triggered by signaling messages from the peer when doing HW clock adjustments */
		sm->prev_corrected_responder_event_timestamp = PTP_TS_UNSET_U64_VALUE;
	}

	if (params->compute_mean_link_delay && (!phase_discont))
		md_pdelay_req_compute_prop_time(port, params->neighbor_rate_ratio, &params->mean_link_delay);

	globals->isMeasuringDelay = true;
	os_memcpy(&cid_src, &sm->rcvdPdelayRespPtr->header.source_port_id.clock_identity, sizeof(struct ptp_clock_identity));
	os_memcpy(&cid_req, &port->identity.clock_identity, sizeof(struct ptp_clock_identity));

	u_scaled_ns_to_ptp_double(&neigh_delay, &params->mean_link_delay);
	u_scaled_ns_to_ptp_double(&neigh_delay_threshold, &globals->meanLinkDelayThresh);

	if ((neigh_delay <= neigh_delay_threshold) &&
	(cid_src != cid_req) && (sm->neighborRateRatioValid)) {
		gptp_as_capable_across_domains_up(port);
		sm->detectedFaults = 0;

	} else if (cid_src != cid_req) {
		filter_reset(&sm->pdelay_filter);
		gptp_as_capable_across_domains_down(port);
		globals->isMeasuringDelay = false;
		sm->detectedFaults = 0;

	} else {
		if (sm->detectedFaults <= globals->allowedFaults) {
			sm->detectedFaults++;

		} else {
			gptp_as_capable_across_domains_down(port);
			globals->isMeasuringDelay = false;
			sm->detectedFaults = 0;
		}
	}

	sm->state = PDELAY_REQ_SM_STATE_WAITING_FOR_PDELAY_INTERVAL_TIMER;
}


static void md_pdelay_req_sm_waiting_interval_static_pdelay(struct gptp_port_common *port)
{
	struct ptp_md_entity_pdelay_req_sm *sm = &port->md.pdelay_req_sm;
	struct ptp_md_entity_globals *globals = &port->md.globals;

	sm->rcvdPdelayRespFollowUp = false;
	sm->lostResponses++;
	globals->isMeasuringDelay = false;

	/* use static rate ratio */
	get_static_rate_ratio(port);

	/* for some reason pdelay could not be computed, just use the configured or pre-computed one */
	get_static_pdelay(port);

	sm->state = PDELAY_REQ_SM_STATE_WAITING_FOR_PDELAY_INTERVAL_TIMER;
}

static void md_pdelay_req_sm_waiting_resp(struct ptp_md_entity_pdelay_req_sm *sm)
{
	sm->rcvdMDTimestampReceive = false;
	sm->state = PDELAY_REQ_SM_STATE_WAITING_FOR_PDELAY_RESP;
}


/** MDPdelayReq state machine 11.2.15 - Figure 11.8
 * The MDPdelayReq state machine shall is responsible for the following:
 * - Sending Pdelay_Req messages and restarting the pdelayIntervalTimer,
 * - Detecting that the peer mechanism is running,
 * - Detecting if Pdelay_Resp and/or Pdelay_Resp_Follow_Up messages corresponding to a Pdelay_Req message sent are not received,
 * - Detecting whether more than one Pdelay_Resp is received within one Pdelay_Req message transmission interval (see 11.5.2.2),
 * - Computing propagation time on the attached link when Pdelay_Resp and Pdelay_Resp_Follow_Up messages are received, and
 * - Computing the ratio of the frequency of the LocalClock entity of the time-aware system at the other end of the attached link to the frequency of the LocalClock entity of the current time-aware system
 * \return	0 on success or negative value on failure
 * \param port	pointer to the gptp port common context used to retrieve port params and state machine variables
 * \param event	value of the sync receive event triggering this state machine
*/
int md_pdelay_req_sm(struct gptp_port_common *port, ptp_pdelay_req_sm_event_t event)
{
	struct gptp_ctx *gptp = port->gptp;
	struct ptp_port_params *params = &port->params;
	struct ptp_md_entity_pdelay_req_sm *sm = &port->md.pdelay_req_sm;
	ptp_pdelay_req_sm_state_t state;
	bool port_oper = gptp->net_ports[port->port_id].port_oper;
	int rc = 0;
	u64 now = 0;

	if ((params->begin) ||
	(!port_oper) ||
	(!*sm->portEnabled0)) {
		sm->state = PDELAY_REQ_SM_STATE_NOT_ENABLED;
		sm->req_to_ts_min = 0xFFFFFFFFFFFFFFFF;
		sm->req_to_ts_max = 0;
		goto exit;
	}

start:
	state = sm->state;

	switch (state) {
	case PDELAY_REQ_SM_STATE_NOT_ENABLED:
		if (port_oper && *sm->portEnabled0) {
			if (port->pdelay_transmit_enabled)
				md_pdelay_req_sm_initial_send(port);
			else
				/* AUTOMOTIVE PROFILE - not in the specs, custom addition for pdelay 'silent' mode
				where the pdelay request transmit state machine is not scheduled at all */
				md_pdelay_req_sm_silent_pdelay(port);
		}
		break;

	case PDELAY_REQ_SM_STATE_RESET:
		/*
		 * 802.1AS-rev draft 1.0
		 * Pdelay_Req storm issue
		 */
		if (event == PDELAY_REQ_SM_EVENT_REQ_INTERVAL) {
			/*
			 * The revised MDPdelayReq SM doesn't
			 * clearly specify to clear these flags...
			 * It's however mentioned as a seperate note and anyway
			 * required to forget any previously received messages.
			 */
			sm->rcvdPdelayResp = false;
			sm->rcvdPdelayRespFollowUp = false;
			md_pdelay_req_sm_send(port);
		}
		break;

	case PDELAY_REQ_SM_STATE_INITIAL_SEND_PDELAY_REQ:
	case PDELAY_REQ_SM_STATE_SEND_PDELAY_REQ:
		if (event == PDELAY_REQ_SM_EVENT_REQ_INTERVAL) {
			/* FIXME. Should not happen and is not in the specs.  Temporary WA to handle the case where a timestamp
			is never received upon ethernet cable disconnection or not received before the next transmit interval. */
			os_log(LOG_DEBUG, "Port(%u): resetting upon timestamp not received (state %s event %s)\n",
			       port->port_id, get_pdelay_req_state_str(state), get_pdelay_req_event_str(event));
			md_pdelay_req_sm_reset(port);
		} else if (sm->rcvdMDTimestampReceive) {
			os_clock_gettime64(gptp->clock_monotonic, &sm->ts_time);
			os_log(LOG_DEBUG, "Port(%u): event %s received (req_to_ts %" PRIu64 " us)\n",
			       port->port_id, get_pdelay_req_event_str(event), (sm->ts_time - sm->req_time) / 1000);
			md_pdelay_req_sm_waiting_resp(sm);
		}

		break;

	case PDELAY_REQ_SM_STATE_WAITING_FOR_PDELAY_RESP:
		if (event == PDELAY_REQ_SM_EVENT_REQ_INTERVAL) {
			if ((port->gptp->cfg.profile == CFG_GPTP_PROFILE_AUTOMOTIVE) && (port->cfg.neighborPropDelay_mode == CFG_GPTP_PDELAY_MODE_STATIC)) {
				/* AUTOMOTIVE PROFILE - Persistant pdelay values per AutoCDSFunctionalSpecs 1.1 - 6.2.2.1
				looks like the peer is not sending pdelay response, since we are in automotive mode
				let's use the predefined pdelay value and ignore the timeout event */
				os_log(LOG_DEBUG, "Port(%u): response timeout, switched to static pdelay\n", port->port_id);
				md_pdelay_req_sm_waiting_interval_static_pdelay(port);
			} else {
				os_clock_gettime64(gptp->clock_monotonic, &now);
				os_log(LOG_DEBUG, "Port(%u): resetting upon response timeout (req_to_event %" PRIu64 " us state %s event %s)\n",
				       port->port_id, (now - sm->req_time) / 1000, get_pdelay_req_state_str(state), get_pdelay_req_event_str(event));
			}

			md_pdelay_req_sm_reset(port);
		} else if (sm->rcvdPdelayResp) {
			os_clock_gettime64(gptp->clock_monotonic, &sm->resp_time);
			if (md_pdelay_req_is_resp_valid(sm)) {
				md_pdelay_req_sm_waiting_fup(sm);
			} else {
				os_log(LOG_INFO, "Port(%u): resetting upon invalid response (state %s event %s - %u %u)\n", port->port_id,
					get_pdelay_req_state_str(state), get_pdelay_req_event_str(event),
					ntohs(sm->rcvdPdelayRespPtr->header.sequence_id), ntohs(sm->txPdelayReqPtr->header.sequence_id));
				port->stats.num_rx_ptp_packet_discard++;
				md_pdelay_req_sm_reset(port);
			}
		}

		break;

	case PDELAY_REQ_SM_STATE_WAITING_FOR_PDELAY_RESP_FOLLOW_UP:
		if (event == PDELAY_REQ_SM_EVENT_REQ_INTERVAL) {
			os_clock_gettime64(gptp_port_common_to_clock(port), &now);
			os_log(LOG_DEBUG, "Port(%u): resetting upon response follow-up timeout (req_to_event %" PRIu64 " us state %s event%s)\n",
			       port->port_id, (now - sm->req_time) / 1000, get_pdelay_req_state_str(state), get_pdelay_req_event_str(event));
			md_pdelay_req_sm_reset(port);
		} else if ((sm->rcvdPdelayResp) &&
			   (sm->rcvdPdelayRespPtr->header.sequence_id == sm->txPdelayReqPtr->header.sequence_id)) {

			os_log(LOG_INFO, "Port(%u): received duplicate pDelayResp (state %s event %s) - %u %u\n", port->port_id,
			       get_pdelay_req_state_str(state), get_pdelay_req_event_str(event),
			       ntohs(sm->rcvdPdelayRespPtr->header.sequence_id), ntohs(sm->txPdelayReqPtr->header.sequence_id));
			/* Subclause 11.2.2
			 * If multiple PdelayResp messages are received, set asCapable to false.
			 * It doesn't appear in PdelayReq SM but is mandatory nonetheless.
			 */
			gptp_as_capable_across_domains_down(port);

			sm->multipleResponses++;
			port->stats.num_rx_ptp_packet_discard++;

			md_pdelay_req_sm_reset(port);
		} else if (sm->rcvdPdelayRespFollowUp) {
			os_clock_gettime64(gptp->clock_monotonic, &sm->fup_time);
			if (md_pdelay_req_is_resp_fup_valid(sm))
				md_pdelay_req_sm_waiting_interval(port);
			else
				os_log(LOG_INFO, "Port(%u): invalid response follow-up (state %s event %s - %u %u)\n", port->port_id,
					get_pdelay_req_state_str(state), get_pdelay_req_event_str(event),
					ntohs(sm->rcvdPdelayRespFollowUpPtr->header.sequence_id), ntohs(sm->txPdelayReqPtr->header.sequence_id));
		}
		break;

	case PDELAY_REQ_SM_STATE_WAITING_FOR_PDELAY_INTERVAL_TIMER:
		if (event == PDELAY_REQ_SM_EVENT_REQ_INTERVAL) {
			sm->req_to_ts = sm->ts_time - sm->req_time;
			sm->req_to_ts_min = min(sm->req_to_ts_min, sm->req_to_ts);
			sm->req_to_ts_max = max(sm->req_to_ts_max, sm->req_to_ts);
			sm->req_to_resp = sm->resp_time - sm->req_time;
			sm->req_to_fup = sm->fup_time - sm->req_time;
			os_log(LOG_DEBUG, "Port(%u): pdelay timing: req_to_ts %" PRIu64 " (min %" PRIu64 " max %" PRIu64 ") us \
				req_to_resp %" PRIu64 " us req_to_fup %" PRIu64 " us\n",
			       port->port_id, sm->req_to_ts / 1000, sm->req_to_ts_min / 1000, sm->req_to_ts_max / 1000, sm->req_to_resp / 1000, sm->req_to_fup / 1000);
			sm->ts_time = 0;
			sm->resp_time = 0;
			sm->fup_time = 0;
			md_pdelay_req_sm_send(port);
		}
		break;

	default:
		break;
	}

	os_log(LOG_DEBUG, "Port(%u): s %d state %s event %s new state %s\n",
	       port->port_id, sm->s, get_pdelay_req_state_str(state), get_pdelay_req_event_str(event), pdelay_req_state_str[sm->state]);

	/* 802.1AS-2020 - Handling of the transition from RESET to SEND is done here
	only upon timeout event (i.e. no transmit interval timer is running anymore). If the RESET state is entered due to an
	invalid response, meaning that the transmit interval timer is still running, the RESET state handler will be then trigger
	normaly upon timer timeout */
	if (state != sm->state) {
		if ((sm->state == PDELAY_REQ_SM_STATE_RESET) && (event == PDELAY_REQ_SM_EVENT_REQ_INTERVAL))
			event = PDELAY_REQ_SM_EVENT_REQ_INTERVAL;
		else
			event = PDELAY_REQ_SM_EVENT_RUN;

		goto start;
	}

exit:
	return rc;
}



/*
* MDPdelayRespSM state machine and funtions
*/

static void md_pdelay_resp_sm_initial_waiting_req(struct ptp_md_entity_pdelay_resp_sm *sm)
{
	sm->rcvdPdelayReq = false;

	sm->rcvdMDTimestampReceive = false;

	sm->state = PDELAY_RESP_SM_STATE_INITIAL_WAITING_FOR_PDELAY_REQ;
}


static void md_pdelay_resp_sm_waiting_req(struct gptp_port_common *port)
{
	struct ptp_md_entity_pdelay_resp_sm *sm = &port->md.pdelay_resp_sm;

	sm->rcvdMDTimestampReceive = false;

	sm->txPdelayRespFollowUpPtr = md_set_pdelay_resp_fup(port);

	gptp_net_tx(net_port_from_gptp_common(port), sm->txPdelayRespFollowUpPtr, sizeof(struct ptp_pdelay_resp_follow_up_pdu), 0, PTP_DOMAIN_0, (sm->s < 0)? PTP_CMLDS_MAJOR_SDOID : PTP_DOMAIN_MAJOR_SDOID);

	port->stats.num_tx_pdelayrespfup++;

	sm->state = PDELAY_RESP_SM_STATE_WAITING_FOR_PDELAY_REQ;
}


static void md_pdelay_resp_sm_waiting_timestamp(struct gptp_port_common *port)
{
	struct ptp_md_entity_pdelay_resp_sm *sm = &port->md.pdelay_resp_sm;

	sm->rcvdPdelayReq = false;

	sm->txPdelayRespPtr = md_set_pdelay_resp(port);

	gptp_net_tx(net_port_from_gptp_common(port), sm->txPdelayRespPtr, sizeof(struct ptp_pdelay_resp_pdu), 1, PTP_DOMAIN_0,  (sm->s < 0)? PTP_CMLDS_MAJOR_SDOID : PTP_DOMAIN_MAJOR_SDOID);

	port->stats.num_tx_pdelayresp++;

	sm->state = PDELAY_RESP_SM_STATE_SENT_PDELAY_RESP_WAITING_FOR_TIMESTAMP;
}

/** MDPdelayResp state machine (11.2.15)
 * The MDPdelayResp state machine is responsible for responding to Pdelay_Req messages, received
 * from the MD entity at the other end of the attached link, with Pdelay_Resp and Pdelay_Resp_Follow_Up messages
 * \return	0 on success or negative value on failure
 * \param port	pointer to the gptp port common context used to retrieve port params and state machine variables
 * \param event	value of the sync receive event triggering this state machine
*/
int md_pdelay_resp_sm(struct gptp_port_common *port, ptp_pdelay_resp_sm_event_t event)
{
	struct ptp_md_entity_pdelay_resp_sm *sm = &port->md.pdelay_resp_sm;
	struct ptp_port_params *params = &port->params;
	ptp_pdelay_resp_sm_state_t state = sm->state;
	bool port_oper = port->gptp->net_ports[port->port_id].port_oper;
	int rc = 0;

	if ((params->begin) ||
	(!port_oper) ||
	(!*sm->portEnabled1)) {
		sm->state = PDELAY_RESP_SM_STATE_NOT_ENABLED;
		goto exit;
	}

	switch (state) {
	case PDELAY_RESP_SM_STATE_NOT_ENABLED:
		if (*sm->portEnabled1)
			md_pdelay_resp_sm_initial_waiting_req(sm);

		break;

	case PDELAY_RESP_SM_STATE_WAITING_FOR_PDELAY_REQ:
	case PDELAY_RESP_SM_STATE_INITIAL_WAITING_FOR_PDELAY_REQ:
		if (sm->rcvdPdelayReq)
			md_pdelay_resp_sm_waiting_timestamp(port);

		break;

	case PDELAY_RESP_SM_STATE_SENT_PDELAY_RESP_WAITING_FOR_TIMESTAMP:
		if (sm->rcvdMDTimestampReceive) {
			md_pdelay_resp_sm_waiting_req(port);
		}
		else if (event == PDELAY_RESP_SM_EVENT_REQ_RECEIVED){
			/* FIXME. Should not happen and not in the specs.  Temporary WA to handle the case where a timestamp
			is never received upon ethernet cable disconnection. */
			os_log(LOG_INFO, "Port(%u): resetting upon timestamp not received in time (state %s event %s)\n", port->port_id, get_pdelay_resp_state_str(state), get_pdelay_resp_event_str(event));
			md_pdelay_resp_sm_initial_waiting_req(sm);
		}

		break;

	default:
		os_log(LOG_INFO, "Port(%u): ignoring (state %s event %s)\n", port->port_id, get_pdelay_resp_state_str(state), get_pdelay_resp_event_str(event));
		break;
	}

exit:
	os_log(LOG_DEBUG, "Port(%u): state %s event %s new state %s\n", port->port_id, get_pdelay_resp_state_str(state), get_pdelay_resp_event_str(event), pdelay_resp_state_str[sm->state]);

	return rc;
}


/*
* MDSyncRcvSM state machine and functions
*/

/** Creates an MDSyncReceive structure, and returns a pointer to this structure (11.2.13.2.1)
 *
 */
static struct md_sync_receive * md_sync_rcv_set_md_sync_receive(struct gptp_port *port, struct ptp_follow_up_pdu *fup)
{
	struct ptp_instance_md_entity *md = &port->md;
	struct ptp_u_scaled_ns sync_rx_ts_u_scaled_ns, tmp_u_scaled_ns;
	double delay_asymmetry_double;

	/* followUpCorrectionField is set equal to the correctionField (see 11.4.2.4) of the most recently
	received FollowUp message */
	pdu_correction_field_to_scaled_ns(&md->sync_rcv.followUpCorrectionField, fup->header.correction_field);
	os_memcpy(&md->sync_rcv.sourcePortIdentity, &port->sync_rx.header.source_port_id, sizeof(struct ptp_port_identity));

	/* logMessageInterval is set equal to the logMessageInterval (see 11.4.2.8) of the most recently
	received Sync message */
	md->sync_rcv.logMessageInterval = fup->header.log_msg_interval;

	/* preciseOriginTimestamp is set equal to the preciseOriginTimestamp (see 11.4.4.2.1) of the most
	recently received Follow_Up message */
	ntoh_ptp_timestamp(&md->sync_rcv.preciseOriginTimestamp, &fup->precise_origin_timestamp);

	/* rateRatio is set equal to the quantity (cumulativeScaledRateOffset x 2^-41 )+1.0, where the
	cumulativeScaledRateOffset field is for the most recently received Follow_Up message (see 11.4.4.3.6) */
	md->sync_rcv.rateRatio = ((double)(s32)ntohl(fup->tlv.cumulative_scaled_rate_offset) / (1ULL << 41)) + 1.0;
	os_log(LOG_DEBUG, "Port(%u): rateRatio %1.16f\n", port->port_id, md->sync_rcv.rateRatio);

	/* upstreamTxTime is set equal to the <syncEventIngressTimestamp> for the most recently received
	Sync message, minus the mean propagation time on the link attached to this port
	(neighborPropDelay, see 10.2.4.7) divided by neighborRateRatio (see 10.2.4.6), minus
	delayAsymmetry (see 10.2.4.8) for this port divided by rateRatio [see e) above]. The
	<syncEventIngressTimestamp> is equal to the timestamp value measured relative to the timestamp
	measurement plane, minus any ingressLatency (see 8.4.3), */
	u64_to_u_scaled_ns(&sync_rx_ts_u_scaled_ns, port->sync_rx_ts);
	os_log(LOG_DEBUG, "fup_rx_ts_us %"PRIu64"\n", sync_rx_ts_u_scaled_ns.u.s.nanoseconds);
	u_scaled_ns_sub(&md->sync_rcv.upstreamTxTime, &sync_rx_ts_u_scaled_ns, get_mean_link_delay(port));

	os_log(LOG_DEBUG, "Port(%u): upstreamTxTime %u:%"PRIu64":%u\n", port->port_id,
		md->sync_rcv.upstreamTxTime.u.s.nanoseconds_msb,
		md->sync_rcv.upstreamTxTime.u.s.nanoseconds,
		md->sync_rcv.upstreamTxTime.u.s.fractional_nanoseconds);

	scaled_ns_to_ptp_double(&delay_asymmetry_double, get_delay_asymmetry(port));
	ptp_double_to_u_scaled_ns(&tmp_u_scaled_ns, (delay_asymmetry_double / md->sync_rcv.rateRatio));
	u_scaled_ns_sub(&md->sync_rcv.upstreamTxTime, &md->sync_rcv.upstreamTxTime, &tmp_u_scaled_ns);

	os_log(LOG_DEBUG, "Port(%u): upstreamTxTime %u:%"PRIu64":%u\n", port->port_id,
		md->sync_rcv.upstreamTxTime.u.s.nanoseconds_msb,
		md->sync_rcv.upstreamTxTime.u.s.nanoseconds,
		md->sync_rcv.upstreamTxTime.u.s.fractional_nanoseconds);

	/* gmTimeBaseIndicator is set equal to the gmTimeBaseIndicator of the most recently received
	Follow_Up message (see 11.4.4) */
	md->sync_rcv.gmTimeBaseIndicator = ntohs(fup->tlv.gm_time_base_indicator);

	/* lastGmPhaseChange is set equal to the lastGmPhaseChange of the most recently received
	Follow_Up message (see 11.4.4) */
	ntoh_scaled_ns(&md->sync_rcv.lastGmPhaseChange, &fup->tlv.last_gm_phase_change);

	/* lastGmFreqChange is set equal to the lastGmFreqChange of the most recently received Follow_Up
	message (see 11.4.4) */
	md->sync_rcv.lastGmFreqChange = ((double)(s32)ntohl(fup->tlv.scaled_last_gm_freq_change) / (1ULL << 41));

	return &md->sync_rcv;
}


/** Transmits an MDSyncReceive structure to the PortSyncSyncReceive state machine of the PortSync entity of this port (11.2.13.2.1)
 *
 */
static void md_sync_rcv_tx_md_sync_receive(struct gptp_port *port, struct md_sync_receive *sync_rcv)
{
	port->port_sync.sync_receive_sm.rcvdMDSync = true;
	port->port_sync.sync_receive_sm.rcvdMDSyncPtr = sync_rcv;

	port_sync_sync_rcv_sm(port);
}


static void md_sync_rcv_sm_discard(struct gptp_port *port)
{
	struct ptp_instance_md_entity *md = &port->md;

	port->stats.num_md_sync_rcv_sm_reset++;

	md->sync_rcv_sm.rcvdSync = false;
	md->sync_rcv_sm.rcvdFollowUp = false;

	port->sync_rcv_sm_state = SYNC_RCV_SM_STATE_DISCARD;
}

static void md_sync_rcv_sm_waiting_for_follow_up(struct gptp_port *port)
{
	struct ptp_instance_md_entity *md = &port->md;

	md->sync_rcv_sm.rcvdSync = false;
	md->sync_rcv_sm.upstreamSyncInterval = md->sync_rcv_sm.rcvdSyncPtr->header.log_msg_interval;

	/* start a timer waiting for sync follow up reception */
	os_log(LOG_DEBUG, "Port(%u): timer(%p) %d ms\n", port->port_id, &port->follow_up_receive_timeout_timer, log_to_ms(md->sync_rcv_sm.upstreamSyncInterval));

	timer_restart(&port->follow_up_receive_timeout_timer, log_to_ms(md->sync_rcv_sm.upstreamSyncInterval));

	port->sync_rcv_sm_state = SYNC_RCV_SM_STATE_WAITING_FOR_FOLLOW_UP;
}


static void md_sync_rcv_sm_waiting_for_sync(struct gptp_port *port)
{
	struct ptp_instance_md_entity *md = &port->md;

	md->sync_rcv_sm.rcvdFollowUp = false;

	/* fill in a MDSyncReceive structure and sends it to the PortSyncSyncReceive state machine of the PortSync entrity */
	md->sync_rcv_sm.txMDSyncReceivePtr = md_sync_rcv_set_md_sync_receive(port, md->sync_rcv_sm.rcvdFollowUpPtr);
	md_sync_rcv_tx_md_sync_receive(port, md->sync_rcv_sm.txMDSyncReceivePtr);

	port->sync_rcv_sm_state = SYNC_RCV_SM_STATE_WAITING_FOR_SYNC;
}

/** MDSyncRcvSM state machine (802.1AS-2020: 11.2.14.3)
 * The MDSyncRcvSM state machine receives Sync and Follow_Up messages,
 * places the time-synchronization information in an MDSyncReceive structure, and sends the structure to the
 * PortSyncSyncReceive state machine of the PortSync entity of this port
 * \return	0 on success or negative value on failure
 * \param port	pointer to the gptp port common context used to retrieve port params and state machine variables
 * \param event	value of the sync receive event triggering this state machine
 */
int md_sync_rcv_sm(struct gptp_port *port, ptp_sync_rcv_sm_event_t event)
{
	struct ptp_instance_md_entity *md = &port->md;
	ptp_sync_rcv_sm_state_t state = port->sync_rcv_sm_state;
	struct ptp_instance_params *params = &port->instance->params;
	bool port_oper = get_port_oper(port);
	int rc = 0;

	if ((params->begin) || (!params->instance_enable) ||
	    ((md->sync_rcv_sm.rcvdSync) && ((!port_oper) ||
						(!port->params.ptp_port_enabled) ||
						(!port->params.as_capable)))) {
		if (md->sync_rcv_sm.rcvdSync)
			os_log(LOG_INFO, "Port(%u): resetting upon link not ready (state %s - %d %d %d)\n",
				port->port_id, get_sync_rcv_state_str(state), port_oper, port->params.ptp_port_enabled, port->params.as_capable);

		md_sync_rcv_sm_discard(port);
		goto exit;
	}

start:
	state = port->sync_rcv_sm_state;

	switch (state) {
	case SYNC_RCV_SM_STATE_DISCARD:
		if ((md->sync_rcv_sm.rcvdSync) &&
			(port_oper) &&
			(port->params.ptp_port_enabled) &&
			(port->params.as_capable)) {

			md_sync_rcv_sm_waiting_for_follow_up(port);
		}
		break;

	case SYNC_RCV_SM_STATE_WAITING_FOR_FOLLOW_UP:
		if ((!port->instance->gptp->force_2011) && /* If forced to 802.1AS-2011 spec, do not reset the state on SYNC receive here. */
			((md->sync_rcv_sm.rcvdSync) &&
				(port_oper) &&
				(port->params.ptp_port_enabled) &&
				(port->params.as_capable))) {

			md_sync_rcv_sm_waiting_for_follow_up(port);
		} else if (md->sync_rcv_sm.rcvdFollowUp) {
			if (md->sync_rcv_sm.rcvdFollowUpPtr->header.sequence_id == md->sync_rcv_sm.rcvdSyncPtr->header.sequence_id) {
				/* stop the timer in case a correct FUP has been received before timeout */
				timer_stop(&port->follow_up_receive_timeout_timer);

				md_sync_rcv_sm_waiting_for_sync(port);
			} else {
				os_log(LOG_INFO, "Port(%u): bad sequence id (state %s - FUP(%u) SYNC(%u))\n",
					port->port_id, get_sync_rcv_state_str(state), md->sync_rcv_sm.rcvdFollowUpPtr->header.sequence_id,
					md->sync_rcv_sm.rcvdSyncPtr->header.sequence_id);
			}
		} else if (event == SYNC_RCV_SM_EVENT_FUP_TIMEOUT) {
			os_log(LOG_INFO, "Port(%u): resetting upon timeout (state %s)\n", port->port_id, get_sync_rcv_state_str(state));
			md_sync_rcv_sm_discard(port);
		}
		break;

	case SYNC_RCV_SM_STATE_WAITING_FOR_SYNC:
		if ((md->sync_rcv_sm.rcvdSync) &&
			(port_oper) &&
			(port->params.ptp_port_enabled) &&
			(port->params.as_capable)) {

			md_sync_rcv_sm_waiting_for_follow_up(port);
		}
		break;

	default:
		break;
	}

exit:
	os_log(LOG_DEBUG, "Port(%u): domain(%u, %u) state %s event %s new state %s\n",
		port->port_id, port->instance->index, port->instance->domain.domain_number, get_sync_rcv_state_str(state), get_sync_rcv_event_str(event), sync_rcv_state_str[port->sync_rcv_sm_state]);

	if (state != port->sync_rcv_sm_state) {
		event = SYNC_RCV_SM_EVENT_RUN;
		goto start;
	}

	return rc;
}

static struct ptp_sync_pdu *set_sync(struct gptp_port *port)
{
	return md_set_sync(port);
}

static void tx_sync(struct gptp_port *port, struct ptp_sync_pdu *sync)
{
	port->stats.num_tx_sync++;

	gptp_net_tx(net_port_from_gptp(port), sync, sizeof(struct ptp_sync_pdu), 1, port->instance->domain.domain_number, PTP_DOMAIN_MAJOR_SDOID);
}

static struct ptp_follow_up_pdu *set_follow_up(struct gptp_port *port)
{
	return md_set_follow_up(port);
}

static void tx_follow_up(struct gptp_port *port, struct ptp_follow_up_pdu *fup)
{
	port->stats.num_tx_fup++;

	gptp_net_tx(net_port_from_gptp(port), fup, sizeof(struct ptp_follow_up_pdu), 0, port->instance->domain.domain_number, PTP_DOMAIN_MAJOR_SDOID);
}

static void md_sync_send_sm_initializing(struct gptp_port *port)
{
	struct ptp_instance_md_entity *md = &port->md;

	md->sync_send_sm.rcvdMDSync = false;
	md->sync_send_sm.rcvdMDTimestampReceive = false;
	port->sync_tx_seqnum = sequence_id_random();

	port->sync_snd_sm_state = SYNC_SND_SM_STATE_INITIALIZING;
}

static void md_sync_send_sm_send_sync(struct gptp_port *port)
{
	struct ptp_instance_md_entity *md = &port->md;

	md->sync_send_sm.rcvdMDSync = false;
	md->sync_send_sm.txSyncPtr = set_sync(port);
	tx_sync(port, md->sync_send_sm.txSyncPtr);

	port->sync_tx_seqnum++;

	port->sync_snd_sm_state = SYNC_SND_SM_STATE_SEND_SYNC;
}

static void md_sync_send_sm_send_follow_up(struct gptp_port *port)
{
	struct ptp_instance_md_entity *md = &port->md;

	md->sync_send_sm.rcvdMDTimestampReceive = false;
	md->sync_send_sm.txFollowUpPtr = set_follow_up(port);
	tx_follow_up(port, md->sync_send_sm.txFollowUpPtr);

	port->sync_snd_sm_state = SYNC_SND_SM_STATE_SEND_FOLLOW_UP;
}


/** MDSyncSendSM state machine
 * The MDSyncSendSM state machine receives an MDSyncSend structure from the PortSyncSyncSend state
 * machine of the PortSync entity of this port and transmits a Sync and corresponding Follow_Up message
 * \return	0 on success or negative value on failure
 * \param port	pointer to the gptp port common context used to retrieve port params and state machine variables
 * \param event	value of the sync send event triggering this state machine
 */

int md_sync_send_sm(struct gptp_port *port, ptp_sync_snd_sm_event_t event)
{
	ptp_sync_snd_sm_state_t state = port->sync_snd_sm_state;
	struct ptp_instance_params *params = &port->instance->params;
	struct ptp_instance_md_entity *md = &port->md;
	bool port_oper = get_port_oper(port);
	int rc = 0;

	if ((params->begin) || (!params->instance_enable) ||
	(md->sync_send_sm.rcvdMDSync && ((!port_oper) || (!port->params.ptp_port_enabled) || (!port->params.as_capable)))) {
		md_sync_send_sm_initializing(port);
		goto exit;
	}

resend:
	switch (state) {
	case SYNC_SND_SM_STATE_INITIALIZING:
	case SYNC_SND_SM_STATE_SEND_FOLLOW_UP:
		if (md->sync_send_sm.rcvdMDSync &&
		port_oper &&
		port->params.ptp_port_enabled &&
		port->params.as_capable)
			md_sync_send_sm_send_sync(port);
		break;

	case SYNC_SND_SM_STATE_SEND_SYNC:
		if (md->sync_send_sm.rcvdMDTimestampReceive)
			md_sync_send_sm_send_follow_up(port);
		else if (md->sync_send_sm.rcvdMDSync) {
			/* FIXME. Should not happen and not in the specs.  Temporary WA to handle the case where a timestamp
			is never received upon ethernet cable disconnection. */
			os_log(LOG_DEBUG, "Port(%u) domain(%u, %u): resetting upon timestamp not received in time (state %s event %s)\n",
				port->port_id, port->instance->index, port->instance->domain.domain_number, get_sync_snd_state_str(state),
				get_sync_snd_event_str(event));
			state = SYNC_SND_SM_STATE_INITIALIZING;
			goto resend;
		}
		break;

	default:
		break;
	}
exit:
	os_log(LOG_DEBUG, "Port(%u) domain(%u, %u): state %s event %s new state %s\n", port->port_id, port->instance->index,
		port->instance->domain.domain_number, get_sync_snd_state_str(state), get_sync_snd_event_str(event),
		sync_snd_state_str[port->sync_snd_sm_state]);

	return rc;
}


/** 11.2.21.3.1 isSupportedLogPdelayReqInterval (logPdelayReqInterval): A Boolean function that returns
TRUE if the Pdelay_Req interval given by the argument logPdelaytReqInterval is supported by the PTP Port
and FALSE if the Pdelay_Req interval is not supported by the PTP Port. The argument
logPdelayReqInterval has the same data type and format as the field logLinkDelayInterval of the message
interval request TLV (see 10.6.4.3.6)
*/
static bool is_supported_log_pdelay_req_interval(struct ptp_md_link_delay_interval_setting_sm *sm, s8 log_pdelay_req_interval)
{
	if ((log_pdelay_req_interval < sm->log_supported_closest_longer_pdelayreq_Interval) || (log_pdelay_req_interval > sm->log_supported_pdelayreq_interval_max))
		return false;
	else
		return true;
}


/** 11.2.21.3.2 computeLogPdelayReqInterval (logRequestedPdelayReqInterval): An Integer8 function
that computes and returns the logPdelayReqInterval, based on the logRequestedPdelayReqInterval. This
function is defined as indicated below. It is defined here so that the detailed code that it invokes does not
need to be placed into the state machine diagram.
*/
static s8 compute_log_pdelay_req_interval(struct ptp_md_link_delay_interval_setting_sm *sm, s8 log_requested_pdelay_req_interval)
{
	if (is_supported_log_pdelay_req_interval (sm, log_requested_pdelay_req_interval)) {
		/* The requested Sync Interval is supported and returned */
		return log_requested_pdelay_req_interval;
	} else {
		if (log_requested_pdelay_req_interval > sm->log_supported_pdelayreq_interval_max) {
			/* Return the fastest supported rate, even if faster than the requested rate */
			return sm->log_supported_pdelayreq_interval_max;
		} else {
			/* Return the fastest supported rate that is still slower than the requested rate. */
			return sm->log_supported_closest_longer_pdelayreq_Interval;
		}
	}
}

static void md_link_delay_interval_setting_sm_not_enabled(struct gptp_port_common *port)
{
	struct ptp_md_link_delay_interval_setting_sm *sm = &port->md.link_interval_sm;
	struct ptp_port_params *params = &port->params;

	if (port->use_mgt_settable_log_pdelayreq_interval) {
		port->md.globals.currentLogPdelayReqInterval = port->mgt_settable_log_pdelayreq_interval;
		u64_to_u_scaled_ns(&port->md.globals.pdelayReqInterval, log_to_ns(port->md.globals.currentLogPdelayReqInterval));
	}

	if (port->use_mgt_settable_compute_neighbor_rate_ratio) {
		params->current_compute_neighbor_rate_ratio = port->mgt_settable_compute_neighbor_rate_ratio;
		params->compute_neighbor_rate_ratio = params->current_compute_neighbor_rate_ratio;
	}

	if (port->use_mgt_settable_compute_mean_link_delay) {
		params->current_compute_mean_link_delay = port->mgt_settable_compute_mean_link_delay;
		params->compute_mean_link_delay = params->current_compute_mean_link_delay;
	}

	sm->state = LINK_INTERVAL_SETTING_SM_STATE_NOT_ENABLED;
}

static void md_link_delay_interval_setting_sm_initialize(struct gptp_port_common *port)
{
	struct ptp_md_link_delay_interval_setting_sm *sm = &port->md.link_interval_sm;
	struct ptp_port_params *params = &port->params;
	u64 pdelay_req_interval;

	if (!port->use_mgt_settable_log_pdelayreq_interval) {
		port->md.globals.currentLogPdelayReqInterval = port->md.globals.initialLogPdelayReqInterval;

		pdelay_req_interval = log_to_ns(port->md.globals.currentLogPdelayReqInterval);
		u64_to_u_scaled_ns(&port->md.globals.pdelayReqInterval, pdelay_req_interval);

		os_log(LOG_INFO, "Port(%u): initialLogPdelayReqInterval %d (%"PRIu64" ms)\n",
			port->port_id, port->md.globals.initialLogPdelayReqInterval, pdelay_req_interval / NS_PER_MS);
	}

	if (!port->use_mgt_settable_compute_neighbor_rate_ratio)
		params->current_compute_neighbor_rate_ratio = params->compute_neighbor_rate_ratio = params->initial_compute_neighbor_rate_ratio;

	if (!port->use_mgt_settable_compute_mean_link_delay)
		params->current_compute_mean_link_delay = params->compute_mean_link_delay = params->initial_compute_mean_link_delay;

	sm->rcvd_signaling_msg1 = false;

	sm->state = LINK_INTERVAL_SETTING_SM_STATE_INITIALIZE;
}

/*
* 11.5.2.2
* The currentLogPdelayReqInterval specifies the current value of the mean time interval between successive
* Pdelay_Req messages. The default value of initialLogPdelayReqInterval is 0. Every port supports the value
* 127; the port does not send Pdelay_Req messages when currentLogPdelayReqInterval has this value (see ).
* A port may support other values, except for the reserved values -128 through -125, inclusive, and 124
* through 126, inclusive. A port shall ignore requests (see ) for unsupported values
*
* 11.5.2.3
* Every port supports the value 127; the port
* does not send Sync messages when currentLogSyncInterval has this value (see ). A port may support other
* values, except for the reserved values -128 through -125, inclusive, and 124 through 126, inclusive. A port
* ignores requests (see ) for unsupported values.
*/
static void md_link_delay_interval_setting_sm_set_interval(struct gptp_port_common *port)
{
	struct ptp_md_link_delay_interval_setting_sm *sm = &port->md.link_interval_sm;
	struct ptp_port_params *params = &port->params;
	s8 computed_pdelay_req_interval;
	u64 pdelay_req_interval = 0;

	if (!port->use_mgt_settable_log_pdelayreq_interval) {
		switch (sm->rcvd_signaling_ptr_ldis->u.itlv.link_delay_interval)
		{
		case (-128): /* dont change the interval */
			pdelay_req_interval = log_to_ns(port->md.globals.currentLogPdelayReqInterval);

			break;

		case 126: /* set interval to initial value */
			pdelay_req_interval = log_to_ns(port->md.globals.initialLogPdelayReqInterval);
			u64_to_u_scaled_ns(&port->md.globals.pdelayReqInterval, pdelay_req_interval);
			timer_restart(&port->md.pdelay_req_sm.req_timer, pdelay_req_interval / NS_PER_MS);
			port->md.globals.currentLogPdelayReqInterval = port->md.globals.initialLogPdelayReqInterval;

			break;

		default: /* use indicated value; note that the value of 127 will result in an interval of
			* 2 127 s, or approximately 5.4 u 10 30 years, which indicates that the Pdelay
			* requester should stop sending for all practical purposes, in accordance
			* with Table 10-9. */
			if (sm->rcvd_signaling_ptr_ldis->u.itlv.link_delay_interval == 127) {
				if(timer_is_running(&port->md.pdelay_req_sm.req_timer))
					timer_stop(&port->md.pdelay_req_sm.req_timer);
				port->md.globals.currentLogPdelayReqInterval = sm->rcvd_signaling_ptr_ldis->u.itlv.link_delay_interval;
			} else {
				/* supported values, apply new interval */
				computed_pdelay_req_interval = compute_log_pdelay_req_interval(sm, sm->rcvd_signaling_ptr_ldis->u.itlv.link_delay_interval);
				pdelay_req_interval = log_to_ns(computed_pdelay_req_interval);

				u64_to_u_scaled_ns(&port->md.globals.pdelayReqInterval, pdelay_req_interval);
				timer_restart(&port->md.pdelay_req_sm.req_timer, pdelay_req_interval / NS_PER_MS);
				port->md.globals.currentLogPdelayReqInterval = computed_pdelay_req_interval;
			}

			break;
		}
	}

	os_log(LOG_DEBUG, "Port(%u): pdelayreq_interval %"PRIu64" ms (log %d)\n", port->port_id, pdelay_req_interval / NS_PER_MS, port->md.globals.currentLogPdelayReqInterval);

	/* interval TLV bit field as defined in 802.1AS - 10.5.4.3.9 */
	if (!port->use_mgt_settable_compute_neighbor_rate_ratio) {
		params->current_compute_neighbor_rate_ratio = !!(sm->rcvd_signaling_ptr_ldis->u.itlv.flags & ITLV_FLAGS_COMPUTE_RATIO_MASK);
		params->compute_neighbor_rate_ratio = params->current_compute_neighbor_rate_ratio;
	}

	if (!port->use_mgt_settable_compute_mean_link_delay) {
		params->current_compute_mean_link_delay = !!(sm->rcvd_signaling_ptr_ldis->u.itlv.flags & ITLV_FLAGS_COMPUTE_DELAY_MASK);
		params->compute_mean_link_delay = params->current_compute_mean_link_delay;
	}

	os_log(LOG_DEBUG, "Port(%u): compute_neighbor_rate_ratio %d compute_mean_link_delay %d\n", port->port_id, params->compute_neighbor_rate_ratio, params->compute_mean_link_delay);

	sm->rcvd_signaling_msg1 = false;

	sm->state = LINK_INTERVAL_SETTING_SM_STATE_SET_INTERVAL;
}


/** LinkDelayIntervalSetting state machine (802.1AS-2020 section 11.2.21)
 * This state machine is responsible for setting the global variables that give the duration of the
 * mean intervals between successive Sync and successive Pdelay_Req messages, both at initialization and in
 * response to the receipt of a Signaling message that contains a Message Interval Request TLV
 * \return	0 on success or negative value on failure
 * \param port	pointer to the gptp port common context used to retrieve port params and state machine variables
 * \param event	value of the link interval signaling event triggering this state machine
 */
void md_link_delay_interval_setting_sm(struct gptp_port_common *port)
{
	struct ptp_md_link_delay_interval_setting_sm *sm = &port->md.link_interval_sm;
	ptp_link_interval_setting_sm_state_t state = sm->state;
	struct ptp_port_params *params = &port->params;
	bool port_oper = port->gptp->net_ports[port->port_id].port_oper;

	if ((params->begin) ||
	(!port_oper) ||
	(!*sm->portEnabled3) ||
	(port->use_mgt_settable_log_pdelayreq_interval) ||
	(port->use_mgt_settable_compute_neighbor_rate_ratio) ||
	(port->use_mgt_settable_compute_mean_link_delay)) {
		md_link_delay_interval_setting_sm_not_enabled(port);
	}

	if ((params->begin) || (!port_oper) || (!*sm->portEnabled3))
		goto exit;

	switch (state) {
	case LINK_INTERVAL_SETTING_SM_STATE_NOT_ENABLED:
		if (port_oper && *sm->portEnabled3)
			md_link_delay_interval_setting_sm_initialize(port);

		break;

	case LINK_INTERVAL_SETTING_SM_STATE_INITIALIZE:
	case LINK_INTERVAL_SETTING_SM_STATE_SET_INTERVAL:
		if (sm->rcvd_signaling_msg1)
			md_link_delay_interval_setting_sm_set_interval(port);

		break;

	default:
		break;
	}

exit:
	os_log(LOG_DEBUG, "Port(%u): state %s new state %s\n", port->port_id, get_link_interval_setting_state_str(state), get_link_interval_setting_state_str(sm->state));
}


static struct ptp_signaling_pdu * md_link_set_delay_sync(struct gptp_port *port, ptp_link_delay_transmit_sm_event_t event)
{
	struct ptp_signaling_pdu *msg = &port->signaling_tx;

	os_log(LOG_DEBUG, "Port(%u)\n", port->port_id);

	md_set_common_header(msg, port->instance->domain.domain_number);

	msg->header.msg_type = PTP_MSG_TYPE_SIGNALING;
	msg->header.msg_length = htons(sizeof(struct ptp_signaling_pdu));

	if (port->instance->gptp->force_2011)
		msg->header.flags = htons(PTP_FLAG_PTP_TIMESCALE);
	else
		msg->header.minor_version_ptp = PTP_MINOR_VERSION;

	msg->header.source_port_id.port_number = htons(get_port_identity_number(port));
	copy_64(msg->header.source_port_id.clock_identity, get_port_identity_clock_id(port));
	msg->header.sequence_id = htons(port->signaling_tx_seqnum);
	port->signaling_tx_seqnum++;
	msg->header.control = 0;
	msg->header.log_msg_interval = 0;

	os_memcpy(&msg->target_port_identity, get_port_identity(port), sizeof(struct ptp_port_identity));
	os_memset(&msg->u.itlv, 0, sizeof(struct ptp_interval_tlv));

	/* 802.1AS - 10.5.4.3 */
	msg->tlv_type = htons(PTP_TLV_TYPE_ORGANIZATION_EXTENSION);
	msg->length_field = htons(12);
	msg->organization_id[0] = 0x00;
	msg->organization_id[1] = 0x80;
	msg->organization_id[2] = 0xC2;
	msg->organization_sub_type[0] = 0x00;
	msg->organization_sub_type[1] = 0x00;
	msg->organization_sub_type[2] = PTP_TLV_SUBTYPE_INTERVAL_REQUEST;
	if (port->ratio_is_valid)
		msg->u.itlv.flags = ITLV_FLAGS_COMPUTE_RATIO_MASK | ITLV_FLAGS_COMPUTE_DELAY_MASK;
	else
		msg->u.itlv.flags = 0;

	if (event == LINK_TRANSMIT_SM_EVENT_INITIAL) {
		/*
		request the peer to use initial (fast) delay/sync/announce log intervals values using special values
		as defined by 802.1AS - 10.5.4.3.6 / 10.5.4.3.7
		*/
		msg->u.itlv.link_delay_interval = 126;
		msg->u.itlv.time_sync_interval = 126;
		msg->u.itlv.announce_interval = 126;
	} else {
		/* Avnu AutoCDS - 6.2.1.5
		operLogPdelayReqInterval is the operational Pdelay request interval. A device moves to this value on all slave
		ports once the measured values have stabilized
		*/
		msg->u.itlv.link_delay_interval = port->cfg.operLogPdelayReqInterval;

		/* Avnu AutoCDS - 6.2.1.6
		operLogSyncInterval is the Sync interval that a device moves to and signals on a slave port once it has achieved
		synchronization.
		*/
		msg->u.itlv.time_sync_interval = port->cfg.operLogSyncInterval;

		/*
		Note: we are using the special value -128 (meaning do not change rate) to make sure the peer does not deduce a
		value of 0 for announce interval when decoding this TLVs.
		*/
		msg->u.itlv.announce_interval = (-128);
	}

	return msg;
}

static int md_link_delay_sync_transmit(struct gptp_port *port, ptp_link_delay_transmit_sm_event_t event)
{
	struct ptp_signaling_pdu *msg = md_link_set_delay_sync(port, event);

	port->stats.num_tx_sig++;

	return gptp_net_tx(net_port_from_gptp(port), msg, sizeof(struct ptp_signaling_pdu), 0, port->instance->domain.domain_number, PTP_DOMAIN_MAJOR_SDOID);
}

/* This SM is not in the 802.1AS specs, but required to control transmit of signaling messages
(e.g. upon slave transition from not sync to sync state in order to ask the GM to slow down
sync packet rate)
*/
int md_link_delay_sync_transmit_sm(struct gptp_port *port, ptp_link_delay_transmit_sm_event_t event)
{
	ptp_link_delay_transmit_sm_state_t state = port->link_transmit_sm_state;
	int rc = 0;

transmit:
	switch (event) {
	case LINK_TRANSMIT_SM_EVENT_INITIAL:
		rc = md_link_delay_sync_transmit(port, event);
		port->link_transmit_sm_state = LINK_TRANSMIT_SM_STATE_INITIAL;
		os_log(LOG_DEBUG, "Port(%u): sent signaling to switch to initial settings\n", port->port_id);
		break;

	case LINK_TRANSMIT_SM_EVENT_OPER:
		rc = md_link_delay_sync_transmit(port, event);
		port->link_transmit_sm_state = LINK_TRANSMIT_SM_STATE_OPER;
		os_log(LOG_DEBUG, "Port(%u): sent signaling to switch to operational settings\n", port->port_id);
		break;

	case LINK_TRANSMIT_SM_EVENT_RATIO_NOT_VALID:
	case LINK_TRANSMIT_SM_EVENT_RATIO_VALID:
		os_log(LOG_DEBUG, "Port(%u): sent signaling to toggle ratio calculation (event %d)\n", port->port_id, event);
		if (port->link_transmit_sm_state == LINK_TRANSMIT_SM_STATE_INITIAL)
			event = LINK_TRANSMIT_SM_EVENT_INITIAL;
		else
			event = LINK_TRANSMIT_SM_EVENT_OPER;
		goto transmit;
		break;

	default:
		break;
	}

	os_log(LOG_DEBUG, "Port(%u): domain(%u, %u) state %d new state %d event %d\n", port->port_id, port->instance->index, port->instance->domain.domain_number, state, port->link_transmit_sm_state, event);

	return rc;
}
