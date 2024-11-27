/*
* Copyright 2014 Freescale Semiconductor, Inc.
* Copyright 2020-2024 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @brief ACMP IEEE 1722.1 code
 @details Handles ACMP IEEE 1722.1 stack
*/

#include "os/stdlib.h"

#include "common/log.h"
#include "common/ether.h"
#include "common/hash.h"

#include "acmp.h"
#include "avdecc.h"
#include "acmp_ieee.h"


static const u8 avtp_base_dst_mac[6] = MC_ADDR_MAAP_BASE;

int acmp_ieee_get_command_timeout_ms(acmp_message_type_t msg_type)
{
	switch (msg_type) {
	case ACMP_CONNECT_TX_COMMAND:
		return 2000;
	case ACMP_CONNECT_RX_COMMAND:
		return 4500;
	case ACMP_DISCONNECT_RX_COMMAND:
		return 500;
	case ACMP_DISCONNECT_TX_COMMAND:
	case ACMP_GET_TX_STATE_COMMAND:
	case ACMP_GET_RX_STATE_COMMAND:
	case ACMP_GET_TX_CONNECTION_COMMAND:
		return 200;
	default:
		return -1;
	}
}

const char *acmp_ieee_msgtype2string(acmp_message_type_t msg_type)
{
	switch (msg_type) {
	case2str(ACMP_CONNECT_TX_COMMAND);
	case2str(ACMP_CONNECT_TX_RESPONSE);
	case2str(ACMP_DISCONNECT_TX_COMMAND);
	case2str(ACMP_DISCONNECT_TX_RESPONSE);
	case2str(ACMP_GET_TX_STATE_COMMAND);
	case2str(ACMP_GET_TX_STATE_RESPONSE);
	case2str(ACMP_CONNECT_RX_COMMAND);
	case2str(ACMP_CONNECT_RX_RESPONSE);
	case2str(ACMP_DISCONNECT_RX_COMMAND);
	case2str(ACMP_DISCONNECT_RX_RESPONSE);
	case2str(ACMP_GET_RX_STATE_COMMAND);
	case2str(ACMP_GET_RX_STATE_RESPONSE);
	case2str(ACMP_GET_TX_CONNECTION_COMMAND);
	case2str(ACMP_GET_TX_CONNECTION_RESPONSE);
	default:
		return (char *) "Unknown ACMP 1722.1 message type";
	}
}

/** Copy listener stream info params to PDU
 * \return                     none
 * \param pdu                  pointer to the ACMP PDU
 * \param listener_info        pointer to the listener_stream_info structure
 */
static void acmp_ieee_listener_copy_info(struct acmp_pdu *pdu, struct listener_stream_info *listener_info)
{
	acmp_listener_copy_common_params(pdu, listener_info);

	if (listener_info->u.ieee.connected)
		pdu->connection_count = htons(1);
	else
		pdu->connection_count = htons(0);

}

/** Connects the AVDECC listener to a stream (8.2.2.5.2.6).
 * Sends an indication to AVTP / media application and initiates the SRP
 * listener registration.
 * \return	ACMP status
 * \param entity	pointer to the entity context
 * \param listener_info	pointer to the stream information context
 * \param pdu		pointer to the ACMP PDU
 */
static u8 acmp_ieee_listener_connect(struct entity *entity, struct listener_stream_info *listener_info, struct acmp_pdu *pdu)
{
	u8 rc = ACMP_STAT_SUCCESS;

	/*
	 * Already connected, success.
	 */
	if (listener_info->u.ieee.connected)
		goto exit;

	/* Save Listener stream info parameters */
	copy_64(&listener_info->controller_entity_id, &pdu->controller_entity_id);
	listener_info->flags = pdu->flags;
	os_memcpy(listener_info->stream_dest_mac, pdu->stream_dest_mac, 6);
	copy_64(&listener_info->stream_id, &pdu->stream_id);
	listener_info->stream_vlan_id = pdu->stream_vlan_id;
	copy_64(&listener_info->talker_entity_id, &pdu->talker_entity_id);
	listener_info->talker_unique_id = pdu->talker_unique_id;

	rc = acmp_listener_stack_connect(entity, ntohs(pdu->listener_unique_id), ntohs(pdu->flags));
	if (rc != ACMP_STAT_SUCCESS)
		goto exit;

	listener_info->u.ieee.connected = 1;

	os_log(LOG_INFO, "controller(%016"PRIx64") stream_id(%016"PRIx64") talker(%016"PRIx64", %u)\n",
		ntohll(listener_info->controller_entity_id), ntohll(listener_info->stream_id),
		ntohll(listener_info->talker_entity_id), ntohs(listener_info->talker_unique_id));

	/* FIXME
	if (acmp->fast_connect) {
		save current connection to a file
	}
	FIXME */
exit:
	return rc;
}

/** Disconnects the stream from the AVDECC listener (8.2.2.5.2.7).
 * Send an indication to AVTP / media application and initiates the SRP
 * listener de-registration.
 * \return	ACMP status
 * \param entity	pointer to the entity context
 * \param listener_info	pointer to the stream information context
 * \param unique_id	listener unique ID
 */
static u8 acmp_ieee_listener_disconnect(struct entity *entity, struct listener_stream_info *listener_info, u16 unique_id)
{
	u8 rc;

	rc = acmp_listener_stack_disconnect(entity, unique_id);

	// what happens if we fail the disconnect ? still connected ?
	listener_info->u.ieee.connected = 0;
	listener_info->flags = 0;

	return rc;
}

/** Returns true if the listener is connected to another stream source as provided
 * in the ACMP PDU or if it is not connected (8.2.2.5.2.2).
 * \return	0 or 1
 * \param listener_info	pointer to the stream information context
 * \param pdu		pointer to the ACMP PDU
 */
static int acmp_ieee_listener_connected(struct listener_stream_info *listener_info, struct acmp_pdu *pdu)
{
	if (listener_info->u.ieee.connected) {
		if (cmp_64(&listener_info->talker_entity_id, &pdu->talker_entity_id)
		&& listener_info->talker_unique_id == pdu->talker_unique_id)
			return 0;
		else
			return 1;
	}
	else
		return 0;
}

/** Returns true if the listener is connected to the stream source as provided
 * in the ACMP PDU (8.2.2.5.2.3).
 * \return	0 or 1
 * \param listener_info	pointer to the stream information context
 * \param pdu		pointer to the ACMP PDU
 */
static int acmp_ieee_listener_connected_to(struct listener_stream_info *listener_info, struct acmp_pdu *pdu)
{
	if ((listener_info->u.ieee.connected) && cmp_64(&listener_info->talker_entity_id, &pdu->talker_entity_id)
	&& listener_info->talker_unique_id == pdu->talker_unique_id)
		return 1;
	else
		return 0;
}

/** Helper function to check if a stream input or output is running
 * \return bool, true if stream is running, false otherwise
 * \param entity
 * \param stream_desc_type, AEM_DESC_TYPE_STREAM_INPUT or AEM_DESC_TYPE_STREAM_OUTPUT
 * \param stream_desc_index, index of the descriptor
 */
bool acmp_ieee_is_stream_running(struct entity *entity, u16 stream_desc_type, u16 stream_desc_index)
{
	bool ret = false;
	void *desc;

	if (stream_desc_type != AEM_DESC_TYPE_STREAM_INPUT && stream_desc_type != AEM_DESC_TYPE_STREAM_OUTPUT) {
		os_log(LOG_ERR, "entity(%p) descriptor type (%u) not supported\n", entity, stream_desc_type);
		goto out;
	}

	desc = aem_get_descriptor(entity->aem_dynamic_descs, stream_desc_type, stream_desc_index, NULL);
	if (!desc) {
		os_log(LOG_ERR, "entity(%p) stream descriptor(%u, %u) not found.\n", entity, stream_desc_type, stream_desc_index);
		goto out;
	}

	if (stream_desc_type == AEM_DESC_TYPE_STREAM_INPUT) {
		ret = ((struct stream_input_dynamic_desc *)desc)->u.ieee.connected;
	} else if (stream_desc_type == AEM_DESC_TYPE_STREAM_OUTPUT) {
		ret = (((struct stream_output_dynamic_desc *)desc)->u.ieee.connection_count > 0);
	}

out:
	return ret;
}

/** For demo purposes.
 * Disconnects the listener if its talker departed.
 * \return	none
 * \param acmp		ACMP context
 * \param entity_id	pointer to 64 bits talker entity ID
 */
void acmp_ieee_listener_talker_left(struct acmp_ctx *acmp, u64 entity_id)
{
	struct entity *entity = container_of(acmp, struct entity, acmp);
	struct listener_stream_info *listener_info;
	int i;

	if (entity->flags & AVDECC_FAST_CONNECT_MODE) {
		for (i = 0; i < acmp->max_listener_streams; i++) {
			listener_info = &acmp->u.ieee.listener_info[i];

			if ((listener_info->u.ieee.connected && (listener_info->u.ieee.flags_priv & ACMP_LISTENER_FL_FAST_CONNECT))
			&& cmp_64(&entity_id, &listener_info->talker_entity_id)) {
				acmp_ieee_listener_disconnect(entity, listener_info, i);

				if (listener_info->u.ieee.flags_priv & ACMP_LISTENER_FL_FAST_CONNECT_BTB)
					listener_info->u.ieee.flags_priv &= ~ACMP_LISTENER_FL_FAST_CONNECT_PENDING;
			}
		}
	}
}

/** Used for back-to-back demo configuration
 * Configures a saved state into the listener_info context with the provided
 * talker entity ID.
 * \return	none
 * \param acmp		ACMP context
 * \param entity_id	pointer to 64 bits talker entity ID
 * \param port_id	port on which the talker entity was discovered
 */
void acmp_ieee_listener_fast_connect_btb(struct acmp_ctx *acmp, u64 entity_id, unsigned int port_id)
{
	struct entity *entity = container_of(acmp, struct entity, acmp);
	struct listener_stream_info *listener_info;
	struct stream_descriptor *stream_input;
	int i;

	if ((entity->flags & (AVDECC_FAST_CONNECT_MODE | AVDECC_FAST_CONNECT_BTB)) != (AVDECC_FAST_CONNECT_MODE | AVDECC_FAST_CONNECT_BTB))
		return;

	/* Search for streams enabled for fast connect back to back, not connected and missing talker information */
	for (i = 0; i < acmp->max_listener_streams; i++) {
		stream_input = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_STREAM_INPUT, i, NULL);

		if (port_id != ntohs(stream_input->avb_interface_index))
			continue;

		listener_info = &acmp->u.ieee.listener_info[i];

		if (!(listener_info->u.ieee.flags_priv & ACMP_LISTENER_FL_FAST_CONNECT_BTB))
			continue;

		if (listener_info->u.ieee.connected)
			continue;

		if (listener_info->u.ieee.flags_priv & ACMP_LISTENER_FL_FAST_CONNECT_PENDING)
			continue;

		copy_64(&listener_info->talker_entity_id, &entity_id);
		listener_info->u.ieee.flags_priv |= ACMP_LISTENER_FL_FAST_CONNECT_PENDING;

		os_log(LOG_INIT, "acmp(%p) has a saved state for listener(%016"PRIx64", %u) -> talker(%016"PRIx64", %u)\n",
			acmp, ntohll(entity->desc->entity_id), i,
			ntohll(listener_info->talker_entity_id), ntohs(listener_info->talker_unique_id));
	}
}

/** Fast-connects to the provided entity ID (8.2.2.1.1).
 * Parses the list of listener_info contexts and check if a connection was previously
 * saved and matches the provided talker entity ID. On success, try to connect
 * directly to the talker.
 * \return		none
 * \param acmp		ACMP context
 * \param entity_id	pointer to 64 bits talker entity ID
 * \param port_id	port on which to try the connection.
 */
void acmp_ieee_listener_fast_connect(struct acmp_ctx *acmp, u64 entity_id, unsigned int port_id)
{
	struct entity *entity = container_of(acmp, struct entity, acmp);
	struct avdecc_ctx *avdecc = entity->avdecc;
	struct listener_stream_info *listener_info;
	struct net_tx_desc *desc_rsp;
	struct acmp_pdu *acmp_cmd;
	struct stream_descriptor *stream_input;
	void *buf_rsp;
	struct avdecc_port *port;
	int i;

	if (!(entity->flags & AVDECC_FAST_CONNECT_MODE))
		return;

	/* Search for streams enabled for fast connect, not connected and with full talker information */
	for (i = 0; i < acmp->max_listener_streams; i++) {
		listener_info = &acmp->u.ieee.listener_info[i];

		if (!(listener_info->u.ieee.flags_priv & ACMP_LISTENER_FL_FAST_CONNECT))
			continue;

		if (listener_info->u.ieee.connected)
			continue;

		if (!(listener_info->u.ieee.flags_priv & ACMP_LISTENER_FL_FAST_CONNECT_PENDING))
			continue;

		stream_input = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_STREAM_INPUT, i, NULL);

		if (entity_id == listener_info->talker_entity_id && port_id == ntohs(stream_input->avb_interface_index)) {
			port = &avdecc->port[port_id];

			desc_rsp = net_tx_alloc(&port->net_tx, ACMP_NET_DATA_SIZE);
			if (!desc_rsp) {
				os_log(LOG_ERR, "acmp(%p) Cannot alloc tx descriptor\n", acmp);
				return;
			}

			buf_rsp = NET_DATA_START(desc_rsp);
			acmp_cmd = (struct acmp_pdu *)((char *)buf_rsp + OFFSET_TO_ACMP);

			os_memset(acmp_cmd, 0 , sizeof(struct acmp_pdu));
			copy_64(&acmp_cmd->listener_entity_id, &entity->desc->entity_id);
			acmp_cmd->listener_unique_id = htons(i);
			copy_64(&acmp_cmd->controller_entity_id, &listener_info->controller_entity_id);

			acmp_ieee_listener_copy_info(acmp_cmd, listener_info);
			acmp_cmd->flags |= htons(ACMP_FLAG_FAST_CONNECT/* | ACMP_FLAG_CLASS_B */);

			if (acmp_send_cmd(acmp, port, acmp_cmd, desc_rsp, ACMP_CONNECT_TX_COMMAND, 0, acmp->sequence_id, NULL, 0) < 0)
				os_log(LOG_ERR, "acmp(%p) Cannot send fast-connect command\n", acmp);
		}
	}
}

__init static void acmp_ieee_listener_fast_connect_init(struct acmp_ctx *acmp, struct avdecc_entity_config *cfg)
{
	struct entity *entity = container_of(acmp, struct entity, acmp);
	struct listener_stream_info *listener_info;
	int i;

	if (cfg->flags & AVDECC_FAST_CONNECT_MODE) {
		os_log(LOG_INIT, "acmp(%p) fast-connect enabled\n", acmp);

		/* Provide default fast connect settings for first stream only */
		if (!cfg->talker_unique_id_n) {
			cfg->talker_unique_id_n = 1;
			cfg->talker_unique_id[0] = 0;
		}

		if (!cfg->listener_unique_id_n) {
			cfg->listener_unique_id_n = 1;
			cfg->listener_unique_id[0] = 0;
		}

		/* Init with user configuration */
		for (i = 0; (i < cfg->listener_unique_id_n) && (i < cfg->talker_unique_id_n); i++) {
			if (cfg->listener_unique_id[i] < acmp->max_listener_streams) {
				listener_info = &acmp->u.ieee.listener_info[cfg->listener_unique_id[i]];

				listener_info->talker_unique_id = htons(cfg->talker_unique_id[i]);
				listener_info->u.ieee.flags_priv |= ACMP_LISTENER_FL_FAST_CONNECT;

				if (i < cfg->talker_entity_id_n) {
					listener_info->talker_entity_id = htonll(cfg->talker_entity_id[i]);
					listener_info->u.ieee.flags_priv |= ACMP_LISTENER_FL_FAST_CONNECT_PENDING;
				} else if (cfg->flags & AVDECC_FAST_CONNECT_BTB) {
					listener_info->u.ieee.flags_priv |= ACMP_LISTENER_FL_FAST_CONNECT_BTB;
				}
			}
		}

		for (i = 0; i < acmp->max_listener_streams; i++) {
			listener_info = &acmp->u.ieee.listener_info[i];

			if (listener_info->u.ieee.flags_priv & ACMP_LISTENER_FL_FAST_CONNECT_PENDING)
				os_log(LOG_INIT, "acmp(%p) has a saved state for listener(%016"PRIx64", %u) -> talker(%016"PRIx64", %u)\n",
					acmp, ntohll(entity->desc->entity_id), i,
					ntohll(listener_info->talker_entity_id), ntohs(listener_info->talker_unique_id));
		}
	}
}

/** Fast disconnect listener streams (8.2.2.1.2).
 *
 * Parses the list of listener_info contexts and checks if they are currently connected,
 * in which case a disconnect command is sent to the talker entity.
 * The function doesn't follow the specification exactly, we should wait for a talker
 * response and retransmit the command in case of timeout.
 *
 * \return	none
 * \param acmp		ACMP context
 */
static void acmp_ieee_listener_fast_disconnect(struct acmp_ctx *acmp)
{
	struct entity *entity = container_of(acmp, struct entity, acmp);
	struct avdecc_ctx *avdecc = entity->avdecc;
	void *buf_rsp;
	struct net_tx_desc *desc_rsp;
	struct acmp_pdu *acmp_rsp;
	struct listener_stream_info *listener_info;
	struct avdecc_port *port;
	struct stream_descriptor *stream_input;
	int i;

	for (i = 0; i < acmp->max_listener_streams; i++) {
		listener_info = &acmp->u.ieee.listener_info[i];

		if (!listener_info->u.ieee.connected)
			continue;

		stream_input = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_STREAM_INPUT, i, NULL);
		port = &avdecc->port[ntohs(stream_input->avb_interface_index)];

		/* AVTP disconnect and SRP deregister. */
		acmp_ieee_listener_disconnect(entity, listener_info, i);

		desc_rsp = net_tx_alloc(&port->net_tx, ACMP_NET_DATA_SIZE);
		if (!desc_rsp) {
			os_log(LOG_ERR, "acmp(%p) Cannot alloc tx descriptor\n", acmp);

			break;
		}

		buf_rsp = NET_DATA_START(desc_rsp);
		acmp_rsp = (struct acmp_pdu *)((char *)buf_rsp + OFFSET_TO_ACMP);

		os_memset(acmp_rsp, 0 , sizeof(struct acmp_pdu));
		copy_64(&acmp_rsp->controller_entity_id, &listener_info->controller_entity_id);
		copy_64(&acmp_rsp->talker_entity_id, &listener_info->talker_entity_id);
		acmp_rsp->talker_unique_id = listener_info->talker_unique_id;
		copy_64(&acmp_rsp->listener_entity_id, &entity->desc->entity_id);
		acmp_rsp->listener_unique_id = htons(i);

		acmp_send_cmd(acmp, port, acmp_rsp, desc_rsp, ACMP_DISCONNECT_TX_COMMAND, 0, 0, NULL, 0);
	}
}

/** Copy talker stream info params to PDU
 * \return                     none
 * \param pdu                  pointer to the ACMP PDU
 * \param stream_info          pointer to the talker_stream_info structure
 */
static void acmp_ieee_talker_copy_stream_info(struct acmp_pdu *pdu, struct talker_stream_info *stream_info)
{
	/* Do not provide any stream info if it is not created */
	if (stream_info->u.ieee.connection_count) {
		acmp_talker_copy_common_params(pdu, stream_info);
		pdu->connection_count = htons(stream_info->u.ieee.connection_count);
	}
}

/** Checks if the listener information from the ACMP PDU is connected to the provided
 * talker stream (stream_info).
 * \return	listener_pair pointer if connected, NULL otherwise
 * \param acmp		ACMP context
 * \param stream_info	pointer to the talker stream context
 * \param pdu		pointer to the ACMP PDU
 */
static struct listener_pair *acmp_ieee_talker_connected_to(struct acmp_ctx *acmp, struct talker_stream_info *stream_info, struct acmp_pdu *pdu)
{
	int i;

	for (i = 0; i < acmp->u.ieee.max_listener_pairs; i++) {
		if (stream_info->u.ieee.listeners[i].connected
		&& cmp_64(&stream_info->u.ieee.listeners[i].listener_entity_id, &pdu->listener_entity_id)
		&& stream_info->u.ieee.listeners[i].listener_unique_id == pdu->listener_unique_id)
			return &stream_info->u.ieee.listeners[i];
	}
	return NULL;
}

/** Get a free listener_pair struct from talker stream context
 * talker stream (stream_info).
 * \return	listener_pair pointer if available, NULL otherwise
 * \param acmp		ACMP context
 * \param stream_info	pointer to the talker stream context
 */
static struct listener_pair *acmp_ieee_talker_get_free_listener(struct acmp_ctx *acmp, struct talker_stream_info *stream_info)
{
	int i;

	if (stream_info->u.ieee.connection_count >= acmp->u.ieee.max_listener_pairs)
		return NULL;

	for (i = 0; i < acmp->u.ieee.max_listener_pairs; i++)
		if (!stream_info->u.ieee.listeners[i].connected)
			return &stream_info->u.ieee.listeners[i];

	return NULL;
}

/** Connects a stream to an AVDECC listener (8.2.2.6.2.2).
 * If this is the first listener, SRP talker registration is requested.
 * \return              ACMP status
 * \param entity        pointer to the entity context
 * \param stream_info   pointer to the stream information context
 * \param pdu           pointer to the ACMP PDU
 */
static u8 acmp_ieee_talker_connect(struct entity *entity, struct talker_stream_info *stream_info, struct acmp_pdu *pdu)
{
	struct stream_output_dynamic_desc *stream_output_dynamic;
	u16 talker_unique_id = ntohs(pdu->talker_unique_id);
	struct avdecc_ctx *avdecc = entity->avdecc;
	struct avb_interface_descriptor *avb_itf;
	struct stream_descriptor *stream_output;
	struct listener_pair *listener;
	u8 rc = ACMP_STAT_SUCCESS;
	sr_class_t req_class;
	unsigned int port_id;

	listener = acmp_ieee_talker_get_free_listener(&entity->acmp, stream_info);
	if (!listener) {
		os_log(LOG_ERR, "avdecc(%p) reached maximum number of listeners\n", avdecc);
		rc = ACMP_STAT_TALKER_MISBEHAVING;
		goto exit;
	}

	stream_output = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_STREAM_OUTPUT, talker_unique_id, NULL);
	if (!stream_output) {
		rc = ACMP_STAT_TALKER_MISBEHAVING;
		goto exit;
	}

	stream_output_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_OUTPUT, talker_unique_id, NULL);
	if (!stream_output_dynamic) {
		rc = ACMP_STAT_TALKER_MISBEHAVING;
		goto exit;
	}

	port_id = ntohs(stream_output->avb_interface_index);

	avb_itf = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_AVB_INTERFACE, port_id, NULL);
	if (!avb_itf) {
		rc = ACMP_STAT_TALKER_MISBEHAVING;
		goto exit;
	}

	if (pdu->flags & htons(ACMP_FLAG_CLASS_B))
		req_class = SR_CLASS_B;
	else
		req_class = SR_CLASS_A;

	/* Stream class check */
	if (req_class == SR_CLASS_B) {
		if(!(stream_output->stream_flags & htons(AEM_STREAM_FLAG_CLASS_B))) {
			os_log(LOG_ERR, "avdecc(%p) incompatible request from listener(%016"PRIx64")\n", avdecc, ntohll(pdu->listener_entity_id));
			rc = ACMP_STAT_INCOMPATIBLE_REQUEST;
			goto exit;
		}
	} else {
		if(!(stream_output->stream_flags & htons(AEM_STREAM_FLAG_CLASS_A))) {
			os_log(LOG_ERR, "avdecc(%p) incompatible request from listener(%016"PRIx64")\n", avdecc, ntohll(pdu->listener_entity_id));
			rc = ACMP_STAT_INCOMPATIBLE_REQUEST;
			goto exit;
		}
	}

	/* Only perform stack stream connection for the first connection */
	if (!stream_info->u.ieee.connection_count) {

		/* Set talker stream info parameters */
		stream_info->stream_class = req_class;
		/* FIXME, will be allocated by MAAP one day.
		* For now, use a hash of entity id and stream index
		*/
		os_memcpy(stream_info->stream_dest_mac, avtp_base_dst_mac, 6);
		stream_info->stream_dest_mac[5] = rotating_hash_u8((u8 *)&entity->desc->entity_id, 8, 0);
		stream_info->stream_dest_mac[5] = rotating_hash_u8((u8 *)&stream_output->descriptor_index, 2, stream_info->stream_dest_mac[5]);

		stream_info->stream_vlan_id = htons(0); // FIXME, this value indicates to use the default VLAN ID from the SRP domain being used
		os_memset(&stream_info->stream_id, 0, sizeof(stream_info->stream_id));
		os_memcpy(&stream_info->stream_id, avb_itf->mac_address, 6);
		*(((u16 *)&stream_info->stream_id) + 3) = stream_output->descriptor_index;

		if (acmp_talker_declare(avdecc, port_id, stream_output, stream_output_dynamic, ntohs(pdu->flags)) < 0) {
			os_log(LOG_ERR, "acmp(%p) cannot connect SRP\n", &entity->acmp);
			rc = ACMP_STAT_TALKER_MISBEHAVING;
			goto exit;
		}

		if (acmp_talker_avtp_stack_connect(entity, port_id, talker_unique_id, stream_output, stream_output_dynamic, ntohs(pdu->flags)) < 0) {
			os_log(LOG_ERR, "acmp(%p) cannot connect AVTP\n", &entity->acmp);
			rc = ACMP_STAT_TALKER_MISBEHAVING;
			goto exit;
		}
	} else {
		/* Check new listener compatibility */
		if (req_class != stream_info->stream_class) {
			rc = ACMP_STAT_INCOMPATIBLE_REQUEST;
			os_log(LOG_ERR, "incompatible request on stream_id(%016"PRIx64")\n", ntohll(stream_info->stream_id));
			goto exit;
		}
	}

	/* Set listener info */
	copy_64(&listener->listener_entity_id, &pdu->listener_entity_id);
	listener->listener_unique_id = pdu->listener_unique_id;
	listener->connected = 1;

	stream_info->u.ieee.connection_count++;

	os_log(LOG_INFO, "success\n");
exit:
	return rc;
}

/** Disconnects the talker stream from an AVDECC listener (8.2.2.6.2.4).
 * If there are no more listeners the stream is disconnected.
 * \return                      ACMP status
 * \param entity                pointer to the entity context
 * \param stream_info           pointer to the stream information context
 * \param listener              pointer to listener information context
 * \param talker_unique_id      talker stream unique id
 */
static u8 acmp_ieee_talker_disconnect(struct entity *entity, struct talker_stream_info *stream_info, struct listener_pair *listener, u16 talker_unique_id)
{
	struct stream_output_dynamic_desc *stream_output_dynamic;
	struct avdecc_ctx *avdecc = entity->avdecc;
	struct stream_descriptor *stream_output;
	u8 rc = ACMP_STAT_SUCCESS;
	unsigned int port_id;

	listener->connected = 0;

	if (--stream_info->u.ieee.connection_count == 0) {
		stream_output = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_STREAM_OUTPUT, talker_unique_id, NULL);
		if (!stream_output) {
			rc = ACMP_STAT_TALKER_MISBEHAVING;
			goto exit;
		}

		stream_output_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_OUTPUT, talker_unique_id, NULL);
		if (!stream_output_dynamic) {
			rc = ACMP_STAT_TALKER_MISBEHAVING;
			goto exit;
		}

		port_id = ntohs(stream_output->avb_interface_index);

		if (acmp_talker_withdraw(avdecc, port_id, stream_output_dynamic) < 0) {
			os_log(LOG_ERR, "acmp(%p) cannot disconnect SRP\n", &entity->acmp);
			rc = ACMP_STAT_TALKER_MISBEHAVING;
			goto exit;
		}

		if (acmp_talker_avtp_stack_disconnect(avdecc, port_id, talker_unique_id, stream_output_dynamic) < 0) {
			os_log(LOG_ERR, "acmp(%p) cannot disconnect AVTP\n", &entity->acmp);
			rc = ACMP_STAT_TALKER_MISBEHAVING;
			goto exit;
		}
	}

exit:
	return rc;
}

/** Disconnect talker streams locally.
 *
 * Parses the list of talker_info contexts and checks if they have connected listeners,
 * in which case a local disconnect is executed for each of the listeners.
 * The function is not standard but allows for cleanup of talker resources when the entity is
 * being stopped.
 *
 * \return	none
 * \param acmp		ACMP context
 */
static void acmp_ieee_talker_local_disconnect(struct acmp_ctx *acmp)
{
	struct entity *entity = container_of(acmp, struct entity, acmp);
	struct talker_stream_info *stream_info;
	struct listener_pair *listener;
	int i, j;

	for (i = 0; i < acmp->max_talker_streams; i++) {
		if (!acmp_talker_unique_valid(acmp, i))
			continue;

		stream_info = &acmp->u.ieee.talker_info[i];

		for (j = 0; j < acmp->u.ieee.max_listener_pairs; j++) {
			if (!stream_info->u.ieee.listeners[j].connected)
				continue;

			listener = &stream_info->u.ieee.listeners[j];

			acmp_ieee_talker_disconnect(entity, stream_info, listener, i);
		}
	}
}

/** Main ACMP listener receive function.
 * Implementation of the ACMP listener state machine (8.2.2.5.3).
 * \return	0 on success, negative otherwise
 * \param acmp	pointer to the ACMP context
 * \param pdu		pointer to the ACMP PDU
 * \param msg_type	ACMP message type (8.2.1.5)
 * \param status	status from AVTP control header (8.2.1.6)
 * \param port_id	avdecc port / interface index on which we received the PDU
 */
int acmp_ieee_listener_rcv(struct acmp_ctx *acmp, struct acmp_pdu *pdu, u8 msg_type, u8 status, unsigned int port_id)
{
	struct entity *entity = container_of(acmp, struct entity, acmp);
	struct avdecc_ctx *avdecc = entity->avdecc;
	struct net_tx_desc *desc_rsp;
	struct acmp_pdu *acmp_rsp;
	u16 status_rsp = 0;
	int rc = 0;
	u16 unique_id = ntohs(pdu->listener_unique_id);
	u16 orig_sequence_id;
	struct listener_stream_info *listener_info;
	struct avdecc_port *port_cmd, *port_rsp = &avdecc->port[port_id];

	if (ACMP_IS_COMMAND(msg_type))
		os_log(LOG_INFO, "acmp(%p) %s: controller(%016"PRIx64") listener(%016"PRIx64", %u) talker(%016"PRIx64", %u)\n",
			acmp, acmp_ieee_msgtype2string(msg_type), ntohll(pdu->controller_entity_id),
			ntohll(entity->desc->entity_id), unique_id,
			ntohll(pdu->talker_entity_id), ntohs(pdu->talker_unique_id));
	else
		os_log(LOG_INFO, "acmp(%p) %s: controller(%016"PRIx64") listener(%016"PRIx64", %u) talker(%016"PRIx64", %u) status(%d)\n",
			acmp, acmp_ieee_msgtype2string(msg_type), ntohll(pdu->controller_entity_id),
			ntohll(entity->desc->entity_id), unique_id,
			ntohll(pdu->talker_entity_id), ntohs(pdu->talker_unique_id), status);

	/* Allocate/init net descriptor for either command or response */
	desc_rsp = acmp_net_tx_init(acmp, port_rsp, pdu, false);
	if (!desc_rsp) {
		rc = -1;
		goto exit;
	}

	acmp_rsp = (struct acmp_pdu *)((char *)NET_DATA_START(desc_rsp) + OFFSET_TO_ACMP);

	if (!acmp_listener_unique_valid(acmp, unique_id)) {
		if (ACMP_IS_LISTENER_COMMAND(msg_type)) {
			acmp_rsp->sequence_id = pdu->sequence_id;

			rc = acmp_send_rsp(acmp, port_rsp, desc_rsp, msg_type + 1, ACMP_STAT_LISTENER_UNKNOWN_ID);
		} else
			net_tx_free(desc_rsp);

		goto exit;
	}

	listener_info = &acmp->u.ieee.listener_info[unique_id];

	switch (msg_type) {
	case ACMP_CONNECT_RX_COMMAND:
		if (!acmp_ieee_listener_connected(listener_info, pdu)) {
			struct stream_descriptor *stream_input = aem_get_descriptor(entity->aem_descs,
											AEM_DESC_TYPE_STREAM_INPUT, unique_id, NULL);

			acmp_rsp->flags = pdu->flags;

			/* The CONNECT_TX_COMMAND should be sent on the port attached to the STREAM_INPUT to be connected */
			port_cmd = &avdecc->port[ntohs(stream_input->avb_interface_index)];

			rc = acmp_send_cmd(acmp, port_cmd, acmp_rsp, desc_rsp, ACMP_CONNECT_TX_COMMAND, 0, pdu->sequence_id, NULL, 0);

			/* Disable fast-connect as we received a controller command on this stream input */
			listener_info->u.ieee.flags_priv &= ~ACMP_LISTENER_FL_FAST_CONNECT;
		} else {
			acmp_rsp->sequence_id = pdu->sequence_id;

			rc = acmp_send_rsp(acmp, port_rsp, desc_rsp, ACMP_CONNECT_RX_RESPONSE, ACMP_STAT_LISTENER_EXCLUSIVE);
		}

		break;

	case ACMP_CONNECT_TX_RESPONSE:
		if (status == ACMP_STAT_SUCCESS) {
			status_rsp = acmp_ieee_listener_connect(entity, listener_info, pdu);
		}
		else
			status_rsp = status;

		acmp_ieee_listener_copy_info(acmp_rsp, listener_info);

		if (avdecc_inflight_cancel(entity, &acmp->inflight, ntohs(pdu->sequence_id), &orig_sequence_id, NULL, NULL) < 0)
			os_log(LOG_ERR, "acmp(%p) Could not cancel inflight seq %d\n", acmp, ntohs(pdu->sequence_id));
		else
			acmp_rsp->sequence_id = orig_sequence_id;

		/* Do not send controller response in fast-connect */
		if (!(listener_info->u.ieee.flags_priv & ACMP_LISTENER_FL_FAST_CONNECT))
			rc = acmp_send_rsp(acmp, port_rsp, desc_rsp, ACMP_CONNECT_RX_RESPONSE, status_rsp);
		else
			net_tx_free(desc_rsp);

		break;

	case ACMP_DISCONNECT_RX_COMMAND:
		if (acmp_ieee_listener_connected_to(listener_info, pdu)) {
			if ((status_rsp = acmp_ieee_listener_disconnect(entity, listener_info, unique_id)) == ACMP_STAT_SUCCESS) {
				struct stream_descriptor *stream_input = aem_get_descriptor(entity->aem_descs,
												AEM_DESC_TYPE_STREAM_INPUT, unique_id, NULL);

				acmp_rsp->flags = pdu->flags;

				/* The DISCONNECT_TX_COMMAND should be sent on the port attached to the STREAM_INPUT to be disconnected */
				port_cmd = &avdecc->port[ntohs(stream_input->avb_interface_index)];

				rc = acmp_send_cmd(acmp, port_cmd, acmp_rsp, desc_rsp, ACMP_DISCONNECT_TX_COMMAND, 0, pdu->sequence_id, NULL, 0);

				goto exit;
			}
		}
		else
			status_rsp = ACMP_STAT_NOT_CONNECTED;

		acmp_rsp->sequence_id = pdu->sequence_id;

		rc = acmp_send_rsp(acmp, port_rsp, desc_rsp, ACMP_DISCONNECT_RX_RESPONSE, status_rsp);

		break;

	case ACMP_DISCONNECT_TX_RESPONSE:
		if (avdecc_inflight_cancel(entity, &acmp->inflight, ntohs(pdu->sequence_id), &orig_sequence_id, NULL, NULL) < 0)
			os_log(LOG_ERR, "acmp(%p) Could not cancel inflight seq %d\n", acmp, ntohs(pdu->sequence_id));
		else
			acmp_rsp->sequence_id = orig_sequence_id;

		rc = acmp_send_rsp(acmp, port_rsp, desc_rsp, ACMP_DISCONNECT_RX_RESPONSE, status);

		break;

	case ACMP_GET_RX_STATE_COMMAND:
		acmp_ieee_listener_copy_info(acmp_rsp, listener_info);

		acmp_rsp->sequence_id = pdu->sequence_id;

		rc = acmp_send_rsp(acmp, port_rsp, desc_rsp, ACMP_GET_RX_STATE_RESPONSE, ACMP_STAT_SUCCESS);

		break;

	default:
		os_log(LOG_ERR, "acmp(%p) message type (%x) not supported\n", acmp, msg_type);
		net_tx_free(desc_rsp);
		rc = -1;

		break;
	}

exit:
	return rc;
}

/** Main ACMP talker receive function.
 * Implementation of the ACMP talker state machine (8.2.2.6.3).
 * \return	0 on success, negative otherwise
 * \param acmp		pointer to the ACMP context
 * \param pdu		pointer to the ACMP PDU
 * \param msg_type	ACMP message type (8.2.1.5)
 * \param status	status from AVTP control header (8.2.1.6)
 * \param port_id	avdecc port / interface index on which we received the PDU
 */
int acmp_ieee_talker_rcv(struct acmp_ctx *acmp, struct acmp_pdu *pdu, u8 msg_type, u8 status, unsigned int port_id)
{
	struct entity *entity = container_of(acmp, struct entity, acmp);
	struct avdecc_ctx *avdecc = entity->avdecc;
	struct net_tx_desc *desc_rsp;
	struct acmp_pdu *acmp_rsp;
	u16 status_rsp = 0;
	int rc = 0;
	u16 unique_id = ntohs(pdu->talker_unique_id);
	u16 connection_count;
	struct listener_pair *listener;
	struct talker_stream_info *stream_info;
	struct avdecc_port *port_rsp = &avdecc->port[port_id];

	os_log(LOG_INFO, "acmp(%p) port(%u) %s: controller(%016"PRIx64") talker(%016"PRIx64", %u) listener(%016"PRIx64", %u)\n",
	       acmp, port_id, acmp_ieee_msgtype2string(msg_type), ntohll(pdu->controller_entity_id),
	       ntohll(entity->desc->entity_id), unique_id,
	       ntohll(pdu->listener_entity_id), ntohs(pdu->listener_unique_id));

	/* Only responses are sent by talker */
	desc_rsp = acmp_net_tx_init(acmp, port_rsp, pdu, true);
	if (!desc_rsp) {
		rc = -1;
		goto exit;
	}

	acmp_rsp = (struct acmp_pdu *)((char *)NET_DATA_START(desc_rsp) + OFFSET_TO_ACMP);

	if (!acmp_talker_unique_valid(acmp, unique_id)) {
		if (ACMP_IS_TALKER_COMMAND(msg_type)) {
			rc = acmp_send_rsp(acmp, port_rsp, desc_rsp, msg_type + 1, ACMP_STAT_TALKER_UNKNOWN_ID);
		} else
			net_tx_free(desc_rsp);

		goto exit;
	}

	stream_info = &acmp->u.ieee.talker_info[unique_id];

	switch (msg_type) {
	case ACMP_CONNECT_TX_COMMAND:
		if ((listener = acmp_ieee_talker_connected_to(acmp, stream_info, pdu)) == NULL) {
			status_rsp = acmp_ieee_talker_connect(entity, stream_info, pdu);

			os_log(LOG_INFO, "acmp(%p) listener(%016"PRIx64", %d) unique id(%x) is connected: connection_count %u/%u\n",
				acmp, ntohll(pdu->listener_entity_id), ntohs(pdu->listener_unique_id), unique_id, stream_info->u.ieee.connection_count, acmp->u.ieee.max_listener_pairs);
		}
		else
			status_rsp = ACMP_STAT_SUCCESS;

		acmp_ieee_talker_copy_stream_info(acmp_rsp, stream_info);

		acmp_rsp->flags = pdu->flags;

		rc = acmp_send_rsp(acmp, port_rsp, desc_rsp, ACMP_CONNECT_TX_RESPONSE, status_rsp);

		break;

	case ACMP_DISCONNECT_TX_COMMAND:
		if ((listener = acmp_ieee_talker_connected_to(acmp, stream_info, pdu)) != NULL) {
			status_rsp = acmp_ieee_talker_disconnect(entity, stream_info, listener, unique_id);

			acmp_ieee_talker_copy_stream_info(acmp_rsp, stream_info);

			os_log(LOG_INFO, "acmp(%p) listener(%016"PRIx64", %d) unique id(%x) has disconnected: connection_count %u/%u\n",
				acmp, ntohll(pdu->listener_entity_id), ntohs(pdu->listener_unique_id), unique_id, stream_info->u.ieee.connection_count, acmp->u.ieee.max_listener_pairs);
		}
		else
			status_rsp = ACMP_STAT_NO_SUCH_CONNECTION;

		rc = acmp_send_rsp(acmp, port_rsp, desc_rsp, ACMP_DISCONNECT_TX_RESPONSE, status_rsp);

		break;

	case ACMP_GET_TX_STATE_COMMAND:
		acmp_ieee_talker_copy_stream_info(acmp_rsp, stream_info);

		rc = acmp_send_rsp(acmp, port_rsp, desc_rsp, ACMP_GET_TX_STATE_RESPONSE, ACMP_STAT_SUCCESS);

		break;

	case ACMP_GET_TX_CONNECTION_COMMAND:
		connection_count = ntohs(pdu->connection_count);

		if ((connection_count >= acmp->u.ieee.max_listener_pairs) || !stream_info->u.ieee.listeners[connection_count].connected)
			status_rsp = ACMP_STAT_NO_SUCH_CONNECTION;
		else {
			copy_64(&acmp_rsp->listener_entity_id, &stream_info->u.ieee.listeners[connection_count].listener_entity_id);
			acmp_rsp->listener_unique_id = stream_info->u.ieee.listeners[connection_count].listener_unique_id;

			acmp_ieee_talker_copy_stream_info(acmp_rsp, stream_info);

			status_rsp = ACMP_STAT_SUCCESS;
		}

		rc = acmp_send_rsp(acmp, port_rsp, desc_rsp, ACMP_GET_TX_CONNECTION_RESPONSE, status_rsp);

		break;

	default:
		os_log(LOG_ERR, "acmp(%p) message type (%x) not supported\n", acmp, msg_type);
		net_tx_free(desc_rsp);
		rc = -1;

		break;
	}

exit:
	return rc;
}

__init unsigned int acmp_ieee_data_size(struct avdecc_entity_config *cfg)
{
	return cfg->max_talker_streams * cfg->max_listener_pairs * sizeof(struct listener_pair);
}

__init int acmp_ieee_init(struct acmp_ctx *acmp, void *data, struct avdecc_entity_config *cfg)
{
	struct entity *entity = container_of(acmp, struct entity, acmp);
	struct listener_pair *listener_pairs;
	int i;
	int rc = 0;

	if (!data) {
		os_log(LOG_ERR, "acmp(%p) No allocated memory for ListenerPairs array\n", acmp);
		rc = -1;
		goto exit;
	}

	acmp->u.ieee.max_listener_pairs = cfg->max_listener_pairs;

	acmp->u.ieee.listener_info = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_INPUT, 0, NULL);
	acmp->u.ieee.talker_info = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_OUTPUT, 0, NULL);

	listener_pairs = (struct listener_pair *)data;

	for (i = 0; i < acmp->max_talker_streams; i++) {
		acmp->u.ieee.talker_info[i].u.ieee.listeners = &listener_pairs[i * cfg->max_listener_pairs];

		/* Init presentation_time_offset to invalid value.
		 * If not user defined by connection time, the ACMP code will default it to the SR class max transit time.
		 */
		acmp->u.ieee.talker_info[i].presentation_time_offset = STREAM_PRESENTATION_TIME_OFFSET_INVALID;
	}

	if (acmp->max_listener_streams)
		acmp_ieee_listener_fast_connect_init(acmp, cfg);

exit:
	return rc;
}

__exit int acmp_ieee_exit(struct acmp_ctx *acmp)
{
	acmp_ieee_listener_fast_disconnect(acmp);

	acmp_ieee_talker_local_disconnect(acmp);

	os_log(LOG_INIT, "done\n");

	return 0;
}
