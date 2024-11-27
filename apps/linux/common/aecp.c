/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <arpa/inet.h>

#include <genavb/genavb.h>
#include <genavb/aem.h>
#include <genavb/net_types.h>


/** Generic function to send an AECP SET_CONTROL command or response, and optionally wait for a response.
 *
 * \return AVB_SUCCESS on success, or genavb error code.
 * \param	ctrl_h				Handle to an avb_control_handle, opened for an AVB_CTRL_AVDECC_CONTROLLER or AVB_CTRL_AVDECC_CONTROLLED channel.
 * \param	entity_id			Pointer to the entity ID [Network Order] to send the message to. If NULL, the message will be assumed to be an AECP response, and an AECP command otherwise.
 * \param	unsolicited		A flag indicating if it's an unsolicited response (1 for unsolicited responses, 0 in all other cases)
 * \param	descriptor_index	Index of the CONTROL descriptor to modify.
 * \param	value				New value to set for the specified CONTROL descriptor (if the message is a command), or current value to report (if the message is a response). On return, will
 * contain the values from the response if sync != 0.
 * \param	len					Length of the memory area pointed to by value. If sync != 0, will be updated to contain the length of the values contained in the response.
 * \param	status				On return in sync mode, will contain the status from the AECP response if command was successful. Ignored otherwise.
 * \param	sync				If sync == 0, the function will return without waiting for a response from the target entity. Otherwise, the function will block until receiving a response
 * and value, len and status will be updated to match the response.
 */
int aecp_aem_send_set_control(struct avb_control_handle *ctrl_h, avb_u64 *entity_id, int unsolicited, avb_u16 descriptor_index, void *value, avb_u16 *len, avb_u8 *status, int sync)
{
	struct avb_aecp_msg aecp_msg;
	struct aecp_aem_pdu *aecp_cmd = (struct aecp_aem_pdu *) aecp_msg.buf;
	struct aecp_aem_set_get_control_pdu *set_control_cmd = (struct aecp_aem_set_get_control_pdu *)(aecp_cmd + 1);
	void *values_cmd = set_control_cmd + 1;
	int value_len;
	int rc;

	if (!value) {
		printf("%s: ERROR: NULL pointer passed for value parameter\n", __func__);
		rc = AVB_ERR_CTRL_TX;
		goto exit;
	}

	if (!len) {
		printf("%s: ERROR: NULL pointer passed for len parameter\n", __func__);
		rc = AVB_ERR_CTRL_TX;
		goto exit;
	}
	value_len = *len;

	if (sync && !entity_id) {
		printf("%s: ERROR: sync mode requested but cannot send a command without an entity id\n", __func__);
		rc = AVB_ERR_CTRL_TX;
		goto exit;
	}

	memset(aecp_cmd, 0, sizeof(struct aecp_aem_pdu));
	memset(set_control_cmd, 0, sizeof(struct aecp_aem_set_get_control_pdu));
	aecp_msg.len = sizeof(struct aecp_aem_pdu) + sizeof(struct aecp_aem_set_get_control_pdu) + value_len;
	if (aecp_msg.len > AVB_AECP_MAX_MSG_SIZE) {
		value_len -= aecp_msg.len - AVB_AECP_MAX_MSG_SIZE;
		aecp_msg.len = AVB_AECP_MAX_MSG_SIZE;
		printf("%s: WARNING: provided value doesn't fit in AEM message, clamping to %d instead of requested %d\n", __func__, value_len, *len);
	}
	aecp_msg.status = AECP_AEM_SUCCESS;
	if (entity_id) {
		AECP_AEM_SET_U_CMD_TYPE(aecp_cmd, 0, AECP_AEM_CMD_SET_CONTROL);
		aecp_msg.msg_type = AECP_AEM_COMMAND;
		memcpy(&aecp_cmd->entity_id, entity_id, 8);
	}
	else {
		AECP_AEM_SET_U_CMD_TYPE(aecp_cmd, unsolicited, AECP_AEM_CMD_SET_CONTROL);
		aecp_msg.msg_type = AECP_AEM_RESPONSE;
	}

	set_control_cmd->descriptor_type = htons(AEM_DESC_TYPE_CONTROL);
	set_control_cmd->descriptor_index = htons(descriptor_index);
	memcpy(values_cmd, value, value_len);

	if (sync) {
		unsigned int msg_type = AVB_MSG_AECP;
		unsigned int msg_len = sizeof(struct avb_aecp_msg);

		rc = avb_control_send_sync(ctrl_h, &msg_type, &aecp_msg, sizeof(struct avb_aecp_msg), &aecp_msg, &msg_len, -1);

		if ((rc == AVB_SUCCESS) && (msg_type == AVB_MSG_AECP)) {
			value_len = aecp_msg.len - sizeof(struct aecp_aem_pdu) - sizeof(struct aecp_aem_set_get_control_pdu);
			if (value_len > *len) {
				printf("%s: WARNING: response values (%d bytes) cannot fit in provided space (%d bytes)\n", __func__, value_len, *len);
				value_len = *len;
			}

			*len = value_len;
			memcpy(value, values_cmd, value_len);

			if (status)
				*status = aecp_msg.status;
		}
	}
	else
		rc = avb_control_send(ctrl_h, AVB_MSG_AECP, &aecp_msg, sizeof(struct avb_aecp_msg));

	if (rc != AVB_SUCCESS)
		printf("%s: avb_control_send() failed: AVB_MSG_AECP SET CONTROL entity_id(%" PRIx64 ") desc_idx(%d) value(%p)\n", __func__, entity_id?ntohll(*entity_id):0, descriptor_index, value);

exit:
	return rc;
}

/** [DEPRECATED] Sends an AECP SET_CONTROL command or response for a single UINT8 value type.
 *
 * This function will return without waiting for a response from the target entity.
 * \return AVB_SUCCESS on success, or genavb error code.
 * \param	ctrl_h				Handle to an avb_control_handle, opened for an AVB_CTRL_AVDECC_CONTROLLER or AVB_CTRL_AVDECC_CONTROLLED channel.
 * \param	entity_id			Pointer to the entity ID [Network Order] to send the message. If NULL, the message will be assumed to be an AECP response, and an AECP command otherwise.
 * \param	descriptor_index	Index of the CONTROL descriptor to modify.
 * \param	value				NEW value to set for the specified CONTROL descriptor (if the message is a command), or current value to report (if the message is a response).
 */
int aecp_aem_send_set_control_single_u8(struct avb_control_handle *ctrl_h, avb_u64 *entity_id, avb_u16 descriptor_index, avb_u8 value)
{
	avb_u16 len = 1;

	return aecp_aem_send_set_control(ctrl_h, entity_id, 1, descriptor_index, &value, &len, NULL, 0);
}

/** [DEPRECATED] Sends an AECP SET_CONTROL command or response for an UTF8 string type.
 *
 * This function will return without waiting for a response from the target entity.
 * \return AVB_SUCCESS on success, or genavb error code.
 * \param	ctrl_h				Handle to an avb_control_handle, opened for an AVB_CTRL_AVDECC_CONTROLLER or AVB_CTRL_AVDECC_CONTROLLED channel.
 * \param	entity_id			Pointer to the entity ID [Network Order] to send the message to. If NULL, the message will be assumed to be an AECP response, and an AECP command otherwise.
 * \param	descriptor_index	Index of the CONTROL descriptor to modify.
 * \param	value				NEW value to set for the specified CONTROL descriptor (if the message is a command), or current value to report (if the message is a response).
 */
int aecp_aem_send_set_control_utf8(struct avb_control_handle *ctrl_h, avb_u64 *entity_id, avb_u16 descriptor_index, char *value)
{
	avb_u16 len = strlen(value) + 1;
	return aecp_aem_send_set_control(ctrl_h, entity_id, 1, descriptor_index, value, &len, NULL, 0);
}

/** Sends an AECP SET_CONTROL command single UINT8 value type and optionally wait for a response.
 *
 * This function will return without waiting for a response from the target entity.
 * \return AVB_SUCCESS on success, or genavb error code.
 * \param	ctrl_h				Handle to an avb_control_handle, opened for an AVB_CTRL_AVDECC_CONTROLLER or AVB_CTRL_AVDECC_CONTROLLED channel.
 * \param	entity_id			Pointer to the entity ID [Network Order] to send the message.
 * \param	descriptor_index	Index of the CONTROL descriptor to modify.
 * \param	value				Pointer to the NEW value to set for the specified CONTROL descriptor (In sync mode, it always contains the current descriptor value).
 * \param	status				On return in sync mode, will contain the status from the AECP response if command was successful. Ignored otherwise.
 * \param	sync				If sync == 0, the function will return without waiting for a response from the target entity. Otherwise, the function will block until receiving a response
 */
int aecp_aem_send_set_control_single_u8_command(struct avb_control_handle *ctrl_h, avb_u64 *entity_id, avb_u16 descriptor_index, avb_u8 *value, avb_u8 *status, int sync)
{
	int rc;
	avb_u16 len = 1;

	if (!entity_id)
		rc = -AVB_ERR_INVALID_PARAMS;
	else
		rc = aecp_aem_send_set_control(ctrl_h, entity_id, 0, descriptor_index, value, &len, status, sync);

	return rc;
}

/** Sends an AECP SET_CONTROL response for a single UINT8 value type
 *
 * This function will return without waiting for a response from the target entity.
 * \return AVB_SUCCESS on success, or genavb error code.
 * \param	ctrl_h				Handle to an avb_control_handle, opened for an AVB_CTRL_AVDECC_CONTROLLER or AVB_CTRL_AVDECC_CONTROLLED channel.
 * \param	descriptor_index	Index of the CONTROL descriptor to modify.
 * \param	value				Pointer to the current value to report.
 */
int aecp_aem_send_set_control_single_u8_response(struct avb_control_handle *ctrl_h, avb_u16 descriptor_index, avb_u8 *value)
{
	avb_u16 len = 1;

	return aecp_aem_send_set_control(ctrl_h, NULL, 0, descriptor_index, value, &len, NULL, 0);
}

/** Sends an AECP SET_CONTROL unsolicited response for a single UINT8 value type
 *
 * This function will return without waiting for a response from the target entity.
 * \return AVB_SUCCESS on success, or genavb error code.
 * \param	ctrl_h				Handle to an avb_control_handle, opened for an AVB_CTRL_AVDECC_CONTROLLER or AVB_CTRL_AVDECC_CONTROLLED channel.
 * \param	descriptor_index	Index of the CONTROL descriptor to modify.
 * \param	value				Pointer to the current value to report.
 */
int aecp_aem_send_set_control_single_u8_unsolicited_response(struct avb_control_handle *ctrl_h, avb_u16 descriptor_index, avb_u8 *value)
{
	avb_u16 len = 1;

	return aecp_aem_send_set_control(ctrl_h, NULL, 1, descriptor_index, value, &len, NULL, 0);
}

/** Sends an AECP SET_CONTROL command for an UTF8 string type and optionally wait for a response.
 *
 * This function will return without waiting for a response from the target entity.
 * \return AVB_SUCCESS on success, or genavb error code.
 * \param	ctrl_h				Handle to an avb_control_handle, opened for an AVB_CTRL_AVDECC_CONTROLLER or AVB_CTRL_AVDECC_CONTROLLED channel.
 * \param	entity_id			Pointer to the entity ID [Network Order] to send the message.
 * \param	descriptor_index	Index of the CONTROL descriptor to modify.
 * \param	value				NEW value to set for the specified CONTROL descriptor.
 * \param	status				On return in sync mode, will contain the status from the AECP response if command was successful. Ignored otherwise.
 * \param	sync				If sync == 0, the function will return without waiting for a response from the target entity. Otherwise, the function will block until receiving a response
 */
int aecp_aem_send_set_control_utf8_command(struct avb_control_handle *ctrl_h, avb_u64 *entity_id, avb_u16 descriptor_index, char *value, avb_u8 *status, int sync)
{
	int rc;
	avb_u16 len = strlen(value) + 1;

	if (!entity_id)
		rc = -AVB_ERR_INVALID_PARAMS;
	else
		rc = aecp_aem_send_set_control(ctrl_h, entity_id, 0, descriptor_index, value, &len, status, sync);

	return rc;
}

/** Sends an AECP SET_CONTROL response for an UTF8 string type
 *
 * This function will return without waiting for a response from the target entity.
 * \return AVB_SUCCESS on success, or genavb error code.
 * \param	ctrl_h				Handle to an avb_control_handle, opened for an AVB_CTRL_AVDECC_CONTROLLER or AVB_CTRL_AVDECC_CONTROLLED channel.
 * \param	descriptor_index	Index of the CONTROL descriptor to modify.
 * \param	value				Current value to report.
 */
int aecp_aem_send_set_control_utf8_response(struct avb_control_handle *ctrl_h, avb_u16 descriptor_index, char *value)
{
	avb_u16 len = strlen(value) + 1;

	return aecp_aem_send_set_control(ctrl_h, NULL, 0, descriptor_index, value, &len, NULL, 0);
}

/** Sends an AECP SET_CONTROL unsolicited response for an UTF8 string type
 *
 * This function will return without waiting for a response from the target entity.
 * \return AVB_SUCCESS on success, or genavb error code.
 * \param	ctrl_h				Handle to an avb_control_handle, opened for an AVB_CTRL_AVDECC_CONTROLLER or AVB_CTRL_AVDECC_CONTROLLED channel.
 * \param	descriptor_index	Index of the CONTROL descriptor to modify.
 * \param	value				Current value to report.
 */
int aecp_aem_send_set_control_utf8_unsolicited_response(struct avb_control_handle *ctrl_h, avb_u16 descriptor_index, char *value)
{
	avb_u16 len = strlen(value) + 1;

	return aecp_aem_send_set_control(ctrl_h, NULL, 1, descriptor_index, value, &len, NULL, 0);
}

/** Sends an AECP GET_CONTROL command.
 *
 * \return AVB_SUCCESS on success, or genavb error code.
 * \param	ctrl_h				Handle to an avb_control_handle, opened for an AVB_CTRL_AVDECC_CONTROLLER channel.
 * \param	entity_id			Pointer to the entity ID [Network Order] to send the message to.
 * \param	descriptor_index	Index of the CONTROL descriptor to retrieve.
 * \param	value			On return, will contain the values from the response if sync != 0.
 * \param	len					Length of the memory area pointed to by value. If sync != 0, will be updated to contain the length of the values contained in the response.
 * \param	status				On return in sync mode, will contain the status from the AECP response if command was successful. Ignored otherwise.
 * \param	sync				If sync == 0, the function will return without waiting for a response from the target entity. Otherwise, the function will block until receiving a response
 * and value, len and status will be updated to match the response.
 *
 */
int aecp_aem_send_get_control(struct avb_control_handle *ctrl_h, avb_u64 *entity_id, avb_u16 descriptor_index, void *value, avb_u16 *len, avb_u8 *status, int sync)
{
	struct avb_aecp_msg aecp_msg;
	struct aecp_aem_pdu *aecp_cmd = (struct aecp_aem_pdu *) aecp_msg.buf;
	struct aecp_aem_set_get_control_pdu *get_control_cmd = (struct aecp_aem_set_get_control_pdu *)(aecp_cmd + 1);
	void *values_cmd = get_control_cmd + 1;
	avb_u16 value_len;
	int rc;

	memset(aecp_cmd, 0, sizeof(struct aecp_aem_pdu));
	memset(get_control_cmd, 0, sizeof(struct aecp_aem_set_get_control_pdu));
	aecp_msg.len = sizeof(struct aecp_aem_pdu) + sizeof(struct aecp_aem_set_get_control_pdu);
	AECP_AEM_SET_U_CMD_TYPE(aecp_cmd, 0, AECP_AEM_CMD_GET_CONTROL);
	aecp_msg.msg_type = AECP_AEM_COMMAND;
	memcpy(&aecp_cmd->entity_id, entity_id, 8);

	get_control_cmd->descriptor_type = htons(AEM_DESC_TYPE_CONTROL);
	get_control_cmd->descriptor_index = htons(descriptor_index);

	if (sync) {
		unsigned int msg_type = AVB_MSG_AECP;
		unsigned int msg_len = sizeof(struct avb_aecp_msg);

		rc = avb_control_send_sync(ctrl_h, &msg_type, &aecp_msg, sizeof(struct avb_aecp_msg), &aecp_msg, &msg_len, -1);

		if (value && len && (*len) && (rc == AVB_SUCCESS) && (msg_type == AVB_MSG_AECP) && (aecp_msg.status == AECP_AEM_SUCCESS)) {
			value_len = aecp_msg.len - sizeof(struct aecp_aem_pdu) - sizeof(struct aecp_aem_set_get_control_pdu);
			if (value_len > *len) {
				printf("%s: WARNING: response values (%d bytes) cannot fit in provided space (%d bytes)\n", __func__, value_len, *len);
				value_len = *len;
			}

			*len = value_len;
			memcpy(value, values_cmd, value_len);
		}

		if (status && (rc == AVB_SUCCESS))
			*status = aecp_msg.status;
	}
	else
		rc = avb_control_send(ctrl_h, AVB_MSG_AECP, &aecp_msg, sizeof(struct avb_aecp_msg));

	if (rc != AVB_SUCCESS)
		printf("%s: avb_control_send_sync() failed: AVB_MSG_AECP SET CONTROL entity_id(%" PRIx64 ") desc_idx(%d)\n", __func__, ntohll(*entity_id), descriptor_index);

	return rc;
}

/** Acquire or release a given entity.
 *
 * \return	1 on success, 0 if entity already acquired by another entity, -1 otherwise.
 * \param	ctrl_h				Handle to an avb_control_handle, opened for an AVB_CTRL_AVDECC_CONTROLLER channel.
 * \param	entity_id			Pointer to the entity ID [Network Order] to acquire/release.
 * \param	acquire				Set to 1 to acquire entity, 0 to release.
 */
int aecp_aem_acquire_entity(struct avb_control_handle *ctrl_h, avb_u64 *entity_id, int acquire)
{
	struct avb_aecp_msg aecp_msg;
	struct aecp_aem_pdu *aecp_cmd = (struct aecp_aem_pdu *) aecp_msg.buf;
	struct aecp_aem_acquire_entity_pdu *acquire_pdu = (struct aecp_aem_acquire_entity_pdu *)(aecp_cmd + 1);
	unsigned int msg_type, msg_len;
	int rc;

	memset(aecp_cmd, 0, sizeof(struct aecp_aem_pdu));
	memset(acquire_pdu, 0, sizeof(struct aecp_aem_acquire_entity_pdu));
	aecp_msg.len = sizeof(struct aecp_aem_pdu) + sizeof(struct aecp_aem_acquire_entity_pdu);
	AECP_AEM_SET_U_CMD_TYPE(aecp_cmd, 0, AECP_AEM_CMD_ACQUIRE_ENTITY);
	aecp_msg.msg_type = AECP_AEM_COMMAND;
	memcpy(&aecp_cmd->entity_id, entity_id, 8);

	acquire_pdu->descriptor_type = htons(AEM_DESC_TYPE_ENTITY);
	acquire_pdu->descriptor_index = htons(0);
	if (!acquire)
		acquire_pdu->flags = ntohl(AECP_AEM_ACQUIRE_RELEASE);

	msg_type = AVB_MSG_AECP;
	msg_len = sizeof(struct avb_aecp_msg);
	rc = avb_control_send_sync(ctrl_h, &msg_type, &aecp_msg, sizeof(struct avb_aecp_msg), &aecp_msg, &msg_len, -1);

	if ((rc == AVB_SUCCESS) && (msg_type == AVB_MSG_AECP)) {
		if (aecp_msg.status == AECP_AEM_SUCCESS)
			rc = 1;
		else if (aecp_msg.status == AECP_AEM_ENTITY_ACQUIRED)
			rc = 0;
		else
			rc = -1;
	} else
		rc = -1;

	return rc;
}


/** Sends an AECP READ_DESCRIPTOR command.
 *
 * \return AVB_SUCCESS on success, or genavb error code.
 * \param	ctrl_h				Handle to an avb_control_handle, opened for an AVB_CTRL_AVDECC_CONTROLLER channel.
 * \param	entity_id			Pointer to the entity ID [Network Order] to send the message to.
 * \param	configuration_index	Index of the Configuration to read the descriptor from.
 * \param	descriptor_type		Type of the descriptor to retrieve.
 * \param	descriptor_index	Index of the CONTROL descriptor to retrieve.
 * \param	descriptor_buffer	On return, will contain the requested descriptor if sync != 0.
 * \param	len					Length of the memory area pointed to by descriptor_buffer. If sync != 0, will be updated to contain the length of the values contained in the response.
 * \param	status				On return in sync mode, will contain the status from the AECP response if command was successful. Ignored otherwise.
 * \param	sync				If sync == 0, the function will return without waiting for a response from the target entity. Otherwise, the function will block until receiving a response
 * and descriptor, len and status will be updated to match the response.
 *
 */
int aecp_aem_send_read_descriptor(struct avb_control_handle *ctrl_h, avb_u64 *entity_id, avb_u16 configuration_index, avb_u16 descriptor_type, avb_u16 descriptor_index, void *buffer, avb_u16 *len, avb_u8 *status, int sync)
{
	struct avb_aecp_msg aecp_msg;
	struct aecp_aem_pdu *aecp_cmd = (struct aecp_aem_pdu *) aecp_msg.buf;
	struct aecp_aem_read_desc_cmd_pdu *read_desc_cmd = (struct aecp_aem_read_desc_cmd_pdu *)(aecp_cmd + 1);
	int rc;

	memset(aecp_cmd, 0, sizeof(struct aecp_aem_pdu));
	memset(read_desc_cmd, 0, sizeof(struct aecp_aem_read_desc_cmd_pdu));
	aecp_msg.len = sizeof(struct aecp_aem_pdu) + sizeof(struct aecp_aem_read_desc_cmd_pdu);
	AECP_AEM_SET_U_CMD_TYPE(aecp_cmd, 0, AECP_AEM_CMD_READ_DESCRIPTOR);
	aecp_msg.msg_type = AECP_AEM_COMMAND;
	memcpy(&aecp_cmd->entity_id, entity_id, 8);

	read_desc_cmd->configuration_index = htons(configuration_index);
	read_desc_cmd->descriptor_type = htons(descriptor_type);
	read_desc_cmd->descriptor_index = htons(descriptor_index);

	if (sync) {
		unsigned int msg_type = AVB_MSG_AECP;
		unsigned int msg_len = sizeof(struct avb_aecp_msg);
		rc = avb_control_send_sync(ctrl_h, &msg_type, &aecp_msg, sizeof(struct avb_aecp_msg), &aecp_msg, &msg_len, -1);

		if (buffer && len && (rc == AVB_SUCCESS) && (msg_type == AVB_MSG_AECP) && (aecp_msg.status == AECP_AEM_SUCCESS)) {
			struct aecp_aem_read_desc_rsp_pdu *read_desc_rsp = (struct aecp_aem_read_desc_rsp_pdu *)(aecp_cmd + 1);
			void *descriptor_rsp = read_desc_rsp + 1;
			avb_u16 descriptor_len;

			descriptor_len = aecp_msg.len - sizeof(struct aecp_aem_pdu) - sizeof(struct aecp_aem_read_desc_rsp_pdu);
			if (descriptor_len > *len) {
				printf("%s: WARNING: response values (%d bytes) cannot fit in provided space (%d bytes)\n", __func__, descriptor_len, *len);
				descriptor_len = *len;
			}

			*len = descriptor_len;
			memcpy(buffer, descriptor_rsp, descriptor_len);
		}

		if (status && (rc == AVB_SUCCESS))
			*status = aecp_msg.status;

	}
	else
		rc = avb_control_send(ctrl_h, AVB_MSG_AECP, &aecp_msg, sizeof(struct avb_aecp_msg));

	if (rc != AVB_SUCCESS)
		printf("%s: avb_control_send_sync() failed: AVB_MSG_AECP READ DESCRIPTOR entity_id(%" PRIx64 ") desc_idx(%d) error %s\n", __func__, ntohll(*entity_id), descriptor_index, avb_strerror(rc));

	return rc;
}


/** Sends an AECP START_STREAM command.
 *
 * \return AVB_SUCCESS on success, or genavb error code.
 * \param	ctrl_h				Handle to an avb_control_handle, opened for an AVB_CTRL_AVDECC_CONTROLLER channel.
 * \param	entity_id			Pointer to the entity ID [Network Order] to send the message to.
 * \param	descriptor_type			Type of the stream descriptor to start (AEM_DESC_TYPE_STREAM_OUTPUT or AEM_DESC_TYPE_STREAM_INPUT).
 * \param	descriptor_index		Index of the stream descriptor to start.
 * \param	status				On return in sync mode, will contain the status from the AECP response if command was successful. Ignored otherwise.
 * \param	sync				If sync == 0, the function will return without waiting for a response from the target entity. Otherwise, the function will block until receiving a response
 * and descriptor, len and status will be updated to match the response.
 *
 */
int aecp_aem_send_start_streaming(struct avb_control_handle *ctrl_h, avb_u64 *entity_id, avb_u16 descriptor_type, avb_u16 descriptor_index, avb_u8 *status, int sync)
{
	struct avb_aecp_msg aecp_msg;
	struct aecp_aem_pdu *aecp_cmd = (struct aecp_aem_pdu *) aecp_msg.buf;
	struct aecp_aem_start_streaming_cmd_pdu *start_streaming_cmd = (struct aecp_aem_start_streaming_cmd_pdu *)(aecp_cmd + 1);
	int rc;

	memset(aecp_cmd, 0, sizeof(struct aecp_aem_pdu));
	memset(start_streaming_cmd, 0, sizeof(struct aecp_aem_start_streaming_cmd_pdu));
	aecp_msg.len = sizeof(struct aecp_aem_pdu) + sizeof(struct aecp_aem_start_streaming_cmd_pdu);
	AECP_AEM_SET_U_CMD_TYPE(aecp_cmd, 0, AECP_AEM_CMD_START_STREAMING);
	aecp_msg.msg_type = AECP_AEM_COMMAND;
	memcpy(&aecp_cmd->entity_id, entity_id, 8);

	start_streaming_cmd->descriptor_type = htons(descriptor_type);
	start_streaming_cmd->descriptor_index = htons(descriptor_index);

	if (sync) {
		unsigned int msg_type = AVB_MSG_AECP;
		unsigned int msg_len = sizeof(struct avb_aecp_msg);
		rc = avb_control_send_sync(ctrl_h, &msg_type, &aecp_msg, sizeof(struct avb_aecp_msg), &aecp_msg, &msg_len, -1);

		if (status && (rc == AVB_SUCCESS))
			*status = aecp_msg.status;
	}
	else
		rc = avb_control_send(ctrl_h, AVB_MSG_AECP, &aecp_msg, sizeof(struct avb_aecp_msg));

	if (rc != AVB_SUCCESS)
		printf("%s: avb_control_send_sync() failed: AVB_MSG_AECP START_STREAMING entity_id(%" PRIx64 ") desc_idx(%d) error %s\n", __func__, ntohll(*entity_id), descriptor_index, avb_strerror(rc));

	return rc;
}

/** Sends an AECP STOP_STREAM command.
 *
 * \return AVB_SUCCESS on success, or genavb error code.
 * \param	ctrl_h				Handle to an avb_control_handle, opened for an AVB_CTRL_AVDECC_CONTROLLER channel.
 * \param	entity_id			Pointer to the entity ID [Network Order] to send the message.
 * \param	descriptor_type			Type of the stream descriptor to stop (AEM_DESC_TYPE_STREAM_OUTPUT or AEM_DESC_TYPE_STREAM_INPUT).
 * \param	descriptor_index		Index of the stream descriptor to stop.
 * \param	status				On return in sync mode, will contain the status from the AECP response if command was successful. Ignored otherwise.
 * \param	sync				If sync == 0, the function will return without waiting for a response from the target entity. Otherwise, the function will block until receiving a response
 * and descriptor, len and status will be updated to match the response.
 *
 */
int aecp_aem_send_stop_streaming(struct avb_control_handle *ctrl_h, avb_u64 *entity_id, avb_u16 descriptor_type, avb_u16 descriptor_index, avb_u8 *status, int sync)
{
	struct avb_aecp_msg aecp_msg;
	struct aecp_aem_pdu *aecp_cmd = (struct aecp_aem_pdu *) aecp_msg.buf;
	struct aecp_aem_stop_streaming_cmd_pdu *stop_streaming_cmd = (struct aecp_aem_stop_streaming_cmd_pdu *)(aecp_cmd + 1);
	int rc;

	memset(aecp_cmd, 0, sizeof(struct aecp_aem_pdu));
	memset(stop_streaming_cmd, 0, sizeof(struct aecp_aem_stop_streaming_cmd_pdu));
	aecp_msg.len = sizeof(struct aecp_aem_pdu) + sizeof(struct aecp_aem_stop_streaming_cmd_pdu);
	AECP_AEM_SET_U_CMD_TYPE(aecp_cmd, 0, AECP_AEM_CMD_STOP_STREAMING);
	aecp_msg.msg_type = AECP_AEM_COMMAND;
	memcpy(&aecp_cmd->entity_id, entity_id, 8);

	stop_streaming_cmd->descriptor_type = htons(descriptor_type);
	stop_streaming_cmd->descriptor_index = htons(descriptor_index);

	if (sync) {
		unsigned int msg_type = AVB_MSG_AECP;
		unsigned int msg_len = sizeof(struct avb_aecp_msg);
		rc = avb_control_send_sync(ctrl_h, &msg_type, &aecp_msg, sizeof(struct avb_aecp_msg), &aecp_msg, &msg_len, -1);

		if (status && (rc == AVB_SUCCESS))
			*status = aecp_msg.status;
	}
	else
		rc = avb_control_send(ctrl_h, AVB_MSG_AECP, &aecp_msg, sizeof(struct avb_aecp_msg));

	if (rc != AVB_SUCCESS)
		printf("%s: avb_control_send_sync() failed: AVB_MSG_AECP STOP_STREAMING entity_id(%" PRIx64 ") desc_idx(%d) error %s\n", __func__, ntohll(*entity_id), descriptor_index, avb_strerror(rc));

	return rc;
}

/** Creates a human-readable string describing an AVDECC format structure.
 *
 * \return	Number of bytes copied into str. May be more than size if output had to be truncated (the function will never copy more than size bytes into str), or a negative value on error.
 * \param 	format		Pointer to the AVDECC format structure to decode.
 * \param 	str			Pointer to character string to use as output buffer.
 * \param 	size		Size in bytes of the buffer pointed to by str.
 */
int avdecc_fmt_pretty_printf(const struct avdecc_format *format, char *str, size_t size)
{
#define APPEND_STR(...)	do { \
								rc = snprintf(current, remaining, __VA_ARGS__); \
								if (rc >= remaining) return (size - remaining + rc); \
								if (rc < 0) return rc; \
								current += rc; \
								remaining -= rc; \
							} while (0)


	char *current = str;
	int remaining = size;
	int rc;

	switch (format->u.s.subtype) {
	case AVTP_SUBTYPE_61883_IIDC:
		if (format->u.s.subtype_u.iec61883.sf == 0)
			APPEND_STR("IIDC");
		else
			switch (format->u.s.subtype_u.iec61883.fmt) {
			case IEC_61883_CIP_FMT_6:	// For AVDECC formats, fdf != IEC_61883_6_FDF_NODATA should be a valid assumption
				APPEND_STR("61883-6");
				switch (format->u.s.subtype_u.iec61883.format_u.iec61883_6.fdf_u.fdf.evt) {
				case IEC_61883_6_FDF_EVT_AM824:
					APPEND_STR(" AM824");
					break;
				case IEC_61883_6_FDF_EVT_FLOATING:
					APPEND_STR(" FLOAT");
					break;
				case IEC_61883_6_FDF_EVT_INT32:
					APPEND_STR(" INT32");
					break;
				default:
					APPEND_STR(" unknown");
					break;;
				}

				APPEND_STR(" %dchans", avdecc_fmt_channels_per_sample(format));
				APPEND_STR(" %dHz", avdecc_fmt_sample_rate(format));
				break;
			case IEC_61883_CIP_FMT_4:
				APPEND_STR("61883-4/MPEG2-TS");
				break;
			case IEC_61883_CIP_FMT_8:
				APPEND_STR("61883-8");
				break;
			default:
				APPEND_STR("undetermined");
			}


		break;
	case AVTP_SUBTYPE_AAF:
		APPEND_STR("AAF");
		APPEND_STR(" %dchans", avdecc_fmt_channels_per_sample(format));
		APPEND_STR(" %d/%dbits", format->u.s.subtype_u.aaf.format_u.pcm.bit_depth, avdecc_fmt_bits_per_sample(format));
		APPEND_STR(" %dHz", avdecc_fmt_sample_rate(format));
		APPEND_STR(" %dsamples/packet", avdecc_fmt_samples_per_packet(format, SR_CLASS_A)); //samples_per_packet doesn't rely on SR class
		break;

	case AVTP_SUBTYPE_CVF:
		APPEND_STR("CVF");
		break;
	default:
		APPEND_STR("undetermined");
	}

	return (size - remaining);
}
