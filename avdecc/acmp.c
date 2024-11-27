/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @brief ACMP common code
 @details Handles ACMP common stack functions between IEEE 1722.1 and AVnu MILAN
*/

#include "os/stdlib.h"

#include "common/log.h"
#include "common/ether.h"
#include "common/hash.h"

#include "acmp.h"
#include "acmp_ieee.h"
#include "acmp_milan.h"
#include "avdecc.h"


static const u8 acmp_dst_mac[6] = MC_ADDR_AVDECC_ADP_ACMP;

int acmp_inflight_timeout(struct inflight_ctx *entry);

static const char *acmp_msgtype2string(struct acmp_ctx *acmp, acmp_message_type_t msg_type)
{
	struct entity *entity = container_of(acmp, struct entity, acmp);
	struct avdecc_ctx *avdecc = entity->avdecc;

	if (!avdecc->milan_mode)
		return acmp_ieee_msgtype2string(msg_type);
	else
		return acmp_milan_msgtype2string(msg_type);
}

static int acmp_get_command_timeout_ms(struct acmp_ctx *acmp, u8 msg_type)
{
	struct entity *entity = container_of(acmp, struct entity, acmp);
	struct avdecc_ctx *avdecc = entity->avdecc;

	if (!avdecc->milan_mode)
		return acmp_ieee_get_command_timeout_ms(msg_type);
	else
		return acmp_milan_get_command_timeout_ms(msg_type);
}

/** Sends an ACMP Response message to an IPC
 * \return	0 on success, -1 otherwise
 * \param acmp		pointer to the ACMP context
 * \param pdu		pointer to the ACMP PDU command
 * \param message_type	ACMP message type
 * \param ipc		IPC the command came through (will be stored in the inflight entry to send the response through the same channel).
 * \param ipc_dst	Slot, within the ipc channel, the message was received from (will be stored in the inflight entry to send the response to the same slot).
 */
static int acmp_ipc_tx_rsp(struct acmp_ctx *acmp, struct acmp_pdu *pdu, u8 message_type, u8 status, struct ipc_tx *ipc, unsigned int ipc_dst)
{
	struct ipc_desc *desc;
	int rc = -1;

	desc = ipc_alloc(ipc, sizeof(struct genavb_acmp_response));
	if (desc) {
		desc->dst = ipc_dst;
		desc->type = GENAVB_MSG_ACMP_RESPONSE;
		desc->len = sizeof(struct genavb_acmp_response);
		os_memset(&desc->u.acmp_response, 0 , sizeof(struct genavb_acmp_response));

		desc->u.acmp_response.message_type = message_type;
		desc->u.acmp_response.status = status;
		copy_64(&desc->u.acmp_response.stream_id, &pdu->stream_id);
		copy_64(&desc->u.acmp_response.talker_entity_id, &pdu->talker_entity_id);
		copy_64(&desc->u.acmp_response.listener_entity_id, &pdu->listener_entity_id);
		desc->u.acmp_response.talker_unique_id = pdu->talker_unique_id;
		desc->u.acmp_response.listener_unique_id = pdu->listener_unique_id;
		os_memcpy(desc->u.acmp_response.stream_dest_mac, &pdu->stream_dest_mac, 6);
		desc->u.acmp_response.connection_count = pdu->connection_count;
		desc->u.acmp_response.flags = pdu->flags;
		desc->u.acmp_response.stream_vlan_id = pdu->stream_vlan_id;

		if (ipc_tx(ipc, desc) < 0) {
			os_log(LOG_ERR, "acmp(%p) ipc_tx() through ipc(%p) failed for message_type(%d) status(%d)\n",
				acmp, ipc, message_type, status);
			goto err_ipc_tx;
		}

		rc = 0;
	} else {
		os_log(LOG_ERR, "acmp(%p) ipc_alloc() failed for message_type(%d) status(%d)\n",
			acmp, message_type, status);
	}

	return rc;

err_ipc_tx:
	ipc_free(ipc, desc);
	return rc;
}
/** Allocate a network tx descriptor for ACMP PDU and init all fields to 0.
 * \return              the allocated and initialized network descriptor, or NULL on allocation failure
 * \param acmp		pointer to the ACMP context
 * \param port		pointer to the avdecc port on which the packet will be sent
 */
struct net_tx_desc *acmp_net_tx_alloc(struct acmp_ctx *acmp, struct avdecc_port *port)
{
	struct net_tx_desc *desc;
	struct acmp_pdu *new_pdu;

	desc = net_tx_alloc(&port->net_tx, ACMP_NET_DATA_SIZE);
	if (!desc) {
		os_log(LOG_ERR, "acmp(%p) Cannot alloc tx descriptor\n", acmp);
		goto exit;
	}

	new_pdu = (struct acmp_pdu *)((char *)NET_DATA_START(desc) + OFFSET_TO_ACMP);

	os_memset(new_pdu, 0 , sizeof(struct acmp_pdu));

exit:
	return desc;
}

/** Allocate a network tx descriptor for ACMP command or response based on fields from a received ACMP PDU
 * Only controller_entity_id, talker_entity_id, talker_unique_id, listener_entity_id, listener_unique_id
 * and sequence_id (for response only) are copied, other fields are set to 0.
 * \return              the allocated and initialized network descriptor, or NULL on allocation failure
 * \param acmp		pointer to the ACMP context
 * \param port		pointer to the avdecc port on which the packet will be sent
 * \param pdu           pointer to the ACMP PDU
 * \param is_resp       true for ACMP response (copy the sequence_id) or false to keep sequence_id to 0
 */
struct net_tx_desc *acmp_net_tx_init(struct acmp_ctx *acmp, struct avdecc_port *port, struct acmp_pdu *pdu, bool is_resp)
{
	struct net_tx_desc *desc;
	struct acmp_pdu *new_pdu;

	desc = acmp_net_tx_alloc(acmp, port);
	if (!desc)
		goto exit;

	new_pdu = (struct acmp_pdu *)((char *)NET_DATA_START(desc) + OFFSET_TO_ACMP);

	copy_64(&new_pdu->controller_entity_id, &pdu->controller_entity_id);
	copy_64(&new_pdu->talker_entity_id, &pdu->talker_entity_id);
	copy_64(&new_pdu->listener_entity_id, &pdu->listener_entity_id);

	new_pdu->talker_unique_id = pdu->talker_unique_id;
	new_pdu->listener_unique_id = pdu->listener_unique_id;

	if (is_resp)
		new_pdu->sequence_id = pdu->sequence_id;

exit:
	return desc;
}

/** Sends an ACMP PDU packet to the network.
 * \return	0 on success, -1 otherwise
 * \param acmp		pointer to the ACMP context
 * \param port		pointer to the avdecc port on which the packet will be sent
 * \param desc		pointer to the network TX descriptor
 * \param msg_type	ACMP message type
 * \param status	ACMP status
 */
static int acmp_send_packet(struct acmp_ctx *acmp, struct avdecc_port *port, struct net_tx_desc *desc, u8 msg_type, u16 status)
{
	struct acmp_pdu *pdu;
	void *buf;

	buf = NET_DATA_START(desc);
	pdu = (struct acmp_pdu *)((char *)buf + OFFSET_TO_ACMP);

	desc->len += net_add_eth_header(buf, acmp_dst_mac, ETHERTYPE_AVTP);
	desc->len += avdecc_add_common_header((char *)buf + desc->len, AVTP_SUBTYPE_ACMP, msg_type, ACMP_PDU_LEN, status);
	desc->len += sizeof(struct acmp_pdu);

	os_log(LOG_INFO, "acmp(%p) port(%u) %s: controller(%016"PRIx64") talker(%016"PRIx64", %u) listener(%016"PRIx64", %u)\n",
		acmp, port->port_id, acmp_msgtype2string(acmp, msg_type), ntohll(pdu->controller_entity_id),
		ntohll(pdu->talker_entity_id), ntohs(pdu->talker_unique_id),
		ntohll(pdu->listener_entity_id), ntohs(pdu->listener_unique_id));

	if (avdecc_net_tx(port, desc) < 0) {
		os_log(LOG_ERR, "acmp(%p) port(%u) send failed\n", acmp, port->port_id);
		goto err;
	}

	return 0;

err:
	return -1;
}

/** Sends an ACMP PDU command to the network.
 * An inflight context is created to manage timeout and answer from the remote entity.
 * \return	0 on success, -1 otherwise
 * \param acmp		pointer to the ACMP context
 * \param port		pointer to the avdecc port on which the packet will be sent
 * \param pdu		pointer to the ACMP PDU command
 * \param desc		pointer to the network TX descriptor
 * \param msg_type	ACMP message type
 * \param retried	boolean, true if this command is a retry
 * \param orig_seq_id	the sequence ID of the command which initiated this command
 * \param ipc		IPC channel the command came through (will be stored in the inflight entry to send the response through the same channel).
 * \param ipc_dst	Slot, within the ipc channel, the message was received from (will be stored in the inflight entry to send the response to the same slot).
 */
int acmp_send_cmd(struct acmp_ctx *acmp, struct avdecc_port *port, struct acmp_pdu *pdu, struct net_tx_desc *desc,
			u8 msg_type, u8 retried, u16 orig_seq_id, struct ipc_tx *ipc, unsigned int ipc_dst)
{
	struct entity *entity = container_of(acmp, struct entity, acmp);

	if (!retried) {
		struct inflight_ctx *entry = avdecc_inflight_get(entity);

		pdu->sequence_id = htons(acmp->sequence_id);
		if (entry){
			int cmd_timeout = acmp_get_command_timeout_ms(acmp, msg_type);
			if (cmd_timeout < 0) {
				os_log(LOG_ERR, "acmp(%p) Invalid timeout for command %u\n", acmp, msg_type);
			} else {

				entry->cb = acmp_inflight_timeout;
				entry->data.msg_type = msg_type;
				entry->data.retried = 0;
				entry->data.sequence_id = acmp->sequence_id;
				entry->data.orig_seq_id = orig_seq_id;
				entry->data.port_id = port->port_id;
				entry->data.pdu.acmp = *pdu;
				entry->data.priv[0] = (uintptr_t)ipc;
				entry->data.priv[1] = (uintptr_t)ipc_dst;

				if (avdecc_inflight_start(&acmp->inflight, entry, cmd_timeout) < 0)
					os_log(LOG_ERR, "acmp(%p) Could not start inflight\n", acmp);
			}
		}
		acmp->sequence_id++;
	}

	return acmp_send_packet(acmp, port, desc, msg_type, ACMP_STAT_SUCCESS);
}

/** Sends an ACMP PDU response to the network.
 * \return	0 on success, -1 otherwise
 * \param acmp		pointer to the ACMP context
 * \param port		pointer to the avdecc port on which the packet will be sent
 * \param desc		pointer to the network TX descriptor
 * \param msg_type	ACMP message type
 * \param status	ACMP status
 */
int acmp_send_rsp(struct acmp_ctx *acmp, struct avdecc_port *port, struct net_tx_desc *desc, u8 msg_type, u16 status)
{
	return acmp_send_packet(acmp, port, desc, msg_type, status);
}

/** Copy common (1722.1 and Milan) listener params to PDU
 * \return                      none
 * \param pdu                   pointer to the ACMP PDU
 * \param listener_params       pointer to the stream_input_dynamic_desc structure
 */
void acmp_listener_copy_common_params(struct acmp_pdu *pdu, struct stream_input_dynamic_desc *listener_params)
{
	copy_64(&pdu->stream_id, &listener_params->stream_id);

	copy_64(&pdu->talker_entity_id, &listener_params->talker_entity_id);
	pdu->talker_unique_id = listener_params->talker_unique_id;

	os_memcpy(pdu->stream_dest_mac, listener_params->stream_dest_mac, 6);

	pdu->flags = listener_params->flags;
	pdu->stream_vlan_id = listener_params->stream_vlan_id;
}

/** Perform SRP and AVTP listener connection
 * Sends an indication to AVTP / media application and initiates, if enabled, the SRP
 * listener registration.
 * \return                      ACMP status
 * \param entity                pointer to the entity context
 * \param listener_unique_id    valid listener unique id (in host order)
 * \param flags                 ACMP flags (in host order)
 */
u8 acmp_listener_stack_connect(struct entity *entity, u16 listener_unique_id, u16 flags)
{
	struct avdecc_ctx *avdecc = entity->avdecc;
	struct stream_descriptor *stream_input;
	struct stream_input_dynamic_desc *stream_input_dynamic;
	struct ipc_desc *desc;
	u8 rc = ACMP_STAT_SUCCESS;
	unsigned int port_id;

	/* FIXME do we need to wait for a status ? */

	/* Send listener connect to AVTP */
	stream_input = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_STREAM_INPUT, listener_unique_id, NULL);
	if (!stream_input) {
		os_log(LOG_ERR, "acmp(%p) cannot find descriptor type %d index %u\n",
			&entity->acmp, AEM_DESC_TYPE_STREAM_INPUT, listener_unique_id);
		rc = ACMP_STAT_LISTENER_MISBEHAVING;
		goto exit;
	}

	stream_input_dynamic = aem_get_descriptor(entity->aem_dynamic_descs,
							AEM_DESC_TYPE_STREAM_INPUT, listener_unique_id, NULL);
	if (!stream_input_dynamic) {
		os_log(LOG_ERR, "acmp(%p) cannot find dynamic descriptor type %d index %u\n",
			&entity->acmp, AEM_DESC_TYPE_STREAM_INPUT, listener_unique_id);
		rc = ACMP_STAT_LISTENER_MISBEHAVING;
		goto exit;
	}

	port_id = ntohs(stream_input->avb_interface_index);

	desc = ipc_alloc(&avdecc->ipc_tx_media_stack, sizeof(struct genavb_msg_media_stack_connect));
	if (desc) {
		desc->dst = IPC_DST_ALL;
		desc->type = GENAVB_MSG_MEDIA_STACK_CONNECT;
		desc->len = sizeof(struct genavb_msg_media_stack_connect);

		os_memcpy(desc->u.media_stack_connect.stream_params.dst_mac, stream_input_dynamic->stream_dest_mac, 6);
		copy_64(&desc->u.media_stack_connect.stream_params.stream_id, &stream_input_dynamic->stream_id);
		desc->u.media_stack_connect.stream_params.port = avdecc_port_to_logical(avdecc, port_id);
		copy_64(&desc->u.media_stack_connect.stream_params.format, &stream_input->current_format);
		desc->u.media_stack_connect.stream_params.subtype = desc->u.media_stack_connect.stream_params.format.u.s.subtype;
		desc->u.media_stack_connect.stream_params.stream_class = (flags & ACMP_FLAG_CLASS_B) ? sr_class_low() : sr_class_high();
		desc->u.media_stack_connect.stream_params.direction = AVTP_DIRECTION_LISTENER;
		desc->u.media_stack_connect.entity_index = entity->index;
		desc->u.media_stack_connect.configuration_index = ntohs(entity->desc->current_configuration);
		desc->u.media_stack_connect.stream_index = listener_unique_id;

		desc->u.media_stack_connect.flags = flags;

		/* FIXME */
		desc->u.media_stack_connect.stream_params.flags = IPC_AVTP_FLAGS_MCR;
		desc->u.media_stack_connect.stream_params.clock_domain = GENAVB_MEDIA_CLOCK_DOMAIN_STREAM;
		/* FIXME */

		if (ipc_tx(&avdecc->ipc_tx_media_stack, desc) < 0) {
			os_log(LOG_ERR, "avdecc(%p) ipc_tx() failed\n", avdecc);
			ipc_free(&avdecc->ipc_tx_media_stack, desc);
		}
	} else {
		os_log(LOG_ERR, "avdecc(%p) ipc_alloc() failed\n", avdecc);
	}


	if (avdecc->srp_enabled) {
		/* Send listener connect to MSRP */
		desc = ipc_alloc(&avdecc->port[port_id].ipc_tx_srp, sizeof(struct ipc_msrp_listener_register));
		if (desc) {
			desc->type = GENAVB_MSG_LISTENER_REGISTER;
			desc->len = sizeof(struct ipc_msrp_listener_register);

			desc->u.msrp_listener_register.port = avdecc_port_to_logical(avdecc, port_id);
			copy_64(&desc->u.msrp_listener_register.stream_id, &stream_input_dynamic->stream_id);

			if (ipc_tx(&avdecc->port[port_id].ipc_tx_srp, desc) < 0) {
				os_log(LOG_ERR, "avdecc(%p) ipc_tx() failed\n", avdecc);
				ipc_free(&avdecc->port[port_id].ipc_tx_srp, desc);
			}
		} else {
			os_log(LOG_ERR, "avdecc(%p) ipc_alloc() failed\n", avdecc);
		}
	}

exit:
	return rc;
}

/** Disconnects the stream from the AVDECC listener (8.2.2.5.2.7).
 * Send an indication to AVTP / media application and initiates the SRP
 * listener de-registration.
 * \return                      ACMP status
 * \param entity                pointer to the entity context
 * \param listener_unique_id    valid listener unique ID (in host order)
 */
u8 acmp_listener_stack_disconnect(struct entity *entity, u16 listener_unique_id)
{
	struct avdecc_ctx *avdecc = entity->avdecc;
	struct stream_descriptor *stream_input;
	struct stream_input_dynamic_desc *stream_input_dynamic;
	struct ipc_desc *desc;
	u8 rc = ACMP_STAT_SUCCESS;
	unsigned int port_id;

	/* FIXME do we need to wait for a status ? */

	stream_input = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_STREAM_INPUT, listener_unique_id, NULL);
	if (!stream_input) {
		os_log(LOG_ERR, "acmp(%p) cannot find descriptor type %d index %u\n",
			&entity->acmp, AEM_DESC_TYPE_STREAM_INPUT, listener_unique_id);
		rc = ACMP_STAT_LISTENER_MISBEHAVING;
		goto exit;
	}

	stream_input_dynamic = aem_get_descriptor(entity->aem_dynamic_descs,
							AEM_DESC_TYPE_STREAM_INPUT, listener_unique_id, NULL);
	if (!stream_input_dynamic) {
		os_log(LOG_ERR, "acmp(%p) cannot find dynamic descriptor type %d index %u\n",
			&entity->acmp, AEM_DESC_TYPE_STREAM_INPUT, listener_unique_id);
		rc = ACMP_STAT_LISTENER_MISBEHAVING;
		goto exit;
	}

	port_id = ntohs(stream_input->avb_interface_index);

	if (avdecc->srp_enabled) {
		/* Send listener disconnect to MSRP */
		desc = ipc_alloc(&avdecc->port[port_id].ipc_tx_srp, sizeof(struct ipc_msrp_listener_deregister));
		if (desc) {
			desc->type = GENAVB_MSG_LISTENER_DEREGISTER;
			desc->len = sizeof(struct ipc_msrp_listener_deregister);

			desc->u.msrp_listener_deregister.port = avdecc_port_to_logical(avdecc, port_id);
			copy_64(&desc->u.msrp_listener_deregister.stream_id, &stream_input_dynamic->stream_id);

			if (ipc_tx(&avdecc->port[port_id].ipc_tx_srp, desc) < 0) {
				os_log(LOG_ERR, "avdecc(%p) ipc_tx() failed\n", avdecc);
				ipc_free(&avdecc->port[port_id].ipc_tx_srp, desc);
			}
		} else {
			os_log(LOG_ERR, "avdecc(%p) ipc_alloc() failed\n", avdecc);
		}
	}

	/* Send listener disconnect to AVTP */
	desc = ipc_alloc(&avdecc->ipc_tx_media_stack, sizeof(struct genavb_msg_media_stack_disconnect));
	if (desc) {
		desc->dst = IPC_DST_ALL;
		desc->type = GENAVB_MSG_MEDIA_STACK_DISCONNECT;
		desc->len = sizeof(struct genavb_msg_media_stack_disconnect);

		desc->u.media_stack_disconnect.stream_index = listener_unique_id;
		copy_64(&desc->u.media_stack_disconnect.stream_id, &stream_input_dynamic->stream_id);
		desc->u.media_stack_disconnect.port = avdecc_port_to_logical(avdecc, port_id);
		desc->u.media_stack_disconnect.stream_class = (ntohs(stream_input_dynamic->flags) & ACMP_FLAG_CLASS_B) ? sr_class_low() : sr_class_high();
		desc->u.media_stack_disconnect.direction = AVTP_DIRECTION_LISTENER;

		if (ipc_tx(&avdecc->ipc_tx_media_stack, desc) < 0) {
			os_log(LOG_ERR, "avdecc(%p) ipc_tx() failed\n", avdecc);
			ipc_free(&avdecc->ipc_tx_media_stack, desc);
		}
	} else {
		os_log(LOG_ERR, "avdecc(%p) ipc_alloc() failed\n", avdecc);
	}

	os_log(LOG_INFO, "avdecc(%p) stream_id(%016"PRIx64")\n", avdecc, ntohll(stream_input_dynamic->stream_id));

exit:
	return rc;
}

/** Checks if the provided listener unique ID is valid for the AVDECC entity (8.2.2.5.2.1).
 * \return	1 if unique id is valid, 0 otherwise
 * \param acmp		pointer to the ACMP context
 * \param unique_id	unique ID
 */
int acmp_listener_unique_valid(struct acmp_ctx *acmp, u16 unique_id)
{
	struct entity *entity = container_of(acmp, struct entity, acmp);

	if (aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_STREAM_INPUT, unique_id, NULL))
		return 1;
	else
		return 0;
}

/** Checks if the provided talker unique ID is valid for the AVDECC entity (8.2.2.6.2.1).
 * \return	1 if unique id is valid, 0 otherwise
 * \param acmp		pointer to the ACMP context
 * \param unique_id	unique ID
 */
int acmp_talker_unique_valid(struct acmp_ctx *acmp, u16 unique_id)
{
	struct entity *entity = container_of(acmp, struct entity, acmp);

	if (aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_STREAM_OUTPUT, unique_id, NULL))
		return 1;
	else
		return 0;
}

/** Copy common (1722.1 and Milan) talker params to PDU
 * \return                      none
 * \param pdu                   pointer to the ACMP PDU
 * \param talker_params         pointer to the stream_output_dynamic_desc structure
 */
void acmp_talker_copy_common_params(struct acmp_pdu *pdu, struct stream_output_dynamic_desc *talker_params)
{
	copy_64(&pdu->stream_id, &talker_params->stream_id);
	os_memcpy(pdu->stream_dest_mac, talker_params->stream_dest_mac, 6);
	pdu->stream_vlan_id = talker_params->stream_vlan_id;
}

/** Perform SRP talker connection
 * \return                              0 on success, otherwise -1
 * \param avdecc                        pointer to the avdecc context
 * \param port_id                       port / AVB interface index
 * \param stream_output                 pointer to the stream output descriptor
 * \param stream_output_dynamic         pointer to the stream output dynamic descriptor
 * \param flags                         ACMP flags (in host order)
 */
int acmp_talker_declare(struct avdecc_ctx *avdecc, unsigned int port_id, struct stream_descriptor *stream_output, struct stream_output_dynamic_desc *stream_output_dynamic, u16 flags)
{
	struct ipc_desc *desc;
	int rc = 0;

	if (avdecc->srp_enabled) {
		/* Send talker connect to MSRP */
		desc = ipc_alloc(&avdecc->port[port_id].ipc_tx_srp, sizeof(struct ipc_msrp_talker_register));
		if (desc) {
			unsigned int max_frame_size, max_interval_frames;

			desc->type = GENAVB_MSG_TALKER_REGISTER;
			desc->len = sizeof(struct ipc_msrp_talker_register);

			desc->u.msrp_talker_register.port = avdecc_port_to_logical(avdecc, port_id);
			copy_64(&desc->u.msrp_talker_register.stream_id, &stream_output_dynamic->stream_id);
			desc->u.msrp_talker_register.params.stream_class = (flags & ACMP_FLAG_CLASS_B) ? sr_class_low() : sr_class_high();
			os_memcpy(desc->u.msrp_talker_register.params.destination_address, stream_output_dynamic->stream_dest_mac, 6);

			if (stream_output_dynamic->stream_vlan_id)
				desc->u.msrp_talker_register.params.vlan_id = ntohs(stream_output_dynamic->stream_vlan_id);
			else
				desc->u.msrp_talker_register.params.vlan_id = VLAN_VID_DEFAULT;

			avdecc_fmt_tspec((struct avdecc_format *)&stream_output->current_format, desc->u.msrp_talker_register.params.stream_class, &max_frame_size, &max_interval_frames);

			desc->u.msrp_talker_register.params.max_frame_size = max_frame_size;
			desc->u.msrp_talker_register.params.max_interval_frames = max_interval_frames;
			desc->u.msrp_talker_register.params.accumulated_latency = 0;
			desc->u.msrp_talker_register.params.rank = NORMAL;

			if (ipc_tx(&avdecc->port[port_id].ipc_tx_srp, desc) < 0) {
				os_log(LOG_ERR, "avdecc(%p) ipc_tx_srp() failed\n", avdecc);
				ipc_free(&avdecc->port[port_id].ipc_tx_srp, desc);
				rc = -1;
			}
		} else {
			os_log(LOG_ERR, "avdecc(%p) ipc_alloc() failed\n", avdecc);
			rc = -1;
		}
	}

	os_log(LOG_INFO, "avdecc(%p) stream_id(%016"PRIx64") sent connect notification to SRP\n", avdecc, ntohll(stream_output_dynamic->stream_id));

	return rc;
}

/** Perform AVTP talker connection
 * \return                              0 on success, otherwise -1
 * \param entity                        pointer to the entity context
 * \param port_id                       port / AVB interface index
 * \param stream_output                 pointer to the stream output descriptor
 * \param stream_output_dynamic         pointer to the stream output dynamic descriptor
 * \param talker_unique_id              valid talker unique id (in host order)
 * \param flags                         ACMP flags (in host order)
 */
int acmp_talker_avtp_stack_connect(struct entity *entity, unsigned int port_id, u16 talker_unique_id, struct stream_descriptor *stream_output, struct stream_output_dynamic_desc *stream_output_dynamic, u16 flags)
{
	struct avdecc_ctx *avdecc = entity->avdecc;
	struct ipc_desc *desc;
	int rc = 0;

	/* Send talker connect to AVTP */
	desc = ipc_alloc(&avdecc->ipc_tx_media_stack, sizeof(struct genavb_msg_media_stack_connect));
	if (desc) {
		desc->dst = IPC_DST_ALL;
		desc->type = GENAVB_MSG_MEDIA_STACK_CONNECT;
		desc->len = sizeof(struct genavb_msg_media_stack_connect);

		os_memcpy(desc->u.media_stack_connect.stream_params.dst_mac, stream_output_dynamic->stream_dest_mac, 6);
		copy_64(&desc->u.media_stack_connect.stream_params.stream_id, &stream_output_dynamic->stream_id);
		desc->u.media_stack_connect.stream_params.port = avdecc_port_to_logical(avdecc, port_id);
		copy_64(&desc->u.media_stack_connect.stream_params.format, &stream_output->current_format);
		desc->u.media_stack_connect.stream_params.subtype = desc->u.media_stack_connect.stream_params.format.u.s.subtype;
		desc->u.media_stack_connect.stream_params.stream_class = (stream_output_dynamic->stream_class == SR_CLASS_B) ? sr_class_low() : sr_class_high();
		desc->u.media_stack_connect.stream_params.clock_domain = GENAVB_MEDIA_CLOCK_DOMAIN_PTP;
		desc->u.media_stack_connect.stream_params.direction = AVTP_DIRECTION_TALKER;
		desc->u.media_stack_connect.entity_index = entity->index;
		desc->u.media_stack_connect.configuration_index = ntohs(entity->desc->current_configuration);
		desc->u.media_stack_connect.stream_index = talker_unique_id;
		desc->u.media_stack_connect.stream_params.talker.latency = max(CFG_AVTP_DEFAULT_LATENCY, sr_class_interval_p(desc->u.media_stack_connect.stream_params.stream_class) / sr_class_interval_q(desc->u.media_stack_connect.stream_params.stream_class));

		if (stream_output_dynamic->presentation_time_offset == STREAM_PRESENTATION_TIME_OFFSET_INVALID)
			desc->u.media_stack_connect.stream_params.talker.max_transit_time = sr_class_max_transit_time(desc->u.media_stack_connect.stream_params.stream_class);
		else
			desc->u.media_stack_connect.stream_params.talker.max_transit_time = stream_output_dynamic->presentation_time_offset;

		desc->u.media_stack_connect.stream_params.flags = IPC_AVTP_FLAGS_MAX_TRANSIT_TIME_VALID;

		desc->u.media_stack_connect.flags = flags;

		if (stream_output_dynamic->stream_vlan_id == htons(0))
			desc->u.media_stack_connect.stream_params.talker.vlan_id = htons(VLAN_VID_DEFAULT);
		else
			desc->u.media_stack_connect.stream_params.talker.vlan_id = stream_output_dynamic->stream_vlan_id;

		if (ipc_tx(&avdecc->ipc_tx_media_stack, desc) < 0) {
			os_log(LOG_ERR, "avdecc(%p) ipc_tx_avtp() failed\n", avdecc);
			ipc_free(&avdecc->ipc_tx_media_stack, desc);
			rc = -1;
		}
	} else {
		os_log(LOG_ERR, "avdecc(%p) ipc_alloc() failed\n", avdecc);
		rc = -1;
	}

	/* Reset Talker's concerned counters when it starts streaming and increment the start_stream counter
	 * As per Milan v1.2, section 5.3.7.7
	 */
	stream_output_dynamic->diagnostic_counters.media_reset = 0;
	stream_output_dynamic->diagnostic_counters.timestamp_uncertain = 0;
	stream_output_dynamic->diagnostic_counters.frame_tx = 0;
	stream_output_dynamic->diagnostic_counters.stream_start += 1;

	aecp_register_get_counters_async_notification(entity, AEM_DESC_TYPE_STREAM_OUTPUT, stream_output_dynamic->unique_id);

	os_log(LOG_INFO, "avdecc(%p) stream_id(%016"PRIx64") sent connect notification to AVTP\n", avdecc, ntohll(stream_output_dynamic->stream_id));

	return rc;
}

/** Perform SRP talker disconnect
 * \return                              0 on success, otherwise -1
 * \param avdecc                        pointer to the avdecc context
 * \param port_id                       port / AVB interface index
 * \param stream_output_dynamic         pointer to the stream output dynamic descriptor
 */
int acmp_talker_withdraw(struct avdecc_ctx *avdecc, unsigned int port_id, struct stream_output_dynamic_desc *stream_output_dynamic)
{
	struct ipc_desc *desc;
	int rc = 0;

	if (avdecc->srp_enabled) {
		/* Send talker disconnect to MSRP */
		desc = ipc_alloc(&avdecc->port[port_id].ipc_tx_srp, sizeof(struct ipc_msrp_talker_deregister));
		if (desc) {
			desc->type = GENAVB_MSG_TALKER_DEREGISTER;
			desc->len = sizeof(struct ipc_msrp_talker_deregister);

			desc->u.msrp_talker_deregister.port = avdecc_port_to_logical(avdecc, port_id);
			copy_64(&desc->u.msrp_talker_deregister.stream_id, &stream_output_dynamic->stream_id);

			if (ipc_tx(&avdecc->port[port_id].ipc_tx_srp, desc) < 0) {
				os_log(LOG_ERR, "avdecc(%p) ipc_tx() failed\n", avdecc);
				ipc_free(&avdecc->port[port_id].ipc_tx_srp, desc);
				rc = -1;
			}
		} else {
			os_log(LOG_ERR, "avdecc(%p) ipc_alloc() failed\n", avdecc);
			rc = -1;
		}
	}

	os_log(LOG_INFO, "avdecc(%p) stream_id(%016"PRIx64") sent disconnect notification to SRP\n", avdecc, ntohll(stream_output_dynamic->stream_id));

	return rc;
}

/** Perform AVTP talker disconnect
 * \return                              0 on success, otherwise -1
 * \param avdecc                        pointer to the avdecc context
 * \param port_id                       port / AVB interface index
 * \param talker_unique_id              talker stream unique id (in host order)
 * \param stream_output_dynamic         pointer to the stream output dynamic descriptor
 */
int acmp_talker_avtp_stack_disconnect(struct avdecc_ctx *avdecc, unsigned int port_id, u16 talker_unique_id, struct stream_output_dynamic_desc *stream_output_dynamic)
{
	struct ipc_desc *desc;
	int rc = 0;

	desc = ipc_alloc(&avdecc->ipc_tx_media_stack, sizeof(struct genavb_msg_media_stack_disconnect));
	if (desc) {
		desc->dst = IPC_DST_ALL;
		desc->type = GENAVB_MSG_MEDIA_STACK_DISCONNECT;
		desc->len = sizeof(struct genavb_msg_media_stack_disconnect);

		desc->u.media_stack_disconnect.stream_index = talker_unique_id;
		desc->u.media_stack_disconnect.port = avdecc_port_to_logical(avdecc, port_id);
		desc->u.media_stack_disconnect.stream_class = stream_output_dynamic->stream_class;
		desc->u.media_stack_disconnect.direction = AVTP_DIRECTION_TALKER;

		copy_64(&desc->u.media_stack_disconnect.stream_id, &stream_output_dynamic->stream_id);

		if (ipc_tx(&avdecc->ipc_tx_media_stack, desc) < 0) {
			os_log(LOG_ERR, "avdecc(%p) ipc_tx() failed\n", avdecc);
			ipc_free(&avdecc->ipc_tx_media_stack, desc);
			rc = -1;
		}
	} else {
		os_log(LOG_ERR, "avdecc(%p) ipc_alloc() failed\n", avdecc);
		rc = -1;
	}

	stream_output_dynamic->diagnostic_counters.stream_stop += 1;

	aecp_register_get_counters_async_notification(stream_output_dynamic->entity, AEM_DESC_TYPE_STREAM_OUTPUT, stream_output_dynamic->unique_id);

	os_log(LOG_INFO, "avdecc(%p) stream_id(%016"PRIx64") sent disconnect notification to AVTP\n", avdecc, ntohll(stream_output_dynamic->stream_id));

	return rc;
}

/** Helper function to check if a stream input or output is running
 * \return bool, true if stream is running, false otherwise
 * \param entity
 * \param stream_desc_type, AEM_DESC_TYPE_STREAM_INPUT or AEM_DESC_TYPE_STREAM_OUTPUT
 * \param stream_desc_index, index of the descriptor
 */
bool acmp_is_stream_running(struct entity *entity, u16 stream_desc_type, u16 stream_desc_index)
{
	struct avdecc_ctx *avdecc = entity->avdecc;

	if (!avdecc->milan_mode)
		return acmp_ieee_is_stream_running(entity, stream_desc_type, stream_desc_index);
	else
		return acmp_milan_is_stream_running(entity, stream_desc_type, stream_desc_index);
}

/** Update streaming_wait flag and send a GENAVB_MSG_MEDIA_STACK_BIND to update the status of the stream
 * \return positive value (0 if nothing changed, 1 if stream descriptor was updated) if successful, negative otherwise
 * \param entity
 * \param stream_desc_type, AEM_DESC_TYPE_STREAM_INPUT or AEM_DESC_TYPE_STREAM_OUTPUT
 * \param stream_desc_index, index of the descriptor
 */
int acmp_start_streaming(struct entity *entity, u16 stream_desc_type, u16 stream_desc_index)
{
	struct avdecc_ctx *avdecc = entity->avdecc;
	int rc = 0;

	if (avdecc->milan_mode)
		rc = acmp_milan_start_streaming(entity, stream_desc_type, stream_desc_index);
	else
		os_log(LOG_DEBUG, "entity(%p) tried to start stream(%u, %u) on ieee side.\n", entity, stream_desc_type, stream_desc_index);

	return rc;
}

/** Update streaming_wait flag and send a GENAVB_MSG_MEDIA_STACK_BIND to update the status of the stream
 * \return positive value (0 if nothing changed, 1 if stream descriptor was updated) if successful, negative otherwise
 * \param entity
 * \param stream_desc_type, AEM_DESC_TYPE_STREAM_INPUT or AEM_DESC_TYPE_STREAM_OUTPUT
 * \param stream_desc_index, index of the descriptor
 */
int acmp_stop_streaming(struct entity *entity, u16 stream_desc_type, u16 stream_desc_index)
{
	struct avdecc_ctx *avdecc = entity->avdecc;
	int rc = 0;

	if (avdecc->milan_mode)
		rc = acmp_milan_stop_streaming(entity, stream_desc_type, stream_desc_index);
	else
		os_log(LOG_DEBUG, "entity(%p) tried to stop stream(%u, %u) on ieee side.\n", entity, stream_desc_type, stream_desc_index);

	return rc;
}

/** Allocate a network tx descriptor for ACMP and init all fields with the PDU
 * \return        	the allocated and initialized network descriptor
 * \param pdu     	pointer to the ACMP PDU
 * \param port		pointer to the avdecc port on which the packet will be sent
 */
static struct net_tx_desc *acmp_net_tx_clone(struct acmp_pdu *acmp, struct avdecc_port *port)
{
	struct net_tx_desc *desc;
	void *buf;
	struct acmp_pdu *pdu;

	desc = net_tx_alloc(&port->net_tx, ACMP_NET_DATA_SIZE);
	if (!desc) {
		os_log(LOG_ERR, "acmp(%p) Cannot alloc tx descriptor\n", acmp);
		return desc;
	}

	buf = NET_DATA_START(desc);
	pdu = (struct acmp_pdu *)((char *)buf + OFFSET_TO_ACMP);
	*pdu = *acmp;

	return desc;
}

/** ACMP timeout handling callback.
 * Called by the generic inflight handling layer of AVDECC upon
 * expiration of the timer associated to an ACMP command.
 * Corresponds to the timeout states of ACMP listener state machines (8.2.2.5.3).
 * \return	0 if timer needs to be restarted, 1 if timer needs to be stopped
 * \param entry	pointer to the inflight context
 */
int acmp_inflight_timeout(struct inflight_ctx *entry)
{
	struct acmp_ctx *acmp = container_of(entry->list_head, struct acmp_ctx, inflight);
	struct entity *entity = container_of(acmp, struct entity, acmp);
	struct avdecc_ctx *avdecc = entity->avdecc;
	struct avdecc_port *port;
	struct net_tx_desc *desc;
	int rc;
	u8 msg_type = 0;

	os_log(LOG_DEBUG, "acmp(%p) inflight_ctx(%p) sequence id : %x\n", acmp, entry, entry->data.pdu.acmp.sequence_id);

	if (entry->data.retried) {
		rc = AVDECC_INFLIGHT_TIMER_STOP;
		switch (entry->data.msg_type) {
		case ACMP_CONNECT_TX_COMMAND:
		case ACMP_DISCONNECT_TX_COMMAND:
			if (!avdecc->milan_mode) {
				/* We're a Listener, so we send a timeout response back to the controller */
				if ((entry->data.pdu.acmp.flags & htons(ACMP_FLAG_FAST_CONNECT)) == 0) { /* Do not send timeout messages in fast-connect */
					port = &avdecc->port[entry->data.port_id];

					desc = acmp_net_tx_clone(&entry->data.pdu.acmp, port);
					if (!desc)
						goto exit;

					if (entry->data.msg_type == ACMP_CONNECT_TX_COMMAND)
						msg_type = ACMP_CONNECT_RX_RESPONSE;
					else
						msg_type = ACMP_DISCONNECT_RX_RESPONSE;

					acmp_send_rsp(acmp, port, desc, msg_type, ACMP_STAT_LISTENER_TALKER_TIMEOUT);
				}
			} else {
				acmp_milan_listener_sink_event(entity, ntohs(entry->data.pdu.acmp.listener_unique_id), ACMP_LISTENER_SINK_SM_EVENT_TMR_NO_RESP);
			}
			break;
		default: /* We're a Controller, so we send an IPC back to the controller app */
			/* FIXME Technically, this is not a Listener-Talker timeout */
			acmp_ipc_tx_rsp(acmp, &entry->data.pdu.acmp, entry->data.msg_type + 1, ACMP_STAT_LISTENER_TALKER_TIMEOUT,
					    (void *)entry->data.priv[0], (unsigned int)entry->data.priv[1]);
			break;
		}
	} else {
		if (avdecc->milan_mode)
			acmp_milan_listener_sink_event(entity, ntohs(entry->data.pdu.acmp.listener_unique_id), ACMP_LISTENER_SINK_SM_EVENT_TMR_NO_RESP);

		rc = AVDECC_INFLIGHT_TIMER_RESTART;

		port = &avdecc->port[entry->data.port_id];

		desc = acmp_net_tx_clone(&entry->data.pdu.acmp, port);
		if (!desc)
			goto exit;

		entry->data.retried = 1;

		acmp_send_cmd(acmp, port, NULL, desc, entry->data.msg_type, 1, 0, NULL, 0);
	}
exit:
	return rc;
}

/** Main ACMP controller receive function.
 * Implementation of the ACMP controller state machine (8.2.2.4.3).
 * \return	0 on success, negative otherwise
 * \param acmp		pointer to the ACMP context
 * \param pdu		pointer to the ACMP PDU
 * \param msg_type	ACMP message type (8.2.1.5)
 * \param status	ACMP message status (8.2.1.6)
 */
int acmp_controller_rcv(struct acmp_ctx *acmp,struct acmp_pdu *pdu, u8 msg_type, u8 status)
{
	struct entity *entity = container_of(acmp, struct entity, acmp);
	struct ipc_tx *ipc;
	uintptr_t ipc_dst;
	int rc;

	os_log(LOG_INFO, "acmp(%p) %s\n", acmp, acmp_msgtype2string(acmp, msg_type));

	rc = avdecc_inflight_cancel(entity, &acmp->inflight, ntohs(pdu->sequence_id), NULL, (void **)&ipc, (void **)&ipc_dst);

	if ((rc < 0) || !ipc) {
		os_log(LOG_ERR, "acmp(%p) Received an ACMP message msg_type(%d) status(%d) with no valid inflight entry\n",
			acmp, msg_type, status);
		goto exit;
	}

	acmp_ipc_tx_rsp(acmp, pdu, msg_type, status, ipc, ipc_dst);

exit:
	return rc;
}

/** Main ACMP network receive function.
 * \return	0 on success, negative otherwise
 * \param avdecc	pointer to the AVDECC port
 * \param pdu		pointer to the ACMP PDU
 * \param msg_type	ACMP message type (8.2.1.5)
 * \param status	status from AVTP control header (8.2.1.6)
 */
int acmp_net_rx(struct avdecc_port *port, struct acmp_pdu *pdu, u8 msg_type, u8 status)
{
	struct avdecc_ctx *avdecc = avdecc_port_to_context(port);
	struct entity *entity;
	int rc = 0;

	switch (msg_type) {
	case ACMP_CONNECT_TX_RESPONSE:
	case ACMP_DISCONNECT_TX_RESPONSE:
	case ACMP_CONNECT_RX_COMMAND:
	case ACMP_DISCONNECT_RX_COMMAND:
	case ACMP_GET_RX_STATE_COMMAND:
		entity = avdecc_get_entity(avdecc, pdu->listener_entity_id);
		if (entity && avdecc_entity_port_valid(entity, port->port_id)) {
			if (!avdecc->milan_mode)
				rc = acmp_ieee_listener_rcv(&entity->acmp, pdu, msg_type, status, port->port_id);
			else
				rc = acmp_milan_listener_rcv(&entity->acmp, pdu, msg_type, status, port->port_id);
		}

		break;

	case ACMP_CONNECT_TX_COMMAND:
	case ACMP_DISCONNECT_TX_COMMAND:
	case ACMP_GET_TX_STATE_COMMAND:
	case ACMP_GET_TX_CONNECTION_COMMAND:
		entity = avdecc_get_entity(avdecc, pdu->talker_entity_id);
		if (entity && avdecc_entity_port_valid(entity, port->port_id)) {
			if (!avdecc->milan_mode)
				rc = acmp_ieee_talker_rcv(&entity->acmp, pdu, msg_type, status, port->port_id);
			else
				rc = acmp_milan_talker_rcv(&entity->acmp, pdu, msg_type, status, port->port_id);
		}

		break;

	case ACMP_CONNECT_RX_RESPONSE:
	case ACMP_DISCONNECT_RX_RESPONSE:
	case ACMP_GET_RX_STATE_RESPONSE:
	case ACMP_GET_TX_STATE_RESPONSE:
	case ACMP_GET_TX_CONNECTION_RESPONSE:
		entity = avdecc_get_entity(avdecc, pdu->controller_entity_id);
		if (entity && avdecc_entity_port_valid(entity, port->port_id))
			rc = acmp_controller_rcv(&entity->acmp, pdu, msg_type, status);

		break;

	default:
		os_log(LOG_ERR, "port(%u) Unknown message type (%d) \n", port->port_id, msg_type);
		rc = -1;
		break;
	}

	return rc;
}

/** Main ACMP IPC receive function
 * \return 0 on success or negative value otherwise.
 * \param entity	Controller entity the IPC was received for.
 * \param rx_desc	Pointer to the received ACMP command.
 * \param len		Length of the received IPC message payload.
 * \param ipc		IPC the message was received through.
 * \param ipc_dst	Slot, within the ipc channel, the message was received from.
 */
int acmp_ipc_rx(struct entity *entity, struct ipc_acmp_command *acmp_command, u32 len, struct ipc_tx *ipc, unsigned int ipc_dst)
{
	struct acmp_ctx *acmp = &entity->acmp;
	struct net_tx_desc *desc_cmd;
	struct acmp_pdu *acmp_cmd;
	int rc = 0;
	struct avdecc_ctx *avdecc = entity->avdecc;
	struct avdecc_port *port;
	struct entity_discovery *entity_disc = NULL;
	u64 targetted_entity_id;
	unsigned int num_interfaces;

	if (len < sizeof(struct ipc_acmp_command)) {
		os_log(LOG_ERR, "acmp(%p) Invalid IPC ACMP message size (%d instead of expected %zd)\n",
			acmp, len, sizeof(struct ipc_acmp_command));
		rc = -1;
		goto exit;
	}

	os_log(LOG_DEBUG, "acmp(%p) ACMP message_type (%d)\n", acmp, acmp_command->message_type);

	switch (acmp_command->message_type) {
	case ACMP_CONNECT_RX_COMMAND:
	case ACMP_DISCONNECT_RX_COMMAND:
	case ACMP_GET_RX_STATE_COMMAND:
		copy_64(&targetted_entity_id, &acmp_command->listener_entity_id);
		break;
	case ACMP_GET_TX_STATE_COMMAND:
	case ACMP_GET_TX_CONNECTION_COMMAND:
		copy_64(&targetted_entity_id, &acmp_command->talker_entity_id);
		break;
	default:
		os_log(LOG_ERR, "acmp(%p) Received invalid command type %d\n", acmp, acmp_command->message_type);
		rc = -1;
		goto exit;
	}

	/* Get number of supported interfaces for the controller entity. */
	num_interfaces = aem_get_descriptor_max(entity->aem_descs, AEM_DESC_TYPE_AVB_INTERFACE);

	entity_disc = adp_find_entity_discovery_any(avdecc, targetted_entity_id, num_interfaces);
	if (!entity_disc) {
		os_log(LOG_ERR, "acmp(%p) cannot send command %s, targetted entity(%"PRIx64") not visible on the network.\n",
			acmp, acmp_msgtype2string(acmp, acmp_command->message_type), htonll(targetted_entity_id));
		rc = -1;
		goto exit;
	}

	/* Send command on the port on which we discovered the entity. */
	port = discovery_to_avdecc_port(entity_disc->disc);

	desc_cmd = acmp_net_tx_alloc(acmp, port);
	if (!desc_cmd) {
		rc = -1;
		goto exit;
	}

	acmp_cmd = (struct acmp_pdu *)((char *)NET_DATA_START(desc_cmd) + OFFSET_TO_ACMP);

	copy_64(&acmp_cmd->controller_entity_id, &entity->desc->entity_id);
	copy_64(&acmp_cmd->talker_entity_id, &acmp_command->talker_entity_id);
	copy_64(&acmp_cmd->listener_entity_id, &acmp_command->listener_entity_id);
	acmp_cmd->talker_unique_id = acmp_command->talker_unique_id;
	acmp_cmd->listener_unique_id = acmp_command->listener_unique_id;
	acmp_cmd->connection_count = acmp_command->connection_count;
	acmp_cmd->flags = acmp_command->flags;

	rc = acmp_send_cmd(acmp, port, acmp_cmd, desc_cmd, acmp_command->message_type, 0, acmp->sequence_id, ipc, ipc_dst);
	if (rc < 0)
		os_log(LOG_ERR, "acmp(%p) Cannot send ACMP command %s\n",
			acmp, acmp_msgtype2string(acmp, acmp_command->message_type));

exit:
	return rc;
}

__init unsigned int acmp_data_size(struct avdecc_entity_config *cfg)
{
	if (!cfg->milan_mode)
		return acmp_ieee_data_size(cfg);
	else
		return 0;
}

__init int acmp_init(struct acmp_ctx *acmp, void *data, struct avdecc_entity_config *cfg)
{
	struct entity *entity = container_of(acmp, struct entity, acmp);
	struct avdecc_ctx *avdecc = entity->avdecc;
	int rc = 0;

	acmp->max_listener_streams = aem_get_listener_streams(entity->aem_descs);
	acmp->max_talker_streams = aem_get_talker_streams(entity->aem_descs);

	if (acmp->max_listener_streams > cfg->max_listener_streams) {
		os_log(LOG_ERR, "AVDECC listener streams (%u) above configured max (%u)\n",
			acmp->max_listener_streams, cfg->max_listener_streams);
		rc = -1;
		goto exit;
	}

	if (acmp->max_talker_streams > cfg->max_talker_streams) {
		os_log(LOG_ERR, "AVDECC talker streams (%u) above configured max (%u)\n",
			acmp->max_talker_streams, cfg->max_talker_streams);
		rc = -1;
		goto exit;
	}

	list_head_init(&acmp->inflight);

	if (!avdecc->milan_mode)
		rc = acmp_ieee_init(acmp, data, cfg);
	else
		rc = acmp_milan_init(acmp);

	if (rc < 0)
		goto exit;

	os_log(LOG_INIT, "acmp(%p) done\n", acmp);

exit:
	return rc;
}

__exit int acmp_exit(struct acmp_ctx *acmp)
{
	struct entity *entity = container_of(acmp, struct entity, acmp);
	struct avdecc_ctx *avdecc = entity->avdecc;

	if (!avdecc->milan_mode)
		acmp_ieee_exit(acmp);
	else
		acmp_milan_exit(acmp);

	os_log(LOG_INIT, "done\n");

	return 0;
}
