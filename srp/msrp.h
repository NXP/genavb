/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2018-2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
  @file		msrp.h
  @brief	MSRP module interface.
  @details	All prototypes and definitions necessary for MSRP module usage
		are provided within this header file.
*/


#ifndef _MSRP_H
#define _MSRP_H

#include "common/net.h"
#include "common/list.h"
#include "common/srp.h"

#include "mrp.h"

/* 802.1Q, section 35.2.4.5 MAP Context for MSRP */
#define MSRP_MAX_MAP_CONTEXT 1

/**
 * MSRP reservation instance definition
 */
struct msrp_reservation {
	bool valid;
	u32 port_id;
	u16 direction;
	u32 declaration_type;
	u32 accumulated_latency;
	u64 failed_bridge_id;
	u32 failure_code;
	u64 dropped_frames;
	u32 stream_age; /**< 802.1Q-2011 35.2.1.4 c)  per port , per stream variable */
};

/**
 * MSRP stream port instance definition
 */
struct msrp_stream_port {
	msrp_listener_declaration_type_t listener_declaration_type;	/**< 802.1Q -35.2.1.3 */
	msrp_talker_declaration_type_t talker_declaration_type;		/**< 802.1Q -35.2.1.3 */

	struct list_head registered_talker_attributes;		/**< List of currently registered talker attributes */
	struct list_head registered_listener_attributes;		/**< List of currently registered listener attributes */
	struct mrp_attribute *declared_talker_attribute;		/**< Pointer to declared talker attribute */
	struct mrp_attribute *declared_listener_attribute;	/**< Pointer to declared listener attribute */

	struct msrp_reservation reservations[2];	/**< stream reservation table 802.1Q - 12.22.5 */
};

/**
 * MSRP stream instance definition
 */
struct msrp_stream {
	struct list_head list;
	struct msrp_map *map;				/**< pointer to the msrp map context */
	sr_class_t sr_class;
	struct msrp_pdu_fv_talker_failed fv;	/**< MSRP talker failed PDU infos used for MSRP listener, talker advertise and talker failed */

	u16 listener_registered_state;			/**< Bitmask (bit per port): current state of the listener attribute for this stream */
	u16 listener_declared_state;			/**< Bitmask (bit per port): current state of the listener attribute for this stream */
	u16 listener_user_declared_state;		/**< Bitmask (bit per port): user requested state of the listener attribute for this stream */
	u16 talker_registered_state;			/**< Bitmask (bit per port): current state of the talker attribute for this stream */
	u16 talker_declared_state;				/**< Bitmask (bit per port): current state of the talker attribute for this stream */
	u16 talker_user_declared_state;			/**< Bitmask (bit per port): user requested state of the talker attribute for this stream */
	u16 fdb_state;							/**< Bitmask (bit per port): state of the port (Forwarding-1/Filtering-0) */
	u16 fqtss_state;					/**< Bitmask (bit per port): current state of fqtss for this stream (added-1/removed-0) */
	unsigned int vlan_state;				/**< current state of the vlan  for this stream */

	/* variable size array */
	struct msrp_stream_port port[];
};

/**
 * MSRP domain instance definition
 */
struct msrp_domain {
	struct list_head list;
	struct msrp_port *port;		/**< MSRP port context this domain is attached to */
	struct msrp_pdu_fv_domain fv;	/**< MSRP domain specific infos */
	unsigned int state;
};

struct msrp_latency_parameter {
	u16 port_id;
	u32 traffic_class;
	u32 port_tc_max_latency;
};

/**
 * MSRP port context structure
 */
struct msrp_port {
	unsigned int port_id;
	unsigned int logical_port;

	struct srp_port *srp_port;

	unsigned int flags;

	struct msrp_latency_parameter latency[CFG_TRAFFIC_CLASS_MAX];		/**< 802.1Q-2011 35.2.1.4 a) */

	bool msrp_port_enabled_status;		/**< 802.1Q-2011 35.2.1.4 e) */
	u64 failed_registrations;			/**< 802.1Q-2011 10.7.12.1) */
	unsigned char last_pdu_origin[6];			/**< 802.1Q-2011 10.7.12.2) */
	unsigned int srp_domain_boundary_port[CFG_MSRP_MAX_CLASSES];	/**< 802.1Q-2011 35.2.1.4 h) */
	unsigned short sr_pvid;						/**< 802.1Q-2011 35.2.1.4 i) */

	struct msrp_domain *domain[CFG_MSRP_MAX_CLASSES];		/* Keep track of the domains we declared */

	struct list_head domains;		/**< chained list of the domains */
	struct mrp_application	mrp_app;	/**< MRP generic data associated to MSRP */
	unsigned int num_domains;
	unsigned int num_rx_pkts;
	unsigned int num_tx_pkts;
	unsigned int num_tx_err;
};

/**
 * MSRP MAP context structure
 */
struct msrp_map {
	unsigned int map_id;
	struct list_head streams;		/**< chained list of the streams */
	unsigned int num_streams;
	u16 forwarding_state; /**< Bitmask (bit per port): state of the port (Forwarding/Discarding/...) */
};

/**
 * MSRP global context structure
 */
struct msrp_ctx {
	struct msrp_map map[MSRP_MAX_MAP_CONTEXT];		/* 802.1Q, section 35.2.4.5. MAP context.for MSRP */

	struct srp_ctx *srp;			/**< reference to the main SRP context that includes MSRP/MVRP and MMRP components */

	bool is_bridge;

	struct ipc_rx ipc_rx_avdecc;		/**< receive context for AVDECC IPC */

	struct ipc_rx ipc_rx;			/**< receive context for MEDIA STACK IPC */
	struct ipc_tx ipc_tx;			/**< transmit context for MSRP IPC */
	struct ipc_tx ipc_tx_sync;		/**< transmit context for MSRP sync IPC */

	bool talker_pruning;			/**< 802.1Q-2011 35.2.1.4 b) */
	bool msrp_enabled_status;		/**< 802.1Q-2011 35.2.1.4 d) */
	u32 msrp_max_fan_in_ports;		/**< 802.1Q-2011 35.2.1.4 f) */
	u32 msrp_latency_max_frame_size;	/**< 802.1Q-2011 35.2.1.4 g) */

	u16 operational_state; /**< Bitmask (bit per port): link state of the port (up/down) */
	u16 enabled_state; /**< Bitmask (bit per port): msrp enabled state of the port */

	unsigned int port_max;

	/* variable size array */
	struct msrp_port port[];
};


struct __attribute__ ((packed)) msrp_listener_attribute_value {
	u64 stream_id;
	u8 declaration_type;
};

#define msrp_talker_advertise_attribute_value msrp_pdu_fv_talker_advertise
#define msrp_talker_failed_attribute_value msrp_pdu_fv_talker_failed
#define msrp_domain_attribute_value msrp_pdu_fv_domain

unsigned int msrp_to_logical_port(struct msrp_ctx *msrp, unsigned int port_id);
int msrp_talker_attribute_cmp(msrp_attribute_type_t attr_type, u8 *val, msrp_attribute_type_t new_attr_type, u8 *new_val);
int msrp_port_enable(struct msrp_port *port);
void msrp_port_disable(struct msrp_port *port);
void msrp_enable(struct msrp_ctx *msrp);
void msrp_disable(struct msrp_ctx *msrp);
int msrp_init(struct msrp_ctx *msrp, struct msrp_config *cfg, unsigned long priv);
int msrp_exit(struct msrp_ctx *msrp);
int msrp_process_packet(struct msrp_ctx *msrp, unsigned int port_id, struct net_rx_desc *desc);
void msrp_register_vlan(struct msrp_port *port, struct msrp_stream *stream);
void msrp_deregister_vlan(struct msrp_port *port, struct msrp_stream *stream);
void msrp_stream_free(struct msrp_stream *stream);

void msrp_port_status(struct msrp_ctx *msrp, struct ipc_mac_service_status *status);

#endif  /* _MSRP_H */
