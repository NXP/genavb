/*
 * Copyright 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief 802.1AS Clock Slave entity State Machines implementation
 @details Implementation of 802.1AS state machine and functions for Clock Slave entity
*/

#include "clock_sl_fsm.h"
#include "clock_ms_fsm.h"
#include "target_clock_adj.h"
#include "common/ptp_time_ops.h"
#include "config.h"

#include "os/clock.h"

static const char *clock_slave_sync_state_str[] = {
	[CLOCK_SLAVE_SYNC_SM_STATE_INITIALIZING] = "INITIALIZING",
	[CLOCK_SLAVE_SYNC_SM_STATE_SEND_SYNC_INDICATION] = "SEND_SYNC_INDICATION",
};

/* 10.2.12.2.1 updateSlaveTime(): updates the global variable clockSlaveTime (see 10.2.3.3), based on
information received from the SiteSync and LocalClock entities. It is the responsibility of the application to
filter slave times appropriately (see B.3 and B.4 for examples). As one example, clockSlaveTime can be
incremented by localClockTickInterval (see 10.2.3.18) divided by rateRatio member of the received
PortSyncSync structure. If no time-aware system is grandmaster-capable, i.e., gmPresent is FALSE, then
clockSlaveTime is set to the time provided by the LocalClock. This function is invoked when
rcvdLocalClockTick is TRUE. */
static void update_slave_time(struct gptp_port *port)
{
	struct ptp_instance_params *params = &port->instance->params;

	if (!params->gm_present) {
		u64 local_time;
		int err;

		err = os_clock_gettime64(gptp_port_to_clock(port), &local_time);
		if (err) {
			os_log(LOG_ERR, "os_clock_gettime64() failed with err %d\n", err);
			return;
		}

		u64_to_ptp_extended_timestamp(&params->clock_slave_time, local_time);
	} else {

	}
}

static void clock_slave_sync_sm_initializing(struct gptp_port *port)
{
	struct ptp_clock_slave_entity *clock_slave = &port->instance->clock_slave;

	clock_slave->rcvdPSSync = false;

	port->clock_slave_sync_sm_state = CLOCK_SLAVE_SYNC_SM_STATE_INITIALIZING;
}

/** Out of 802.1AS standard - Determine the synchronization state (SYNC/NOT SYNC) of the target clock based on the offset from the GM
 *
 * - The current synchronization state of the target clock is determine using 2 thresholds:
 * CFG_GPTP_SYNC_THRESH_HIGH : the target clock is considered as not synchronized if sync_diff is above this value
 * CFG_GPTP_SYNC_THRESH_LOW :  the target is considered as synchronized if sync_diff is below this value
 * - If a sync_indication handler exists it is notified only upon synchronization state change
 * - This function also computes how much time it takes to the target clock to reach synchonized state
 * since the reception of the first SYNC message from the GM
 *
 * \return none.
 * \param port context of the port for which synchronization state is update
 * \param sync_diff	64-bit timestamp diff between target clock timestamp when the sync message is received
 *  and the sync message itself set by the remote sender
 */
static void clock_slave_update_sync_state(struct gptp_port *port, u64 sync_diff)
{
	struct gptp_instance *instance = port->instance;
	struct gptp_ctx *gptp = instance->gptp;
	ptp_port_sync_state_t sync_state = port->sync_info.state;

	if (sync_diff <= CFG_GPTP_SYNC_THRESH_LOW) {
		if (sync_state != SYNC_STATE_SYNCHRONIZED)
			os_clock_gettime64(gptp->clock_monotonic, &port->sync_time_ns);

		sync_state = SYNC_STATE_SYNCHRONIZED;
	} else if (sync_diff >= CFG_GPTP_SYNC_THRESH_HIGH) {
		if (sync_state != SYNC_STATE_NOT_SYNCHRONIZED) {
			os_clock_gettime64(gptp->clock_monotonic, &port->nosync_time_ns);

			/* do not log a sync loss upon initial transition from
			undefined state to not synchronized */
			if (sync_state != SYNC_STATE_UNDEFINED)
				instance->stats.num_synchro_loss++;
		}

		sync_state = SYNC_STATE_NOT_SYNCHRONIZED;
	}

	if (gptp->sync_indication && (sync_state != port->sync_info.state) && (!port->instance->is_grandmaster)) {
		/* report sync time only upon transition from not sync to sync */
		if(sync_state == SYNC_STATE_SYNCHRONIZED)
			port->sync_info.sync_time_ms = (port->sync_time_ns - port->nosync_time_ns) / NS_PER_MS;

		os_log(LOG_DEBUG, "%s - offset %"PRIu64"\n", PTP_SYNC_STATE(sync_state), sync_diff);

		port->sync_info.port_id = port->port_id;
		port->sync_info.state = sync_state;
		port->sync_info.domain = instance->domain.domain_number;
		gptp_sync_indication(port);
	}
}


/* Out of 802.1AS standard - This is required to synchronize the target clock with the GM clock:
*/
void clock_slave_adjust_on_sync(struct gptp_port *port, u64 sync_receipt_time_u64, u64 sync_receipt_local_time_u64)
{
	struct gptp_instance *instance = port->instance;

	if (target_clock_adjust_on_sync(&instance->target_clkadj_params, sync_receipt_time_u64, sync_receipt_local_time_u64, port->port_sync.sync_receive_sm.rateRatio)) {
		/* Phase discontinuity reported */

		/* FIXME code below not supported with CMLDS, need to be revisited */
	#if 0
		/* FIXME update all ports */
		gptp_ratio_invalid(port);
	#endif
	}

	instance->stats.num_adjust_on_sync++;
}



/*
802.1AS - 10.2.12.3 Figure 10-9 ClockSlaveSync state machine
*/
static void clock_slave_sync_sm_send_indication(struct gptp_port *port)
{
	struct gptp_instance *instance = port->instance;
	struct ptp_clock_slave_entity *clock_slave = &instance->clock_slave;
	struct gptp_ctx *gptp = instance->gptp;
	struct ptp_instance_params *params = &instance->params;
#ifdef CFG_LOG_OFFSET
	u64 now;
#endif

	if (clock_slave->rcvdPSSync) {
		ptp_double neighbor_prop_delay_double, upstream_tx_time_double;
		ptp_double neighborRateRatio = get_neighbor_rate_ratio(port);
		ptp_double delay_asymmetry_double;
		u64 sync_receipt_time_u64, sync_receipt_local_time_u64;
		ptp_double neighbor_transit_time_double;
		struct ptp_u_scaled_ns neighbor_transit_time_u_scaled_ns;
		struct ptp_u_scaled_ns fup_correction_field_u_scaled_ns;
		struct ptp_u_scaled_ns sync_receipt_time_u_scaled_ns;
		struct ptp_u_scaled_ns precise_origin_timestamp_u_scaled_ns;
		struct ptp_u_scaled_ns delay_asymmetry_u_scaled_ns;

		/* syncReceiptTime */
		ptp_timestamp_to_u_scaled_ns(&precise_origin_timestamp_u_scaled_ns, &clock_slave->rcvdPSSyncPtr->preciseOriginTimestamp);
		scaled_ns_to_u_scaled_ns(&fup_correction_field_u_scaled_ns, &clock_slave->rcvdPSSyncPtr->followUpCorrectionField);
		u_scaled_ns_add(&sync_receipt_time_u_scaled_ns, &precise_origin_timestamp_u_scaled_ns, &fup_correction_field_u_scaled_ns);

		os_log(LOG_DEBUG, "pot=%"PRIu64" cor=%"PRIu64" pot+cor=%"PRIu64"\n", precise_origin_timestamp_u_scaled_ns.u.s.nanoseconds, fup_correction_field_u_scaled_ns.u.s.nanoseconds, sync_receipt_time_u_scaled_ns.u.s.nanoseconds);

		/* mean_link_delay should be small, but may have fractional nanoseconds so use a double */
		u_scaled_ns_to_ptp_double(&neighbor_prop_delay_double, get_mean_link_delay(port));
		neighbor_transit_time_double = neighbor_prop_delay_double * (clock_slave->rcvdPSSyncPtr->rateRatio / neighborRateRatio);
		ptp_double_to_u_scaled_ns(&neighbor_transit_time_u_scaled_ns, neighbor_transit_time_double);
		u_scaled_ns_add(&sync_receipt_time_u_scaled_ns, &sync_receipt_time_u_scaled_ns, &neighbor_transit_time_u_scaled_ns);

		scaled_ns_to_u_scaled_ns(&delay_asymmetry_u_scaled_ns, get_delay_asymmetry(port));
		u_scaled_ns_add(&sync_receipt_time_u_scaled_ns, &sync_receipt_time_u_scaled_ns, &delay_asymmetry_u_scaled_ns);

		u_scaled_ns_to_ptp_extended_timestamp(&params->sync_receipt_time, &sync_receipt_time_u_scaled_ns);
		u_scaled_ns_to_u64(&sync_receipt_time_u64, &sync_receipt_time_u_scaled_ns);

		os_log(LOG_DEBUG, "pot+cor+delay=%"PRIu64" (%"PRIu64")\n", sync_receipt_time_u_scaled_ns.u.s.nanoseconds, sync_receipt_time_u64);

		/* syncReceiptLocalTime */
		scaled_ns_to_ptp_double(&delay_asymmetry_double, get_delay_asymmetry(port));
		upstream_tx_time_double = (neighbor_prop_delay_double / neighborRateRatio) + (delay_asymmetry_double / clock_slave->rcvdPSSyncPtr->rateRatio);
		ptp_double_to_u_scaled_ns(&params->sync_receipt_local_time, upstream_tx_time_double);
		u_scaled_ns_add(&params->sync_receipt_local_time, &params->sync_receipt_local_time, &clock_slave->rcvdPSSyncPtr->upstreamTxTime);
		u_scaled_ns_to_u64(&sync_receipt_local_time_u64, &params->sync_receipt_local_time);

		os_log(LOG_DEBUG, "upstream_tx_time64=%u:%"PRIu64":%u sync_receipt_local_time_u64=%"PRIu64"\n",
			clock_slave->rcvdPSSyncPtr->upstreamTxTime.u.s.nanoseconds_msb,
			clock_slave->rcvdPSSyncPtr->upstreamTxTime.u.s.nanoseconds,
			clock_slave->rcvdPSSyncPtr->upstreamTxTime.u.s.fractional_nanoseconds,
			sync_receipt_local_time_u64);

		/* cumulativeRateRatio 14.4.2 */
		instance->cumulativeRateRatio = (s32)((clock_slave->rcvdPSSyncPtr->rateRatio - 1.0) * POW_2_41);

		/* 14.3.8, 14.3.9 */
		if (params->gm_time_base_indicator != clock_slave->rcvdPSSyncPtr->gmTimeBaseIndicator) {
			u64 system_time;

			if (gptp_system_time(gptp, &system_time) < 0) {
				os_log(LOG_ERR, "gptp_system_time() failed\n");
				system_time = 0;
			}
			system_time /= 10000000; /* 0.01s */

			if (clock_slave->rcvdPSSyncPtr->lastGmPhaseChange.u.s.nanoseconds || clock_slave->rcvdPSSyncPtr->lastGmPhaseChange.u.s.nanoseconds_msb || clock_slave->rcvdPSSyncPtr->lastGmPhaseChange.u.s.fractional_nanoseconds)
				instance->timeOfLastGmPhaseChangeEvent = system_time;

			if (clock_slave->rcvdPSSyncPtr->lastGmFreqChange)
				instance->timeOfLastGmFreqChangeEvent = system_time;
		}

		params->gm_time_base_indicator = clock_slave->rcvdPSSyncPtr->gmTimeBaseIndicator;
		params->last_gm_phase_change = clock_slave->rcvdPSSyncPtr->lastGmPhaseChange;
		params->last_gm_freq_change = clock_slave->rcvdPSSyncPtr->lastGmFreqChange;

		// invokeApplicationInterfaceFunction(ClockTargetPhaseDiscontinuity.result); // TODO notify phase discontinuity to applications

		os_clock_convert(gptp->clock_local, sync_receipt_local_time_u64, instance->clock_target, &sync_receipt_local_time_u64);

		/*
		 * Report GM changes to the target clock adjustment logic.
		 * This allows to converge to the new adjustment faster.
		 */
		if (os_memcmp(&clock_slave->prev_gm_clock_identity, &params->gm_priority.u.s.root_system_identity.u.s.clock_identity, sizeof(struct ptp_clock_identity))) {
			target_clkadj_gm_change(&instance->target_clkadj_params);

			os_memcpy(&clock_slave->prev_gm_clock_identity, &params->gm_priority.u.s.root_system_identity.u.s.clock_identity, sizeof(struct ptp_clock_identity));

			params->do_clock_adjust = 1;
		} else {
		#if defined (CONFIG_GENAVB_HYBRID)
			/*
			 * WA: protection against spurious wrong timestamp not reflecting the real synchronization offset
			 * between the target and the grandmaster. If we are in synchronized state and if at least 2
			 * consecutives offset measurements are above the offset correction threshold then the target clock
			 * is adjusted. Else the clock offset/frequency adjustements and sync/no sync state check are simply bypassed.
			 */
			if ((os_llabs(sync_receipt_time_u64 - sync_receipt_local_time_u64) > CFG_GPTP_SYNC_THRESH_HIGH) && (port->sync_info.state == SYNC_STATE_SYNCHRONIZED) && (params->do_clock_adjust)) {
				params->do_clock_adjust = 0;
				os_log(LOG_DEBUG, "do_clock_adjust = 0 - sync_diff = %"PRIu64" \n", os_llabs(sync_receipt_time_u64 - sync_receipt_local_time_u64));
			} else
		#endif
				params->do_clock_adjust = 1;
		}

		/*
		 * Out of 802.1AS standard - target clock adjustement and determination of synchronization state
		 */
		if (params->do_clock_adjust) {
			clock_slave_adjust_on_sync(port, sync_receipt_time_u64, sync_receipt_local_time_u64);
			clock_slave_update_sync_state(port, os_llabs(sync_receipt_time_u64 - sync_receipt_local_time_u64));
		}

#ifdef CFG_LOG_OFFSET
		/*
		 * Used to monitor offset value over time
		 */
		os_clock_gettime64(gptp_port_to_clock(port), &now);
		os_log(LOG_INFO, "Port(%u): time %"PRId64" state %s offset %"PRId64"\n", port->sync_info.port_id, now, PTP_SYNC_STATE(port->sync_info.state), sync_receipt_time_u64 - sync_receipt_local_time_u64);
#endif
	}

	if (clock_slave->rcvdLocalClockTick)
		update_slave_time(port);

	/*
	 * Notify ClockMasterSyncOffset state machine
	 */
	if (clock_slave->rcvdPSSync) {
		instance->clock_master.sync_offset_sm.rcvd_sync_receipt_time = true;

		clock_master_sync_offset_sm(instance);
	}

	clock_slave->rcvdPSSync = clock_slave->rcvdLocalClockTick = false;
	clock_slave->rcvdPSSyncPtr = NULL;

	port->clock_slave_sync_sm_state = CLOCK_SLAVE_SYNC_SM_STATE_SEND_SYNC_INDICATION;

}


/** 802.1AS state machine for Clock Slave entity
 * \return	0 on success, negative value on failure
 * \param port	pointer to the gptp port context, used to retrieve the state machine variables
 */
int clock_slave_sync_sm(struct gptp_port *port)
{
	ptp_clock_slave_sync_sm_state_t state = port->clock_slave_sync_sm_state;
	struct ptp_clock_slave_entity *clock_slave = &port->instance->clock_slave;
	struct ptp_instance_params *params = &port->instance->params;
	int rc = 0;

	if ((params->begin) || (!params->instance_enable)) {
		clock_slave_sync_sm_initializing(port);
		goto exit;
	}

	switch (state) {
	case CLOCK_SLAVE_SYNC_SM_STATE_INITIALIZING:
	case CLOCK_SLAVE_SYNC_SM_STATE_SEND_SYNC_INDICATION:
		if ((clock_slave->rcvdPSSync) || (clock_slave->rcvdLocalClockTick))
			clock_slave_sync_sm_send_indication(port);

		break;

	default:
		break;
	}

exit:

	os_log(LOG_DEBUG, "Port(%u): domain(%u, %u) state %s new state %s\n", port->port_id,
		port->instance->index, port->instance->domain.domain_number,
		clock_slave_sync_state_str[state],
		clock_slave_sync_state_str[port->clock_slave_sync_sm_state]);

	return rc;
}
