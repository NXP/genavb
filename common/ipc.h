/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief IPC Service implementation
 @details
*/

#ifndef _COMMON_IPC_H_
#define _COMMON_IPC_H_

#include "common/types.h"
#include "genavb/genavb.h"

/* IPC return codes */
#define IPC_TX_ERR_NO_READER	1 /**< An IPC message was sent but no reader was listening */
#define IPC_TX_ERR_QUEUE_FULL	2 /**< An IPC message was sent but the IPC queue was full */
#define IPC_TX_ERR_UNKNOWN		3 /**< An unknown error happened while trying to send an IPC */

/* IPC identifiers
 * Identifiers are always in groups of three.
 * The first member of the group is intendend for commands (from the external application to a stack component),
 * the second for indications/async responses (from a stack component to an external application) and
 * the third one for sync responses (from a stack component to an external application).
 * command IPCs can be of the type "many to one"
 * response and indication IPCs can be of the type "one to many"
 */
typedef enum {
	/* AVDECC */
	IPC_MEDIA_STACK_AVDECC = 0,
	IPC_AVDECC_MEDIA_STACK,
	IPC_UNUSED_0,

	/* AVDECC controller */
	IPC_CONTROLLER_AVDECC,
	IPC_AVDECC_CONTROLLER,
	IPC_AVDECC_CONTROLLER_SYNC,

	/* AVDECC controlled */
	IPC_CONTROLLED_AVDECC,
	IPC_AVDECC_CONTROLLED,
	IPC_UNUSED_1,

	/* MSRP Endpoint 0 */
	IPC_MEDIA_STACK_MSRP,
	IPC_MSRP_MEDIA_STACK,
	IPC_MSRP_MEDIA_STACK_SYNC,

	/* MSRP Bridge */
	IPC_MEDIA_STACK_MSRP_BRIDGE,
	IPC_MSRP_BRIDGE_MEDIA_STACK,
	IPC_MSRP_BRIDGE_MEDIA_STACK_SYNC,

	/* MVRP Endpoint 0 */
	IPC_MEDIA_STACK_MVRP,
	IPC_MVRP_MEDIA_STACK,
	IPC_MVRP_MEDIA_STACK_SYNC,

	/* MVRP Bridge */
	IPC_MEDIA_STACK_MVRP_BRIDGE,
	IPC_MVRP_BRIDGE_MEDIA_STACK,
	IPC_MVRP_BRIDGE_MEDIA_STACK_SYNC,

	/* Clock domain */
	IPC_MEDIA_STACK_CLOCK_DOMAIN,
	IPC_CLOCK_DOMAIN_MEDIA_STACK,
	IPC_CLOCK_DOMAIN_MEDIA_STACK_SYNC,

	/* MAAP */
	IPC_MEDIA_STACK_MAAP,
	IPC_MAAP_MEDIA_STACK,
	IPC_MAAP_MEDIA_STACK_SYNC,

	/* gPTP Endpoint 0 */
	IPC_MEDIA_STACK_GPTP,
	IPC_GPTP_MEDIA_STACK,
	IPC_GPTP_MEDIA_STACK_SYNC,

	/* gPTP Bridge */
	IPC_MEDIA_STACK_GPTP_BRIDGE,
	IPC_GPTP_BRIDGE_MEDIA_STACK,
	IPC_GPTP_BRIDGE_MEDIA_STACK_SYNC,

	/* AVTP */
	IPC_MEDIA_STACK_AVTP,
	IPC_AVTP_MEDIA_STACK,
	IPC_AVTP_MEDIA_STACK_SYNC,

	IPC_AVTP_STATS,
	IPC_UNUSED_2,
	IPC_UNUSED_3,

	/* MAC Service Endpoint 0 */
	IPC_MEDIA_STACK_MAC_SERVICE,
	IPC_MAC_SERVICE_MEDIA_STACK,
	IPC_MAC_SERVICE_MEDIA_STACK_SYNC,

	/* MAC Service Bridge */
	IPC_MEDIA_STACK_MAC_SERVICE_BRIDGE,
	IPC_MAC_SERVICE_BRIDGE_MEDIA_STACK,
	IPC_MAC_SERVICE_BRIDGE_MEDIA_STACK_SYNC,

	/* MSRP Endpoint 1 */
	IPC_MEDIA_STACK_MSRP_1,
	IPC_MSRP_1_MEDIA_STACK,
	IPC_MSRP_1_MEDIA_STACK_SYNC,

	/* MVRP Endpoint 1 */
	IPC_MEDIA_STACK_MVRP_1,
	IPC_MVRP_1_MEDIA_STACK,
	IPC_MVRP_1_MEDIA_STACK_SYNC,

	/* MAC Service Endpoint 1 */
	IPC_MEDIA_STACK_MAC_SERVICE_1,
	IPC_MAC_SERVICE_1_MEDIA_STACK,
	IPC_MAC_SERVICE_1_MEDIA_STACK_SYNC,

	/* gPTP Endpoint 1 */
	IPC_MEDIA_STACK_GPTP_1,
	IPC_GPTP_1_MEDIA_STACK,
	IPC_GPTP_1_MEDIA_STACK_SYNC,

	/* HSR */
	IPC_HSR_STACK,
	IPC_UNUSED_4,
	IPC_UNUSED_5,

	/* SRP Endpoint - bridge communication */
	IPC_SRP_ENDPOINT_BRIDGE,
	IPC_SRP_BRIDGE_ENDPOINT,
	IPC_UNUSED_6,

	/* AVTP - AVDECC communication */
	IPC_AVDECC_AVTP,
	IPC_AVTP_AVDECC,
	IPC_UNUSED_7,

	IPC_ID_MAX,

	IPC_ID_NONE
} ipc_id_t;

#define	IPC_MEDIA_STACK_MSRP_0			IPC_MEDIA_STACK_MSRP
#define	IPC_MSRP_0_MEDIA_STACK			IPC_MSRP_MEDIA_STACK
#define	IPC_MSRP_0_MEDIA_STACK_SYNC		IPC_MSRP_MEDIA_STACK_SYNC

#define	IPC_MEDIA_STACK_MVRP_0			IPC_MEDIA_STACK_MVRP
#define	IPC_MVRP_0_MEDIA_STACK			IPC_MVRP_MEDIA_STACK
#define	IPC_MVRP_0_MEDIA_STACK_SYNC		IPC_MVRP_MEDIA_STACK_SYNC

#define	IPC_MEDIA_STACK_MAC_SERVICE_0		IPC_MEDIA_STACK_MAC_SERVICE
#define	IPC_MAC_SERVICE_0_MEDIA_STACK		IPC_MAC_SERVICE_MEDIA_STACK
#define	IPC_MAC_SERVICE_0_MEDIA_STACK_SYNC	IPC_MAC_SERVICE_MEDIA_STACK_SYNC

#define	IPC_MEDIA_STACK_GPTP_0			IPC_MEDIA_STACK_GPTP
#define	IPC_GPTP_0_MEDIA_STACK			IPC_GPTP_MEDIA_STACK
#define	IPC_GPTP_0_MEDIA_STACK_SYNC		IPC_GPTP_MEDIA_STACK_SYNC

/* IPC types */
enum {
	IPC_TYPE_SINGLE_READER_WRITER = 0,	/* Single writer, reader */
	IPC_TYPE_MANY_READERS,			/* Many readers, single writer. Meant for indications and responses. */
	IPC_TYPE_MANY_WRITERS,			/* Many writers, single reader. Meant for commands. */
};

#define IPC_MEDIA_STACK_AVDECC_TYPE		IPC_TYPE_SINGLE_READER_WRITER
#define IPC_AVDECC_MEDIA_STACK_TYPE		IPC_TYPE_SINGLE_READER_WRITER

#define IPC_CONTROLLER_AVDECC_TYPE		IPC_TYPE_SINGLE_READER_WRITER
#define IPC_AVDECC_CONTROLLER_TYPE		IPC_TYPE_SINGLE_READER_WRITER
#define IPC_AVDECC_CONTROLLER_SYNC_TYPE		IPC_TYPE_SINGLE_READER_WRITER

#define IPC_CONTROLLED_AVDECC_TYPE		IPC_TYPE_SINGLE_READER_WRITER
#define IPC_AVDECC_CONTROLLED_TYPE		IPC_TYPE_SINGLE_READER_WRITER

#define IPC_MEDIA_STACK_MSRP_TYPE		IPC_TYPE_MANY_WRITERS
#define IPC_MSRP_MEDIA_STACK_TYPE		IPC_TYPE_MANY_READERS
#define IPC_MSRP_MEDIA_STACK_SYNC_TYPE		IPC_TYPE_MANY_READERS

#define IPC_MEDIA_STACK_MSRP_1_TYPE		IPC_TYPE_MANY_WRITERS
#define IPC_MSRP_1_MEDIA_STACK_TYPE		IPC_TYPE_MANY_READERS
#define IPC_MSRP_1_MEDIA_STACK_SYNC_TYPE	IPC_TYPE_MANY_READERS

#define IPC_MEDIA_STACK_MSRP_BRIDGE_TYPE	IPC_TYPE_MANY_WRITERS
#define IPC_MSRP_BRIDGE_MEDIA_STACK_TYPE	IPC_TYPE_MANY_READERS
#define IPC_MSRP_BRIDGE_MEDIA_STACK_SYNC_TYPE	IPC_TYPE_MANY_READERS

#define IPC_MEDIA_STACK_MVRP_TYPE		IPC_TYPE_MANY_WRITERS
#define IPC_MVRP_MEDIA_STACK_TYPE		IPC_TYPE_MANY_READERS
#define IPC_MVRP_MEDIA_STACK_SYNC_TYPE		IPC_TYPE_MANY_READERS

#define IPC_MEDIA_STACK_MVRP_1_TYPE		IPC_TYPE_MANY_WRITERS
#define IPC_MVRP_1_MEDIA_STACK_TYPE		IPC_TYPE_MANY_READERS
#define IPC_MVRP_1_MEDIA_STACK_SYNC_TYPE	IPC_TYPE_MANY_READERS

#define IPC_MEDIA_STACK_MVRP_BRIDGE_TYPE	IPC_TYPE_MANY_WRITERS
#define IPC_MVRP_BRIDGE_MEDIA_STACK_TYPE	IPC_TYPE_MANY_READERS
#define IPC_MVRP_BRIDGE_MEDIA_STACK_SYNC_TYPE	IPC_TYPE_MANY_READERS

#define IPC_MEDIA_STACK_CLOCK_DOMAIN_TYPE	IPC_TYPE_MANY_WRITERS
#define IPC_CLOCK_DOMAIN_MEDIA_STACK_TYPE	IPC_TYPE_MANY_READERS
#define IPC_CLOCK_DOMAIN_MEDIA_STACK_SYNC_TYPE	IPC_TYPE_MANY_READERS

#define IPC_MEDIA_STACK_MAAP_TYPE		IPC_TYPE_MANY_WRITERS
#define IPC_MAAP_MEDIA_STACK_TYPE		IPC_TYPE_MANY_READERS
#define IPC_MAAP_MEDIA_STACK_SYNC_TYPE		IPC_TYPE_MANY_READERS

#define IPC_MEDIA_STACK_GPTP_TYPE		IPC_TYPE_MANY_WRITERS
#define IPC_GPTP_MEDIA_STACK_TYPE		IPC_TYPE_MANY_READERS
#define IPC_GPTP_MEDIA_STACK_SYNC_TYPE		IPC_TYPE_MANY_READERS

#define IPC_MEDIA_STACK_GPTP_1_TYPE		IPC_TYPE_MANY_WRITERS
#define IPC_GPTP_1_MEDIA_STACK_TYPE		IPC_TYPE_MANY_READERS
#define IPC_GPTP_1_MEDIA_STACK_SYNC_TYPE	IPC_TYPE_MANY_READERS

#define IPC_MEDIA_STACK_GPTP_BRIDGE_TYPE	IPC_TYPE_MANY_WRITERS
#define IPC_GPTP_BRIDGE_MEDIA_STACK_TYPE	IPC_TYPE_MANY_READERS
#define IPC_GPTP_BRIDGE_MEDIA_STACK_SYNC_TYPE	IPC_TYPE_MANY_READERS

#define IPC_MEDIA_STACK_AVTP_TYPE		IPC_TYPE_MANY_WRITERS
#define IPC_AVTP_MEDIA_STACK_TYPE		IPC_TYPE_MANY_READERS
#define IPC_AVTP_MEDIA_STACK_SYNC_TYPE		IPC_TYPE_MANY_READERS

#define IPC_AVTP_STATS_TYPE			IPC_TYPE_SINGLE_READER_WRITER

#define IPC_AVDECC_AVTP_TYPE			IPC_TYPE_SINGLE_READER_WRITER
#define IPC_AVTP_AVDECC_TYPE			IPC_TYPE_SINGLE_READER_WRITER

#define IPC_MEDIA_STACK_MAC_SERVICE_TYPE	IPC_TYPE_MANY_WRITERS
#define IPC_MAC_SERVICE_MEDIA_STACK_TYPE	IPC_TYPE_MANY_READERS
#define IPC_MAC_SERVICE_MEDIA_STACK_SYNC_TYPE	IPC_TYPE_MANY_READERS

#define IPC_MEDIA_STACK_MAC_SERVICE_1_TYPE	IPC_TYPE_MANY_WRITERS
#define IPC_MAC_SERVICE_1_MEDIA_STACK_TYPE	IPC_TYPE_MANY_READERS
#define IPC_MAC_SERVICE_1_MEDIA_STACK_SYNC_TYPE	IPC_TYPE_MANY_READERS

#define IPC_MEDIA_STACK_MAC_SERVICE_BRIDGE_TYPE	IPC_TYPE_MANY_WRITERS
#define IPC_MAC_SERVICE_BRIDGE_MEDIA_STACK_TYPE	IPC_TYPE_MANY_READERS
#define IPC_MAC_SERVICE_BRIDGE_MEDIA_STACK_SYNC_TYPE	IPC_TYPE_MANY_READERS

#define IPC_SRP_BRIDGE_ENDPOINT_TYPE			IPC_TYPE_SINGLE_READER_WRITER
#define IPC_SRP_ENDPOINT_BRIDGE_TYPE			IPC_TYPE_SINGLE_READER_WRITER

enum {
	/* Hearbeat message from app to stack */
	IPC_HEARTBEAT = GENAVB_MSG_TYPE_MAX,

	/* AVTP IPC message types*/
	IPC_AVTP_CONNECT,
	IPC_AVTP_LISTENER_CONNECT_RESPONSE,
	IPC_AVTP_TALKER_CONNECT_RESPONSE,
	IPC_AVTP_DISCONNECT,
	IPC_AVTP_DISCONNECT_RESPONSE,

	IPC_AVTP_PROCESS_STATS,
	IPC_AVTP_STREAM_TALKER_STATS,
	IPC_AVTP_STREAM_LISTENER_STATS,
	IPC_AVTP_CLOCK_DOMAIN_STATS,
	IPC_AVTP_CLOCK_GRID_STATS,
	IPC_AVTP_CLOCK_GRID_CONSUMER_STATS,
	IPC_AVTP_GET_COUNTERS,
	IPC_AVTP_COUNTERS,

	IPC_MAC_SERVICE_GET_STATUS,
	IPC_MAC_SERVICE_STATUS,

	/* IPC to transfer network buffers */
	IPC_NETWORK_BUFFER,
};

enum {
	IPC_HSR_OPERATION_MODE_SET,
};

/* MSRP messages */
#define ipc_msrp_listener_register genavb_msg_listener_register
#define ipc_msrp_listener_deregister genavb_msg_listener_deregister
#define ipc_msrp_listener_response genavb_msg_listener_response
#define ipc_msrp_listener_status genavb_msg_listener_status
#define ipc_msrp_listener_declaration_status genavb_msg_listener_declaration_status

#define ipc_msrp_talker_register genavb_msg_talker_register
#define ipc_msrp_talker_deregister genavb_msg_talker_deregister
#define ipc_msrp_talker_response genavb_msg_talker_response
#define ipc_msrp_talker_status genavb_msg_talker_status
#define ipc_msrp_talker_declaration_status genavb_msg_talker_declaration_status

/* MVRP messages */
#define ipc_mvrp_vlan_register genavb_msg_vlan_register
#define ipc_mvrp_vlan_deregister genavb_msg_vlan_deregister
#define ipc_mvrp_vlan_response genavb_msg_vlan_response

/* Clock Domain messages */
#define ipc_clock_domain_set_source genavb_msg_clock_domain_set_source
#define ipc_clock_domain_response genavb_msg_clock_domain_response
#define ipc_clock_domain_get_status genavb_msg_clock_domain_get_status
#define ipc_clock_domain_status genavb_msg_clock_domain_status

/* AVTP messages */
#define ipc_avtp_connect		genavb_stream_params

/* MAAP messages */
#define ipc_maap_create genavb_msg_maap_create
#define ipc_maap_create_response genavb_msg_maap_create_response
#define ipc_maap_delete genavb_msg_maap_delete
#define ipc_maap_delete_response genavb_msg_maap_delete_response
#define ipc_maap_status genavb_maap_status

/* MEDIA STACK messages */
#define ipc_media_stack_bind genavb_msg_media_stack_bind

struct ipc_avtp_disconnect {
	avb_u8 stream_id[8];		/**< Stream ID (in network order) */
	avb_u16 port;			/**< Network port */
	sr_class_t stream_class;	/**< Stream class */
	avtp_direction_t direction;	/**< Stream direction */
};

struct ipc_avtp_listener_connect_response {
	u64 stream_id; /* In Big Endian */
	u16 status;	/**< 0 for success, non-zero value for failure. */
};

struct ipc_avtp_talker_connect_response {
	u64 stream_id;	/* In Big Endian */
	u16 status;	/**< 0 for success, non-zero value for failure. */
	u32 latency;	/* Transmit batch in nanoseconds */
	u32 batch;	/* Transmit batch in packets units */
	u32 max_payload_size; /* Transmit maximum packet payload */
};

struct ipc_avtp_disconnect_response {
	u64 stream_id; /* In Big Endian */
	u16 status;		/**< 0 for success, non-zero value for failure. */
};

struct ipc_avtp_counters {
	u32 stream_listener_count;
	u32 stream_talker_count;

	u8 data[]; /* Flexible size array: ipc_avtp_listener_counters * stream_listener_count followed by ipc_avtp_talker_counters * stream_talker_count */
};

struct ipc_avtp_listener_counters {
	u64 stream_id;
	struct {
		u32 rx;
		u32 pkt_lost;
		u32 mr;
		u32 tu;
		u32 unsupported_format;
		u32 early_timestamp;
		u32 late_timestamp;
	} counters;
};

struct ipc_avtp_talker_counters {
	u64 stream_id;
	struct {
		u32 tx;
		u32 mr;
	} counters;
};

#define IPC_AVTP_FLAGS_MCR	GENAVB_STREAM_FLAGS_MCR
#define IPC_AVTP_FLAGS_MAX_TRANSIT_TIME_VALID	GENAVB_STREAM_FLAGS_MAX_TRANSIT_TIME_VALID

struct ipc_mac_service_get_status {
	u16 port_id;
};

struct ipc_mac_service_status {
	u16 port_id;
	u8 operational;
	u8 point_to_point;
	u32 rate;
};

/* Generic messages */
#define ipc_error_response	genavb_msg_error_response

struct ipc_heartbeat  {
	u32 status;
};

/* IPC flags */
#define IPC_FLAGS_AVB_MSG_SYNC	(1 << 0)	/* Used in the public API to specify the response (if any) should be sent in the corresponding "sync" channel */

/* IPC generic descriptor */
#define ipc_acmp_command	genavb_acmp_command
#define ipc_acmp_response	genavb_acmp_response
#define ipc_aecp_msg		genavb_aecp_msg
#define	ipc_adp_msg		genavb_adp_msg

#define IPC_DST_ALL	0xffff

struct ipc_desc {
	u32 type;	/* ipc message type */
	u32 len;	/* length of the following IPC data */
	u32 flags;	/* Flags altering the IPC behavior */
	u16 src;	/* source of the ipc message, within an ipc channel */
	u16 dst;	/* destination of the ipc message, within an ipc channel */
	union {
		struct ipc_msrp_listener_register msrp_listener_register;
		struct ipc_msrp_listener_deregister msrp_listener_deregister;
		struct ipc_msrp_listener_response msrp_listener_response;
		struct ipc_msrp_listener_status msrp_listener_status;
		struct ipc_msrp_listener_declaration_status msrp_listener_declaration_status;

		struct ipc_msrp_talker_register msrp_talker_register;
		struct ipc_msrp_talker_deregister msrp_talker_deregister;
		struct ipc_msrp_talker_response msrp_talker_response;
		struct ipc_msrp_talker_status msrp_talker_status;
		struct ipc_msrp_talker_declaration_status msrp_talker_declaration_status;

		struct ipc_mvrp_vlan_register mvrp_vlan_register;
		struct ipc_mvrp_vlan_deregister mvrp_vlan_deregister;
		struct ipc_mvrp_vlan_response mvrp_vlan_response;

		struct ipc_clock_domain_set_source clock_domain_set_source;
		struct ipc_clock_domain_response clock_domain_response;
		struct ipc_clock_domain_get_status clock_domain_get_status;
		struct ipc_clock_domain_status clock_domain_status;

		struct genavb_msg_gm_get_status	gm_get_status;
		struct genavb_msg_gm_status	gm_status;

		struct genavb_msg_gptp_port_params	gptp_port_params;

		struct genavb_msg_managed_get	managed_get;
		struct genavb_msg_managed_get_response	managed_get_response;
		struct genavb_msg_managed_set	managed_set;
		struct genavb_msg_managed_set_response	managed_set_response;

		struct genavb_msg_media_stack_connect media_stack_connect;
		struct genavb_msg_media_stack_disconnect media_stack_disconnect;
		struct genavb_msg_media_stack_bind media_stack_bind;
		struct genavb_msg_media_stack_unbind media_stack_unbind;

		struct ipc_avtp_connect avtp_connect;
		struct ipc_avtp_listener_connect_response avtp_listener_connect_response;
		struct ipc_avtp_talker_connect_response avtp_talker_connect_response;

		struct ipc_avtp_disconnect avtp_disconnect;
		struct ipc_avtp_disconnect_response avtp_disconnect_response;

		struct ipc_acmp_command acmp_command;
		struct ipc_acmp_response acmp_response;
		struct ipc_aecp_msg aecp_msg;
		struct ipc_adp_msg adp_msg;

		struct ipc_mac_service_get_status mac_service_get_status;
		struct ipc_mac_service_status mac_service_status;

		struct ipc_maap_create maap_create;
		struct ipc_maap_create_response maap_create_response;
		struct ipc_maap_delete maap_delete;
		struct ipc_maap_delete_response maap_delete_response;
		struct ipc_maap_status maap_status;

		struct ipc_error_response error;

		struct ipc_heartbeat hearbeat;

		u8 data[0];
	} u;
};

#include "os/ipc.h"

#endif /* _COMMON_IPC_H_ */
