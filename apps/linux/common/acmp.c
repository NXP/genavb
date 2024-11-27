/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2019-2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#include <genavb/genavb.h>

/** Sends an ACMP command to the stack, and optionally return the response.
 * Note: the parameters of the function are an exact match to the fields of an ACMP command PDU.
 *
 * \return AVB return code (AVB_SUCCESS or negative error code).
 * \param	ctrl_h				Handle to an avb_control_handle, opened for an AVB_CTRL_AVDECC_CONTROLLER channel.
 * \param	msg_type			Type of ACMP message to send.
 * \param	talker_entity_id	[Network Order] Entity ID of the talker affected by the command, if any.
 * \param	talker_unique_id	[Network Order] Unique ID of the stream output being targeted on the talker, if any.
 * \param	listener_entity_id	[Network Order] Entity ID of the listener affected by the command, if any.
 * \param	listener_unique_id	[Network Order] Unique ID of the stream input being targeted on the listener, if any.
 * \param	connection_count	[Network Order] Index of the connection to get information about, for a ACMP_GET_TX_CONNECTION_COMMAND. Will be ignored by the stack for other commands.
 * \param	flags				[Network Order] Stream attributes.
 * \param	acmp_rsp			If non-NULL, the function will wait for an ACMP response (after sending the command) and acmp_rsp will point to it on return.
 */
int acmp_send_command(struct avb_control_handle *ctrl_h, acmp_message_type_t msg_type, avb_u64 talker_entity_id, avb_u16 talker_unique_id, avb_u64 listener_entity_id, avb_u16 listener_unique_id, avb_u16 connection_count, avb_u16 flags, struct avb_acmp_response *acmp_rsp)
{
	int rc;
	struct avb_acmp_command cmd;

	cmd.message_type = msg_type;
	cmd.talker_entity_id = talker_entity_id;
	cmd.talker_unique_id = talker_unique_id;
	cmd.listener_entity_id = listener_entity_id;
	cmd.listener_unique_id = listener_unique_id;
	cmd.connection_count = connection_count;
	cmd.flags = flags;

	if (acmp_rsp) {
		unsigned int avb_msg_type = AVB_MSG_ACMP_COMMAND;
		unsigned int avb_msg_len = sizeof(struct avb_acmp_response);

		rc = avb_control_send_sync(ctrl_h, &avb_msg_type, &cmd, sizeof(struct avb_acmp_command), acmp_rsp, &avb_msg_len, -1);
		if ((rc == AVB_SUCCESS) && (avb_msg_type == AVB_MSG_ACMP_RESPONSE))
			rc = acmp_rsp->status;
	}
	else
		rc = avb_control_send(ctrl_h, AVB_MSG_ACMP_COMMAND, &cmd, sizeof(struct avb_acmp_command));


	return rc;
}


/** Connects a stream output of a talker to a stream input of a listener.
 *
 * \return AVB return code (AVB_SUCCESS or negative error code).
 * \param	ctrl_h				Handle to an avb_control_handle, opened for an AVB_CTRL_AVDECC_CONTROLLER channel.
 * \param	talker_entity_id	[Network Order] Entity ID of the talker affected by the command.
 * \param	talker_unique_id	[Network Order] Unique ID of the stream output to use on the talker.
 * \param	listener_entity_id	[Network Order] Entity ID of the listener affected by the command.
 * \param	listener_unique_id	[Network Order] Unique ID of the stream input to use on the listener.
 * \param	flags				[Network Order] Stream attributes.
 * \param	acmp_rsp			If non-NULL, the function will wait for an ACMP response (after sending the command) and acmp_rsp will point to it on return.
 */
int acmp_connect_stream(struct avb_control_handle *ctrl_h, avb_u64 talker_entity_id, avb_u16 talker_unique_id, avb_u64 listener_entity_id, avb_u16 listener_unique_id, avb_u16 flags, struct avb_acmp_response *acmp_rsp)
{
	return acmp_send_command(ctrl_h, ACMP_CONNECT_RX_COMMAND, talker_entity_id, talker_unique_id, listener_entity_id, listener_unique_id, 0, flags, acmp_rsp);
}

/** Disconnects a stream output of a talker to a stream input of a listener.
 *
 * \return AVB return code (AVB_SUCCESS or negative error code).
 * \param	ctrl_h				Handle to an avb_control_handle, opened for an AVB_CTRL_AVDECC_CONTROLLER channel.
 * \param	talker_entity_id	[Network Order] Entity ID of the talker affected by the command.
 * \param	talker_unique_id	[Network Order] Unique ID of the stream output to use on the talker.
 * \param	listener_entity_id	[Network Order] Entity ID of the listener affected by the command.
 * \param	listener_unique_id	[Network Order] Unique ID of the stream input to use on the listener.
 * \param	acmp_rsp			If non-NULL, the function will wait for an ACMP response (after sending the command) and acmp_rsp will point to it on return.
 */
int acmp_disconnect_stream(struct avb_control_handle *ctrl_h, avb_u64 talker_entity_id, avb_u16 talker_unique_id, avb_u64 listener_entity_id, avb_u16 listener_unique_id, struct avb_acmp_response *acmp_rsp)
{
	return acmp_send_command(ctrl_h, ACMP_DISCONNECT_RX_COMMAND, talker_entity_id, talker_unique_id, listener_entity_id, listener_unique_id, 0, 0, acmp_rsp);
}

/** Returns information about a given stream input of a listener entity.
 *
 * \return AVB return code (AVB_SUCCESS or negative error code).
 * \param	ctrl_h				Handle to an avb_control_handle, opened for an AVB_CTRL_AVDECC_CONTROLLER channel.
 * \param	listener_entity_id	[Network Order] Entity ID of the listener affected by the command.
 * \param	listener_unique_id	[Network Order] Unique ID of the stream input to gather information about.
 * \param	acmp_rsp			If non-NULL, the function will wait for an ACMP response (after sending the command) and acmp_rsp will point to it on return.
 */
int acmp_get_rx_state(struct avb_control_handle *ctrl_h, avb_u64 listener_entity_id, avb_u16 listener_unique_id, struct avb_acmp_response *acmp_rsp)
{
	return acmp_send_command(ctrl_h, ACMP_GET_RX_STATE_COMMAND, 0, 0, listener_entity_id, listener_unique_id, 0, 0, acmp_rsp);
}

/** Returns information about a given stream output of a talker entity.
 *
 * \return AVB return code (AVB_SUCCESS or negative error code).
 * \param	ctrl_h				Handle to an avb_control_handle, opened for an AVB_CTRL_AVDECC_CONTROLLER channel.
 * \param	talker_entity_id	[Network Order] Entity ID of the talker affected by the command.
 * \param	talker_unique_id	[Network Order] Unique ID of the stream output to gather information about.
 * \param	acmp_rsp			If non-NULL, the function will wait for an ACMP response (after sending the command) and acmp_rsp will point to it on return.
 */
int acmp_get_tx_state(struct avb_control_handle *ctrl_h, avb_u64 talker_entity_id, avb_u16 talker_unique_id, struct avb_acmp_response *acmp_rsp)
{
	return acmp_send_command(ctrl_h, ACMP_GET_TX_STATE_COMMAND, talker_entity_id, talker_unique_id, 0, 0, 0, 0, acmp_rsp);
}

/** Returns information about a specific stream.
 *
 * \return AVB return code (AVB_SUCCESS or negative error code).
 * \param	ctrl_h				Handle to an avb_control_handle, opened for an AVB_CTRL_AVDECC_CONTROLLER channel.
 * \param	talker_entity_id	[Network Order] Entity ID of the talker affected by the command.
 * \param	talker_unique_id	[Network Order] Unique ID of the stream output the stream is connected to.
 * \param	connection_count	[Network Order] Index of the connection to get information about.
 * \param	acmp_rsp			If non-NULL, the function will wait for an ACMP response (after sending the command) and acmp_rsp will point to it on return.
 */

int acmp_get_tx_connection(struct avb_control_handle *ctrl_h, avb_u64 talker_entity_id, avb_u16 talker_unique_id, avb_u16 connection_count, struct avb_acmp_response *acmp_rsp)
{
	return acmp_send_command(ctrl_h, ACMP_GET_TX_CONNECTION_COMMAND, talker_entity_id, talker_unique_id, 0, 0, connection_count, 0, acmp_rsp);
}
