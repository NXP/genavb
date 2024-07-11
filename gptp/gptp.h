/*
 * Copyright 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief GPTP main header file
 @details Definition of GPTP stack component entry point functions and global context structure.
*/

#ifndef _GPTP_H_
#define _GPTP_H_

#include "os/stdlib.h"
#include "os/clock.h"

#include "genavb/init.h"

#include "common/net.h"
#include "common/timer.h"
#include "common/ptp.h"
#include "common/types.h"
#include "common/filter.h"
#include "common/log.h"
#include "common/ipc.h"

#include "gptp_managed_objects.h"

#include "config.h"
#include "target_clock_adj.h"

#include "gptp/gptp_entry.h"

#define gptp_port_to_clock(p) ((p)->instance->gptp->clock_local)
#define gptp_port_common_to_clock(p) ((p)->gptp->clock_local)

/*Note: bit8 to bit15 are reserve for bridge port id in sja1105 layer */
#define gptp_ts_info(sdoid, domain, msgtype) (((sdoid) << 24) | ((domain) << 16) | (msgtype))
#define gptp_ts_info_msgtype(ts_info) ((ts_info) & 0xff)
#define gptp_ts_info_domain(ts_info) (((ts_info) & 0xff0000) >> 16)
#define gptp_ts_info_sdoid(ts_info) (((ts_info) & 0xff000000) >> 24)

struct gptp_net_port_stats {
	u32 num_rx_pkts;
	u32 num_tx_pkts;

	u32 num_tx_err;
	u32 num_tx_err_alloc;
	u32 num_rx_err_etype;
	u32 num_rx_err_portid;

	u32 num_rx_err_domain;
	u32 num_rx_err_sdoid;
	u32 num_err_domain_unknown;

	u32 num_hwts_request;
	u32 num_hwts_handler;
};

struct gptp_instance_stats {
	/* per instance statistics */
	u32 num_adjust_on_sync;
	u32 num_synchro_loss;
};

struct gptp_instance_port_stats {
	/* per port statistics */
	u32 num_rx_pkts;
	u32 num_rx_sync;
	u32 num_rx_sync_timeout;
	u32 num_rx_fup;
	u32 num_rx_fup_timeout;
	u32 num_rx_announce;
	u32 num_rx_announce_dropped;
	u32 num_rx_announce_timeout;
	u32 num_rx_sig;

	u32 num_tx_pkts;
	u32 num_tx_sync;
	u32 num_tx_fup;
	u32 num_tx_announce;
	u32 num_tx_sig;

	u32 num_md_sync_rcv_sm_reset;

	u32 num_not_as_capable;
};

/* IEEE 8021AS-2020 - sections 14.10 and 14.17 */
struct gptp_port_stats {
	u64 peer_clock_id;

	u32 num_rx_pdelayreq;
	u32 num_rx_pdelayresp;
	u32 num_rx_pdelayrespfup;
	u32 num_rx_ptp_packet_discard;
	u32 num_rx_pdelayresp_lost_exceeded;

	u32 num_tx_pdelayreq;
	u32 num_tx_pdelayresp;
	u32 num_tx_pdelayrespfup;

	u32 num_md_pdelay_req_sm_reset;
};

/* Per common port parameters */
struct gptp_port_common_config {
	/* Automotive profile params */
	u8 neighborPropDelay_mode;					/* set to 1 if predefined pdelay mechanism is used. 0 means relying on standard pdelay exchange */
	double initial_neighborPropDelay;	/* expressed in ns. per port predefined value, but filled using single value from the default configuration file */
	double neighborPropDelay_sensitivity;			/* expressed in ns. global to all ports. defined the amount of ns between to pdelay measurement that would trigger a new indication to the host main code */
};

/**
 * GPTP port common definition (used by CMLDS and Domain 0)
 */
struct gptp_port_common {
	/* contains all user configurable parameters */
	struct gptp_port_common_config cfg;

	struct gptp_ctx *gptp;

	unsigned int port_id;

	/* 802.1AS - 8.5.2 */
	struct ptp_port_identity identity;

	/* per instance and per port entity (used by CMLDS and Domain 0) */
	struct ptp_md_entity md;

	/* per port and per link params (used by CMLDS and Domain 0) */
	struct ptp_port_params params;

	bool use_mgt_settable_log_pdelayreq_interval;
	s8 mgt_settable_log_pdelayreq_interval;

	bool use_mgt_settable_compute_neighbor_rate_ratio;
	bool mgt_settable_compute_neighbor_rate_ratio;

	bool use_mgt_settable_compute_mean_link_delay;
	bool mgt_settable_compute_mean_link_delay;

	/* port supported features */
	/* if set to 1 this port should not transmit any pdelay request to the network.
	 * Used both in automotive profile and 802.1AS-2020. This is a non standard
	 * option for 802.1AS-2020, used to prevent pdelay transmission, in domain0,
	 * when CMLDS is used
	 */
	bool pdelay_transmit_enabled;
	bool as_capable_static; /* set to 1 if the link is always considered as AS capable, else 802.1AS capability results from the pdelay mechanism */

	/* static pdelay feature */
	struct gptp_pdelay_info pdelay_info;

	struct gptp_port_stats stats;
	struct stats pdelay_stats;

	struct ptp_signaling_pdu signaling_rx;
};


/* Per gPTP instance  port parameters */
struct gptp_instance_port_config {
	s8 operLogPdelayReqInterval;
	s8 operLogSyncInterval;
};


/**
 * GPTP port definition
 */
struct gptp_port {
	/* contains all per port user configurable parameters */
	struct gptp_instance_port_config cfg;

	struct gptp_instance *instance;

	unsigned int port_id;

	/* CMLDS's link-port associated to this gptp port */
	u16 link_id;

	struct gptp_port_common *c;

	struct ptp_instance_port_params params;

	/* per port entities and state machines */
	struct ptp_port_sync_entity port_sync;
	struct ptp_instance_md_entity md;
	struct port_announce_interval_sm announce_interval_sm;
	struct port_announce_transmit_sm announce_transmit_sm;
	struct ptp_sync_interval_setting_sm sync_interval_sm;
	struct ptp_gptp_capable_receive_sm gptp_capable_receive_sm;
	struct ptp_gptp_capable_transmit_sm gptp_capable_transmit_sm;
	struct ptp_gptp_capable_interval_setting_sm gptp_capable_interval_sm;

	/* timers */
	struct timer sync_receive_timeout_timer;
	struct timer follow_up_receive_timeout_timer;
	struct timer rsync_timer;

	/* 802.1AS-2020 - section 10.7.3.1 number of sync intervals to wait without receiving
	synchronization information, before assuming that the master is no longer
	transmitting synchronization information */
	u8 sync_receipt_timeout;

	/* 802.1AS-2020 - section 10.7.3.2 number of announce intervals to wait without receiving
	an Announce message, before assuming that the master is no longer transmitting
	Announce messages */
	u8 announce_receipt_timeout;

	/* 802.1AS-2020 - 10.7.3.3 number of gPTP-capable intervals to wait without
	receiving from its neighbor a signaling message containing a gPTP-capable TLV,
	before determining its neighbor is no longer invoking anymore gPTP. */
	u8 gptp_capable_receipt_timeout;

	/* 802.1AS-2020 - section 14.8 */
	bool use_mgt_settable_log_sync_interval;
	s8 mgt_settable_log_sync_interval;

	bool use_mgt_settable_log_announce_interval;
	s8 mgt_settable_log_announce_interval;

	bool use_mgt_settable_log_gptp_capable_message_interval;
	s8 mgt_settable_log_gptp_capable_message_interval;

	/* control ratio/pdelay calculation on GM side via signaling messages */
	bool ratio_is_valid;

	/* port supported features */
	bool gm_id_static; /* set to 1 if GM Id is hard coded, else grandmaster results from the BMCA algorithm */
	bool link_up_static; /* set to 1 if the ethernet link is always considered up, else link state is results from the platform's network abstraction layer */
	bool fast_link_interval; /* if set to 1, send signaling message to master to increase sync/pdelay packets rate */

	/* pdu's */
	struct ptp_announce_pdu announce_tx;
	struct ptp_sync_pdu sync_tx;
	struct ptp_follow_up_pdu follow_up_tx;
	struct ptp_announce_pdu announce_rx;
	struct ptp_sync_pdu sync_rx;
	struct ptp_follow_up_pdu follow_up_rx;

	struct ptp_signaling_pdu signaling_tx;
	struct ptp_signaling_pdu signaling_rx;

	/* timestamps */
	u64 sync_rx_ts;
	u64 sync_tx_ts;

	/* tx/rx sequence number */
	u16 sync_tx_seqnum;
	u16 announce_rx_seqnum;
	u16 announce_tx_seqnum;
	u16 signaling_rx_seqnum;
	u16 signaling_tx_seqnum;

	ptp_port_announce_info_sm_state_t port_announce_info_sm_state;
	ptp_sync_rcv_sm_state_t sync_rcv_sm_state;
	ptp_sync_snd_sm_state_t sync_snd_sm_state;
	ptp_port_sync_sync_rcv_sm_state_t port_sync_sync_rcv_sm_state;
	ptp_clock_slave_sync_sm_state_t clock_slave_sync_sm_state;
	ptp_link_delay_transmit_sm_state_t link_transmit_sm_state;

	struct gptp_sync_info sync_info;
	u64 nosync_time_ns;
	u64 sync_time_ns;

	/* reverse sync feature control */
	u8 rsync_running;

	/* control ratio/pdelay calculation on GM side via signaling messages */
	unsigned int ratio_last_num_tx_pdelayresp;

	struct gptp_instance_port_stats stats;
};


/* Per gPTP instance  parameters */
struct gptp_instance_config {
	u64 gm_id; /* grand master id used in case of static slave configuration or resulting from bmca */
};

/* General parameters */
struct gptp_global_config {
	unsigned int profile; /* gptp profile (standard, automotive) */

	bool rsync; /* set to 1 to enable rsync feature */
	unsigned int rsync_interval; /* defines rsync packet interval in ms*/

	unsigned int statsInterval; /* defines the interval in second between statistics output */
};

/**
 * GPTP instance structure
 */
struct gptp_instance {
	/* contains all per instance user configurable parameters */
	struct gptp_instance_config cfg;

	struct timer_ctx timer_ctx;

	/* the time-aware system the instance belongs to */
	struct gptp_ctx *gptp;

	/* instance 's domain */
	struct ptp_domain domain;

	/* instance index within the time-aware system instances array */
	u8 index;

	/* to check wether this system is the GM or not (set by the BMCA) */
	bool is_grandmaster;

	/* add proper layers and per domain */
	os_clock_id_t clock_target;
	os_clock_id_t clock_source;

	struct target_clkadj_params target_clkadj_params;

	/* per time aware system entities */
	struct ptp_clock_slave_entity clock_slave;
	struct ptp_site_sync_entity site_sync;
	struct ptp_clock_master_entity clock_master;

	/* per instance global variables */
	struct ptp_instance_params params;

	/* managed objects without specific storage */
	u8 numberPorts;	/* 14.2.2 */
	bool gmCapable; /* 14.2.8 */
	u32 gmChangeCount; /* 14.3.6 */
	u32 timeOfLastGmChangeEvent; /* 14.3.7 */
	u32 timeOfLastGmPhaseChangeEvent; /* 14.3.8 */
	u32 timeOfLastGmFreqChangeEvent; /* 14.3.9 */
	s32 cumulativeRateRatio; /* 14.4.2 */
	u8 versionNumber; /* 14.6.21 */

	s8 clock_master_log_sync_interval;
	ptp_port_state_sm_state_t port_state_sm_state;
	ptp_site_sync_sync_sm_state_t site_sync_sync_sm_state;
	ptp_clock_master_sync_send_sm_state_t clock_master_sync_send_sm_state;
	ptp_clock_master_sync_receive_sm_state_t clock_master_sync_receive_sm_state;
	ptp_clock_master_sync_offset_sm_state_t clock_master_sync_offset_sm_state;

	/* timers */
	struct timer clock_master_sync_send_timer;

	struct gptp_gm_info gm_info;

	struct gptp_instance_stats stats;

	/* per port gptp domains */
	/* variable size array */
	struct gptp_port ports[];
};

struct gptp_net_port {
	struct net_rx net_rx;
	struct net_tx net_tx;

	unsigned int port_id;
	unsigned int logical_port;
	bool initialized;
	bool port_oper; /* 802.1.AS-2020 - 10.2.5.12 a Boolean that is set if the time-aware system's MAC is operational (Tx/Rx frames, Spanning Tree, Relay) */
	bool asymmetry_measurement_mode;

	/* per port per direction timestamping compensation */
	int rx_delay_compensation;
	int tx_delay_compensation;

	struct gptp_net_port_stats stats;
};


/**
 * Link-Port (802.1AS-2020 - 11.2.17)
 *
 * In accordance with IEEE Std 1588-2019 the term Link-Port refers to a port of the CMLDS.
 */
struct gptp_link_port {
	/* 802.1-AS-2020 - 11.2.18.1
	A per-Link-Port Boolean that is TRUE if both the value of portDS.delayMechanism is Common_P2P
	and the value of portDS.ptpPortEnabled is TRUE, for at least one PTP Port that uses the CMLDS that
	is invoked on the Link Port; otherwise, the value is FALSE.
	*/
	bool cmlds_link_port_enabled;

	struct gptp_port_common c;
};

#define CFG_LINK_NUM_PORT CFG_GPTP_MAX_NUM_PORT

/**
 * Common Mean Link Delay context (802.1AS-2020 - 11.2.17)
 *
 * The CMLDS makes the mean propagation delay and neighbor rate ratio available
 * to all active domains per Link-Port.
 */
struct cmlds_ctx {
	struct gptp_link_port link_ports[CFG_LINK_NUM_PORT];

	struct ptp_clock_identity this_clock;
	u16 number_link_ports;
};


/**
 * GPTP Time-aware system global context
 */
struct gptp_ctx {
	/* contains all user configurable parameters */
	struct gptp_global_config cfg;

	unsigned int domain_max;
	struct gptp_instance *instances[CFG_MAX_GPTP_DOMAINS];

	struct gptp_port_common common_ports[CFG_GPTP_MAX_NUM_PORT];

	bool management_enabled;

	/* IEEE 802.1AS-2011 standard version only */
	bool force_2011;

	struct cmlds_ctx cmlds;

	/* Managed objects data tree */
	struct gptp_managed_objects module;

	os_clock_id_t clock_local;
	os_clock_id_t clock_monotonic;

	struct ptp_local_clock_entity local_clock;

	struct timer_ctx *timer_ctx;

	struct timer stats_timer;

	void (*sync_indication)(struct gptp_sync_info *info);
	void (*gm_indication)(struct gptp_gm_info *info);
	void (*pdelay_indication)(struct gptp_pdelay_info *info);

	struct ipc_rx ipc_rx;
	struct ipc_tx ipc_tx_sync;
	struct ipc_tx ipc_tx;

	struct ipc_rx ipc_rx_mac_service;
	struct ipc_tx ipc_tx_mac_service;

	unsigned int port_max;

	/* variable size array */
	struct gptp_net_port net_ports[];
};

int gptp_net_tx(struct gptp_net_port *net_port, void *msg, int msg_len, unsigned char ts_required, u8 domain_number, u8 sdoid);
int gptp_system_time(struct gptp_ctx *gptp, u64 *ns);
void gptp_gm_indication(struct gptp_instance *instance, struct ptp_priority_vector *gm_vector);
void gptp_sync_indication(struct gptp_port *port);
void gptp_ipc_gm_status(struct gptp_instance *instance, struct ipc_tx *ipc, unsigned int ipc_dst);
void gptp_ipc_gptp_port_params(struct gptp_instance *instance, struct gptp_port *port, struct ipc_tx *ipc, unsigned int ipc_dst);
void gptp_ratio_invalid(struct gptp_port *port);
void gptp_ratio_valid(struct gptp_port *port);

void gptp_port_update_fsm(struct gptp_port *port);

void gptp_as_capable_across_domains_down(struct gptp_port_common *port);
void gptp_as_capable_across_domains_up(struct gptp_port_common *port);

void gptp_neighbor_gptp_capable_down(struct gptp_port *port);
void gptp_neighbor_gptp_capable_up(struct gptp_port *port);

void gptp_update_as_capable(struct gptp_port *port);

const char *gptp_port_role2string(ptp_port_role_t port_role);

void gptp_instance_priority_vector_update(void *data);

struct ptp_u_scaled_ns *get_mean_link_delay(struct gptp_port *port);
ptp_double get_neighbor_rate_ratio(struct gptp_port *port);
struct ptp_scaled_ns *get_delay_asymmetry(struct gptp_port *port);

struct ptp_port_identity *get_port_identity(struct gptp_port *port);
u16 get_port_identity_number(struct gptp_port *port);
u8 *get_port_identity_clock_id(struct gptp_port *port);

bool get_port_oper(struct gptp_port *port);
bool get_port_asymmetry(struct gptp_port *port);

struct gptp_net_port *net_port_from_gptp(struct gptp_port *port);
struct gptp_net_port *net_port_from_gptp_common(struct gptp_port_common *port_common);



#endif /* _GPTP_H_ */
