/*
 * Copyright 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Implementation of 802.1 AS Site entity State Machines
 @details Implementation of 802.1 AS state machine and functions for Site entity
*/

#include "site_fsm.h"
#include "bmca.h"
#include "port_fsm.h"
#include "clock_sl_fsm.h"
#include "gptp.h"


static const char *port_state_sm_state_str[] = {
	[PORT_STATE_SM_STATE_INIT_BRIDGE] = "INIT_BRIDGE",
	[PORT_STATE_SM_STATE_SELECTION] = "SELECTION",
};

static const char *port_role2string[] = {
	[MASTER_PORT] = "MASTER",
	[SLAVE_PORT] = "SLAVE",
	[PASSIVE_PORT] = "PASSIVE",
	[DISABLED_PORT] = "DISABLED",
};

static const char *spanning_tree2string[] = {
	[SPANNING_TREE_RECEIVED] = "TREE_RECEIVED",
	[SPANNING_TREE_MINE] = "TREE_MINE",
	[SPANNING_TREE_AGED] = "TREE_AGED",
	[SPANNING_TREE_DISABLED] = "TREE_DISABLED",
};

static const char *site_sync_sync_sm_state_str[] = {
	[SITE_SYNC_SYNC_SM_STATE_INITIALIZING] = "INITIALIZING",
	[SITE_SYNC_SYNC_SM_STATE_RECEIVING_SYNC] = "RECEIVING SYNC",
};

/*
* PortRoleSelectionSM state machine and functions (802.1AS - 10.3.12)
*/


/* updtRoleDisabledTree - 10.3.12.1.1
 * sets all the elements of the selectedRole array (see 10.2.3.20) to
 * DisabledPort. Sets lastGmPriority to all ones. Sets the pathTrace array (see 10.3.8.21) to contain the single
 * element thisClock (see 10.2.3.22).
 */
static void updt_role_disabled_tree(struct gptp_instance *instance)
{
	int i;

	for (i = 0; i < instance->numberPorts + 1; i++)
		instance->params.selected_role[i] = DISABLED_PORT;

	os_memset(&instance->params.last_gm_priority, 0xFF, sizeof(struct ptp_priority_vector));

	os_memset(&instance->params.path_trace[0], 0, MAX_PTLV_ENTRIES * sizeof(struct ptp_clock_identity));
	os_memcpy(&instance->params.path_trace[0], &instance->params.this_clock.identity[0], sizeof(struct ptp_clock_identity));
	instance->params.num_ptlv = 1;

	gptp_ipc_gm_status(instance, &instance->gptp->ipc_tx, IPC_DST_ALL);
}

/* clearReselectTree - 10.3.12.1.2
 * sets all the elements of the reselect array (see 10.3.8.1) to FALSE.
 */
static void clear_reselect_tree(struct gptp_instance *instance)
{
	instance->params.reselect = 0x0;
}

/* setSelectedTree - 10.3.12.1.3
 * sets all the elements of the selected array (see 10.3.8.2) to TRUE.
 */
static void set_selected_tree(struct gptp_instance *instance)
{
	int i;

	instance->params.selected = 0xffff;

	for (i = 0; i < instance->numberPorts; i++) {
		port_announce_info_sm(&instance->ports[i], PORT_ANNOUNCE_INFO_SM_EVENT_RUN);
		port_announce_transmit_sm(&instance->ports[i], PORT_ANNOUNCE_TRANSMIT_SM_EVENT_RUN);
	}
}

/* updtRolesTree - 10.3.12.1.4
 *
 */
static void updt_roles_tree(struct gptp_instance *instance)
{
	int i, j;
	int best_vector_port = 0;
	struct ptp_priority_vector *best_vector;
	struct gptp_port *port;
	ptp_port_role_t previous_port_role;
	struct ptp_priority_vector gm_path_priority[CFG_GPTP_MAX_NUM_PORT] = {0};
	bool prev_is_grandmaster;

	/*
	a) Computes the gmPathPriorityVector for each port that has a portPriorityVector and for which neither
	announce receipt timeout nor, if gmPresent is TRUE, sync receipt timeout have occurred,
	*/
	for (i = 0; i < instance->numberPorts; i++) {
		port = &instance->ports[i];
		/* not aged means no announce nor sync receipt timeout occured */
		if (port->params.info_is == SPANNING_TREE_RECEIVED){
			/* (see 10.3.5) */
			os_memcpy(&gm_path_priority[i], &port->params.port_priority, sizeof(struct ptp_priority_vector));
			gm_path_priority[i].u.s.steps_removed = htons(ntohs(gm_path_priority[i].u.s.steps_removed) + 1);
			gm_path_priority[i].u.s.port_number = htons(get_port_identity_number(port));
			os_log(LOG_DEBUG, "(a) port %d - steps %d port_number %d info_is %d\n", i, ntohs(gm_path_priority[i].u.s.steps_removed), ntohs(gm_path_priority[i].u.s.port_number), port->params.info_is);
		}
	}

	/*
	b) Saves gmPriority (see 10.3.8.19) in lastGmPriority (see 10.3.8.20), computes the gmPriorityVector
	for the time-aware system and saves it in gmPriority, chosen as the best of the set consisting of the
	systemPriorityVector (for this time-aware system) and the gmPathPriorityVector for each port for
	which the clockIdentity of the master port is not equal to thisClock (see 10.2.3.22),
	*/
	os_memcpy(&instance->params.last_gm_priority, &instance->params.gm_priority, sizeof(struct ptp_priority_vector));
	best_vector = &instance->params.system_priority;
	for (i = 0; i < instance->numberPorts; i++) {
		port = &instance->ports[i];
		if (port->params.info_is == SPANNING_TREE_RECEIVED) {
			dump_priority_vector(&gm_path_priority[i], instance->index, instance->domain.domain_number, "(b) gm path priority vector", LOG_DEBUG);
			if (os_memcmp(&gm_path_priority[i].u.s.root_system_identity.u.s.clock_identity, &instance->params.this_clock, sizeof(struct ptp_clock_identity))){
				if (compare_system_identity(best_vector, &gm_path_priority[i]) == BMCA_VECTOR_B_BETTER) {
					best_vector = &gm_path_priority[i];
					best_vector_port = i;
					os_log(LOG_DEBUG, "(b) port %d - gm priority is better\n", best_vector_port);
				}
			}
		}
	}

	os_memcpy(&instance->params.gm_priority, best_vector, sizeof(struct ptp_priority_vector));

	/*
	c) Sets the per-time-aware system global variables leap61, leap59, currentUtcOffsetValid,
	timeTraceable, frequencyTraceable, currentUtcOffset, and timeSource as follows:
	1) If the gmPriorityVector was set to the gmPathPriorityVector of one of the ports, then leap61,
	leap59, currentUtcOffsetValid, timeTraceable, frequencyTraceable, currentUtcOffset, and
	timeSource are set to annLeap61, annLeap59, annCurrentUtcOffsetValid, annTimeTraceable,
	annFrequencyTraceable, annCurrentUtcOffset, and annTimeSource, respectively, for that port.
	2) If the gmPriorityVector was set to the systemPriorityVector, then leap61, leap59,
	currentUtcOffsetValid, timeTraceable, frequencyTraceable, currentUtcOffset, and timeSource
	are set to sysLeap61, sysLeap59, sysCurrentUtcOffsetValid, sysTimeTraceable,
	sysFrequencyTraceable, sysCurrentUtcOffset, and sysTimeSource, respectively.
	*/
	if (best_vector != &instance->params.system_priority) {
		os_log(LOG_DEBUG, "(c) best vector is gm path vector from port %d\n", best_vector_port);
		instance->params.leap61 = instance->ports[best_vector_port].params.ann_leap61;
		instance->params.leap59 = instance->ports[best_vector_port].params.ann_leap59;
		instance->params.current_utc_offset_valid = instance->ports[best_vector_port].params.ann_current_utc_offset_valid;
		instance->params.time_traceable = instance->ports[best_vector_port].params.ann_time_traceable;
		instance->params.frequency_traceable = instance->ports[best_vector_port].params.ann_frequency_traceable;
		instance->params.current_utc_offset = instance->ports[best_vector_port].params.ann_current_utc_offset;
		instance->params.time_source = instance->ports[best_vector_port].params.ann_time_source;

		/* (see 10.3.5) */
		instance->params.gm_priority.u.s.steps_removed = htons(ntohs(instance->ports[best_vector_port].params.port_priority.u.s.steps_removed) + 1);
		instance->params.gm_priority.u.s.port_number = htons(get_port_identity_number(&instance->ports[best_vector_port]));
	} else {
		os_log(LOG_DEBUG, "(c) best vector is system\n");
		instance->params.leap61 = instance->params.sys_leap61;
		instance->params.leap59 = instance->params.sys_leap59;
		instance->params.current_utc_offset_valid = instance->params.sys_current_utc_offset_valid;
		instance->params.time_traceable = instance->params.sys_time_traceable;
		instance->params.frequency_traceable = instance->params.sys_frequency_traceable;
		instance->params.current_utc_offset = instance->params.sys_current_utc_offset;
		instance->params.time_source = instance->params.sys_time_source;

		/* (see 10.3.5) */
		instance->params.gm_priority.u.s.steps_removed = htons(0);
		instance->params.gm_priority.u.s.port_number = htons(0);
		instance->params.gm_priority.u.s.source_port_identity.port_number = htons(0);
	}

	/*
	d) Computes the masterPriorityVector for each port (see 10.3.5)
	*/
	dump_priority_vector(&instance->params.system_priority, instance->index, instance->domain.domain_number, "(d) system_priority", LOG_DEBUG);
	dump_priority_vector(&instance->params.gm_priority, instance->index, instance->domain.domain_number, "(d) gm_priority", LOG_DEBUG);
	for (i = 0; i < instance->numberPorts; i++) {
		port = &instance->ports[i];
		os_memcpy(&port->params.master_priority, &instance->params.gm_priority, sizeof(struct ptp_priority_vector));
		os_memcpy(&port->params.master_priority.u.s.source_port_identity.clock_identity, &instance->params.system_priority.u.s.source_port_identity.clock_identity, sizeof(struct ptp_clock_identity));
		port->params.master_priority.u.s.source_port_identity.port_number = htons(get_port_identity_number(port));
		port->params.master_priority.u.s.port_number = htons(get_port_identity_number(port));

		os_log(LOG_DEBUG, "(d) port %d - info_is %d\n", i, port->params.info_is);
		dump_priority_vector(&port->params.master_priority, instance->index, instance->domain.domain_number, "(d) master_priority", LOG_DEBUG);
	}

	/*
	e) Computes masterStepsRemoved, which is equal to:
	1) messageStepsRemoved (see 10.3.9.7) for the port associated with the gmPriorityVector,
	incremented by 1, if the gmPriorityVector is not the systemPriorityVector, or
	2) 0 if the gmPriorityVector is the systemPriorityVector,
	*/
	if (best_vector != &instance->params.system_priority)
		instance->params.master_steps_removed = instance->ports[best_vector_port].params.message_steps_removed + 1;
	else
		instance->params.master_steps_removed = 0;


	/*
	f) assigns the port role for port j and sets selectedRole[j] equal to this port role, as follows, for j = 1, 2,
	..., numberPorts:
	*/
	for (i = 0; i < instance->numberPorts; i++) {
		port = &instance->ports[i];

		/* per specs port->identity.port_number start at 1 for port 0 so we use it as our j index */
		j = get_port_identity_number(port);
		previous_port_role = instance->params.selected_role[j];

		os_log(LOG_DEBUG, "(f) port %d info_is %s\n", i, spanning_tree2string[port->params.info_is]);

		switch (port->params.info_is) {
		/*
		3) If the port is disabled (infoIs == Disabled), selectedRole[j] is set to DisabledPort.
		*/
		case SPANNING_TREE_DISABLED:
			instance->params.selected_role[j] = DISABLED_PORT;
			break;

		/*
		4) If announce receipt timeout, or sync receipt timeout with gmPresent set to TRUE, have
		occurred (infoIs = Aged), updtInfo is set to TRUE and selectedRole[j] is set to MasterPort.
		*/
		case SPANNING_TREE_AGED:
			port->params.updt_info = true;
			instance->params.selected_role[j] = MASTER_PORT;
			break;

		/*
		5) If the portPriorityVector was derived from another port on the time-aware system or from the
		time-aware system itself as the root (infoIs == Mine), selectedRole[j] is set to MasterPort. In
		addition, updtInfo is set to TRUE if the portPriorityVector differs from the
		masterPriorityVector or portStepsRemoved differs from masterStepsRemoved.
		*/
		case SPANNING_TREE_MINE:
			instance->params.selected_role[j] = MASTER_PORT;
			if(os_memcmp(&port->params.port_priority, &port->params.master_priority, sizeof(struct ptp_priority_vector)) ||
			(port->params.port_steps_removed != instance->params.master_steps_removed))
				port->params.updt_info = true;
			break;


		case SPANNING_TREE_RECEIVED:
			dump_priority_vector(&instance->params.gm_priority, instance->index, instance->domain.domain_number, "(f) gm_priority", LOG_DEBUG);
			dump_priority_vector(&port->params.port_priority, instance->index, instance->domain.domain_number, "(f) port_priority", LOG_DEBUG);
			/*
			6) If the portPriorityVector was received in an Announce message and announce receipt timeout,
			or sync receipt timeout with gmPresent TRUE, have not occurred (infoIs == Received), and the
			gmPriorityVector is now derived from the portPriorityVector, selectedRole[j] is set to SlavePort
			and updtInfo is set to FALSE.
			*/

			if (!compare_clock_identity(&instance->params.gm_priority.u.s.root_system_identity.u.s.clock_identity, &port->params.port_priority.u.s.root_system_identity.u.s.clock_identity)) {
				instance->params.selected_role[j] = SLAVE_PORT;
				port->params.updt_info = false;
				os_log(LOG_DEBUG, "(f) SLAVE_PORT updt_info = FALSE\n");
			}

			/*
			7) If the portPriorityVector was received in an Announce message and announce receipt timeout,
			or sync receipt timeout with gmPresent TRUE, have not occurred (infoIs == Received), the
			gmPriorityVector is not now derived from the portPriorityVector, the masterPriorityVector is
			not better than the portPriorityVector, and the sourcePortIdentity component of the
			portPriorityVector does not reflect another port on the time-aware system, selectedRole[j] is set
			to PassivePort and updtInfo is set to FALSE.

			8) If the portPriorityVector was received in an Announce message and announce receipt timeout,
			or sync receipt timeout with gmPresent TRUE, have not occurred (infoIs == Received), the
			gmPriorityVector is not now derived from the portPriorityVector, the masterPriorityVector is
			not better than the portPriorityVector, and the sourcePortIdentity component of the
			portPriorityVector does reflect another port on the time-aware system, selectedRole[j] set to
			PassivePort and updtInfo is set to FALSE.

			9) If the portPriorityVector was received in an Announce message and announce receipt timeout,
			or sync receipt timeout with gmPresent TRUE, have not occurred (infoIs == Received), the
			gmPriorityVector is not now derived from the portPriorityVector, and the masterPriorityVector
			is better than the portPriorityVector, selectedRole[j] set to MasterPort and updtInfo is set to
			TRUE.
			*/
			else {
				if (compare_system_identity(&port->params.master_priority, &port->params.port_priority) != BMCA_VECTOR_A_BETTER) {
					instance->params.selected_role[j] = PASSIVE_PORT;
					port->params.updt_info = false;
					os_log(LOG_DEBUG, "(f) PASSIVE_PORT updt_info = FALSE\n");
				}

				if (compare_system_identity(&port->params.master_priority, &port->params.port_priority) == BMCA_VECTOR_A_BETTER) {
					instance->params.selected_role[j] = MASTER_PORT;
					port->params.updt_info = true;
					os_log(LOG_DEBUG, "(f) MASTER_PORT updt_info = TRUE\n");
				}
			}
			break;
		}

		if (previous_port_role != instance->params.selected_role[j]) {
			os_log(LOG_INFO, "Port(%u): role changed from %s to %s (%s)\n", port->port_id, port_role2string[previous_port_role], port_role2string[instance->params.selected_role[j]], spanning_tree2string[port->params.info_is]);
		}
	}

	/*
	g) Updates gmPresent as follows:
	10) gmPresent is set to TRUE if the priority1 field of the rootSystemIdentity of the
	gmPriorityVector is less than 255.
	11) gmPresent is set to FALSE if the priority1 field of the rootSystemIdentity of the
	gmPriorityVector is equal to 255.
	*/
	if (instance->params.gm_priority.u.s.root_system_identity.u.s.priority_1 < 255)
		instance->params.gm_present = true;
	else
		instance->params.gm_present = false;

	/*
	h) Assigns the port role for port 0, and sets selectedRole[0], as follows:
	12) if selectedRole[j] is set to SlavePort for any port with portNumber j, j = 1, 2, ..., numberPorts,
	selectedRole[0] is set to PassivePort.
	13) if selectedRole[j] is not set to SlavePort for any port with portNumber j, j = 1, 2, ...,
	numberPorts, selectedRole[0] is set to SlavePort.
	*/
	instance->params.selected_role[0] = SLAVE_PORT;

	for (j = 1; j < instance->numberPorts + 1; j++) {
		if (instance->params.selected_role[j] == SLAVE_PORT) {
			instance->params.selected_role[0] = PASSIVE_PORT;
			break;
		}
	}

	/*
	i) If the clockIdentity member of the systemIdentity (see 10.3.2) member of gmPriority (see 10.3.8.19)
	is equal to thisClock (see 10.2.3.22), i.e., if the current time-aware system is the grandmaster, the
	pathTrace array is set to contain the single element thisClock (see 10.2.3.22).
	*/
	prev_is_grandmaster = instance->is_grandmaster;
	instance->is_grandmaster = false;

	if (!os_memcmp(&instance->params.gm_priority.u.s.root_system_identity.u.s.clock_identity, &instance->params.this_clock, sizeof(struct ptp_clock_identity))) {
		/* at start-up our vector is the only one so far but we claim ourselves as GM only if our priority1 is < 255 */
		if (instance->params.gm_present)
			instance->is_grandmaster = true;

		os_memset(&instance->params.path_trace[0], 0, MAX_PTLV_ENTRIES * sizeof(struct ptp_clock_identity));
		os_memcpy(&instance->params.path_trace[0], &instance->params.this_clock.identity[0], sizeof(struct ptp_clock_identity));
		instance->params.num_ptlv = 1;
		os_log(LOG_DEBUG, "(i) path_trace contains only this_clock, is_grandmaster = %d\n", instance->is_grandmaster);
	}

	/* Non standard */
	if (instance->is_grandmaster != prev_is_grandmaster)
		target_clkadj_system_role_change(&instance->target_clkadj_params, instance->is_grandmaster);

	if (instance->params.gm_present) {
		dump_priority_vector(best_vector, instance->index, instance->domain.domain_number, "best master", LOG_DEBUG);

		/*send notification to upper layer about the selected GM Id */
		gptp_gm_indication(instance, best_vector);
	}
}


static void port_state_selection_sm_init_bridge(struct gptp_instance *instance)
{
	updt_role_disabled_tree(instance);

	instance->port_state_sm_state = PORT_STATE_SM_STATE_INIT_BRIDGE;
}

static void port_state_selection_sm_selection(struct gptp_instance *instance)
{
	clear_reselect_tree(instance);

	updt_roles_tree(instance);

	set_selected_tree(instance);

	instance->port_state_sm_state = PORT_STATE_SM_STATE_SELECTION;
}


/** PortStateSelection - 802.1AS-2020 10.3.13
 * This state machine is used only if externalPortConfigurationEnabled is
 * FALSE (if this variable is TRUE, the PortStateSettingExt state machine is used instead). The state machine
 * updates the gmPathPriority vector for each PTP Port of the PTP Instance, the gmPriorityVector for the PTP
 * Instance, and the masterPriorityVector for each PTP Port of the PTP Instance. The state machine determines
 * the PTP Port state for each PTP Port and updates gmPresent
 */
void port_state_selection_sm(struct gptp_instance *instance)
{
	ptp_port_state_sm_state_t state;
	struct ptp_instance_params *params = &instance->params;
	int i;

	/*
	AutoCDSFunctionalSpecs-1_4 - 6.2.1.1
	In automotive profile, ports roles are statically defined
	*/
	if (instance->gptp->cfg.profile == CFG_GPTP_PROFILE_AUTOMOTIVE)
		return;

	if (params->begin || (!params->instance_enable)) {
		port_state_selection_sm_init_bridge(instance);
		/* Unconditional transition state. Next call to this state
		machine should end-up to SELECTION state (if none of
		the conditions above is verified) */
		return;
	}

start:
	state = instance->port_state_sm_state;

	switch(state) {
	case PORT_STATE_SM_STATE_SELECTION:
		for (i = 1; i < instance->numberPorts + 1; i++) {
			if (instance->params.reselect & (1 << i)) {
				port_state_selection_sm_selection(instance);
				break;
			}
		}
		break;

	case PORT_STATE_SM_STATE_INIT_BRIDGE:
		/* Unconditional Transferts */
		port_state_selection_sm_selection(instance);
		break;

	default:
		break;
	}

	os_log(LOG_DEBUG, "domain(%u, %u) state %s new state %s\n", instance->index, instance->domain.domain_number, port_state_sm_state_str[state], port_state_sm_state_str[instance->port_state_sm_state]);

	if (state != instance->port_state_sm_state)
		goto start;
}


static void site_sync_sync_sm_initializing(struct gptp_instance *instance)
{
	instance->site_sync.sync_sm.rcvdPSSync = false;

	instance->site_sync_sync_sm_state = SITE_SYNC_SYNC_SM_STATE_INITIALIZING;
}


/* setPSSyncSend - 10.2.6.2.1
 *
 *
 */
static struct port_sync_sync *set_pssync_send(struct gptp_instance *instance, struct port_sync_sync *rx_pss)
{
	os_memcpy(&instance->site_sync.pssync, rx_pss, sizeof(struct port_sync_sync));

	return &instance->site_sync.pssync;
}

/* txPSSync - 10.2.6.2.2
 * transmits a copy of the PortSyncSync structure pointed to by
 * txPSSyncPtr to the PortSyncSyncSend state machine of each PortSync entity and the ClockSlaveSync state
 * machine of the ClockSlave entity of this time-aware system.
 *
 */
static void tx_pssync(struct gptp_instance *instance)
{
	int i;

	/*
	* If the information was sent by a PortSync entity the state machine also receives the portIdentity of
	* the port on the upstream time-aware system that sent the information to this time-aware system
	* (if the information was sent by the ClockMaster entity, this portIdentity is zero)
	*/
	for (i = 0; i < instance->numberPorts; i++) {
		os_memcpy(&instance->ports[i].port_sync.sync, instance->site_sync.sync_sm.txPSSyncPtr, sizeof(struct port_sync_sync));
		instance->ports[i].port_sync.sync_send_sm.rcvd_pssync_psss = true;
		instance->ports[i].port_sync.sync_send_sm.rcvd_pssync_ptr = &instance->ports[i].port_sync.sync;
		port_sync_sync_send_sm(&instance->ports[i], PORT_SYNC_SYNC_SEND_SM_EVENT_RUN);
	}

	if(instance->site_sync.sync_sm.txPSSyncPtr->localPortNumber != 0) {
		os_memcpy(&instance->clock_slave.pssync, instance->site_sync.sync_sm.txPSSyncPtr, sizeof(struct port_sync_sync));
		instance->clock_slave.rcvdPSSync = true;
		instance->clock_slave.rcvdPSSyncPtr = &instance->clock_slave.pssync;
		clock_slave_sync_sm(&instance->ports[instance->site_sync.sync_sm.txPSSyncPtr->localPortNumber - 1]);
	}
}

static void site_sync_sync_sm_receiving_sync(struct gptp_instance *instance)
{
	instance->site_sync.sync_sm.rcvdPSSync = false;
	instance->site_sync.sync_sm.txPSSyncPtr = set_pssync_send(instance, instance->site_sync.sync_sm.rcvdPSSyncPtr);
	tx_pssync(instance);

	instance->params.parent_log_sync_interval = instance->site_sync.sync_sm.txPSSyncPtr->logMessageInterval;

	instance->site_sync_sync_sm_state = SITE_SYNC_SYNC_SM_STATE_RECEIVING_SYNC;
}


/** SiteSyncSyncSM - 802.1AS - 10.2.6
 * The state  machine receives time-synchronization information, accumulated rateRatio, and syncReceiptTimeoutTime
 * from the PortSync entity (PortSyncSyncReceive state machine) of the current slave port or from the
 * ClockMaster entity (ClockMasterSyncSend state machine). If the information was sent by a PortSync entity
 * the state machine also receives the portIdentity of the port on the upstream time-aware system that sent the
 * information to this time-aware system (if the information was sent by the ClockMaster entity, this
 * portIdentity is zero). The state machine sends a PortSyncSync structure to the PortSync entities of all the
 * ports and to the ClockSlave entity
 */
int site_sync_sync_sm(struct gptp_instance *instance)
{
	ptp_site_sync_sync_sm_state_t state = instance->site_sync_sync_sm_state;
	struct ptp_instance_params *params = &instance->params;
	int rc = 0;

	if (params->begin || (!params->instance_enable)) {
		site_sync_sync_sm_initializing(instance);
		goto exit;
	}

	switch (state) {
	case SITE_SYNC_SYNC_SM_STATE_INITIALIZING:
	case SITE_SYNC_SYNC_SM_STATE_RECEIVING_SYNC:
		if ((instance->site_sync.sync_sm.rcvdPSSync) &&
		(params->selected_role[instance->site_sync.sync_sm.rcvdPSSyncPtr->localPortNumber] == SLAVE_PORT) &&
		(params->gm_present))
			site_sync_sync_sm_receiving_sync(instance);
		break;

	default:
		break;
	}

exit:
	os_log(LOG_DEBUG, "Local Port(%d) domain(%u, %u) state %s new state %s\n",
		((instance->site_sync.sync_sm.rcvdPSSync) ? instance->site_sync.sync_sm.rcvdPSSyncPtr->localPortNumber : -1),
		instance->index, instance->domain.domain_number,
		site_sync_sync_sm_state_str[state],
		site_sync_sync_sm_state_str[instance->site_sync_sync_sm_state]);

	return rc;
}
