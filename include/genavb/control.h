/*
 * Copyright 2018, 2020-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief GenAVB/TSN public control API
 \details control API definition for the GenAVB/TSN library

 \copyright Copyright 2018, 2020-2024 NXP
*/

#ifndef _GENAVB_PUBLIC_CONTROL_API_H_
#define _GENAVB_PUBLIC_CONTROL_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "types.h"
#include "error.h"

/**
 * \ingroup control
 * GENAVB control message types.
 */
typedef enum {
	GENAVB_MSG_MEDIA_STACK_CONNECT,		/**< Stream connect message (from GenAVB/TSN stack to media stack/application). Valid in ::GENAVB_CTRL_AVDECC_MEDIA_STACK channel. */
	GENAVB_MSG_MEDIA_STACK_DISCONNECT,	/**< Stream connect message (from GenAVB/TSN stack to media stack/application). Valid in ::GENAVB_CTRL_AVDECC_MEDIA_STACK channel. */
	GENAVB_MSG_AECP,			/**< AECP messages (commands/responses). Valid in ::GENAVB_CTRL_AVDECC_CONTROLLER and ::GENAVB_CTRL_AVDECC_CONTROLLED channels. */
	GENAVB_MSG_ACMP_COMMAND,		/**< ACMP commands. Valid in ::GENAVB_CTRL_AVDECC_CONTROLLER channel. On a Talker or Listener, ACMP messages are either handled directly by the GenAVB/TSN stack or passed to the application through a ::GENAVB_CTRL_AVDECC_MEDIA_STACK channel. */
	GENAVB_MSG_ACMP_RESPONSE,		/**< ACMP responses. Valid in ::GENAVB_CTRL_AVDECC_CONTROLLER channel. On a Talker or Listener, ACMP messages are either handled directly by the GenAVB/TSN stack or passed to the application through a ::GENAVB_CTRL_AVDECC_MEDIA_STACK channel. */
	GENAVB_MSG_ADP,				/**< ADP messages (commands/responses). Valid in ::GENAVB_CTRL_AVDECC_CONTROLLER channel. */
	GENAVB_MSG_LISTENER_REGISTER,		/**< MSRP Listener register command. Send valid in ::GENAVB_CTRL_MSRP channel. */
	GENAVB_MSG_LISTENER_DEREGISTER,		/**< MSRP Listener deregister command. Send valid in ::GENAVB_CTRL_MSRP channel. */
	GENAVB_MSG_LISTENER_RESPONSE,		/**< MSRP Listener register/deregister response. Receive valid in ::GENAVB_CTRL_MSRP channel. */
	GENAVB_MSG_LISTENER_STATUS,		/**< MSRP Listener status indication. Receive valid in ::GENAVB_CTRL_MSRP channel. */
	GENAVB_MSG_TALKER_REGISTER,		/**< MSRP Talker register command. Send valid in ::GENAVB_CTRL_MSRP channel. */
	GENAVB_MSG_TALKER_DEREGISTER,		/**< MSRP Talker deregister command. Send valid in ::GENAVB_CTRL_MSRP channel. */
	GENAVB_MSG_TALKER_RESPONSE,		/**< MSRP Talker register/deregister response. Receive valid in ::GENAVB_CTRL_MSRP channel. */
	GENAVB_MSG_TALKER_STATUS,		/**< MSRP Talker status indication. Receive valid in ::GENAVB_CTRL_MSRP channel. */
	GENAVB_MSG_VLAN_REGISTER,		/**< MSRP Vlan register command. Send valid in ::GENAVB_CTRL_MVRP channel. */
	GENAVB_MSG_VLAN_DEREGISTER,		/**< MSRP Vlan deregister command. Send valid in ::GENAVB_CTRL_MVRP channel. */
	GENAVB_MSG_VLAN_RESPONSE,		/**< MSRP Vlan register/deregister response. Receive valid in ::GENAVB_CTRL_MVRP channel. */
	GENAVB_MSG_CLOCK_DOMAIN_SET_SOURCE,	/**< Clock domain set source command. Send valid in ::GENAVB_CTRL_CLOCK_DOMAIN channel. */
	GENAVB_MSG_CLOCK_DOMAIN_RESPONSE,	/**< Clock domain response. Receive valid in ::GENAVB_CTRL_CLOCK_DOMAIN channel. */
	GENAVB_MSG_CLOCK_DOMAIN_GET_STATUS,	/**< Clock domain get status command. Send valid in ::GENAVB_CTRL_CLOCK_DOMAIN channel. */
	GENAVB_MSG_CLOCK_DOMAIN_STATUS,		/**< Clock domain status indications. Receive valid in ::GENAVB_CTRL_CLOCK_DOMAIN channel. */
	GENAVB_MSG_GM_GET_STATUS,		/**< GPTP get grand master status command. Send valid in ::GENAVB_CTRL_GPTP and ::GENAVB_CTRL_GPTP_BRIDGE channels. */
	GENAVB_MSG_GM_STATUS,			/**< GPTP grand master status indication/response. Receive valid in ::GENAVB_CTRL_GPTP and ::GENAVB_CTRL_GPTP_BRIDGE channels. */
	GENAVB_MSG_GPTP_PORT_PARAMS,			/**< GPTP port params status. Receive valid in ::GENAVB_CTRL_GPTP and ::GENAVB_CTRL_GPTP_BRIDGE channels. */
	GENAVB_MSG_MANAGED_GET,			/**< Managed object get command. Send valid in ::GENAVB_CTRL_GPTP, ::GENAVB_CTRL_GPTP_BRIDGE, ::GENAVB_CTRL_MSRP and ::GENAVB_CTRL_MSRP_BRIDGE channels. */
	GENAVB_MSG_MANAGED_SET,			/**< Managed object set command. Send valid in ::GENAVB_CTRL_GPTP, ::GENAVB_CTRL_GPTP_BRIDGE, ::GENAVB_CTRL_MSRP and ::GENAVB_CTRL_MSRP_BRIDGE channels. */
	GENAVB_MSG_MANAGED_GET_RESPONSE,	/**< Managed object get response. Receive valid in ::GENAVB_CTRL_GPTP, ::GENAVB_CTRL_GPTP_BRIDGE, ::GENAVB_CTRL_MSRP and ::GENAVB_CTRL_MSRP_BRIDGE channels. */
	GENAVB_MSG_MANAGED_SET_RESPONSE,	/**< Managed object set response. Receive valid in ::GENAVB_CTRL_GPTP, ::GENAVB_CTRL_GPTP_BRIDGE, ::GENAVB_CTRL_MSRP and ::GENAVB_CTRL_MSRP_BRIDGE channels. */
	GENAVB_MSG_MAAP_CREATE_RANGE,		/**< MAAP create range command. Send valid in ::GENAVB_CTRL_MAAP channel. */
	GENAVB_MSG_MAAP_DELETE_RANGE,		/**< MAAP delete range command. Send valid in ::GENAVB_CTRL_MAAP channel. */
	GENAVB_MSG_MAAP_CREATE_RANGE_RESPONSE,		/**< MAAP create range response. Receive valid in ::GENAVB_CTRL_MAAP channel. */
	GENAVB_MSG_MAAP_DELETE_RANGE_RESPONSE,		/**< MAAP delete range response. Receive valid in ::GENAVB_CTRL_MAAP channel. */
	GENAVB_MSG_MAAP_STATUS,			/**< MAAP range status indication. Receive valid in ::GENAVB_CTRL_MAAP channel. */
	GENAVB_MSG_TALKER_DECLARATION_STATUS,	/**< MSRP Talker declaration status indication. Receive valid in ::GENAVB_CTRL_MSRP channel. */
	GENAVB_MSG_LISTENER_DECLARATION_STATUS,	/**< MSRP Listener declaration status indication. Receive valid in ::GENAVB_CTRL_MSRP channel. */
	GENAVB_MSG_MEDIA_STACK_BIND,		/**< Stream bind message (From GenAVB/TSN stack to media stack/application and vice versa). Valid in ::GENAVB_CTRL_AVDECC_MEDIA_STACK channel. */
	GENAVB_MSG_MEDIA_STACK_UNBIND,		/**< Stream unbind message (from GenAVB/TSN stack to media stack/application). Valid in ::GENAVB_CTRL_AVDECC_MEDIA_STACK channel. */
	GENAVB_MSG_ERROR_RESPONSE,		/**< Generic error response. Receive valid in all control channels. */
	GENAVB_MSG_DEREGISTER_ALL,		/**< MSRP deregister all stream declarations command. Send valid in ::GENAVB_CTRL_MSRP channel. */
	GENAVB_MSG_TYPE_MAX
} genavb_msg_type_t;


/**
 * \ingroup control
 * GENAVB control channel types.
 * Determines the types of messages the application wishes to send/receive through a control channel.
 * Currently, only one single channel of each type is supported by the GenAVB/TSN stack.
 */
typedef enum {
	GENAVB_CTRL_AVDECC_MEDIA_STACK = 0,	/**< Notification of stream connections/disconnections and bind/unbind to the media stack */
	GENAVB_CTRL_AVDECC_CONTROLLER,		/**< Communication between the GenAVB/TSN stack and a controller application */
	GENAVB_CTRL_AVDECC_CONTROLLED,		/**< Communication between the GenAVB/TSN stack and a talker or listener application (AECP commands to application, responses from application) */
	GENAVB_CTRL_MSRP,				/**< Communication between talker or listener application and the GenAVB/TSN MSRP endpoint 0 stack */
	GENAVB_CTRL_MVRP,				/**< Communication between talker or listener application and the GenAVB/TSN MVRP endpoint 0 stack */
	GENAVB_CTRL_CLOCK_DOMAIN,			/**< Communication between talker or listener application and the GenAVB/TSN clock domain component */
	GENAVB_CTRL_GPTP,				/**< Communication between talker or listener application and the GenAVB/TSN GPTP endpoint 0 stack */
	GENAVB_CTRL_GPTP_BRIDGE,			/**< Communication between talker or listener application and the GenAVB/TSN GPTP bridge stack */
	GENAVB_CTRL_MSRP_BRIDGE,			/**< Communication between talker or listener application and the GenAVB/TSN MSRP bridge stack */
	GENAVB_CTRL_MVRP_BRIDGE,			/**< Communication between talker or listener application and the GenAVB/TSN MVRP bridge stack */
	GENAVB_CTRL_MAAP,				/**< Communication between the GenAVB/TSN Stack and a controller application */
	GENAVB_CTRL_MSRP_1,				/**< Communication between talker or listener application and the GenAVB/TSN MSRP endpoint 1 stack */
	GENAVB_CTRL_MVRP_1,				/**< Communication between talker or listener application and the GenAVB/TSN MVRP endpoint 1 stack */
	GENAVB_CTRL_GPTP_1,				/**< Communication between talker or listener application and the GenAVB/TSN GPTP endpoint 1 stack */
	GENAVB_CTRL_ID_MAX
} genavb_control_id_t;

#define GENAVB_CTRL_MSRP_0	GENAVB_CTRL_MSRP
#define GENAVB_CTRL_MVRP_0	GENAVB_CTRL_MVRP
#define GENAVB_CTRL_GPTP_0	GENAVB_CTRL_GPTP

/**
 * \ingroup control
 * Generic error response.
 */
struct genavb_msg_error_response {
	avb_u32 type;		/**< Original command type */
	avb_u32 len;		/**< Original command length */
	avb_u32 status;		/**< Command status */
};

#define GENAVB_MAX_MANAGED_SIZE	1000

/**
 * \ingroup control
 * Generic managed object get command.
 */
struct genavb_msg_managed_get {
	avb_u8 data[GENAVB_MAX_MANAGED_SIZE];
};

/**
 * \ingroup control
 * Generic managed object set command.
 */
struct genavb_msg_managed_set {
	avb_u8 data[GENAVB_MAX_MANAGED_SIZE];
};

/**
 * \ingroup control
 * Generic managed object get response.
 */
struct genavb_msg_managed_get_response {
	avb_u8 data[GENAVB_MAX_MANAGED_SIZE];
};

/**
 * \ingroup control
 * Generic managed object set response.
 */
struct genavb_msg_managed_set_response {
	avb_u8 data[GENAVB_MAX_MANAGED_SIZE];
};

/** Open a communication control channel to the GenAVB/TSN stack
 * \ingroup control
 * \return	::GENAVB_SUCCESS or negative error code. On success handle argument is updated with control handle, to be used for control messages receive/send with the GenAVB/TSN stack
 * \param 	genavb	pointer to GenAVB/TSN library handle
 * \param 	handle	pointer to control handle pointer
 * \param	id	control message type identifier
 */
int genavb_control_open(struct genavb_handle const *genavb, struct genavb_control_handle **handle, genavb_control_id_t id);


/** Send control message to the avb stack
 * \ingroup control
 * \return 		::GENAVB_SUCCESS or negative error code
 * \param handle	control handle returned by ::genavb_control_open.
 * \param msg_type	type of message sent.
 * \param msg		buffer containing the message to send.
 * \param msg_len	length of the message in bytes.
 */
int genavb_control_send(struct genavb_control_handle const *handle, genavb_msg_type_t msg_type, void const *msg, unsigned int msg_len);


/** Send control message to the GenAVB/TSN stack and wait for response.
 * \ingroup control
 * \return 		::GENAVB_SUCCESS or negative error code
 * \param handle	control handle returned by ::genavb_control_open.
 * \param msg_type	type of the message to be sent (on successful return, contains the type of the received message).
 * \param msg		buffer containing the message to send on input.
 * \param msg_len	length of the message to send in bytes.
 * \param rsp		buffer containing the response on return. Will return an error if NULL.
 * \param rsp_len	maximum length of the response buffer in bytes (on successful return, contains the actual length of the received message).
 * \param timeout	How long to wait for a response, in milliseconds. A negative value means an infinite timeout.  A value of zero will cause genavb_control_send_sync to return immediately, even if no response is ready.
 */
int genavb_control_send_sync(struct genavb_control_handle const *handle, genavb_msg_type_t *msg_type, void const *msg, unsigned int msg_len, void *rsp, unsigned int *rsp_len, int timeout);


/** Receive control message from the GenAVB/TSN stack
 * \ingroup control
 * \return 		::GENAVB_SUCCESS or negative error code
 * \param handle	control handle returned by ::genavb_control_open.
 * \param msg_type	type of the received message.
 * \param msg		buffer containing the received message. Will return an error if NULL.
 * \param msg_len	maximum length of the response buffer in bytes (on successful return, contains the actual length of the received message).
 */
int genavb_control_receive(struct genavb_control_handle const *handle, genavb_msg_type_t *msg_type, void *msg, unsigned int *msg_len);


/** Close a given communication control channel
 * \ingroup control
 * \return	::GENAVB_SUCCESS or negative error code.
 * \param 	handle	control handle for the control to be closed.
 */
int genavb_control_close(struct genavb_control_handle *handle);


/* OS specific headers */
#include "os/control.h"

#ifdef __cplusplus
}
#endif

#endif /* _GENAVB_PUBLIC_CONTROL_API_H_ */
