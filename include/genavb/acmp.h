/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file acmp.h
 \brief GenAVB/TSN public API
 \details 1722.1 and Milan ACMP helper definitions.

 \copyright Copyright 2014-2016 Freescale Semiconductor, Inc.
 \copyright Copyright 2016, 2021, 2023-2024 NXP
*/
#ifndef _GENAVB_PUBLIC_ACMP_H_
#define _GENAVB_PUBLIC_ACMP_H_


/**
 * \ingroup control
 * ACMP message types.
 * Follows definition of table 8.1 from IEEE1722.1-2013 */
typedef enum{
	ACMP_CONNECT_TX_COMMAND			= 0,	/**< Connect Talker Stream source command */
	ACMP_CONNECT_TX_RESPONSE		= 1,	/**< Connect Talker Stream source response */
	ACMP_DISCONNECT_TX_COMMAND		= 2,	/**< Disconnect Talker Stream source command. */
	ACMP_DISCONNECT_TX_RESPONSE		= 3,	/**< Disconnect Talker Stream source response. */
	ACMP_GET_TX_STATE_COMMAND		= 4,	/**< Get Talker Stream source connection state command. */
	ACMP_GET_TX_STATE_RESPONSE		= 5,	/**< Get Talker Stream source connection state response. */
	ACMP_CONNECT_RX_COMMAND			= 6,	/**< Connect Listener Stream sink command. */
	ACMP_CONNECT_RX_RESPONSE		= 7,	/**< Connect Listener Stream sink response. */
	ACMP_DISCONNECT_RX_COMMAND		= 8,	/**< Disconnect Listener Stream sink command. */
	ACMP_DISCONNECT_RX_RESPONSE		= 9,	/**< Disconnect Listener Stream sink response. */
	ACMP_GET_RX_STATE_COMMAND		= 10,	/**< Get Listener Stream sink connection state command. */
	ACMP_GET_RX_STATE_RESPONSE		= 11,	/**< Get Listener Stream sink connection state response. */
	ACMP_GET_TX_CONNECTION_COMMAND		= 12,	/**< Get a specific Talker connection info command. */
	ACMP_GET_TX_CONNECTION_RESPONSE		= 13	/**< Get a specific Talker connection info response. */
} acmp_message_type_t;

#define ACMP_PROBE_TX_COMMAND           ACMP_CONNECT_TX_COMMAND		/**< Milan use: Rename CONNECT_TX_COMMAND to PROBE_TX_COMMAND. */
#define ACMP_PROBE_TX_RESPONSE          ACMP_CONNECT_TX_RESPONSE	/**< Milan use: Rename CONNECT_TX_RESPONSE to PROBE_TX_RESPONSE. */
#define ACMP_BIND_RX_COMMAND            ACMP_CONNECT_RX_COMMAND		/**< Milan use: Rename CONNECT_RX_COMMAND to BIND_RX_COMMAND. */
#define ACMP_BIND_RX_RESPONSE           ACMP_CONNECT_RX_RESPONSE	/**< Milan use: Rename CONNECT_RX_RESPONSE to BIND_RX_RESPONSE. */
#define ACMP_UNBIND_RX_COMMAND          ACMP_DISCONNECT_RX_COMMAND	/**< Milan use: Rename DISCONNECT_RX_COMMAND to UNBIND_RX_COMMAND. */
#define ACMP_UNBIND_RX_RESPONSE         ACMP_DISCONNECT_RX_RESPONSE	/**< Milan use: Rename DISCONNECT_RX_RESPONSE to UNBIND_RX_RESPONSE. */

/**
 * \ingroup control
 * ACMP Status codes returned by the stack to the application.
 * Follows definition of table 8.2 from IEEE1722.1-2013 */
typedef enum{
	ACMP_STAT_SUCCESS			= 0,	/**< Command executed successfully. */
	ACMP_STAT_LISTENER_UNKNOWN_ID		= 1,	/**< Listener does not have the specified unique identifier. */
	ACMP_STAT_TALKER_UNKNOWN_ID		= 2,	/**< Talker does not have the specified unique identifier. */
	ACMP_STAT_TALKER_DEST_MAC_FAIL		= 3,	/**< Talker could not allocate a destination MAC for the Stream. */
	ACMP_STAT_TALKER_NO_STREAM_INDEX	= 4,	/**< Talker does not have an available Stream index for the Stream. */
	ACMP_STAT_TALKER_NO_BANDWIDTH		= 5,	/**< Talker could not allocate bandwidth for the Stream. */
	ACMP_STAT_TALKER_EXCLUSIVE		= 6,	/**< Talker already has an established Stream and only supports one Listener. */
	ACMP_STAT_LISTENER_TALKER_TIMEOUT	= 7,	/**< Listener had timeout for all retries when trying to send command to Talker. */
	ACMP_STAT_LISTENER_EXCLUSIVE		= 8,	/**< The AVDECC Listener already has an established connection to a Stream. */
	ACMP_STAT_STATE_UNAVAILABLE		= 9,	/**< Could not get the state from the AVDECC Entity. */
	ACMP_STAT_NOT_CONNECTED			= 10,	/**< Trying to disconnect when not connected or not connected to the AVDECC Talker specified. */
	ACMP_STAT_NO_SUCH_CONNECTION		= 11,	/**< Trying to obtain connection info for an AVDECC Talker connection which does not exist. */
	ACMP_STAT_COULD_NOT_SEND_MESSAGE	= 12,	/**< The AVDECC Listener failed to send the message to the AVDECC Talker. */
	ACMP_STAT_TALKER_MISBEHAVING		= 13,	/**< Talker was unable to complete the command because an internal error occurred. */
	ACMP_STAT_LISTENER_MISBEHAVING		= 14,	/**< Listener was unable to complete the command because an internal error occurred. */
	ACMP_STAT_CONTROLLER_NOT_AUTHORIZED	= 16,	/**< The AVDECC Controller with the specified Entity ID is not authorized to change Stream connections. */
	ACMP_STAT_INCOMPATIBLE_REQUEST		= 17,	/**< The AVDECC Listener is trying to connect to an AVDECC Talker that is already streaming with a different traffic class, etc. or does not support the requested traffic class. */
	ACMP_STAT_NOT_SUPPORTED			= 31,	/**< The command is not supported. */
} acmp_status_t;


/**
 * \ingroup control
 * ACMP message flags.
 * Follows definition of table 8.3 from IEEE 1722.1-2013
 */
#define ACMP_FLAG_CLASS_B                       (1 << 0) /**< Indicates that the Stream is Class B instead of Class A (default 0 is class A). */
#define ACMP_FLAG_FAST_CONNECT                  (1 << 1) /**< Fast Connect Mode, the connection is being attempted in fast connect mode. */
#define ACMP_FLAG_SAVED_STATE                   (1 << 2) /**< Connection has saved state (used in Get State only). */
#define ACMP_FLAG_STREAMING_WAIT                (1 << 3) /**< The AVDECC Talker does not start streaming until explicitly being told to by the control protocol. */
#define ACMP_FLAG_SUPPORTS_ENCRYPTED            (1 << 4) /**< Indicates that the Stream supports streaming with encrypted PDUs. */
#define ACMP_FLAG_ENCRYPTED_PDU                 (1 << 5) /**< Indicates that the Stream is using encrypted PDUs. */
#define ACMP_FLAG_TALKER_FAILED                 (1 << 6) /**< Indicates that the listener has registered an SRP Talker Failed attribute for the Stream. */

/** Milan use: Rename ACMP_FLAG_TALKER_FAILED to ACMP_FLAG_REGISTERING_FAILED
 * Indicates that a talker registering a matching listener failed when declaring an advertise,
 * or that a listener is registering a matching talker failed attribute.
 */
#define ACMP_FLAG_REGISTERING_FAILED            ACMP_FLAG_TALKER_FAILED

#endif /* _GENAVB_PUBLIC_ACMP_H_ */
