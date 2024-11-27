/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file		ptp.h
 @brief	PTP protocol common definitions
 @details	PDU and protocol definitions for all PTP applications
*/

#ifndef _PTP_H_
#define _PTP_H_

#include "genavb/ptp.h"
#include "common/types.h"
#include "common/filter.h"
#include "common/timer.h"

#include "gptp/config.h"

#define PTP_MAXFREQ_PPB	200000 /* Maximum allowed clock frequency offset per 802.1AS, in parts per billion (100ppm from TAI, so 200ppm between 2 clocks) */


/*
* PTP Version (802.1AS-2020 10.6.2.2.3, 10.6.2.2.4)
*/
#define PTP_MINOR_VERSION 1
#define PTP_VERSION 2

/*
* PTP domain (802.1AS-2020 -8.1)
*/
#define PTP_DOMAIN_0 0
#define PTP_DOMAIN_NUMBER_MAX	127
#define PTP_DOMAIN_MINOR_SDOID	0x00
#define PTP_DOMAIN_MAJOR_SDOID	0x1

/*
* CMLDS
*/
#define PTP_CMLDS_MINOR_SDOID 0x00
#define PTP_CMLDS_MAJOR_SDOID 0x2

struct __attribute ((packed)) ptp_domain {
	u8 domain_number;
	union {
		u16 sdo_id;
		struct __attribute__ ((packed)) {
#ifdef __BIG_ENDIAN__
			u16 minor_sdo_id:8;
			u16 major_sdo_id:4;
			u16 rsvd:4;
#else
			u16 rsvd:4;
			u16 major_sdo_id:4;
			u16 minor_sdo_id:8;
#endif
		}s;
	}u;
};


/*
* GM Clock Quality (802.1AS - 10.5.3.2.3)
*/
struct __attribute__ ((packed)) ptp_clock_quality {
	u8 clock_class;
	u8 clock_accuracy;
	u16 offset_scaled_log_variance;
};

/*
*  System identity (802.1AS - 10.3.2)
*
* The systemIdentity attribute is defined for convenience when comparing two time-aware systems to
* determine which is a better candidate for root and if the time-aware system is grandmaster-capable (i.e., the
* value of priority1 is less than 255, see 8.6.2.1)
*/
struct __attribute__ ((packed)) ptp_system_identity {
	union {
		u8 system_identity[14];
		struct __attribute__ ((packed)) {
			u8 priority_1;
			struct ptp_clock_quality clock_quality;
			u8 priority_2;
			struct ptp_clock_identity clock_identity;
		}s;
	}u;
};


/*
* Port time-synchronization spanning tree priority vectors (802.1AS - 10.3.4)
*
* Time-aware systems send best master selection information to each other in announce messages.
* The priority vector is the basis for a concise specification in the BMCA's determination of the time
* synchronization spanning tree and grandmaster
*
* Big-endian
*/
struct __attribute__ ((packed)) ptp_priority_vector {
	union {
		u8 priority[28];
		struct __attribute__ ((packed)) {
			struct ptp_system_identity root_system_identity;
			u16 steps_removed;
			struct ptp_port_identity source_port_identity;
			u16 port_number;
		}s;
	}u;
};



/*
* Precise Timestamp (802.1AS - )
*/
struct __attribute__ ((packed)) ptp_timestamp {
	u16 seconds_msb;
	u32 seconds_lsb;
	u32 nanoseconds;
};

struct __attribute__ ((packed)) ptp_extended_timestamp {
	u16 seconds_msb;
	u32 seconds_lsb;
	u32 fractional_nanoseconds_msb;
	u16 fractional_nanoseconds_lsb;
};

/* 802.1AS - 6.3.1 */
struct __attribute__ ((packed)) ptp_scaled_ns {
	union {
		u8 scaled_nanoseconds[12];
		struct __attribute__ ((packed)) {
			/* Keep all values has unsigned and do unsigned math */
			u16 nanoseconds_msb;			/* Should remain 0 for about 500 years (2**64/NSECS_PER_SEC/SECS_PER_DAY/DAYS_PER_YEAR = 584), so will be ignored in all computations */
			u64 nanoseconds;
			u16 fractional_nanoseconds;
		} s;
	} u;
};

/* 802.1AS - 6.3.2 */
struct __attribute__ ((packed)) ptp_u_scaled_ns {
	union {
		u8 u_scaled_nanoseconds[12];
		struct __attribute__ ((packed)) {
			u16 nanoseconds_msb;			/* Should remain 0 for about 500 years (2**64/NSECS_PER_SEC/SECS_PER_DAY/DAYS_PER_YEAR = 584), so will be ignored in all computations */
			u64 nanoseconds;
			u16 fractional_nanoseconds;
		} s;
	} u;
};


/* 802.1AS - 6.3.3 */
struct __attribute__ ((packed)) ptp_time_interval {
	s64 scaled_nanoseconds;
};

typedef double ptp_double;


/*
* TLV types (1588-2008 - Table 34)
*/

struct __attribute__ ((packed)) ptp_tlv_header {
	u16 tlv_type;
	u16 length_field;
};

/* TLV types */
#define PTP_TLV_TYPE_MANAGEMENT					0x0001
#define PTP_TLV_TYPE_MANAGEMENT_ERROR_STATUS			0x0002
#define PTP_TLV_TYPE_ORGANIZATION_EXTENSION			0x0003
#define PTP_TLV_TYPE_REQUEST_UNICAST_TRANSMISSION		0x0004
#define PTP_TLV_TYPE_GRANT_UNICAST_TRANSMISSION			0x0005
#define PTP_TLV_TYPE_CANCEL_UNICAST_TRANSMISSION		0x0006
#define PTP_TLV_TYPE_ACKNOWLEDGE_CANCEL_UNICAST_TRANSMISSION	0x0007
#define PTP_TLV_TYPE_PATH_TRACE					0x0008
#define PTP_TLV_TYPE_ALTERNATE_TIME_OFFSET_INDICATOR		0x0009
#define PTP_TLV_TYPE_AUTHENTICATION				0x2000
#define PTP_TLV_TYPE_AUTHENTICATION_CHALLENGE			0x2001
#define PTP_TLV_TYPE_SECURITY_ASSOCIATION_UPDATE		0x2002
#define PTP_TLV_TYPE_CUM_FREQ_SCALE_FACTOR_OFFSET		0x2003
#define PTP_TLV_TYPE_ORGANIZATION_EXTENSION_DO_NOT_PROPAGATE		0x8000

/* IEEE 802.1AS-2020 - sections 10.6.4.3.5, 10.6.4.4.5 and 10.6.4.5.5 */
#define PTP_TLV_SUBTYPE_INTERVAL_REQUEST		2
#define PTP_TLV_SUBTYPE_GPTP_CAPABLE_MESSAGE	4
#define PTP_TLV_SUBTYPE_GPTP_CAPABLE_INTERVAL	5


/*
* Path Trace TLV (802.1AS - Table 10.8, 10.5.3.2.8)
*/

/* FIXME, there shouldn't be such limitation to the number of path_sequence entries */
#define MAX_PTLV_ENTRIES 16

struct __attribute__ ((packed)) ptp_path_tlv {
	struct ptp_tlv_header header;
	struct ptp_clock_identity path_sequence[MAX_PTLV_ENTRIES];
};

/*
* Follow Up TLV
*/
struct __attribute__ ((packed)) ptp_follow_up_tlv {
	u16 tlv_type;
	u16 length_field;
	u8 organization_id[3];
	u8 organization_sub_type[3];
	s32 cumulative_scaled_rate_offset;
	u16 gm_time_base_indicator;
	struct ptp_scaled_ns last_gm_phase_change;
	s32 scaled_last_gm_freq_change;
};



/*
* Message Interval request TLV (802.1AS-2020 - Table 10.14)
*/
struct __attribute__ ((packed)) ptp_interval_tlv {
	s8 link_delay_interval;
	s8 time_sync_interval;
	s8 announce_interval;
	u8 flags;
	u16 reserved;
};

/* Table 10-14 - Definitions of bits of flags field of message interval request TLV */
#define ITLV_FLAGS_COMPUTE_RATIO_MASK	(0x01)
#define ITLV_FLAGS_COMPUTE_DELAY_MASK	(0x02)


/*
* gPTP Capable TLV (802.1AS-2020 - Table 10.19)
*/
struct __attribute__ ((packed)) ptp_gptp_capable_tlv {
	s8 log_gptp_capable_message_interval;
	u8 flags;
	u32 reserved;
};


/*
* Announce Message (802.1AS - Table 10.7)
*/
struct __attribute__ ((packed)) ptp_announce_pdu {
	struct ptp_hdr header;
	u8 reserved1[10];
	u16 current_utc_offset;
	u8 reserved2;
	u8 grandmaster_priority1;
	struct ptp_clock_quality grandmaster_clock_quality;
	u8 grandmaster_priority2;
	struct ptp_clock_identity grandmaster_identity;
	u16 steps_removed;
	u8 time_source;
	struct ptp_path_tlv ptlv;
};


/*
* Signaling Message (802.1AS - 10.5.4 Table 10.9)
*/
struct __attribute__ ((packed)) ptp_signaling_pdu {
	struct ptp_hdr header;
	struct ptp_port_identity target_port_identity; /* default to 0xFF */
	u16 tlv_type;
	u16 length_field;
	u8 organization_id[3];
	u8 organization_sub_type[3];
	union {
		struct ptp_interval_tlv itlv;
		struct ptp_gptp_capable_tlv ctlv;
	} u;
};

/*
* Sync Message (802.1AS -)
*/
struct __attribute__ ((packed)) ptp_sync_pdu {
	struct ptp_hdr header;
	struct ptp_timestamp timestamp;
};


/*
* follow-up Message (802.1AS -)
*/
struct __attribute__ ((packed)) ptp_follow_up_pdu {
	struct ptp_hdr header;
	struct ptp_timestamp precise_origin_timestamp;
	struct ptp_follow_up_tlv tlv;
};

/*
* PDelay request Message (802.1AS -)
*/
struct __attribute__ ((packed)) ptp_pdelay_req_pdu {
	struct ptp_hdr header;
	u8 reserved[20];
};

/*
* PDelay response Message (802.1AS -)
*/
struct __attribute__ ((packed)) ptp_pdelay_resp_pdu {
	struct ptp_hdr header;
	struct ptp_timestamp request_receipt_timestamp;
	struct ptp_port_identity requesting_port_identity;
};


/*
* Pdelay response follow-up Message (802.1AS -)
*/
struct __attribute__ ((packed)) ptp_pdelay_resp_follow_up_pdu {
	struct ptp_hdr header;
	struct ptp_timestamp response_origin_timestamp;
	struct ptp_port_identity requesting_port_identity;
};


/*
* Port Roles (802.1AS - Table 10.1 and Table 14.5) */
typedef enum {
	DISABLED_PORT = 3,	/** any port of the time-aware system for which portEnabled, ptpPortEnabled, and asCapable are not all TRUE */
	MASTER_PORT = 6,	/** any port, P, of the time aware system that is closer than any other port of the gptp communication path connected to P */
	PASSIVE_PORT = 7,	/** any port of the time-aware system whose port role is not MasterPort, SlavePort or DisabledPort */
	SLAVE_PORT = 9		/** the one port of the time-aware system that is closest to the root time-aware system. Does not transmit announce or sync messages */
}ptp_port_role_t;


/*
* Time Sources (802.1AS - 8.6.2.7)
*/
typedef enum {
	TIME_SOURCE_ATOMIC_CLOCK = 0x10,
	TIME_SOURCE_GPS = 0x20,
	TIME_SOURCE_TERRESTRIAL_RADIO = 0x30,
	TIME_SOURCE_PTP = 0x40,
	TIME_SOURCE_NTP = 0x50,
	TIME_SOURCE_HAND_SET = 0x60,
	TIME_SOURCE_OTHER = 0x90,
	TIME_SOURCE_INTERNAL_OSCILLATOR = 0xA0
}ptp_time_source_t;


/*
* Spanning Tree Information (802.1AS - 10.3.9.2)
*/
typedef enum {
	SPANNING_TREE_RECEIVED = 0,
	SPANNING_TREE_MINE,
	SPANNING_TREE_AGED,
	SPANNING_TREE_DISABLED
}ptp_spanning_tree_info_t;


/*
* Values for message_type field (802.1AS - Table 11-3)
*
* Media dependent, full duplex point to point
*/
#define PTP_MSG_TYPE_SYNC		0x0
#define PTP_MSG_TYPE_DELAY_REQ		0x1
#define PTP_MSG_TYPE_PDELAY_REQ		0x2
#define PTP_MSG_TYPE_PDELAY_RESP	0x3
#define PTP_MSG_TYPE_FOLLOW_UP		0x8
#define PTP_MSG_TYPE_DELAY_RESP		0x9
#define PTP_MSG_TYPE_PDELAY_RESP_FUP	0xA

/*
* Values for message_type field (802.1AS - Table 10-5)
*
* Media independent layer
*/

#define PTP_MSG_TYPE_ANNOUNCE		0xB
#define PTP_MSG_TYPE_SIGNALING		0xC
#define PTP_MSG_TYPE_MANAGEMENT		0xD


/*
* Flags bits definitions (802.1AS Cor1 2013 - Table 10.6)
*/

//FIXME for now using 802.1AS-2011 octet 1 definition for backward compatibility
#if 0
/* octet 1 */
#define PTP_FLAG_LEAP_61		(1<<1)
#define PTP_FLAG_LEAP_59		(1<<2)
#define PTP_FLAG_CURRENT_UTC_OFF_VALID	(1<<3)
#define PTP_FLAG_PTP_TIMESCALE		(1<<4)
#define PTP_FLAG_TIME_TRACEABLE		(1<<5)
#define PTP_FLAG_FREQUENCY_TRACEABLE	(1<<6)
#else
/* octet 1 */
#define PTP_FLAG_LEAP_61		(1<<0)
#define PTP_FLAG_LEAP_59		(1<<1)
#define PTP_FLAG_CURRENT_UTC_OFF_VALID	(1<<2)
#define PTP_FLAG_PTP_TIMESCALE		(1<<3)
#define PTP_FLAG_TIME_TRACEABLE		(1<<4)
#define PTP_FLAG_FREQUENCY_TRACEABLE	(1<<5)
#endif
/* octect 0 */
#define PTP_FLAG_ALTERNATE	(1<<0) /* defined in 1588 only */
#define PTP_FLAG_TWO_STEP	(1<<1) /* defined in 1588 only */
#define PTP_FLAG_UNICAST	(1<<2)
#define PTP_FLAG_PROFILE1	(1<<5) /* defined in 1588 only */
#define PTP_FLAG_PROFILE2	(1<<6) /* defined in 1588 only */
#define PTP_FLAG_SECURITY	(1<<7) /* defined in 1588 only */


/*
* Values for control field (802.1AS - Table 11-7)
*/
#define PTP_CONTROL_SYNC 		0x0
#define PTP_CONTROL_FOLLOW_UP_2011 		0x2
#define PTP_CONTROL_PDELAY_REQ_2011 		0x5
#define PTP_CONTROL_PDELAY_RESP_2011 		0x5
#define PTP_CONTROL_PDELAY_RESP_FUP_2011 	0x5
#define PTP_CONTROL_ANNOUNCE_2011 	0x5

/*
* Values for control field (802.1AS -2020 - 10.6.2.2.13
*/
#define PTP_CONTROL_FOLLOW_UP		0x0
#define PTP_CONTROL_PDELAY_REQ		0x0
#define PTP_CONTROL_PDELAY_RESP	0x0
#define PTP_CONTROL_PDELAY_RESP_FUP	0x0
#define PTP_CONTROL_ANNOUNCE		0x0

/*
* Values for log message interval field (802.1AS - 11.4.2.8)
*/
#define PTP_LOG_MSG_PDELAY_RESP		0x7F
#define PTP_LOG_MSG_PDELAY_RESP_FUP	0x7F
#define PTP_LOG_MSG_SIGNALING		0x7F


/*
* Delay mechanism (802.1AS-2020 - Table 14.8)
*/
typedef enum {
	P2P = 2, /* The port uses instance-specific peer-top-peer delay mechanism*/
	COMMON_P2P = 3, /* The port uses CMLDS */
	SPECIAL = 4 /*The port uses transport with native time transfer mechanism. No peer-to-peer delay meachism*/
} ptp_delay_mechanism_t;

/*
* Per PTP-Port or Per Link-Port global variables. Used by the
* PdelayReq and PdelayResp state machines.
*/
struct ptp_port_params {
	/*
	* 802.1AS-2020 - Table 10.1
	*/
	u8 begin;
	ptp_double neighbor_rate_ratio;
	struct ptp_u_scaled_ns mean_link_delay;
	struct ptp_scaled_ns delay_asymmetry;
	bool compute_neighbor_rate_ratio;
	bool current_compute_neighbor_rate_ratio;
	bool initial_compute_neighbor_rate_ratio;
	bool compute_mean_link_delay;
	bool current_compute_mean_link_delay;
	bool initial_compute_mean_link_delay;
	bool port_oper; /* 802.1.AS-2020 - 10.2.5.12 a Boolean that is set if the time-aware system's MAC is operational (Tx/Rx frames, Spanning Tree, Relay) */
	u16 this_port; /* FIXME port number not used since duplicated by port->port_id */
	bool as_capable_across_domains;
};

/*
* Per port global variables
*/
struct ptp_instance_port_params {
	/*
	* 802.1AS-2020 - Table 10.1
	* Global variables used by time synchronization state machines.
	*/
	bool ptp_port_enabled; /* 802.1AS - 10.2.4.12 a Boolean that is set if the time-synchronization and bmca functions of the port are enabled */
	bool as_capable; /* TRUE = pdelay-req-resp from peer, FALSE = ptp link not yet up */
	struct ptp_u_scaled_ns sync_receipt_timeout_time_interval;
	s8 current_log_sync_interval;
	s8 initial_log_sync_interval;
	struct ptp_u_scaled_ns sync_interval;

	u16 this_port; /* port number not used since duplicated by port->port_id */
	bool sync_locked;
	bool neighbor_gptp_capable;
	bool sync_slow_down;
	struct ptp_u_scaled_ns old_sync_interval;
	bool gptp_capable_message_slow_down;
	struct ptp_u_scaled_ns gptp_capable_message_interval;
	struct ptp_u_scaled_ns old_gptp_capable_message_interval;
	s8 initial_log_gptp_capable_message_interval;
	s8 current_log_gptp_capable_message_interval;

	/*Avnu AutoCDSFunctionalSpec-1_4 - 6.2.1.5 / 6.2.1.6 */
	s8 oper_log_pdelay_req_interval; /* A device moves to this value on all slave ports once the measured values have stabilized*/
	s8 oper_log_sync_interval; /*operLogSyncInterval is the Sync interval that a device moves to and signals on a slave port once it has achieved synchronization*/

	int rate_ratio;

	/* 802.1AS-2020 - Table 10.3
	* Global variables used by the best master clock selection, external port
	* configuration, and announce interval setting state machines.
	*/
	struct ptp_u_scaled_ns announce_receipt_timeout_time_interval;
	bool announce_slow_down;
	struct ptp_u_scaled_ns old_announce_interval;
	ptp_spanning_tree_info_t info_is;
	struct ptp_priority_vector master_priority;
	s8 current_log_announce_interval;
	s8 initial_log_announce_interval;
	struct ptp_u_scaled_ns announce_interval;
	u16 message_steps_removed;
	bool new_info;
	struct ptp_priority_vector port_priority;
	u16 port_steps_removed;
	struct ptp_announce_pdu *rcvd_announce_ptr;
	bool rcvd_msg;
	bool updt_info;
	u8 ann_leap61;
	u8 ann_leap59;
	u8 ann_current_utc_offset_valid;
	bool ann_ptp_timescale;
	u8 ann_time_traceable;
	u8 ann_frequency_traceable;
	u16 ann_current_utc_offset;
	ptp_time_source_t ann_time_source;
	struct ptp_clock_identity received_path_trace[MAX_PTLV_ENTRIES];

	/* 802.1AS-2020 - 14.8.5 */
	ptp_delay_mechanism_t delay_mechanism;
};


/* 802.1AS-2020 - section 10.7.3 */
#define SYNC_RECEIPT_TIMEOUT		3
#define ANNOUNCE_RECEIPT_TIMEOUT	3
#define GPTP_CAPABLE_RECEIPT_TIMEOUT	9

/*
* Per instance global variables
*/
struct ptp_instance_params {
	/* 802.1AS-2020 - Table 10.1
	* Global variables used by time synchronization state machines.
	*/
	u8 begin;
	struct ptp_u_scaled_ns clock_master_sync_interval;
	struct ptp_extended_timestamp clock_slave_time;
	struct ptp_extended_timestamp sync_receipt_time;
	struct ptp_u_scaled_ns sync_receipt_local_time;
	ptp_double clock_source_freq_offset;
	struct ptp_scaled_ns clock_source_phase_offset;
	u16 clock_source_time_base_indicator;
	u16 clock_source_time_base_indicator_old;
	struct ptp_scaled_ns clock_source_last_gm_phase_change;
	ptp_double clock_source_last_gm_freq_change;
	struct ptp_u_scaled_ns current_time;
	bool gm_present;
	ptp_double gm_rate_ratio;
	u16 gm_time_base_indicator;
	struct ptp_scaled_ns last_gm_phase_change;
	ptp_double last_gm_freq_change;
	struct ptp_time_interval local_clock_tick_interval;
	struct ptp_u_scaled_ns local_time;
	ptp_port_role_t selected_role[CFG_GPTP_MAX_NUM_PORT + 1]; /* 10.2.3.20 */
	struct ptp_extended_timestamp master_time;
	struct ptp_clock_identity this_clock;
	s8 parent_log_sync_interval;
	bool instance_enable;
	struct ptp_u_scaled_ns sync_receipt_timeout_time;

	/* 802.1AS-2020 - Table 10.3
	* Global variables used by the best master clock selection, external port
	* configuration, and announce interval setting state machines.
	*/
	u16 reselect;
	u16 selected;
	u16 master_steps_removed;
	u8 leap61;
	u8 leap59;
	u8 current_utc_offset_valid;
	bool ptp_timescale;
	u8 time_traceable;
	u8 frequency_traceable;
	s16 current_utc_offset;
	ptp_time_source_t time_source;
	u8 sys_leap61;
	u8 sys_leap59;
	u8 sys_current_utc_offset_valid;
	bool sys_ptp_timescale;
	u8 sys_time_traceable;
	u8 sys_frequency_traceable;
	s16 sys_current_utc_offset;
	ptp_time_source_t sys_time_source;
	struct ptp_priority_vector system_priority;
	struct ptp_priority_vector gm_priority;
	struct ptp_priority_vector last_gm_priority;
	struct ptp_clock_identity path_trace[MAX_PTLV_ENTRIES];
	bool external_port_configuration_enabled;
	u16 last_announce_report;

	/* Non standard */
	u8 do_clock_adjust;
	u16 num_ptlv;
};


/*
* 802.1AS Entities definitions
*/


/* 10.2.2.3.1
When sent from the PortSync or ClockMaster entity, it provides the SiteSync entity with master clock timing
information, timestamp of receipt of a time-synchronization event message compensated for propagation
time on the upstream link, and the time at which sync receipt timeout occurs if a subsequent Sync message is
not received by then. The information is used by the SiteSync entity to compute the rate ratio of the local
oscillator relative to the master and is communicated to the other PortSync entities for use in computing
master clock timing information.
When sent from the SiteSync entity to the PortSync or ClockMaster entity, the structure contains
information needed to compute the synchronization information that will be included in respective fields of
the time-synchronization event and general messages that will be sent, and also to compute the synchronized
time that the ClockSlave entity will supply to the ClockTarget entity.
*/
struct port_sync_sync
{
	/* IEEE 802.1AS-2020 section 10.2.2.1.2
	This parameter is the domain number of the gPTP domain in which this structure is sent.
	NOTE: The domain number member is not essential because the state machines that send and receive this structure are
	per domain, and each state machine implicitly knows the number of the domain in which it operates
	u8 domainNumber;
	*/
	u16 localPortNumber;
	struct ptp_u_scaled_ns syncReceiptTimeoutTime;
	struct ptp_scaled_ns followUpCorrectionField;
	struct ptp_port_identity sourcePortIdentity;
	s8 logMessageInterval;
	struct ptp_timestamp preciseOriginTimestamp;
	struct ptp_u_scaled_ns upstreamTxTime;
	ptp_double rateRatio;
	u16 gmTimeBaseIndicator;
	struct ptp_scaled_ns lastGmPhaseChange;
	ptp_double lastGmFreqChange;
};

/* 9.2.1
This interface is used by the ClockSource entity to provide time to the ClockMaster entity of a time-aware
system. The ClockSource entity invokes the ClockSourceTime.invoke function to provide the time, relative
to the ClockSource, that this function is invoked.
9.2.2 */
struct clock_source_time_invoke {
	struct ptp_extended_timestamp source_time;
	u16 time_base_indicator;
	struct ptp_scaled_ns last_gm_phase_change;
	ptp_double last_gm_freq_change;
};

/***************** ClockSlave entity *********************/


/*
The ClockSlave entity receives grandmaster time-synchronization and
current grandmaster information from the SiteSync entity, and makes the
information available to an external application, referred to as a clockTarget
entity (see 9.3 through 9.6), via one or more application service interfaces
*/
struct ptp_clock_slave_entity {
	struct port_sync_sync pssync;

	/*
	10.2.12.1.1 a Boolean variable that notifies the current state machine when a PortSyncSync
	structure is received from the SiteSyncSync state machine of the SiteSync entity. This variable is reset by
	this state machine.
	*/
	bool rcvdPSSync;

	/*
	10.2.12.1.2 a Boolean variable that notifies the current state machine when the
	LocalClock entity updates its time. This variable is reset by this state machine.
	*/
	bool rcvdLocalClockTick;

	/*
	10.2.12.1.3 a pointer to the received PortSyncSync structure
	*/
	struct port_sync_sync *rcvdPSSyncPtr;

	/*
	 * Non standard, GM clock identity of the previous sync received
	 */
	struct ptp_clock_identity prev_gm_clock_identity;
};

typedef enum {
	CLOCK_SLAVE_SYNC_SM_STATE_INITIALIZING = 0,
	CLOCK_SLAVE_SYNC_SM_STATE_SEND_SYNC_INDICATION
}ptp_clock_slave_sync_sm_state_t;


/***************** ClockMaster entity *********************/


/*
ClockMasterSyncSend (one instance per time-aware system): receives masterTime from the
ClockMasterSyncReceive state machine, receives phase and frequency offset between masterTime
and syncReceiptTime from the ClockMasterSyncOffset state machine, and provides masterTime
(i.e., synchronized time) and the phase and frequency offset to the SiteSync entity using a
PortSyncSync structure. This state machine is optional for time-aware systems that are not
grandmaster-capable (see 8.6.2.1 and 10.1.2).
*/
struct clock_master_sync_send_sm {
	/*
	the time in seconds, relative to the LocalClock entity, when synchronization
	information will next be sent to the SiteSync entity, via a PortSyncSync structure. The data type for
	syncSendTime is UScaledNs.
	*/
	struct ptp_u_scaled_ns syncSendTime;

	/*
	a pointer to the PortSyncSync structure transmitted by the state machine.
	*/
	struct port_sync_sync *txPSSyncPtr;
};

/*
ClockMasterSyncReceive (one instance per time-aware system): receives ClockSourceTime.invoke
functions from the ClockSource entity and notifications of LocalClock entity ticks (see 10.2.3.18),
updates masterTime, and provides masterTime to ClockMasterSyncOffset and
ClockMasterSyncSend state machines. This state machine is optional for time-aware systems that
are not grandmaster-capable (see 8.6.2.1 and 10.1.2).
*/
struct clock_master_sync_receive_sm {
	/*
	a Boolean variable that notifies the current state machine when
	ClockSourceTime.invoke function is received from the Clock source entity. This variable is reset by this
	state machine.
	*/
	bool rcvd_clock_source_req;

	/*
	a pointer to the received ClockSourceTime.invoke function
	parameters.
	*/
	struct clock_source_time_invoke *rcvd_clock_source_req_ptr;

	/*
	a Boolean variable that notifies the current state machine when the
	LocalClock entity updates its time. This variable is reset by this state machine.
	*/
	bool rcvd_local_clock_tick;
};


/*
ClockMasterSyncOffset (one instance per time-aware system): receives syncReceiptTime from the
ClockSlave entity and masterTime from the ClockMasterSyncReceive state machine, computes
phase offset and frequency offset between masterTime and syncReceiptTime if the time-aware
system is not the grandmaster, and provides the frequency and phase offsets to the
ClockMasterSyncSend state machine. This state machine is optional for time-aware systems that are
not grandmaster-capable (see 8.6.2.1 and 10.1.2).
*/
struct clock_master_sync_offset_sm {
	/*
	a Boolean variable that notifies the current state machine when
	syncReceiptTime is received from the ClockSlave entity. This variable is reset by this state machine.
	*/
	bool rcvd_sync_receipt_time;
};


/*
The ClockMaster entity receives information from an external time
source, referred to as a ClockSource entity (see 9.2), via an application interface, and provides the
information to the SiteSync entity
*/
struct ptp_clock_master_entity {
	struct port_sync_sync pssync;
	struct clock_master_sync_send_sm sync_send_sm;
	struct clock_master_sync_receive_sm sync_receive_sm;
	struct clock_master_sync_offset_sm sync_offset_sm;

	struct ptp_u_scaled_ns sync_receipt_time_prev;
	struct ptp_u_scaled_ns sync_receipt_local_time_prev;
	bool clock_source_freq_offset_init;
	ptp_double ratio_average;
};


typedef enum {
	CLOCK_MASTER_SYNC_SEND_SM_STATE_INITIALIZING = 0,
	CLOCK_MASTER_SYNC_SEND_SM_STATE_SEND_SYNC_INDICATION
}ptp_clock_master_sync_send_sm_state_t;

typedef enum {
	CLOCK_MASTER_SYNC_SEND_SM_EVENT_INTERVAL = 0,
	CLOCK_MASTER_SYNC_SEND_SM_EVENT_RUN
}ptp_clock_master_sync_send_sm_event_t;

typedef enum {
	CLOCK_MASTER_SYNC_OFFSET_SM_STATE_INITIALIZING = 0,
	CLOCK_MASTER_SYNC_OFFSET_SM_STATE_RECEIVED_SYNC_RECEIPT_TIME
}ptp_clock_master_sync_offset_sm_state_t;

typedef enum {
	CLOCK_MASTER_SYNC_RECEIVE_SM_STATE_INITIALIZING = 0,
	CLOCK_MASTER_SYNC_RECEIVE_SM_STATE_WAITING,
	CLOCK_MASTER_SYNC_RECEIVE_SM_STATE_RECEIVE_SOURCE_TIME
}ptp_clock_master_sync_receive_sm_state_t;

/***************** PortSync entity *********************/



/* PortSyncsyncReceiveSM variables 10.2.7.1
*
*/
struct port_sync_sync_receive_sm
{
	/*
	a Boolean variable that notifies the current state machine when an
	MDSyncReceive structure is received from the MDSyncReceiveSM state machine of an MD entity of the
	same port (see 10.2.2.1). This variable is reset by this state machine.
	*/
	bool rcvdMDSync;

	/*
	a pointer to the received MDSyncReceive structure indicated by rcvdMDSync.
	*/
	struct md_sync_receive *rcvdMDSyncPtr;

	/*
	a pointer to the PortSyncSync structure transmitted by the state machine.
	*/
	struct port_sync_sync *txPSSyncPtr;

	/*
	a Double variable that holds the ratio of the frequency of the grandmaster to the
	frequency of the LocalClock entity. This frequency ratio is computed by (a) measuring the ratio of the
	grandmaster frequency to the LocalClock frequency at the grandmaster time-aware system and initializing
	rateRatio to this value in the ClockMasterSend state machine of the grandmaster node, and (b)
	accumulating, in the PortSyncSyncReceive state machine of each time-aware system, the frequency offset of
	the LocalClock entity of the time-aware system at the remote end of the link attached to that port to the
	frequency of the LocalClock entity of this time-aware system.
	*/
	ptp_double rateRatio;
};


typedef enum {
	PORT_SYNC_SYNC_SEND_SM_STATE_TRANSMIT_INIT = 0,
	PORT_SYNC_SYNC_SEND_SM_STATE_SEND_MD_SYNC,
	PORT_SYNC_SYNC_SEND_SM_STATE_SYNC_RECEIPT_TIMEOUT,
} ptp_port_sync_sync_send_sm_state_t;

typedef enum {
	PORT_SYNC_SYNC_SEND_SM_EVENT_PSSYNC_TIMEOUT = 0,
	PORT_SYNC_SYNC_SEND_SM_EVENT_RUN
} ptp_port_sync_sync_send_sm_event_t;

/* PortSyncsyncSendSM variables  802.1AS-2020 section 10.2.12.1
*
*/
struct port_sync_sync_send_sm
{
	/*
	Boolean variable that notifies the current state machine when a PortSyncSync
	structure is received from the SiteSyncSync state machine of the SiteSync entity of the time-aware system
	(see 10.2.2.3). This variable is reset by this state machine.
	*/
	bool rcvd_pssync_psss;

	/*
	pointer to the received PortSyncSync structure indicated by rcvdPSSync.
	*/
	struct port_sync_sync *rcvd_pssync_ptr;

	/*
	the sourcePortIdentity member of the most recently received
	PortSyncSync structure. The data type for lastSourcePortIdentity is PortIdentity.
	*/
	struct ptp_port_identity last_source_port_identity;

	/*
	the preciseOriginTimestamp member of the most recently
	received PortSyncSync structure. The data type for lastPreciseOriginTimestamp is Timestamp.
	*/
	struct ptp_timestamp last_precise_origin_timestamp;

	/*
	the followUpCorrectionField member of the most recently
	received PortSyncSync member. The data type for lastFollowUpCorrectionField is ScaledNs.
	*/
	struct ptp_scaled_ns last_follow_up_correction_field;

	/*
	the rateRatio member of the most recently received PortSyncSync structure. The
	data type for lastRateRatio is Double.
	*/
	ptp_double last_rate_ratio;

	/*
	the upstreamTxTime of the most recently received PortSyncSync
	member. The data type for lastUpstreamTxTime is UScaledNs.
	*/
	struct ptp_u_scaled_ns last_upstream_tx_time;

	/*
	the value of currentTime (i.e., the time relative to the LocalClock entity)
	when the most recent MDSyncSend structure was sent. The data type for lastSyncSentTime is UScaledNs.
	*/
	struct ptp_u_scaled_ns last_sync_sent_time;

	/*
	the portNumber of the port on which time-synchronization information was
	most recently received. The data type for lastReceivedPortNum is UInteger16.
	*/
	u16 last_rcvd_port_num;

	/*
	 the gmTimeBaseIndicator of the most recently received
	PortSyncSync member. The data type for lastGmTimeBaseIndicator is UInteger16.
	*/
	u16 last_gm_time_base_indicator;

	/*
	the lastGmPhaseChange of the most recently received PortSyncSync
	member. The data type for lastGmPhaseChange is ScaledNs.
	*/
	struct ptp_scaled_ns last_gm_phase_change;

	/*
	the lastGmFreqChange of the most recently received PortSyncSync
	member. The data type for lastGmPhaseChange is Double.
	*/
	ptp_double last_gm_freq_change;

	/*
	a pointer to the MDSyncSend structure sent to the MD entity of this port.
	*/
	struct md_sync_send *tx_md_sync_send_ptr;

	/*
	10.2.12.1.13 numberSyncTransmissions: A count of the number of consecutive Sync message
	transmissions after the SyncIntervalSetting state machine (see Figure 10-20) has set syncSlowdown
	(see 10.2.5.17) to TRUE. The data type for numberSyncTransmissions is UInteger8.
	*/
	u8 number_sync_transmissions;

	/*
	10.2.12.1.14 interval1: A local variable that holds either syncInterval or oldSyncInterval. The data type for
	interval1 is UScaledNs.
	*/
	struct ptp_u_scaled_ns interval1;

	/*
	the value of the syncReceiptTimeoutTime member of the most
	recently received PortSyncSync structure. The data type for syncReceiptTimeoutTime is UScaledNs.
	*/
	struct ptp_u_scaled_ns sync_receipt_timeout_time;

	/*
	Custom additions (not specified in standard but needed for some computations)
	*/
	ptp_port_sync_sync_send_sm_state_t state;

	struct timer sync_transmit_timer;
	struct timer sync_receipt_timeout_timer;
};


typedef enum {
	PORT_ANNOUNCE_RCV_SM_STATE_DISCARD = 0,
	PORT_ANNOUNCE_RCV_SM_STATE_RECEIVE
} ptp_port_announce_rcv_sm_state_t;

/* PortAnnounceReceiveSM variables IEEE 802.1AS-2020 section 10.3.11
*
*/
struct port_announce_receive_sm {
	/*
	a Boolean variable that notifies the current state machine when Announce
	message information is received from the MD entity of the same port.
	This variable is reset by this state machine.
	*/
	bool rcvd_announce_par;

	/*
	a pointer to the announce pdu received on the port
	*/
	struct ptp_announce_pdu *rcvd_announce_ptr;

	/*
	Custom additions (not specified in standard but needed for some computations)
	*/
	ptp_port_announce_rcv_sm_state_t state;

	struct timer timeout_timer;
};

/* PortAnnounceInformationSM variables 10.3.11
*
*/

struct port_announce_information_sm {
	/*
	a variable used to save the time at which announce receipt
	timeout occurs. The data type for announceReceiptTimeoutTime is UScaledNs.
	*/
	struct ptp_u_scaled_ns announceReceiptTimeoutTime;

	/*
	the messagePriorityVector corresponding to the received Announce
	information. The data type for messagePriority is UInteger224 (see 10.3.4).
	*/
	struct ptp_priority_vector messagePriority;

	/*
	an Enumeration2 that holds the value returned by rcvInfo() (see 10.3.11.2.1).
	*/
	u8 rcvdInfo;
};


typedef enum {
	PORT_ANNOUNCE_TRANSMIT_SM_STATE_TRANSMIT_INIT = 0,
	PORT_ANNOUNCE_TRANSMIT_SM_STATE_TRANSMIT_ANNOUNCE,
	PORT_ANNOUNCE_TRANSMIT_SM_STATE_TRANSMIT_PERIODIC,
	PORT_ANNOUNCE_TRANSMIT_SM_STATE_IDLE
} ptp_port_announce_transmit_sm_state_t;

typedef enum {
	PORT_ANNOUNCE_TRANSMIT_SM_EVENT_MIN = 0,
	PORT_ANNOUNCE_TRANSMIT_SM_EVENT_TRANSMIT_INTERVAL = PORT_ANNOUNCE_TRANSMIT_SM_EVENT_MIN,
	PORT_ANNOUNCE_TRANSMIT_SM_EVENT_RUN,
	PORT_ANNOUNCE_TRANSMIT_SM_EVENT_MAX = PORT_ANNOUNCE_TRANSMIT_SM_EVENT_RUN
} ptp_port_announce_transmit_sm_event_t;


/*
* PortAnnounceTransmit state machine variables - 802.1AS-2020 section 10.3.16
*/

struct port_announce_transmit_sm {
	/*
	The time relative to the local clock at which the next transmission of announce information is to occur.
	*/
	struct ptp_u_scaled_ns announce_send_time;

	/*
	A count of the number of consecutives announce message transmissions after the AnnounceIntervalSetting
	state mahcine has announceSlowDown to TRUE.
	*/
	u8 number_announce_transmissions;

	/*
	A local variable that holds either announceInterval or oldAnnounceInterval.
	*/
	struct ptp_u_scaled_ns interval2;

	/*
	Custom additions (not specified in standard but needed for some computations)
	*/
	ptp_port_announce_transmit_sm_state_t state;

	struct timer transmit_timer;
};


typedef enum {
	PORT_ANNOUNCE_INTERVAL_SM_STATE_NOT_ENABLED = 0,
	PORT_ANNOUNCE_INTERVAL_SM_STATE_INITIALIZE,
	PORT_ANNOUNCE_INTERVAL_SM_STATE_SET_INTERVAL,
} ptp_port_announce_interval_sm_state_t;

/*
* AnnounceIntervalSetting state machine variables - 802.1AS-2020 section 10.3.17
*/

struct port_announce_interval_sm {
	/*
	a Boolean variable that notifies the current state machine when a Signaling
	message that contains a Message Interval Request TLV (see 10.5.4.3) is received. This variable is reset by
	the current state machine.
	*/
	bool rcvd_signaling_msg2;

	/*
	a pointer to a structure whose members contain the values of the fields of the
	received Signaling message that contains a Message Interval Request TLV (see 10.5.4.3).
	*/
	struct ptp_signaling_pdu *rcvd_signaling_ptr_ais;

	/*
	10.3.17.1.3 logSupportedAnnounceIntervalMax: The maximum supported logarithm to base 2 of the
	announce interval. The data type for logSupportedAnnounceIntervalMax is Integer8.
	*/
	s8 log_supported_announce_interval_max;

	/*
	10.3.17.1.4 logSupportedClosestLongerAnnounceInterval: The logarithm to base 2 of the announce
	interval, such that logSupportedClosestLongerAnnounceInterval > logRequestedAnnounceInterval, that is
	numerically closest to logRequestedAnnounceInterval, where logRequestedAnnounceInterval is the
	argument of the function computeLogAnnounceInterval() (see 10.3.17.2.2). The data type for
	logSupportedClosestLongerAnnounceInterval is Integer8.
	*/
	s8 log_supported_closest_longer_announce_interval;

	/*
	10.3.17.1.5 computedLogAnnounceInterval: A variable used to hold the result of the function
	computeLogAnnounceInterval(). The data type for computedLogAnnounceInterval is Integer8.
	*/
	s8 computed_log_announce_interval;

	/*
	Custom additions (not specified in standard but needed for some computations)
	*/
	ptp_port_announce_interval_sm_state_t state;
};


/*
The PortSync entity for a SlavePort receives best master selection information
from the time-aware system at the other end of the associated link, compares this to
the current best master information that it has, and forwards the result of the
comparison to the Site Sync entity.

The PortSync entity for a SlavePort also receives time-synchronization information from the MD
entity associated with the port, and forwards it to the SiteSync entity.

The PortSync entity for a MasterPort sends best master selection and time-synchronization
information to the MD entity for the port, which in turn sends the respective messages
*/
struct ptp_port_sync_entity {
	struct port_sync_sync sync;
	struct port_sync_sync_receive_sm sync_receive_sm;
	struct port_sync_sync_send_sm sync_send_sm;
	struct port_announce_receive_sm announce_receive_sm;
	struct port_announce_information_sm announce_information_sm;
};

typedef enum {
	PORT_SYNC_SYNC_RCV_SM_STATE_DISCARD = 0,
	PORT_SYNC_SYNC_RCV_SM_STATE_RECEIVED_MDSYNC
} ptp_port_sync_sync_rcv_sm_state_t;

typedef enum {
	PORT_ANNOUNCE_INFO_SM_STATE_DISABLED = 0,
	PORT_ANNOUNCE_INFO_SM_STATE_AGED,
	PORT_ANNOUNCE_INFO_SM_STATE_UPDATE,
	PORT_ANNOUNCE_INFO_SM_STATE_CURRENT,
	PORT_ANNOUNCE_INFO_SM_STATE_RECEIVE,
	PORT_ANNOUNCE_INFO_SM_STATE_SUPERIOR_MASTER_PORT,
	PORT_ANNOUNCE_INFO_SM_STATE_REPEATED_MASTER_PORT,
	PORT_ANNOUNCE_INFO_SM_STATE_INFERIOR_MASTER_OR_OTHER
} ptp_port_announce_info_sm_state_t;

typedef enum {
	PORT_ANNOUNCE_INFO_SM_EVENT_SYNC_TIMEOUT = 0,
	PORT_ANNOUNCE_INFO_SM_EVENT_ANNOUNCE_TIMEOUT,
	PORT_ANNOUNCE_INFO_SM_EVENT_RUN,
} ptp_port_announce_info_sm_event_t;


typedef enum {
	ANNOUNCE_PRIO_SUPERIOR_MASTER_INFO = 0,
	ANNOUNCE_PRIO_REPEATED_MASTER_INFO,
	ANNOUNCE_PRIO_INFERIOR_MASTER_INFO,
	ANNOUNCE_PRIO_OTHER_INFO,
}ptp_announce_priority_info_t;


typedef enum {
	PORT_STATE_SM_STATE_INIT_BRIDGE = 0,
	PORT_STATE_SM_STATE_SELECTION
} ptp_port_state_sm_state_t;


/***************** SiteSync entity *********************/


/* SiteSyncSyncSM variables 10.2.6.1
*
*/
struct ptp_site_sync_sync_sm {
	/*
	a Boolean variable that notifies the current state machine when a PortSyncSync
	structure (see 10.2.2.3) is received from the PortSyncSyncReceive state machine of a PortSync entity or
	from the ClockMasterSyncSend state machine of the ClockMaster entity. This variable is reset by this state
	machine.
	*/
	bool rcvdPSSync;

	/*
	a pointer to the received PortSyncSync structure indicated by rcvdPSSync.
	*/
	struct port_sync_sync *rcvdPSSyncPtr;

	/*
	a pointer to the PortSyncSync structure transmitted by the state machine.
	*/
	struct port_sync_sync *txPSSyncPtr;
};



/*
The SiteSync entity executes the portion of best master clock selection associated with
the time-aware system as a whole, i.e., it uses the best master information received on
each port to determine which port has received the best information, and updates the roles
of all the ports.It also distributes synchronization information received on the SlavePort
to all the ports whose role is MasterPort
*/
struct ptp_site_sync_entity {
	struct port_sync_sync pssync;
	struct ptp_site_sync_sync_sm sync_sm;
};


typedef enum {
	SITE_SYNC_SYNC_SM_STATE_INITIALIZING = 0,
	SITE_SYNC_SYNC_SM_STATE_RECEIVING_SYNC
} ptp_site_sync_sync_sm_state_t;


/***************** MD entity *********************/


/*
* Per Instance and per PTP-Port MD entity global variables (11.2.13)
*/
struct ptp_instance_md_entity_globals {
	/*
	the sequenceId for the next Sync message to be sent by this MD entity. The data
	type for syncSequenceId is UInteger16.
	*/
	u16 syncSequenceId;

	/*
	Boolean variable that is TRUE if the PTP Port is capable of receiving one-step
	Sync messages.
	*/
	bool oneStepReceive;

	/*
	A Boolean variable that is TRUE if the PTP Port is capable of transmitting
	one-step Sync messages.*/
	bool oneStepTransmit;

	/*
	A Boolean variable that is TRUE if the PTP Port will be transmitting one-step
	Sync messages (see 11.1.3).
	*/
	bool oneStepTxOper;
};


/*
* Per PTP-Port or per Link-Port MD entity global variables (11.2.13)
*/
struct ptp_md_entity_globals {
	/*
	the current value of the logarithm to base 2 of the mean time
	interval, in seconds, between the sending of successive Pdelay_Req messages (see 11.5.2.2). This value is
	set in the LinkDelaySyncIntervalSetting state machine (see 11.2.17). The data type for
	currentLogPdelayReqInterval is Integer8.
	*/
	s8 currentLogPdelayReqInterval;

	/*
	the initial value of the logarithm to base 2 of the mean time
	interval, in seconds, between the sending of successive Pdelay_Req messages (see 11.5.2.2). The data type
	for initialLogPdelayReqInterval is Integer8.
	*/
	s8 initialLogPdelayReqInterval;

	/*
	a variable containing the mean Pdelay_Req message transmission interval
	for the port corresponding to this MD entity. The value is set in the LinkDelaySyncIntervalSetting state
	machine (see 11.2.17). The data type for pdelayReqInterval is UScaledNs.
	*/
	struct ptp_u_scaled_ns pdelayReqInterval;

	/*
	the number of Pdelay_Req messages for which a valid response is not
	received, above which a port is considered to not be exchanging peer delay messages with its neighbor. The
	data type for allowedLostResponses is UInteger8. The required value of allowedLostResponses is given in 11.5.3.
	*/
	u8 allowedLostResponses;

	/*
	The  number of faults above which asCapableAcrossDomains is set to FALSE, i.e., the port is considered not capable
	of interoperating with its neighbor via the IEEE 802.1AS protocol
	*/
	u8 allowedFaults;

	/*
	a Boolean that is TRUE if the port is measuring link propagation delay. For a
	full-duplex, point-to-point link, the port is measuring link propagation delay if it is receiving Pdelay_Resp
	and Pdelay_Resp_Follow_Up messages from the port at the other end of the link (i.e., it performs the
	measurement using the peer delay mechanism).
	*/
	bool isMeasuringDelay;

	/*
	the propagation time threshold, above which a port is not considered
	capable of participating in the IEEE 802.1AS protocol. If meanLinkDelay (see 10.2.4.7) exceeds
	meanLinkDelayThresh, then asCapable (see 11.2.12.4) is set to FALSE. The data type for
	meanLinkDelayThresh is UScaledNs.
	*/
	struct ptp_u_scaled_ns meanLinkDelayThresh;
};


typedef enum {
	PDELAY_REQ_SM_STATE_MIN_VALUE = 0,
	PDELAY_REQ_SM_STATE_NOT_ENABLED = PDELAY_REQ_SM_STATE_MIN_VALUE,
	PDELAY_REQ_SM_STATE_INITIAL_SEND_PDELAY_REQ,
	PDELAY_REQ_SM_STATE_RESET,
	PDELAY_REQ_SM_STATE_SEND_PDELAY_REQ,
	PDELAY_REQ_SM_STATE_WAITING_FOR_PDELAY_RESP,
	PDELAY_REQ_SM_STATE_WAITING_FOR_PDELAY_RESP_FOLLOW_UP,
	PDELAY_REQ_SM_STATE_WAITING_FOR_PDELAY_INTERVAL_TIMER,
	PDELAY_REQ_SM_STATE_MAX_VALUE = PDELAY_REQ_SM_STATE_WAITING_FOR_PDELAY_INTERVAL_TIMER
} ptp_pdelay_req_sm_state_t;

typedef enum {
	PDELAY_REQ_SM_EVENT_MIN_VALUE = 0,
	PDELAY_REQ_SM_EVENT_REQ_INTERVAL = PDELAY_REQ_SM_EVENT_MIN_VALUE,
	PDELAY_REQ_SM_EVENT_RUN,
	PDELAY_REQ_SM_EVENT_MAX_VALUE = PDELAY_REQ_SM_EVENT_RUN
} ptp_pdelay_req_sm_event_t;

/*
* MDPdelayReqSM variables 11.2.15.1
*/
struct ptp_md_entity_pdelay_req_sm {
	/*
	a variable used to save the time at which the Pdelay_Req interval timer is
	started A Pdelay_Req message is sent when this timer expires.
	*/
	u64 pdelayIntervalTimer;

	/*
	a Boolean variable that notifies the current state machine when a Pdelay_Resp
	message is received. This variable is reset by the current state machine.
	*/
	bool rcvdPdelayResp;

	/*
	a pointer to a structure whose members contain the values of the fields of the
	Pdelay_Resp message whose receipt is indicated by rcvdPdelayResp (see 11.2.15.1.2).
	*/
	struct ptp_pdelay_resp_pdu *rcvdPdelayRespPtr;

	/*
	a Boolean variable that notifies the current state machine when a
	Pdelay_Resp_Follow_Up message is received. This variable is reset by the current state machine
	*/
	bool rcvdPdelayRespFollowUp;

	/*
	a pointer to a structure whose members contain the values of the
	fields of the Pdelay_Resp_Follow_Up message whose receipt is indicated by rcvdPdelayRespFollowUp
	*/
	struct ptp_pdelay_resp_follow_up_pdu *rcvdPdelayRespFollowUpPtr;

	/*
	a pointer to a structure whose members contain the values of the fields of a
	Pdelay_Req message to be transmitted.
	*/
	struct ptp_pdelay_req_pdu *txPdelayReqPtr;

	/*
	a Boolean variable that notifies the current state machine when
	the <pdelayReqEventEgressTimestamp> (see 11.3.2.1) for a transmitted Pdelay_Req message is received.
	This variable is reset by the current state machine.
	*/
	bool rcvdMDTimestampReceive;

	/*
	a variable that holds the sequenceId for the next Pdelay_Req message
	to be transmitted by this MD entity. The data type for pdelayReqSequenceId is UInteger16.
	*/
	u16 pdelayReqSequenceId;

	/*
	a count of the number of consecutive Pdelay_Req messages sent by the port,
	for which Pdelay_Resp and/or Pdelay_Resp_Follow_Up messages are not received. The data type for
	lostResponses is UInteger16.
	*/
	u16 lostResponses;

	/* PICS AVnu PTP-5
	a count of the number of multiple Pdelay_Resp messages received by the port.
	The data type for multipleResponses is UInteger16.
	*/
	u16 multipleResponses;

	/*
	a Boolean variable that indicates whether or not the function
	computePdelayRateRatio() (see 11.2.15.2.3) successfully computed neighborRateRatio (see 10.2.4.6).
	*/
	bool neighborRateRatioValid;

	/*
	A count of the number of consecutive faults
	*/
	u8 detectedFaults;

	/*
	Boolean variable whose value is equal to ptpPortEnabled (see 10.2.5.13) if
	this state machine is invoked by the instance-specific peer-to-peer delay mechanism and is equal to
	cmldsLinkPortEnabled (see 11.2.18.1) if this state machine is invoked by the CMLDS
	*/
	/*Note: Type changed to pointer to make easier tracking of the ptpPortEnabled/cmldsLinkPortEnabled value changes
	see gptp.c::gptp_begin_port_update_fsm() */
	bool *portEnabled0;

	/*
	A variable whose value is +1 if this state machine is invoked by the instance-specific peer-to-
	peer delay mechanism and -1 if this state machine is invoked by the CMLDS
	*/
	s8 s;

	/*
	Custom additions (not specified in standard but needed for some computations)
	*/
	u64 prev_pdelay_response_event_ingress_timestamp;	// in nanoseconds
	u64 prev_corrected_responder_event_timestamp;	// in nanoseconds
	struct filter pdelay_filter;

	unsigned int prev_rate_ratio_local_clk_phase_discont;
	unsigned int prev_pdelay_local_clk_phase_discont;

	ptp_pdelay_req_sm_state_t state;

	u64 req_tx_ts;
	u64 resp_rx_ts;

	struct timer req_timer;

	struct ptp_pdelay_req_pdu req_tx;
	struct ptp_pdelay_resp_pdu resp_rx;
	struct ptp_pdelay_resp_follow_up_pdu resp_fup_rx;

	/* pdelay timing profiling */
	u64 req_time;
	u64 req_to_ts_min;
	u64 req_to_ts_max;
	u64 ts_time;
	u64 resp_time;
	u64 fup_time;
	u64 req_to_ts;
	u64 req_to_resp;
	u64 req_to_fup;
};


typedef enum {
	PDELAY_RESP_SM_STATE_MIN_VALUE = 0,
	PDELAY_RESP_SM_STATE_NOT_ENABLED = PDELAY_REQ_SM_EVENT_MIN_VALUE,
	PDELAY_RESP_SM_STATE_WAITING_FOR_PDELAY_REQ,
	PDELAY_RESP_SM_STATE_INITIAL_WAITING_FOR_PDELAY_REQ,
	PDELAY_RESP_SM_STATE_SENT_PDELAY_RESP_WAITING_FOR_TIMESTAMP,
	PDELAY_RESP_SM_STATE_MAX_VALUE = PDELAY_RESP_SM_STATE_SENT_PDELAY_RESP_WAITING_FOR_TIMESTAMP
} ptp_pdelay_resp_sm_state_t;


typedef enum {
	PDELAY_RESP_SM_EVENT_MIN_VALUE = 0,
	PDELAY_RESP_SM_EVENT_REQ_RECEIVED = PDELAY_RESP_SM_EVENT_MIN_VALUE,
	PDELAY_RESP_SM_EVENT_RUN,
	PDELAY_RESP_SM_EVENT_MAX_VALUE = PDELAY_RESP_SM_EVENT_RUN
} ptp_pdelay_resp_sm_event_t;

/*
* MDPdelayRespSM variables 11.2.16.1
*/
struct ptp_md_entity_pdelay_resp_sm {
	/*
	a Boolean variable that notifies the current state machine when a Pdelay_Req
	message is received. This variable is reset by the current state machine.
	*/
	bool rcvdPdelayReq;

	/*
	a Boolean variable that notifies the current state machine when
	the <pdelayRespEventEgressTimestamp> (see 11.3.2.1) for a transmitted Pdelay_Resp message is received.
	This variable is reset by the current state machine.
	*/
	bool rcvdMDTimestampReceive;

	/*
	a pointer to a structure whose members contain the values of the fields of a
	Pdelay_Resp message to be transmitted.
	*/
	struct ptp_pdelay_resp_pdu *txPdelayRespPtr;

	/*
	a pointer to a structure whose members contain the values of the
	fields of a Pdelay_Resp_Follow_Up message to be transmitted.
	*/
	struct ptp_pdelay_resp_follow_up_pdu *txPdelayRespFollowUpPtr;

	/*
	Boolean variable whose value is equal to ptpPortEnabled (see 10.2.5.13) if
	this state machine is invoked by the instance-specific peer-to-peer delay mechanism and is equal to
	cmldsLinkPortEnabled (see 11.2.18.1) if this state machine is invoked by the CMLDS
	*/
	/*Note: Type changed to pointer to make easier tracking of the ptpPortEnabled/cmldsLinkPortEnabled value changes
	see gptp.c::gptp_begin_port_update_fsm() */
	bool *portEnabled1;

	/*
	Custom additions (not specified in standard but needed for some computations)
	*/

	/*
	A variable whose value is +1 if this state machine is invoked by the instance-specific peer-to-
	peer delay mechanism and -1 if this state machine is invoked by the CMLDS
	*/
	s8 s;

	ptp_pdelay_resp_sm_state_t state;

	u64 req_rx_ts;
	u64 resp_tx_ts;

	struct ptp_pdelay_req_pdu req_rx;
	struct ptp_pdelay_resp_pdu resp_tx;
	struct ptp_pdelay_resp_follow_up_pdu resp_fup_tx;
};



/*
This structure contains information that is sent by the MD entity of a port to the PortSync entity of that port.
It provides the PortSync entity with master clock timing information and timestamp of receipt of a time-
synchronization event message compensated for propagation time on the upstream link. The information is
sent to the PortSync entity upon receipt of time-synchronization information by the MD entity of the port.
The information is in turn provided by the PortSync entity to the SiteSync entity. The information is used by
the SiteSync entity to compute the rate ratio of the local oscillator relative to the master and is
communicated to the other PortSync entities for use in computing master clock timing information.
*/

struct md_sync_receive {
	struct ptp_scaled_ns followUpCorrectionField;
	struct ptp_port_identity sourcePortIdentity;
	u8 logMessageInterval;
	struct ptp_timestamp preciseOriginTimestamp;
	struct ptp_u_scaled_ns upstreamTxTime;
	ptp_double rateRatio;
	u16 gmTimeBaseIndicator;
	struct ptp_scaled_ns lastGmPhaseChange;
	double lastGmFreqChange;
};

/*
This structure contains information that is sent by the PortSync entity of a port to the MD entity of that port
when requesting that the MD entity cause time-synchronization information to be sent. The structure
contains information that reflects the most recent time-synchronization information received by this time-
aware system, and is used to determine the contents of the time-synchronization event message and possibly
separate general message that will be sent by this port.
*/
struct md_sync_send {
	struct ptp_scaled_ns followUpCorrectionField;
	struct ptp_port_identity sourcePortIdentity;
	s8 logMessageInterval;
	struct ptp_timestamp preciseOriginTimestamp;
	struct ptp_u_scaled_ns upstreamTxTime;
	ptp_double rateRatio;
	u16 gmTimeBaseIndicator;
	struct ptp_scaled_ns lastGmPhaseChange;
	ptp_double lastGmFreqChange;

	/*
	Custom additions (not specified in standard but needed for some computations)
	*/

	/* Added for AVnu test gPTP.com.c.14.2. portNumber of the port on which time-synchronization information was most recently received.*/
	u16 lastRcvdPortNum;
};

/*
* MDSyncReceiveSM variables 11.2.13.1
*/
struct ptp_md_entity_sync_receive_sm{
	/*
	a variable used to save the time at which the information
	conveyed by a received Sync message will be discarded if the associated Follow_Up message is not received
	by then. The data type for syncReceiptTimeout is UScaledNs
	*/
	struct ptp_u_scaled_ns followUpReceiptTimeoutTime;

	/*
	a Boolean variable that notifies the current state machine when a Sync message is
	received. This variable is reset by the current state machine.
	*/
	bool rcvdSync;

	/*
	a Boolean variable that notifies the current state machine when a Follow_Up
	message is received. This variable is reset by the current state machine.
	*/
	bool rcvdFollowUp;

	/*
	a pointer to a structure whose members contain the values of the fields of the
	Sync message whose receipt is indicated by rcvdSync (see 11.2.13.1.2).
	*/
	struct ptp_sync_pdu *rcvdSyncPtr;

	/*
	a pointer to a structure whose members contain the values of the fields of the
	Follow_Up message whose receipt is indicated by rcvdFollowUp (see 11.2.13.1.3).
	*/
	struct ptp_follow_up_pdu *rcvdFollowUpPtr;

	/*
	a pointer to a structure whose members contain the values of the
	parameters of an MDSyncReceive structure to be transmitted.
	*/
	struct md_sync_receive *txMDSyncReceivePtr;

	/*
	the sync interval (see 10.6.2.1) for the upstream port that sent the
	received Sync message
	*/
	s8 upstreamSyncInterval;
};


/*
* MDSyncSendSM variables 11.2.14.1
*/
struct ptp_md_entity_sync_send_sm{
	/*
	a Boolean variable that notifies the current state machine when an MDSyncSend
	structure is received. This variable is reset by the current state machine.
	*/
	bool rcvdMDSync;

	/*
	a pointer to a structure whose members contain the values of the fields of a Sync
	message to be transmitted.
	*/
	struct ptp_sync_pdu *txSyncPtr;

	/*
	a Boolean variable that notifies the current state machine when
	the <syncEventEgressTimestamp> (see 11.3.2.1) for a transmitted Sync message is received. This variable is
	reset by the current state machine.
	*/
	bool rcvdMDTimestampReceive;

	/*
	a pointer to the received MDTimestampReceive structure (see 11.2.9).
	*/
	struct md_timestamp_receive *rcvdMDTimestampReceivePtr;

	/*
	a pointer to a structure whose members contain the values of the fields of a
	Follow_Up message to be transmitted.
	*/
	struct ptp_follow_up_pdu *txFollowUpPtr;
};


typedef enum {
	LINK_INTERVAL_SETTING_SM_STATE_MIN_VALUE = 0,
	LINK_INTERVAL_SETTING_SM_STATE_NOT_ENABLED = LINK_INTERVAL_SETTING_SM_STATE_MIN_VALUE,
	LINK_INTERVAL_SETTING_SM_STATE_INITIALIZE,
	LINK_INTERVAL_SETTING_SM_STATE_SET_INTERVAL,
	LINK_INTERVAL_SETTING_SM_STATE_MAX_VALUE = LINK_INTERVAL_SETTING_SM_STATE_SET_INTERVAL
} ptp_link_interval_setting_sm_state_t;



/*
* LinkDelayIntervalSetting variables 11.2.21
*/
struct ptp_md_link_delay_interval_setting_sm {

	/*
	11.2.21.2.1 rcvdSignalingMsg1: A Boolean variable that notifies the current state machine when a
	Signaling message that contains a Message Interval Request TLV (see 10.6.4.3) is received. This variable is
	reset by the current state machine.
	*/
	bool rcvd_signaling_msg1;

	/*
	11.2.21.2.2 rcvdSignalingPtrLDIS: A pointer to a structure whose members contain the values of the fields
	of the received Signaling message that contains a Message Interval Request TLV (see 10.6.4.3).
	*/
	struct ptp_signaling_pdu *rcvd_signaling_ptr_ldis;

	/*
	11.2.21.2.3 portEnabled3: A Boolean variable whose value is equal to ptpPortEnabled (see 10.2.5.13) if
	this state machine is invoked by the instance-specific peer-to-peer delay mechanism and is equal to
	cmldsLinkPortEnabled (see 11.2.18.1) if this state machine is invoked by the CMLDS.
	*/
	/*Note: Type changed to pointer to make easier tracking of the ptpPortEnabled/cmldsLinkPortEnabled value changes
	see gptp.c::gptp_begin_port_update_fsm() */
	bool *portEnabled3;

	/*
	11.2.21.2.4 logSupportedPdelayReqIntervalMax: The maximum supported logarithm to base 2 of the
	Pdelay_Req interval. The data type for logSupportedPdelayReqIntervalMax is Integer8.
	*/
	s8 log_supported_pdelayreq_interval_max;

	/*
	11.2.21.2.5 logSupportedClosestLongerPdelayReqInterval: The logarithm to base 2 of the Pdelay_Req
	interval, such that logSupportedClosestLongerPdelayReqInterval > logRequestedPdelayReqInterval, that is
	numerically closest to logRequestedPdelayReqInterval, where logRequestedPdelayReqInterval is the
	argument of the function computeLogPdelayReqInterval() (see 11.2.21.3.2). The data type for
	logSupportedClosestLongerPdelayReqInterval is Integer8.
	*/
	s8 log_supported_closest_longer_pdelayreq_Interval;

	/*
	11.2.21.2.6 computedLogPdelayReqInterval: A variable used to hold the result of the function
	computeLogPdelayReqInterval(). The data type for computedLogPdelayReqInterval is Integer8
	*/
	s8 computed_log_pdelayReq_interval;

	/*
	Custom additions (not specified in standard but needed for some computations)
	*/
	ptp_link_interval_setting_sm_state_t state;
};


typedef enum {
	SYNC_INTERVAL_SETTING_SM_STATE_MIN_VALUE = 0,
	SYNC_INTERVAL_SETTING_SM_STATE_NOT_ENABLED = SYNC_INTERVAL_SETTING_SM_STATE_MIN_VALUE,
	SYNC_INTERVAL_SETTING_SM_STATE_INITIALIZE,
	SYNC_INTERVAL_SETTING_SM_STATE_SET_INTERVAL,
	SYNC_INTERVAL_SETTING_SM_STATE_MAX_VALUE = SYNC_INTERVAL_SETTING_SM_STATE_SET_INTERVAL
} ptp_sync_interval_setting_sm_state_t;


/*
* SyncIntervalSetting variables 10.3.18
*/
struct ptp_sync_interval_setting_sm {
	/*
	10.3.18.1.1 A Boolean variable that notifies the current state machine when a
	Signaling message that contains a Message Interval Request TLV (see 10.6.4.3) is received. This variable is
	reset by the current state machine.
	*/
	bool rcvd_signaling_msg3;

	/*
	10.3.18.1.2 A pointer to a structure whose members contain the values of the fields
	of the received Signaling message that contains a Message Interval Request TLV (see 10.6.4.3).
	*/
	struct ptp_signaling_pdu *rcvd_signaling_ptr_sis;

	/*
	10.3.18.1.3 logSupportedSyncIntervalMax: The maximum supported logarithm to base 2 of the sync
	interval. The data type for logSupportedSyncIntervalMax is Integer8.
	*/
	s8 log_supported_sync_interval_max;

	/*
	10.3.18.1.4 The logarithm to base 2 of the sync interval, such
	that logSupportedClosestLongerSyncInterval > logRequestedSyncInterval, that is numerically closest to
	logRequestedSyncInterval, where logRequestedSyncInterval is the argument of the function
	computeLogSyncInterval() (see 10.3.18.2.2). The data type for logSupportedClosestLongerSyncInterval is
	Integer8.
	*/
	s8 log_supported_closest_longer_sync_interval;

	/*
	10.3.18.1.5 A variable used to hold the result of the function
	computeLogSyncInterval(). The data type for computedLogSyncInterval is Integer8.
	*/
	s8 computed_log_sync_interval;

	/*
	Custom additions (not specified in standard but needed for some computations)
	*/
	ptp_sync_interval_setting_sm_state_t state;
};


typedef enum {
	GPTP_CAPABLE_TRANSMIT_SM_EVENT_MIN_VALUE = 0,
	GPTP_CAPABLE_TRANSMIT_SM_EVENT_RUN = GPTP_CAPABLE_TRANSMIT_SM_EVENT_MIN_VALUE,
	GPTP_CAPABLE_TRANSMIT_SM_EVENT_INTERVAL,
	GPTP_CAPABLE_TRANSMIT_SM_EVENT_MAX_VALUE = GPTP_CAPABLE_TRANSMIT_SM_EVENT_INTERVAL
} ptp_gptp_capable_transmit_sm_event_t;

typedef enum {
	GPTP_CAPABLE_TRANSMIT_SM_STATE_MIN_VALUE = 0,
	GPTP_CAPABLE_TRANSMIT_SM_STATE_NOT_ENABLED = GPTP_CAPABLE_TRANSMIT_SM_STATE_MIN_VALUE,
	GPTP_CAPABLE_TRANSMIT_SM_STATE_INITIALIZE,
	GPTP_CAPABLE_TRANSMIT_SM_STATE_TRANSMIT_TLV,
	GPTP_CAPABLE_TRANSMIT_SM_STATE_MAX_VALUE = GPTP_CAPABLE_TRANSMIT_SM_STATE_TRANSMIT_TLV
} ptp_gptp_capable_transmit_sm_state_t;

/*
* gPTPCapableTransmit variables 802.1AS-2020 - 10.4.1
*/
struct ptp_gptp_capable_transmit_sm {
	/*
	10.4.1.1.1 intervalTimer: A variable used to save the time at which the gPTP-capable message interval
	timer is set (see Figure 10-21). A Signaling message containing a gPTP-capable TLV is sent when this timer
	expires. The data type for intervalTimer is UScaledNs.
	*/
	struct ptp_u_scaled_ns interval_timer;

	/*
	10.4.1.1.2 txSignalingMsgPtr: A pointer to a structure whose members contain the values of the fields of a
	Signaling message to be transmitted, which contains a gPTP-capable TLV (see 10.6.4.4).
	*/
	struct ptp_signaling_pdu *tx_signaling_msg_ptr;

	/*
	10.4.1.1.3 interval3: A local variable that holds either gPtpCapableMessageInterval
	oldGptpCapableMessageInterval. The data type for interval3 is UScaledNs.
	*/
	struct ptp_u_scaled_ns interval3;

	/*
	10.4.1.1.4 numberGptpCapableMessageTransmissions: A count of the number of consecutive
	transmissions of Signaling messages that contain a gPTP-capable TLV, after the GptpCapableIntervalSetting
	state machine (see Figure 10-23 in 10.4.3.3) has set gPtpCapableMessageSlowdown (see 10.2.5.19) to
	TRUE. The data type for numberGptpCapableMessageTransmissions is UInteger8.
	*/
	u8 number_gptp_capable_message_transmissions;

	/*
	Custom additions (not specified in standard but needed for some computations)
	*/
	ptp_gptp_capable_transmit_sm_state_t state;

	struct timer timeout_timer;

	struct ptp_signaling_pdu msg;
};


typedef enum {
	GPTP_CAPABLE_RECEIVE_SM_EVENT_MIN_VALUE = 0,
	GPTP_CAPABLE_RECEIVE_SM_EVENT_RUN = GPTP_CAPABLE_RECEIVE_SM_EVENT_MIN_VALUE,
	GPTP_CAPABLE_RECEIVE_SM_EVENT_TIMEOUT,
	GPTP_CAPABLE_RECEIVE_SM_EVENT_MAX_VALUE = GPTP_CAPABLE_RECEIVE_SM_EVENT_TIMEOUT
} ptp_gptp_capable_receive_sm_event_t;

typedef enum {
	GPTP_CAPABLE_RECEIVE_SM_STATE_MIN_VALUE = 0,
	GPTP_CAPABLE_RECEIVE_SM_STATE_NOT_ENABLED = GPTP_CAPABLE_RECEIVE_SM_STATE_MIN_VALUE,
	GPTP_CAPABLE_RECEIVE_SM_STATE_INITIALIZE,
	GPTP_CAPABLE_RECEIVE_SM_STATE_RECEIVED_TLV,
	GPTP_CAPABLE_RECEIVE_SM_STATE_MAX_VALUE = GPTP_CAPABLE_RECEIVE_SM_STATE_RECEIVED_TLV
} ptp_gptp_capable_receive_sm_state_t;

/*
* gPTPCapableReceive variables 802.1AS-2020 - 10.4.2
*/
struct ptp_gptp_capable_receive_sm {
	/*
	10.4.2.1.1 rcvdGptpCapableTlv: A Boolean variable that notifies the current state machine when a
	Signaling message containing a gPTP-capable TLV is received. This variable is reset by the current state
	machine.
	*/
	bool rcvd_gptp_capable_tlv;

	/*
	10.4.2.1.2 rcvdSignalingMsgPtr: A pointer to a structure whose members contain the values of the fields of
	a Signaling message whose receipt is indicated by rcvdGptpCapableTlv (see 10.4.2.1.1).
	*/
	struct ptp_signaling_pdu *rcvd_signaling_msg_ptr;

	/*
	10.4.2.1.3 gPtpCapableReceiptTimeoutTimeInterval: The time interval after which, if a Signaling
	message containing a gPTP-capable TLV is not received, the neighbor of this PTP Port is considered to no
	longer be invoking gPTP. The data type for gPtpCapableReceiptTimeoutTimeInterval is UScaledNs.
	*/
	struct ptp_u_scaled_ns gptp_capable_receipt_timeout_time_interval;

	/*
	10.4.2.1.4 timeoutTime: A variable used to save the time at which the neighbor of this PTP Port is
	considered to no longer be invoking gPTP if a Signaling message containing a gPTP-capable TLV is not
	received. The data type for timeoutTime is UScaledNs.
	*/
	struct ptp_u_scaled_ns timeout_time;

	/*
	Custom additions (not specified in standard but needed for some computations)
	*/
	ptp_gptp_capable_receive_sm_state_t state;

	struct timer timeout_timer;
};


typedef enum {
	GPTP_CAPABLE_INTERVAL_SM_STATE_MIN_VALUE = 0,
	GPTP_CAPABLE_INTERVAL_SM_STATE_NOT_ENABLED = GPTP_CAPABLE_INTERVAL_SM_STATE_MIN_VALUE,
	GPTP_CAPABLE_INTERVAL_SM_STATE_INITIALIZE,
	GPTP_CAPABLE_INTERVAL_SM_STATE_SET_INTERVAL,
	GPTP_CAPABLE_INTERVAL_SM_STATE_MAX_VALUE = GPTP_CAPABLE_INTERVAL_SM_STATE_SET_INTERVAL
} ptp_gptp_capable_interval_setting_sm_state_t;

/*
* gPTPCapableIntervalSetting variables 802.1AS-2020 - 10.4.4
*/
struct ptp_gptp_capable_interval_setting_sm {
	/*
	10.4.3.1.1 rcvdSignalingMsg4: A Boolean variable that notifies the current state machine when a Signaling
	message that contains a gPTP-capable Message Interval Request TLV (see 10.6.4.5) is received. This
	variable is reset by the current state machine.
	*/
	bool rcvd_signaling_msg4;

	/*
	10.4.3.1.2 rcvdSignalingPtrGIS: A pointer to a structure whose members contain the values of the fields of
	the received Signaling message that contains a gPTP-capable Message Interval Request TLV (see 10.6.4.5).
	*/
	struct ptp_signaling_pdu *rcvd_signaling_ptr_gis;

	/*
	10.4.3.1.3 logSupportedGptpCapableMessageIntervalMax: The maximum supported logarithm to base 2
	of the gPTP-capable message interval. The data type for logSupportedGptpCapableMessageIntervalMax is
	Integer8.
	*/
	s8 log_supported_gptp_capable_message_interval_max;

	/*
	10.4.3.1.4 logSupportedClosestLongerGptpCapableMessageInterval: The logarithm to base 2 of the
	gPTP-capable message interval, such that logSupportedClosestLongerGptpCapableMessageInterval >
	logRequestedGptpCapableMessageInterval,that is numerically closest to
	logRequestedGptpCapableMessageInterval, where logRequestedGptpCapableMessageInterval is the
	argument of the function computeLogGptpCapableMessageInterval() (see 10.4.3.2.2). The data type for
	logSupportedClosestLongerGptpCapableMessageInterval is Integer8.
	*/
	s8 log_supported_closest_longer_gptp_capable_message_interval;

	/*
	10.4.3.1.5 computedLogGptpCapableMessageInterval: A variable used to hold the result of the function
	computeLogGptpCapableMessageInterval(). The data type for computedLogGptpCapableMessageInterval
	is Integer8.
	*/
	s8 computed_log_gptp_capable_message_interval;

	/*
	Custom additions (not specified in standard but needed for some computations)
	*/
	ptp_gptp_capable_interval_setting_sm_state_t state;
};


/*
A time-aware system contains one MD entity per port. This entity contains functions generic to all media
*/
struct ptp_instance_md_entity {
	struct ptp_instance_md_entity_globals globals;
	struct ptp_md_entity_sync_receive_sm sync_rcv_sm;
	struct ptp_md_entity_sync_send_sm sync_send_sm;
	struct md_sync_receive sync_rcv;
	struct md_sync_send sync_snd;
};

/* MD entity elements used by both Domain 0 instance and the CMLDS */
struct ptp_md_entity {
	struct ptp_md_entity_globals globals;
	struct ptp_md_entity_pdelay_req_sm pdelay_req_sm;
	struct ptp_md_entity_pdelay_resp_sm pdelay_resp_sm;
	struct ptp_md_link_delay_interval_setting_sm link_interval_sm;
};


typedef enum {
	SYNC_RCV_SM_STATE_MIN_VALUE = 0,
	SYNC_RCV_SM_STATE_DISCARD = SYNC_RCV_SM_STATE_MIN_VALUE,
	SYNC_RCV_SM_STATE_WAITING_FOR_FOLLOW_UP,
	SYNC_RCV_SM_STATE_WAITING_FOR_SYNC,
	SYNC_RCV_SM_STATE_MAX_VALUE = SYNC_RCV_SM_STATE_WAITING_FOR_SYNC
} ptp_sync_rcv_sm_state_t;

typedef enum {
	SYNC_RCV_SM_EVENT_MIN_VALUE = 0,
	SYNC_RCV_SM_EVENT_FUP_TIMEOUT = SYNC_RCV_SM_EVENT_MIN_VALUE,
	SYNC_RCV_SM_EVENT_RUN,
	SYNC_RCV_SM_EVENT_MAX_VALUE = SYNC_RCV_SM_EVENT_RUN
} ptp_sync_rcv_sm_event_t;

typedef enum {
	SYNC_SND_SM_STATE_MIN_VALUE = 0,
	SYNC_SND_SM_STATE_INITIALIZING = SYNC_SND_SM_STATE_MIN_VALUE,
	SYNC_SND_SM_STATE_SEND_SYNC,
	SYNC_SND_SM_STATE_SEND_FOLLOW_UP,
	SYNC_SND_SM_STATE_MAX_VALUE = SYNC_SND_SM_STATE_SEND_FOLLOW_UP
} ptp_sync_snd_sm_state_t;


typedef enum {
	SYNC_SND_SM_EVENT_MIN_VALUE = 0,
	SYNC_SND_SM_EVENT_RUN = SYNC_SND_SM_EVENT_MIN_VALUE,
	SYNC_SND_SM_EVENT_MAX_VALUE = SYNC_SND_SM_EVENT_RUN,
} ptp_sync_snd_sm_event_t;


typedef enum {
	LINK_INTERVAL_SM_STATE_MIN_VALUE = 0,
	LINK_INTERVAL_SM_STATE_NOT_ENABLED = LINK_INTERVAL_SM_STATE_MIN_VALUE,
	LINK_INTERVAL_SM_STATE_INITIALIZE,
	LINK_INTERVAL_SM_STATE_SET_INTERVALS,
	LINK_INTERVAL_SM_STATE_MAX_VALUE = LINK_INTERVAL_SM_STATE_SET_INTERVALS
} ptp_link_delay_interval_sm_state_t;


typedef enum {
	LINK_INTERVAL_SM_EVENT_MIN_VALUE = 0,
	LINK_INTERVAL_SM_EVENT_RCVD_MSG1 = LINK_INTERVAL_SM_EVENT_MIN_VALUE,
	LINK_INTERVAL_SM_EVENT_RUN,
	LINK_INTERVAL_SM_EVENT_MAX_VALUE = LINK_INTERVAL_SM_EVENT_RUN
} ptp_link_delay_interval_sm_event_t;

typedef enum {
	LINK_TRANSMIT_SM_STATE_INITIAL = 0,
	LINK_TRANSMIT_SM_STATE_OPER
} ptp_link_delay_transmit_sm_state_t;


typedef enum {
	LINK_TRANSMIT_SM_EVENT_INITIAL = 0,
	LINK_TRANSMIT_SM_EVENT_OPER,
	LINK_TRANSMIT_SM_EVENT_RATIO_NOT_VALID,
	LINK_TRANSMIT_SM_EVENT_RATIO_VALID
} ptp_link_delay_transmit_sm_event_t;


/***************** LocalClock entity *********************/

/*
The LocalClock entity is a free-running clock (see 3.3) that provides a common time to the time-aware
system, relative to an arbitrary epoch
*/
struct ptp_local_clock_entity {
	/* Non standard */
	unsigned int phase_discont;	 /* Incremented for each offset adjustment of the local clock */
	ptp_double rate_ratio_adjustment; /* current rate ratio adjustment applied to the local clock */
};


/***************** Common Mean Link Delay Service  *********************/

/*
commonServicesPortDS 802.1AS-2020 - 14.14

The commonServicesPortDS enables a PTP Port of a PTP Instance to determine which port of the respective
common service corresponds to that PTP Port.
*/
struct ptp_common_services_port_ds {
	/* The value is the portNumber attribute of the cmldsLinkPortDS.portIdentity
	of the Link Port that corresponds to this PTP Port. */
	u16 cmlds_link_port_port_number;
};

/*
cmldsDefaultDS 802.1AS-2020 - 14.15

The cmldsDefaultDS describes the per-time-aware-system attributes of the Common Mean Link Delay
Services
*/
struct ptp_cmlds_default_ds {
	struct ptp_clock_identity clock_identity;
	u16 number_link_ports;
};

/*
cmldsLinkPortDS 802.1AS-2020 - 14.16

The cmldsLinkPortDS represents time-aware Link Port capabilities for the Common Mean Link Delay
Service of a Link Port of a time-aware system
*/
struct ptp_cmlds_link_port_ds {
	struct ptp_port_identity port_identity;

	bool cmlds_link_port_enabled;

	bool is_measuring_delay;

	bool as_capable_across_domains;

	struct ptp_time_interval mean_link_delay;
	struct ptp_time_interval mean_link_delay_thresh;
	struct ptp_time_interval delay_asymmetry;

	s32 neighbor_rate_Ratio;

	s8 initial_log_pdelayreq_interval;
	s8 current_log_pdelayreq_interval;
	bool use_mgt_settable_log_pdelayreq_interval;
	s8 mgt_settable_log_pdelayreq_interval;

	bool initial_compute_neighbor_rate_ratio;
	bool current_compute_neighbor_rate_ratio;
	bool use_mgt_settable_compute_neighbor_rate_ratio;
	bool mgt_settable_compute_neighbor_rate_ratio;

	bool initial_compute_mean_link_delay;
	bool current_compute_mean_link_delay;
	bool use_mgt_settable_compute_mean_link_delay;
	bool mgt_settable_compute_mean_link_delay;

	u8 allowed_lost_responses;
	u8 allowed_faults;

	u8 version_number;

	struct ptp_timestamp pdelay_truncated_timestamps_array[4];

	u8 minor_version_number;
};

/*
cmldsLinkPortStatisticsDS - 802.1AS-2020 - 14.17

There is one cmldsLinkPortStatisticsDS table per Link Port of a time-aware system. The
cmldsLinkPortStatisticsDS table contains a set of counters for each Link Port that supports the time-
synchronization capability
*/
struct ptp_cmlds_link_port_statistics_ds {
	u32 rx_pdelay_request_count;
	u32 rx_pdelay_response_count;
	u32 rx_pdelay_response_follow_up_count;
	u32 rx_ptp_packet_discard_count;
	u32 pdelay_allowed_lost_responses_exceeded_count;
	u32 tx_pdelay_request_count;
	u32 tx_pdelay_response_count;
	u32 tx_pdelay_response_follow_up_count;
};

/*
cmldsAsymmetryMeasurementModeDS - 802.1AS-2020 - 14.18

For every Link Port of the Common Mean Link Delay Service of a time-aware system, the
cmldsAsymmetryMeasurementModeDS contains the single member asymmetryMeasurementMode, which
is used to enable/disable the Asymmetry Compensation Measurement Procedure
*/
struct ptp_cmlds_asymmetry_measurement_mode_ds {
	bool asymmetry_measurement_mode;
};

#endif /* _PTP_H_ */
