/*
 * Copyright 2018, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file control_srp.h
 \brief GenAVB public control API
 \details SRP control API definition for the GenAVB library

 \copyright Copyright 2018, 2021, 2023 NXP
*/

#ifndef _GENAVB_PUBLIC_CONTROL_SRP_API_H_
#define _GENAVB_PUBLIC_CONTROL_SRP_API_H_

#include "control.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "types.h"
#include "srp.h"
#include "sr_class.h"

/**
 * \ingroup control
 * MSRP Stream params
 */
struct genavb_msrp_stream_params {
	avb_u32 stream_class;			/**< ::sr_class_t */
	avb_u8 destination_address[6];		/**< Stream destination mac address (in network order) */
	avb_u16 vlan_id;			/**< Stream Vlan id, one of [VLAN_VID_MIN, VLAN_VID_MAX], VLAN_VID_DEFAULT */
	avb_u16 max_frame_size;
	avb_u16 max_interval_frames;
	avb_u32 accumulated_latency;
	avb_u32 rank;				/**< ::msrp_rank_t */
};

/**
 * \ingroup control
 * MSRP Listener register command
 * Sent by the application to make a Listener MSRP declaration on the network
 */
struct genavb_msg_listener_register {
	avb_u16 port;				/**< Network port */
	avb_u8 stream_id[8];			/**< Stream ID (in network order) */
};

/**
 * \ingroup control
 * MSRP Listener deregister command
 * Sent by the application to withdraw a Listener MSRP declaration from the network
 */
struct genavb_msg_listener_deregister {
	avb_u16 port;				/**< Network port */
	avb_u8 stream_id[8];			/**< Stream ID (in network order) */
};

/**
 * \ingroup control
 * MSRP Listener stream status
 */
typedef enum {
	NO_TALKER = 0,				/**< No Talker declaration for this stream */
	ACTIVE,					/**< Active Talker declaration for this stream */
	FAILED					/**< Failed Talker declaration for this stream. No data can reach the listener */
} genavb_listener_stream_status_t;

/**
 * \ingroup control
 * MSRP Listener status indication
 * Sent by the MSRP stack, reflecting Talker MSRP declarations on the network, for declared Listener streams
 */
struct genavb_msg_listener_status {
	avb_u16 port;				/**< Network port */
	avb_u8 stream_id[8];			/**< Stream ID (in network order) */
	avb_u32 status;				/**< ::genavb_listener_stream_status_t */
	struct genavb_msrp_stream_params params;
	struct msrp_failure_information failure;
};

/**
 * \ingroup control
 * MSRP Listener register/deregister response
 * Sent by the MSRP stack in response to a Listener register/deregister command. Status field contains the
 * result of the command execution.
 */
struct genavb_msg_listener_response {
	avb_u16 port;				/**< Network port */
	avb_u8 stream_id[8];			/**< Stream ID (in network order) */
	avb_u32 status;
};

/**
 * \ingroup control
 * MSRP Talker register command
 * Sent by the application to make a Talker MSRP declaration on the network
 */
struct genavb_msg_talker_register {
	avb_u16 port;				/**< Network port */
	avb_u8 stream_id[8];			/**< Stream ID (in network order) */
	struct genavb_msrp_stream_params params;
};

/**
 * \ingroup control
 * MSRP Talker deregister command
 * Sent by the application to withdraw a Talker MSRP declaration from the network
 */
struct genavb_msg_talker_deregister {
	avb_u16 port;				/**< Network port */
	avb_u8 stream_id[8];			/**< Stream ID (in network order) */
};

/**
 * \ingroup control
 * MSRP Talker stream status
 */
typedef enum {
	NO_LISTENER = 0,			/**< No Listener declaration for this stream */
	FAILED_LISTENER,			/**< Failed Listener declaration for this stream. Stream can't reach any Listener */
	ACTIVE_AND_FAILED_LISTENERS,		/**< Active and Failed Listener declaration for this stream. Stream can't reach some Listener(s) */
	ACTIVE_LISTENER				/**< Active Listener declaration for this stream. Stream can reach all Listeners */
} genavb_talker_stream_status_t;

/**
 * \ingroup control
 * MSRP Talker status indication
 * Sent by the MSRP stack, reflecting Listener MSRP declarations on the network, for declared Talker streams
 */
struct genavb_msg_talker_status {
	avb_u16 port;				/**< Network port */
	avb_u8 stream_id[8];			/**< Stream ID (in network order) */
	avb_u32 status;				/**< ::genavb_talker_stream_status_t */
};

/**
 * \ingroup control
 * MSRP Talker register/deregister response
 * Sent by the MSRP stack in response to a Talker register/deregister command. Status field contains the
 * result of the command execution.
 */
struct genavb_msg_talker_response {
	avb_u16 port;				/**< Network port */
	avb_u8 stream_id[8];			/**< Stream ID (in network order) */
	avb_u32 status;
};

/**
 * \ingroup control
 * MSRP Talker stream declaration type
 */
typedef enum {
	NO_TALKER_DECLARATION = 0,              /**< No Talker declaration for this stream */
	TALKER_ADVERTISE,                       /**< Talker declaring an advertise */
	TALKER_FAILED,                          /**< Talker declaring a failed */
} genavb_talker_stream_declaration_type_t;

/**
 * \ingroup control
 * MSRP Talker declaration status indication
 */
struct genavb_msg_talker_declaration_status {
	avb_u16 port;                           /**< Network port */
	avb_u8 stream_id[8];                    /**< Stream ID (in network order) */
	avb_u32 declaration_type;               /**< ::genavb_talker_stream_declaration_type_t */
	struct msrp_failure_information failure;
};

/**
 * \ingroup control
 * MSRP Listener stream declaration type
 */
typedef enum {
	NO_LISTENER_DECLARATION = 0,            /**< No Listener declaration for this stream */
	LISTENER_READY,                         /**< Listener declaring a ready attribute */
	LISTENER_READY_FAILED,                  /**< Listener declaring a ready failed attribute */
	LISTENER_FAILED,                        /**< Listener declaring a failed attribute */
} genavb_listener_stream_declaration_type_t;

/**
 * \ingroup control
 * MSRP Listener declaration status indication
 */
struct genavb_msg_listener_declaration_status {
	avb_u16 port;                           /**< Network port */
	avb_u8 stream_id[8];                    /**< Stream ID (in network order) */
	avb_u32 declaration_type;               /**< ::genavb_listener_stream_declaration_type_t */
};

/**
 * \ingroup control
 * MSRP channel message type.
 */
union genavb_msg_msrp {
	struct genavb_msg_listener_register listener_register;
	struct genavb_msg_listener_deregister listener_deregister;
	struct genavb_msg_listener_response listener_response;
	struct genavb_msg_listener_status listener_status;
	struct genavb_msg_listener_declaration_status listener_declaration_status;

	struct genavb_msg_talker_register talker_register;
	struct genavb_msg_talker_deregister talker_deregister;
	struct genavb_msg_talker_response talker_response;
	struct genavb_msg_talker_status talker_status;
	struct genavb_msg_talker_declaration_status talker_declaration_status;

	struct genavb_msg_error_response error_response;
};

/**
 * \ingroup control
 * MVRP Vlan register command
 * Sent by the application to make a VLAN MVRP declaration on the network
 */
struct genavb_msg_vlan_register {
	avb_u16 port;				/**< Network port */
	avb_u16 vlan_id;			/**< Vlan id */
};

/**
 * \ingroup control
 * MVRP Vlan deregister command
 * Sent by the application to withdraw a VLAN MVRP declaration from the network
 */
struct genavb_msg_vlan_deregister {
	avb_u16 port;				/**< Network port */
	avb_u16 vlan_id;			/**< Vlan id */
};

/**
 * \ingroup control
 * MVRP Vlan register/deregister response
 * Sent by the MVRP stack in response to a Vlan register/deregister command. Status field contains the
 * result of the command execution.
 */
struct genavb_msg_vlan_response {
	avb_u16 port;				/**< Network port */
	avb_u16 vlan_id;			/**< Vlan id */
	avb_u32 status;
};

/**
 * \ingroup control
 * MVRP channel message type.
 */
union genavb_msg_mvrp {
	struct genavb_msg_vlan_register vlan_register;
	struct genavb_msg_vlan_deregister vlan_deregister;
	struct genavb_msg_vlan_response vlan_response;
	struct genavb_msg_error_response error_response;
};

#ifdef __cplusplus
}
#endif

#endif /* _GENAVB_PUBLIC_CONTROL_SRP_API_H_ */
