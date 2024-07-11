/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @brief AECP common code
 @details Handles AECP stack
*/

#include "os/stdlib.h"
#include "os/log.h"
#include "os/string.h"

#include "common/log.h"
#include "common/types.h"

#include "genavb/aem.h"
#include "genavb/ptp.h"

#include "aecp.h"
#include "avdecc.h"

static const u8 aecp_mvu_protocol_id[6] = MILAN_VENDOR_UNIQUE_PROTOCOL_ID;

static int aecp_aem_send_command(struct aecp_ctx *aecp, struct avdecc_port *port, struct aecp_aem_pdu *pdu, struct net_tx_desc *desc, u8 *mac_dst, u16 len, struct ipc_tx *ipc, unsigned int ipc_dst);
static int aecp_aem_send_response(struct aecp_ctx *aecp, struct avdecc_port *port, struct aecp_aem_pdu *pdu, struct net_tx_desc *desc, u64 controller_id, u16 sequence_id, u8 status, u8 unsolicited, u8 *mac_dst, u16 len);

#define IS_VALID_GET_COUNTERS_DESCRIPTOR_TYPE(desc_type) ((desc_type) == AEM_DESC_TYPE_ENTITY || \
							 (desc_type) == AEM_DESC_TYPE_CLOCK_SOURCE || \
							 (desc_type) == AEM_DESC_TYPE_CLOCK_DOMAIN || \
							 (desc_type) == AEM_DESC_TYPE_AVB_INTERFACE || \
							 (desc_type) == AEM_DESC_TYPE_STREAM_INPUT)

static const char *aecp_mvu_cmdtype2string(aecp_mvu_command_type_t cmd_type)
{
	switch (cmd_type) {
	case2str(AECP_MVU_CMD_GET_MILAN_INFO);
	default:
		return (char *) "Unknown AECP MVU command type";
	}
}

static const char *aecp_aem_cmdtype2string(aecp_aem_command_type_t cmd_type)
{
	switch (cmd_type) {
	case2str(AECP_AEM_CMD_ACQUIRE_ENTITY);
	case2str(AECP_AEM_CMD_LOCK_ENTITY);
	case2str(AECP_AEM_CMD_ENTITY_AVAILABLE);
	case2str(AECP_AEM_CMD_CONTROLLER_AVAILABLE);
	case2str(AECP_AEM_CMD_READ_DESCRIPTOR);
	case2str(AECP_AEM_CMD_WRITE_DESCRIPTOR);
	case2str(AECP_AEM_CMD_SET_CONFIGURATION);
	case2str(AECP_AEM_CMD_GET_CONFIGURATION);
	case2str(AECP_AEM_CMD_SET_STREAM_FORMAT);
	case2str(AECP_AEM_CMD_GET_STREAM_FORMAT);
	case2str(AECP_AEM_CMD_SET_VIDEO_FORMAT);
	case2str(AECP_AEM_CMD_GET_VIDEO_FORMAT);
	case2str(AECP_AEM_CMD_SET_SENSOR_FORMAT);
	case2str(AECP_AEM_CMD_GET_SENSOR_FORMAT);
	case2str(AECP_AEM_CMD_SET_STREAM_INFO);
	case2str(AECP_AEM_CMD_GET_STREAM_INFO);
	case2str(AECP_AEM_CMD_SET_NAME);
	case2str(AECP_AEM_CMD_GET_NAME);
	case2str(AECP_AEM_CMD_SET_ASSOCIATION_ID);
	case2str(AECP_AEM_CMD_GET_ASSOCIATION_ID);
	case2str(AECP_AEM_CMD_SET_SAMPLING_RATE);
	case2str(AECP_AEM_CMD_GET_SAMPLING_RATE);
	case2str(AECP_AEM_CMD_SET_CLOCK_SOURCE);
	case2str(AECP_AEM_CMD_GET_CLOCK_SOURCE);
	case2str(AECP_AEM_CMD_SET_CONTROL);
	case2str(AECP_AEM_CMD_GET_CONTROL);
	case2str(AECP_AEM_CMD_INCREMENT_CONTROL);
	case2str(AECP_AEM_CMD_DECREMENT_CONTROL);
	case2str(AECP_AEM_CMD_SET_SIGNAL_SELECTOR);
	case2str(AECP_AEM_CMD_GET_SIGNAL_SELECTOR);
	case2str(AECP_AEM_CMD_SET_MIXER);
	case2str(AECP_AEM_CMD_GET_MIXER);
	case2str(AECP_AEM_CMD_SET_MATRIX);
	case2str(AECP_AEM_CMD_GET_MATRIX);
	case2str(AECP_AEM_CMD_START_STREAMING);
	case2str(AECP_AEM_CMD_STOP_STREAMING);
	case2str(AECP_AEM_CMD_REGISTER_UNSOLICITED_NOTIFICATION);
	case2str(AECP_AEM_CMD_DEREGISTER_UNSOLICITED_NOTIFICATION);
	case2str(AECP_AEM_CMD_IDENTIFY_NOTIFICATION);
	case2str(AECP_AEM_CMD_GET_AVB_INFO);
	case2str(AECP_AEM_CMD_GET_AS_PATH);
	case2str(AECP_AEM_CMD_GET_COUNTERS);
	case2str(AECP_AEM_CMD_REBOOT);
	case2str(AECP_AEM_CMD_GET_AUDIO_MAP);
	case2str(AECP_AEM_CMD_ADD_AUDIO_MAPPINGS);
	case2str(AECP_AEM_CMD_REMOVE_AUDIO_MAPPINGS);
	case2str(AECP_AEM_CMD_GET_VIDEO_MAP);
	case2str(AECP_AEM_CMD_ADD_VIDEO_MAPPINGS);
	case2str(AECP_AEM_CMD_REMOVE_VIDEO_MAPPINGS);
	case2str(AECP_AEM_CMD_GET_SENSOR_MAP);
	case2str(AECP_AEM_CMD_ADD_SENSOR_MAPPINGS);
	case2str(AECP_AEM_CMD_REMOVE_SENSOR_MAPPINGS);
	case2str(AECP_AEM_CMD_START_OPERATION);
	case2str(AECP_AEM_CMD_ABORT_OPERATION);
	case2str(AECP_AEM_CMD_OPERATION_STATUS);
	case2str(AECP_AEM_CMD_AUTH_ADD_KEY);
	case2str(AECP_AEM_CMD_AUTH_DELETE_KEY);
	case2str(AECP_AEM_CMD_AUTH_GET_KEY_LIST);
	case2str(AECP_AEM_CMD_AUTH_GET_KEY);
	case2str(AECP_AEM_CMD_AUTH_ADD_KEY_TO_CHAIN);
	case2str(AECP_AEM_CMD_AUTH_DELETE_KEY_FROM_CHAIN);
	case2str(AECP_AEM_CMD_AUTH_GET_KEYCHAIN_LIST);
	case2str(AECP_AEM_CMD_AUTH_GET_IDENTITY);
	case2str(AECP_AEM_CMD_AUTH_ADD_TOKEN);
	case2str(AECP_AEM_CMD_AUTH_DELETE_TOKEN);
	case2str(AECP_AEM_CMD_AUTHENTICATE);
	case2str(AECP_AEM_CMD_DEAUTHENTICATE);
	case2str(AECP_AEM_CMD_ENABLE_TRANSPORT_SECURITY);
	case2str(AECP_AEM_CMD_DISABLE_TRANSPORT_SECURITY);
	case2str(AECP_AEM_CMD_ENABLE_STREAM_ENCRYPTION);
	case2str(AECP_AEM_CMD_DISABLE_STREAM_ENCRYPTION);
	case2str(AECP_AEM_CMD_SET_MEMORY_OBJECT_LENGTH);
	case2str(AECP_AEM_CMD_GET_MEMORY_OBJECT_LENGTH);
	case2str(AECP_AEM_CMD_SET_STREAM_BACKUP);
	case2str(AECP_AEM_CMD_GET_STREAM_BACKUP);
	case2str(AECP_AEM_CMD_EXPANSION);
	default:
		return (char *) "Unknown AECP AEM command type";
	}
}

/** Helper function to display basic information about a given AECP AEM PDU.
 *
 * \param pdu	pointer to AECP AEM PDU.
 * \param msg_type	message type of the PDU (from the AVTP part of the packet)
 * \param status	status field of the PDU (from the AVTP part of the packet)
 */
static inline void debug_dump_aecp_aem(struct aecp_ctx *aecp, struct aecp_aem_pdu *pdu, int msg_type, u16 status)
{
	u64 controller_entity_id = pdu->controller_entity_id;
	u64 entity_id = pdu->entity_id;

	os_log(LOG_DEBUG, "aecp(%p) AEM message type(%x) status(%u) u(%d) command_type(%x) entity(%016"PRIx64") controller(%016"PRIx64") seq_id(%d)\n",
			aecp, msg_type, status, AECP_AEM_GET_U(pdu), AECP_AEM_GET_CMD_TYPE((struct aecp_aem_pdu *)pdu),
			ntohll(entity_id), ntohll(controller_entity_id), ntohs(pdu->sequence_id));
}

/*
 * Monitor timer handler that will check if registered controller is still available
 * \param data, the registered controller
 */
static void aecp_monitor_timer_handler(void *data)
{
	struct unsolicited_ctx *entry = (struct unsolicited_ctx *)data;
	struct aecp_ctx *aecp = entry->aecp;
	struct entity *entity = container_of(aecp, struct entity, aecp);
	struct avdecc_port *port_rsp = &entity->avdecc->port[entry->port_id];
	struct net_tx_desc *net_desc;
	struct aecp_aem_pdu *aecp_pdu;
	u16 len	= sizeof(struct aecp_aem_pdu);
	void *buf;

	net_desc = net_tx_alloc(&port_rsp->net_tx, DEFAULT_NET_DATA_SIZE);
	if (!net_desc) {
		os_log(LOG_ERR,"aecp(%p): Cannot alloc net_tx\n", aecp);
		goto err;
	}

	buf = NET_DATA_START(net_desc);

	aecp_pdu = (struct aecp_aem_pdu *)((char *)buf + OFFSET_TO_AECP);

	AECP_AEM_SET_U_CMD_TYPE(aecp_pdu, 0, AECP_AEM_CMD_CONTROLLER_AVAILABLE);

	/* PDU's entity id is the controller entity id as the controller is the target of the command otherwise the controller would ignore the command */
	copy_64(&aecp_pdu->controller_entity_id, &entity->desc->entity_id);
	copy_64(&aecp_pdu->entity_id, &entry->controller_id);

	if (aecp_aem_send_command(aecp, port_rsp, aecp_pdu, net_desc, entry->mac_dst, len, NULL, 0) < 0) {

		os_log(LOG_ERR,"aecp(%p) port(%u) couldn't send command CONTROLLER_AVAILABLE to controller(%016"PRIx64").\n",
				aecp, entry->port_id, ntohll(entry->controller_id));

		goto err;
	}

	os_log(LOG_DEBUG,"aecp(%p) port(%u) Sent CONTROLLER_AVAILABLE to controller(%016"PRIx64") mac dest(%016"PRIx64").\n",
			aecp, entry->port_id, ntohll(entry->controller_id), NTOH_MAC_VALUE(entry->mac_dst));

err:
	return;
}

static void aecp_milan_get_counters_async_unsolicited_notification_timer_handler(void *data)
{
	struct avb_interface_dynamic_desc *avb_itf_dynamic = (struct avb_interface_dynamic_desc *)data;
	struct entity *entity = avb_itf_dynamic->entity;

	aecp_aem_send_async_unsolicited_notification(&entity->aecp, AECP_AEM_CMD_GET_COUNTERS, AEM_DESC_TYPE_AVB_INTERFACE, avb_itf_dynamic->interface_index);
}

static void aecp_get_as_path_async_unsolicited_notification_timer_handler(void *data)
{
	struct avb_interface_dynamic_desc *avb_itf_dynamic = (struct avb_interface_dynamic_desc *)data;
	struct entity *entity = avb_itf_dynamic->entity;

	avb_itf_dynamic->async_get_as_path_unsolicited_notification_timer_running = false;

	if (avb_itf_dynamic->async_get_as_path_unsolicited_notification_pending) {
		aecp_trigger_async_get_as_path_notification(entity, avb_itf_dynamic->interface_index);
	}
}

__init static void aecp_unsolicited_init(struct aecp_ctx *aecp)
{
	int i;

	list_head_init(&aecp->free_unsolicited);
	list_head_init(&aecp->unsolicited);

	for (i = 0; i < aecp->max_unsolicited_registrations; i++) {
		list_add(&aecp->free_unsolicited, &aecp->unsolicited_storage[i].list);
		os_memset(aecp->unsolicited_storage[i].mac_dst, 0, 6);
	}

	os_log(LOG_INIT, "aecp(%p) %d unsolicited registration max\n", aecp, aecp->max_unsolicited_registrations);
}

/** Find an unsolicited entry based on the controller ID.
 * \param	aecp		AECP context to search into.
 * \param	controller_id	Pointer to the controller ID to match.
 * \param	port_id		avdecc port / interface index on which the controller has registered
 * \return	pointer to unsolicited context matching both controller_id and port_id, or NULL if none found.
 */
static struct unsolicited_ctx *aecp_unsolicited_find(struct aecp_ctx *aecp, u64 controller_id, unsigned int port_id)
{
	struct list_head *list_entry;
	struct unsolicited_ctx *entry;

	list_entry = list_first(&aecp->unsolicited);

	while (list_entry != &aecp->unsolicited) {
		entry = container_of(list_entry, struct unsolicited_ctx, list);

		if ((controller_id == entry->controller_id) && (port_id == entry->port_id))
			return entry;

		list_entry = list_next(list_entry);
	}

	return NULL;
}


/** Add an unsolicited entry to the list.
 * \param	aecp		AECP context where the entry should be added.
 * \param	mac_dst		Pointer to the MAC address of the controller to add to the list.
 * \param	controller_id	Pointer to the ID of the controller to add to the list.
 * \param	port_id		avdecc port / interface index on which the PDU was received
 * \return	* 0 if the entry was added successfully,
 * * 1 if the entry was already present (in such a case, the MAC address of the entry is updated),
 * * -1 on failure.
 */
static int aecp_unsolicited_add(struct aecp_ctx *aecp, u8 *mac_dst, u64 controller_id, unsigned int port_id)
{
	struct entity *entity = container_of(aecp, struct entity, aecp);
	struct avdecc_ctx *avdecc = entity->avdecc;
	struct unsolicited_ctx *entry = NULL;
	struct list_head *list_entry;

	entry = aecp_unsolicited_find(aecp, controller_id, port_id);
	if (entry) {
		os_memcpy(entry->mac_dst, mac_dst, 6);
		return 1;
	}

	if (!list_empty(&aecp->free_unsolicited)) {
		list_entry = list_first(&aecp->free_unsolicited);
		entry = container_of(list_entry, struct unsolicited_ctx, list);
		list_del(list_entry);

		os_memcpy(entry->mac_dst, mac_dst, 6);
		entry->port_id = port_id;
		entry->controller_id = controller_id;
		entry->sequence_id = 0; /* Per AVNU.IO.CONTROL 7.3.21 */
		entry->aecp = aecp;

		list_add(&aecp->unsolicited, &entry->list);

		/* Monitor timer. Per AVNU.IO.CONTROL 7.5.3 */
		entry->monitor_timer.func = aecp_monitor_timer_handler;
		entry->monitor_timer.data = entry;

		if (timer_create(avdecc->timer_ctx, &entry->monitor_timer, 0, MONITOR_TIMER_GRANULARITY) < 0) {
			os_log(LOG_ERR,"aecp(%p) port(%u) couldn't create timer for controller(%016"PRIx64").\n",
					aecp, entry->port_id, ntohll(entry->controller_id));

			goto err;
		}

		timer_start(&entry->monitor_timer, MONITOR_TIMER_INTERVAL);

		return 0;
	} else {
		os_log(LOG_ERR, "No more unsolicited entries available\n");
		goto err;
	}

err:
	return -1;
}

/** Remove an unsolicited entry from the list.
 *
 * \return 0 on success (entry found and removed) or -1 on failure.
 * \param	aecp		AECP context the entry should be removed from.
 * \param	controller_id	pointer to the ID of the controller to be removed.
 * \param	port_id		avdecc port / interface index on which the PDU was received
 */
static int aecp_unsolicited_remove(struct aecp_ctx *aecp, u64 controller_id, unsigned int port_id)
{
	struct unsolicited_ctx *entry = aecp_unsolicited_find(aecp, controller_id, port_id);

	if (entry) {
		list_del(&entry->list);
		list_add(&aecp->free_unsolicited, &entry->list);

		timer_destroy(&entry->monitor_timer);

		return 0;
	} else {
		return -1;
	}
}

/** Checks a given value matches a CONTROL descriptor definition.
 *
 * \return 1 if validation successful or undetermined, 0 otherwise.
 * \param desc		CONTROL descriptor to validate against.
 * \param raw_value	Pointer to the value that should be validated.
 * \param len		Length in bytes of the memory area pointed to by raw_value;
 */
static int aecp_aem_validate_control_value(struct control_descriptor *desc, void *raw_value, u16 len)
{
	int rc = 1;
	int control_value_type = AEM_CONTROL_GET_VALUE_TYPE(ntohs(desc->control_value_type));

	switch (control_value_type) {
	case AEM_CONTROL_LINEAR_UINT8:
	{
		int i;
		u8 *values = raw_value;

		if (len != ntohs(desc->number_of_values)) {
			os_log(LOG_ERR, "Invalid UINT8 Control value: length not equal number_of_values %u != %u \n",
				len, ntohs(desc->number_of_values));
			rc = 0;
			break;
		}

		for (i = 0; i < ntohs(desc->number_of_values); i++) {
			if ((values[i] < desc->value_details.linear_int8[i].min) || (values[i] > desc->value_details.linear_int8[i].max)) {
				os_log(LOG_ERR, "Invalid UINT8 Control value: values[%d] = %u  is out of bound [%u,%u].\n",
					i, values[i], desc->value_details.linear_int8[i].min,
					desc->value_details.linear_int8[i].max);
				rc = 0;
				break;
			}
		}

		break;
	}
	case AEM_CONTROL_UTF8:
	{
		int count = 0;

		if (len > AEM_UTF8_MAX_LENGTH) {
			os_log(LOG_ERR, "Invalid UTF8 Control value: size exceeds max supported %u > %u \n",
				len, AEM_UTF8_MAX_LENGTH);
			rc = 0;
			break;
		}

		count = os_strnlen(raw_value, len);
		if (count == len) {
			os_log(LOG_ERR, "Invalid UTF8 Control value: there is no null terminating character in buffer of size %u \n", len);
			rc = 0;
		} else if (count < (len - 1)) {
			os_log(LOG_ERR, "Invalid UTF8 Control value: value (%s) of size (%u) should have only one null character as last byte:"
					"null character at index (%u) instead of (%u) \n", (char *)raw_value, len, count, len - 1);
			rc = 0;
		}

		break;
	}
	default:
		os_log(LOG_INFO, "Unsupported CONTROL value_type(%d), validity of value undetermined.\n", control_value_type);
		break;
	}

	return rc;
}

/** Copies the descriptor value(s) from a CONTROL descriptor to the values field of a SET_CONTROL PDU.
 *
 * \return -1 on failure, otherwise the number of bytes copied.
 * \param desc		CONTROL descriptor to copy values from.
 * \param raw_value	Pointer to the values field of a SET_CONTROL PDU.
 * \param len		Length in bytes of the memory area pointed to by raw_value. No more than len bytes will be copied.
 *
 */
static int aecp_aem_control_desc_to_pdu(struct control_descriptor *desc, void *raw_value, u16 len)
{
	int count;
	int control_value_type = AEM_CONTROL_GET_VALUE_TYPE(ntohs(desc->control_value_type));

	if (!len) {
		os_log(LOG_ERR, "Invalid null length\n");
		count = -1;
		goto exit;
	}

	switch (control_value_type) {
	case AEM_CONTROL_LINEAR_UINT8:
	{
		int i;
		u8 *values = raw_value;

		if (len < ntohs(desc->number_of_values)) {
			os_log(LOG_ERR, "Invalid UINT8 Control value: length inferior to number_of_values %u < %u \n",
				len, ntohs(desc->number_of_values));
			count = -1;
			goto exit;
		}

		count = ntohs(desc->number_of_values);

		for (i = 0; i < count; i++)
			values[i] = desc->value_details.linear_int8[i].current;

		break;
	}
	case AEM_CONTROL_UTF8:
		count = os_strnlen((char *)desc->value_details.utf8.string, AEM_UTF8_MAX_LENGTH - 1) + 1;  // Remove 1 from max and add it back to account for terminating 0.

		if (len < count) {
			os_log(LOG_ERR, "Invalid UTF8 Control value: length inferior to descriptor value size %u < %u \n", len, count);
			count = -1;
			goto exit;
		}

		os_memcpy(raw_value, desc->value_details.utf8.string, count);

		break;
	default:
		os_log(LOG_INFO, "Unsupported CONTROL value_type(%d) \n", control_value_type);
		count = -1;
		break;
	}
exit:
	return count;
}

/** Copies the descriptor value(s), if valid, from the values field of a SET_CONTROL PDU to a CONTROL descriptor.
 *
 * \return -1 on failure, 0 nothing copied (values are the same as the desciptor) and 1 if values are copied.
 * \param desc		CONTROL descriptor to copy values to.
 * \param raw_value	Pointer to the values field of a SET_CONTROL PDU.
 * \param len		Length in bytes of the memory area pointed to by raw_value. No more than len bytes will be copied.
 *
 */
static int aecp_aem_control_pdu_to_desc(struct control_descriptor *desc, void *raw_value, u16 len)
{
	int rc = 0;
	int control_value_type = AEM_CONTROL_GET_VALUE_TYPE(ntohs(desc->control_value_type));

	/* Validate the values */
	if (!aecp_aem_validate_control_value(desc, raw_value, len)) {
		rc = -1;
		goto exit;
	}

	switch (control_value_type) {
	case AEM_CONTROL_LINEAR_UINT8:
	{
		int i;
		u8 *values = raw_value;

		for (i = 0; i < ntohs(desc->number_of_values); i++) {
			/* Copy values only if differents */
			if (desc->value_details.linear_int8[i].current != values[i]) {
				desc->value_details.linear_int8[i].current = values[i];
				rc = 1;
			}
		}

		break;
	}
	case AEM_CONTROL_UTF8:
	{
		int count = os_strnlen(raw_value, len - 1) + 1;  // Remove 1 from max and add it back to account for terminating 0.
		int desc_values_len = os_strnlen((char *)desc->value_details.utf8.string, AEM_UTF8_MAX_LENGTH - 1) + 1;

		/* Copy value only if different */
		if ((count != desc_values_len) ||
			(os_memcmp(desc->value_details.utf8.string, raw_value, count) != 0)) {

			os_memcpy(desc->value_details.utf8.string, raw_value, count);
			rc = 1;
		}

		break;
	}
	default:
		os_log(LOG_INFO, "Unsupported CONTROL value_type(%d), 0 bytes copied.\n", control_value_type);
		rc = -1;
		break;
	}

exit:
	return rc;
}

/** Send an AECP AEM message through an IPC channel.
 *
 * \return 0 on success or -1 on failure.
 * \param aecp		AECP context.
 * \param pdu		Pointer to AECP AEM PDU to send (content will be copied into the IPC message).
 * \param msg_type	Message type of the AECP AEM PDU (normally one of AECP_AEM_COMMAND, AECP_AEM_RESPONSE)
 * \param status	Status of the AECP AEM PDU.
 * \param len		Length of the EACP AEM PDU.
 * \param ipc		IPC channel to send the message through.
 */
static int aecp_aem_ipc_tx(struct aecp_ctx *aecp, struct aecp_aem_pdu *pdu, u8 msg_type, u8 status, u16 len, struct ipc_tx *ipc, unsigned int ipc_dst)
{
	struct entity *entity = container_of(aecp, struct entity, aecp);
	struct avdecc_ctx *avdecc = entity->avdecc;
	struct ipc_desc *desc;
	int rc = -1;

	os_log(LOG_DEBUG, "aecp(%p) pdu(%p) len(%u) ipc(%p)\n", aecp, pdu, len, ipc);
	debug_dump_aecp_aem(aecp, pdu, msg_type, status);

	desc = ipc_alloc(ipc, sizeof(struct genavb_aecp_msg));
	if (desc) {
		desc->dst = ipc_dst;
		desc->type = GENAVB_MSG_AECP;
		desc->len = sizeof(struct genavb_aecp_msg);

		desc->u.aecp_msg.msg_type =  msg_type;
		desc->u.aecp_msg.status = status;
		if (len > AVB_AECP_MAX_MSG_SIZE) {
			os_log(LOG_ERR, "aecp(%p) Truncating PDU to fit inside IPC buffer, length above limit (%d > %d).\n", aecp, len, AVB_AECP_MAX_MSG_SIZE);
			len = AVB_AECP_MAX_MSG_SIZE;
		}
		desc->u.aecp_msg.len = len;
		os_memcpy(&desc->u.aecp_msg.buf, pdu, len);

		if (ipc_tx(ipc, desc) < 0) {
			os_log(LOG_ERR, "avdecc(%p) ipc_tx() failed\n", avdecc);
			goto err_ipc_tx;
		}

		rc = 0;
	} else {
		os_log(LOG_ERR, "avdecc(%p) ipc_alloc() failed\n", avdecc);
	}

	return rc;

err_ipc_tx:
	ipc_free(ipc, desc);
	return rc;
}

static inline int aecp_aem_ipc_tx_command(struct aecp_ctx *aecp, struct aecp_aem_pdu *pdu, u16 len, struct ipc_tx *ipc, unsigned int ipc_dst)
{
	return aecp_aem_ipc_tx(aecp, pdu, AECP_AEM_COMMAND, AECP_AEM_SUCCESS, len, ipc, ipc_dst);
}

static inline int aecp_aem_ipc_tx_response(struct aecp_ctx *aecp, struct aecp_aem_pdu *pdu, u8 status, u16 len, struct ipc_tx *ipc, unsigned int ipc_dst)
{
	return aecp_aem_ipc_tx(aecp, pdu, AECP_AEM_RESPONSE, status, len, ipc, ipc_dst);
}

/** Allocates a new network tx descriptor and initializes it as an AECP PDU.
 *
 * \return pointer to the new network descriptor, or NULL otherwise.
 * \param port		Pointer to the avdecc port on which the packet will be sent
 * \param buf		Pointer to an AECP PDU whose content should be copied into the new descriptor.
 * \param len		Length of the AECP PDU to be copied.
 * \param pdu		On return, *pdu will point to the start of the AECP PDU within the new descriptor.
 */
static struct net_tx_desc *aecp_net_tx_prepare(struct avdecc_port *port, void *buf, u16 *len, void **pdu)
{
	struct net_tx_desc *desc = NULL;
	void *tx_buf;

	desc = net_tx_alloc(&port->net_tx, DEFAULT_NET_DATA_SIZE);
	if (!desc) {
		os_log(LOG_ERR, "Cannot alloc tx descriptor\n");
		goto exit;
	}

	if (*len > AVDECC_AECP_MAX_SIZE) {
		os_log(LOG_ERR, "Truncating AECP message, length above spec (%u > %zu).\n", *len, AVDECC_AECP_MAX_SIZE);
		*len = AVDECC_AECP_MAX_SIZE;
	}

	tx_buf = NET_DATA_START(desc);
	*pdu = (void *)((char *)tx_buf + OFFSET_TO_AECP);
	os_memcpy(*pdu, buf, *len);

exit:
	return desc;
}

/**
 * Sends an AECP AEM packet on the network.
 * \return 	0 on success, negative otherwise
 * \param	aecp		Pointer to the AECP context.
 * \param	port		Pointer to the avdecc port on which the packet will be sent
 * \param	desc		Packet descriptor to send.
 * \param	status		AECP status (9.2.1.1.6), to be placed in the protocol-specific portions of the AVTP control header.
 * \param	msg_type	AECP message type (9.2.2.1.5), to be placed in the protocol-specific portions of the AVTP control header.
 * \param	mac_dst		MAC address to use as destination.
 * \param	len			Length of the data beyond the AVTP control header.
 */
static int aecp_aem_net_tx(struct aecp_ctx *aecp, struct avdecc_port *port, struct net_tx_desc *desc, u8 msg_type, u8 status, u8 *mac_dst, u16 len)
{
	void *buf_rsp;

	//TODO unify AECP/ACMP codes (at least similar logic if not same code)

	buf_rsp = NET_DATA_START(desc);

	desc->len += net_add_eth_header(buf_rsp, mac_dst, ETHERTYPE_AVTP);
	desc->len += avdecc_add_common_header((char *)buf_rsp + desc->len, AVTP_SUBTYPE_AECP, msg_type, len - sizeof(u64), status);
	desc->len += len;

	os_log(LOG_DEBUG, "aecp(%p) port(%u) AECP message desc(%p) len(%u) total_len(%u) destination mac(%012"PRIx64")\n",
		aecp, port->port_id, desc, len, desc->len, NTOH_MAC_VALUE(mac_dst));

	debug_dump_aecp_aem(aecp, (struct aecp_aem_pdu *)((char *)buf_rsp + OFFSET_TO_AECP), msg_type, status);

	if (avdecc_net_tx(port, desc) < 0) {
		os_log(LOG_ERR, "aecp(%p) port(%u) send failed\n", aecp, port->port_id);
		goto err;
	}

	return 0;

err:
	return -1;
}

static inline int aecp_aem_net_tx_command(struct aecp_ctx *aecp, struct avdecc_port *port, struct net_tx_desc *desc, u8 *mac_dst, u16 len)
{
	return aecp_aem_net_tx(aecp, port, desc, AECP_AEM_COMMAND, AECP_AEM_SUCCESS, mac_dst, len);
}

static inline int aecp_aem_net_tx_response(struct aecp_ctx *aecp, struct avdecc_port *port, struct net_tx_desc *desc, u8 status, u8 *mac_dst, u16 len)
{
	return aecp_aem_net_tx(aecp, port, desc, AECP_AEM_RESPONSE, status, mac_dst, len);
}

static inline int aecp_mvu_net_tx_response(struct aecp_ctx *aecp, struct avdecc_port *port, struct net_tx_desc *desc, u8 status, u8 *mac_dst, u16 len)
{
	return aecp_aem_net_tx(aecp, port, desc, AECP_VENDOR_UNIQUE_RESPONSE, status, mac_dst, len);
}

static int aecp_aem_inflight_network_timeout(struct inflight_ctx *entry)
{
	struct aecp_ctx *aecp = container_of(entry->list_head, struct aecp_ctx, inflight_network);
	struct entity *entity = container_of(aecp, struct entity, aecp);
	struct avdecc_port *port = &entity->avdecc->port[entry->data.port_id];
	int rc = 0;
	bool send_ipc = true;

	os_log(LOG_DEBUG, "aecp(%p) inflight_ctx(%p) sequence id : %x\n", aecp, entry, entry->data.sequence_id);

	if (entry->data.retried) {
		switch (AECP_AEM_GET_CMD_TYPE(&entry->data.pdu.aem)) {
		case AECP_AEM_CMD_CONTROLLER_AVAILABLE:
		{
			/* As the controller didn't respond to our CONTROLLER_AVAILABLE command,
			* we remove it from the registred controllers list and notify it with a DEREGISTER_UNSOLICITED_NOTIFICATION.
			* Per AVNU.IO.CONTROL 7.5.3
			*/
			struct net_tx_desc *net_desc;
			struct aecp_aem_pdu *aecp_notif;
			struct unsolicited_ctx *unsolicited_entry;
			u16 len	= sizeof(struct aecp_aem_pdu);
			void *buf;

			send_ipc = false;

			unsolicited_entry = aecp_unsolicited_find(aecp, entry->data.pdu.aem.entity_id, entry->data.port_id);
			if (!unsolicited_entry) {
				os_log(LOG_ERR,"aecp(%p) port(%u) couldn't find registered controller(%016"PRIx64").\n",
						aecp, entry->data.port_id, ntohll(entry->data.pdu.aem.entity_id));
				break;
			}

			net_desc = net_tx_alloc(&port->net_tx, DEFAULT_NET_DATA_SIZE);
			if (!net_desc) {
				os_log(LOG_ERR,"aecp(%p): Cannot alloc net_tx\n", aecp);
				break;
			}

			buf = NET_DATA_START(net_desc);

			aecp_notif = (struct aecp_aem_pdu *)((char *)buf + OFFSET_TO_AECP);

			AECP_AEM_SET_U_CMD_TYPE(aecp_notif, 1, AECP_AEM_CMD_DEREGISTER_UNSOLICITED_NOTIFICATION);

			if (aecp_aem_send_response(aecp, port, aecp_notif, net_desc, entry->data.pdu.aem.entity_id,
							unsolicited_entry->sequence_id, AECP_AEM_SUCCESS, 1, entry->data.mac_dst, len) < 0) {

				os_log(LOG_ERR,"aecp(%p) port(%u) couldn't send DEREGISTER_UNSOLICITED_NOTIFICATION to controller(%016"PRIx64").\n",
						aecp, entry->data.port_id, ntohll(entry->data.pdu.aem.entity_id));

				break;
			}

			aecp_unsolicited_remove(aecp, entry->data.pdu.aem.entity_id, entry->data.port_id);

			os_log(LOG_DEBUG,"aecp(%p) port(%u) removing timed out controller(%016"PRIx64") from registered list.\n",
					aecp, entry->data.port_id, ntohll(entry->data.pdu.aem.entity_id));

			break;
		}
		default:
			break;
		}

		/* Send error response back to app */
		if (send_ipc)
			aecp_aem_ipc_tx_response(aecp, &entry->data.pdu.aem, AECP_AEM_TIMEOUT, entry->data.len, (void *)entry->data.priv[0], (unsigned int)entry->data.priv[1]);

		rc = AVDECC_INFLIGHT_TIMER_STOP;
	}
	else {
		/* Try sending the command one more time */
		struct net_tx_desc *desc;
		struct aecp_aem_pdu *pdu;

		desc = aecp_net_tx_prepare(port, entry->data.pdu.buf, &entry->data.len, (void **)&pdu);

		if (!desc) {
			os_log(LOG_ERR, "aecp(%p) Cannot prepare tx descriptor\n", aecp);
			rc = AVDECC_INFLIGHT_TIMER_STOP;
		} else {
			rc = aecp_aem_net_tx_command(aecp, port, desc, entry->data.mac_dst, entry->data.len);
			if (rc < 0)
				rc = AVDECC_INFLIGHT_TIMER_STOP;
			else
				rc = AVDECC_INFLIGHT_TIMER_RESTART;

			entry->data.retried = 1;
		}
	}

	return rc;
}

/** Sends an AECP AEM failure response on the network.
 * Takes full ownership of the TX descriptor
 * This function will take an inflight command PDU and update needed fields to send failure responses
 * \return 		0 on success or negative value otherwise.
 * \param aecp		AECP context the command is being sent from.
 * \param pdu		Pointer to the start of the sent AECP AEM PDU.
 * \param port		Pointer to the avdecc port on which the packet will be sent
 * \param desc		Network TX descriptor.
 * \param mac_dst	MAC address to send the command to.
 * \param status	Failure Status of the AECP AEM PDU.
 * \param len		Length of the AECP AEM PDU (after the AVTP header), can be modified on return.
*/
static int aecp_aem_net_tx_inflight_response_failure(struct aecp_ctx *aecp, struct aecp_aem_pdu *pdu, struct avdecc_port *port, struct net_tx_desc *desc, u8 *mac_dst, u8 status, u16 *len)
{
	int rc;

	switch (AECP_AEM_GET_CMD_TYPE(pdu)) {
	case AECP_AEM_CMD_SET_CONTROL:
	{
		/* SET_CONTROL response failures should contain current value in descriptor */
		struct aecp_aem_set_get_control_pdu *set_control_rsp = (struct aecp_aem_set_get_control_pdu *)(pdu + 1);
		void *values_rsp = set_control_rsp + 1;
		struct entity *entity = container_of(aecp, struct entity, aecp);
		struct control_descriptor *ctrl_desc;
		int desc_values_len;

		ctrl_desc = aem_get_descriptor(entity->aem_descs, ntohs(set_control_rsp->descriptor_type),
						ntohs(set_control_rsp->descriptor_index), NULL);

		if (!ctrl_desc) {
			os_log(LOG_ERR, "aecp(%p) Control descriptor (type = %d, index = %d) not found.\n",
					aecp, ntohs(set_control_rsp->descriptor_type), ntohs(set_control_rsp->descriptor_index));
			goto error;
		}

		if ((desc_values_len = aecp_aem_control_desc_to_pdu(ctrl_desc, values_rsp, AVDECC_AECP_MAX_SIZE)) < 0) {
			os_log(LOG_ERR, "aecp(%p) Cannot copy control descriptor values to pdu\n", aecp);
			goto error;
		}

		*len = desc_values_len + sizeof(struct aecp_aem_pdu) + sizeof(struct aecp_aem_set_get_control_pdu);

		rc = aecp_aem_net_tx_response(aecp, port, desc, status, mac_dst, *len);

		break;
	}
	case AECP_AEM_CMD_START_STREAMING:
	case AECP_AEM_CMD_STOP_STREAMING:
	case AECP_AEM_CMD_REMOVE_AUDIO_MAPPINGS:
	case AECP_AEM_CMD_ADD_AUDIO_MAPPINGS:
	case AECP_AEM_CMD_GET_AUDIO_MAP:
	default:
		rc = aecp_aem_net_tx_response(aecp, port, desc, status, mac_dst, *len);
		break;
	}

	return rc;
error:
	net_tx_free(desc);
	return -1;
}

static int aecp_aem_inflight_application_timeout(struct inflight_ctx *entry)
{
	struct aecp_ctx *aecp = container_of(entry->list_head, struct aecp_ctx, inflight_application);
	struct entity *entity = container_of(aecp, struct entity, aecp);
	struct avdecc_port *port = &entity->avdecc->port[entry->data.port_id];
	struct net_tx_desc *desc;
	struct aecp_aem_pdu *pdu;
	int rc = 0;

	os_log(LOG_DEBUG, "aecp(%p) inflight_ctx(%p) sequence id : %x\n", aecp, entry, entry->data.sequence_id);

	desc = aecp_net_tx_prepare(port, entry->data.pdu.buf, &entry->data.len, (void **)&pdu);

	if (!desc) {
		os_log(LOG_ERR, "aecp(%p) Cannot prepare tx descriptor\n", aecp);
		rc = AVDECC_INFLIGHT_TIMER_STOP;
	} else {
		/* Give Application some time (few seconds) before sending a failure status response */
		if (entry->data.retried >= AECP_CFG_MAX_AEM_IN_PROGRESS) {
			u16 len = entry->data.len;

			rc = aecp_aem_net_tx_inflight_response_failure(aecp, pdu, port, desc, entry->data.mac_dst, AECP_AEM_ENTITY_MISBEHAVING, &len);
			if (rc < 0)
				os_log(LOG_ERR, "aecp(%p) Cannot send failure AEM_RESPONSE\n", aecp);

			rc = AVDECC_INFLIGHT_TIMER_STOP;
		} else { /* Send an IN_PROGRESS response */
			rc = aecp_aem_net_tx_response(aecp, port, desc, AECP_AEM_IN_PROGRESS, entry->data.mac_dst, entry->data.len);
			if (rc < 0)
				rc = AVDECC_INFLIGHT_TIMER_STOP;
			else
				rc = AVDECC_INFLIGHT_TIMER_RESTART;

			entry->data.retried++;
		}
	}

	return rc;
}

/** Sends an AECP AEM command on the network.
 *
 * Sends an AECP AEM command to the specified MAC address, and create an inflight entry to monitor/handle the response.
 * Based on IEEE 1722.1-2013 section 9.2.2.3.2.
 *
 * Takes full ownership of the TX descriptor
 * \return 			0 on success or negative value otherwise.
 * \param aecp		AECP context the command is being sent from.
 * \param port		Pointer to the avdecc port on which the packet will be sent
 * \param pdu		Pointer to the start of the AECP AEM PDU within the network TX descriptor.
 * \param desc		Network TX descriptor.
 * \param mac_dst	MAC address to send the command to.
 *  \param len		Length of the AECP AEM PDU (after the AVTP header).
 *  \param ipc		IPC to forward any potential responses to. Will be stored in the inflight entry to be used on response reception.
 */
static int aecp_aem_send_command(struct aecp_ctx *aecp, struct avdecc_port *port, struct aecp_aem_pdu *pdu,
					struct net_tx_desc *desc, u8 *mac_dst, u16 len, struct ipc_tx *ipc, unsigned int ipc_dst)
{
	struct entity *entity = container_of(aecp, struct entity, aecp);
	struct inflight_ctx *entry;
	int rc = -1;

	os_log(LOG_DEBUG, "aecp(%p) pdu(%p) desc(%p) len(%u) ipc_tx(%p) seq_id(%d)\n", aecp, pdu, desc, len, ipc, aecp->sequence_id);

	entry = avdecc_inflight_get(entity);

	pdu->sequence_id = htons(aecp->sequence_id);
	if (entry) {
		entry->cb = aecp_aem_inflight_network_timeout;
		entry->data.msg_type = AECP_AEM_COMMAND;
		entry->data.retried = 0;
		entry->data.sequence_id = aecp->sequence_id;
		if (len > AVDECC_AECP_MAX_SIZE) {
			os_log(LOG_ERR, "aecp(%p) Truncating PDU to fit inside inflight buffer, length above spec (%u > %zu).\n", aecp, len, AVDECC_AECP_MAX_SIZE);
			len = AVDECC_AECP_MAX_SIZE;
		}
		entry->data.len = len;
		os_memcpy(entry->data.pdu.buf, pdu, len);
		entry->data.priv[0] = (uintptr_t)ipc;
		entry->data.priv[1] = (uintptr_t)ipc_dst;
		os_memcpy(entry->data.mac_dst, mac_dst, 6);
		entry->data.port_id = port->port_id;

		if(avdecc_inflight_start(&aecp->inflight_network, entry, AECP_COMMANDS_TIMEOUT) < 0)
			os_log(LOG_ERR, "aecp(%p) Could not start inflight\n", aecp);
		else
			rc = aecp_aem_net_tx_command(aecp, port, desc, mac_dst, len);
	}
	else {
		os_log(LOG_ERR, "aecp(%p) Could not allocate inflight\n", aecp);
	}
	aecp->sequence_id++;

	return rc;
}

/** Sends an AECP AEM response on the network.
 *
 * Sends an AECP AEM response to the specified MAC address on the set port and with the passed sequence id.
 *
 * Takes full ownership of the TX descriptor
 * \return 		0 on success or negative value otherwise.
 * \param aecp		AECP context the command is being sent from.
 * \param port		Pointer to the avdecc port on which the packet will be sent
 * \param pdu		Pointer to the start of the AECP AEM PDU within the network TX descriptor.
 * \param desc		Network TX descriptor.
 * \param controller_id	Controller entity ID which sent the command
 * \param status	Response status
 * \param unsolicited	1 if the response is an unsolicited notification, 0 otherwise
 * \param mac_dst	MAC address to send the command to.
 * \param len		Length of the AECP AEM PDU (after the AVTP header).
 * \param sequence_id	Sequence ID of the response PDU
 */
static int aecp_aem_send_response(struct aecp_ctx *aecp, struct avdecc_port *port, struct aecp_aem_pdu *pdu, struct net_tx_desc *desc, u64 controller_id,
					u16 sequence_id, u8 status, u8 unsolicited, u8 *mac_dst, u16 len)
{
	struct entity *entity = container_of(aecp, struct entity, aecp);

	os_log(LOG_DEBUG, "aecp(%p) pdu(%p) desc(%p) len(%u) seq_id(%d)\n",
		aecp, pdu, desc, len, sequence_id);

	copy_64(&pdu->controller_entity_id, &controller_id);
	copy_64(&pdu->entity_id, &entity->desc->entity_id);
	pdu->sequence_id = htons(sequence_id);

	AECP_AEM_SET_U_CMD_TYPE(pdu, unsolicited, AECP_AEM_GET_CMD_TYPE(pdu));

	return aecp_aem_net_tx_response(aecp, port, desc, status, mac_dst, len);
}

/** Prepares an AECP AEM response from a previously allocated AECP PDU and and send it over the network.
 *
 * Allocates a TX descriptor and copies the content of the specified buffer pointing to the AECP PDU.
 * Sends an AECP AEM response to the specified MAC address on the set port and with the passed sequence id.
 *
 * Does not take ownership of the specified buffer pointing to the AECP PDU.
 * \return 		0 on success or negative value otherwise.
 * \param aecp		AECP context the command is being sent from.
 * \param port		Pointer to the avdecc port on which the packet will be sent
 * \param buf		Pointer to the start of the AECP AEM PDU to be copied
 * \param controller_id	Controller entity ID which sent the command
 * \param status	Response status
 * \param unsolicited	1 if the response is an unsolicited notification, 0 otherwise
 * \param mac_dst	MAC address to send the command to.
 * \param len		Length of the AECP AEM PDU (after the AVTP header).
 * \param sequence_id	Sequence ID of the response PDU
 */
static int aecp_aem_prepare_send_response(struct aecp_ctx *aecp, struct avdecc_port *port, void *buf, u64 controller_id,
					u16 sequence_id, u8 status, u8 unsolicited, u8 *mac_dst, u16 len)
{
	struct net_tx_desc *tx_desc;
	struct aecp_aem_pdu *aecp_cmd;

	os_log(LOG_DEBUG, "aecp(%p) pdu(%p) len(%u)\n", aecp, buf, len);

	tx_desc = aecp_net_tx_prepare(port, buf, &len, (void **)&aecp_cmd);
	if (!tx_desc) {
		os_log(LOG_ERR, "aecp(%p) Cannot alloc tx descriptor\n", aecp);
		return -1;
	}

	return aecp_aem_send_response(aecp, port, aecp_cmd, tx_desc, controller_id, sequence_id, status, unsolicited, mac_dst, len);
}

/** Sends an AECP AEM synchronous unsolicited notification on the network.
 *
 * Sends an AECP AEM synchronous (result to a successful response to a command from a controller) unsolicited notification to
 * the registered controllers (except the one which sent the command)
 *
 * Does not take ownership of the specified buffer pointing to the AECP PDU.
 * \return 		0 on success or negative value otherwise.
 * \param aecp		Pointer to the aecp context struct
 * \param aecp_rsp	Pointer to the AECP AEM PDU containing the successful response.
 * \param controller_id	Controller entity ID which sent the command (to be excluded from the notification if registered)
 * \param len		Length of the AECP AEM PDU (after the AVTP header).
 */
static int aecp_aem_send_sync_unsolicited_notification(struct aecp_ctx *aecp, struct aecp_aem_pdu *aecp_rsp, u64 controller_id, u16 len)
{
	struct entity *entity = container_of(aecp, struct entity, aecp);
	struct avdecc_ctx *avdecc = entity->avdecc;
	struct unsolicited_ctx *unsolicited_entry;
	struct list_head *list_entry;

	list_entry = list_first(&aecp->unsolicited);
	while (list_entry != &aecp->unsolicited) {
		struct avdecc_port *port;

		unsolicited_entry = container_of(list_entry, struct unsolicited_ctx, list);
		port = &avdecc->port[unsolicited_entry->port_id];

		/* Do not send synchronous unsolicited notification to the controller sending the command */
		if (controller_id != unsolicited_entry->controller_id) {

			if (aecp_aem_prepare_send_response(aecp, port, aecp_rsp, unsolicited_entry->controller_id, unsolicited_entry->sequence_id, AECP_AEM_SUCCESS, 1,
								unsolicited_entry->mac_dst, len) < 0) {
				os_log(LOG_ERR, "avdecc(%p) port(%u) couldn't send notification to registered controller (%016"PRIx64")\n",
					avdecc, unsolicited_entry->port_id, ntohll(unsolicited_entry->controller_id));
				goto err;
			}

			unsolicited_entry->sequence_id++;
		}

		list_entry = list_next(list_entry);
	}

	return 0;

err:
	return -1;
}

static int aecp_application_inflight_add(struct aecp_ctx *aecp, struct aecp_aem_pdu *pdu, u16 data_len, u8 *mac_src, unsigned int port_id)
{
	struct inflight_ctx *entry;
	int status = AECP_AEM_SUCCESS;
	struct entity *entity = container_of(aecp, struct entity, aecp);

	entry = avdecc_inflight_get(entity);
	if (!entry) {
		os_log(LOG_ERR, "aecp(%p) Could not allocate inflight\n", aecp);
		status = AECP_AEM_NO_RESOURCES;
		goto exit;
	}

	entry->cb = aecp_aem_inflight_application_timeout;
	entry->data.msg_type = AECP_AEM_COMMAND;
	entry->data.retried = 0;
	entry->data.sequence_id = ntohs(pdu->sequence_id);
	/* IN_PROGRESS responses will be sent with the original PDU, so the length comes from there. */
	entry->data.len = data_len;
	if (entry->data.len > AVDECC_AECP_MAX_SIZE) {
		os_log(LOG_ERR, "aecp(%p) Truncating PDU to fit inside inflight buffer, length above spec (%u > %zu).\n", aecp, entry->data.len, AVDECC_AECP_MAX_SIZE);
		entry->data.len = AVDECC_AECP_MAX_SIZE;
	}

	os_memcpy(entry->data.pdu.buf, pdu, entry->data.len);
	entry->data.priv[0] = 0;
	entry->data.priv[1] = 0;
	/* response would be sent back on the same port we received the command from */
	entry->data.port_id = port_id;
	os_memcpy(entry->data.mac_dst, mac_src, 6);

	if (avdecc_inflight_start(&aecp->inflight_application, entry, AECP_IN_PROGRESS_TIMEOUT) < 0) {
		os_log(LOG_ERR, "aecp(%p) Could not start inflight\n", aecp);
		status = AECP_AEM_ENTITY_MISBEHAVING;
		goto exit;
	}

exit:
	return status;
}

/** Main AECP vendor unique command receive function
 * Follows the AVDECC entity model state machine (9.2.2.3.1.4).
 * \return 	0 on success, negative otherwise
 * \param	aecp		pointer to the AECP context
 * \param	pdu		pointer to the AECP Vendor Unique Format PDU
 * \param	avtp_len	length of the AVTP payload.
 * \param	mac_src		source MAC address of the received PDU
 * \param	port_id		avdecc port / interface index on which we received the PDU
 */
static int aecp_vendor_specific_received_command(struct aecp_ctx *aecp, struct aecp_vuf_pdu *pdu, u16 avtp_len, u8 *mac_src, unsigned int port_id)
{
	struct entity *entity = container_of(aecp, struct entity, aecp);
	struct avdecc_port *port_rsp = &entity->avdecc->port[port_id];
	struct aecp_mvu_pdu *mvu_rsp_pdu = NULL;
	struct aecp_mvu_pdu *mvu_cmd_pdu = NULL;
	struct net_tx_desc *desc_rsp;
	u8 status;
	u16 cmd_type;
	u16 len;
	int rc = 0;

	if (!entity->milan_mode || os_memcmp(pdu->protocol_id, aecp_mvu_protocol_id, 6)) {
		os_log(LOG_ERR, "aecp(%p) Unsupported Vendor Specific protocol_id %02X-%02X-%02X-%02X-%02X-%02X \n",
			aecp, pdu->protocol_id[0], pdu->protocol_id[1], pdu->protocol_id[2], pdu->protocol_id[3], pdu->protocol_id[4], pdu->protocol_id[5]);
		rc = -1;
		goto exit;
	}

	len = sizeof(struct aecp_mvu_pdu); //data size after AVTP control hdr
	mvu_cmd_pdu = (struct aecp_mvu_pdu *)pdu;

	desc_rsp = aecp_net_tx_prepare(port_rsp, pdu, &len, (void **)&mvu_rsp_pdu); //FIXME check if we can re-use same buf
	if (!desc_rsp) {
		os_log(LOG_ERR, "aecp(%p) Cannot alloc tx descriptor\n", aecp);
		rc = -1;
		goto exit;
	}

	cmd_type = AECP_MVU_GET_CMD_TYPE(mvu_cmd_pdu);

	os_log(LOG_DEBUG, "aecp(%p) command (%x, %s) seq_id(%d)\n", aecp, cmd_type, aecp_mvu_cmdtype2string(cmd_type), ntohs(pdu->sequence_id));

	switch (cmd_type) {
	case AECP_MVU_CMD_GET_MILAN_INFO:
	{
		struct aecp_mvu_get_milan_info_rsp_pdu *get_milan_rsp = (struct aecp_mvu_get_milan_info_rsp_pdu *)(mvu_rsp_pdu + 1);

		get_milan_rsp->protocol_version = htonl(MILAN_PROTOCOL_VERSION);
		get_milan_rsp->features_flags = htonl(0);
		get_milan_rsp->certification_version = htonl(MILAN_CERTIFICATION_VERSION(1, 1, 0, 0));

		status = AECP_AEM_SUCCESS;
		len += sizeof(struct aecp_mvu_get_milan_info_rsp_pdu);

		break;
	}
	default:
		status = AECP_AEM_NOT_IMPLEMENTED;
		break;

	}

	rc = aecp_mvu_net_tx_response(aecp, port_rsp, desc_rsp, status, mac_src, len);

exit:
	return rc;
}

/** Build a GET_STREAM_INFO response
 * \return status, AECP_AEM status
 * \param entity, pointer to the entity
 * \param aecp_rsp, pointer to the start of the response pdu
 * \param len, pointer to the len of the pdu
 * \param descriptor_type, type of the aem_desc
 * \param descriptor_index, id of the aem_desc
 */
static u8 aecp_aem_get_stream_info_response(struct entity *entity, struct aecp_aem_pdu *aecp_rsp, u16 *len, u16 descriptor_type, u16 descriptor_index)
{
	struct aecp_aem_get_stream_info_rsp_pdu *get_stream_info_rsp = (struct aecp_aem_get_stream_info_rsp_pdu *)(aecp_rsp + 1);
	struct aecp_aem_milan_get_stream_info_rsp_pdu *get_stream_info_milan_rsp = (struct aecp_aem_milan_get_stream_info_rsp_pdu *)(aecp_rsp + 1);
	struct stream_descriptor *desc;
	void *dynamic_desc;
	u8 status = AECP_AEM_SUCCESS;

	if (!entity->milan_mode) {
		*len += sizeof(struct aecp_aem_get_stream_info_rsp_pdu);
		os_memset(get_stream_info_rsp, 0, sizeof(struct aecp_aem_get_stream_info_rsp_pdu));
	} else {
		*len += sizeof(struct aecp_aem_milan_get_stream_info_rsp_pdu);
		os_memset(get_stream_info_milan_rsp, 0, sizeof(struct aecp_aem_milan_get_stream_info_rsp_pdu));
	}

	get_stream_info_rsp->descriptor_type = htons(descriptor_type);
	get_stream_info_rsp->descriptor_index = htons(descriptor_index);

	if (descriptor_type != AEM_DESC_TYPE_STREAM_INPUT && descriptor_type != AEM_DESC_TYPE_STREAM_OUTPUT) {
		status = AECP_AEM_BAD_ARGUMENTS;
		goto exit;
	}

	desc = aem_get_descriptor(entity->aem_descs, descriptor_type, descriptor_index, NULL);
	if (!desc) {
		status = AECP_AEM_NO_SUCH_DESCRIPTOR;
		goto exit;
	}

	dynamic_desc = aem_get_descriptor(entity->aem_dynamic_descs, descriptor_type, descriptor_index, NULL);
	if (!dynamic_desc) {
		status = AECP_AEM_ENTITY_MISBEHAVING;
		goto exit;
	}

	if (!entity->milan_mode) {
		status = AECP_AEM_NOT_SUPPORTED; /* FIXME add support for GET_STREAM_INFO for IEEE mode */
		goto exit;
	}

	copy_64(&get_stream_info_rsp->stream_format, &desc->current_format);
	get_stream_info_rsp->flags |= htonl(AECP_STREAM_FLAG_STREAM_FORMAT_VALID);

	/* STREAM_INPUT */
	if (descriptor_type == AEM_DESC_TYPE_STREAM_INPUT) {
		struct stream_input_dynamic_desc *in_dynamic_desc = (struct stream_input_dynamic_desc *)dynamic_desc;

		/* Per AVNU.IO.CONTROL 7.3.10.1  (MSRP_ACC_LAT_VALID) (REGISTERING) */
		if (in_dynamic_desc->u.milan.srp_stream_status != NO_TALKER) {
			get_stream_info_milan_rsp->msrp_accumulated_latency = htonl(in_dynamic_desc->u.milan.msrp_accumulated_latency);
			get_stream_info_milan_rsp->flags |= htonl(AECP_STREAM_FLAG_MSRP_ACC_LAT_VALID);

			get_stream_info_milan_rsp->flags_ex |= htonl(AECP_STREAM_FLAG_EX_REGISTERING);
		}

		if (ACMP_MILAN_IS_LISTENER_SINK_BOUND(in_dynamic_desc)) {
			get_stream_info_milan_rsp->flags |= htonl(AECP_STREAM_FLAG_FAST_CONNECT);
			get_stream_info_milan_rsp->flags |= htonl(AECP_STREAM_FLAG_SAVED_STATE);
			get_stream_info_milan_rsp->flags |= htonl(AECP_STREAM_FLAG_BOUND);

			if ((in_dynamic_desc->flags & htons(ACMP_FLAG_STREAMING_WAIT)) != 0) {
				/* Bound and stopped */
				get_stream_info_milan_rsp->flags |= htonl(AECP_STREAM_FLAG_STREAMING_WAIT);
			}
		}

		if (ACMP_MILAN_IS_LISTENER_SINK_SETTLED(in_dynamic_desc)) {
			/* Shall be the values from the ACMP_PROBE_TX_RESPONSE from the talker */
			copy_64(&get_stream_info_milan_rsp->stream_id, &in_dynamic_desc->stream_id);
			get_stream_info_milan_rsp->flags |= htonl(AECP_STREAM_FLAG_STREAM_ID_VALID);

			get_stream_info_milan_rsp->stream_vlan_id = in_dynamic_desc->stream_vlan_id;
			get_stream_info_milan_rsp->flags |= htonl(AECP_STREAM_FLAG_STREAM_VLAN_ID_VALID);

			os_memcpy(get_stream_info_milan_rsp->stream_dest_mac, in_dynamic_desc->stream_dest_mac, 6);
			get_stream_info_milan_rsp->flags |= htonl(AECP_STREAM_FLAG_STREAM_DEST_MAC_VALID);
		}

		/* Per AVNU.IO.CONTROL 7.3.10.1  (REGISTERING_FAILED and MSRP_FAILURE_VALID) */
		if (in_dynamic_desc->u.milan.srp_stream_status == FAILED) {
			get_stream_info_milan_rsp->flags |= htonl(AECP_STREAM_FLAG_REGISTERING_FAILED);

			get_stream_info_milan_rsp->msrp_failure_code = in_dynamic_desc->u.milan.failure.failure_code;
			copy_64(&get_stream_info_milan_rsp->msrp_failure_bridge_id, in_dynamic_desc->u.milan.failure.bridge_id);
			get_stream_info_milan_rsp->flags |= htonl(AECP_STREAM_FLAG_MSRP_FAILURE_VALID);
		}

		get_stream_info_milan_rsp->pbsta = in_dynamic_desc->u.milan.probing_status;
		get_stream_info_milan_rsp->acmpsta = in_dynamic_desc->u.milan.acmp_status;

	/* STREAM_OUTPUT */
	} else if (descriptor_type == AEM_DESC_TYPE_STREAM_OUTPUT) {
		struct stream_output_dynamic_desc *out_dynamic_desc = (struct stream_output_dynamic_desc *)dynamic_desc;

		/* Per AVNU.IO.CONTROL 7.3.10.2  (REGISTERING) */
		if (out_dynamic_desc->u.milan.srp_listener_status != NO_LISTENER) {
			get_stream_info_milan_rsp->flags_ex |= htonl(AECP_STREAM_FLAG_EX_REGISTERING);
		}

		/* Per AVNU.IO.CONTROL 7.3.10.2  (REGISTERING_FAILED) */
		if (out_dynamic_desc->u.milan.srp_listener_status == FAILED_LISTENER) {
			get_stream_info_milan_rsp->flags |= htonl(AECP_STREAM_FLAG_REGISTERING_FAILED);
		}

		copy_64(&get_stream_info_rsp->stream_id, &out_dynamic_desc->stream_id);
		get_stream_info_milan_rsp->flags |= htonl(AECP_STREAM_FLAG_STREAM_ID_VALID);

		get_stream_info_milan_rsp->stream_vlan_id = out_dynamic_desc->stream_vlan_id;
		get_stream_info_milan_rsp->flags |= htonl(AECP_STREAM_FLAG_STREAM_VLAN_ID_VALID);

		if (!is_invalid_mac_addr(out_dynamic_desc->stream_dest_mac)) {
			os_memcpy(get_stream_info_milan_rsp->stream_dest_mac, out_dynamic_desc->stream_dest_mac, 6);
			get_stream_info_milan_rsp->flags |= htonl(AECP_STREAM_FLAG_STREAM_DEST_MAC_VALID);
		}

		if (out_dynamic_desc->u.milan.srp_talker_declaration_type == TALKER_FAILED) {
			get_stream_info_milan_rsp->msrp_failure_code = out_dynamic_desc->u.milan.failure.failure_code;
			copy_64(&get_stream_info_milan_rsp->msrp_failure_bridge_id, out_dynamic_desc->u.milan.failure.bridge_id);
			get_stream_info_milan_rsp->flags |= htonl(AECP_STREAM_FLAG_MSRP_FAILURE_VALID);
		}

		get_stream_info_milan_rsp->msrp_accumulated_latency = htonl(out_dynamic_desc->presentation_time_offset);
		get_stream_info_milan_rsp->flags |= htonl(AECP_STREAM_FLAG_MSRP_ACC_LAT_VALID);
	}

exit:
	return status;
}

static u8 aecp_aem_get_counters_response(struct entity *entity, struct aecp_aem_pdu *aecp_rsp, u16 *len, u16 descriptor_type, u16 descriptor_index)
{
	struct aecp_aem_get_counters_rsp_pdu *get_counters_rsp = (struct aecp_aem_get_counters_rsp_pdu *)(aecp_rsp + 1);
	void *desc;
	u32 counters_valid;
	u8 status = AECP_AEM_SUCCESS;

	*len += sizeof(struct aecp_aem_get_counters_rsp_pdu);
	os_memset(get_counters_rsp, 0, sizeof(*get_counters_rsp));

	get_counters_rsp->descriptor_type = htons(descriptor_type);
	get_counters_rsp->descriptor_index = htons(descriptor_index);

	if (IS_VALID_GET_COUNTERS_DESCRIPTOR_TYPE(descriptor_type) || (entity->milan_mode && descriptor_type == AEM_DESC_TYPE_STREAM_OUTPUT)) {
		desc = aem_get_descriptor(entity->aem_descs, descriptor_type, descriptor_index, NULL);
		if (!desc) {
			status = AECP_AEM_NO_SUCH_DESCRIPTOR;
			goto exit;
		}
	} else {
		status = AECP_AEM_BAD_ARGUMENTS;
		goto exit;
	}

	/*
	 * FIXME
	 * Complete missing counters, set to 0 for now
	 */
	switch (descriptor_type) {
	case AEM_DESC_TYPE_AVB_INTERFACE:
	{
		struct avb_interface_dynamic_desc *avb_itf_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, descriptor_type, descriptor_index, NULL);
		if (!avb_itf_dynamic) {
			status = AECP_AEM_ENTITY_MISBEHAVING;
			break;
		}

		counters_valid = ((1 << AECP_AEM_COUNTER_AVB_INTERFACE_LINK_UP)
					| (1 << AECP_AEM_COUNTER_AVB_INTERFACE_LINK_DOWN)
					| (1 << AECP_AEM_COUNTER_AVB_INTERFACE_GPTP_GM_CHANGED));

		get_counters_rsp->counters_valid = htonl(counters_valid);
		get_counters_rsp->counters_block[AECP_AEM_COUNTER_AVB_INTERFACE_LINK_UP] = htonl(avb_itf_dynamic->link_up);
		get_counters_rsp->counters_block[AECP_AEM_COUNTER_AVB_INTERFACE_LINK_DOWN] = htonl(avb_itf_dynamic->link_down);
		get_counters_rsp->counters_block[AECP_AEM_COUNTER_AVB_INTERFACE_GPTP_GM_CHANGED] = htonl(avb_itf_dynamic->gptp_gm_changed);
	}
	break;

	case AEM_DESC_TYPE_CLOCK_DOMAIN:
		counters_valid = (1 << AECP_AEM_COUNTER_CLOCK_DOMAIN_LOCKED) | (1 << AECP_AEM_COUNTER_CLOCK_DOMAIN_UNLOCKED);

		get_counters_rsp->counters_valid = htonl(counters_valid);
		get_counters_rsp->counters_block[AECP_AEM_COUNTER_CLOCK_DOMAIN_LOCKED] = htonl(0);
		get_counters_rsp->counters_block[AECP_AEM_COUNTER_CLOCK_DOMAIN_UNLOCKED] = htonl(0);
	break;

	case AEM_DESC_TYPE_STREAM_INPUT:
		counters_valid = ((1 << AECP_AEM_COUNTER_STREAM_INPUT_MEDIA_LOCKED)
								| (1 << AECP_AEM_COUNTER_STREAM_INPUT_MEDIA_UNLOCKED)
								| (1 << AECP_AEM_COUNTER_STREAM_INPUT_STREAM_RESET)
								| (1 << AECP_AEM_COUNTER_STREAM_INPUT_SEQ_NUM_MISMATCH)
								| (1 << AECP_AEM_COUNTER_STREAM_INPUT_MEDIA_RESET)
								| (1 << AECP_AEM_COUNTER_STREAM_INPUT_TIMESTAMP_UNCERTAIN)
								| (1 << AECP_AEM_COUNTER_STREAM_INPUT_UNSUPPORTED_FORMAT)
								| (1 << AECP_AEM_COUNTER_STREAM_INPUT_LATE_TIMESTAMP)
								| (1 << AECP_AEM_COUNTER_STREAM_INPUT_EARLY_TIMESTAMP)
								| (1 << AECP_AEM_COUNTER_STREAM_INPUT_FRAMES_RX));

		get_counters_rsp->counters_valid = htonl(counters_valid);

		get_counters_rsp->counters_block[AECP_AEM_COUNTER_STREAM_INPUT_MEDIA_LOCKED] = htonl(0);
		get_counters_rsp->counters_block[AECP_AEM_COUNTER_STREAM_INPUT_MEDIA_UNLOCKED] = htonl(0);
		get_counters_rsp->counters_block[AECP_AEM_COUNTER_STREAM_INPUT_STREAM_RESET] = htonl(0);
		get_counters_rsp->counters_block[AECP_AEM_COUNTER_STREAM_INPUT_SEQ_NUM_MISMATCH] = htonl(0);
		get_counters_rsp->counters_block[AECP_AEM_COUNTER_STREAM_INPUT_MEDIA_RESET] = htonl(0);
		get_counters_rsp->counters_block[AECP_AEM_COUNTER_STREAM_INPUT_TIMESTAMP_UNCERTAIN] = htonl(0);
		get_counters_rsp->counters_block[AECP_AEM_COUNTER_STREAM_INPUT_UNSUPPORTED_FORMAT] = htonl(0);
		get_counters_rsp->counters_block[AECP_AEM_COUNTER_STREAM_INPUT_LATE_TIMESTAMP] = htonl(0);
		get_counters_rsp->counters_block[AECP_AEM_COUNTER_STREAM_INPUT_EARLY_TIMESTAMP] = htonl(0);
		get_counters_rsp->counters_block[AECP_AEM_COUNTER_STREAM_INPUT_FRAMES_RX] = htonl(0);
	break;

	case AEM_DESC_TYPE_STREAM_OUTPUT:
		counters_valid = ((1 << AECP_AEM_COUNTER_STREAM_OUTPUT_STREAM_START)
								| (1 << AECP_AEM_COUNTER_STREAM_OUTPUT_STREAM_STOP)
								| (1 << AECP_AEM_COUNTER_STREAM_OUTPUT_MEDIA_RESET)
								| (1 << AECP_AEM_COUNTER_STREAM_OUTPUT_TIMESTAMP_UNCERTAIN)
								| (1 << AECP_AEM_COUNTER_STREAM_OUTPUT_FRAMES_TX));

		get_counters_rsp->counters_valid = htonl(counters_valid);

		get_counters_rsp->counters_block[AECP_AEM_COUNTER_STREAM_OUTPUT_STREAM_START] = htonl(0);
		get_counters_rsp->counters_block[AECP_AEM_COUNTER_STREAM_OUTPUT_STREAM_STOP] = htonl(0);
		get_counters_rsp->counters_block[AECP_AEM_COUNTER_STREAM_OUTPUT_MEDIA_RESET] = htonl(0);
		get_counters_rsp->counters_block[AECP_AEM_COUNTER_STREAM_OUTPUT_TIMESTAMP_UNCERTAIN] = htonl(0);
		get_counters_rsp->counters_block[AECP_AEM_COUNTER_STREAM_OUTPUT_FRAMES_TX] = htonl(0);
	break;
	}

exit:
	return status;
}

static u8 aecp_aem_get_as_path_response(struct entity *entity, struct aecp_aem_pdu *aecp_rsp, u16 *len, u16 descriptor_index)
{
	struct aecp_aem_get_as_path_rsp_pdu *get_as_path_rsp = (struct aecp_aem_get_as_path_rsp_pdu *)(aecp_rsp + 1);
	struct ptp_clock_identity *path_sequence_rsp = (struct ptp_clock_identity *)(get_as_path_rsp + 1);
	struct avb_interface_dynamic_desc *avb_itf_dynamic;
	u8 status = AECP_AEM_SUCCESS;

	*len += sizeof(struct aecp_aem_get_as_path_rsp_pdu);
	os_memset(get_as_path_rsp, 0, sizeof(struct aecp_aem_get_as_path_rsp_pdu));

	get_as_path_rsp->descriptor_index = htons(descriptor_index);

	avb_itf_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_AVB_INTERFACE, descriptor_index, NULL);
	if (!avb_itf_dynamic) {
		status = AECP_AEM_ENTITY_MISBEHAVING;
		goto exit;
	}

	*len += (avb_itf_dynamic->num_ptlv_entries * sizeof(struct ptp_clock_identity));
	get_as_path_rsp->count = htons(avb_itf_dynamic->num_ptlv_entries);

	os_memcpy(path_sequence_rsp, &avb_itf_dynamic->path_sequence , avb_itf_dynamic->num_ptlv_entries * sizeof(struct ptp_clock_identity));

exit:
	return status;
}

/** Check if there is a registered controller with different controller id than the one requesting the command
 *
 * \return 		true if there is at least another controller than the one requesting the command registered, false otherwise.
 * \param aecp		AECP context the command is being sent from.
 * \param requesting_controller_id		ID of the controller requesting the command
 */
static bool aecp_need_sync_unsolicited_notifications(struct aecp_ctx *aecp, u64 requesting_controller_id)
{
	struct list_head *list_entry;
	struct unsolicited_ctx *unsolicited_entry;

	list_entry = list_first(&aecp->unsolicited);

	while (list_entry != &aecp->unsolicited) {
		unsolicited_entry = container_of(list_entry, struct unsolicited_ctx, list);

		if (requesting_controller_id != unsolicited_entry->controller_id)
			return true;

		list_entry = list_next(list_entry);
	}

	return false;
}

/** Send an unsolicited notification to registered controllers
 *
 * Takes ownership of the specified buffer pointing to the AECP PDU.
 *
 * \return 		0 on success or negative value otherwise.
 * \param aecp		AECP context the command is being sent from.
 * \param desc		Pointer to the net descriptor
 * \param aecp_pdu	Pointer to the AECP PDU buffer
 * \param excluded_controller_id	Controller entity ID which sent the command
 * \param len		Length of the AECP AEM PDU (after the AVTP header).
 */
static int aecp_aem_send_unsolicited_notification(struct aecp_ctx *aecp, struct net_tx_desc *desc, struct aecp_aem_pdu *aecp_pdu , u64 excluded_controller_id, u16 len)
{
	struct entity *entity = container_of(aecp, struct entity, aecp);
	struct list_head *list_entry;
	struct unsolicited_ctx *unsolicited_entry;
	u16 notification_type = AECP_AEM_GET_CMD_TYPE(aecp_pdu);

	/* If no registered controller, exit (and free descriptor) early */
	if (list_empty(&aecp->unsolicited))
		goto free_and_exit;

	/* Get the first controller in the list. We have at least one in the list */
	list_entry = list_first(&aecp->unsolicited);

	do {
		struct avdecc_port *port_rsp;

		unsolicited_entry = container_of(list_entry, struct unsolicited_ctx, list);

		port_rsp = &entity->avdecc->port[unsolicited_entry->port_id];

		list_entry = list_next(list_entry);

		/* Don't send the notification to the controller generating it in case of a sync unsolicited notification */
		if (unsolicited_entry->controller_id == excluded_controller_id)
			continue;

		/* Always clone the original descriptor using the right tx port then send it. */
		if (aecp_aem_prepare_send_response(aecp, port_rsp, aecp_pdu, unsolicited_entry->controller_id,
							unsolicited_entry->sequence_id, AECP_AEM_SUCCESS, 1, unsolicited_entry->mac_dst, len) < 0) {

			os_log(LOG_ERR,"aecp(%p) port(%u) couldn't prepare and send unsolicited notification (%x, %s) to controller(%016"PRIx64").\n",
					aecp, unsolicited_entry->port_id, notification_type, aecp_aem_cmdtype2string(notification_type), ntohll(unsolicited_entry->controller_id));

			goto err_prepare_rsp;
		}

		/* Incrementing the sequence_id per AVNU.IO.CONTROL 7.5.1 */
		unsolicited_entry->sequence_id++;

		os_log(LOG_DEBUG,"aecp(%p) port(%u) sent unsolicited notification (%x, %s) to controller(%016"PRIx64").\n",
				aecp, unsolicited_entry->port_id, notification_type, aecp_aem_cmdtype2string(notification_type), ntohll(unsolicited_entry->controller_id));

	} while (list_entry != &aecp->unsolicited);

free_and_exit:
	/* Always free the original descriptor. */
	net_tx_free(desc);

	return 0;

err_prepare_rsp:
	net_tx_free(desc);

	return -1;
}

/** Send a synchronous unsolicited notification
 *
 * Takes ownership of the specified buffer pointing to the AECP PDU.
 *
 * \return 		0 on success or negative value otherwise.
 * \param aecp		AECP context the command is being sent from.
 * \param desc		Pointer to the net descriptor
 * \param aecp_pdu	Pointer to the AECP PDU buffer
 * \param controller_id	Controller entity ID which sent the command
 * \param len		Length of the AECP AEM PDU (after the AVTP header).
 */
static int aecp_aem_send_sync_unsolicited_notification_full(struct aecp_ctx *aecp, struct net_tx_desc *desc, struct aecp_aem_pdu *aecp_pdu, u64 controller_id, u16 len)
{
	return aecp_aem_send_unsolicited_notification(aecp, desc, aecp_pdu, controller_id, len);
}

/** Sends an AECP AEM asynchronous unsolicited notification on the network to the registered controllers.
 * Notifies changes in the state/dynamic descriptors of the entity.
 * Allocate and fill the AECP PDU buffer according to the notification type.
 *
 * \return int, 0 if successful -1 otherwise
 * \param aecp, pointer to the aecp context
 * \param notification_type, type of the notification (IEEE Std 1722.1-2013 7.5.2 for the list of available unsolicited notification types)
 * \param descriptor_type, type of the aem_desc
 * \param descriptor_index, id of the aem_desc
 */
int aecp_aem_send_async_unsolicited_notification(struct aecp_ctx *aecp, u16 notification_type, u16 descriptor_type, u16 descriptor_index)
{
	struct entity *entity = container_of(aecp, struct entity, aecp);
	struct unsolicited_ctx *unsolicited_entry;
	struct list_head *list_entry;
	struct net_tx_desc *desc = NULL;
	struct aecp_aem_pdu *aecp_pdu = NULL;
	u16 len	= sizeof(struct aecp_aem_pdu);
	struct avdecc_port *port_rsp;
	void *buf;
	u8 status;

	if (list_empty(&aecp->unsolicited))
		goto exit;

	/* Allocate a network descriptor  using the port response of the first controller
	 * in the list (there is at least one) to contruct the response buffer. aecp_aem_send_unsolicited_notification()
	 * will clone it using the right port for each controller anyway.
	 */
	list_entry = list_first(&aecp->unsolicited);
	unsolicited_entry = container_of(list_entry, struct unsolicited_ctx, list);
	port_rsp = &entity->avdecc->port[unsolicited_entry->port_id];

	desc = net_tx_alloc(&port_rsp->net_tx, DEFAULT_NET_DATA_SIZE);
	if (!desc) {
		os_log(LOG_ERR,"aecp(%p): Cannot alloc net_tx\n", aecp);
		goto err_tx_alloc;
	}

	buf = NET_DATA_START(desc);

	aecp_pdu = (struct aecp_aem_pdu *)((char *)buf + OFFSET_TO_AECP);

	/* Set command type of the response */
	AECP_AEM_SET_U_CMD_TYPE(aecp_pdu, 1, notification_type);

	switch (notification_type) {
		case AECP_AEM_CMD_GET_STREAM_INFO:
		{
			status = aecp_aem_get_stream_info_response(entity, aecp_pdu, &len, descriptor_type, descriptor_index);
			break;
		}
		case AECP_AEM_CMD_GET_COUNTERS:
		{
			status = aecp_aem_get_counters_response(entity, aecp_pdu, &len, descriptor_type, descriptor_index);
			break;
		}
		case AECP_AEM_CMD_GET_AS_PATH:
		{
			status = aecp_aem_get_as_path_response(entity, aecp_pdu, &len, descriptor_index);
			break;
		}
		case AECP_AEM_CMD_LOCK_ENTITY:
		{
			struct aecp_aem_lock_entity_pdu *lock_rsp = (struct aecp_aem_lock_entity_pdu *)(aecp_pdu + 1);
			struct entity_dynamic_desc *entity_dynamic;

			status = AECP_AEM_SUCCESS;

			len += sizeof(struct aecp_aem_lock_entity_pdu);
			os_memset(lock_rsp, 0, sizeof(struct aecp_aem_lock_entity_pdu));

			lock_rsp->descriptor_type = htons(descriptor_type);
			lock_rsp->descriptor_index = htons(descriptor_index);

			if (descriptor_type != AEM_DESC_TYPE_ENTITY) {
				status = AECP_AEM_BAD_ARGUMENTS;
				break;
			}

			entity_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, descriptor_type, descriptor_index, NULL);
			if (!entity_dynamic) {
				status = AECP_AEM_NO_SUCH_DESCRIPTOR;
				break;
			}

			if (entity_dynamic->lock_status == LOCKED)
				copy_64(&lock_rsp->locked_id, &entity_dynamic->locking_controller_id);
			else
				lock_rsp->flags |= htonl(AECP_AEM_LOCK_UNLOCK);

			break;
		}
		default:
			os_log(LOG_ERR,"aecp(%p): Unknown notification_type(%x) or not yet implemented for the unsolicited notification\n", aecp, notification_type);
			goto err;
	}

	if (status != AECP_AEM_SUCCESS) {
		os_log(LOG_ERR,"aecp(%p): Failed to get successful async unsolicited notification for command (%x, %s), status %u\n",
							aecp, notification_type, aecp_aem_cmdtype2string(notification_type), status);
		goto err;
	}

	if (aecp_aem_send_unsolicited_notification(aecp, desc, aecp_pdu, 0, len) < 0) {
		os_log(LOG_ERR,"aecp(%p): Couldn't send the async unsolicited notification (%x, %s)\n",
							aecp, notification_type, aecp_aem_cmdtype2string(notification_type));
		goto err_send_rsp;
	}

exit:
	return 0;

err:
	net_tx_free(desc);

err_send_rsp:
err_tx_alloc:
	return -1;
}

/** Start a timer to send an GET_COUNTERS async unsolicited notification after a short delay if the timer is not currently running
 * \return none
 * \paran entity
 * \param port_id
 */
void aecp_milan_register_get_counters_async_notification(struct entity *entity, unsigned int port_id)
{
	struct avb_interface_dynamic_desc *avb_itf_dynamic;

	if (!entity->milan_mode)
		goto exit;

	avb_itf_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_AVB_INTERFACE, port_id, NULL);

	if (!avb_itf_dynamic)
		goto exit;

	if (!timer_is_running(&avb_itf_dynamic->u.milan.async_get_counters_unsolicited_notification_timer))
		timer_start(&avb_itf_dynamic->u.milan.async_get_counters_unsolicited_notification_timer, AECP_MILAN_GET_COUNTERS_ASYNC_UNSOLICITED_NOTIFICATION_MS);

exit:
	return;
}

/** This function directly sends a GET_AS_PATH async unsolicited notification, if no previous notification has been sent in the past timer period (i.e timer not running).
 * Otherwise, it just registers a new notification to be sent at timer expiration.
 * That limits the GET_AS_PATH notifications' rate to one per configured timer period (currently one per second).
 * \return none
 * \paran entity
 * \param port_id
 */
void aecp_trigger_async_get_as_path_notification(struct entity *entity, unsigned int port_id)
{
	struct avb_interface_dynamic_desc *avb_itf_dynamic;

	avb_itf_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_AVB_INTERFACE, port_id, NULL);

	if (!avb_itf_dynamic)
		goto exit;

	if (!avb_itf_dynamic->async_get_as_path_unsolicited_notification_timer_running) {
		aecp_aem_send_async_unsolicited_notification(&entity->aecp, AECP_AEM_CMD_GET_AS_PATH, AEM_DESC_TYPE_AVB_INTERFACE, port_id);

		avb_itf_dynamic->async_get_as_path_unsolicited_notification_pending = false;
		avb_itf_dynamic->async_get_as_path_unsolicited_notification_timer_running = true;

		timer_start(&avb_itf_dynamic->async_get_as_path_unsolicited_notification_timer, AECP_GET_AS_PATH_ASYNC_UNSOLICITED_NOTIFICATION_MS);
	} else {
		avb_itf_dynamic->async_get_as_path_unsolicited_notification_pending = true;
	}

exit:
	return;
}

/** Main AECP AEM receive function for controller's AECP command
 * Follows the AVDECC entity model state machine (9.2.2.3.1.4).
 * \return 	0 on success, negative otherwise
 * \param	aecp		pointer to the AECP context
 * \param	pdu			pointer to the AECP PDU
 * \param	avtp_len	length of the AVTP payload.
 * \param	mac_src		source MAC address of the received PDU
 * \param	port_id		avdecc port / interface index on which we received the PDU
 */
static int aecp_aem_received_controller_command(struct aecp_ctx *aecp, struct aecp_aem_pdu *pdu, u16 avtp_len, u8 *mac_src, unsigned int port_id)
{
	struct entity *entity = container_of(aecp, struct entity, aecp);
	struct avdecc_port *port_rsp = &entity->avdecc->port[port_id];
	u64 entity_id = pdu->entity_id;
	u64 controller_entity_id = pdu->controller_entity_id;
	struct aecp_aem_pdu *aecp_rsp = NULL;
	struct net_tx_desc *desc_rsp;
	struct unsolicited_ctx *unsolicited_entry;
	u8 status;
	u16 cmd_type;
	u16 len	= sizeof(struct aecp_aem_pdu); //data size after AVTP control hdr
	int rc = 0;
	bool send_unsolicited_notification = false; /* to send a synchronous unsolicited notification to registered controllers if command is fully handled here and made changes to the entity */

	desc_rsp = aecp_net_tx_prepare(port_rsp, pdu, &len, (void **)&aecp_rsp); //FIXME check if we can re-use same buf
	if (!desc_rsp) {
		os_log(LOG_ERR, "aecp(%p) Cannot alloc tx descriptor\n", aecp);
		rc = -1;
		goto exit;
	}

	cmd_type = AECP_AEM_GET_CMD_TYPE(pdu);

	os_log(LOG_DEBUG, "aecp(%p) command (%x, %s) seq_id(%d)\n", aecp, cmd_type, aecp_aem_cmdtype2string(cmd_type), ntohs(pdu->sequence_id));

	switch (cmd_type) {
	case AECP_AEM_CMD_READ_DESCRIPTOR:
	{
		struct aecp_aem_read_desc_cmd_pdu *read_desc_cmd = (struct aecp_aem_read_desc_cmd_pdu *)(pdu + 1);
		void *desc;
		u16 desc_len;

		desc = aem_get_descriptor(entity->aem_descs, ntohs(read_desc_cmd->descriptor_type), ntohs(read_desc_cmd->descriptor_index), &desc_len);
		if (desc) {
			struct aecp_aem_read_desc_rsp_pdu *read_desc_rsp = (struct aecp_aem_read_desc_rsp_pdu *)(aecp_rsp + 1);

			os_memcpy((read_desc_rsp + 1), desc, desc_len);
			read_desc_rsp->configuration_index = read_desc_cmd->configuration_index; //FIXME config handling
			status = AECP_AEM_SUCCESS;
			len += (desc_len + sizeof(struct aecp_aem_read_desc_rsp_pdu));
		}
		else {
			struct aecp_aem_read_desc_cmd_pdu *read_desc_fail = (struct aecp_aem_read_desc_cmd_pdu *)(aecp_rsp + 1);

			read_desc_fail->descriptor_type = read_desc_cmd->descriptor_type;
			read_desc_fail->descriptor_index = read_desc_cmd->descriptor_index;
			read_desc_fail->configuration_index = read_desc_cmd->configuration_index; //FIXME config handling
			status = AECP_AEM_NO_SUCH_DESCRIPTOR;
			len += sizeof(struct aecp_aem_read_desc_cmd_pdu);
		}
		break;
	}
	case AECP_AEM_CMD_ENTITY_AVAILABLE:
	{
		status = AECP_AEM_SUCCESS;
		break;
	}
	case AECP_AEM_CMD_ACQUIRE_ENTITY:
	{
		struct aecp_aem_acquire_entity_pdu *acquire_cmd  = (struct aecp_aem_acquire_entity_pdu *)(pdu + 1);
		struct aecp_aem_acquire_entity_pdu *acquire_rsp = (struct aecp_aem_acquire_entity_pdu *)(aecp_rsp + 1);
		struct entity_dynamic_desc *entity_dynamic = aem_get_descriptor(entity->aem_dynamic_descs,
										     AEM_DESC_TYPE_ENTITY, 0, NULL);

		len += sizeof(struct aecp_aem_acquire_entity_pdu);
		os_memset(acquire_rsp, 0, sizeof(struct aecp_aem_acquire_entity_pdu));

		acquire_rsp->descriptor_type = acquire_cmd->descriptor_type;
		acquire_rsp->descriptor_index = acquire_cmd->descriptor_index;

		if (entity->milan_mode) {
			/* AVNU.IO.CONTROL 7.3.1 */
			status = AECP_AEM_NOT_IMPLEMENTED;
			break;
		}

		if (acquire_cmd->descriptor_type != ntohs(AEM_DESC_TYPE_ENTITY)) {
			/* IEEE 1722.1-2013 7.4.1.2 */
			status = AECP_AEM_BAD_ARGUMENTS;
			break;
		}

		if (acquire_cmd->flags & ntohl(AECP_AEM_ACQUIRE_RELEASE)) {
			if (controller_entity_id == entity_dynamic->acquiring_controller_id) {
				entity_dynamic->acquire_status = RELEASED;
				status = AECP_AEM_SUCCESS;
				os_log(LOG_INFO, "aecp(%p) Controller %"PRIx64" released entity %"PRIx64"\n",
					aecp, ntohll(entity_dynamic->acquiring_controller_id), ntohll(entity_id));
			}
			else
				status = AECP_AEM_ENTITY_ACQUIRED; //FIXME send a CONTROLLER_AVAILABLE to owner
		}
		else {
			if (entity_dynamic->acquire_status == RELEASED) {
				entity_dynamic->acquire_status = ACQUIRED;
				status = AECP_AEM_SUCCESS;
				entity_dynamic->acquiring_controller_id = controller_entity_id;
				os_log(LOG_INFO, "aecp(%p) Controller %"PRIx64" acquired entity %"PRIx64"\n",
					aecp, ntohll(entity_dynamic->acquiring_controller_id), ntohll(entity_id));
			}
			else {
				if (controller_entity_id == entity_dynamic->acquiring_controller_id)
					status = AECP_AEM_SUCCESS; /* aquired again by the same controller */
				else
					status = AECP_AEM_ENTITY_ACQUIRED;
			}
		}

		if (entity_dynamic->acquire_status == ACQUIRED)
			copy_64(&acquire_rsp->owner_id, &entity_dynamic->acquiring_controller_id);
		break;
	}
	case AECP_AEM_CMD_LOCK_ENTITY:
	{
		struct aecp_aem_lock_entity_pdu *lock_cmd  = (struct aecp_aem_lock_entity_pdu *)(pdu + 1);
		struct aecp_aem_lock_entity_pdu *lock_rsp = (struct aecp_aem_lock_entity_pdu *)(aecp_rsp + 1);
		struct entity_dynamic_desc *entity_dynamic = aem_get_descriptor(entity->aem_dynamic_descs,
										     AEM_DESC_TYPE_ENTITY, 0, NULL);

		len += sizeof(struct aecp_aem_lock_entity_pdu);
		os_memset(lock_rsp, 0, sizeof(struct aecp_aem_lock_entity_pdu));

		lock_rsp->descriptor_type = lock_cmd->descriptor_type;
		lock_rsp->descriptor_index = lock_cmd->descriptor_index;

		if (lock_cmd->descriptor_type != ntohs(AEM_DESC_TYPE_ENTITY)) {
			status = AECP_AEM_NOT_SUPPORTED;
			break;
		}

		if (avdecc_entity_is_acquired(entity, controller_entity_id)) {
			status = AECP_AEM_ENTITY_ACQUIRED;
			break;
		}

		if (lock_cmd->flags & ntohl(AECP_AEM_LOCK_UNLOCK)) {
			if (entity_dynamic->lock_status == UNLOCKED) {
				status = AECP_AEM_SUCCESS; /* Already Unlocked */
			} else {
				if (controller_entity_id == entity_dynamic->locking_controller_id) {
					entity_dynamic->lock_status = UNLOCKED;
					status = AECP_AEM_SUCCESS;
					send_unsolicited_notification = true;

					if (timer_is_running(&entity_dynamic->lock_timer))
						timer_stop(&entity_dynamic->lock_timer);
					os_log(LOG_INFO, "aecp(%p) Controller %"PRIx64" unlocked entity %"PRIx64"\n",
						aecp, ntohll(controller_entity_id), ntohll(entity_id));
				} else {
					status = AECP_AEM_ENTITY_LOCKED;
				}
			}
		} else {
			if (entity_dynamic->lock_status == UNLOCKED) {
				entity_dynamic->lock_status = LOCKED;
				status = AECP_AEM_SUCCESS;
				send_unsolicited_notification = true;

				/* Start the 1 min lock timer. */
				timer_start(&entity_dynamic->lock_timer, AVDECC_CFG_ENTITY_LOCK_TIMER_MS);
				entity_dynamic->locking_controller_id = controller_entity_id;
				os_log(LOG_INFO, "aecp(%p) Controller %"PRIx64" locked entity %"PRIx64"\n",
					aecp, ntohll(entity_dynamic->locking_controller_id), ntohll(entity_id));
			} else {
				if (controller_entity_id == entity_dynamic->locking_controller_id) {
					status = AECP_AEM_SUCCESS; /* locked again by the same controller, restart the locking timer */
					if (timer_is_running(&entity_dynamic->lock_timer))
						timer_stop(&entity_dynamic->lock_timer);
					timer_start(&entity_dynamic->lock_timer, AVDECC_CFG_ENTITY_LOCK_TIMER_MS);
				} else {
					status = AECP_AEM_ENTITY_LOCKED;
				}
			}
		}

		if (entity_dynamic->lock_status == LOCKED)
			copy_64(&lock_rsp->locked_id, &entity_dynamic->locking_controller_id);

		break;
	}
	case AECP_AEM_CMD_SET_CONFIGURATION:
	{
		struct aecp_aem_set_configuration_pdu *set_configuration_cmd  = (struct aecp_aem_set_configuration_pdu *)(pdu + 1);
		struct aecp_aem_set_configuration_pdu *set_configuration_rsp = (struct aecp_aem_set_configuration_pdu *)(aecp_rsp + 1);
		struct entity_descriptor *entity_desc = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_ENTITY, 0, NULL);
		unsigned int num_streams, i;

		len += sizeof(struct aecp_aem_set_configuration_pdu);
		os_memset(set_configuration_rsp, 0, sizeof(struct aecp_aem_set_configuration_pdu));

		/* The response always contains the current value, even on failure (IEEE1722.1-2013 7.4.7.1). Init to current value, and change later on success if needed */
		set_configuration_rsp->configuration_index = entity_desc->current_configuration;

		if (avdecc_entity_is_acquired(entity, controller_entity_id)) {
			status = AECP_AEM_ENTITY_ACQUIRED;
			break;
		}

		if (avdecc_entity_is_locked(entity, controller_entity_id)) {
			status = AECP_AEM_ENTITY_LOCKED;
			break;
		}

		num_streams = aem_get_descriptor_max(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_INPUT);
		for (i = 0; i < num_streams; i++) {
			if (acmp_is_stream_running(entity, AEM_DESC_TYPE_STREAM_INPUT, i)) {
				status = AECP_AEM_STREAM_IS_RUNNING;
				goto send_rsp;
			}
		}

		num_streams = aem_get_descriptor_max(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_OUTPUT);
		for (i = 0; i < num_streams; i++) {
			if (acmp_is_stream_running(entity, AEM_DESC_TYPE_STREAM_OUTPUT, i)) {
				status = AECP_AEM_STREAM_IS_RUNNING;
				goto send_rsp;
			}
		}

		/* FIXME only support one configuration currently */
		if (entity_desc->current_configuration == set_configuration_cmd->configuration_index) {
			status = AECP_AEM_SUCCESS;
		} else {
			status = AECP_AEM_NOT_SUPPORTED;
		}

		break;
	}
	case AECP_AEM_CMD_GET_CONFIGURATION:
	{
		struct aecp_aem_get_configuration_rsp_pdu *get_configuration_rsp = (struct aecp_aem_get_configuration_rsp_pdu *)(aecp_rsp + 1);
		struct entity_descriptor *entity_desc = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_ENTITY, 0, NULL);

		status = AECP_AEM_SUCCESS;

		len += sizeof(struct aecp_aem_get_configuration_rsp_pdu);
		os_memset(get_configuration_rsp, 0, sizeof(struct aecp_aem_get_configuration_rsp_pdu));

		get_configuration_rsp->configuration_index = entity_desc->current_configuration;

		break;
	}
	case AECP_AEM_CMD_SET_STREAM_FORMAT:
	{
		struct aecp_aem_set_stream_format_pdu *set_stream_format_cmd  = (struct aecp_aem_set_stream_format_pdu *)(pdu + 1);
		struct aecp_aem_set_stream_format_pdu *set_stream_format_rsp = (struct aecp_aem_set_stream_format_pdu *)(aecp_rsp + 1);
		u16 descriptor_type = ntohs(set_stream_format_cmd->descriptor_type);
		struct stream_descriptor *stream_desc;
		unsigned int i;

		len += sizeof(struct aecp_aem_set_stream_format_pdu);
		os_memset(set_stream_format_rsp, 0, sizeof(struct aecp_aem_set_stream_format_pdu));

		set_stream_format_rsp->descriptor_type = set_stream_format_cmd->descriptor_type;
		set_stream_format_rsp->descriptor_index = set_stream_format_cmd->descriptor_index;

		if (descriptor_type != AEM_DESC_TYPE_STREAM_INPUT && descriptor_type != AEM_DESC_TYPE_STREAM_OUTPUT) {
			status = AECP_AEM_BAD_ARGUMENTS;
			break;
		}

		stream_desc = aem_get_descriptor(entity->aem_descs, descriptor_type, ntohs(set_stream_format_cmd->descriptor_index), NULL);
		if (!stream_desc) {
			status = AECP_AEM_NO_SUCH_DESCRIPTOR;
			break;
		}

		/* The response always contains the current value, even on failure (IEEE1722.1-2013 7.4.9.1). Init to current value, and change later on success if needed */
		copy_64(&set_stream_format_rsp->stream_format, &stream_desc->current_format);

		if (avdecc_entity_is_acquired(entity, controller_entity_id)) {
			status = AECP_AEM_ENTITY_ACQUIRED;
			break;
		}

		if (avdecc_entity_is_locked(entity, controller_entity_id)) {
			status = AECP_AEM_ENTITY_LOCKED;
			break;
		}

		if (acmp_is_stream_running(entity, descriptor_type, ntohs(set_stream_format_cmd->descriptor_index))) {
			status = AECP_AEM_STREAM_IS_RUNNING;
			break;
		}

		status = AECP_AEM_NOT_SUPPORTED;
		for (i = 0; i < ntohs(stream_desc->number_of_formats); i++) {
			if (cmp_64(&set_stream_format_cmd->stream_format, &stream_desc->formats[i])) {
				/* FIXME check audio mappings referencing channels of the old stream format per AVNU.IO.CONTROL 7.3.7 */
				/* Update the descriptor format and send the unsolicited notification (AVNU.IO.CONTROL 7.5.2) only on format change*/
				if (!cmp_64(&stream_desc->current_format, &set_stream_format_cmd->stream_format)) {
					copy_64(&stream_desc->current_format, &set_stream_format_cmd->stream_format);
					send_unsolicited_notification = true;
				}

				copy_64(&set_stream_format_rsp->stream_format, &stream_desc->current_format);

				status = AECP_AEM_SUCCESS;
				break;
			}
		}

		break;
	}
	case AECP_AEM_CMD_GET_STREAM_FORMAT:
	{
		struct aecp_aem_get_stream_format_cmd_pdu *get_stream_format_cmd  = (struct aecp_aem_get_stream_format_cmd_pdu *)(pdu + 1);
		struct aecp_aem_get_stream_format_rsp_pdu *get_stream_format_rsp = (struct aecp_aem_get_stream_format_rsp_pdu *)(aecp_rsp + 1);
		u16 descriptor_type = ntohs(get_stream_format_cmd->descriptor_type);
		struct stream_descriptor *stream_desc;

		status = AECP_AEM_SUCCESS;

		len += sizeof(struct aecp_aem_get_stream_format_rsp_pdu);
		os_memset(get_stream_format_rsp, 0, sizeof(struct aecp_aem_get_stream_format_rsp_pdu));

		get_stream_format_rsp->descriptor_type = get_stream_format_cmd->descriptor_type;
		get_stream_format_rsp->descriptor_index = get_stream_format_cmd->descriptor_index;

		if (descriptor_type != AEM_DESC_TYPE_STREAM_INPUT && descriptor_type != AEM_DESC_TYPE_STREAM_OUTPUT) {
			status = AECP_AEM_BAD_ARGUMENTS;
			break;
		}

		stream_desc = aem_get_descriptor(entity->aem_descs, descriptor_type, ntohs(get_stream_format_cmd->descriptor_index), NULL);
		if (!stream_desc) {
			status = AECP_AEM_NO_SUCH_DESCRIPTOR;
			break;
		}

		copy_64(&get_stream_format_rsp->stream_format, &stream_desc->current_format);

		break;
	}
	case AECP_AEM_CMD_REGISTER_UNSOLICITED_NOTIFICATION:
	{
		if (aecp_unsolicited_add(aecp, mac_src, controller_entity_id, port_id) < 0) {
			os_log(LOG_ERR, "aecp(%p) Reached max unsolicited registrations (%d), ignoring REGISTER_UNSOLICITED_NOTIFICATION from controller %"PRIx64".\n",
					aecp, aecp->max_unsolicited_registrations, ntohll(controller_entity_id));

			status = AECP_AEM_NO_RESOURCES;
		} else
			status = AECP_AEM_SUCCESS;
		break;
	}
	case AECP_AEM_CMD_DEREGISTER_UNSOLICITED_NOTIFICATION:
	{
		if (aecp_unsolicited_remove(aecp, controller_entity_id, port_id) < 0) {
			os_log(LOG_ERR, "aecp(%p) received DEREGISTER_UNSOLICITED_NOTIFICATION from controller %"PRIx64" but mac address not previously registered, ignoring.\n",
					aecp, ntohll(controller_entity_id));

			status = AECP_AEM_BAD_ARGUMENTS;
		} else
			status = AECP_AEM_SUCCESS;
		break;
	}
	case AECP_AEM_CMD_SET_CONTROL:
	{
		struct aecp_aem_set_get_control_pdu *set_control_cmd  = (struct aecp_aem_set_get_control_pdu *)(pdu + 1);
		struct aecp_aem_set_get_control_pdu *set_control_rsp = (struct aecp_aem_set_get_control_pdu *)(aecp_rsp + 1);
		void *values_cmd = set_control_cmd + 1;
		void *values_rsp = set_control_rsp + 1;
		struct control_descriptor *desc;
		int rc;
		int desc_values_len;

		status = AECP_AEM_IN_PROGRESS;

		os_memcpy(set_control_rsp, set_control_cmd, avtp_len - len);
		len = avtp_len;

		if (avdecc_entity_is_acquired(entity, controller_entity_id)) {
			status = AECP_AEM_ENTITY_ACQUIRED;
			break;
		}

		if (avdecc_entity_is_locked(entity, controller_entity_id)) {
			status = AECP_AEM_ENTITY_LOCKED;
			break;
		}

		desc = aem_get_descriptor(entity->aem_descs, ntohs(set_control_cmd->descriptor_type), ntohs(set_control_cmd->descriptor_index), NULL);
		if (!desc) {
			status = AECP_AEM_NO_SUCH_DESCRIPTOR;
			break;
		}

		if ((desc_values_len = aecp_aem_control_desc_to_pdu(desc, values_rsp, AVDECC_AECP_MAX_SIZE)) < 0) {
			status = AECP_AEM_ENTITY_MISBEHAVING;
			break;
		}

		len = desc_values_len;
		len += sizeof(struct aecp_aem_pdu) + sizeof(struct aecp_aem_set_get_control_pdu);

		if (AEM_CONTROL_GET_R(ntohs(desc->control_value_type))) {
			status = AECP_AEM_NOT_SUPPORTED;
			break;
		}

		if (!aecp_aem_validate_control_value(desc, values_cmd, avtp_len - sizeof(struct aecp_aem_pdu) - sizeof(struct aecp_aem_set_get_control_pdu))) {
			status = AECP_AEM_BAD_ARGUMENTS;
			break;
		}

		/* Send AECP command to external control application */
		if (aecp_aem_ipc_tx_command(aecp, pdu, avtp_len, &entity->avdecc->ipc_tx_controlled, IPC_DST_ALL) < 0) {
			status = AECP_AEM_ENTITY_MISBEHAVING;
			break;
		}
		os_log(LOG_INFO, "aecp(%p) successfully sent AVB_MSG_AECP IPC command type (%x)\n", aecp, cmd_type);

		/* Add to application inflight list */
		rc = aecp_application_inflight_add(aecp, pdu, avtp_len, mac_src, port_id);
		if (rc != AECP_AEM_SUCCESS) {
			os_log(LOG_ERR, "aecp(%p) Could not add to application inflight\n", aecp);
			status = rc;
			break;
		}

		break;
	}
	case AECP_AEM_CMD_GET_CONTROL:
	{
		struct aecp_aem_set_get_control_pdu *set_control_cmd  = (struct aecp_aem_set_get_control_pdu *)(pdu + 1);
		struct aecp_aem_set_get_control_pdu * set_control_rsp = (struct aecp_aem_set_get_control_pdu *)(aecp_rsp + 1);
		void * values_rsp = set_control_rsp + 1;
		struct control_descriptor *desc;
		int desc_values_len;

		status = AECP_AEM_SUCCESS;

		os_memcpy(set_control_rsp, set_control_cmd, avtp_len - len);
		len = avtp_len;

		desc = aem_get_descriptor(entity->aem_descs, ntohs(set_control_cmd->descriptor_type), ntohs(set_control_cmd->descriptor_index), NULL);
		if (!desc) {
			status = AECP_AEM_NO_SUCH_DESCRIPTOR;
			break;
		}

		if ((desc_values_len = aecp_aem_control_desc_to_pdu(desc, values_rsp, AVDECC_AECP_MAX_SIZE)) < 0) {
			status = AECP_AEM_ENTITY_MISBEHAVING;
			break;

		}

		len = desc_values_len;
		len += sizeof(struct aecp_aem_pdu) + sizeof(struct aecp_aem_set_get_control_pdu);

		break;
	}
	case AECP_AEM_CMD_GET_AVB_INFO:
	{
		struct aecp_aem_get_avb_info_cmd_pdu *get_avb_info_cmd  = (struct aecp_aem_get_avb_info_cmd_pdu *)(pdu + 1);
		struct aecp_aem_get_avb_info_rsp_pdu *get_avb_info_rsp = (struct aecp_aem_get_avb_info_rsp_pdu *)(aecp_rsp + 1);
		struct aecp_aem_get_avb_info_msrp_mappings_format *msrp_mappings_rsp = (struct aecp_aem_get_avb_info_msrp_mappings_format *)(get_avb_info_rsp + 1);
		struct avb_interface_descriptor *desc;
		struct avb_interface_dynamic_desc *dynamic_desc;

		status = AECP_AEM_SUCCESS;

		len += sizeof(struct aecp_aem_get_avb_info_rsp_pdu);
		len += CFG_SR_CLASS_MAX * sizeof(struct aecp_aem_get_avb_info_msrp_mappings_format);

		os_memset(get_avb_info_rsp, 0,
				sizeof(struct aecp_aem_get_avb_info_rsp_pdu) + CFG_SR_CLASS_MAX * sizeof(struct aecp_aem_get_avb_info_msrp_mappings_format));

		get_avb_info_rsp->descriptor_type = get_avb_info_cmd->descriptor_type;
		get_avb_info_rsp->descriptor_index = get_avb_info_cmd->descriptor_index;

		if (get_avb_info_cmd->descriptor_type != ntohs(AEM_DESC_TYPE_AVB_INTERFACE)) {
			status = AECP_AEM_BAD_ARGUMENTS;
			break;
		}

		desc = aem_get_descriptor(entity->aem_descs, ntohs(get_avb_info_cmd->descriptor_type), ntohs(get_avb_info_cmd->descriptor_index), NULL);
		if (!desc) {
			status = AECP_AEM_NO_SUCH_DESCRIPTOR;
			break;
		}

		dynamic_desc = aem_get_descriptor(entity->aem_dynamic_descs, ntohs(get_avb_info_cmd->descriptor_type), ntohs(get_avb_info_cmd->descriptor_index), NULL);
		if (!dynamic_desc) {
			status = AECP_AEM_ENTITY_MISBEHAVING;
			break;
		}

		get_avb_info_rsp->flags = (AECP_AEM_AVB_INFO_AS_CAPABLE) | (AECP_AEM_AVB_INFO_GPTP_ENABLED) | (AECP_AEM_AVB_INFO_SRP_ENABLED);
		copy_64(&get_avb_info_rsp->gptp_grandmaster_id, &dynamic_desc->gptp_grandmaster_id);
		get_avb_info_rsp->gptp_domain_number = desc->domain_number;

		/* FIXME : Default values for SR class A mapping, but should be from SRP domain indication */
		if (entity->milan_mode) {
			get_avb_info_rsp->msrp_mappings_count = htons(1);

			msrp_mappings_rsp->traffic_class = sr_class_id(SR_CLASS_A);
			msrp_mappings_rsp->priority = sr_class_pcp(SR_CLASS_A);
			msrp_mappings_rsp->vlan_id = MRP_DEFAULT_VID;
		}

		//FIXME needs to hook propagation delay and msrp_mappings.

		break;
	}
	case AECP_AEM_CMD_SET_STREAM_INFO:
	{
		struct aecp_aem_set_stream_info_pdu *set_stream_info_cmd  = (struct aecp_aem_set_stream_info_pdu *)(pdu + 1);
		struct aecp_aem_set_stream_info_pdu *set_stream_info_rsp = (struct aecp_aem_set_stream_info_pdu *)(aecp_rsp + 1);
		u16 descriptor_type = ntohs(set_stream_info_cmd->descriptor_type);
		struct stream_descriptor *desc;
		struct stream_output_dynamic_desc *stream_out_dynamic_desc;
		unsigned int i;

		status = AECP_AEM_SUCCESS;

		len += sizeof(struct aecp_aem_set_stream_info_pdu);
		os_memset(set_stream_info_rsp, 0, sizeof(struct aecp_aem_set_stream_info_pdu));

		set_stream_info_rsp->descriptor_type = set_stream_info_cmd->descriptor_type;
		set_stream_info_rsp->descriptor_index = set_stream_info_cmd->descriptor_index;

		if (descriptor_type != AEM_DESC_TYPE_STREAM_INPUT && descriptor_type != AEM_DESC_TYPE_STREAM_OUTPUT) {
			status = AECP_AEM_BAD_ARGUMENTS;
			break;
		}

		/* For Milan, SET_STREAM_INFO is not supported for STREAM_INPUT. Per AVNU.IO.CONTROL 7.3.9.
		* FIXME: IEEE 1722.1 should implement it.
		*/
		if (descriptor_type == AEM_DESC_TYPE_STREAM_INPUT) {
			status = AEM_NOT_SUPPORTED;
			break;
		}

		desc = aem_get_descriptor(entity->aem_descs, descriptor_type, ntohs(set_stream_info_cmd->descriptor_index), NULL);
		if (!desc) {
			status = AECP_AEM_NO_SUCH_DESCRIPTOR;
			break;
		}

		stream_out_dynamic_desc = aem_get_descriptor(entity->aem_dynamic_descs, descriptor_type, ntohs(set_stream_info_cmd->descriptor_index), NULL);
		if (!stream_out_dynamic_desc) {
			status = AECP_AEM_ENTITY_MISBEHAVING;
			break;
		}

		if (avdecc_entity_is_acquired(entity, controller_entity_id)) {
			status = AECP_AEM_ENTITY_ACQUIRED;

		} else if (avdecc_entity_is_locked(entity, controller_entity_id)) {
			status = AECP_AEM_ENTITY_LOCKED;

		} else if (acmp_is_stream_running(entity, descriptor_type, ntohs(set_stream_info_cmd->descriptor_index))) {
			status = AECP_AEM_STREAM_IS_RUNNING;
		}

		/* Do the sub-commands check only if not failed already. */
		if (status == AECP_AEM_SUCCESS) {
			if (set_stream_info_cmd->flags & htonl(AECP_STREAM_FLAG_STREAM_FORMAT_VALID)) {
				status = AECP_AEM_NOT_SUPPORTED;

				for (i = 0; i < ntohs(desc->number_of_formats); i++) {
					if (cmp_64(&set_stream_info_cmd->stream_format, &desc->formats[i])) {
						status = AECP_AEM_SUCCESS;
						break;
					}
				}
			}

			if (set_stream_info_cmd->flags & htonl(AECP_STREAM_FLAG_STREAM_ID_VALID)) {
				status = AECP_AEM_NOT_SUPPORTED;
			}

			if (set_stream_info_cmd->flags & htonl(AECP_STREAM_FLAG_STREAM_DEST_MAC_VALID)) {
				/* FIXME only support automatic mode for MAC attribution (MAAP or self assigned addresses) for now */
				if (!is_invalid_mac_addr(set_stream_info_cmd->stream_dest_mac)) {
					status = AECP_AEM_NOT_SUPPORTED;
				}
			}

			if (set_stream_info_cmd->flags & htonl(AECP_STREAM_FLAG_STREAM_VLAN_ID_VALID)) {
				status = AECP_AEM_NOT_SUPPORTED;
			}

			if (set_stream_info_cmd->flags & htonl(AECP_STREAM_FLAG_MSRP_ACC_LAT_VALID)) {
				if (!entity->milan_mode)
					status = AECP_AEM_NOT_SUPPORTED;
				else if (ntohl(set_stream_info_cmd->msrp_accumulated_latency) > STREAM_PRESENTATION_TIME_OFFSET_MAX) /* AVNU.IO.CONTROL 7.3.9 */
					status = AECP_AEM_BAD_ARGUMENTS;
			}
		}

		/* If all sub commands are successful, do all updates at once:
		 * This behavior is specified in AVNU.IO.CONTROL 7.3.9 for Milan, so adapt it also
		 * for legacy IEEE1722.1
		 */
		if (status == AECP_AEM_SUCCESS) {
			if (set_stream_info_cmd->flags & htonl(AECP_STREAM_FLAG_STREAM_FORMAT_VALID)) {
				/* Update the descriptor format and send the unsolicited notification (AVNU.IO.CONTROL 7.5.2) only on format change*/
				if (!cmp_64(&desc->current_format, &set_stream_info_cmd->stream_format)) {
					copy_64(&desc->current_format, &set_stream_info_cmd->stream_format);
					send_unsolicited_notification = true;
				}
			}

			if (set_stream_info_cmd->flags & htonl(AECP_STREAM_FLAG_STREAM_ID_VALID)) {
				/* Update the dynamic descriptor stream_id and send the unsolicited notification (AVNU.IO.CONTROL 7.5.2) only on stream_id change*/
				if (!cmp_64(&stream_out_dynamic_desc->stream_id, &set_stream_info_cmd->stream_id)) {
					copy_64(&stream_out_dynamic_desc->stream_id, &set_stream_info_cmd->stream_id);
					send_unsolicited_notification = true;
				}
			}

			if (set_stream_info_cmd->flags & htonl(AECP_STREAM_FLAG_MSRP_ACC_LAT_VALID)) {
				/* Update the dynamic descriptor msrp_accumulated_latency and send the unsolicited notification (AVNU.IO.CONTROL 7.5.2) only on msrp_accumulated_latency change*/
				if (stream_out_dynamic_desc->presentation_time_offset != ntohl(set_stream_info_cmd->msrp_accumulated_latency)) {
					stream_out_dynamic_desc->presentation_time_offset = ntohl(set_stream_info_cmd->msrp_accumulated_latency);
					send_unsolicited_notification = true;
				}
			}
		}

		/* Always set valid values, even on failures. */

		copy_64(&set_stream_info_rsp->stream_format, &desc->current_format);
		set_stream_info_rsp->flags |= htonl(AECP_STREAM_FLAG_STREAM_FORMAT_VALID);

		/* For Milan STREAM_OUTPUT, the stream_id and vlan_id are generated on init and always valid.
		 * FIXME: Make the same behavior for IEEE1722.1 (currently stream_id only valid when the stream has been
		 * connected and vlan_id is set to an internally default value)
		 */
		if (entity->milan_mode) {
			copy_64(&set_stream_info_rsp->stream_id, &stream_out_dynamic_desc->stream_id);
			set_stream_info_rsp->flags |= htonl(AECP_STREAM_FLAG_STREAM_ID_VALID);

			set_stream_info_rsp->stream_vlan_id = stream_out_dynamic_desc->stream_vlan_id;
			set_stream_info_rsp->flags |= htonl(AECP_STREAM_FLAG_STREAM_VLAN_ID_VALID);

			/* Per AVNU.IO.CONTROL 7.3.10.2: For Milan STREAM_OUTPUT, msrp_accumulated_latency is always valid */
			set_stream_info_rsp->msrp_accumulated_latency = htonl(stream_out_dynamic_desc->presentation_time_offset);
			set_stream_info_rsp->flags |= htonl(AECP_STREAM_FLAG_MSRP_ACC_LAT_VALID);
		}

		if (!is_invalid_mac_addr(stream_out_dynamic_desc->stream_dest_mac)) {
			os_memcpy(set_stream_info_rsp->stream_dest_mac, stream_out_dynamic_desc->stream_dest_mac, 6);
			set_stream_info_rsp->flags |= htonl(AECP_STREAM_FLAG_STREAM_DEST_MAC_VALID);
		}

		break;
	}
	case AECP_AEM_CMD_GET_STREAM_INFO:
	{
		struct aecp_aem_get_stream_info_cmd_pdu *get_stream_info_cmd  = (struct aecp_aem_get_stream_info_cmd_pdu *)(pdu + 1);

		status = aecp_aem_get_stream_info_response(entity, aecp_rsp, &len, ntohs(get_stream_info_cmd->descriptor_type), ntohs(get_stream_info_cmd->descriptor_index));

		break;
	}
	case AECP_AEM_CMD_SET_NAME:
	{
		struct aecp_aem_set_name_pdu *set_name_cmd  = (struct aecp_aem_set_name_pdu *)(pdu + 1);
		struct aecp_aem_set_name_pdu *set_name_rsp = (struct aecp_aem_set_name_pdu *)(aecp_rsp + 1);
		struct entity_descriptor *entity_desc = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_ENTITY, 0, NULL);
		void *object_name = NULL;
		void *desc;

		status = AECP_AEM_SUCCESS;

		len += sizeof(struct aecp_aem_set_name_pdu);
		os_memset(set_name_rsp, 0, sizeof(struct aecp_aem_set_name_pdu));

		set_name_rsp->descriptor_type = set_name_cmd->descriptor_type;
		set_name_rsp->descriptor_index = set_name_cmd->descriptor_index;
		set_name_rsp->name_index = set_name_cmd->name_index;
		set_name_rsp->configuration_index = set_name_cmd->configuration_index;

		/* FIXME we currently support only one configuration */
		if (entity_desc->current_configuration != set_name_cmd->configuration_index) {
			status = AECP_AEM_NOT_SUPPORTED;
			break;
		}

		desc = aem_get_descriptor(entity->aem_descs, ntohs(set_name_cmd->descriptor_type), ntohs(set_name_cmd->descriptor_index), NULL);
		if (!desc) {
			status = AECP_AEM_NO_SUCH_DESCRIPTOR;
			break;
		}

		switch (ntohs(set_name_cmd->descriptor_type)) {
		case AEM_DESC_TYPE_ENTITY:
			if (ntohs(set_name_cmd->name_index) == 0) {
				object_name = (void *)((struct entity_descriptor *)desc)->entity_name;

			} else if (ntohs(set_name_cmd->name_index) == 1) {
				object_name = (void *)((struct entity_descriptor *)desc)->group_name;

			} else {
				status = AECP_AEM_BAD_ARGUMENTS;
			}
			break;

		case AEM_DESC_TYPE_CONFIGURATION:
			object_name = (void *)((struct configuration_descriptor *)desc)->object_name;
			break;
		case AEM_DESC_TYPE_AUDIO_UNIT:
			object_name = (void *)((struct audio_unit_descriptor *)desc)->object_name;
			break;
		case AEM_DESC_TYPE_VIDEO_UNIT:
			object_name = (void *)((struct video_unit_descriptor *)desc)->object_name;
			break;
		case AEM_DESC_TYPE_STREAM_INPUT:
		case AEM_DESC_TYPE_STREAM_OUTPUT:
			object_name = (void *)((struct stream_descriptor *)desc)->object_name;
			break;
		case AEM_DESC_TYPE_JACK_INPUT:
		case AEM_DESC_TYPE_JACK_OUTPUT:
			object_name = (void *)((struct jack_descriptor *)desc)->object_name;
			break;
		case AEM_DESC_TYPE_AVB_INTERFACE:
			object_name = (void *)((struct avb_interface_descriptor *)desc)->object_name;
			break;
		case AEM_DESC_TYPE_CLOCK_SOURCE:
			object_name = (void *)((struct clock_source_descriptor *)desc)->object_name;
			break;
		case AEM_DESC_TYPE_CLOCK_DOMAIN:
			object_name = (void *)((struct clock_domain_descriptor *)desc)->object_name;
			break;
		case AEM_DESC_TYPE_AUDIO_CLUSTER:
			object_name = (void *)((struct audio_cluster_descriptor *)desc)->object_name;
			break;
		case AEM_DESC_TYPE_VIDEO_CLUSTER:
			object_name = (void *)((struct video_cluster_descriptor *)desc)->object_name;
			break;
		case AEM_DESC_TYPE_CONTROL:
			object_name = (void *)((struct control_descriptor *)desc)->object_name;
			break;
		default:
			status = AECP_AEM_BAD_ARGUMENTS;
			break;
		}

		if (avdecc_entity_is_acquired(entity, controller_entity_id)) {
			status = AECP_AEM_ENTITY_ACQUIRED;

		} else if (avdecc_entity_is_locked(entity, controller_entity_id)) {
			status = AECP_AEM_ENTITY_LOCKED;
		}

		if (object_name) {
			/* Update the descriptor object_name and send the unsolicited notification (AVNU.IO.CONTROL 7.5.2) only on object_name change*/
			if (status == AECP_AEM_SUCCESS && os_memcmp(object_name, set_name_cmd->name, AEM_STR_LEN_MAX)) {
				os_memcpy(object_name, set_name_cmd->name, AEM_STR_LEN_MAX);

				send_unsolicited_notification = true;
			}

			/* The response always contains the current value, even on failure (IEEE1722.1-2013 7.4.17.1) */
			os_memcpy(set_name_rsp->name, object_name, AEM_STR_LEN_MAX);
		}

		break;
	}
	case AECP_AEM_CMD_GET_NAME:
	{
		struct aecp_aem_get_name_cmd_pdu *get_name_cmd  = (struct aecp_aem_get_name_cmd_pdu *)(pdu + 1);
		struct aecp_aem_get_name_rsp_pdu *get_name_rsp = (struct aecp_aem_get_name_rsp_pdu *)(aecp_rsp + 1);
		struct entity_descriptor *entity_desc = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_ENTITY, 0, NULL);
		void *desc;

		status = AECP_AEM_SUCCESS;

		len += sizeof(struct aecp_aem_get_name_rsp_pdu);
		os_memset(get_name_rsp, 0, sizeof(struct aecp_aem_get_name_rsp_pdu));

		get_name_rsp->descriptor_type = get_name_cmd->descriptor_type;
		get_name_rsp->descriptor_index = get_name_cmd->descriptor_index;
		get_name_rsp->name_index = get_name_cmd->name_index;
		get_name_rsp->configuration_index = get_name_cmd->configuration_index;

		/* FIXME we currently support only one configuration */
		if (entity_desc->current_configuration != get_name_cmd->configuration_index) {
			status = AECP_AEM_NOT_SUPPORTED;
			break;
		}

		desc = aem_get_descriptor(entity->aem_descs, ntohs(get_name_cmd->descriptor_type), ntohs(get_name_cmd->descriptor_index), NULL);
		if (!desc) {
			status = AECP_AEM_NO_SUCH_DESCRIPTOR;
			break;
		}

		switch (ntohs(get_name_cmd->descriptor_type)) {
		case AEM_DESC_TYPE_ENTITY:
			if (ntohs(get_name_cmd->name_index) == 0) {
				os_memcpy(get_name_rsp->name, ((struct entity_descriptor *)desc)->entity_name, AEM_STR_LEN_MAX);

			} else if (ntohs(get_name_cmd->name_index) == 1) {
				os_memcpy(get_name_rsp->name, ((struct entity_descriptor *)desc)->group_name, AEM_STR_LEN_MAX);

			} else {
				status = AECP_AEM_BAD_ARGUMENTS;
			}
			break;

		case AEM_DESC_TYPE_CONFIGURATION:
			os_memcpy(get_name_rsp->name, ((struct configuration_descriptor *)desc)->object_name, AEM_STR_LEN_MAX);
			break;
		case AEM_DESC_TYPE_AUDIO_UNIT:
			os_memcpy(get_name_rsp->name, ((struct audio_unit_descriptor *)desc)->object_name, AEM_STR_LEN_MAX);
			break;
		case AEM_DESC_TYPE_VIDEO_UNIT:
			os_memcpy(get_name_rsp->name, ((struct video_unit_descriptor *)desc)->object_name, AEM_STR_LEN_MAX);
			break;
		case AEM_DESC_TYPE_STREAM_INPUT:
		case AEM_DESC_TYPE_STREAM_OUTPUT:
			os_memcpy(get_name_rsp->name, ((struct stream_descriptor *)desc)->object_name, AEM_STR_LEN_MAX);
			break;
		case AEM_DESC_TYPE_JACK_INPUT:
		case AEM_DESC_TYPE_JACK_OUTPUT:
			os_memcpy(get_name_rsp->name, ((struct jack_descriptor *)desc)->object_name, AEM_STR_LEN_MAX);
			break;
		case AEM_DESC_TYPE_AVB_INTERFACE:
			os_memcpy(get_name_rsp->name, ((struct avb_interface_descriptor *)desc)->object_name, AEM_STR_LEN_MAX);
			break;
		case AEM_DESC_TYPE_CLOCK_SOURCE:
			os_memcpy(get_name_rsp->name, ((struct clock_source_descriptor *)desc)->object_name, AEM_STR_LEN_MAX);
			break;
		case AEM_DESC_TYPE_CLOCK_DOMAIN:
			os_memcpy(get_name_rsp->name, ((struct clock_domain_descriptor *)desc)->object_name, AEM_STR_LEN_MAX);
			break;
		case AEM_DESC_TYPE_AUDIO_CLUSTER:
			os_memcpy(get_name_rsp->name, ((struct audio_cluster_descriptor *)desc)->object_name, AEM_STR_LEN_MAX);
			break;
		case AEM_DESC_TYPE_VIDEO_CLUSTER:
			os_memcpy(get_name_rsp->name, ((struct video_cluster_descriptor *)desc)->object_name, AEM_STR_LEN_MAX);
			break;
		case AEM_DESC_TYPE_CONTROL:
			os_memcpy(get_name_rsp->name, ((struct control_descriptor *)desc)->object_name, AEM_STR_LEN_MAX);
			break;
		default:
			status = AECP_AEM_BAD_ARGUMENTS;
			break;
		}

		break;
	}
	case AECP_AEM_CMD_SET_SAMPLING_RATE:
	{
		struct aecp_aem_set_sampling_rate_pdu *set_rate_cmd  = (struct aecp_aem_set_sampling_rate_pdu *)(pdu + 1);
		struct aecp_aem_set_sampling_rate_pdu *set_rate_rsp = (struct aecp_aem_set_sampling_rate_pdu *)(aecp_rsp + 1);
		u32 current_sampling_rate = 0;
		void *desc;

		status = AECP_AEM_SUCCESS;

		len += sizeof(struct aecp_aem_set_sampling_rate_pdu);
		os_memset(set_rate_rsp, 0, sizeof(struct aecp_aem_set_sampling_rate_pdu));

		set_rate_rsp->descriptor_type = set_rate_cmd->descriptor_type;
		set_rate_rsp->descriptor_index = set_rate_cmd->descriptor_index;

		desc = aem_get_descriptor(entity->aem_descs, ntohs(set_rate_cmd->descriptor_type), ntohs(set_rate_cmd->descriptor_index), NULL);
		if (!desc) {
			status = AECP_AEM_NO_SUCH_DESCRIPTOR;
			break;
		}

		/* FIXME only support one sampling rate */
		switch (ntohs(set_rate_cmd->descriptor_type)) {
		case AEM_DESC_TYPE_AUDIO_UNIT:
			if (((struct audio_unit_descriptor *)desc)->current_sampling_rate != set_rate_cmd->sampling_rate)
				status = AECP_AEM_NOT_SUPPORTED;

			current_sampling_rate = ((struct audio_unit_descriptor *)desc)->current_sampling_rate;
			break;
		case AEM_DESC_TYPE_VIDEO_CLUSTER:
			if (((struct video_cluster_descriptor *)desc)->current_sampling_rate != set_rate_cmd->sampling_rate)
				status = AECP_AEM_NOT_SUPPORTED;

			current_sampling_rate = ((struct video_cluster_descriptor *)desc)->current_sampling_rate;
			break;
		case AEM_DESC_TYPE_SENSOR_CLUSTER:
			status = AECP_AEM_NOT_SUPPORTED; /* FIXME SENSOR_CLUSTER descriptor not implemented yet */
			break;
		default:
			status = AECP_AEM_BAD_ARGUMENTS;
			break;
		}

		if (avdecc_entity_is_acquired(entity, controller_entity_id)) {
			status = AECP_AEM_ENTITY_ACQUIRED;

		} else if (avdecc_entity_is_locked(entity, controller_entity_id)) {
			status = AECP_AEM_ENTITY_LOCKED;
		}

		/* The response always contains the current value, even on failure (IEEE1722.1-2013 7.4.21.1). */
		set_rate_rsp->sampling_rate = current_sampling_rate;

		break;
	}
	case AECP_AEM_CMD_GET_SAMPLING_RATE:
	{
		struct aecp_aem_get_sampling_rate_cmd_pdu *get_rate_cmd  = (struct aecp_aem_get_sampling_rate_cmd_pdu *)(pdu + 1);
		struct aecp_aem_get_sampling_rate_rsp_pdu *get_rate_rsp = (struct aecp_aem_get_sampling_rate_rsp_pdu *)(aecp_rsp + 1);
		void *desc;

		status = AECP_AEM_SUCCESS;

		len += sizeof(struct aecp_aem_get_sampling_rate_rsp_pdu);
		os_memset(get_rate_rsp, 0, sizeof(struct aecp_aem_get_sampling_rate_rsp_pdu));

		get_rate_rsp->descriptor_type = get_rate_cmd->descriptor_type;
		get_rate_rsp->descriptor_index = get_rate_cmd->descriptor_index;

		desc = aem_get_descriptor(entity->aem_descs, ntohs(get_rate_cmd->descriptor_type), ntohs(get_rate_cmd->descriptor_index), NULL);
		if (!desc) {
			status = AECP_AEM_NO_SUCH_DESCRIPTOR;
			break;
		}

		switch (ntohs(get_rate_cmd->descriptor_type)) {
		case AEM_DESC_TYPE_AUDIO_UNIT:
			get_rate_rsp->sampling_rate = ((struct audio_unit_descriptor *)desc)->current_sampling_rate;
			break;
		case AEM_DESC_TYPE_VIDEO_CLUSTER:
			get_rate_rsp->sampling_rate = ((struct video_cluster_descriptor *)desc)->current_sampling_rate;
			break;
		case AEM_DESC_TYPE_SENSOR_CLUSTER:
			status = AECP_AEM_NOT_SUPPORTED; /* FIXME SENSOR_CLUSTER descriptor not implemented yet */
			break;
		default:
			status = AECP_AEM_BAD_ARGUMENTS;
			break;
		}

		break;
	}
	case AECP_AEM_CMD_SET_CLOCK_SOURCE:
	{
		struct aecp_aem_set_clock_source_pdu *set_clock_source_cmd  = (struct aecp_aem_set_clock_source_pdu *)(pdu + 1);
		struct aecp_aem_set_clock_source_pdu *set_clock_source_rsp = (struct aecp_aem_set_clock_source_pdu *)(aecp_rsp + 1);
		u16 descriptor_type = ntohs(set_clock_source_cmd->descriptor_type);
		struct clock_domain_descriptor *desc;

		len += sizeof(struct aecp_aem_set_clock_source_pdu);
		os_memset(set_clock_source_rsp, 0, sizeof(struct aecp_aem_set_clock_source_pdu));

		set_clock_source_rsp->descriptor_type = set_clock_source_cmd->descriptor_type;
		set_clock_source_rsp->descriptor_index = set_clock_source_cmd->descriptor_index;

		if (descriptor_type != AEM_DESC_TYPE_CLOCK_DOMAIN) {
			status = AECP_AEM_BAD_ARGUMENTS;
			break;
		}

		desc = aem_get_descriptor(entity->aem_descs, descriptor_type, ntohs(set_clock_source_cmd->descriptor_index), NULL);
		if (!desc) {
			status = AECP_AEM_NO_SUCH_DESCRIPTOR;
			break;
		}

		/* The response always contains the current value, even on failure (IEEE1722.1-2013 7.4.24.1). Init to current value, and change later on success if needed */
		set_clock_source_rsp->clock_source_index = desc->clock_source_index;

		if (avdecc_entity_is_acquired(entity, controller_entity_id)) {
			status = AECP_AEM_ENTITY_ACQUIRED;
			break;
		}

		if (avdecc_entity_is_locked(entity, controller_entity_id)) {
			status = AECP_AEM_ENTITY_LOCKED;
			break;
		}

		/* FIXME we currently support only one clock source */
		/* FIXME we should pass the command to app to handle clock domain changes */
		if (set_clock_source_cmd->clock_source_index == desc->clock_source_index)
			status = AECP_AEM_SUCCESS;
		else
			status = AECP_AEM_NOT_SUPPORTED;

		/* FIXME send unsolicited notification on aecp_ipc_rx_controlled() */

		break;
	}
	case AECP_AEM_CMD_GET_CLOCK_SOURCE:
	{
		struct aecp_aem_get_clock_source_cmd_pdu *get_clock_source_cmd  = (struct aecp_aem_get_clock_source_cmd_pdu *)(pdu + 1);
		struct aecp_aem_get_clock_source_rsp_pdu *get_clock_source_rsp = (struct aecp_aem_get_clock_source_rsp_pdu *)(aecp_rsp + 1);
		u16 descriptor_type = ntohs(get_clock_source_cmd->descriptor_type);
		struct clock_domain_descriptor *desc;

		status = AECP_AEM_SUCCESS;

		len += sizeof(struct aecp_aem_get_clock_source_rsp_pdu);
		os_memset(get_clock_source_rsp, 0, sizeof(struct aecp_aem_get_clock_source_rsp_pdu));

		get_clock_source_rsp->descriptor_type = get_clock_source_cmd->descriptor_type;
		get_clock_source_rsp->descriptor_index = get_clock_source_cmd->descriptor_index;

		if (descriptor_type != AEM_DESC_TYPE_CLOCK_DOMAIN) {
			status = AECP_AEM_BAD_ARGUMENTS;
			break;
		}

		desc = aem_get_descriptor(entity->aem_descs, descriptor_type, ntohs(get_clock_source_cmd->descriptor_index), NULL);
		if (!desc) {
			status = AECP_AEM_NO_SUCH_DESCRIPTOR;
			break;
		}

		get_clock_source_rsp->clock_source_index = desc->clock_source_index;

		break;
	}
	case AECP_AEM_CMD_START_STREAMING:
	{
		struct aecp_aem_start_streaming_cmd_pdu *start_streaming_cmd  = (struct aecp_aem_start_streaming_cmd_pdu *)(pdu + 1);
		struct aecp_aem_start_streaming_cmd_pdu *start_streaming_rsp = (struct aecp_aem_start_streaming_cmd_pdu *)(aecp_rsp + 1);
		u16 descriptor_type = ntohs(start_streaming_cmd->descriptor_type);
		struct stream_descriptor *desc;

		os_log(LOG_DEBUG, "aecp(%p) received command type START_STREAMING (%x)\n", aecp, cmd_type);
		status = AECP_AEM_IN_PROGRESS;

		len += sizeof(struct aecp_aem_start_streaming_cmd_pdu);
		os_memset(start_streaming_rsp, 0, sizeof(struct aecp_aem_start_streaming_cmd_pdu));

		start_streaming_rsp->descriptor_type = start_streaming_cmd->descriptor_type;
		start_streaming_rsp->descriptor_index = start_streaming_cmd->descriptor_index;

		if (descriptor_type != AEM_DESC_TYPE_STREAM_INPUT && descriptor_type != AEM_DESC_TYPE_STREAM_OUTPUT) {
			status = AECP_AEM_BAD_ARGUMENTS;
			break;
		}

		if (entity->milan_mode && descriptor_type == AEM_DESC_TYPE_STREAM_OUTPUT) {
			status = AECP_AEM_NOT_SUPPORTED;
			break;
		}

		if (avdecc_entity_is_acquired(entity, controller_entity_id)) {
			status = AECP_AEM_ENTITY_ACQUIRED;
			break;
		}

		if (avdecc_entity_is_locked(entity, controller_entity_id)) {
			status = AECP_AEM_ENTITY_LOCKED;
			break;
		}

		desc = aem_get_descriptor(entity->aem_descs, descriptor_type, ntohs(start_streaming_cmd->descriptor_index), NULL);
		if (!desc) {
			status = AECP_AEM_NO_SUCH_DESCRIPTOR;
			break;
		}

		/* Send AECP command to external control application */
		if (aecp_aem_ipc_tx_command(aecp, pdu, len, &entity->avdecc->ipc_tx_controlled, IPC_DST_ALL) < 0) {
			status = AECP_AEM_ENTITY_MISBEHAVING;
			break;
		}
		os_log(LOG_DEBUG, "aecp(%p) successfully sent AVB_MSG_AECP IPC command type (%x)\n", aecp, cmd_type);

		/* Add to application inflight list */
		rc = aecp_application_inflight_add(aecp, pdu, avtp_len, mac_src, port_id);
		if (rc != AECP_AEM_SUCCESS) {
			os_log(LOG_ERR, "aecp(%p) Could not add to application inflight\n", aecp);
			status = rc;
			break;
		}
		os_log(LOG_DEBUG, "aecp(%p) successfully added entity (%p) to application inflight list \n", aecp, entity);

		break;
	}
	case AECP_AEM_CMD_STOP_STREAMING:
	{
		struct aecp_aem_stop_streaming_cmd_pdu *stop_streaming_cmd  = (struct aecp_aem_stop_streaming_cmd_pdu *)(pdu + 1);
		struct aecp_aem_stop_streaming_cmd_pdu *stop_streaming_rsp = (struct aecp_aem_stop_streaming_cmd_pdu *)(aecp_rsp + 1);
		u16 descriptor_type = ntohs(stop_streaming_cmd->descriptor_type);
		struct stream_descriptor *desc;

		os_log(LOG_DEBUG, "aecp(%p) received command type STOP_STREAMING (%x)\n", aecp, cmd_type);
		status = AECP_AEM_IN_PROGRESS;

		len += sizeof(struct aecp_aem_stop_streaming_cmd_pdu);
		os_memset(stop_streaming_rsp, 0, sizeof(struct aecp_aem_stop_streaming_cmd_pdu));

		stop_streaming_rsp->descriptor_type = stop_streaming_cmd->descriptor_type;
		stop_streaming_rsp->descriptor_index = stop_streaming_cmd->descriptor_index;

		if (descriptor_type != AEM_DESC_TYPE_STREAM_INPUT && descriptor_type != AEM_DESC_TYPE_STREAM_OUTPUT) {
			status = AECP_AEM_BAD_ARGUMENTS;
			break;
		}

		if (entity->milan_mode && descriptor_type == AEM_DESC_TYPE_STREAM_OUTPUT) {
			status = AECP_AEM_NOT_SUPPORTED;
			break;
		}

		if (avdecc_entity_is_acquired(entity, controller_entity_id)) {
			status = AECP_AEM_ENTITY_ACQUIRED;
			break;
		}

		if (avdecc_entity_is_locked(entity, controller_entity_id)) {
			status = AECP_AEM_ENTITY_LOCKED;
			break;
		}

		desc = aem_get_descriptor(entity->aem_descs, descriptor_type, ntohs(stop_streaming_cmd->descriptor_index), NULL);
		if (!desc) {
			status = AECP_AEM_NO_SUCH_DESCRIPTOR;
			break;
		}

		/* Send AECP command to external control application */
		if (aecp_aem_ipc_tx_command(aecp, pdu, len, &entity->avdecc->ipc_tx_controlled, IPC_DST_ALL) < 0) {
			status = AECP_AEM_ENTITY_MISBEHAVING;
			break;
		}
		os_log(LOG_DEBUG, "aecp(%p) successfully sent AVB_MSG_AECP IPC command type (%x)\n", aecp, cmd_type);

		/* Add to application inflight list */
		rc = aecp_application_inflight_add(aecp, pdu, avtp_len, mac_src, port_id);
		if (rc != AECP_AEM_SUCCESS) {
			os_log(LOG_ERR, "aecp(%p) Could not add to application inflight\n", aecp);
			status = rc;
			break;
		}
		os_log(LOG_DEBUG, "aecp(%p) successfully added entity (%p) to application inflight list \n", aecp, entity);

		break;
	}
	case AECP_AEM_CMD_GET_COUNTERS:
	{
		struct aecp_aem_get_counters_cmd_pdu *get_counters_cmd  = (struct aecp_aem_get_counters_cmd_pdu *)(pdu + 1);
		struct aecp_aem_get_counters_rsp_pdu *get_counters_rsp = (struct aecp_aem_get_counters_rsp_pdu *)(aecp_rsp + 1);

		status = AECP_AEM_SUCCESS;

		if (avdecc_entity_is_acquired(entity, controller_entity_id)) {
			status = AECP_AEM_ENTITY_ACQUIRED;
		} else if (avdecc_entity_is_locked(entity, controller_entity_id)) {
			status = AECP_AEM_ENTITY_LOCKED;
		}

		if (status != AECP_AEM_SUCCESS) {
			len += sizeof(struct aecp_aem_get_counters_rsp_pdu);

			os_memset(get_counters_rsp, 0, sizeof(*get_counters_rsp));

			get_counters_rsp->descriptor_type = get_counters_cmd->descriptor_type;
			get_counters_rsp->descriptor_index = get_counters_cmd->descriptor_index;

			break;
		}

		status = aecp_aem_get_counters_response(entity, aecp_rsp, &len, ntohs(get_counters_cmd->descriptor_type), ntohs(get_counters_cmd->descriptor_index));

		break;
	}
	case AECP_AEM_CMD_REMOVE_AUDIO_MAPPINGS:
	case AECP_AEM_CMD_ADD_AUDIO_MAPPINGS:
	{
		struct aecp_aem_modify_audio_mappings_cmd_pdu *audio_map_cmd  = (struct aecp_aem_modify_audio_mappings_cmd_pdu *)(pdu + 1);
		struct aecp_aem_modify_audio_mappings_cmd_pdu *audio_map_rsp  = (struct aecp_aem_modify_audio_mappings_cmd_pdu *)(aecp_rsp + 1);
		struct aecp_aem_get_audio_map_mappings_format *audio_mappings = (struct aecp_aem_get_audio_map_mappings_format *)(audio_map_cmd + 1);
		u16 mapping_stream_index, mapping_stream_channel, mapping_cluster_offset, mapping_cluster_channel;
		u16 number_of_mappings = ntohs(audio_map_cmd->number_of_mappings);
		u16 descriptor_type = ntohs(audio_map_cmd->descriptor_type);
		struct stream_port_descriptor *stream_port_desc;
		struct stream_descriptor *stream_desc;
		unsigned int i, j;

		os_log(LOG_DEBUG, "aecp(%p) received command type ADD/REMOVE_AUDIO_MAPPINGS (%x)\n", aecp, cmd_type);
		status = AECP_AEM_IN_PROGRESS;

		os_memcpy(audio_map_rsp, audio_map_cmd, avtp_len - len);
		len = avtp_len;

		if (descriptor_type != AEM_DESC_TYPE_STREAM_PORT_INPUT && descriptor_type != AEM_DESC_TYPE_STREAM_PORT_OUTPUT) {
			status = AECP_AEM_BAD_ARGUMENTS;
			break;
		}

		if (avdecc_entity_is_acquired(entity, controller_entity_id)) {
			status = AECP_AEM_ENTITY_ACQUIRED;
			break;
		}

		if (avdecc_entity_is_locked(entity, controller_entity_id)) {
			status = AECP_AEM_ENTITY_LOCKED;
			break;
		}

		stream_port_desc = aem_get_descriptor(entity->aem_descs, descriptor_type, ntohs(audio_map_cmd->descriptor_index), NULL);
		if (!stream_port_desc) {
			status = AECP_AEM_NO_SUCH_DESCRIPTOR;
			break;
		}

		/* Check validity of audio_mappings in the command as per MILAN Specification v1.2 5.4.2.27 */
		for (i = 0; i < number_of_mappings; i++) {
			mapping_stream_index = audio_mappings[i].mapping_stream_index;
			mapping_stream_channel = audio_mappings[i].mapping_stream_channel;
			mapping_cluster_offset = audio_mappings[i].mapping_cluster_offset;
			mapping_cluster_channel = audio_mappings[i].mapping_cluster_channel;

			stream_desc = aem_get_descriptor(entity->aem_descs, (descriptor_type == AEM_DESC_TYPE_STREAM_PORT_OUTPUT) ? AEM_DESC_TYPE_STREAM_OUTPUT : AEM_DESC_TYPE_STREAM_INPUT,
							ntohs(mapping_stream_index), NULL);

			/* A mapping is invalid if it references a channel of a Stream Input/Output that does not exist */
			if (!stream_desc) {
				status = AECP_AEM_BAD_ARGUMENTS;
				break;
			}

			if (descriptor_type == AEM_DESC_TYPE_STREAM_PORT_OUTPUT) {
				/* Changing mapping while Stream Output is streaming not supported */
				if (acmp_is_stream_running(entity, AEM_DESC_TYPE_STREAM_OUTPUT, ntohs(mapping_stream_index))) {
					status = AECP_AEM_STREAM_IS_RUNNING;
					break;
				}
			}

			/* TODO: check that the sampling rate of the stream input/output (through its current_format) that is referrenced by the mapping
			 * is the same as the current_sampling_rate of the Audio Unit, otherwise the command is invalid
			 */

			for (j = i + 1; j < number_of_mappings; j++) {
				if (descriptor_type == AEM_DESC_TYPE_STREAM_PORT_INPUT) {
					/* The command is invalid on a Stream Port Input
					 * if it contains two different mappings that reference the same cluster’s channel
					 * and two different stream’s channels that are not redundant.
					 * As per MILAN Specification v1.2 5.4.2.27
					 */
					/* TODO: exonerate redundant streams and accept the mapping */
					if ((audio_mappings[j].mapping_cluster_offset == mapping_cluster_offset) &&
					    (audio_mappings[j].mapping_cluster_channel == mapping_cluster_channel) &&
					    ((audio_mappings[j].mapping_stream_index != mapping_stream_index) ||
					    (audio_mappings[j].mapping_stream_channel != mapping_stream_channel))) {
						status = AECP_AEM_BAD_ARGUMENTS;
						break;
					}

				} else { /* AEM_DESC_TYPE_STREAM_PORT_OUTPUT */
					/* The command is invalid on a Stream Port Output
					 * if it contains two different mappings that reference the same stream’s channel
					 * and two different cluster’s channels.
					 * As per MILAN Specification v1.2 5.4.2.27
					 */
					if ((audio_mappings[j].mapping_stream_index == mapping_stream_index) &&
					    (audio_mappings[j].mapping_stream_channel == mapping_stream_channel) &&
					    ((audio_mappings[j].mapping_cluster_offset != mapping_cluster_offset) ||
					    (audio_mappings[j].mapping_cluster_channel != mapping_cluster_channel))) {
						status = AECP_AEM_BAD_ARGUMENTS;
						break;
					}
				}
			}

			if (status != AECP_AEM_IN_PROGRESS)
				break;
		}

		if (status != AECP_AEM_IN_PROGRESS)
			break;


		/* Send AECP command to external control application */
		if (aecp_aem_ipc_tx_command(aecp, pdu, len, &entity->avdecc->ipc_tx_controlled, IPC_DST_ALL) < 0) {
			status = AECP_AEM_ENTITY_MISBEHAVING;
			break;
		}
		os_log(LOG_DEBUG, "aecp(%p) successfully sent AVB_MSG_AECP IPC command type (%x)\n", aecp, cmd_type);

		/* Add to application inflight list */
		rc = aecp_application_inflight_add(aecp, aecp_rsp, len, mac_src, port_id);
		if (rc != AECP_AEM_SUCCESS) {
			os_log(LOG_ERR, "aecp(%p) Could not add to application inflight\n", aecp);
			status = rc;
			break;
		}
		os_log(LOG_DEBUG, "aecp(%p) successfully added entity (%p) to application inflight list \n", aecp, entity);

		break;
	}
	case AECP_AEM_CMD_GET_AUDIO_MAP:
	{
		struct aecp_aem_get_audio_map_cmd_pdu *get_audio_map_cmd  = (struct aecp_aem_get_audio_map_cmd_pdu *)(pdu + 1);
		struct aecp_aem_get_audio_map_rsp_pdu *get_audio_map_rsp = (struct aecp_aem_get_audio_map_rsp_pdu *)(aecp_rsp + 1);
		u16 descriptor_type = ntohs(get_audio_map_cmd->descriptor_type);
		struct stream_port_descriptor *stream_port_desc;

		os_log(LOG_DEBUG, "aecp(%p) received command type GET_AUDIO_MAP (%x)\n", aecp, cmd_type);
		status = AECP_AEM_IN_PROGRESS;

		len += sizeof(struct aecp_aem_get_audio_map_rsp_pdu);
		os_memset(get_audio_map_rsp, 0, sizeof(struct aecp_aem_get_audio_map_rsp_pdu));

		get_audio_map_rsp->descriptor_type = get_audio_map_cmd->descriptor_type;
		get_audio_map_rsp->descriptor_index = get_audio_map_cmd->descriptor_index;
		get_audio_map_rsp->map_index = get_audio_map_cmd->map_index;

		if (descriptor_type != AEM_DESC_TYPE_STREAM_PORT_INPUT && descriptor_type != AEM_DESC_TYPE_STREAM_PORT_OUTPUT) {
			status = AECP_AEM_BAD_ARGUMENTS;
			break;
		}

		stream_port_desc = aem_get_descriptor(entity->aem_descs, descriptor_type, ntohs(get_audio_map_cmd->descriptor_index), NULL);
		if (!stream_port_desc) {
			status = AECP_AEM_NO_SUCH_DESCRIPTOR;
			break;
		}

		/* If STREAM_PORT has static mappings: return NOT_SUPPORTED. (Per AVNU.IO.CONTROL 7.3.26 for STREAM_PORT_OUTPUT) */
		if (ntohs(stream_port_desc->number_of_maps) > 0) {
			status = AECP_AEM_NOT_SUPPORTED;
			break;
		}

		/* Send AECP command to external control application */
		if (aecp_aem_ipc_tx_command(aecp, pdu, avtp_len, &entity->avdecc->ipc_tx_controlled, IPC_DST_ALL) < 0) {
			status = AECP_AEM_ENTITY_MISBEHAVING;
			break;
		}
		os_log(LOG_DEBUG, "aecp(%p) successfully sent AVB_MSG_AECP IPC command type (%x)\n", aecp, cmd_type);

		/* Add to application inflight list */
		rc = aecp_application_inflight_add(aecp, aecp_rsp, len, mac_src, port_id);
		if (rc != AECP_AEM_SUCCESS) {
			os_log(LOG_ERR, "aecp(%p) Could not add to application inflight\n", aecp);
			status = rc;
			break;
		}
		os_log(LOG_DEBUG, "aecp(%p) successfully added entity (%p) to application inflight list \n", aecp, entity);

		break;
	}
	case AECP_AEM_CMD_GET_AS_PATH:
	{
		struct aecp_aem_get_as_path_cmd_pdu *get_as_path_cmd  = (struct aecp_aem_get_as_path_cmd_pdu *)(pdu + 1);

		status = aecp_aem_get_as_path_response(entity, aecp_rsp, &len, ntohs(get_as_path_cmd->descriptor_index));

		break;
	}
	default:
		status = AECP_AEM_NOT_IMPLEMENTED;
		break;

	}

send_rsp:
	if (send_unsolicited_notification && aecp_need_sync_unsolicited_notifications(aecp, controller_entity_id)) {
		/* status is SUCCESS and need to send a synchronous unsolicited notification. */

		/* clone the desc_rsp and send the solicited notification/response before the unsolicited one */
		rc = aecp_aem_prepare_send_response(aecp, port_rsp, aecp_rsp, controller_entity_id,
											ntohs(aecp_rsp->sequence_id), status, 0, mac_src, len);
		if (rc < 0) {
			os_log(LOG_ERR, "aecp(%p) Could not prepare and send the solicited response for command (%x, %s)\n", aecp, cmd_type, aecp_aem_cmdtype2string(cmd_type));
			net_tx_free(desc_rsp);
			goto exit;
		}
		/* Send the original buffer as synchronous unsolicited notifications on commands that directly changed the PAAD-AE's state
		* Commands relying on external apps to change the PAAD-AE's state sends the response and unsolicited notifications in aecp_ipc_rx_controlled
		* Per AVNU.IO.CONTROL 7.5.2
		*/
		aecp_aem_send_sync_unsolicited_notification_full(aecp, desc_rsp, aecp_rsp, controller_entity_id, len);

	} else {
		/* Don't send response when expecting response from app first. */
		if (status != AECP_AEM_IN_PROGRESS)
			rc = aecp_aem_net_tx_response(aecp, port_rsp, desc_rsp, status, mac_src, len);
		else
			net_tx_free(desc_rsp);
	}

	unsolicited_entry = aecp_unsolicited_find(aecp, controller_entity_id, port_id);
	if (unsolicited_entry) {
		/* Restart monitor timer of the controller sending the (valid) command if he is registered. Per AVNU.IO.CONTROL 7.5.3 */
		os_log(LOG_DEBUG,"aecp(%p) port(%u) controller(%016"PRIx64") sent us an AECP command (%u), restarting departing monitor timer",
				aecp, port_id, ntohll(controller_entity_id), cmd_type);
		timer_restart(&unsolicited_entry->monitor_timer, MONITOR_TIMER_INTERVAL);
	}

exit:
	return rc;
}

/** Main AECP AEM receive function for entity's AECP command
 * Follows the AVDECC entity model state machine (9.2.2.3.1.4).
 * \return 	0 on success, negative otherwise
 * \param	aecp		pointer to the AECP context
 * \param	pdu			pointer to the AECP PDU
 * \param	avtp_len	length of the AVTP payload.
 * \param	mac_src		source MAC address of the received PDU
 * \param	port_id		avdecc port / interface index on which we received the PDU
 */
static int aecp_aem_received_entity_command(struct aecp_ctx *aecp, struct aecp_aem_pdu *pdu, u16 avtp_len, u8 *mac_src, unsigned int port_id)
{
	struct entity *entity = container_of(aecp, struct entity, aecp);
	struct avdecc_port *port_rsp = &entity->avdecc->port[port_id];
	struct aecp_aem_pdu *aecp_rsp = NULL;
	struct net_tx_desc *desc_rsp;
	u8 status;
	u16 cmd_type;
	u16 len	= sizeof(struct aecp_aem_pdu); //data size after AVTP control hdr
	int rc = 0;

	desc_rsp = aecp_net_tx_prepare(port_rsp, pdu, &len, (void **)&aecp_rsp); //FIXME check if we can re-use same buf
	if (!desc_rsp) {
		os_log(LOG_ERR, "aecp(%p) Cannot alloc tx descriptor\n", aecp);
		rc = -1;
		goto exit;
	}

	cmd_type = AECP_AEM_GET_CMD_TYPE(pdu);

	os_log(LOG_DEBUG, "aecp(%p) command (%x, %s) seq_id(%d)\n", aecp, cmd_type, aecp_aem_cmdtype2string(cmd_type), ntohs(pdu->sequence_id));

	switch (cmd_type) {
	case AECP_AEM_CMD_CONTROLLER_AVAILABLE:
	{
		status = AECP_AEM_SUCCESS;
		break;
	}
	default:
		status = AECP_AEM_NOT_IMPLEMENTED;
		break;
	}

	rc = aecp_aem_net_tx_response(aecp, port_rsp, desc_rsp, status, mac_src, len);

exit:
	return rc;
}

/** Handle normal and unsolicited AECP AEM responses coming from the entity over the network
 *
 * \return 0 on success or -1 on failure.
 * \param aecp		AECP context that received the response.
 * \param pdu		Pointer to received AECP AEM PDU.
 * \param status	Status field from the AECP packet (contained within the AVTP part).
 * \param len		Length of the received PDU.
 * \param port_id		avdecc port / interface index on which we received the PDU
 */
static int aecp_aem_received_entity_response(struct aecp_ctx *aecp, struct aecp_aem_pdu *pdu, u8 status, u16 len, unsigned int port_id)
{
	struct entity *entity = container_of(aecp, struct entity, aecp);
	struct avdecc_ctx *avdecc = entity->avdecc;
	struct ipc_tx *ipc =  &avdecc->ipc_tx_controller;
	unsigned int ipc_dst = IPC_DST_ALL;
	struct inflight_ctx *entry;
	int rc;

	if (!AECP_AEM_GET_U(pdu)) {
		entry = avdecc_inflight_find(&aecp->inflight_network, ntohs(pdu->sequence_id));
		if (entry) {
			ipc = (void *)entry->data.priv[0];
			ipc_dst = (unsigned int)entry->data.priv[1];

			// Handle IN_PROGRESS responses
			if (status == AECP_AEM_IN_PROGRESS) {
				avdecc_inflight_restart(entry);
				rc = 0;
				goto exit;
			}
			else
				avdecc_inflight_remove(entity, entry);

		} else {
			rc = -1;
			goto exit;
		}
	}

	if (ipc)
		rc = aecp_aem_ipc_tx_response(aecp, pdu, status, len, ipc, ipc_dst);
	else
		rc = -1;

exit:
	return rc;
}

/** Handle normal and unsolicited AECP AEM responses coming from the controller over the network
 *
 * \return 0 on success or -1 on failure.
 * \param aecp		AECP context that received the response.
 * \param pdu		Pointer to received AECP AEM PDU.
 * \param status	Status field from the AECP packet (contained within the AVTP part).
 * \param len		Length of the received PDU.
 * \param port_id		avdecc port / interface index on which we received the PDU
 */
static int aecp_aem_received_controller_response(struct aecp_ctx *aecp, struct aecp_aem_pdu *pdu, u8 status, u16 len, unsigned int port_id)
{
	struct entity *entity = container_of(aecp, struct entity, aecp);
	struct inflight_ctx *entry;
	int rc = 0;

	if (!AECP_AEM_GET_U(pdu)) {
		entry = avdecc_inflight_find(&aecp->inflight_network, ntohs(pdu->sequence_id));
		if (entry) {
			switch (AECP_AEM_GET_CMD_TYPE(pdu)) {
			case AECP_AEM_CMD_CONTROLLER_AVAILABLE:
			{
				struct unsolicited_ctx *unsolicited_entry;

				unsolicited_entry = aecp_unsolicited_find(aecp, pdu->entity_id, port_id);
				if (!unsolicited_entry) {
					os_log(LOG_ERR,"aecp(%p) port(%u) couldn't retrieve registered controller(%016"PRIx64").\n",
							aecp, port_id, ntohll(pdu->entity_id));

					break;
				}

				/* Restart monitor timer */
				timer_restart(&unsolicited_entry->monitor_timer, MONITOR_TIMER_INTERVAL);

				os_log(LOG_DEBUG,"aecp(%p) port(%u) controller(%016"PRIx64") RECEIVED CONTROLLER_AVAILABLE, restarting monitor timer.\n",
						aecp, port_id, ntohll(pdu->entity_id));

				break;
			}
			default:
				break;
			}

			avdecc_inflight_remove(entity, entry);

			rc = 0;
			goto exit;
		}
		else {
			rc = -1;
			goto exit;
		}
	}

exit:
	return rc;
}

__init unsigned int aecp_data_size(struct avdecc_entity_config *cfg)
{
	return cfg->max_unsolicited_registrations * sizeof(struct unsolicited_ctx);
}

__init int aecp_init_timers(struct entity *entity)
{
	struct avb_interface_dynamic_desc *avb_itf_dynamic;
	unsigned int num_interfaces;
	int i;

	num_interfaces = aem_get_descriptor_max(entity->aem_descs, AEM_DESC_TYPE_AVB_INTERFACE);
	for (i = 0; i < num_interfaces; i++) {
		avb_itf_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_AVB_INTERFACE, i, NULL);

		if (entity->milan_mode) {
			avb_itf_dynamic->u.milan.async_get_counters_unsolicited_notification_timer.func = aecp_milan_get_counters_async_unsolicited_notification_timer_handler;
			avb_itf_dynamic->u.milan.async_get_counters_unsolicited_notification_timer.data = avb_itf_dynamic;

			if (timer_create(entity->avdecc->timer_ctx, &avb_itf_dynamic->u.milan.async_get_counters_unsolicited_notification_timer, 0,
						AECP_MILAN_GET_COUNTERS_ASYNC_UNSOLICITED_NOTIFICATION_GRANULARITY_MS) < 0)
				goto err_counter_timer_create;
		}

		avb_itf_dynamic->async_get_as_path_unsolicited_notification_timer.func = aecp_get_as_path_async_unsolicited_notification_timer_handler;
		avb_itf_dynamic->async_get_as_path_unsolicited_notification_timer.data = avb_itf_dynamic;
		avb_itf_dynamic->async_get_as_path_unsolicited_notification_pending = false;
		avb_itf_dynamic->async_get_as_path_unsolicited_notification_timer_running = false;

		if (timer_create(entity->avdecc->timer_ctx, &avb_itf_dynamic->async_get_as_path_unsolicited_notification_timer, 0,
					AECP_GET_AS_PATH_ASYNC_UNSOLICITED_NOTIFICATION_GRANULARITY_MS) < 0)
			goto err_path_timer_create;
	}

	return 0;

err_path_timer_create:
	if (entity->milan_mode)
		timer_destroy(&avb_itf_dynamic->u.milan.async_get_counters_unsolicited_notification_timer);

err_counter_timer_create:
	while (i--) {
		avb_itf_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_AVB_INTERFACE, i, NULL);

		if (entity->milan_mode)
			timer_destroy(&avb_itf_dynamic->u.milan.async_get_counters_unsolicited_notification_timer);

		timer_destroy(&avb_itf_dynamic->async_get_as_path_unsolicited_notification_timer);
	}

	return -1;
}

__init int aecp_init(struct aecp_ctx *aecp, void *data, struct avdecc_entity_config *cfg)
{
	struct entity *entity = container_of(aecp, struct entity, aecp);

	list_head_init(&aecp->inflight_network);
	list_head_init(&aecp->inflight_application);

	aecp->max_unsolicited_registrations = cfg->max_unsolicited_registrations;
	aecp->unsolicited_storage = (struct unsolicited_ctx *)data;

	aecp_unsolicited_init(aecp);

	if (aecp_init_timers(entity) < 0)
		goto err_timer_init;

	os_log(LOG_INIT, "aecp(%p) done\n", aecp);

	return 0;

err_timer_init:
	return -1;
}

__exit static void aecp_exit_timers(struct aecp_ctx *aecp)
{
	struct entity *entity = container_of(aecp, struct entity, aecp);
	struct avb_interface_dynamic_desc *avb_itf_dynamic;
	unsigned int num_interfaces;
	int i;

	num_interfaces = aem_get_descriptor_max(entity->aem_descs, AEM_DESC_TYPE_AVB_INTERFACE);
	for (i = 0; i < num_interfaces; i++) {
		avb_itf_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_AVB_INTERFACE, i, NULL);

		if (entity->milan_mode)
			timer_destroy(&avb_itf_dynamic->u.milan.async_get_counters_unsolicited_notification_timer);

		timer_destroy(&avb_itf_dynamic->async_get_as_path_unsolicited_notification_timer);
	}
}

__exit int aecp_exit(struct aecp_ctx *aecp)
{
	aecp_exit_timers(aecp);

	os_log(LOG_INIT, "done\n");

	return 0;
}

/** Main AECP receive function.
 * \return 	0 on success, negative otherwise
 * \param	port		pointer to the AVDECC port
 * \param	pdu		pointer to the AECP PDU
 * \param	msg_type	AECP message type (9.2.1.1.5)
 * \param	status		AECP status (9.2.1.1.6)
 * \param 	len			length of the AECP PDU
 * \param	mac_src		source MAC address of the received PDU
 */
int aecp_net_rx(struct avdecc_port *port, struct aecp_pdu *pdu, u8 msg_type, u8 status, u16 len, u8 *mac_src)
{
	u64 controller_entity_id = pdu->controller_entity_id;
	u64 entity_id = pdu->entity_id;
	struct entity *entity;
	struct aecp_ctx *aecp = NULL;
	struct avdecc_ctx *avdecc = avdecc_port_to_context(port);

	os_log(LOG_DEBUG, "port(%u) AECP message len(%u) source mac(%012"PRIx64")\n", port->port_id, len, NTOH_MAC_VALUE(mac_src));

	switch(msg_type) {
	case AECP_AEM_COMMAND:
		entity = avdecc_get_entity(avdecc, entity_id);
		if (!entity || !avdecc_entity_port_valid(entity, port->port_id)) {
			os_log(LOG_ERR, "avdecc(%p) port(%u) aecp command does not match any local entity (message type(%d), entity(%"PRIx64"), controller(%"PRIx64")) \n",
					avdecc, port->port_id, msg_type, ntohll(entity_id), ntohll(controller_entity_id));
			goto exit;
		}
		aecp = &entity->aecp;

		if (AECP_AEM_GET_CMD_TYPE((struct aecp_aem_pdu *)pdu) == AECP_AEM_CMD_CONTROLLER_AVAILABLE) {
			/* Entity sent a CONTROLLER_AVAILABLE command to the controller */
			aecp_aem_received_entity_command(aecp, (struct aecp_aem_pdu *)pdu, len, mac_src, port->port_id);
		} else {
			/* Controller's commands sent to entity */
			aecp_aem_received_controller_command(aecp, (struct aecp_aem_pdu *)pdu, len, mac_src, port->port_id);
		}

		break;

	case AECP_AEM_RESPONSE:
		/* Response to command sent from the controller entity */
		entity = avdecc_get_entity(avdecc, controller_entity_id);

		if (!entity || !avdecc_entity_port_valid(entity, port->port_id)) {
			os_log(LOG_ERR, "avdecc(%p) port(%u) aecp response does not match any local controller (message type(%d), entity(%"PRIx64"), controller(%"PRIx64")) \n",
					avdecc, port->port_id, msg_type, ntohll(entity_id), ntohll(controller_entity_id));
			// TODO handle IDENTITY_NOTIFICATION responses
			goto exit;
		}

		aecp = &entity->aecp;

		if (AECP_AEM_GET_CMD_TYPE((struct aecp_aem_pdu *)pdu) == AECP_AEM_CMD_CONTROLLER_AVAILABLE) {
			/* Controller responded to entity's CONTROLLER_AVAILABLE command */
			aecp_aem_received_controller_response(aecp, (struct aecp_aem_pdu *)pdu, status, len, port->port_id);
		} else {
			/* Entity responded to controller's command */
			aecp_aem_received_entity_response(aecp, (struct aecp_aem_pdu *)pdu, status, len, port->port_id);
		}

		break;

	case AECP_VENDOR_UNIQUE_COMMAND:
		entity = avdecc_get_entity(avdecc, entity_id);
		if (!entity || !avdecc_entity_port_valid(entity, port->port_id)) {
			os_log(LOG_ERR, "avdecc(%p) port(%u) aecp vendor unique command does not match any local entity (message type(%d), entity(%"PRIx64"), controller(%"PRIx64")) \n",
					avdecc, port->port_id, msg_type, ntohll(entity_id), ntohll(controller_entity_id));
			goto exit;
		}

		aecp = &entity->aecp;
		aecp_vendor_specific_received_command(aecp, (struct aecp_vuf_pdu *)pdu, len, mac_src, port->port_id);
		break;

	default:
		os_log(LOG_ERR, "avdecc(%p) port(%u) aecp message type (%d) not supported\n", avdecc, port->port_id, msg_type);
		break;
	}

exit:
	debug_dump_aecp_aem(aecp, (struct aecp_aem_pdu *)pdu, msg_type, status);
	return 0;
}

/** Main AECP IPC receive function, for controller entities.
 * \return 0 on success or negative value otherwise.
 * \param entity	Controller entity the IPC was received for.
 * \param aecp_msg	Pointer to the received AECP message.
 * \param len		Length of the received IPC message payload.
 * \param ipc		IPC the message was received through.
 */
int aecp_ipc_rx_controller(struct entity *entity, struct ipc_aecp_msg *aecp_msg, u32 len, struct ipc_tx *ipc, unsigned int ipc_dst)
{
	struct aecp_aem_pdu *aecp_msg_pdu = (struct aecp_aem_pdu *)aecp_msg->buf;
	struct avdecc_ctx *avdecc = entity->avdecc;
	struct aecp_aem_pdu *aecp_cmd = NULL;
	struct net_tx_desc *tx_desc = NULL;
	struct entity_discovery *entity_disc;
	unsigned int num_interfaces;
	struct avdecc_port *port;
	u8 *mac_dst;
	u64 entity_id;
	int rc;

	os_log(LOG_DEBUG, "avdecc(%p) ipc_tx(%p) aecp_msg(%p) len(%u)\n", avdecc, ipc, aecp_msg, len);

	/* Only AEM messages are supported, so check that the provided message is big enough for at least the base AEM fields. */
	if (len < (offsetof(struct ipc_aecp_msg, buf) + sizeof(struct aecp_aem_pdu))) {
		os_log(LOG_ERR, "avdecc(%p) Invalid IPC AECP AEM message size (%u instead of at least %lu)\n",
			avdecc, len, offsetof(struct ipc_aecp_msg, buf) + sizeof(struct aecp_aem_pdu));
		rc = -1;
		goto exit;
	}

	debug_dump_aecp_aem(&entity->aecp, aecp_msg_pdu, aecp_msg->msg_type, aecp_msg->status);

	entity_id = aecp_msg_pdu->entity_id;

	/* Get number of supported interfaces for the controller entity. */
	num_interfaces = aem_get_descriptor_max(entity->aem_descs, AEM_DESC_TYPE_AVB_INTERFACE);

	entity_disc = adp_find_entity_discovery_any(avdecc, entity_id, num_interfaces);
	if (!entity_disc) {
		os_log(LOG_ERR, "avdecc(%p) Cannot send command, receiving entity(%"PRIx64") not visible on the network.\n",
			avdecc, htonll(aecp_msg_pdu->entity_id));
		rc = -1;
		goto exit;
	}

	/* Send command on the port on which we discovered the entity. */
	port = discovery_to_avdecc_port(entity_disc->disc);

	tx_desc = aecp_net_tx_prepare(port, aecp_msg->buf, &aecp_msg->len, (void **)&aecp_cmd);
	if (!tx_desc) {
		os_log(LOG_ERR, "avdecc(%p) Cannot alloc tx descriptor\n", avdecc);
		rc = -1;
		goto exit;
	}

	copy_64(&aecp_cmd->controller_entity_id, &entity->desc->entity_id);
	copy_64(&aecp_cmd->entity_id, &entity_disc->info.entity_id);
	mac_dst = entity_disc->info.mac_addr;

	rc = aecp_aem_send_command(&entity->aecp, port, aecp_cmd, tx_desc, mac_dst, aecp_msg->len, ipc, ipc_dst);
	if (rc < 0) {
		os_log(LOG_ERR, "avdecc(%p) Cannot send aecp command\n", avdecc);
		rc = -1;
		goto exit;
	}

exit:
	return rc;
}

void aecp_ipc_rx_controlled(struct entity *entity, struct ipc_aecp_msg *aecp_msg, u32 len)
{
	struct avdecc_ctx *avdecc = entity->avdecc;
	struct aecp_ctx *aecp = &entity->aecp;
	struct aecp_aem_pdu *aecp_rx_rsp;
	struct control_descriptor *ctrl_desc;
	struct inflight_ctx *entry = NULL;
	u64 inflight_controller_id = 0;
	u16 cmd_type;
	bool desc_changed = false;

	os_log(LOG_DEBUG, "entity(%p) aecp_msg(%p) len(%u)\n", entity, aecp_msg, len);

	/* Only AEM messages are supported, so check that the provided message is big enough for at least the base AEM fields. */
	if (len < (offsetof(struct ipc_aecp_msg, buf) + sizeof(struct aecp_aem_pdu))) {
		os_log(LOG_ERR, "avdecc(%p) Invalid IPC AECP AEM message size (%u instead of at least %lu)\n",
				avdecc, len, offsetof(struct ipc_aecp_msg, buf) + sizeof(struct aecp_aem_pdu));
		return;
	}

	aecp_rx_rsp = (struct aecp_aem_pdu *)aecp_msg->buf;

	debug_dump_aecp_aem(&entity->aecp, aecp_rx_rsp, aecp_msg->msg_type, aecp_msg->status);

	if (aecp_msg->msg_type != AECP_AEM_RESPONSE) {
		os_log(LOG_ERR, "avdecc(%p) Received message type %d but only AECP_AEM_RESPONSE allowed on channel\n", avdecc, aecp_msg->msg_type);
		return;
	}

	/* Get the in-flight entry if regular AECP response (Not Unsolicited)*/
	if (!AECP_AEM_GET_U(aecp_rx_rsp)) {
		entry = aem_inflight_find_controller(&aecp->inflight_application, ntohs(aecp_rx_rsp->sequence_id), aecp_rx_rsp->controller_entity_id);
		if (!entry) {
			os_log(LOG_ERR, "avdecc(%p) Received regular AECP response from application with sequence id %d,"
					"but no command was received with that sequence id.\n", avdecc, ntohs(aecp_rx_rsp->sequence_id));
			return;
		}
	}

	/* Update AEM structures only on successful responses
	 * FIXME rely on app to maintain descriptor values instead?
	 */

	cmd_type = AECP_AEM_GET_CMD_TYPE(aecp_rx_rsp);
	switch (cmd_type) {
	case AECP_AEM_CMD_SET_CONTROL:
	{
		struct aecp_aem_set_get_control_pdu *set_control_rsp;
		void *values_rsp;
		u16 values_len, values_len_max_rsp;
		int rc;

		if (len < (offsetof(struct ipc_aecp_msg, buf) + sizeof(struct aecp_aem_pdu) + sizeof(struct aecp_aem_set_get_control_pdu))) {
			os_log(LOG_ERR, "avdecc(%p) Invalid AEM SET_CONTROL message size (%u < %lu)\n",
					avdecc, len, offsetof(struct ipc_aecp_msg, buf) + sizeof(struct aecp_aem_pdu)
					+ sizeof(struct aecp_aem_set_get_control_pdu));
			return;
		}

		set_control_rsp  = (struct aecp_aem_set_get_control_pdu *)(aecp_rx_rsp + 1);
		values_rsp = set_control_rsp + 1;
		values_len = aecp_msg->len - sizeof(struct aecp_aem_pdu) - sizeof(struct aecp_aem_set_get_control_pdu);
		values_len_max_rsp = len - (offsetof(struct ipc_aecp_msg, buf) + sizeof(struct aecp_aem_pdu) + sizeof(struct aecp_aem_set_get_control_pdu));

		ctrl_desc = aem_get_descriptor(entity->aem_descs, ntohs(set_control_rsp->descriptor_type),
						ntohs(set_control_rsp->descriptor_index), NULL);

		if (!ctrl_desc) {
			os_log(LOG_ERR, "avdecc(%p) Control descriptor (type = %d, index = %d) reported by application not found.\n",
					avdecc, ntohs(set_control_rsp->descriptor_type), ntohs(set_control_rsp->descriptor_index));
			return;
		}

		if (aecp_msg->status == AECP_AEM_SUCCESS) {
			u16 new_values_len;
			void *new_values;

			/* On successful regular response: get values from the inflight (coming from the command)
			   and write it back into the response msg */
			if (!AECP_AEM_GET_U(aecp_rx_rsp)) {

				new_values_len = entry->data.len - (sizeof(struct aecp_aem_pdu) + sizeof(struct aecp_aem_set_get_control_pdu));
				new_values = entry->data.pdu.buf + sizeof(struct aecp_aem_pdu) + sizeof(struct aecp_aem_set_get_control_pdu);
				/* Check if we have enough space in the response msg*/
				if (values_len_max_rsp < new_values_len) {
					os_log(LOG_ERR, "avdecc(%p) Not enough space for values in the IPC AECP AEM message"
								"(%u instead of at least %u)\n",
								avdecc, values_len_max_rsp, new_values_len);
					return;
				} else {
					/* Copy the new value and update the AECP msg length */
					os_memcpy(values_rsp, new_values, new_values_len);
					aecp_msg->len = entry->data.len;
				}
			} else {
				/* On unsolicited notification, take the value coming from response msg */
				new_values_len = values_len;
				new_values = values_rsp;
			}

			/* Validate and copy values to descriptor */
			rc = aecp_aem_control_pdu_to_desc(ctrl_desc, new_values, new_values_len);

			if (rc < 0) {
				os_log(LOG_ERR, "avdecc(%p) Application reported invalid Control value for descriptor %d\n",
						avdecc, ntohs(set_control_rsp->descriptor_index));
				return;
			} else if (rc > 0) {
				/* Value changed: send an unsolicited notification later*/
				desc_changed = true;

			}
		} else {
			/* On failed regular response: the response msg should contain the current value in the descriptor*/
			if (!AECP_AEM_GET_U(aecp_rx_rsp)) {

				rc = aecp_aem_control_desc_to_pdu(ctrl_desc, values_rsp, values_len_max_rsp);
				if (rc < 0) {
					os_log(LOG_ERR, "avdecc(%p) Cannot copy control descriptor values to IPC AECP AEM message\n", avdecc);
					return;
				}

				aecp_msg->len = rc + sizeof(struct aecp_aem_pdu) + sizeof(struct aecp_aem_set_get_control_pdu);
			} else {
				/* Unsolicited notification with error status have no meaning*/
				os_log(LOG_ERR, "avdecc(%p) Unsolicited notifications can not have error status.\n", avdecc);
				return;
			}
		}
		break;
	}
	case AECP_AEM_CMD_START_STREAMING:
	{
		struct aecp_aem_start_streaming_cmd_pdu *start_streaming_rsp;
		int rc = 0;

		if (len < (offsetof(struct ipc_aecp_msg, buf) + sizeof(struct aecp_aem_pdu) + sizeof(struct aecp_aem_start_streaming_cmd_pdu))) {
			os_log(LOG_ERR, "avdecc(%p) Invalid AEM START_STREAMING message size (%u < %lu)\n",
					avdecc, len, offsetof(struct ipc_aecp_msg, buf) + sizeof(struct aecp_aem_pdu)
					+ sizeof(struct aecp_aem_start_streaming_cmd_pdu));
			return;
		}

		start_streaming_rsp = (struct aecp_aem_start_streaming_cmd_pdu *)(aecp_rx_rsp + 1);

		if (aecp_msg->status == AECP_AEM_SUCCESS) {
			rc = acmp_start_streaming(entity, ntohs(start_streaming_rsp->descriptor_type), ntohs(start_streaming_rsp->descriptor_index));
			if (rc < 0) {
				os_log(LOG_ERR, "avdecc(%p) Cannot start stream and update binding parameters\n", avdecc);
				return;
			}

			/* Value changed: send an unsolicited notification later*/
			if (rc > 0)
				desc_changed = true;
		}

		break;
	}
	case AECP_AEM_CMD_STOP_STREAMING:
	{
		struct aecp_aem_stop_streaming_cmd_pdu *stop_streaming_rsp;
		int rc = 0;

		if (len < (offsetof(struct ipc_aecp_msg, buf) + sizeof(struct aecp_aem_pdu) + sizeof(struct aecp_aem_stop_streaming_cmd_pdu))) {
			os_log(LOG_ERR, "avdecc(%p) Invalid AEM STOP_STREAMING message size (%u < %lu)\n",
					avdecc, len, offsetof(struct ipc_aecp_msg, buf) + sizeof(struct aecp_aem_pdu)
					+ sizeof(struct aecp_aem_stop_streaming_cmd_pdu));
			return;
		}

		stop_streaming_rsp = (struct aecp_aem_stop_streaming_cmd_pdu *)(aecp_rx_rsp + 1);

		if (aecp_msg->status == AECP_AEM_SUCCESS) {
			rc = acmp_stop_streaming(entity, ntohs(stop_streaming_rsp->descriptor_type), ntohs(stop_streaming_rsp->descriptor_index));
			if (rc < 0) {
				os_log(LOG_ERR, "avdecc(%p) Cannot stop stream and update binding parameters\n", avdecc);
				return;
			}

			/* Value changed: send an unsolicited notification later*/
			if (rc > 0)
				desc_changed = true;
		}

		break;
	}
	case AECP_AEM_CMD_REMOVE_AUDIO_MAPPINGS:
	case AECP_AEM_CMD_ADD_AUDIO_MAPPINGS:
	{
		if (len < (offsetof(struct ipc_aecp_msg, buf) + sizeof(struct aecp_aem_pdu) + sizeof(struct aecp_aem_modify_audio_mappings_cmd_pdu))) {
			os_log(LOG_ERR, "avdecc(%p) Invalid AEM ADD/REMOVE_AUDIO_MAPPINGS message size (%u < %lu)\n",
					avdecc, len, offsetof(struct ipc_aecp_msg, buf) + sizeof(struct aecp_aem_pdu)
					+ sizeof(struct aecp_aem_modify_audio_mappings_cmd_pdu));
			return;
		}

		if (aecp_msg->status == AECP_AEM_SUCCESS) {
			desc_changed = true;
		}

		break;
	}
	case AECP_AEM_CMD_GET_AUDIO_MAP:
	{
		if (len < (offsetof(struct ipc_aecp_msg, buf) + sizeof(struct aecp_aem_pdu) + sizeof(struct aecp_aem_get_audio_map_rsp_pdu))) {
			os_log(LOG_ERR, "avdecc(%p) Invalid AEM GET_AUDIO_MAP message size (%u < %lu)\n",
					avdecc, len, offsetof(struct ipc_aecp_msg, buf) + sizeof(struct aecp_aem_pdu)
					+ sizeof(struct aecp_aem_get_audio_map_rsp_pdu));
			return;
		}

		break;
	}
	default:
		break;
	}

	// Respond to the in-flight command if present
	if (entry) {
		struct avdecc_port *port = &avdecc->port[entry->data.port_id];

		inflight_controller_id = entry->data.pdu.aem.controller_entity_id;

		if (aecp_aem_prepare_send_response(aecp, port, aecp_msg->buf, inflight_controller_id, ntohs(aecp_rx_rsp->sequence_id), aecp_msg->status, 0,
							entry->data.mac_dst, aecp_msg->len) < 0) {
			os_log(LOG_ERR, "avdecc(%p) port(%u) couldn't send response back to requesting controller(%016"PRIx64")\n",
				avdecc, port->port_id, ntohll(inflight_controller_id));
			avdecc_inflight_remove(entity, entry);
			return;
		}

		avdecc_inflight_remove(entity, entry);
	}

	if ((aecp_msg->status == AECP_AEM_SUCCESS) && desc_changed)
	{
		switch (cmd_type) {
		case AECP_AEM_CMD_WRITE_DESCRIPTOR:
		case AECP_AEM_CMD_SET_CONFIGURATION:
		case AECP_AEM_CMD_SET_STREAM_FORMAT:
		case AECP_AEM_CMD_SET_VIDEO_FORMAT:
		case AECP_AEM_CMD_SET_SENSOR_FORMAT:
		case AECP_AEM_CMD_SET_STREAM_INFO:
		case AECP_AEM_CMD_SET_NAME:
		case AECP_AEM_CMD_SET_ASSOCIATION_ID:
		case AECP_AEM_CMD_SET_SAMPLING_RATE:
		case AECP_AEM_CMD_SET_CLOCK_SOURCE:
		case AECP_AEM_CMD_SET_CONTROL:
		case AECP_AEM_CMD_INCREMENT_CONTROL:
		case AECP_AEM_CMD_DECREMENT_CONTROL:
		case AECP_AEM_CMD_SET_SIGNAL_SELECTOR:
		case AECP_AEM_CMD_SET_MIXER:
		case AECP_AEM_CMD_SET_MATRIX:
		case AECP_AEM_CMD_START_STREAMING:
		case AECP_AEM_CMD_STOP_STREAMING:
		case AECP_AEM_CMD_REBOOT:
		case AECP_AEM_CMD_ADD_AUDIO_MAPPINGS:
		case AECP_AEM_CMD_REMOVE_AUDIO_MAPPINGS:
		case AECP_AEM_CMD_ADD_VIDEO_MAPPINGS:
		case AECP_AEM_CMD_REMOVE_VIDEO_MAPPINGS:
		case AECP_AEM_CMD_ADD_SENSOR_MAPPINGS:
		case AECP_AEM_CMD_REMOVE_SENSOR_MAPPINGS:
		{
			// TODO check for acquired, send to controller ( if != from previous)
			aecp_aem_send_sync_unsolicited_notification(aecp, (struct aecp_aem_pdu *)aecp_msg->buf, inflight_controller_id, aecp_msg->len);

			break;
		}
		default:
			break;
		}
	}
}
