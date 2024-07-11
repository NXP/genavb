/*
 * Copyright 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Implementation of 802.1 AS Port entity State Machines
 @details Implementation of 802.1 AS state machine and functions for Port entity
*/

#include "port_fsm.h"
#include "bmca.h"
#include "site_fsm.h"
#include "md_fsm_802_3.h"
#include "gptp.h"
#include "common/ptp_time_ops.h"


static const char *port_sync_sync_rcv_sm_state_str[] = {
	[PORT_SYNC_SYNC_RCV_SM_STATE_DISCARD] = "DISCARD",
	[PORT_SYNC_SYNC_RCV_SM_STATE_RECEIVED_MDSYNC] = "RECEIVED MDSYNC"
};

static const char *port_sync_sync_send_sm_state_str[] = {
	[PORT_SYNC_SYNC_SEND_SM_STATE_TRANSMIT_INIT] = "INIT",
	[PORT_SYNC_SYNC_SEND_SM_STATE_SEND_MD_SYNC] = "SEND MD SYNC",
	[PORT_SYNC_SYNC_SEND_SM_STATE_SYNC_RECEIPT_TIMEOUT] = "SYNC RECEIPT TIMEOUT",
};

static const char *port_sync_sync_send_sm_event_str[] = {
	[PORT_SYNC_SYNC_SEND_SM_EVENT_PSSYNC_TIMEOUT] ="SYNC TIMEOUT",
	[PORT_SYNC_SYNC_SEND_SM_EVENT_RUN] ="RUN"
};

static const char *port_announce_rcv_sm_state_str[] = {
	[PORT_ANNOUNCE_RCV_SM_STATE_DISCARD] = "DISCARD",
	[PORT_ANNOUNCE_RCV_SM_STATE_RECEIVE] = "RECEIVE"
};

static const char *port_announce_transmit_sm_state_str[] = {
	[PORT_ANNOUNCE_TRANSMIT_SM_STATE_IDLE] = "IDLE",
	[PORT_ANNOUNCE_TRANSMIT_SM_STATE_TRANSMIT_ANNOUNCE] = "ANNOUNCE",
	[PORT_ANNOUNCE_TRANSMIT_SM_STATE_TRANSMIT_PERIODIC] = "PERIODIC",
	[PORT_ANNOUNCE_TRANSMIT_SM_STATE_TRANSMIT_INIT] = "INIT"
};

static const char *port_announce_transmit_sm_event_str[] = {
	[PORT_ANNOUNCE_TRANSMIT_SM_EVENT_TRANSMIT_INTERVAL] = "TRANSMIT INTERVAL",
	[PORT_ANNOUNCE_TRANSMIT_SM_EVENT_RUN] = "RUN"
};

static const char *port_announce_info_sm_state_str[] = {
	[PORT_ANNOUNCE_INFO_SM_STATE_DISABLED] = "DISABLED",
	[PORT_ANNOUNCE_INFO_SM_STATE_AGED] = "AGED",
	[PORT_ANNOUNCE_INFO_SM_STATE_UPDATE] = "UPDATE",
	[PORT_ANNOUNCE_INFO_SM_STATE_CURRENT] = "CURRENT",
	[PORT_ANNOUNCE_INFO_SM_STATE_RECEIVE] = "RECEIVE",
	[PORT_ANNOUNCE_INFO_SM_STATE_SUPERIOR_MASTER_PORT] = "SUPERIOR_MASTER",
	[PORT_ANNOUNCE_INFO_SM_STATE_REPEATED_MASTER_PORT] = "REPEATED_MASTER",
	[PORT_ANNOUNCE_INFO_SM_STATE_INFERIOR_MASTER_OR_OTHER] = "INFERIOR_MASTER"
};

static const char *port_announce_info_sm_event_str[] = {
	[PORT_ANNOUNCE_INFO_SM_EVENT_SYNC_TIMEOUT] = "SYNC_TIMEOUT",
	[PORT_ANNOUNCE_INFO_SM_EVENT_ANNOUNCE_TIMEOUT] = "ANNOUNCE_TIMEOUT",
	[PORT_ANNOUNCE_INFO_SM_EVENT_RUN] = "RUN"
};

static const char *port_announce_interval_state_str[] = {
	[PORT_ANNOUNCE_INTERVAL_SM_STATE_NOT_ENABLED]= "NOT_ENABLED",
	[PORT_ANNOUNCE_INTERVAL_SM_STATE_INITIALIZE]= "INITIALIZE",
	[PORT_ANNOUNCE_INTERVAL_SM_STATE_SET_INTERVAL]= "SET_INTERVAL"
};

static const char *sync_interval_setting_state_str[] = {
	[SYNC_INTERVAL_SETTING_SM_STATE_NOT_ENABLED]= "NOT_ENABLED",
	[SYNC_INTERVAL_SETTING_SM_STATE_INITIALIZE]= "INITIALIZE",
	[SYNC_INTERVAL_SETTING_SM_STATE_SET_INTERVAL]= "SET_INTERVAL",
};

static const char *get_sync_interval_setting_state_str(ptp_sync_interval_setting_sm_state_t state)
{
	if (state > SYNC_INTERVAL_SETTING_SM_STATE_MAX_VALUE || state < SYNC_INTERVAL_SETTING_SM_STATE_MIN_VALUE)
		return "Unknown PTP sync interval setting state";
	else
		return sync_interval_setting_state_str[state];
}

static const char *gptp_capable_receive_state_str[] = {
	[GPTP_CAPABLE_RECEIVE_SM_STATE_NOT_ENABLED]= "NOT_ENABLED",
	[GPTP_CAPABLE_RECEIVE_SM_STATE_INITIALIZE]= "INITIALIZE",
	[GPTP_CAPABLE_RECEIVE_SM_STATE_RECEIVED_TLV]= "RECEIVED_TLV",
};

static const char *get_gptp_capable_receive_state_str(ptp_gptp_capable_receive_sm_state_t state)
{
	if (state > GPTP_CAPABLE_RECEIVE_SM_STATE_MAX_VALUE || state < GPTP_CAPABLE_RECEIVE_SM_STATE_MIN_VALUE)
		return "Unknown gPTP capable receive state";
	else
		return gptp_capable_receive_state_str[state];
}

static const char *gptp_capable_transmit_state_str[] = {
	[GPTP_CAPABLE_TRANSMIT_SM_STATE_NOT_ENABLED]= "NOT_ENABLED",
	[GPTP_CAPABLE_TRANSMIT_SM_STATE_INITIALIZE]= "INITIALIZE",
	[GPTP_CAPABLE_TRANSMIT_SM_STATE_TRANSMIT_TLV]= "TRANSMIT_TLV",
};

static const char *get_gptp_capable_transmit_state_str(ptp_gptp_capable_transmit_sm_state_t state)
{
	if (state > GPTP_CAPABLE_TRANSMIT_SM_STATE_MAX_VALUE || state < GPTP_CAPABLE_TRANSMIT_SM_STATE_MIN_VALUE)
		return "Unknown gPTP capable transmit state";
	else
		return gptp_capable_transmit_state_str[state];
}

static const char *gptp_capable_interval_setting_state_str[] = {
	[GPTP_CAPABLE_INTERVAL_SM_STATE_NOT_ENABLED]= "NOT_ENABLED",
	[GPTP_CAPABLE_INTERVAL_SM_STATE_INITIALIZE]= "INITIALIZE",
	[GPTP_CAPABLE_INTERVAL_SM_STATE_SET_INTERVAL]= "SET_INTERVAL",
};

static const char *get_gptp_capable_interval_setting_state_str(ptp_gptp_capable_interval_setting_sm_state_t state)
{
	if (state > GPTP_CAPABLE_INTERVAL_SM_STATE_MAX_VALUE || state < GPTP_CAPABLE_INTERVAL_SM_STATE_MIN_VALUE)
		return "Unknown gPTP capable interval setting state";
	else
		return gptp_capable_interval_setting_state_str[state];
}

/*
* PortSyncSyncReceiveSM state machine and functions
*/


/* 10.2.7.2.1
setPSSyncPSSR: creates a PortSyncSync structure to be transmitted, and returns a pointer to this structure.
The members are set as follows:
a) localPortNumber is set equal to thisPort,
b) followUpCorrectionField, sourcePortIdentity, logMessageInterval, and preciseOriginTimestamp are
copied from the received MDSyncReceive structure,
c) upstreamTxTime is set equal to the upstreamTxTime member of the MDSyncReceive structure
pointed to by rcvdMDSyncPtr,
d) syncReceiptTimeoutTime is set equal to currentTime plus syncReceiptTimeoutTimeInterval (see
10.2.4.2), and
e) the function argument rateRatio is set equal to the local variable rateRatio (computed just prior to
invoking setPSSyncReceive (see Figure 10-4). The rateRatio member of the PortSyncSync structure
is then set equal to the function argument rateRatio.
*/
static struct port_sync_sync * sync_rcv_set_pssync_pssr(struct gptp_port *port, struct md_sync_receive *sync_rcv, struct ptp_u_scaled_ns syncReceiptTimeoutInterval, ptp_double rateRatio)
{
	struct ptp_port_sync_entity *port_sync = &port->port_sync;
	u64 timeout;

	port_sync->sync.localPortNumber = get_port_identity_number(port);

	os_memcpy(&port_sync->sync.syncReceiptTimeoutTime, &syncReceiptTimeoutInterval, sizeof(struct ptp_u_scaled_ns));
	u_scaled_ns_to_u64(&timeout, &port_sync->sync.syncReceiptTimeoutTime);

	os_log(LOG_DEBUG, "Port(%u): syncReceiptTimeoutTime %"PRIu64" ns\n", port->port_id, timeout);
	timer_restart(&port->sync_receive_timeout_timer, timeout/NS_PER_MS);

	os_memcpy(&port_sync->sync.followUpCorrectionField, &sync_rcv->followUpCorrectionField, sizeof(struct ptp_scaled_ns ));
	os_memcpy(&port_sync->sync.sourcePortIdentity, &sync_rcv->sourcePortIdentity, sizeof(struct ptp_port_identity));
	port_sync->sync.logMessageInterval = sync_rcv->logMessageInterval;
	os_memcpy(&port_sync->sync.preciseOriginTimestamp, &sync_rcv->preciseOriginTimestamp, sizeof(struct ptp_timestamp));
	os_memcpy(&port_sync->sync.upstreamTxTime, &sync_rcv->upstreamTxTime, sizeof(struct ptp_u_scaled_ns));
	port_sync->sync.rateRatio = rateRatio;

	/*IEEE 802.1AS-2020 sections 10.2.2.3.11, 10.2.2.3.12, 10.2.2.3.13*/
	port_sync->sync.gmTimeBaseIndicator = sync_rcv->gmTimeBaseIndicator;
	os_memcpy(&port_sync->sync.lastGmPhaseChange, &sync_rcv->lastGmPhaseChange, sizeof(struct ptp_scaled_ns));
	port_sync->sync.lastGmFreqChange = sync_rcv->lastGmFreqChange;

	return &port_sync->sync;
}


/* 10.2.7.2.2
txPSSyncPSSR: transmits a copy of the PortSyncSync structure pointed to by txPSSyncPtr to
the SiteSyncSync state machine of this time-aware system
*/
static void port_sync_sync_rcv_tx_pssync_pssr(struct gptp_port *port, struct port_sync_sync *sync)
{
	port->instance->site_sync.sync_sm.rcvdPSSync = true;
	port->instance->site_sync.sync_sm.rcvdPSSyncPtr = sync;
	site_sync_sync_sm(port->instance);
}


static void port_sync_sync_rcv_sm_discard(struct gptp_port *port)
{
	port->port_sync_sync_rcv_sm_state = PORT_SYNC_SYNC_RCV_SM_STATE_DISCARD;
}


static void port_sync_sync_rcv_sm_mdsync_received(struct gptp_port *port)
{
	struct ptp_port_sync_entity *port_sync = &port->port_sync;
	ptp_double sync_receipt_timeout_time_interval;

	port_sync->sync_receive_sm.rcvdMDSync = false;
	port_sync->sync_receive_sm.rateRatio = port_sync->sync_receive_sm.rcvdMDSyncPtr->rateRatio;
	port_sync->sync_receive_sm.rateRatio += (get_neighbor_rate_ratio(port) - 1.0);

	os_log(LOG_DEBUG, "Port(%u): rateRatio %1.16f\n", port->port_id, port_sync->sync_receive_sm.rateRatio);

	sync_receipt_timeout_time_interval = (ptp_double)((u64)port->sync_receipt_timeout *  log_to_ns(port_sync->sync_receive_sm.rcvdMDSyncPtr->logMessageInterval));
	ptp_double_to_u_scaled_ns(&port->params.sync_receipt_timeout_time_interval, sync_receipt_timeout_time_interval);
	port_sync->sync_receive_sm.txPSSyncPtr = sync_rcv_set_pssync_pssr (port, port_sync->sync_receive_sm.rcvdMDSyncPtr, port->params.sync_receipt_timeout_time_interval, port_sync->sync_receive_sm.rateRatio);
	if (port_sync->sync_receive_sm.txPSSyncPtr != NULL)
		port_sync_sync_rcv_tx_pssync_pssr(port, port_sync->sync_receive_sm.txPSSyncPtr);

	port->port_sync_sync_rcv_sm_state = PORT_SYNC_SYNC_RCV_SM_STATE_RECEIVED_MDSYNC;
}


/** PortSyncSyncReceiveSM state machine (10.2.7)
 * PortSyncSyncReceiveSM: The state machine receives time-synchronization information, accumulated rateRatio, and
 * syncReceiptTimeoutTime from the MD entity (MDSyncReceiveSM state machine) of the same port. The
 * state machine adds, to rateRatio, the fractional frequency offset of the LocalClock entity relative to the
 * LocalClock entity of the upstream time-aware system at the remote end of the link attached to this port. The
 * state machine computes syncReceiptTimeoutTime.[adjust] The state machine sends this information to the SiteSync
 * entity (SiteSyncSync state machine).
 * \return	0 on success or negative value on failure
 * \param port	pointer to the gptp port context used to retrieve port params and state machine variables
 */
int port_sync_sync_rcv_sm(struct gptp_port *port)
{
	ptp_port_sync_sync_rcv_sm_state_t state = port->port_sync_sync_rcv_sm_state;
	struct ptp_instance_params *params = &port->instance->params;
	bool port_oper = get_port_oper(port);
	int rc = 0;

	if ((params->begin) || (!params->instance_enable) ||
	(port->port_sync.sync_receive_sm.rcvdMDSync &&
	(!port_oper ||
	!port->params.ptp_port_enabled ||
	!port->params.as_capable))) {
		/* FIXME Where else should this go?
		   (This is not required by the spec but needed for hardware clock adjustment) */
		port->port_sync.sync_receive_sm.rateRatio = 1.0;

		port_sync_sync_rcv_sm_discard(port);
		goto exit;
	}

	switch (state) {
	case PORT_SYNC_SYNC_RCV_SM_STATE_DISCARD:
		if (port->port_sync.sync_receive_sm.rcvdMDSync &&
		port_oper &&
		port->params.ptp_port_enabled &&
		port->params.as_capable) {
			port_sync_sync_rcv_sm_mdsync_received(port);
		}

		break;

	case PORT_SYNC_SYNC_RCV_SM_STATE_RECEIVED_MDSYNC:
		/* Avnu AutoCDSFunctionalSpecs v1.1 - section 6.3
		*No verification of sourcePortIdentity. Because there are no Announce messages in the automotive
		*network, it is not possible for an AED-E slave to know the sourcePortIdentity of its link partner until the
		*first Sync message is received. Therefore an AED-E Slave shall not perform verification of the
		*sourcePortIdentity field of Sync and Followup messages.
		*/
		if (port->port_sync.sync_receive_sm.rcvdMDSync &&
		port_oper &&
		port->params.ptp_port_enabled &&
		port->params.as_capable) {
			if (port->gm_id_static)
				port_sync_sync_rcv_sm_mdsync_received(port);
			else
				if (!compare_clock_identity((struct ptp_clock_identity *)port->port_sync.sync_receive_sm.rcvdMDSyncPtr->sourcePortIdentity.clock_identity, (struct ptp_clock_identity *)params->gm_priority.u.s.source_port_identity.clock_identity))
					port_sync_sync_rcv_sm_mdsync_received(port);
		}
		break;
	default:
		break;
	}
exit:
	os_log(LOG_DEBUG, "Port(%u): state %s new state %s\n", port->port_id,
		port_sync_sync_rcv_sm_state_str[state],
		port_sync_sync_rcv_sm_state_str[port->port_sync_sync_rcv_sm_state]);

	return rc;
}


/* setMDSync (10.2.11.2.1)
 *
 */
static struct md_sync_send * set_md_sync(struct gptp_port *port)
{
	struct port_sync_sync_send_sm *sm = &port->port_sync.sync_send_sm;
	struct ptp_instance_md_entity *md = &port->md;

	os_memcpy(&md->sync_snd.sourcePortIdentity, get_port_identity(port), sizeof(struct ptp_port_identity)); /*802.1AS Cor1-2013 - 10.2.11.2.1*/
	md->sync_snd.logMessageInterval = port->params.current_log_sync_interval;
	md->sync_snd.preciseOriginTimestamp = sm->last_precise_origin_timestamp;
	md->sync_snd.rateRatio = sm->last_rate_ratio;
	md->sync_snd.followUpCorrectionField = sm->last_follow_up_correction_field;
	md->sync_snd.upstreamTxTime = sm->last_upstream_tx_time;
	md->sync_snd.gmTimeBaseIndicator = sm->last_gm_time_base_indicator;
	md->sync_snd.lastGmPhaseChange = sm->last_gm_phase_change;
	md->sync_snd.lastGmFreqChange = sm->last_gm_freq_change;

	/* Note in the specs. Added for AVnu test gPTP.com.c.14.2 (see md_set_follow_up) */
	md->sync_snd.lastRcvdPortNum = sm->last_rcvd_port_num;

	return &md->sync_snd;
}

/* txMDSync (10.2.11.2.2)
 *
 */
static void tx_md_sync(struct gptp_port *port)
{
	port->md.sync_send_sm.rcvdMDSync = true;

	md_sync_send_sm(port, SYNC_SND_SM_EVENT_RUN);
}

static void port_sync_sync_send_sm_transmit_init(struct gptp_port *port)
{
	struct port_sync_sync_send_sm *sm = &port->port_sync.sync_send_sm;

	sm->rcvd_pssync_psss = false;
	port->params.sync_slow_down = false;
	sm->number_sync_transmissions = 0;

	sm->state = PORT_SYNC_SYNC_SEND_SM_STATE_TRANSMIT_INIT;
}

static void port_sync_sync_send_sm_send_md_sync(struct gptp_port *port)
{
	struct port_sync_sync_send_sm *sm = &port->port_sync.sync_send_sm;
	struct ptp_instance_port_params *params = &port->params;
	u64 sync_receipt_timeout_time, interval1;
	u64 current_time = 0;

	os_clock_gettime64(gptp_port_to_clock(port), &current_time);

	if (sm->rcvd_pssync_psss) {
		sm->last_rcvd_port_num = sm->rcvd_pssync_ptr->localPortNumber;
		sm->last_precise_origin_timestamp = sm->rcvd_pssync_ptr->preciseOriginTimestamp;
		sm->last_follow_up_correction_field = sm->rcvd_pssync_ptr->followUpCorrectionField;
		sm->last_rate_ratio = sm->rcvd_pssync_ptr->rateRatio;
		sm->last_upstream_tx_time = sm->rcvd_pssync_ptr->upstreamTxTime;
		sm->last_gm_time_base_indicator = sm->rcvd_pssync_ptr->gmTimeBaseIndicator;
		sm->last_gm_phase_change = sm->rcvd_pssync_ptr->lastGmPhaseChange;
		sm->last_gm_freq_change = sm->rcvd_pssync_ptr->lastGmFreqChange;
		sm->sync_receipt_timeout_time = sm->rcvd_pssync_ptr->syncReceiptTimeoutTime;
		params->sync_locked = (port->instance->params.parent_log_sync_interval == params->current_log_sync_interval);

		/* start sync receipt timeout timer */
		u_scaled_ns_to_u64 (&sync_receipt_timeout_time, &sm->sync_receipt_timeout_time);
		timer_restart(&sm->sync_receipt_timeout_timer, sync_receipt_timeout_time / NS_PER_MS);
	}

	sm->rcvd_pssync_psss = false;

	u64_to_u_scaled_ns(&sm->last_sync_sent_time, current_time);
	sm->tx_md_sync_send_ptr = set_md_sync(port);
	tx_md_sync(port);

	if (params->sync_slow_down) {
		if (sm->number_sync_transmissions > port->sync_receipt_timeout) {
			sm->interval1 = params->sync_interval;
			sm->number_sync_transmissions = 0;
			port->params.sync_slow_down = false;
		} else {
			sm->interval1 = params->old_sync_interval;
			sm->number_sync_transmissions++;
		}
	} else {
		sm->number_sync_transmissions = 0;
		sm->interval1 = params->sync_interval;
	}

	/* start sync transmit interval timer */
	u_scaled_ns_to_u64(&interval1, &sm->interval1);
	timer_restart(&sm->sync_transmit_timer, interval1 / NS_PER_MS);

	sm->state = PORT_SYNC_SYNC_SEND_SM_STATE_SEND_MD_SYNC;
}

static void port_sync_sync_send_sm_sync_receipt_timeout(struct gptp_port *port)
{
	struct port_sync_sync_send_sm *sm = &port->port_sync.sync_send_sm;

	sm->rcvd_pssync_psss = false;

	sm->state = PORT_SYNC_SYNC_SEND_SM_STATE_SYNC_RECEIPT_TIMEOUT;
}


/** PortSyncSyncSend (10.2.12.3)
 * (one instance per port) receives time-synchronization information from the
 * SiteSync entity, requests that the MD entity of the corresponding port send a time-synchronization
 * event message, receives the <syncEventEgressTimestamp> for this event message from the MD
 * entity, uses the most recent time-synchronization information received from the SiteSync entity and
 * the timestamp to compute time-synchronization information that will be sent by the MD entity in a
 * general message (e.g., for full-duplex IEEE 802.3 media) or a subsequent event message (e.g., for
 * IEEE 802.11 media), and sends this latter information to the MD entity.
 *
 * The state machine receives time-synchronization information from the SiteSyncSync state
 * machine, corresponding to the receipt of the most recent synchronization information on either the slave
 * port, if this PTP Instance is not the Grandmaster PTP Instance, or from the ClockMasterSyncSend state
 * machine, if this PTP Instance is the Grandmaster PTP Instance. The state machine causes time-
 * synchronization information to be sent to the MD entity if this PTP Port is a MasterPort.
 *

 * \return	0 on success or negative value on failure
 * \param port	pointer to the gptp port context used to retrieve port params and state machine variables
 * \param event	value of the sync send event triggering this state machine
 */
int port_sync_sync_send_sm(struct gptp_port *port, ptp_port_sync_sync_send_sm_event_t event)
{
	struct port_sync_sync_send_sm *sm = &port->port_sync.sync_send_sm;
	ptp_port_sync_sync_send_sm_state_t state;
	struct ptp_instance_params *params = &port->instance->params;
	bool port_oper = get_port_oper(port);
	u16 this_port = get_port_identity_number(port);
	int rc = 0;

	if (params->begin || !params->instance_enable ||
	(sm->rcvd_pssync_psss && (!port_oper || !port->params.ptp_port_enabled || !port->params.as_capable))) {
		port_sync_sync_send_sm_transmit_init(port);
		goto exit;
	}

start:
	state = sm->state;

	switch (state) {
	case PORT_SYNC_SYNC_SEND_SM_STATE_TRANSMIT_INIT:
		if (sm->rcvd_pssync_psss && (sm->rcvd_pssync_ptr->localPortNumber != this_port) &&
		port_oper && port->params.ptp_port_enabled && port->params.as_capable &&
		(params->selected_role[this_port] == MASTER_PORT))
			port_sync_sync_send_sm_send_md_sync(port);

		break;

	case PORT_SYNC_SYNC_SEND_SM_STATE_SEND_MD_SYNC:
		if ((event == PORT_SYNC_SYNC_SEND_SM_EVENT_PSSYNC_TIMEOUT) && !port->params.sync_locked) {
			port_sync_sync_send_sm_sync_receipt_timeout(port);
		} else if (((sm->rcvd_pssync_psss && port->params.sync_locked && (sm->rcvd_pssync_ptr->localPortNumber != this_port)) ||
		(!port->params.sync_locked && !timer_is_running(&sm->sync_transmit_timer) && (sm->last_rcvd_port_num != this_port))) &&
		port_oper && port->params.ptp_port_enabled && port->params.as_capable && (params->selected_role[this_port] == MASTER_PORT)) {
			/* !timer_is_running() is being used instead of the (currentTime – lastSyncSentTime >= interval1) condition in the standard
			 * This works because:
			 *	- As long as the DUT is asCapable, the timer is running and transmits Syncs every interval1
			 *	- So we can't exceed this deadline while the timer is running
			 *	- The timer is restarted after the Syncs has been sent unless the DUT is no longer asCapable
			 *	- The first time the DUT becomes asCapable and when the first Sync is transmitted, the timer is started
			 */
			port_sync_sync_send_sm_send_md_sync(port);
		}

		break;

	case PORT_SYNC_SYNC_SEND_SM_STATE_SYNC_RECEIPT_TIMEOUT:
		if (sm->rcvd_pssync_psss && (sm->rcvd_pssync_ptr->localPortNumber != this_port) &&
		port_oper && port->params.ptp_port_enabled && port->params.as_capable
		&& (params->selected_role[this_port] == MASTER_PORT))
			port_sync_sync_send_sm_send_md_sync(port);

		break;

	default:
		break;
	}

	os_log(LOG_DEBUG, "Port(%u): domain(%u, %u) state %s event %s new state %s\n", port->port_id, port->instance->index, port->instance->domain.domain_number, port_sync_sync_send_sm_state_str[state], port_sync_sync_send_sm_event_str[event], port_sync_sync_send_sm_state_str[sm->state]);

	if (state != sm->state) {
		event = PORT_SYNC_SYNC_SEND_SM_EVENT_RUN;
		goto start;
	}

exit:
	return rc;
}

/*
* PortInformationSM state machine and functions
*/

/* recordOtherAnnounceInfo() - 10.3.11.2.2
* saves parameters of the received Announce message for this port in the per-port global variables
*/
static void record_other_announce_info(struct gptp_port *port)
{
	struct ptp_announce_pdu *announce = port->port_sync.announce_receive_sm.rcvd_announce_ptr;
	u16 flags = ntohs(announce->header.flags);

	port->params.ann_leap61 = flags & PTP_FLAG_LEAP_61;
	port->params.ann_leap59 = flags & PTP_FLAG_LEAP_59;
	port->params.ann_current_utc_offset_valid = flags & PTP_FLAG_CURRENT_UTC_OFF_VALID;
	port->params.ann_time_traceable = flags & PTP_FLAG_TIME_TRACEABLE;
	port->params.ann_frequency_traceable = flags & PTP_FLAG_FREQUENCY_TRACEABLE;
	port->params.ann_current_utc_offset = ntohs(announce->current_utc_offset);
	port->params.ann_time_source = announce->time_source;
}

static void gm_change(struct gptp_port *port)
{
	struct gptp_ctx *gptp = port->instance->gptp;
	u64 system_time;

	/* 14.3.6 - 14.3.9 */
	port->instance->gmChangeCount++;

	if (gptp_system_time(gptp, &system_time) < 0) {
		os_log(LOG_ERR, "gptp_system_time() failed\n");
		system_time = 0;
	}

	system_time /= 10000000; /* 0.01s */
	port->instance->timeOfLastGmChangeEvent = system_time;
	port->instance->timeOfLastGmPhaseChangeEvent = system_time;
	port->instance->timeOfLastGmFreqChangeEvent = system_time;
}

static void port_announce_info_sm_disabled(struct gptp_port *port)
{
	u64 now_ns = 0;

	os_log(LOG_DEBUG, "Port(%u)\n", port->port_id);

	port->params.rcvd_msg = false;
	os_clock_gettime64(gptp_port_to_clock(port), &now_ns);
	u64_to_u_scaled_ns(&port->port_sync.announce_information_sm.announceReceiptTimeoutTime, now_ns);
	port->params.info_is = SPANNING_TREE_DISABLED;
	port->instance->params.reselect |= (1 << get_port_identity_number(port));
	port->instance->params.selected &= ~(1 << get_port_identity_number(port));

	port->port_announce_info_sm_state = PORT_ANNOUNCE_INFO_SM_STATE_DISABLED;

	/* 10.3.12 - Figure 10.14 port role state machine triggered as soon as one port has its
	reselected variable set to TRUE */
	port_state_selection_sm(port->instance);
}

static void port_announce_info_sm_current(struct gptp_port *port)
{
	os_log(LOG_DEBUG, "Port(%u)\n", port->port_id);

	port->port_announce_info_sm_state = PORT_ANNOUNCE_INFO_SM_STATE_CURRENT;
}

static void port_announce_info_sm_superior(struct gptp_port *port)
{
	u64 announce_receipt_timeout_time_interval;

	os_log(LOG_DEBUG, "Port(%u)\n", port->port_id);

	/* Sending port is new master port */
	os_memcpy(&port->params.port_priority, &port->port_sync.announce_information_sm.messagePriority, sizeof(struct ptp_priority_vector));
	record_other_announce_info(port);

	/* 10.3.9.1 */
	announce_receipt_timeout_time_interval = (u64)port->announce_receipt_timeout * log_to_ns(port->port_sync.announce_receive_sm.rcvd_announce_ptr->header.log_msg_interval);
	u64_to_u_scaled_ns(&port->params.announce_receipt_timeout_time_interval, announce_receipt_timeout_time_interval);
	timer_restart(&port->port_sync.announce_receive_sm.timeout_timer, announce_receipt_timeout_time_interval / NS_PER_MS);

	port->params.info_is = SPANNING_TREE_RECEIVED;
	port->instance->params.reselect |= (1 << get_port_identity_number(port));
	port->instance->params.selected &= ~(1 << get_port_identity_number(port));
	port->params.rcvd_msg = false;
	port->port_sync.announce_receive_sm.rcvd_announce_ptr = NULL; //FIXME in the specs but looks dangerous...

	port->port_announce_info_sm_state = PORT_ANNOUNCE_INFO_SM_STATE_SUPERIOR_MASTER_PORT;

	gm_change(port);

	/* 10.3.12 - Figure 10.14 port role state machine triggered as soon as one port has its
	reselected variable set to TRUE */
	port_state_selection_sm(port->instance);
}

static void port_announce_info_sm_repeated(struct gptp_port *port)
{
	u64 announce_receipt_timeout_time_interval;

	os_log(LOG_DEBUG, "Port(%u)\n", port->port_id);

	/* Sending port is same master port */
	u_scaled_ns_to_u64(&announce_receipt_timeout_time_interval, &port->params.announce_receipt_timeout_time_interval);
	timer_restart(&port->port_sync.announce_receive_sm.timeout_timer, announce_receipt_timeout_time_interval / NS_PER_MS);

	port->params.rcvd_msg = false;
	port->port_sync.announce_receive_sm.rcvd_announce_ptr = NULL;

	port->port_announce_info_sm_state = PORT_ANNOUNCE_INFO_SM_STATE_REPEATED_MASTER_PORT;
}

static void port_announce_info_sm_inferior(struct gptp_port *port)
{
	os_log(LOG_DEBUG, "Port(%u)\n", port->port_id);

	port->params.rcvd_msg = false;
	port->port_sync.announce_receive_sm.rcvd_announce_ptr = NULL;

	port->port_announce_info_sm_state = PORT_ANNOUNCE_INFO_SM_STATE_INFERIOR_MASTER_OR_OTHER;

	gm_change(port);
}

static void port_announce_info_sm_update(struct gptp_port *port)
{
	os_log(LOG_DEBUG, "Port(%u)\n", port->port_id);

	os_memcpy(&port->params.port_priority, &port->params.master_priority, sizeof(struct ptp_priority_vector));
	port->params.port_steps_removed = port->instance->params.master_steps_removed;

	port->params.updt_info = false;
	port->params.info_is = SPANNING_TREE_MINE;
	port->params.new_info = true;

	port->port_announce_info_sm_state = PORT_ANNOUNCE_INFO_SM_STATE_UPDATE;

	port_announce_transmit_sm(port, PORT_ANNOUNCE_TRANSMIT_SM_EVENT_RUN);
}

static void port_announce_info_sm_aged(struct gptp_port *port)
{
	os_log(LOG_DEBUG, "Port(%u)\n", port->port_id);

	port->params.info_is = SPANNING_TREE_AGED;
	port->instance->params.reselect |= (1 << get_port_identity_number(port));
	port->instance->params.selected &= ~(1 << get_port_identity_number(port));

	port->port_announce_info_sm_state = PORT_ANNOUNCE_INFO_SM_STATE_AGED;

	/* 10.3.12 - Figure 10.14 port role state machine triggered as soon as one port has its
	reselected variable set to TRUE */
	port_state_selection_sm(port->instance);
}



/* rcvInfo - 10.3.11.2.1
 * decodes the messagePriorityVector (see 10.3.4 and 10.3.5) and stepsRemoved 10.5.3.2.6) field from the
 * Announce information pointed to by rcvdAnnouncePtr (see 10.3.9.11)
 */
static int rcv_info (struct gptp_port *port, struct ptp_announce_pdu *rcvdAnnouncePtr)
{
	int result;
	ptp_announce_priority_info_t info;

	struct port_announce_information_sm *port_info_sm = &port->port_sync.announce_information_sm;

	/* Stores the messagePriorityVector and stepsRemoved field value in messagePriority and
	messageStepsRemoved, respectively */
	copy_priority_vector_from_message(port, &port_info_sm->messagePriority, rcvdAnnouncePtr);
	port->params.message_steps_removed = ntohs(rcvdAnnouncePtr->steps_removed);

	/* compare received message (A) vector Vs local port vector (B)*/
	result = compare_msg_priority_vector(&port_info_sm->messagePriority, &port->params.port_priority);

	dump_priority_vector(&port_info_sm->messagePriority, port->instance->index, port->instance->domain.domain_number, "messagePriority", LOG_DEBUG);
	dump_priority_vector(&port->params.port_priority, port->instance->index, port->instance->domain.domain_number, "port_priority", LOG_DEBUG);

	switch (result) {
	case BMCA_VECTOR_A_BETTER:
		/* Returns SuperiorMasterInfo if the received message conveys the port role MasterPort, and the
		messagePriorityVector is superior to the portPriorityVector of the port */
		info = ANNOUNCE_PRIO_SUPERIOR_MASTER_INFO;
		break;

	case BMCA_VECTOR_A_B_SAME:
		/* Returns RepeatedMasterInfo if the received message conveys the port role MasterPort, and the
		messagePriorityVector is the same as the portPriorityVector of the port */
		info = ANNOUNCE_PRIO_REPEATED_MASTER_INFO;
		break;

	case BMCA_VECTOR_B_BETTER:
		/* Returns InferiorMasterInfo if the received message conveys the port role MasterPort, and the
		messagePriorityVector is worse than the portPriorityVector of the port */
		info = ANNOUNCE_PRIO_INFERIOR_MASTER_INFO;
		break;

	default:
		info = ANNOUNCE_PRIO_OTHER_INFO;
		break;
	}

	return info;
}

static void port_announce_info_sm_receive(struct gptp_port *port)
{
	port->port_sync.announce_information_sm.rcvdInfo = rcv_info(port, port->port_sync.announce_receive_sm.rcvd_announce_ptr);

	switch (port->port_sync.announce_information_sm.rcvdInfo) {
	case ANNOUNCE_PRIO_SUPERIOR_MASTER_INFO:
		port_announce_info_sm_superior(port);
		break;

	case ANNOUNCE_PRIO_REPEATED_MASTER_INFO:
		port_announce_info_sm_repeated(port);
		break;

	case ANNOUNCE_PRIO_INFERIOR_MASTER_INFO:
		port_announce_info_sm_inferior(port);
		break;

	default:
		break;
	}
}


/** PortAnnounceInformationSM state machine (10.3.11)
 * The state machine receives new qualified Announce information from the PortAnnounceReceive state machine (see 10.3.10) of the same
 * port and determines if the Announce information is better than the current best master information it knows about
 * \return	0 on success or negative value on failure
 * \param port	pointer to the gptp port context used to retrieve port params and state machine variables
 * \param event	value of the announce information event triggering this state machine
 */
int port_announce_info_sm(struct gptp_port *port, ptp_port_announce_info_sm_event_t event)
{
	ptp_port_announce_info_sm_state_t state;
	struct ptp_instance_params *params = &port->instance->params;
	bool port_oper = get_port_oper(port);
	u16 this_port = get_port_identity_number(port);
	int rc = 0;

	/*
	In automotive profile, GM node is statically defined
	*/
	if (port->gm_id_static)
		goto exit;

start:
	state = port->port_announce_info_sm_state;

	if ((params->begin) || (!params->instance_enable) ||
	((!port_oper || !port->params.ptp_port_enabled || !port->params.as_capable)
	 && (port->params.info_is != SPANNING_TREE_DISABLED))) {
		port_announce_info_sm_disabled(port);
		goto exit;
	}

	switch(state) {
	case PORT_ANNOUNCE_INFO_SM_STATE_DISABLED:
		if (port_oper && port->params.ptp_port_enabled && port->params.as_capable)
			port_announce_info_sm_aged(port);
		else if (port->params.rcvd_msg)
			port_announce_info_sm_disabled(port);
		break;

	case PORT_ANNOUNCE_INFO_SM_STATE_CURRENT:
		if (((event == PORT_ANNOUNCE_INFO_SM_EVENT_ANNOUNCE_TIMEOUT)
		|| ((event == PORT_ANNOUNCE_INFO_SM_EVENT_SYNC_TIMEOUT) && params->gm_present))
		&& (port->params.info_is == SPANNING_TREE_RECEIVED)
		&& !port->params.updt_info && !port->params.rcvd_msg) {
			port_announce_info_sm_aged(port);
		}
		else if (port->params.rcvd_msg && !port->params.updt_info) {
			port_announce_info_sm_receive(port);
		}
		else if ((port->instance->params.selected & (1 << this_port)) && (port->params.updt_info)) {
			port_announce_info_sm_update(port);
		}
		break;

	case PORT_ANNOUNCE_INFO_SM_STATE_AGED:
		if ((port->instance->params.selected & (1 << this_port)) && (port->params.updt_info)) {
			port_announce_info_sm_update(port);
		}
		break;

	case PORT_ANNOUNCE_INFO_SM_STATE_UPDATE:
	case PORT_ANNOUNCE_INFO_SM_STATE_SUPERIOR_MASTER_PORT:
	case PORT_ANNOUNCE_INFO_SM_STATE_REPEATED_MASTER_PORT:
	case PORT_ANNOUNCE_INFO_SM_STATE_INFERIOR_MASTER_OR_OTHER:
		/* Unconditional Transferts */
		port_announce_info_sm_current(port);
		break;

	default:
		break;
	}

	os_log(LOG_DEBUG, "Port(%u): domain(%u, %u) state %s event %s new state %s\n", port->port_id, port->instance->index, port->instance->domain.domain_number, port_announce_info_sm_state_str[state], port_announce_info_sm_event_str[event], port_announce_info_sm_state_str[port->port_announce_info_sm_state]);

	if (state != port->port_announce_info_sm_state) {
		event = PORT_ANNOUNCE_INFO_SM_EVENT_RUN;
		goto start;
	}

exit:
	return rc;
}



/*
* PortAnnounceSM state machine and functions
*/


/*
 * qualifies the received Announce message pointed to by rcvdAnnouncePtr (IEEE 802.1AS-2020 - 10.3.11.2.1)
 */
static bool qualify_announce (struct gptp_port *port, struct ptp_announce_pdu *announce)
{
	bool ptlv_present = false;
	u8 num_ptlv_entries = 0;
	u16 tlv_len;
	struct ptp_clock_identity *clk_id;
	int i;

	/*
	If clockIdentity is equal to thisClock, the Announce message is not qualified
	*/
	if(!os_memcmp(&announce->header.source_port_id.clock_identity[0], get_port_identity_clock_id(port), sizeof(struct ptp_clock_identity))) {
		os_log(LOG_DEBUG, "Port(%u): announce: peer %"PRIx64" thisClock %"PRIx64"\n", port->port_id, get_64(announce->grandmaster_identity.identity), get_64(get_port_identity_clock_id(port)));
		goto not_qualified;
	}

	/*
	If the stepsRemoved field is greater than or equal to 255, the Announce message is not qualified
	*/
	if(ntohs(announce->steps_removed) >= 255) {
		os_log(LOG_DEBUG, "Port(%u): announce: steps removed %u\n", port->port_id, ntohs(announce->steps_removed));
		goto not_qualified;
	}

	/*
	If a path trace TLV is present and one of the elements of the pathSequence array field of the path
	trace TLV is equal to thisClock (i.e., the clockIdentity of the current time-aware system), the Announce
	message is not qualified
	*/
	if(ntohs(announce->ptlv.header.tlv_type) == PTP_TLV_TYPE_PATH_TRACE) {
		ptlv_present = true;
		tlv_len = ntohs(announce->ptlv.header.length_field);
		clk_id = &announce->ptlv.path_sequence[0];

		/* check the tlv length is multiple of 8 bytes i.e size of struct ptp_clock_identity */
		if (tlv_len % 8)
			goto not_qualified;

		/* making sure we have enough room to store all clock_identity from the message tlv plus our */
		if (tlv_len > ((MAX_PTLV_ENTRIES - 1) * sizeof(struct ptp_clock_identity))) {
			os_log(LOG_ERR, "Port(%u): announce: tlv_len %d too big\n", port->port_id, tlv_len);
			tlv_len = (MAX_PTLV_ENTRIES - 1) * sizeof(struct ptp_clock_identity);
		}

		os_log(LOG_DEBUG, "Port(%u): announce: tlv_len %u first ptlv %"PRIx64" thisClock %"PRIx64"\n", port->port_id, tlv_len, get_64(clk_id), get_64(get_port_identity_clock_id(port)));
		while (tlv_len) {
			if(!os_memcmp(clk_id, get_port_identity_clock_id(port), sizeof(struct ptp_clock_identity))) {
				os_log(LOG_DEBUG, "Port(%u): announce: ptlv %"PRIx64" thisClock %"PRIx64"\n", port->port_id, get_64(clk_id), get_64(get_port_identity_clock_id(port)));
				goto not_qualified;
			}
			clk_id++;
			tlv_len -= sizeof(struct ptp_clock_identity);
		}
	}

	/*
	The Announce message is qualified. If a path trace TLV is present and the portRole of the port is SlavePort,
	the pathSequence array field of the TLV is copied to the global array pathTrace, and thisClock is appended to pathTrace
	*/
	if ((port->instance->params.selected_role[get_port_identity_number(port)] == SLAVE_PORT) && ptlv_present) {
		tlv_len = ntohs(announce->ptlv.header.length_field);
		clk_id = &announce->ptlv.path_sequence[0];

		/* making sure we have enough room to store all clock_identity from the message tlv plus our */
		if (tlv_len > ((MAX_PTLV_ENTRIES - 1) * sizeof(struct ptp_clock_identity))) {
			os_log(LOG_ERR, "Port(%u): announce: tlv_len %d too big\n", port->port_id, tlv_len);
			tlv_len = (MAX_PTLV_ENTRIES - 1) * sizeof(struct ptp_clock_identity);
		}

		while (tlv_len) {
			os_memcpy(&port->instance->params.path_trace[num_ptlv_entries], clk_id, sizeof(struct ptp_clock_identity));
			num_ptlv_entries++;
			clk_id++;
			tlv_len -= sizeof(struct ptp_clock_identity);
		}

		os_memcpy(&port->instance->params.path_trace[num_ptlv_entries], get_port_identity_clock_id(port), sizeof(struct ptp_clock_identity));
		port->instance->params.num_ptlv = num_ptlv_entries + 1;

		gptp_ipc_gm_status(port->instance, &port->instance->gptp->ipc_tx, IPC_DST_ALL);
	}

	for (i = 0; i < num_ptlv_entries; i++)
		os_log(LOG_DEBUG, "Port(%u): ptlv%d %"PRIx64"\n", port->port_id, i, get_64(&port->instance->params.path_trace[i]));

	return true;

not_qualified:
	return false;
}



static void port_announce_rcv_sm_discard(struct gptp_port *port)
{
	struct port_announce_receive_sm *sm = &port->port_sync.announce_receive_sm;

	sm->rcvd_announce_par = false;

	port->params.rcvd_msg = false;

	sm->state = PORT_ANNOUNCE_RCV_SM_STATE_DISCARD;
}


static void port_announce_rcv_sm_receive(struct gptp_port *port)
{
	struct port_announce_receive_sm *sm = &port->port_sync.announce_receive_sm;

	sm->rcvd_announce_par = false;

	port->params.rcvd_msg = qualify_announce(port, sm->rcvd_announce_ptr);

	os_log(LOG_DEBUG, "Port(%u): Announce message is %s\n", port->port_id, port->params.rcvd_msg? "qualified":"not qualified");

	sm->state = PORT_ANNOUNCE_RCV_SM_STATE_RECEIVE;

	port_announce_info_sm(port, PORT_ANNOUNCE_INFO_SM_EVENT_RUN);
}


/** PortAnnounceReceiveSM state machine (10.3.7)
 * The state machine receives Announce information from the MD entity of the same port, determines if the Announce
 * message is qualified, and if so, sets the rcvdMsg variable
 * \return	0 on success or negative value on failure
 * \param port	pointer to the gptp port context used to retrieve port params and state machine variables
  */
int port_announce_rcv_sm(struct gptp_port *port)
{
	struct port_announce_receive_sm *sm = &port->port_sync.announce_receive_sm;
	struct ptp_instance_params *params = &port->instance->params;
	bool port_oper = get_port_oper(port);
	ptp_port_announce_rcv_sm_state_t state;
	int rc = 0;

	/*
	In automotive profile, GM node is statically defined
	*/
	if (port->gm_id_static)
		goto exit;

	if ((params->begin || !params->instance_enable ||
	(sm->rcvd_announce_par &&
	(!port_oper ||
	!port->params.ptp_port_enabled ||
	!port->params.as_capable))) &&
	!params->external_port_configuration_enabled) {
		port_announce_rcv_sm_discard(port);
		goto exit;
	}

start:
	state = sm->state;

	switch (state) {
	case PORT_ANNOUNCE_RCV_SM_STATE_DISCARD:
		if (sm->rcvd_announce_par &&
		port_oper &&
		port->params.ptp_port_enabled &&
		port->params.as_capable)
			port_announce_rcv_sm_receive(port);
		break;

	case PORT_ANNOUNCE_RCV_SM_STATE_RECEIVE:
		if (sm->rcvd_announce_par &&
		port_oper &&
		port->params.ptp_port_enabled &&
		port->params.as_capable &&
		!port->params.rcvd_msg)
			port_announce_rcv_sm_receive(port);
		break;

	default:
		break;
	}

	os_log(LOG_DEBUG, "Port(%u): domain(%u, %u) state %s new state %s\n", port->port_id,
		port->instance->index, port->instance->domain.domain_number,
		port_announce_rcv_sm_state_str[state],
		port_announce_rcv_sm_state_str[sm->state]);

	if (state != sm->state)
		goto start;

exit:
	return rc;
}


/* txAnnounce - 10.3.13.2.1
 * transmits Announce information to the MD entity of this port
 */
static void tx_announce(struct gptp_port *port)
{
	/*
	a) The components of the messagePriorityVector are set to the values of the respective components of
	the masterPriorityVector of this port.
	*/

	/*
	b) The grandmasterIdentity,  grandmasterClockQuality, grandmasterPriority1, and grandmasterPriority2 fields
	of the Announce message are set equal to the corresponding components of the messagePriorityVector.
	*/

	/*
	c) The value of the stepsRemoved field of the Announce message is set equal to masterStepsRemoved.
	*/

	/*
	d) The Announce message flags leap61, leap59, currentUtcOffsetValid, timeTraceable, and
	frequencyTraceable, and the Announce message fields currentUtcOffset and timeSource, are set
	equal to the values of the global variables leap61, leap59, currentUtcOffsetValid, timeTraceable,
	frequencyTraceable, currentUtcOffset, and timeSource, respectively (see 10.3.8.4 through
	10.3.8.10).
	*/

	/*
	e) The sequenceId field of the Announce message is set in accordance with 10.4.7.
	*/

	/*
	f) A path trace TLV (see 10.5.3.3) is constructed, with its pathSequence field (see 10.5.3.3.4) set equal
	to the pathTrace array (see 10.3.8.21). If appending the path trace TLV to the Announce message
	does not cause the media-dependent layer frame to exceed any respective maximum size, the path
	trace TLV is appended to the Announce message; otherwise, it is not appended.
	*/

	md_announce_transmit(port);
}

static void port_announce_transmit_sm_transmit_init(struct gptp_port *port)
{
	struct port_announce_transmit_sm *sm = &port->announce_transmit_sm;

	port->params.new_info = true;

	port->params.announce_slow_down = false;

	sm->number_announce_transmissions = 0;

	sm->state = PORT_ANNOUNCE_TRANSMIT_SM_STATE_TRANSMIT_INIT;
}

static void port_announce_transmit_sm_idle(struct gptp_port *port)
{
	struct port_announce_transmit_sm *sm = &port->announce_transmit_sm;
	u64 transmit_interval_ns;

	u_scaled_ns_to_u64(&transmit_interval_ns, &sm->interval2);
	timer_restart(&sm->transmit_timer, transmit_interval_ns/NSECS_PER_MS);

	sm->state = PORT_ANNOUNCE_TRANSMIT_SM_STATE_IDLE;

	os_log(LOG_DEBUG, "Port(%u): announce_interval %"PRIu64" ms\n", port->port_id, transmit_interval_ns/NSECS_PER_MS);
}

static void port_announce_transmit_sm_transmit_periodic(struct gptp_port *port)
{

	struct port_announce_transmit_sm *sm = &port->announce_transmit_sm;

	port->params.new_info = (port->params.new_info) || (port->instance->params.selected_role[get_port_identity_number(port)] == MASTER_PORT);

	sm->state = PORT_ANNOUNCE_TRANSMIT_SM_STATE_TRANSMIT_PERIODIC;
}

static void port_announce_transmit_sm_transmit_announce(struct gptp_port *port)
{
	struct port_announce_transmit_sm *sm = &port->announce_transmit_sm;
	struct ptp_instance_port_params *params = &port->params;

	params->new_info = false;

	tx_announce(port);

	if (params->announce_slow_down) {
		if (sm->number_announce_transmissions >= port->announce_receipt_timeout) {
			sm->interval2 = params->announce_interval;
			sm->number_announce_transmissions = 0;
			params->announce_slow_down = false;
		} else {
			sm->interval2 = params->old_announce_interval;
			sm->number_announce_transmissions++;
		}
	} else {
		sm->number_announce_transmissions = 0;
		sm->interval2 = params->announce_interval;
	}

	sm->state = PORT_ANNOUNCE_TRANSMIT_SM_STATE_TRANSMIT_ANNOUNCE;
}


/** PortAnnounceTransmit state machine (IEEE 802.1AS-2020 - 10.3.16)
 * (one instance per port) transmits Announce information to the MD entity
 * when an announce interval has elapsed, port roles have been updated, and portPriority and
 * portStepsRemoved information has been updated with newly determined masterPriority and
 * masterStepsRemoved information. This state machine is invoked by the PortSync entity of the port
 * \return	0 on success or negative value on failure
 * \param port	pointer to the gptp port context used to retrieve port params and state machine variables
 * \param event	value of the announce transmit event triggering this state machine
 */
int port_announce_transmit_sm(struct gptp_port *port, ptp_port_announce_transmit_sm_event_t event)
{
	struct port_announce_transmit_sm *sm = &port->announce_transmit_sm;
	struct ptp_instance_params *params = &port->instance->params;
	ptp_port_announce_transmit_sm_state_t state;
	u16 this_port =get_port_identity_number(port);
	int rc = 0;

	/*
	* In automotive profile, GM node is statically defined
	*/
	if (port->gm_id_static)
		goto exit;

	/*
	* 802.1AS Annexe D - the special condition BEGIN supercedes all other global conditions
	*/
	if (params->begin || !params->instance_enable)
		port_announce_transmit_sm_transmit_init(port);

start:
	state = sm->state;

	switch(state) {
	case PORT_ANNOUNCE_TRANSMIT_SM_STATE_IDLE:
		if ((event == PORT_ANNOUNCE_TRANSMIT_SM_EVENT_TRANSMIT_INTERVAL) &&
			(((port->instance->params.selected & (1 << this_port)) && !port->params.updt_info) ||
			params->external_port_configuration_enabled)) {
			port_announce_transmit_sm_transmit_periodic(port);
		} else if (port->params.new_info &&
			(port->instance->params.selected_role[this_port] == MASTER_PORT) &&
			(((port->instance->params.selected & (1 << this_port)) && !port->params.updt_info) ||
			params->external_port_configuration_enabled) &&
			!get_port_asymmetry(port)) {
			port_announce_transmit_sm_transmit_announce(port);
		}
		break;

	case PORT_ANNOUNCE_TRANSMIT_SM_STATE_TRANSMIT_INIT:
	case PORT_ANNOUNCE_TRANSMIT_SM_STATE_TRANSMIT_ANNOUNCE:
	case PORT_ANNOUNCE_TRANSMIT_SM_STATE_TRANSMIT_PERIODIC:
		/* Unconditional Transferts */
		port_announce_transmit_sm_idle(port);
		break;

	default:
		break;
	}

	os_log(LOG_DEBUG, "Port(%u): domain(%u, %u) state %s event %s new state %s\n", port->port_id, port->instance->index, port->instance->domain.domain_number, port_announce_transmit_sm_state_str[state], port_announce_transmit_sm_event_str[event], port_announce_transmit_sm_state_str[sm->state]);

	/*
	* 802.1AS Annexe D - completion of each state block is followed by reentry to that state, until
	* all state blocks have executed to the point that variable assignements and other consequences
	* of their execution remain unchanged
	*/
	if (state != sm->state) {
		event = PORT_ANNOUNCE_TRANSMIT_SM_EVENT_RUN;
		goto start;
	}

exit:
	return rc;
}



/** 10.3.17.2.1 isSupportedLogAnnounceInterval (logAnnounceInterval): A Boolean function that returns TRUE if the
announce interval given by the argument logAnnounceInterval is supported by the PTP Port and FALSE if the sync
interval is not supported by the PTP Port. The argument logAnnounceInterval has the same data type and format
as the field announceInterval of the message interval request TLV (see 10.6.4.3.7).
*/
static bool is_supported_log_announce_interval(struct port_announce_interval_sm *sm, s8 log_announce_interval)
{
	if ((log_announce_interval < sm->log_supported_closest_longer_announce_interval) || (log_announce_interval > sm->log_supported_announce_interval_max))
		return false;
	else
		return true;
}

/** 10.3.17.2.2 computeLogAnnounceInterval (logRequestedAnnounceInterval): An Integer8 function that computes
and returns the logAnnounceInterval, based on the logRequestedAnnounceInterval.
*/
static s8 compute_log_announce_interval(struct port_announce_interval_sm *sm, s8 log_requested_announce_interval)
{
	if (is_supported_log_announce_interval (sm, log_requested_announce_interval)) {
		/* The requested Announce Interval is supported and returned */
		return log_requested_announce_interval;
	} else {
		if (log_requested_announce_interval > sm->log_supported_announce_interval_max) {
			/* Return the fastest supported rate, even if faster than the requested rate */
			return sm->log_supported_announce_interval_max;
		} else {
			/* Return the fastest supported rate that is still slower than the requested rate. */
			return sm->log_supported_closest_longer_announce_interval;
		}
	}
}

static void port_announce_interval_setting_sm_not_enabled(struct gptp_port *port)
{
	struct port_announce_interval_sm *sm = &port->announce_interval_sm;
	struct ptp_instance_port_params *params = &port->params;
	u64 log_announce_interval;

	if (port->use_mgt_settable_log_announce_interval) {
		log_announce_interval = log_to_ns(port->mgt_settable_log_announce_interval);
		u64_to_u_scaled_ns(&params->announce_interval, log_announce_interval);
		params->current_log_announce_interval = port->mgt_settable_log_announce_interval;
	}

	sm->state = PORT_ANNOUNCE_INTERVAL_SM_STATE_NOT_ENABLED;
}

static void port_announce_interval_setting_sm_initialize(struct gptp_port *port)
{
	struct port_announce_interval_sm *sm = &port->announce_interval_sm;
	struct ptp_instance_port_params *params = &port->params;
	u64 announce_interval;

	params->current_log_announce_interval = params->initial_log_announce_interval;

	announce_interval = log_to_ns(params->current_log_announce_interval);
	u64_to_u_scaled_ns(&params->announce_interval, announce_interval);
	u64_to_u_scaled_ns(&params->old_announce_interval, announce_interval);

	os_log(LOG_INFO, "Port(%u) domain(%u, %u): initial_log_announce_interval %d (%"PRIu64" ms)\n", port->port_id, port->instance->index,
		port->instance->domain.domain_number,
		params->initial_log_announce_interval,
		announce_interval / NS_PER_MS);

	sm->rcvd_signaling_msg2 = false;
	params->announce_slow_down = false;

	sm->state = PORT_ANNOUNCE_INTERVAL_SM_STATE_INITIALIZE;
}


/** 802.1AS-rev-d1-0 - 10.6.2.2
* Every port supports the value 127; the port does not send Announce messages when
* currentLogAnnounceInterval has this value (see 10.3.14). A port may support other values, except for the
* reserved values -128 through -125, inclusive, and 124 through 126, inclusive. A port ignores requests (see
* 10.3.14) for unsupported values.
*/
static void port_announce_interval_setting_sm_set_interval(struct gptp_port *port)
{
	struct port_announce_interval_sm *sm = &port->announce_interval_sm;
	struct ptp_instance_port_params *params = &port->params;
	s8 computed_log_time_announce_interval;
	u64 old_announce_interval;
	u64 announce_interval = 0;

	/*
	AutoCDSFunctionalSpecs-1_4 - 6.2.4
	When processing a received gPTP signaling Message on a master role port, and AED shall update the syncInterval
	for the receiving port accordingly and shall ignore values for linkDelayInterval and announceInterval
	*/
	if ((port->instance->gptp->cfg.profile == CFG_GPTP_PROFILE_AUTOMOTIVE) && (port->instance->params.selected_role[get_port_identity_number(port)] == MASTER_PORT))
		goto ignore_announce_interval;

	if (!port->use_mgt_settable_log_announce_interval) {
		switch (sm->rcvd_signaling_ptr_ais->u.itlv.announce_interval)
		{
		case (-128): /* dont change the interval */
			announce_interval = log_to_ns(port->params.current_log_announce_interval);

			break;

		case 126: /* set interval to initial value */
			/* FIXME: timer restart should be handled directly by the portAnnounceTransmit state machine
			based on the announceSlowDown state (802.1AS-2020 - section 10.3.10.2) */
			announce_interval = log_to_ns(port->params.initial_log_announce_interval);
			u64_to_u_scaled_ns(&port->params.announce_interval, announce_interval);
			timer_restart(&port->announce_transmit_sm.transmit_timer, announce_interval / NS_PER_MS);
			port->params.current_log_announce_interval = port->params.initial_log_announce_interval;

			break;

		default: /* use indicated value; note that the value of 127 will result in an interval of
			* 2 127 s, or approximately 5.4 u 10 30 years, which indicates that the sender
			* should stop sending for all practical purposes, in accordance
			* with Table 10-10. */
			if (sm->rcvd_signaling_ptr_ais->u.itlv.announce_interval == 127) {
				if (timer_is_running(&port->announce_transmit_sm.transmit_timer))
					timer_stop(&port->announce_transmit_sm.transmit_timer);
				port->params.current_log_announce_interval = sm->rcvd_signaling_ptr_ais->u.itlv.announce_interval;
			} else {
				/* supported values, apply new interval */
				computed_log_time_announce_interval = compute_log_announce_interval (sm, sm->rcvd_signaling_ptr_ais->u.itlv.announce_interval);
				announce_interval = log_to_ns(computed_log_time_announce_interval);
				u64_to_u_scaled_ns(&port->params.announce_interval, announce_interval);
				timer_restart(&port->announce_transmit_sm.transmit_timer, announce_interval / NS_PER_MS);
				port->params.current_log_announce_interval = computed_log_time_announce_interval;
			}

			break;
		}

		u_scaled_ns_to_u64(&announce_interval, &params->announce_interval);
		u_scaled_ns_to_u64(&old_announce_interval, &params->old_announce_interval);
		if (announce_interval > old_announce_interval)
			params->announce_slow_down = true;
		else
			params->announce_slow_down = false;

		os_log(LOG_DEBUG, "Port(%u): announce_interval %"PRIu64" ms (log %d)\n", port->port_id, announce_interval / NS_PER_MS, port->params.current_log_announce_interval);
	}

ignore_announce_interval:
	sm->rcvd_signaling_msg2 = false;

	sm->state = PORT_ANNOUNCE_INTERVAL_SM_STATE_SET_INTERVAL;
}


/** AnnounceIntervalSetting state machine (10.3.14.2)
 * This state machine is responsible for setting the global variables that give the duration of the mean interval between successive
 * Announce messages, both at initialization and in response to the receipt of a Signaling message that contains a Message Interval Request TLV
 * \return	0 on success or negative value on failure
 * \param port	pointer to the gptp port context used to retrieve port params and state machine variables
 * \param event	value of the announce interval signaling event triggering this state machine
 */
void port_announce_interval_setting_sm(struct gptp_port *port)
{
	struct port_announce_interval_sm *sm = &port->announce_interval_sm;
	ptp_port_announce_interval_sm_state_t state = sm->state;
	struct ptp_instance_params *params = &port->instance->params;
	bool port_oper = get_port_oper(port);

	if ((params->begin) || (!params->instance_enable) ||
	(!port_oper) ||
	(!port->params.ptp_port_enabled) ||
	(port->use_mgt_settable_log_announce_interval)){
		port_announce_interval_setting_sm_not_enabled(port);
		goto exit;
	}

	switch (state) {
	case PORT_ANNOUNCE_INTERVAL_SM_STATE_NOT_ENABLED:
		if (port_oper && port->params.ptp_port_enabled && (!port->use_mgt_settable_log_announce_interval))
			port_announce_interval_setting_sm_initialize(port);

		break;

	case PORT_ANNOUNCE_INTERVAL_SM_STATE_INITIALIZE:
	case PORT_ANNOUNCE_INTERVAL_SM_STATE_SET_INTERVAL:
		if (sm->rcvd_signaling_msg2)
			port_announce_interval_setting_sm_set_interval(port);

		break;

	default:
		break;
	}

exit:
	os_log(LOG_DEBUG, "Port(%u): domain(%u, %u) state %s new state %s\n", port->port_id, port->instance->index, port->instance->domain.domain_number, port_announce_interval_state_str[state], port_announce_interval_state_str[sm->state]);
}


/** 10.3.18.2.1 isSupportedLogSyncInterval (logSyncInterval): A Boolean function that returns TRUE if the
sync interval given by the argument logSyncInterval is supported by the PTP Port and FALSE if the sync
interval is not supported by the PTP Port. The argument logSyncInterval has the same data type and format
as the field syncInterval of the message interval request TLV (see 10.6.4.3.7).
*/
static bool is_supported_log_sync_interval(struct ptp_sync_interval_setting_sm *sm, s8 log_sync_interval)
{
	if ((log_sync_interval < sm->log_supported_closest_longer_sync_interval) || (log_sync_interval > sm->log_supported_sync_interval_max))
		return false;
	else
		return true;
}

/** 10.3.18.2.2 computeLogSyncInterval (logRequestedSyncInterval): An Integer8 function that computes
and returns the logSyncInterval, based on the logRequestedSyncInterval.
*/
static s8 compute_log_sync_interval(struct ptp_sync_interval_setting_sm *sm, s8 log_sync_interval)
{
	if (is_supported_log_sync_interval (sm, log_sync_interval)) {
		/* The requested Sync Interval is supported and returned */
		return log_sync_interval;
	} else {
		if (log_sync_interval > sm->log_supported_sync_interval_max) {
			/* Return the fastest supported rate, even if faster than the requested rate */
			return sm->log_supported_sync_interval_max;
		} else {
			/* Return the fastest supported rate that is still slower than the requested rate. */
			return sm->log_supported_closest_longer_sync_interval;
		}
	}
}

static void port_sync_interval_setting_sm_not_enabled(struct gptp_port *port)
{
	struct ptp_sync_interval_setting_sm *sm = &port->sync_interval_sm;
	struct ptp_instance_port_params *params = &port->params;
	u64 sync_interval;

	if (port->use_mgt_settable_log_sync_interval) {
		sync_interval = log_to_ns(port->mgt_settable_log_sync_interval);
		u64_to_u_scaled_ns(&params->sync_interval, sync_interval);
		params->current_log_sync_interval = port->mgt_settable_log_sync_interval;
	}

	sm->state = SYNC_INTERVAL_SETTING_SM_STATE_NOT_ENABLED;
}

static void port_sync_interval_setting_sm_initialize(struct gptp_port *port)
{
	struct ptp_sync_interval_setting_sm *sm = &port->sync_interval_sm;
	struct ptp_instance_port_params *params = &port->params;
	u64 sync_interval;

	params->current_log_sync_interval = params->initial_log_sync_interval;

	sync_interval = log_to_ns(params->current_log_sync_interval);
	u64_to_u_scaled_ns(&params->sync_interval, sync_interval);
	params->old_sync_interval = params->sync_interval;

	os_log(LOG_INFO, "Port(%u): initialLogSyncInterval %d (%"PRIu64" ms)\n",
		port->port_id, params->initial_log_sync_interval, sync_interval / NS_PER_MS);

	sm->rcvd_signaling_msg3 = false;
	params->sync_slow_down = false;

	sm->state = SYNC_INTERVAL_SETTING_SM_STATE_INITIALIZE;
}

static void port_sync_interval_setting_sm_set_intervals(struct gptp_port *port)
{
	struct ptp_sync_interval_setting_sm *sm = &port->sync_interval_sm;
	struct ptp_instance_port_params *params = &port->params;
	s8 computed_log_time_sync_interval;
	u64 old_sync_interval;
	u64 sync_interval = 0;

	if (!port->use_mgt_settable_log_sync_interval) {
		switch (sm->rcvd_signaling_ptr_sis->u.itlv.time_sync_interval) {
		case (-128): /* do not change the interval */
			sync_interval = log_to_ns(params->current_log_sync_interval);

			break;

		case 126: /* set interval to initial value */
			/* FIXME: timer restart should be handled directly by the portSyncSyncSend state machine base
			on the syncSlowDown state (802.1AS-2020 section 10.2.5.17)*/
			sync_interval = log_to_ns(params->initial_log_sync_interval);
			u64_to_u_scaled_ns(&params->sync_interval, sync_interval);
			timer_restart(&port->port_sync.sync_send_sm.sync_transmit_timer, sync_interval / NS_PER_MS);
			params->current_log_sync_interval = params->initial_log_sync_interval;

			break;

		default: /* use indicated value; note that the value of 127 instructs the receiving
			port to stop sending, in accordance with Table 10-14. */
			if (sm->rcvd_signaling_ptr_sis->u.itlv.time_sync_interval == 127) {
				if (timer_is_running(&port->port_sync.sync_send_sm.sync_transmit_timer))
					timer_stop(&port->port_sync.sync_send_sm.sync_transmit_timer);

				params->current_log_sync_interval = sm->rcvd_signaling_ptr_sis->u.itlv.time_sync_interval;
			} else {
				/* supported values, apply new interval */
				computed_log_time_sync_interval = compute_log_sync_interval (sm, sm->rcvd_signaling_ptr_sis->u.itlv.time_sync_interval);
				sync_interval = log_to_ns(computed_log_time_sync_interval);
				u64_to_u_scaled_ns(&params->sync_interval, sync_interval);
				timer_restart(&port->port_sync.sync_send_sm.sync_transmit_timer, sync_interval / NS_PER_MS);
				params->current_log_sync_interval = computed_log_time_sync_interval;
			}

			break;
		}

		u_scaled_ns_to_u64(&sync_interval, &params->sync_interval);
		u_scaled_ns_to_u64(&old_sync_interval, &params->old_sync_interval);
		if (sync_interval > old_sync_interval)
			params->sync_slow_down = true;
		else
			params->sync_slow_down = false;

		os_log(LOG_DEBUG, "Port(%u): sync_interval %"PRIu64" ms (log %d)\n", port->port_id, sync_interval / NS_PER_MS, params->current_log_sync_interval);
	}

	/* FIXME move to ClockMasterSyncSend computeClockMasterSyncInterval() */
	/* 802.1AS - 10.6.2.4 - may change to lowest per port current_log_sync_interval */
	if (port->instance->clock_master_log_sync_interval > params->current_log_sync_interval)
		port->instance->clock_master_log_sync_interval = params->current_log_sync_interval;

	sm->rcvd_signaling_msg3 = true;

	sm->state = SYNC_INTERVAL_SETTING_SM_STATE_INITIALIZE;
}


/** SyncIntervalSetting state machine (802.1AS-2020 section 10.3.18)
 * This state machine is responsible for setting the global variables that give the duration of the mean intervals
 * between successive Sync messages, both at initialization and in response to the receipt of a Signaling message
 * that contains a Message Interval Request TLV
*/
void port_sync_interval_setting_sm(struct gptp_port *port)
{
	struct ptp_sync_interval_setting_sm *sm = &port->sync_interval_sm;
	ptp_sync_interval_setting_sm_state_t state = sm->state;
	struct ptp_instance_params *params = &port->instance->params;
	bool port_oper = get_port_oper(port);

	if ((params->begin) || (!params->instance_enable) ||
	(!port_oper) ||
	(!port->params.ptp_port_enabled) ||
	(port->use_mgt_settable_log_sync_interval)) {
		port_sync_interval_setting_sm_not_enabled(port);
		goto exit;
	}

	switch (state) {
	case SYNC_INTERVAL_SETTING_SM_STATE_NOT_ENABLED:
		if (port_oper && port->params.ptp_port_enabled && (!port->use_mgt_settable_log_sync_interval))
			port_sync_interval_setting_sm_initialize(port);

		break;

	case SYNC_INTERVAL_SETTING_SM_STATE_INITIALIZE:
	case SYNC_INTERVAL_SETTING_SM_STATE_SET_INTERVAL:
		if (sm->rcvd_signaling_msg3)
			port_sync_interval_setting_sm_set_intervals(port);

		break;

	default:
		break;
	}

exit:
	os_log(LOG_DEBUG, "Port(%u): domain(%u, %u) state %s new state %s\n", port->port_id, port->instance->index, port->instance->domain.domain_number, get_sync_interval_setting_state_str(state), get_sync_interval_setting_state_str(sm->state));
}


static void port_gptp_capable_receive_sm_not_enabled(struct gptp_port *port)
{
	struct ptp_gptp_capable_receive_sm *sm = &port->gptp_capable_receive_sm;

	sm->state = GPTP_CAPABLE_RECEIVE_SM_STATE_NOT_ENABLED;
}

static void port_gptp_capable_receive_sm_initialize(struct gptp_port *port)
{
	struct ptp_gptp_capable_receive_sm *sm = &port->gptp_capable_receive_sm;

	gptp_neighbor_gptp_capable_down(port);

	sm->state = GPTP_CAPABLE_RECEIVE_SM_STATE_INITIALIZE;
}

static void port_gptp_capable_receive_sm_received_tlv(struct gptp_port *port)
{
	struct ptp_gptp_capable_receive_sm *sm = &port->gptp_capable_receive_sm;
	u64 capable_interval;

	gptp_neighbor_gptp_capable_up(port);

	capable_interval = port->gptp_capable_receipt_timeout * log_to_ns(sm->rcvd_signaling_msg_ptr->u.ctlv.log_gptp_capable_message_interval);

	timer_restart(&sm->timeout_timer, capable_interval / NS_PER_MS);

	sm->rcvd_gptp_capable_tlv = false;

	sm->state = GPTP_CAPABLE_RECEIVE_SM_STATE_RECEIVED_TLV;
}


/** gPTPCapableReceive state machine (802.1AS-2020 section 10.4.2.2)
 * The GptpCapableReceive state machine shall implement the function specified by the state diagram in
 * Figure 10-22, the local variables specified in 10.4.2.1, the relevant parameters specified in 10.6.4 and
 * 10.6.4.4, and the relevant timing attributes specified in 10.7. This state machine is responsible for setting
 * neighborGptpCapable to TRUE on receipt of a Signaling message containing the gPTP-capable TLV, and
 * setting the timeout time after which neighborGptpCapable is set to FALSE.
*/
void port_gptp_capable_receive_sm(struct gptp_port *port, ptp_gptp_capable_receive_sm_event_t event)
{
	struct ptp_gptp_capable_receive_sm *sm = &port->gptp_capable_receive_sm;
	ptp_gptp_capable_receive_sm_state_t state = sm->state;
	struct ptp_instance_params *params = &port->instance->params;
	bool port_oper = get_port_oper(port);

	if ((params->begin) || (!params->instance_enable) ||
	(!port_oper) ||
	(!port->params.ptp_port_enabled)) {
		port_gptp_capable_receive_sm_not_enabled(port);
		goto exit;
	}

start:
	state = sm->state;

	switch (state) {
	case GPTP_CAPABLE_RECEIVE_SM_STATE_NOT_ENABLED:
		if (port_oper && port->params.ptp_port_enabled)
			port_gptp_capable_receive_sm_initialize(port);

		break;

	case GPTP_CAPABLE_RECEIVE_SM_STATE_INITIALIZE:
		if (sm->rcvd_gptp_capable_tlv)
			port_gptp_capable_receive_sm_received_tlv(port);

		break;

	case GPTP_CAPABLE_RECEIVE_SM_STATE_RECEIVED_TLV:
		if (sm->rcvd_gptp_capable_tlv)
			port_gptp_capable_receive_sm_received_tlv(port);
		else if (event == GPTP_CAPABLE_RECEIVE_SM_EVENT_TIMEOUT)
			port_gptp_capable_receive_sm_initialize(port);

		break;

	default:
		break;
	}

	if (sm->state != state)
		goto start;

exit:
	os_log(LOG_DEBUG, "Port(%u) domain(%u, %u): state %s new state %s\n", port->port_id, port->instance->index, port->instance->domain.domain_number, get_gptp_capable_receive_state_str(state), get_gptp_capable_receive_state_str(sm->state));
}


static struct ptp_signaling_pdu *set_gptp_capable_tlv(struct gptp_port *port)
{
	return md_set_gptp_capable(port);
}

static int tx_gptp_capable_signaling_msg(struct gptp_port *port, struct ptp_signaling_pdu *msg)
{
	return md_transmit_gptp_capable(port, msg);
}

static void port_gptp_capable_transmit_sm_not_enabled(struct gptp_port *port)
{
	struct ptp_gptp_capable_transmit_sm *sm = &port->gptp_capable_transmit_sm;

	sm->state = GPTP_CAPABLE_TRANSMIT_SM_STATE_NOT_ENABLED;
}

static void port_gptp_capable_transmit_sm_initialize(struct gptp_port *port)
{
	struct ptp_gptp_capable_transmit_sm *sm = &port->gptp_capable_transmit_sm;

	port->params.gptp_capable_message_slow_down = false;

	sm->number_gptp_capable_message_transmissions = 0;

	sm->state = GPTP_CAPABLE_TRANSMIT_SM_STATE_INITIALIZE;
}

static void port_gptp_capable_transmit_sm_transmit_tlv(struct gptp_port *port)
{
	struct ptp_gptp_capable_transmit_sm *sm = &port->gptp_capable_transmit_sm;
	struct ptp_instance_port_params *params = &port->params;
	u64 transmit_interval;

	sm->tx_signaling_msg_ptr = set_gptp_capable_tlv(port);
	tx_gptp_capable_signaling_msg(port, sm->tx_signaling_msg_ptr);

	if (port->params.gptp_capable_message_slow_down) {
		if (sm->number_gptp_capable_message_transmissions >= port->gptp_capable_receipt_timeout) {
			sm->interval3 = params->gptp_capable_message_interval;
			sm->number_gptp_capable_message_transmissions = 0;
			port->params.gptp_capable_message_slow_down = false;
		} else {
			sm->interval3 = params->old_gptp_capable_message_interval;
			sm->number_gptp_capable_message_transmissions++;
		}
	} else {
		sm->interval3 = params->gptp_capable_message_interval;
		sm->number_gptp_capable_message_transmissions = 0;
	}

	u_scaled_ns_to_u64(&transmit_interval, &sm->interval3);
	timer_restart(&sm->timeout_timer, transmit_interval / NS_PER_MS);

	sm->state = GPTP_CAPABLE_TRANSMIT_SM_STATE_TRANSMIT_TLV;
}

/** gPTPCapableTransmit state machine (802.1AS-2020 section 10.4.1.2)
 The GptpCapableTransmit state machine shall implement the function specified by the state diagram in
 Figure 10-21, the local variables specified in 10.4.1.1, the functions specified in 10.4.1.2, the relevant
 parameters specified in 10.6.4 and 10.6.4.4, and the relevant timing attributes specified in 10.7. This state
 machine is responsible for setting the parameters of each Signaling message that contains the gPTP-capable
 TLV, and causing these Signaling messages to be transmitted at a regular rate.
*/
void port_gptp_capable_transmit_sm(struct gptp_port *port, ptp_gptp_capable_transmit_sm_event_t event)
{
	struct ptp_gptp_capable_transmit_sm *sm = &port->gptp_capable_transmit_sm;
	struct ptp_instance_params *params = &port->instance->params;
	ptp_gptp_capable_transmit_sm_state_t state = sm->state;
	bool port_oper = get_port_oper(port);

	if ((params->begin) || (!params->instance_enable) ||
	(!port_oper) ||
	(!port->params.ptp_port_enabled)) {
		port_gptp_capable_transmit_sm_not_enabled(port);
		goto exit;
	}

start:
	state = sm->state;

	switch (state) {
	case GPTP_CAPABLE_TRANSMIT_SM_STATE_NOT_ENABLED:
		if (port_oper && port->params.ptp_port_enabled)
			port_gptp_capable_transmit_sm_initialize(port);

		break;

	case GPTP_CAPABLE_TRANSMIT_SM_STATE_INITIALIZE:
		/* UCT */
		port_gptp_capable_transmit_sm_transmit_tlv(port);
		break;

	case GPTP_CAPABLE_TRANSMIT_SM_STATE_TRANSMIT_TLV:
		if (event == GPTP_CAPABLE_TRANSMIT_SM_EVENT_INTERVAL)
			port_gptp_capable_transmit_sm_transmit_tlv(port);

		break;

	default:
		break;
	}

	if (sm->state != state)
		goto start;

exit:
	os_log(LOG_DEBUG, "Port(%u) domain(%u, %u): state %s new state %s\n", port->port_id, port->instance->index, port->instance->domain.domain_number, get_gptp_capable_transmit_state_str(state), get_gptp_capable_transmit_state_str(sm->state));
}


/**10.4.3.2.1 isSupportedLogGptpCapableMessageInterval (logGptpCapableMessageInterval): A
Boolean function that returns TRUE if the gPTP-capable message interval given by the argument
logGptpCapableMessageInterval is supported by the PTP Port and FALSE if the gPTP-capable message
interval is not supported by the PTP Port. The argument logGptpCapableMessageInterval has the same data
type and format as the field logGptpCapableMessageInterval of the gPTP-capable message interval request
TLV (see 10.6.4.5.6).
*/
static bool is_supported_log_gptp_capable_interval(struct ptp_gptp_capable_interval_setting_sm *sm, s8 log_gptp_capable_interval)
{
	if ((log_gptp_capable_interval < sm->log_supported_closest_longer_gptp_capable_message_interval) || (log_gptp_capable_interval > sm->log_supported_gptp_capable_message_interval_max))
		return false;
	else
		return true;
}

/**10.4.3.2.2 computeLogGptpCapableMessageInterval (logRequestedGptpCapableMessageInterval):
An Integer8 function that computes and returns the logGptpCapableMessageInterval, based on the
logRequestedGptpCapableMessageInterval. This function is defined as indicated below. It is defined here so
that the detailed code that it invokes does not need to be placed into the state machine diagram
*/
static s8 compute_log_gptp_capable_message_interval(struct ptp_gptp_capable_interval_setting_sm *sm, s8 log_gptp_capable_interval)
{
	if (is_supported_log_gptp_capable_interval(sm, log_gptp_capable_interval)) {
		/* The requested Sync Interval is supported and returned */
		return log_gptp_capable_interval;
	} else {
		if (log_gptp_capable_interval > sm->log_supported_gptp_capable_message_interval_max) {
			/* Return the fastest supported rate, even if faster than the requested rate */
			return sm->log_supported_gptp_capable_message_interval_max;
		} else {
			/* Return the fastest supported rate that is still slower than the requested rate. */
			return sm->log_supported_closest_longer_gptp_capable_message_interval;
		}
	}
}

static void port_gptp_capable_interval_setting_sm_not_enabled(struct gptp_port *port)
{
	struct ptp_gptp_capable_interval_setting_sm *sm = &port->gptp_capable_interval_sm;
	struct ptp_instance_port_params *params = &port->params;

	if (port->use_mgt_settable_log_gptp_capable_message_interval) {
		params->current_log_gptp_capable_message_interval = port->mgt_settable_log_gptp_capable_message_interval;
		u64_to_u_scaled_ns(&params->gptp_capable_message_interval, log_to_ns(params->current_log_gptp_capable_message_interval));
	}

	sm->state = GPTP_CAPABLE_INTERVAL_SM_STATE_NOT_ENABLED;
}

static void port_gptp_capable_interval_setting_sm_initialize(struct gptp_port *port)
{
	struct ptp_gptp_capable_interval_setting_sm *sm = &port->gptp_capable_interval_sm;
	struct ptp_instance_port_params *params = &port->params;

	params->current_log_gptp_capable_message_interval = params->initial_log_gptp_capable_message_interval;

	u64_to_u_scaled_ns(&params->gptp_capable_message_interval, log_to_ns(params->current_log_gptp_capable_message_interval));

	sm->rcvd_signaling_msg4 = false;

	params->old_gptp_capable_message_interval = params->gptp_capable_message_interval;

	params->gptp_capable_message_slow_down = false;

	sm->state = GPTP_CAPABLE_INTERVAL_SM_STATE_INITIALIZE;
}

static void port_gptp_capable_interval_sm_set_interval(struct gptp_port *port)
{
	struct ptp_gptp_capable_interval_setting_sm *sm = &port->gptp_capable_interval_sm;
	struct ptp_instance_port_params *params = &port->params;

	if (!port->use_mgt_settable_log_gptp_capable_message_interval)
	{
		sm->computed_log_gptp_capable_message_interval = compute_log_gptp_capable_message_interval(sm, sm->rcvd_signaling_ptr_gis->u.ctlv.log_gptp_capable_message_interval);

		switch (sm->rcvd_signaling_ptr_gis->u.ctlv.log_gptp_capable_message_interval)
		{
		case (-128): /* dont change the interval */
			break;

		case 126: /* set interval to initial value */
			params->current_log_gptp_capable_message_interval = params->initial_log_gptp_capable_message_interval;
			u64_to_u_scaled_ns(&params->gptp_capable_message_interval, log_to_ns(params->initial_log_gptp_capable_message_interval));

			break;

		default: /* use indicated value; note that the value of 127 instructs the receiving
				* port to stop sending, in accordance with Table 10-18. */
			if (sm->rcvd_signaling_ptr_gis->u.ctlv.log_gptp_capable_message_interval == 127) {
				if (timer_is_running(&port->gptp_capable_transmit_sm.timeout_timer))
					timer_stop(&port->gptp_capable_transmit_sm.timeout_timer);
				params->current_log_gptp_capable_message_interval = sm->rcvd_signaling_ptr_gis->u.ctlv.log_gptp_capable_message_interval;
			} else {
				u64_to_u_scaled_ns(&params->gptp_capable_message_interval, log_to_ns(sm->computed_log_gptp_capable_message_interval));
				params->current_log_gptp_capable_message_interval = sm->computed_log_gptp_capable_message_interval;
			}

			break;
		}

		if (u_scaled_ns_cmp(&params->gptp_capable_message_interval, &params->old_gptp_capable_message_interval) < 0)
			params->gptp_capable_message_slow_down = true;
		else
			params->gptp_capable_message_slow_down = false;
	}

	sm->rcvd_signaling_msg4 = false;

	sm->state = GPTP_CAPABLE_INTERVAL_SM_STATE_SET_INTERVAL;
}

/** gPTPCapableIntervalSetting state machine (802.1AS-2020 section 10.4.3)
 The GptpCapableIntervalSetting state machine shall implement the function specified by the state diagram
 in Figure 10-23, the local variables specified in 10.4.3.1, the functions specified in 10.4.3.2, the messages
 specified in 10.6, the relevant global variables specified in 10.2.5, the relevant managed objects specified in
 14.8, and the relevant timing attributes specified in 10.7. This state machine is responsible for setting the
 global variables that give the duration of the mean intervals between successive Signaling messages
 containing the gPTP-capable TLV, both at initialization and in response to the receipt of a Signaling message
 that contains a gPTP-capable Message Interval Request TLV (see 10.6.4.5).
*/
void port_gptp_capable_interval_setting_sm(struct gptp_port *port)
{
	struct ptp_gptp_capable_interval_setting_sm *sm = &port->gptp_capable_interval_sm;
	struct ptp_instance_params *params = &port->instance->params;
	ptp_gptp_capable_interval_setting_sm_state_t state = sm->state;
	bool port_oper = get_port_oper(port);

	if ((params->begin) || (!params->instance_enable) || (!port_oper) || (!port->params.ptp_port_enabled) ||
		port->use_mgt_settable_log_gptp_capable_message_interval) {
		port_gptp_capable_interval_setting_sm_not_enabled(port);
		goto exit;
	}

start:
	state = sm->state;

	switch (state) {
	case GPTP_CAPABLE_INTERVAL_SM_STATE_NOT_ENABLED:
		if (port_oper && port->params.ptp_port_enabled && (!port->use_mgt_settable_log_gptp_capable_message_interval))
			port_gptp_capable_interval_setting_sm_initialize(port);

		break;

	case GPTP_CAPABLE_INTERVAL_SM_STATE_INITIALIZE:
	case GPTP_CAPABLE_INTERVAL_SM_STATE_SET_INTERVAL:
		if (sm->rcvd_signaling_msg4)
			port_gptp_capable_interval_sm_set_interval(port);

		break;

	default:
		break;
	}

	if (state != sm->state)
		goto start;

exit:
	os_log(LOG_DEBUG, "Port(%u) domain(%u, %u): state %s new state %s\n", port->port_id, port->instance->index, port->instance->domain.domain_number, get_gptp_capable_interval_setting_state_str(state), get_gptp_capable_interval_setting_state_str(sm->state));
}
