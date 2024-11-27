/*
 * Copyright 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief 802.1AS Clock Master entity State Machines implementation
 @details Implementation of 802.1AS state machine and functions for Clock Master entity
*/

#include "clock_ms_fsm.h"
#include "site_fsm.h"
#include "common/ptp_time_ops.h"
#include "config.h"
#include "../common/ptp.h"

#include "os/clock.h"

static const char *clock_master_sync_send_state_str[] = {
	[CLOCK_MASTER_SYNC_SEND_SM_STATE_INITIALIZING] = "INITIALIZING",
	[CLOCK_MASTER_SYNC_SEND_SM_STATE_SEND_SYNC_INDICATION] = "SYNC INDICATION",
};

static const char *clock_master_sync_send_event_str[] = {
	[CLOCK_MASTER_SYNC_SEND_SM_EVENT_INTERVAL] = "INTERVAL",
	[CLOCK_MASTER_SYNC_SEND_SM_EVENT_RUN] = "RUN"
};


/* 10.2.8.2.1 */
static struct port_sync_sync *set_pssync_cmss(struct gptp_instance *instance, ptp_double gm_rate_ratio)
{
	struct ptp_instance_params *params = &instance->params;
	ptp_double follow_up_correction_field_double;
	struct ptp_u_scaled_ns dt_u_scaled_ns;
	struct ptp_scaled_ns dt_scaled_ns;
	ptp_double dt_double;

	instance->clock_master.pssync.localPortNumber = 0;

	ptp_extended_timestamp_to_ptp_timestamp(&instance->clock_master.pssync.preciseOriginTimestamp, &params->master_time);

	/* followUpCorrectionField is set equal to the sum of
	1) the fractional nanoseconds portion of masterTime.fractionalNanoseconds,
	2) the quantity gmRateRatio x (currentTime - localTime)
	*/
	u_scaled_ns_sub(&dt_u_scaled_ns, &params->current_time, &params->local_time);
	u_scaled_ns_to_scaled_ns(&dt_scaled_ns, &dt_u_scaled_ns);
	scaled_ns_to_ptp_double(&dt_double, &dt_scaled_ns);
	follow_up_correction_field_double = params->master_time.fractional_nanoseconds_lsb + gm_rate_ratio * dt_double;
	ptp_double_to_scaled_ns(&instance->clock_master.pssync.followUpCorrectionField, follow_up_correction_field_double);

	os_memcpy(&instance->clock_master.pssync.sourcePortIdentity.clock_identity, &instance->params.this_clock, sizeof(struct ptp_clock_identity));
	instance->clock_master.pssync.sourcePortIdentity.port_number = 0;

	instance->clock_master.pssync.logMessageInterval = instance->clock_master_log_sync_interval;
	instance->clock_master.pssync.upstreamTxTime = params->local_time;
	u64_to_u_scaled_ns(&instance->clock_master.pssync.syncReceiptTimeoutTime, 0xFFFFFFFFFFFFFFFF);
	instance->clock_master.pssync.rateRatio = gm_rate_ratio;

	instance->clock_master.pssync.gmTimeBaseIndicator = params->clock_source_time_base_indicator;
	instance->clock_master.pssync.lastGmPhaseChange = params->clock_source_phase_offset;
	instance->clock_master.pssync.lastGmFreqChange = params->clock_source_freq_offset;

	return &instance->clock_master.pssync;
}

static void tx_pssync_cmss(struct gptp_instance *instance, struct port_sync_sync *tx_pssync_ptr)
{
	instance->site_sync.sync_sm.rcvdPSSync = true;
	instance->site_sync.sync_sm.rcvdPSSyncPtr = tx_pssync_ptr;

	site_sync_sync_sm(instance);
}

static void clock_master_sync_send_sm_initializing(struct gptp_instance *instance)
{
	instance->clock_master_sync_send_sm_state = CLOCK_MASTER_SYNC_SEND_SM_STATE_INITIALIZING;

	timer_start(&instance->clock_master_sync_send_timer, log_to_ms(instance->clock_master_log_sync_interval));
}

static void clock_master_sync_send_sm_send_sync_indication(struct gptp_instance *instance)
{
	instance->clock_master_sync_send_sm_state = CLOCK_MASTER_SYNC_SEND_SM_STATE_SEND_SYNC_INDICATION;

	instance->clock_master.sync_send_sm.txPSSyncPtr = set_pssync_cmss(instance, instance->params.gm_rate_ratio);
	tx_pssync_cmss(instance, instance->clock_master.sync_send_sm.txPSSyncPtr);

	timer_stop(&instance->clock_master_sync_send_timer);
	timer_start(&instance->clock_master_sync_send_timer, log_to_ms(instance->clock_master_log_sync_interval));
}

/** clockMasterSyncSend - 10.2.8
 * The state machine receives masterTime and clockSourceTimeBaseIndicator from the ClockMasterSyncReceive
 * state machine, and phase and frequency offset between masterTime and syncReceiptTime from the
 * ClockMasterSyncOffset state machine. It provides masterTime (i.e., synchronized time) and the phase and
 * frequency offset to the SiteSync entity via a PortSyncSync structure.
 */
int clock_master_sync_send_sm(struct gptp_instance *instance, ptp_clock_master_sync_send_sm_event_t event)
{
	ptp_clock_master_sync_send_sm_state_t state = instance->clock_master_sync_send_sm_state;
	struct ptp_instance_params *params = &instance->params;
	int rc = 0;

	if (params->begin || (!params->instance_enable)) {
		clock_master_sync_send_sm_initializing(instance);
		goto exit;
	}

	switch(state) {
	case CLOCK_MASTER_SYNC_SEND_SM_STATE_INITIALIZING:
	case CLOCK_MASTER_SYNC_SEND_SM_STATE_SEND_SYNC_INDICATION:
		if (event == CLOCK_MASTER_SYNC_SEND_SM_EVENT_INTERVAL) {
			/* FIXME - Temporary WA to emulate event received from ClockSource entity in order to update the localClock (clock_master_sync_receive_sm).
			To be removed once the required support of application interface per 9.1 is implemented. */
			instance->clock_master.sync_receive_sm.rcvd_local_clock_tick = true;
			instance->params.gm_rate_ratio = 1.0;
			clock_master_sync_receive_sm(instance);

			clock_master_sync_send_sm_send_sync_indication(instance);
		}
		break;

	default:
		break;
	}
exit:
	os_log(LOG_DEBUG, "domain(%u, %u) state %s event %s new state %s\n", instance->index, instance->domain.domain_number, clock_master_sync_send_state_str[state], clock_master_sync_send_event_str[event], clock_master_sync_send_state_str[instance->clock_master_sync_send_sm_state]);
	return rc;
}

/* 10.2.9.1 computes clockSourceFreqOffset (see 10.2.3.6), using
successive values of masterTime computed by the ClockMasterSyncReceive state machine (see 10.2.10) and
successive values of syncReceiptTime computed by the ClockSlaveSync state machine (see 10.2.12). Any
scheme that uses this information to compute clockSourceFreqOffset is acceptable as long as the
performance requirements specified in B.2.4 are met. */
static ptp_double compute_clock_source_freq_offset(struct gptp_instance *instance)
{
	struct ptp_instance_params *params = &instance->params;
	struct gptp_ctx *gptp = instance->gptp;
	struct ptp_u_scaled_ns sync_receipt_time;
	struct ptp_u_scaled_ns dt_u_scaled_ns, dt_local_u_scaled_ns;
	struct ptp_scaled_ns dt_scaled_ns, dt_local_scaled_ns;
	ptp_double dt_double, dt_local_double;
	ptp_double ratio;

	ptp_extended_timestamp_to_u_scaled_ns(&sync_receipt_time, &params->sync_receipt_time);

	if (instance->clock_master.clock_source_freq_offset_init) {
		u_scaled_ns_sub(&dt_u_scaled_ns, &sync_receipt_time, &instance->clock_master.sync_receipt_time_prev);
		u_scaled_ns_sub(&dt_local_u_scaled_ns, &params->sync_receipt_local_time, &instance->clock_master.sync_receipt_local_time_prev);

		u_scaled_ns_to_scaled_ns(&dt_scaled_ns, &dt_u_scaled_ns);
		u_scaled_ns_to_scaled_ns(&dt_local_scaled_ns, &dt_local_u_scaled_ns);

		scaled_ns_to_ptp_double(&dt_double, &dt_scaled_ns);
		scaled_ns_to_ptp_double(&dt_local_double, &dt_local_scaled_ns);

		ratio = dt_local_double / (dt_double * gptp->local_clock.rate_ratio_adjustment);

		instance->clock_master.ratio_average = 0.1 * ratio + 0.9 * instance->clock_master.ratio_average;

		os_log(LOG_DEBUG, "domain(%u, %u) clock_source_freq_offset + 1.0 = %.16f(%.16f) %.16f\n",
				instance->index, instance->domain.domain_number, instance->clock_master.ratio_average, ratio, gptp->local_clock.rate_ratio_adjustment);
	} else {
		instance->clock_master.ratio_average = 1.0;
	}

	instance->clock_master.clock_source_freq_offset_init = true;

	instance->clock_master.sync_receipt_time_prev = sync_receipt_time;
	instance->clock_master.sync_receipt_local_time_prev = params->sync_receipt_local_time;

	return instance->clock_master.ratio_average - 1.0;
}

static void clock_master_sync_offset_sm_received_sync_receipt_time(struct gptp_instance *instance)
{
	struct ptp_instance_params *params = &instance->params;

	instance->clock_master.sync_offset_sm.rcvd_sync_receipt_time = false;

	if (params->selected_role[0] == PASSIVE_PORT) {
		/* FIXME difference between source_time and sync_receipt_time */
		/* params->clock_source_phase_offset = system->clock_master.sync_offset_sm.rcvd_sync_receipt_time_ptr->source_time.seconds; */
		ptp_double_to_scaled_ns(&params->clock_source_phase_offset, 0.0);

		params->clock_source_freq_offset = compute_clock_source_freq_offset(instance);
	} else {
		if (params->clock_source_time_base_indicator != params->clock_source_time_base_indicator_old) {
			params->clock_source_phase_offset = params->clock_source_last_gm_phase_change;
			params->clock_source_freq_offset = params->clock_source_last_gm_freq_change;
		}

		/* Non standard */
		instance->clock_master.clock_source_freq_offset_init = false;
	}

	instance->clock_master_sync_offset_sm_state = CLOCK_MASTER_SYNC_OFFSET_SM_STATE_RECEIVED_SYNC_RECEIPT_TIME;
}


static void clock_master_sync_offset_sm_initializing(struct gptp_instance *instance)
{
//	struct ptp_instance_params *params = &system->params;

	instance->clock_master.sync_offset_sm.rcvd_sync_receipt_time = false;

	instance->clock_master_sync_offset_sm_state = CLOCK_MASTER_SYNC_OFFSET_SM_STATE_INITIALIZING;

	/* Non standard */
	instance->clock_master.clock_source_freq_offset_init = false;
}

void clock_master_sync_offset_sm(struct gptp_instance *instance)
{
	struct ptp_instance_params *params = &instance->params;

	if ((params->begin) || (!params->instance_enable))
		clock_master_sync_offset_sm_initializing(instance);

	switch (instance->clock_master_sync_offset_sm_state) {
	case CLOCK_MASTER_SYNC_OFFSET_SM_STATE_INITIALIZING:
	case CLOCK_MASTER_SYNC_OFFSET_SM_STATE_RECEIVED_SYNC_RECEIPT_TIME:
		if (instance->clock_master.sync_offset_sm.rcvd_sync_receipt_time)
			clock_master_sync_offset_sm_received_sync_receipt_time(instance);

		break;

	default:
		break;
	}
}

/* 10.2.10.2.1 computeGmRateRatio(): computes gmRateRatio(see 10.2.3.14), using values of sourceTime
conveyed by successive ClockSourceTime.invoke functions (see 9.2.2.1), and corresponding values of
localTime (see 10.2.3.19). Any scheme that uses this information, along with any other information
conveyed by the successive ClockSourceTime.invoke functions and corresponding values of localTime, to
compute gmRateRatio is acceptable as long as the performance requirements specified in B.2.4 are met.
NOTE—As one example, gmRateRatio can be estimated as the ratio of the elapsed time of the ClockSource entity that
supplies time to this time-aware system, to the elapsed time of the LocalClock entity of this time-aware system. This
ratio can be computed for the time interval between a received ClockSourceTime.invoke function and a second received
ClockSourceTime.invoke function some number of ClockSourceTime.invoke functions later, i.e.,
where the successive received ClockSourceTime.invoke functions are indexed from 0 to N, with the first such function
indexed as 0, and localTimej is the value of localTime when the ClockSourceTime.invoke function whose index is j is
received. */
static void compute_gm_rate_ratio(struct gptp_instance *instance)
{
	struct ptp_instance_params *params = &instance->params;

	/* Since we don't implement ClockSource entity and our time comes from LocalClock, here we set the ratio to 1.0 */

	params->gm_rate_ratio = 1.0;
}

/* 10.2.10.2.2 updateMasterTime(): updates the global variable masterTime (see 10.2.3.21), based on
information received from the ClockSource and LocalClock entities. It is the responsibility of the
application to filter master times appropriately. As one example, masterTime can be set equal to the
sourceTime member of the ClockSourceTime.invoke function when this function is received, and can be
incremented by localClockTickInterval (see 10.2.3.18) divided by gmRateRatio (see 10.2.3.14) when
rcvdLocalClockTick is TRUE. */
static void update_master_timer(struct gptp_instance *instance)
{
	struct ptp_instance_params *params = &instance->params;

	if (instance->clock_master.sync_receive_sm.rcvd_clock_source_req)
		params->master_time = instance->clock_master.sync_receive_sm.rcvd_clock_source_req_ptr->source_time;
	else if (instance->clock_master.sync_receive_sm.rcvd_local_clock_tick) {
		u64 source_time;
		int err;

		err = os_clock_gettime64(instance->clock_source, &source_time);
		if (err) {
			os_log(LOG_ERR, "os_clock_gettime64() failed with err %d\n", err);
			return;
		}

		u64_to_ptp_extended_timestamp(&params->master_time, source_time);
	}
}

static void clock_master_sync_receive_sm_receive_source_time(struct gptp_instance *instance)
{
	struct ptp_instance_params *params = &instance->params;
	u64 master_time_u64, current_time_u64;
	struct ptp_u_scaled_ns master_time_u_scaled;

	update_master_timer(instance);

	/* WA - Since our source time comes from the LocalClock we assume no offset between master time and current time */
	ptp_extended_timestamp_to_u_scaled_ns(&master_time_u_scaled, &params->master_time);
	u_scaled_ns_to_u64(&master_time_u64, &master_time_u_scaled);

	os_clock_convert(instance->clock_source, master_time_u64, instance->gptp->clock_local, &current_time_u64);

	u64_to_u_scaled_ns(&params->current_time, current_time_u64);
	params->local_time = params->current_time;

	if (instance->clock_master.sync_receive_sm.rcvd_clock_source_req) {
		compute_gm_rate_ratio(instance);

		params->clock_source_time_base_indicator_old = params->clock_source_time_base_indicator;
		params->clock_source_time_base_indicator = instance->clock_master.sync_receive_sm.rcvd_clock_source_req_ptr->time_base_indicator;
		params->clock_source_last_gm_phase_change = instance->clock_master.sync_receive_sm.rcvd_clock_source_req_ptr->last_gm_phase_change;
		params->clock_source_last_gm_freq_change = instance->clock_master.sync_receive_sm.rcvd_clock_source_req_ptr->last_gm_freq_change;
	}

	instance->clock_master.sync_receive_sm.rcvd_clock_source_req = false;
	instance->clock_master.sync_receive_sm.rcvd_local_clock_tick = false;

	/* UCT */
	instance->clock_master_sync_receive_sm_state = CLOCK_MASTER_SYNC_RECEIVE_SM_STATE_WAITING;
}

static void clock_master_sync_receive_sm_initializing(struct gptp_instance *instance)
{
	struct ptp_instance_params *params = &instance->params;

	ptp_double_to_ptp_extended_timestamp(&params->master_time, 0.0);
	ptp_double_to_u_scaled_ns(&params->local_time, 0.0);
	params->clock_source_time_base_indicator_old = 0;

	instance->clock_master.sync_receive_sm.rcvd_clock_source_req = false;
	instance->clock_master.sync_receive_sm.rcvd_local_clock_tick = false;

	/* UCT */
	instance->clock_master_sync_receive_sm_state = CLOCK_MASTER_SYNC_RECEIVE_SM_STATE_WAITING;
}

void clock_master_sync_receive_sm(struct gptp_instance *instance)
{
	struct ptp_instance_params *params = &instance->params;

	if ((params->begin) || (!params->instance_enable))
		clock_master_sync_receive_sm_initializing(instance);

	switch (instance->clock_master_sync_receive_sm_state) {
	case CLOCK_MASTER_SYNC_RECEIVE_SM_STATE_WAITING:
		if (instance->clock_master.sync_receive_sm.rcvd_clock_source_req || instance->clock_master.sync_receive_sm.rcvd_local_clock_tick)
			clock_master_sync_receive_sm_receive_source_time(instance);

		break;

	default:
		break;
	}
}
