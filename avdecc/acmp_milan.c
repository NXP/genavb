/*
* Copyright 2021-2024 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @brief ACMP Milan code
 @details Handles ACMP Milan stack
*/

#include "os/stdlib.h"

#include "common/log.h"
#include "common/ether.h"
#include "common/hash.h"

#include "acmp_milan.h"
#include "avdecc.h"

#define ACMP_MSG_TYPE_NONE	0xff

#define ACMP_MILAN_IS_LISTENER_BINDING_COMMAND(msg_type) ((msg_type == ACMP_BIND_RX_COMMAND) || (msg_type == ACMP_UNBIND_RX_COMMAND))
#define ACMP_MILAN_IS_LISTENER_NETWORK_RCV(msg_type) ((msg_type != ACMP_MSG_TYPE_NONE))

/* MILAN Specification v1.2 (5.5.3.3 Specification of the Listener’s behavior) */
/* Random timer between 0 and 1 sec per spec: Make it between 0.2 and 1 sec to avoid 0 period timers and give time to async notifications. */
#define ACMP_MILAN_LISTENER_TMR_DELAY_MS      (random_range(200, 1000))

#define ACMP_MILAN_LISTENER_TMR_RETRY_MS      (4 * 1000) /* 4 seconds timer */
#define ACMP_MILAN_LISTENER_TMR_NO_TK_MS      (10 * 1000) /* 10 seconds timer */

#define ACMP_MILAN_LISTENER_TMR_DELAY_GRANULARITY_MS	10
#define ACMP_MILAN_LISTENER_TMR_RETRY_GRANULARITY_MS	100
#define ACMP_MILAN_LISTENER_TMR_NO_TK_GRANULARITY_MS	100

/* MILAN Specification v1.2 (4.3.3.1 Talker attribute declaration) */
#define ACMP_MILAN_TALKER_TMR_PROBE_TX_RECEPTION_MS		(15 * 1000) /* 15 seconds timer */
#define ACMP_MILAN_TALKER_TMR_PROBE_TX_RECEPTION_GRANULARITY_MS	100

/* MILAN Specification v1.2 (5.3.7.5 Plug-and-play SRP parameters) */
#define ACMP_MILAN_TALKER_TMR_SRP_WITHDRAW_MS			(2 * MRP_LVATIMER_VAL_MAX) /* 2 Max supported LeaveALL period for Milan */
#define ACMP_MILAN_TALKER_TMR_SRP_WITHDRAW_GRANULARITY_MS	100

#define ACMP_MILAN_TALKER_ASYNC_UNSOLICITED_NOTIFICATION_MS			1000 /* 1 second timer */
#define ACMP_MILAN_TALKER_ASYNC_UNSOLICITED_NOTIFICATION_GRANULARITY_MS	100

/* Listener's unsolicited notification timer should be lower than all the timers in the listener state machines so we don't miss any states changes */
#define ACMP_MILAN_LISTENER_ASYNC_UNSOLICITED_NOTIFICATION_MS			100 /* 100ms timer */
#define ACMP_MILAN_LISTENER_ASYNC_UNSOLICITED_NOTIFICATION_GRANULARITY_MS	10

#define acmp_milan_listener_sink_srp_state_clear(stream_input_dynamic) \
		acmp_milan_set_stream_input_srp_params((stream_input_dynamic), NULL); \
		(stream_input_dynamic)->u.milan.srp_state = ACMP_LISTENER_SINK_SRP_STATE_NOT_REGISTERING; \
		(stream_input_dynamic)->u.milan.srp_stream_status = NO_TALKER; \

static const char *acmp_milan_listener_sink_state2string(acmp_milan_listener_sink_sm_state_t state)
{
	switch (state) {
	case2str(ACMP_LISTENER_SINK_SM_STATE_UNBOUND);
	case2str(ACMP_LISTENER_SINK_SM_STATE_PRB_W_AVAIL);
	case2str(ACMP_LISTENER_SINK_SM_STATE_PRB_W_DELAY);
	case2str(ACMP_LISTENER_SINK_SM_STATE_PRB_W_RESP);
	case2str(ACMP_LISTENER_SINK_SM_STATE_PRB_W_RESP2);
	case2str(ACMP_LISTENER_SINK_SM_STATE_PRB_W_RETRY);
	case2str(ACMP_LISTENER_SINK_SM_STATE_SETTLED_NO_RSV);
	case2str(ACMP_LISTENER_SINK_SM_STATE_SETTLED_RSV_OK);
	default:
		return (char *) "Unknown listener sink state";
	}
}

static const char *acmp_milan_listener_sink_event2string(acmp_milan_listener_sink_sm_event_t event)
{
	switch (event) {
	case2str(ACMP_LISTENER_SINK_SM_EVENT_TMR_NO_RESP);
	case2str(ACMP_LISTENER_SINK_SM_EVENT_TMR_RETRY);
	case2str(ACMP_LISTENER_SINK_SM_EVENT_TMR_DELAY);
	case2str(ACMP_LISTENER_SINK_SM_EVENT_TMR_NO_TK);
	case2str(ACMP_LISTENER_SINK_SM_EVENT_RCV_BIND_RX_CMD);
	case2str(ACMP_LISTENER_SINK_SM_EVENT_RCV_PROBE_TX_RESP);
	case2str(ACMP_LISTENER_SINK_SM_EVENT_RCV_GET_RX_STATE);
	case2str(ACMP_LISTENER_SINK_SM_EVENT_RCV_UNBIND_RX_CMD);
	case2str(ACMP_LISTENER_SINK_SM_EVENT_EVT_TK_DISCOVERED);
	case2str(ACMP_LISTENER_SINK_SM_EVENT_EVT_TK_DEPARTED);
	case2str(ACMP_LISTENER_SINK_SM_EVENT_EVT_TK_REGISTERED);
	case2str(ACMP_LISTENER_SINK_SM_EVENT_EVT_TK_UNREGISTERED);
	case2str(ACMP_LISTENER_SINK_SM_EVENT_SAVED_BINDING_PARAMS);
	default:
		return (char *) "Unknown listener sink event";
	}
}

const char *acmp_milan_msgtype2string(acmp_message_type_t msg_type)
{
	switch (msg_type) {
	case2str(ACMP_PROBE_TX_COMMAND);
	case2str(ACMP_PROBE_TX_RESPONSE);
	case2str(ACMP_DISCONNECT_TX_COMMAND);
	case2str(ACMP_DISCONNECT_TX_RESPONSE);
	case2str(ACMP_GET_TX_STATE_COMMAND);
	case2str(ACMP_GET_TX_STATE_RESPONSE);
	case2str(ACMP_BIND_RX_COMMAND);
	case2str(ACMP_BIND_RX_RESPONSE);
	case2str(ACMP_UNBIND_RX_COMMAND);
	case2str(ACMP_UNBIND_RX_RESPONSE);
	case2str(ACMP_GET_RX_STATE_COMMAND);
	case2str(ACMP_GET_RX_STATE_RESPONSE);
	case2str(ACMP_GET_TX_CONNECTION_COMMAND);
	case2str(ACMP_GET_TX_CONNECTION_RESPONSE);
	default:
		return (char *) "Unknown ACMP MILAN message type";
	}
}

static u8 acmp_milan_listener_get_msg_type(acmp_milan_listener_sink_sm_event_t event)
{
	u8 rc;

	switch(event) {
	case ACMP_LISTENER_SINK_SM_EVENT_RCV_BIND_RX_CMD:
		rc = ACMP_BIND_RX_COMMAND;
		break;

	case ACMP_LISTENER_SINK_SM_EVENT_RCV_GET_RX_STATE:
		rc = ACMP_GET_RX_STATE_COMMAND;
		break;

	case ACMP_LISTENER_SINK_SM_EVENT_RCV_UNBIND_RX_CMD:
		rc = ACMP_UNBIND_RX_COMMAND;
		break;

	case ACMP_LISTENER_SINK_SM_EVENT_RCV_PROBE_TX_RESP:
		rc = ACMP_PROBE_TX_RESPONSE;
		break;

	default:
		rc = ACMP_MSG_TYPE_NONE;
		break;
	}

	return rc;
}

static acmp_milan_listener_sink_sm_event_t acmp_milan_listener_get_event(u8 msg_type)
{
	acmp_milan_listener_sink_sm_event_t rc;

	switch(msg_type) {
	case ACMP_BIND_RX_COMMAND:
		rc = ACMP_LISTENER_SINK_SM_EVENT_RCV_BIND_RX_CMD;
		break;

	case ACMP_GET_RX_STATE_COMMAND:
		rc = ACMP_LISTENER_SINK_SM_EVENT_RCV_GET_RX_STATE;
		break;

	case ACMP_UNBIND_RX_COMMAND:
		rc = ACMP_LISTENER_SINK_SM_EVENT_RCV_UNBIND_RX_CMD;
		break;

	case ACMP_PROBE_TX_RESPONSE:
		rc = ACMP_LISTENER_SINK_SM_EVENT_RCV_PROBE_TX_RESP;
		break;

	default:
		rc = ACMP_LISTENER_SINK_SM_EVENT_UNKNOWN;
		break;
	}

	return rc;
}

int acmp_milan_get_command_timeout_ms(acmp_message_type_t msg_type)
{
	switch (msg_type) {
	case ACMP_PROBE_TX_COMMAND:
	case ACMP_DISCONNECT_TX_COMMAND:
	case ACMP_GET_TX_STATE_COMMAND:
	case ACMP_BIND_RX_COMMAND:
	case ACMP_UNBIND_RX_COMMAND:
	case ACMP_GET_RX_STATE_COMMAND:
	case ACMP_GET_TX_CONNECTION_COMMAND:
		return 200;
	default:
		return -1;
	}
}

/** Start a timer to send an async unsolicited notification after a short delay if the timer is not currently running
 * \return none
 * \param stream_input_dynamic, the dynamic descriptor of the stream input that triggered the notification
 */
static void acmp_milan_listener_register_async_get_stream_info_notification(struct stream_input_dynamic_desc *stream_input_dynamic)
{
	if (!timer_is_running(&stream_input_dynamic->u.milan.async_unsolicited_notification_timer))
		timer_start(&stream_input_dynamic->u.milan.async_unsolicited_notification_timer, ACMP_MILAN_LISTENER_ASYNC_UNSOLICITED_NOTIFICATION_MS);
}

/** Send unbind message through the media stack ipc channel
 * Sends an indication to media application about unbind stream update, to save in non-volatile
 * memory.
 * \return                      ACMP status
 * \param entity                pointer to the entity context
 * \param listener_unique_id    valid listener unique id (in host order)
 */
static u8 acmp_listener_ipc_send_unbind_status(struct entity *entity, u16 listener_unique_id)
{
	struct avdecc_ctx *avdecc = entity->avdecc;
	struct ipc_desc *desc;
	u8 rc = ACMP_STAT_SUCCESS;

	if (!acmp_listener_unique_valid(&entity->acmp, listener_unique_id)) {
		os_log(LOG_ERR, "acmp(%p) invalid listener unique id %u\n",
			&entity->acmp, listener_unique_id);
		rc = ACMP_STAT_LISTENER_MISBEHAVING;
		goto exit;
	}

	desc = ipc_alloc(&avdecc->ipc_tx_media_stack, sizeof(struct genavb_msg_media_stack_unbind));
	if (desc) {
		desc->dst = IPC_DST_ALL;
		desc->type = GENAVB_MSG_MEDIA_STACK_UNBIND;
		desc->len = sizeof(struct genavb_msg_media_stack_unbind);

		desc->u.media_stack_unbind.entity_id = get_ntohll(&entity->desc->entity_id);
		desc->u.media_stack_unbind.entity_index = entity->index;
		desc->u.media_stack_unbind.listener_stream_index = listener_unique_id;

		if (ipc_tx(&avdecc->ipc_tx_media_stack, desc) < 0) {
			os_log(LOG_ERR, "avdecc(%p) ipc_tx() failed\n", avdecc);
			ipc_free(&avdecc->ipc_tx_media_stack, desc);
		}
	} else {
		os_log(LOG_ERR, "avdecc(%p) ipc_alloc() failed\n", avdecc);
	}

exit:
	return rc;
}

/** Send bind message through the media stack ipc channel
 * Sends an indication to media application about binding parameters update, to save in non-volatile
 * memory.
 * \return                      0 on success, -1 otherwise
 * \param entity                pointer to the entity context
 * \param listener_unique_id    valid listener unique id (in host order)
 */
static int acmp_listener_ipc_send_bind_status(struct entity *entity, u16 listener_unique_id)
{
	struct avdecc_ctx *avdecc = entity->avdecc;
	struct stream_input_dynamic_desc *stream_input_dynamic;
	struct ipc_desc *desc;
	int rc = 0;

	stream_input_dynamic = aem_get_descriptor(entity->aem_dynamic_descs,
							AEM_DESC_TYPE_STREAM_INPUT, listener_unique_id, NULL);
	if (!stream_input_dynamic) {
		os_log(LOG_ERR, "acmp(%p) cannot find dynamic descriptor type %d index %u\n",
			&entity->acmp, AEM_DESC_TYPE_STREAM_INPUT, listener_unique_id);
		rc = -1;
		goto exit;
	}

	desc = ipc_alloc(&avdecc->ipc_tx_media_stack, sizeof(struct genavb_msg_media_stack_bind));
	if (desc) {
		desc->dst = IPC_DST_ALL;
		desc->type = GENAVB_MSG_MEDIA_STACK_BIND;
		desc->len = sizeof(struct genavb_msg_media_stack_bind);

		desc->u.media_stack_bind.entity_id = get_ntohll(&entity->desc->entity_id);
		desc->u.media_stack_bind.entity_index = entity->index;
		desc->u.media_stack_bind.listener_stream_index = listener_unique_id;
		desc->u.media_stack_bind.talker_entity_id = get_ntohll(&stream_input_dynamic->talker_entity_id);
		desc->u.media_stack_bind.talker_stream_index = ntohs(stream_input_dynamic->talker_unique_id);
		desc->u.media_stack_bind.controller_entity_id = get_ntohll(&stream_input_dynamic->controller_entity_id);
		desc->u.media_stack_bind.started = (stream_input_dynamic->flags & htons(ACMP_FLAG_STREAMING_WAIT)) ? ACMP_LISTENER_STREAM_STOPPED : ACMP_LISTENER_STREAM_STARTED;

		if (ipc_tx(&avdecc->ipc_tx_media_stack, desc) < 0) {
			os_log(LOG_ERR, "avdecc(%p) ipc_tx() failed\n", avdecc);
			ipc_free(&avdecc->ipc_tx_media_stack, desc);
		}
	} else {
		os_log(LOG_ERR, "avdecc(%p) ipc_alloc() failed\n", avdecc);
	}

exit:
	return rc;
}

static void acmp_milan_set_stream_input_binding_params(struct stream_input_dynamic_desc *stream_input_dynamic, struct acmp_pdu *pdu)
{
	if (!pdu) {
		stream_input_dynamic->controller_entity_id = 0;
		stream_input_dynamic->talker_entity_id = 0;
		stream_input_dynamic->talker_unique_id = 0;
		stream_input_dynamic->flags = 0;
	} else {
		copy_64(&stream_input_dynamic->controller_entity_id, &pdu->controller_entity_id);
		copy_64(&stream_input_dynamic->talker_entity_id, &pdu->talker_entity_id);
		stream_input_dynamic->talker_unique_id = pdu->talker_unique_id;
		stream_input_dynamic->flags = pdu->flags & htons(ACMP_FLAG_STREAMING_WAIT);
	}
}

/* Per MILAN Specification v1.2 5.5.3: When already bound, restart binding process only
 * if talker_entity_id and talker_unique_id fields are not equal to those saved for the current binding
 */
static bool acmp_milan_need_binding_restart(struct stream_input_dynamic_desc *stream_input_dynamic, struct acmp_pdu *pdu)
{
	if (!cmp_64(&stream_input_dynamic->talker_entity_id, &pdu->talker_entity_id) ||
		stream_input_dynamic->talker_unique_id != pdu->talker_unique_id)
		return true;

	return false;
}

static void acmp_milan_set_stream_input_srp_params(struct stream_input_dynamic_desc *stream_input_dynamic, struct acmp_pdu *pdu)
{
	u8 invalid_stream_dest_mac[6] = {0};

	if (!pdu) {
		stream_input_dynamic->stream_vlan_id = 0;
		stream_input_dynamic->stream_id = 0;
		os_memcpy(stream_input_dynamic->stream_dest_mac, invalid_stream_dest_mac, 6);
	} else {
		stream_input_dynamic->stream_vlan_id = pdu->stream_vlan_id;
		copy_64(&stream_input_dynamic->stream_id, &pdu->stream_id);
		os_memcpy(stream_input_dynamic->stream_dest_mac, pdu->stream_dest_mac, 6);
	}

	/* Register send of an asynchronous GET_STREAM_INFO unsolicited notification per MILAN Specification v1.2 5.4.5.2
	 * to notify changes in the STREAM_INPUT descriptor state (stream_id, stream_dest_mac, stream_vlan_id)
	 */
	acmp_milan_listener_register_async_get_stream_info_notification(stream_input_dynamic);
}

static int acmp_milan_send_get_rx_state_response(struct entity *entity, u16 listener_unique_id, struct avdecc_port *port, struct net_tx_desc *desc_rsp)
{
	struct acmp_pdu *acmp_rsp;
	struct acmp_ctx *acmp = &entity->acmp;
	struct stream_input_dynamic_desc *stream_input_dynamic = aem_get_descriptor(entity->aem_dynamic_descs,
										      AEM_DESC_TYPE_STREAM_INPUT, listener_unique_id, NULL);

	acmp_rsp = (struct acmp_pdu *)((char *)NET_DATA_START(desc_rsp) + OFFSET_TO_ACMP);

	acmp_listener_copy_common_params(acmp_rsp, stream_input_dynamic);

	if (ACMP_MILAN_IS_LISTENER_SINK_BOUND(stream_input_dynamic)) {
		acmp_rsp->connection_count = htons(1);
		acmp_rsp->flags |= htons(ACMP_FLAG_FAST_CONNECT);
	} else {
		acmp_rsp->connection_count = htons(0);
		acmp_rsp->flags &= ~htons(ACMP_FLAG_FAST_CONNECT);
	}

	if ((stream_input_dynamic->u.milan.state == ACMP_LISTENER_SINK_SM_STATE_SETTLED_RSV_OK)
				&& (stream_input_dynamic->u.milan.srp_stream_status == FAILED)) {
		acmp_rsp->flags |= htons(ACMP_FLAG_REGISTERING_FAILED);
	} else {
		acmp_rsp->flags &= ~htons(ACMP_FLAG_REGISTERING_FAILED);
	}

	return acmp_send_rsp(acmp, port, desc_rsp, ACMP_GET_RX_STATE_RESPONSE, ACMP_STAT_SUCCESS);
}

static int acmp_milan_send_unbind_rx_response(struct entity *entity, u16 listener_unique_id, struct avdecc_port *port, struct net_tx_desc *desc_rsp)
{
	struct acmp_pdu *acmp_rsp;
	struct acmp_ctx *acmp = &entity->acmp;
	struct stream_input_dynamic_desc *stream_input_dynamic = aem_get_descriptor(entity->aem_dynamic_descs,
										      AEM_DESC_TYPE_STREAM_INPUT, listener_unique_id, NULL);

	acmp_rsp = (struct acmp_pdu *)((char *)NET_DATA_START(desc_rsp) + OFFSET_TO_ACMP);

	acmp_listener_copy_common_params(acmp_rsp, stream_input_dynamic);

	acmp_rsp->connection_count = htons(0);

	return acmp_send_rsp(acmp, port, desc_rsp, ACMP_UNBIND_RX_RESPONSE, ACMP_STAT_SUCCESS);
}

static int acmp_milan_send_bind_rx_response(struct entity *entity, u16 listener_unique_id, struct avdecc_port *port, struct net_tx_desc *desc_rsp, u16 flags, u16 status)
{
	struct acmp_pdu *acmp_rsp;
	struct acmp_ctx *acmp = &entity->acmp;

	acmp_rsp = (struct acmp_pdu *)((char *)NET_DATA_START(desc_rsp) + OFFSET_TO_ACMP);

	/* STREAMING_WAIT flag same as in the bind command. */
	if (status == ACMP_STAT_SUCCESS) {
		acmp_rsp->flags |= flags & htons(ACMP_FLAG_STREAMING_WAIT);

		acmp_rsp->flags &= ~htons(ACMP_FLAG_FAST_CONNECT | ACMP_FLAG_REGISTERING_FAILED);

		acmp_rsp->connection_count = htons(1);
	}

	return acmp_send_rsp(acmp, port, desc_rsp, ACMP_BIND_RX_RESPONSE, status);
}

static int acmp_milan_send_probe_tx_command(struct entity *entity, u16 listener_unique_id)
{
	struct acmp_pdu *acmp_cmd;
	struct net_tx_desc *desc_cmd;
	int rc = 0;
	struct acmp_ctx *acmp = &entity->acmp;
	struct avdecc_ctx *avdecc = entity->avdecc;
	struct avdecc_port *port_cmd;
	struct stream_input_dynamic_desc *stream_input_dynamic = aem_get_descriptor(entity->aem_dynamic_descs,
										      AEM_DESC_TYPE_STREAM_INPUT, listener_unique_id, NULL);
	struct stream_descriptor *stream_input = aem_get_descriptor(entity->aem_descs,
										      AEM_DESC_TYPE_STREAM_INPUT, listener_unique_id, NULL);

	/* Send PROBE_TX_COMMAND on the port associated with the STREAM_INPUT descriptor */
	port_cmd = &avdecc->port[ntohs(stream_input->avb_interface_index)];

	desc_cmd = acmp_net_tx_alloc(acmp, port_cmd);
	if (!desc_cmd) {
		rc = -1;
		goto exit;
	}

	acmp_cmd = (struct acmp_pdu *)((char *)NET_DATA_START(desc_cmd) + OFFSET_TO_ACMP);

	copy_64(&acmp_cmd->controller_entity_id, &stream_input_dynamic->controller_entity_id);
	copy_64(&acmp_cmd->talker_entity_id, &stream_input_dynamic->talker_entity_id);
	copy_64(&acmp_cmd->listener_entity_id, &entity->desc->entity_id);

	acmp_cmd->talker_unique_id = stream_input_dynamic->talker_unique_id;
	acmp_cmd->listener_unique_id = htons(listener_unique_id);

	/* Save the sequence_id of the PDU to be sent */
	stream_input_dynamic->u.milan.probe_tx_seq_id = acmp->sequence_id;

	rc = acmp_send_cmd(acmp, port_cmd, acmp_cmd, desc_cmd, ACMP_PROBE_TX_COMMAND, 0, acmp->sequence_id, NULL, 0);

exit:
	return rc;
}

/** Non standard check
	* Our low level network code doesn't allow to receive the same Talker stream in two network sockets,
	* In other words, we don't support multiple Listener streams to connect/bind to a same Talker stream
	* So check if this Talker stream is already bound to another Listener stream
	*/
static bool is_listener_already_bound_to_talker_unique_id(struct entity *entity, u16 listener_unique_id, u64 talker_entity_id, u16 talker_unique_id)
{
	struct stream_input_dynamic_desc *stream_input_dynamic;
	unsigned int num_stream_input;
	int i;

	num_stream_input = aem_get_descriptor_max(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_INPUT);

	for (i = 0; i < num_stream_input; i++) {
		/* Skip ourself */
		if (i == listener_unique_id)
			continue;

		stream_input_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_INPUT, i, NULL);

		if (cmp_64(&stream_input_dynamic->talker_entity_id, &talker_entity_id)
				&& (stream_input_dynamic->talker_unique_id == talker_unique_id)) {

			return true;
		}
	}

	return false;
}

static int acmp_milan_listener_bind(struct entity *entity, u16 listener_unique_id, struct avdecc_port *port, struct acmp_pdu *pdu,
					struct net_tx_desc *desc_rsp, bool *binding_restart)
{
	bool need_binding_restart, streaming_wait_changed, controller_changed;
	struct stream_input_dynamic_desc *stream_input_dynamic = aem_get_descriptor(entity->aem_dynamic_descs,
										      AEM_DESC_TYPE_STREAM_INPUT, listener_unique_id, NULL);
	int rc = 0;

	if (is_listener_already_bound_to_talker_unique_id(entity, listener_unique_id, pdu->talker_entity_id, pdu->talker_unique_id)) {
		acmp_milan_send_bind_rx_response(entity, listener_unique_id, port, desc_rsp, pdu->flags, ACMP_STAT_TALKER_EXCLUSIVE);
		rc = -1;
		goto exit;
	}

	need_binding_restart = acmp_milan_need_binding_restart(stream_input_dynamic, pdu);
	streaming_wait_changed = (stream_input_dynamic->flags != (pdu->flags & htons(ACMP_FLAG_STREAMING_WAIT)));
	controller_changed = !cmp_64(&stream_input_dynamic->controller_entity_id, &pdu->controller_entity_id);

	acmp_milan_set_stream_input_binding_params(stream_input_dynamic, pdu);

	rc = acmp_milan_send_bind_rx_response(entity, listener_unique_id, port, desc_rsp, pdu->flags, ACMP_STAT_SUCCESS);

	if (need_binding_restart) {
		rc |= acmp_milan_send_probe_tx_command(entity, listener_unique_id);

		stream_input_dynamic->u.milan.probing_status = ACMP_PROBING_STATUS_ACTIVE;
		stream_input_dynamic->u.milan.acmp_status = 0;
	}

	if (binding_restart)
		*binding_restart = need_binding_restart;

	/* If any binding params has changed, notifiy upper layer. */
	if (need_binding_restart || streaming_wait_changed || controller_changed)
		acmp_listener_ipc_send_bind_status(entity, listener_unique_id);

	/* Register send of an asynchronous GET_STREAM_INFO unsolicited notification per MILAN Specification v1.2 5.4.5.2
	 * to notify changes in the STREAM_INPUT descriptor state (flag: BOUND and flag:STREAMING_WAIT)
	 */
	if (streaming_wait_changed || controller_changed) {
		acmp_milan_listener_register_async_get_stream_info_notification(stream_input_dynamic);
	}

exit:
	return rc;
}

static int acmp_milan_listener_unbind(struct entity *entity, u16 listener_unique_id, struct avdecc_port *port, struct net_tx_desc *desc_rsp)
{
	struct stream_input_dynamic_desc *stream_input_dynamic = aem_get_descriptor(entity->aem_dynamic_descs,
										      AEM_DESC_TYPE_STREAM_INPUT, listener_unique_id, NULL);
	int rc = 0;

	/* Clear the binding parameters. */
	acmp_milan_set_stream_input_binding_params(stream_input_dynamic, NULL);

	stream_input_dynamic->u.milan.probing_status = ACMP_PROBING_STATUS_DISABLED;
	stream_input_dynamic->u.milan.acmp_status = 0;

	rc = acmp_milan_send_unbind_rx_response(entity, listener_unique_id, port, desc_rsp);

	acmp_listener_ipc_send_unbind_status(entity, listener_unique_id);

	/* Register send of an asynchronous GET_STREAM_INFO unsolicited notification per MILAN Specification v1.2 5.4.5.2
	 * to notify changes in the STREAM_INPUT descriptor state (flag: BOUND and flag:STREAMING_WAIT)
	 */
	acmp_milan_listener_register_async_get_stream_info_notification(stream_input_dynamic);

	return rc;
}

static void acmp_listener_sink_delay_timer_handler(void *data)
{
	struct stream_input_dynamic_desc *stream_input_dynamic = (struct stream_input_dynamic_desc *)data;

	acmp_milan_listener_sink_event(stream_input_dynamic->entity, stream_input_dynamic->unique_id, ACMP_LISTENER_SINK_SM_EVENT_TMR_DELAY);
}

static void acmp_listener_sink_retry_timer_handler(void *data)
{
	struct stream_input_dynamic_desc *stream_input_dynamic = (struct stream_input_dynamic_desc *)data;

	acmp_milan_listener_sink_event(stream_input_dynamic->entity, stream_input_dynamic->unique_id, ACMP_LISTENER_SINK_SM_EVENT_TMR_RETRY);
}

static void acmp_listener_sink_tk_timer_handler(void *data)
{
	struct stream_input_dynamic_desc *stream_input_dynamic = (struct stream_input_dynamic_desc *)data;

	acmp_milan_listener_sink_event(stream_input_dynamic->entity, stream_input_dynamic->unique_id, ACMP_LISTENER_SINK_SM_EVENT_TMR_NO_TK);
}

static void acmp_milan_listener_async_unsolicited_notification_timer_handler(void *data)
{
	struct stream_input_dynamic_desc *stream_input_dynamic = (struct stream_input_dynamic_desc *)data;
	struct entity *entity = stream_input_dynamic->entity;

	aecp_aem_send_async_unsolicited_notification(&entity->aecp, AECP_AEM_CMD_GET_STREAM_INFO, AEM_DESC_TYPE_STREAM_INPUT, stream_input_dynamic->unique_id);
}

__init static int acmp_milan_listener_sink_init_timers(struct entity *entity, u16 listener_unique_id)
{
	struct stream_input_dynamic_desc *stream_input_dynamic = aem_get_descriptor(entity->aem_dynamic_descs,
										      AEM_DESC_TYPE_STREAM_INPUT, listener_unique_id, NULL);

	stream_input_dynamic->u.milan.acmp_retry_timer.func = acmp_listener_sink_retry_timer_handler;
	stream_input_dynamic->u.milan.acmp_retry_timer.data = stream_input_dynamic;

	if (timer_create(entity->avdecc->timer_ctx, &stream_input_dynamic->u.milan.acmp_retry_timer, 0, ACMP_MILAN_LISTENER_TMR_RETRY_GRANULARITY_MS) < 0)
		goto err_retry_timer;

	stream_input_dynamic->u.milan.acmp_delay_timer.func = acmp_listener_sink_delay_timer_handler;
	stream_input_dynamic->u.milan.acmp_delay_timer.data = stream_input_dynamic;

	if (timer_create(entity->avdecc->timer_ctx, &stream_input_dynamic->u.milan.acmp_delay_timer, 0, ACMP_MILAN_LISTENER_TMR_DELAY_GRANULARITY_MS) < 0)
		goto err_delay_timer;

	stream_input_dynamic->u.milan.acmp_talker_registration_timer.func = acmp_listener_sink_tk_timer_handler;
	stream_input_dynamic->u.milan.acmp_talker_registration_timer.data = stream_input_dynamic;

	if (timer_create(entity->avdecc->timer_ctx, &stream_input_dynamic->u.milan.acmp_talker_registration_timer, 0, ACMP_MILAN_LISTENER_TMR_NO_TK_GRANULARITY_MS) < 0)
		goto err_tk_timer;

	stream_input_dynamic->u.milan.async_unsolicited_notification_timer.func = acmp_milan_listener_async_unsolicited_notification_timer_handler;
	stream_input_dynamic->u.milan.async_unsolicited_notification_timer.data = stream_input_dynamic;

	if (timer_create(entity->avdecc->timer_ctx, &stream_input_dynamic->u.milan.async_unsolicited_notification_timer, 0,
				ACMP_MILAN_LISTENER_ASYNC_UNSOLICITED_NOTIFICATION_GRANULARITY_MS) < 0)
		goto err_notification_timer;

	return 0;

err_notification_timer:
	timer_destroy(&stream_input_dynamic->u.milan.acmp_talker_registration_timer);

err_tk_timer:
	timer_destroy(&stream_input_dynamic->u.milan.acmp_delay_timer);

err_delay_timer:
	timer_destroy(&stream_input_dynamic->u.milan.acmp_retry_timer);

err_retry_timer:
	return -1;
}

__exit static void acmp_milan_listener_sink_exit_timers(struct entity *entity, u16 listener_unique_id)
{
	struct stream_input_dynamic_desc *stream_input_dynamic = aem_get_descriptor(entity->aem_dynamic_descs,
										      AEM_DESC_TYPE_STREAM_INPUT, listener_unique_id, NULL);

	timer_destroy(&stream_input_dynamic->u.milan.acmp_retry_timer);
	timer_destroy(&stream_input_dynamic->u.milan.acmp_delay_timer);
	timer_destroy(&stream_input_dynamic->u.milan.acmp_talker_registration_timer);
	timer_destroy(&stream_input_dynamic->u.milan.async_unsolicited_notification_timer);
}

/** Helper function to check if a stream input or output is running
 * \return bool, true if stream is running, false otherwise
 * \param entity
 * \param stream_desc_type, AEM_DESC_TYPE_STREAM_INPUT or AEM_DESC_TYPE_STREAM_OUTPUT
 * \param stream_desc_index, index of the descriptor
 */
bool acmp_milan_is_stream_running(struct entity *entity, u16 stream_desc_type, u16 stream_desc_index)
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
		ret = ACMP_MILAN_IS_LISTENER_SINK_BOUND((struct stream_input_dynamic_desc *)desc);
	} else if (stream_desc_type == AEM_DESC_TYPE_STREAM_OUTPUT) {
		ret = ((struct stream_output_dynamic_desc *)desc)->u.milan.avtp_connected;
	}

out:
	return ret;
}

/** Update streaming_wait flag and send a GENAVB_MSG_MEDIA_STACK_BIND to update the status of the stream
 * \return positive value (0 if nothing changed, 1 if stream descriptor was updated) if successful, negative otherwise
 * \param entity
 * \param stream_desc_type, AEM_DESC_TYPE_STREAM_INPUT
 * \param stream_desc_index, index of the descriptor
 */
int acmp_milan_start_streaming(struct entity *entity, u16 stream_desc_type, u16 stream_desc_index)
{
	struct stream_input_dynamic_desc *stream_in_desc;
	int rc = 0;

	if (stream_desc_type != AEM_DESC_TYPE_STREAM_INPUT) {
		os_log(LOG_ERR, "entity(%p) descriptor type(%u) not supported for Milan start streaming\n", entity, stream_desc_type);
		rc = -1;
		goto exit;
	}

	stream_in_desc = aem_get_descriptor(entity->aem_dynamic_descs, stream_desc_type, stream_desc_index, NULL);
	if (!stream_in_desc) {
		os_log(LOG_ERR, "entity(%p) stream input descriptor(%u) not found.\n", entity, stream_desc_index);
		rc = -1;
		goto exit;
	}

	/* Stream is bound and stopped (streaming_wait flag = 1) */
	if (ACMP_MILAN_IS_LISTENER_SINK_BOUND(stream_in_desc) && (stream_in_desc->flags & htons(ACMP_FLAG_STREAMING_WAIT)) != 0) {
		stream_in_desc->flags &= ~(htons(ACMP_FLAG_STREAMING_WAIT));

		acmp_listener_ipc_send_bind_status(entity, stream_desc_index);

		rc = 1;

		/* Register send of an asynchronous GET_STREAM_INFO unsolicited notification per MILAN Specification v1.2 5.4.5.2
		 * to notify changes in the STREAM_INPUT descriptor state (Started state)
		 */
		acmp_milan_listener_register_async_get_stream_info_notification(stream_in_desc);

		os_log(LOG_DEBUG, "entity(%p) stream input(%u) started.\n", entity, stream_desc_index);
	}

exit:
	return rc;
}

/** Update streaming_wait flag and send a GENAVB_MSG_MEDIA_STACK_BIND to update the status of the stream
 * \return positive value (0 if nothing changed, 1 if stream descriptor was updated) if successful, negative otherwise
 * \param entity
 * \param stream_desc_type, AEM_DESC_TYPE_STREAM_INPUT
 * \param stream_desc_index, index of the descriptor
 */
int acmp_milan_stop_streaming(struct entity *entity, u16 stream_desc_type, u16 stream_desc_index)
{
	struct stream_input_dynamic_desc *stream_in_desc;
	int rc = 0;

	if (stream_desc_type != AEM_DESC_TYPE_STREAM_INPUT) {
		os_log(LOG_ERR, "entity(%p) descriptor type(%u) not supported for Milan stop streaming\n", entity, stream_desc_type);
		rc = -1;
		goto exit;
	}

	stream_in_desc = aem_get_descriptor(entity->aem_dynamic_descs, stream_desc_type, stream_desc_index, NULL);
	if (!stream_in_desc) {
		os_log(LOG_ERR, "entity(%p) stream input descriptor(%u) not found.\n", entity, stream_desc_index);
		rc = -1;
		goto exit;
	}

	/* Stream is bound and started (streaming_wait flag = 0) */
	if (ACMP_MILAN_IS_LISTENER_SINK_BOUND(stream_in_desc) && (stream_in_desc->flags & htons(ACMP_FLAG_STREAMING_WAIT)) == 0) {
		stream_in_desc->flags |= htons(ACMP_FLAG_STREAMING_WAIT);

		acmp_listener_ipc_send_bind_status(entity, stream_desc_index);

		rc = 1;

		/* Register send of an asynchronous GET_STREAM_INFO unsolicited notification per MILAN Specification v1.2 5.4.5.2
		 * to notify changes in the STREAM_INPUT descriptor state (Stopped state)
		 */
		acmp_milan_listener_register_async_get_stream_info_notification(stream_in_desc);

		os_log(LOG_DEBUG, "entity(%p) stream input(%u) stopped.\n", entity, stream_desc_index);
	}

exit:
	return rc;
}

/** Get a listener index matching the stream ID
 * \return                             0 on success, negative otherwise
 * \param      entity                  pointer to entity struct
 * \param      stream_id               stream ID (in network order)
 * \param[out] listener_unique_id      pointer to variable holding the matching listener index on success.
 */
int acmp_milan_get_listener_unique_id(struct entity *entity, u64 stream_id, u16 *listener_unique_id)
{
	int i;
	int rc = -1;
	struct acmp_ctx *acmp = &entity->acmp;
	struct stream_input_dynamic_desc *stream_input_dynamic;

	for (i = 0; i < acmp->max_listener_streams; i++) {
		stream_input_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_INPUT, i, NULL);

		if (cmp_64(&stream_input_dynamic->stream_id, &stream_id) && listener_unique_id) {
			*listener_unique_id = i;
			rc = 0;
			goto exit;
		}
	}

exit:
	return rc;
}

/** Main SRP state machine for a listener sink.
 * This state machine is not explicitly stated in the Milan Spec, but it helps to track the listener
 * srp stream status transitions and their listener sink SM event triggering.
 * \return                             none
 * \param      entity                  pointer to entity struct
 * \param      listener_unique_id      listener index (host order)
 * \param      ipc_listener_status     ipc status message for srp listener
 */
void acmp_milan_listener_srp_state_sm(struct entity *entity, u16 listener_unique_id, struct genavb_msg_listener_status *ipc_listener_status)
{
	genavb_listener_stream_status_t listener_status = ipc_listener_status->status;
	struct stream_input_dynamic_desc *stream_input_dynamic = aem_get_descriptor(entity->aem_dynamic_descs,
										      AEM_DESC_TYPE_STREAM_INPUT, listener_unique_id, NULL);
	u64 stream_id;

	if (!stream_input_dynamic) {
		os_log(LOG_ERR, "entity(%p) listener unique id (%u): unable to get stream input dynamic descriptor\n",
							entity, listener_unique_id);

		return;
	}

	if (!ACMP_MILAN_IS_LISTENER_SINK_SETTLED(stream_input_dynamic))
		return;

	/* Per MILAN Specification v1.2 5.5.3.3: EVT_TK_REGISTERED: check that the talker attribute (Talker advertise or Talker failed)
	 * matches Stream ID, Stream Destination MAC Address and Stream VLAN ID that are associated with the settled stream.
	 */
	if (listener_status == ACTIVE || listener_status == FAILED) {

		stream_id = get_64(ipc_listener_status->stream_id);

		if (!cmp_64(&stream_input_dynamic->stream_id, &stream_id)
			|| (os_memcmp(stream_input_dynamic->stream_dest_mac, ipc_listener_status->params.destination_address, 6))
			|| (stream_input_dynamic->stream_vlan_id != htons(ipc_listener_status->params.vlan_id)))
			return;
	}

	/* Always update the listener stream status parameters independently from the srp state */
	stream_input_dynamic->u.milan.srp_stream_status = listener_status;

	if (listener_status != NO_TALKER) {
		stream_input_dynamic->u.milan.msrp_accumulated_latency = ipc_listener_status->params.accumulated_latency;
	} else {
		stream_input_dynamic->u.milan.msrp_accumulated_latency = 0;
	}

	if (listener_status == FAILED) {
		os_memcpy(stream_input_dynamic->u.milan.failure.bridge_id, ipc_listener_status->failure.bridge_id, 8);
		stream_input_dynamic->u.milan.failure.failure_code = ipc_listener_status->failure.failure_code;
	} else {
		os_memset(stream_input_dynamic->u.milan.failure.bridge_id, 0, 8);
		stream_input_dynamic->u.milan.failure.failure_code = 0;
	}

	switch (stream_input_dynamic->u.milan.srp_state) {
	case ACMP_LISTENER_SINK_SRP_STATE_NOT_REGISTERING: /* The sink is not resgistering any talker attribute */
		if (listener_status == ACTIVE || listener_status == FAILED) {
			stream_input_dynamic->u.milan.srp_state = ACMP_LISTENER_SINK_SRP_STATE_REGISTERING;

			/* EVT_TK_REGISTERED is only triggered on SRP state NOT_REGISTERED -> REGISTERED transition */
			acmp_milan_listener_sink_event(entity, listener_unique_id, ACMP_LISTENER_SINK_SM_EVENT_EVT_TK_REGISTERED);
		}
		break;

	case ACMP_LISTENER_SINK_SRP_STATE_REGISTERING: /* The sink is already resgistering a talker attribute */
		if (listener_status == NO_TALKER) {
			stream_input_dynamic->u.milan.srp_state = ACMP_LISTENER_SINK_SRP_STATE_NOT_REGISTERING;

			/* EVT_TK_UNREGISTERED is only triggered on SRP state REGISTERED -> NOT_REGISTERED transition */
			acmp_milan_listener_sink_event(entity, listener_unique_id, ACMP_LISTENER_SINK_SM_EVENT_EVT_TK_UNREGISTERED);
		}
		break;

	default:
		break;
	}

	/* Register send of an asynchronous GET_STREAM_INFO unsolicited notification per MILAN Specification v1.2 5.4.5.2
	 * to notify changes in the STREAM_INPUT's SRP talker registration state
	 */
	acmp_milan_listener_register_async_get_stream_info_notification(stream_input_dynamic);
}

/** Main ACMP MILAN listener sink state machine (8.3.5)
 * \return                      0 on success, negative otherwise
 * \param entity                pointer to entity struct
 * \param listener_unique_id    Listener unique ID (in host order)
 * \param event	        	ACMP listener SM event (8.3.3)
 * \param pdu                   pointer to the received ACMP PDU if network event, NULL otherwise
 * \param status                status from AVTP control header (8.2.1.6) if network event, 0 otherwise
 * \param port_id		avdecc port / interface index on which we received the PDU
 */
static int acmp_milan_listener_sink_sm(struct entity *entity, u16 listener_unique_id, acmp_milan_listener_sink_sm_event_t event,
						struct acmp_pdu *pdu, u8 status, unsigned int port_id)
{
	acmp_milan_listener_sink_sm_state_t listener_sink_state;
	struct stream_input_dynamic_desc *stream_input_dynamic;
	u8 msg_type = acmp_milan_listener_get_msg_type(event);
	struct avdecc_ctx *avdecc = entity->avdecc;
	struct acmp_ctx *acmp = &entity->acmp;
	struct net_tx_desc *desc_rsp = NULL;
	struct avdecc_port *port_rsp = NULL;
	bool stream_status_changed = false;
	bool binding_restart;
	u16 probe_tx_seq_id;
	int rc = 0;

	/* If we received a listener network command, prepare the response PDU based on the received one */
	if (ACMP_IS_LISTENER_COMMAND(msg_type)) {
		/* Response will always be sent on the same port we received the command from. */
		port_rsp = &avdecc->port[port_id];

		desc_rsp = acmp_net_tx_init(acmp, port_rsp, pdu, true);
		if (!desc_rsp) {
			rc = -1;
			goto exit;
		}
	}

	/* Check the listener sink index validity */
	if (!acmp_listener_unique_valid(acmp, listener_unique_id)) {
		if (ACMP_IS_LISTENER_COMMAND(msg_type)) {
			rc = acmp_send_rsp(acmp, port_rsp, desc_rsp, msg_type + 1, ACMP_STAT_LISTENER_UNKNOWN_ID);
		} else {
			os_log(LOG_ERR, "entity(%p) event %s : wrong listener sink index %u\n",
				entity, acmp_milan_listener_sink_event2string(event), listener_unique_id);
			rc = -1;
		}
		goto exit;
	}

	/* Check the locking status for the binding commands */
	if (ACMP_MILAN_IS_LISTENER_BINDING_COMMAND(msg_type)
		&& avdecc_entity_is_locked(entity, pdu->controller_entity_id)) {

		os_log(LOG_INFO, "entity(%p) listener sink %u: entity locked, ignore event %s\n",
			entity, listener_unique_id, acmp_milan_listener_sink_event2string(event));

		acmp_send_rsp(acmp, port_rsp, desc_rsp, msg_type + 1, ACMP_STAT_CONTROLLER_NOT_AUTHORIZED);
		goto exit;
	}

	stream_input_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_INPUT, listener_unique_id, NULL);
	listener_sink_state = stream_input_dynamic->u.milan.state;

	switch (listener_sink_state) {
	case ACMP_LISTENER_SINK_SM_STATE_UNBOUND: /* The sink is not bound */
		switch (event) {
		case ACMP_LISTENER_SINK_SM_EVENT_RCV_BIND_RX_CMD:
			rc = acmp_milan_listener_bind(entity, listener_unique_id, port_rsp, pdu, desc_rsp, NULL);
			if (rc < 0)
				break;

			/* start the ADP discovery state machine */
			adp_milan_listener_sink_discovery_sm(entity, listener_unique_id, ADP_MILAN_LISTENER_SINK_RESET, 0, NULL);

			/* Reset all stream input's counters when changing state from unbound to bound
			 * As per Milan v1.2, section 5.3.8.10
			 */
			os_memset(&stream_input_dynamic->diagnostic_counters, 0, sizeof(stream_input_dynamic->diagnostic_counters));

			listener_sink_state = ACMP_LISTENER_SINK_SM_STATE_PRB_W_RESP;
			break;

		case ACMP_LISTENER_SINK_SM_EVENT_RCV_GET_RX_STATE:
			rc = acmp_milan_send_get_rx_state_response(entity, listener_unique_id, port_rsp, desc_rsp);
			break;

		case ACMP_LISTENER_SINK_SM_EVENT_RCV_UNBIND_RX_CMD:
			rc = acmp_milan_listener_unbind(entity, listener_unique_id, port_rsp, desc_rsp);

			listener_sink_state = ACMP_LISTENER_SINK_SM_STATE_UNBOUND;
			break;

		case ACMP_LISTENER_SINK_SM_EVENT_SAVED_BINDING_PARAMS:
			/* This event is not explicitely mentioned in the spec but based on 8.3.5.2, start the SM with received saved binding params. */

			/* start the ADP discovery state machine */
			adp_milan_listener_sink_discovery_sm(entity, listener_unique_id, ADP_MILAN_LISTENER_SINK_RESET, 0, NULL);

			stream_input_dynamic->u.milan.probing_status = ACMP_PROBING_STATUS_PASSIVE;
			stream_input_dynamic->u.milan.acmp_status = 0;

			stream_status_changed = true;

			listener_sink_state = ACMP_LISTENER_SINK_SM_STATE_PRB_W_AVAIL;
			break;

		default:
			os_log(LOG_ERR, "entity(%p) listener sink %u state %s: invalid event %s\n",
				entity, listener_unique_id,
				acmp_milan_listener_sink_state2string(listener_sink_state), acmp_milan_listener_sink_event2string(event));
			break;
		}

		break;

	case ACMP_LISTENER_SINK_SM_STATE_PRB_W_AVAIL: /* The sink is probing: waiting for ADP to report that the talker is discovered */
		switch (event) {
		case ACMP_LISTENER_SINK_SM_EVENT_RCV_BIND_RX_CMD:
			rc = acmp_milan_listener_bind(entity, listener_unique_id, port_rsp, pdu, desc_rsp, &binding_restart);

			if (rc < 0 || !binding_restart)
				break;

			/* restart the ADP discovery state machine */
			adp_milan_listener_sink_discovery_sm(entity, listener_unique_id, ADP_MILAN_LISTENER_SINK_RESET, 0, NULL);

			listener_sink_state = ACMP_LISTENER_SINK_SM_STATE_PRB_W_RESP;
			break;

		case ACMP_LISTENER_SINK_SM_EVENT_RCV_GET_RX_STATE:
			rc = acmp_milan_send_get_rx_state_response(entity, listener_unique_id, port_rsp, desc_rsp);
			break;

		case ACMP_LISTENER_SINK_SM_EVENT_RCV_UNBIND_RX_CMD:
			rc = acmp_milan_listener_unbind(entity, listener_unique_id, port_rsp, desc_rsp);

			/* stop the ADP discovery SM */
			adp_milan_listener_sink_discovery_sm(entity, listener_unique_id, ADP_MILAN_LISTENER_SINK_RESET, 0, NULL);

			listener_sink_state = ACMP_LISTENER_SINK_SM_STATE_UNBOUND;
			break;

		case ACMP_LISTENER_SINK_SM_EVENT_EVT_TK_DISCOVERED:
			timer_start(&stream_input_dynamic->u.milan.acmp_delay_timer, ACMP_MILAN_LISTENER_TMR_DELAY_MS);

			stream_input_dynamic->u.milan.probing_status = ACMP_PROBING_STATUS_ACTIVE;
			stream_input_dynamic->u.milan.acmp_status = 0;

			stream_status_changed = true;

			listener_sink_state = ACMP_LISTENER_SINK_SM_STATE_PRB_W_DELAY;
			break;

		case ACMP_LISTENER_SINK_SM_EVENT_SAVED_BINDING_PARAMS:
			break;

		default:
			os_log(LOG_ERR, "entity(%p) listener sink %u state %s: invalid event %s\n",
				entity, listener_unique_id,
				acmp_milan_listener_sink_state2string(listener_sink_state), acmp_milan_listener_sink_event2string(event));
			break;
		}

		break;

	case ACMP_LISTENER_SINK_SM_STATE_PRB_W_DELAY: /* The sink is probing: ready to send PROBE_TX_COMMAND but waiting for the TMR_DELAY */
		switch (event) {
		case ACMP_LISTENER_SINK_SM_EVENT_TMR_DELAY:
			rc = acmp_milan_send_probe_tx_command(entity, listener_unique_id);

			listener_sink_state = ACMP_LISTENER_SINK_SM_STATE_PRB_W_RESP;
			break;

		case ACMP_LISTENER_SINK_SM_EVENT_RCV_BIND_RX_CMD:
			rc = acmp_milan_listener_bind(entity, listener_unique_id, port_rsp, pdu, desc_rsp, &binding_restart);

			if (rc < 0 || !binding_restart)
				break;

			timer_stop(&stream_input_dynamic->u.milan.acmp_delay_timer);

			/* restart the ADP discovery state machine */
			adp_milan_listener_sink_discovery_sm(entity, listener_unique_id, ADP_MILAN_LISTENER_SINK_RESET, 0, NULL);

			listener_sink_state = ACMP_LISTENER_SINK_SM_STATE_PRB_W_RESP;
			break;

		case ACMP_LISTENER_SINK_SM_EVENT_RCV_GET_RX_STATE:
			rc = acmp_milan_send_get_rx_state_response(entity, listener_unique_id, port_rsp, desc_rsp);
			break;

		case ACMP_LISTENER_SINK_SM_EVENT_RCV_UNBIND_RX_CMD:
			/* stop the ADP discovery SM */
			adp_milan_listener_sink_discovery_sm(entity, listener_unique_id, ADP_MILAN_LISTENER_SINK_RESET, 0, NULL);

			/* stop TMR_DELAY */
			timer_stop(&stream_input_dynamic->u.milan.acmp_delay_timer);

			rc = acmp_milan_listener_unbind(entity, listener_unique_id, port_rsp, desc_rsp);

			listener_sink_state = ACMP_LISTENER_SINK_SM_STATE_UNBOUND;
			break;

		case ACMP_LISTENER_SINK_SM_EVENT_EVT_TK_DISCOVERED:
			break;

		case ACMP_LISTENER_SINK_SM_EVENT_EVT_TK_DEPARTED:

			/* stop TMR_DELAY */
			timer_stop(&stream_input_dynamic->u.milan.acmp_delay_timer);

			stream_input_dynamic->u.milan.probing_status = ACMP_PROBING_STATUS_PASSIVE;
			stream_input_dynamic->u.milan.acmp_status = 0;

			stream_status_changed = true;

			listener_sink_state = ACMP_LISTENER_SINK_SM_STATE_PRB_W_AVAIL;
			break;

		case ACMP_LISTENER_SINK_SM_EVENT_SAVED_BINDING_PARAMS:
			break;

		default:
			os_log(LOG_ERR, "entity(%p) listener sink %u state %s: invalid event %s\n",
				entity, listener_unique_id,
				acmp_milan_listener_sink_state2string(listener_sink_state), acmp_milan_listener_sink_event2string(event));
			break;
		}

		break;

	case ACMP_LISTENER_SINK_SM_STATE_PRB_W_RESP: /* The sink is probing: just sent the PROBE_TX_COMMAND and waiting for response */
	case ACMP_LISTENER_SINK_SM_STATE_PRB_W_RESP2: /* The sink is probing: timeout on first PROBE_TX_COMMAND sent a second one and waiting for response */
		switch (event) {
		case ACMP_LISTENER_SINK_SM_EVENT_TMR_NO_RESP:
			if (listener_sink_state == ACMP_LISTENER_SINK_SM_STATE_PRB_W_RESP) {
				/* Called from the inflight callback: first PROBE_TX_COMMAND timedout without
				 * response, let the callback retry the command send
				 */

				listener_sink_state = ACMP_LISTENER_SINK_SM_STATE_PRB_W_RESP2;
			} else {
				/* Called from the inflight callback: second PROBE_TX_COMMAND timedout without
				 * response, start the TMR_RETRY and move to the correspondent state.
				 */
				timer_start(&stream_input_dynamic->u.milan.acmp_retry_timer, ACMP_MILAN_LISTENER_TMR_RETRY_MS);

				stream_input_dynamic->u.milan.acmp_status = ACMP_STAT_LISTENER_TALKER_TIMEOUT;

				stream_status_changed = true;

				listener_sink_state = ACMP_LISTENER_SINK_SM_STATE_PRB_W_RETRY;
			}
			break;

		case ACMP_LISTENER_SINK_SM_EVENT_RCV_BIND_RX_CMD:
			probe_tx_seq_id = stream_input_dynamic->u.milan.probe_tx_seq_id;

			rc = acmp_milan_listener_bind(entity, listener_unique_id, port_rsp, pdu, desc_rsp, &binding_restart);

			if (rc < 0 || !binding_restart)
				break;

			/* stop timer TMR_NO_RESP */
			if (avdecc_inflight_cancel(entity, &acmp->inflight, probe_tx_seq_id, NULL, NULL, NULL) < 0)
				os_log(LOG_ERR, "entity(%p) listener sink %u state %s event %s: Could not find an inflight with sequence_id %u\n",
					entity, listener_unique_id, acmp_milan_listener_sink_state2string(listener_sink_state), acmp_milan_listener_sink_event2string(event),
					stream_input_dynamic->u.milan.probe_tx_seq_id);

			/* restart ADP discovery SM */
			adp_milan_listener_sink_discovery_sm(entity, listener_unique_id, ADP_MILAN_LISTENER_SINK_RESET, 0, NULL);

			listener_sink_state = ACMP_LISTENER_SINK_SM_STATE_PRB_W_RESP;
			break;

		case ACMP_LISTENER_SINK_SM_EVENT_RCV_PROBE_TX_RESP:
			//  FIXME the avdecc_inflight checks only the sequence id, we should check also the controller entity id, talker entity id and index
			if (avdecc_inflight_cancel(entity, &acmp->inflight, ntohs(pdu->sequence_id), NULL, NULL, NULL) < 0) {
				os_log(LOG_ERR, "entity(%p) listener sink %u: Could not find an inflight with sequence_id %u\n",
					entity, listener_unique_id, ntohs(pdu->sequence_id));
				rc = -1;
				goto exit;
			}

			if (status != ACMP_STAT_SUCCESS) {
				stream_input_dynamic->u.milan.acmp_status = status;

				stream_status_changed = true;

				/* start TMR_RETRY */
				timer_start(&stream_input_dynamic->u.milan.acmp_retry_timer, ACMP_MILAN_LISTENER_TMR_RETRY_MS);

				listener_sink_state = ACMP_LISTENER_SINK_SM_STATE_PRB_W_RETRY;
			} else {
				/* Save the received SRP params */
				acmp_milan_set_stream_input_srp_params(stream_input_dynamic, pdu);

				/* Stack connect: AVTP connect and SRP register */
				if (acmp_listener_stack_connect(entity, listener_unique_id, ntohs(pdu->flags)) != ACMP_STAT_SUCCESS)
					rc = -1;

				/* start TMR_NO_TK */
				timer_start(&stream_input_dynamic->u.milan.acmp_talker_registration_timer, ACMP_MILAN_LISTENER_TMR_NO_TK_MS);

				stream_input_dynamic->u.milan.probing_status = ACMP_PROBING_STATUS_COMPLETED;
				stream_input_dynamic->u.milan.acmp_status = 0;

				stream_status_changed = true;

				listener_sink_state = ACMP_LISTENER_SINK_SM_STATE_SETTLED_NO_RSV;
			}
			break;

		case ACMP_LISTENER_SINK_SM_EVENT_RCV_GET_RX_STATE:
			rc = acmp_milan_send_get_rx_state_response(entity, listener_unique_id, port_rsp, desc_rsp);
			break;

		case ACMP_LISTENER_SINK_SM_EVENT_RCV_UNBIND_RX_CMD:
			/* stop the ADP discovery SM */
			adp_milan_listener_sink_discovery_sm(entity, listener_unique_id, ADP_MILAN_LISTENER_SINK_RESET, 0, NULL);

			/* stop TMR_NO_RESP */
			if (avdecc_inflight_cancel(entity, &acmp->inflight, stream_input_dynamic->u.milan.probe_tx_seq_id, NULL, NULL, NULL) < 0)
				os_log(LOG_ERR, "entity(%p) listener sink %u state %s event %s: Could not find an inflight with sequence_id %u\n",
					entity, listener_unique_id, acmp_milan_listener_sink_state2string(listener_sink_state), acmp_milan_listener_sink_event2string(event),
					stream_input_dynamic->u.milan.probe_tx_seq_id);

			rc = acmp_milan_listener_unbind(entity, listener_unique_id, port_rsp, desc_rsp);

			listener_sink_state = ACMP_LISTENER_SINK_SM_STATE_UNBOUND;
			break;

		case ACMP_LISTENER_SINK_SM_EVENT_EVT_TK_DISCOVERED:
			break;

		case ACMP_LISTENER_SINK_SM_EVENT_EVT_TK_DEPARTED:
			/* stop TMR_NO_RESP */
			if (avdecc_inflight_cancel(entity, &acmp->inflight, stream_input_dynamic->u.milan.probe_tx_seq_id, NULL, NULL, NULL) < 0)
				os_log(LOG_ERR, "entity(%p) listener sink %u state %s event %s: Could not find an inflight with sequence_id %u\n",
					entity, listener_unique_id, acmp_milan_listener_sink_state2string(listener_sink_state), acmp_milan_listener_sink_event2string(event),
					stream_input_dynamic->u.milan.probe_tx_seq_id);

			stream_input_dynamic->u.milan.probing_status = ACMP_PROBING_STATUS_PASSIVE;
			stream_input_dynamic->u.milan.acmp_status = 0;

			stream_status_changed = true;

			listener_sink_state = ACMP_LISTENER_SINK_SM_STATE_PRB_W_AVAIL;
			break;

		case ACMP_LISTENER_SINK_SM_EVENT_SAVED_BINDING_PARAMS:
			break;

		default:
			os_log(LOG_ERR, "entity(%p) listener sink %u state %s: invalid event %s\n",
				entity, listener_unique_id,
				acmp_milan_listener_sink_state2string(listener_sink_state), acmp_milan_listener_sink_event2string(event));
			break;
		}

		break;

	case ACMP_LISTENER_SINK_SM_STATE_PRB_W_RETRY: /* The sink is probing: timeout on the two PROBE_TX_COMMAND and waiting for TMR_RETRY before retrying */
		switch (event) {
		case ACMP_LISTENER_SINK_SM_EVENT_TMR_RETRY:
			if (stream_input_dynamic->u.milan.talker_state == ADP_LISTENER_SINK_TALKER_STATE_NOT_DISCOVERED) {
				stream_input_dynamic->u.milan.probing_status = ACMP_PROBING_STATUS_PASSIVE;
				stream_input_dynamic->u.milan.acmp_status = 0;

				stream_status_changed = true;

				listener_sink_state = ACMP_LISTENER_SINK_SM_STATE_PRB_W_AVAIL;
			} else {
				/* start TMR_DELAY */
				timer_start(&stream_input_dynamic->u.milan.acmp_delay_timer, ACMP_MILAN_LISTENER_TMR_DELAY_MS);

				listener_sink_state = ACMP_LISTENER_SINK_SM_STATE_PRB_W_DELAY;
			}

			break;

		case ACMP_LISTENER_SINK_SM_EVENT_RCV_BIND_RX_CMD:
			rc = acmp_milan_listener_bind(entity, listener_unique_id, port_rsp, pdu, desc_rsp, &binding_restart);

			if (rc < 0 || !binding_restart)
				break;

			/* stop timer TMR_RETRY */
			timer_stop(&stream_input_dynamic->u.milan.acmp_retry_timer);

			/* Restart the ADP discovery SM */
			adp_milan_listener_sink_discovery_sm(entity, listener_unique_id, ADP_MILAN_LISTENER_SINK_RESET, 0, NULL);

			listener_sink_state = ACMP_LISTENER_SINK_SM_STATE_PRB_W_RESP;
			break;

		case ACMP_LISTENER_SINK_SM_EVENT_RCV_GET_RX_STATE:
			rc = acmp_milan_send_get_rx_state_response(entity, listener_unique_id, port_rsp, desc_rsp);
			break;

		case ACMP_LISTENER_SINK_SM_EVENT_RCV_UNBIND_RX_CMD:
			/* stop the ADP discovery SM */
			adp_milan_listener_sink_discovery_sm(entity, listener_unique_id, ADP_MILAN_LISTENER_SINK_RESET, 0, NULL);
			/* stop TMR_RETRY */
			timer_stop(&stream_input_dynamic->u.milan.acmp_retry_timer);
			rc = acmp_milan_listener_unbind(entity, listener_unique_id, port_rsp, desc_rsp);

			listener_sink_state = ACMP_LISTENER_SINK_SM_STATE_UNBOUND;
			break;

		case ACMP_LISTENER_SINK_SM_EVENT_EVT_TK_DISCOVERED:
			break;

		case ACMP_LISTENER_SINK_SM_EVENT_EVT_TK_DEPARTED:
			/* stop TMR_RETRY */
			timer_stop(&stream_input_dynamic->u.milan.acmp_retry_timer);

			stream_input_dynamic->u.milan.probing_status = ACMP_PROBING_STATUS_PASSIVE;
			stream_input_dynamic->u.milan.acmp_status = 0;

			stream_status_changed = true;

			listener_sink_state = ACMP_LISTENER_SINK_SM_STATE_PRB_W_AVAIL;
			break;

		case ACMP_LISTENER_SINK_SM_EVENT_SAVED_BINDING_PARAMS:
			break;

		default:
			os_log(LOG_ERR, "entity(%p) listener sink %u state %s: invalid event %s\n",
				entity, listener_unique_id,
				acmp_milan_listener_sink_state2string(listener_sink_state), acmp_milan_listener_sink_event2string(event));
			break;
		}

		break;

	case ACMP_LISTENER_SINK_SM_STATE_SETTLED_NO_RSV: /* The sink is settled (has up-to-date SRP params: from PROBE_RX_RESPONSE): waiting SRP talker attribute */
		switch (event) {
		case ACMP_LISTENER_SINK_SM_EVENT_TMR_NO_TK:
			/* Stack disconnect: AVTP disconnect  and SRP deregister */
			if (acmp_listener_stack_disconnect(entity, listener_unique_id) != ACMP_STAT_SUCCESS)
				rc = -1;

			/* Clear SRP params */
			acmp_milan_listener_sink_srp_state_clear(stream_input_dynamic);

			if (stream_input_dynamic->u.milan.talker_state == ADP_LISTENER_SINK_TALKER_STATE_NOT_DISCOVERED) {
				stream_input_dynamic->u.milan.probing_status = ACMP_PROBING_STATUS_PASSIVE;
				stream_input_dynamic->u.milan.acmp_status = 0;

				listener_sink_state = ACMP_LISTENER_SINK_SM_STATE_PRB_W_AVAIL;
			} else {
				stream_input_dynamic->u.milan.probing_status = ACMP_PROBING_STATUS_ACTIVE;
				stream_input_dynamic->u.milan.acmp_status = 0;

				/* start TMR_DELAY */
				timer_start(&stream_input_dynamic->u.milan.acmp_delay_timer, ACMP_MILAN_LISTENER_TMR_DELAY_MS);

				listener_sink_state = ACMP_LISTENER_SINK_SM_STATE_PRB_W_DELAY;
			}

			stream_status_changed = true;

			break;

		case ACMP_LISTENER_SINK_SM_EVENT_RCV_BIND_RX_CMD:
			rc = acmp_milan_listener_bind(entity, listener_unique_id, port_rsp, pdu, desc_rsp, &binding_restart);

			if (rc < 0 || !binding_restart)
				break;

			/* Stack disconnect: AVTP disconnect  and SRP deregister */
			if (acmp_listener_stack_disconnect(entity, listener_unique_id) != ACMP_STAT_SUCCESS)
				rc = -1;

			/* Clear SRP params */
			acmp_milan_listener_sink_srp_state_clear(stream_input_dynamic);

			/* stop TMR_NO_TK */
			timer_stop(&stream_input_dynamic->u.milan.acmp_talker_registration_timer);

			/* Restart the ADP discovery state machine */
			adp_milan_listener_sink_discovery_sm(entity, listener_unique_id, ADP_MILAN_LISTENER_SINK_RESET, 0, NULL);

			listener_sink_state = ACMP_LISTENER_SINK_SM_STATE_PRB_W_RESP;
			break;

		case ACMP_LISTENER_SINK_SM_EVENT_RCV_GET_RX_STATE:
			rc = acmp_milan_send_get_rx_state_response(entity, listener_unique_id, port_rsp, desc_rsp);
			break;

		case ACMP_LISTENER_SINK_SM_EVENT_RCV_UNBIND_RX_CMD:
			/* Stack disconnect: AVTP disconnect  and SRP deregister */
			if (acmp_listener_stack_disconnect(entity, listener_unique_id) != ACMP_STAT_SUCCESS)
				rc = -1;

			/* Clear SRP params */
			acmp_milan_listener_sink_srp_state_clear(stream_input_dynamic);

			/* stop the ADP discovery SM */
			adp_milan_listener_sink_discovery_sm(entity, listener_unique_id, ADP_MILAN_LISTENER_SINK_RESET, 0, NULL);

			/* stop TMR_NO_TK */
			timer_stop(&stream_input_dynamic->u.milan.acmp_talker_registration_timer);

			rc |= acmp_milan_listener_unbind(entity, listener_unique_id, port_rsp, desc_rsp);

			listener_sink_state = ACMP_LISTENER_SINK_SM_STATE_UNBOUND;
			break;

		case ACMP_LISTENER_SINK_SM_EVENT_EVT_TK_DISCOVERED:
			break;

		case ACMP_LISTENER_SINK_SM_EVENT_EVT_TK_DEPARTED:
			break;

		case ACMP_LISTENER_SINK_SM_EVENT_EVT_TK_REGISTERED:
			/* stop TMR_NO_TK */
			timer_stop(&stream_input_dynamic->u.milan.acmp_talker_registration_timer);

			listener_sink_state = ACMP_LISTENER_SINK_SM_STATE_SETTLED_RSV_OK;
			break;

		case ACMP_LISTENER_SINK_SM_EVENT_SAVED_BINDING_PARAMS:
			break;

		default:
			os_log(LOG_ERR, "entity(%p) listener sink %u state %s: invalid event %s\n",
				entity, listener_unique_id,
				acmp_milan_listener_sink_state2string(listener_sink_state), acmp_milan_listener_sink_event2string(event));
			break;
		}
		break;

	case ACMP_LISTENER_SINK_SM_STATE_SETTLED_RSV_OK: /* The sink is settled: has registered SRP talker attribute */
		switch (event) {
		case ACMP_LISTENER_SINK_SM_EVENT_RCV_BIND_RX_CMD:
			rc = acmp_milan_listener_bind(entity, listener_unique_id, port_rsp, pdu, desc_rsp, &binding_restart);

			if (rc < 0 || !binding_restart)
				break;

			/* Stack disconnect: AVTP disconnect  and SRP deregister */
			if (acmp_listener_stack_disconnect(entity, listener_unique_id) != ACMP_STAT_SUCCESS)
				rc = -1;

			/* Clear SRP params */
			acmp_milan_listener_sink_srp_state_clear(stream_input_dynamic);

			/* Restart the ADP discovery state machine */
			adp_milan_listener_sink_discovery_sm(entity, listener_unique_id, ADP_MILAN_LISTENER_SINK_RESET, 0, NULL);

			listener_sink_state = ACMP_LISTENER_SINK_SM_STATE_PRB_W_RESP;
			break;

		case ACMP_LISTENER_SINK_SM_EVENT_RCV_GET_RX_STATE:
			rc = acmp_milan_send_get_rx_state_response(entity, listener_unique_id, port_rsp, desc_rsp);
			break;

		case ACMP_LISTENER_SINK_SM_EVENT_RCV_UNBIND_RX_CMD:
			/* Stack disconnect: AVTP disconnect  and SRP deregister */
			rc = acmp_listener_stack_disconnect(entity, listener_unique_id);

			/* Clear SRP params */
			acmp_milan_listener_sink_srp_state_clear(stream_input_dynamic);

			/* stop the ADP discovery SM */
			adp_milan_listener_sink_discovery_sm(entity, listener_unique_id, ADP_MILAN_LISTENER_SINK_RESET, 0, NULL);

			rc |= acmp_milan_listener_unbind(entity, listener_unique_id, port_rsp, desc_rsp);

			listener_sink_state = ACMP_LISTENER_SINK_SM_STATE_UNBOUND;
			break;

		case ACMP_LISTENER_SINK_SM_EVENT_EVT_TK_DISCOVERED:
			break;

		case ACMP_LISTENER_SINK_SM_EVENT_EVT_TK_DEPARTED:
			break;

		case ACMP_LISTENER_SINK_SM_EVENT_EVT_TK_UNREGISTERED:
			/* Stack disconnect: AVTP disconnect  and SRP deregister */
			rc = acmp_listener_stack_disconnect(entity, listener_unique_id);

			/* Clear SRP params */
			acmp_milan_listener_sink_srp_state_clear(stream_input_dynamic);

			if (stream_input_dynamic->u.milan.talker_state == ADP_LISTENER_SINK_TALKER_STATE_NOT_DISCOVERED) {
				stream_input_dynamic->u.milan.probing_status = ACMP_PROBING_STATUS_PASSIVE;
				stream_input_dynamic->u.milan.acmp_status = 0;

				listener_sink_state = ACMP_LISTENER_SINK_SM_STATE_PRB_W_AVAIL;
			} else {
				stream_input_dynamic->u.milan.probing_status = ACMP_PROBING_STATUS_ACTIVE;
				stream_input_dynamic->u.milan.acmp_status = 0;
				/* start TMR_DELAY */
				timer_start(&stream_input_dynamic->u.milan.acmp_delay_timer, ACMP_MILAN_LISTENER_TMR_DELAY_MS);

				listener_sink_state = ACMP_LISTENER_SINK_SM_STATE_PRB_W_DELAY;
			}

			stream_status_changed = true;

			break;

		case ACMP_LISTENER_SINK_SM_EVENT_SAVED_BINDING_PARAMS:
			break;

		default:
			os_log(LOG_ERR, "entity(%p) listener sink %u state %s: invalid event %s\n",
				entity, listener_unique_id,
				acmp_milan_listener_sink_state2string(listener_sink_state), acmp_milan_listener_sink_event2string(event));
			break;
		}
		break;

	default:
		break;
	}

	os_log(LOG_INFO, "entity(%p) listener sink (%u) : event %s, state from %s to %s\n",
		entity, listener_unique_id,
		acmp_milan_listener_sink_event2string(event),
		acmp_milan_listener_sink_state2string(stream_input_dynamic->u.milan.state), acmp_milan_listener_sink_state2string(listener_sink_state));

	stream_input_dynamic->u.milan.state = listener_sink_state;

	if (stream_status_changed) {
		/* Register send of an asynchronous GET_STREAM_INFO unsolicited notification per MILAN Specification v1.2 5.4.5.2
		 * to notify changes in the STREAM_INPUT descriptor state (Probing and ACMP status)
		 */
		acmp_milan_listener_register_async_get_stream_info_notification(stream_input_dynamic);
	}

exit:
	return rc;
}

/** Main ACMP MILAN listener receive function.
 * \return                      0 on success, negative otherwise
 * \param acmp                  pointer to the ACMP context
 * \param pdu                   pointer to the ACMP PDU
 * \param msg_type	        ACMP message type (8.2.1.5)
 * \param status                status from AVTP control header (8.2.1.6)
 * \param port_id               port on which the PDU is received
 */
int acmp_milan_listener_rcv(struct acmp_ctx *acmp, struct acmp_pdu *pdu, u8 msg_type, u8 status, unsigned int port_id)
{
	struct entity *entity = container_of(acmp, struct entity, acmp);
	acmp_milan_listener_sink_sm_event_t event = acmp_milan_listener_get_event(msg_type);

	if (ACMP_IS_COMMAND(msg_type))
		os_log(LOG_INFO, "acmp(%p) %s: controller(%016"PRIx64") listener(%016"PRIx64", %u) talker(%016"PRIx64", %u)\n",
			acmp, acmp_milan_msgtype2string(msg_type), ntohll(pdu->controller_entity_id),
			ntohll(entity->desc->entity_id), ntohs(pdu->listener_unique_id),
			ntohll(pdu->talker_entity_id), ntohs(pdu->talker_unique_id));
	else
		os_log(LOG_INFO, "acmp(%p) %s: controller(%016"PRIx64") listener(%016"PRIx64", %u) talker(%016"PRIx64", %u) status(%d)\n",
			acmp, acmp_milan_msgtype2string(msg_type), ntohll(pdu->controller_entity_id),
			ntohll(entity->desc->entity_id), ntohs(pdu->listener_unique_id),
			ntohll(pdu->talker_entity_id), ntohs(pdu->talker_unique_id), status);


	return acmp_milan_listener_sink_sm(entity, ntohs(pdu->listener_unique_id), event, pdu, status, port_id);
}

/** ACMP MILAN listener sink event trigger function (non networking events).
 * \return                      0 on success, negative otherwise
 * \param entity                  pointer to the entity struct
 * \param listener_unique_id    Listener unique ID
 * \param event	                ACMP listener sink SM event
 */
int acmp_milan_listener_sink_event(struct entity *entity, u16 listener_unique_id, acmp_milan_listener_sink_sm_event_t event)
{
	return acmp_milan_listener_sink_sm(entity, listener_unique_id, event, NULL, 0, 0);
}

/** ACMP MILAN listener sink receive saved binding params.
 * \return                      0 on success, negative otherwise
 * \param entity                pointer to the entity struct
 * \param listener_unique_id    Listener unique ID
 * \param binding_params        pointer to the genavb_msg_media_stack_bind struct containing the saved binding params
 */
int acmp_milan_listener_sink_rcv_binding_params(struct entity *entity, struct genavb_msg_media_stack_bind *binding_params)
{

	struct stream_input_dynamic_desc *stream_input_dynamic;

	if (!binding_params)
		goto err;

	stream_input_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_INPUT, binding_params->listener_stream_index, NULL);

	if (!stream_input_dynamic)
		goto err;

	/* Sanity checks */
	if (!binding_params->talker_entity_id || !binding_params->controller_entity_id)
		goto err;

	/* Set the saved parameters. */
	stream_input_dynamic->controller_entity_id = htonll(binding_params->controller_entity_id);
	stream_input_dynamic->talker_entity_id = htonll(binding_params->talker_entity_id);
	stream_input_dynamic->talker_unique_id = htons(binding_params->talker_stream_index);
	stream_input_dynamic->flags = (binding_params->started == ACMP_LISTENER_STREAM_STOPPED) ? htons(ACMP_FLAG_STREAMING_WAIT) : htons(0);

	return acmp_milan_listener_sink_sm(entity, binding_params->listener_stream_index, ACMP_LISTENER_SINK_SM_EVENT_SAVED_BINDING_PARAMS, NULL, 0, 0);

err:
	return -1;
}

/** Start a timer to send an async unsolicited notification after a short delay if the timer is not currently running
 * \return none
 * \param stream_output_dynamic, the dynamic descriptor of the stream output that triggered the notification
 */
static void acmp_milan_talker_register_async_get_stream_info_notification(struct stream_output_dynamic_desc *stream_output_dynamic)
{
	if (!timer_is_running(&stream_output_dynamic->u.milan.async_unsolicited_notification_timer))
		timer_start(&stream_output_dynamic->u.milan.async_unsolicited_notification_timer, ACMP_MILAN_TALKER_ASYNC_UNSOLICITED_NOTIFICATION_MS);
}

/** Get a talker index matching the stream ID
 * \return                             0 on success, negative otherwise
 * \param      entity                  pointer to entity struct
 * \param      stream_id               stream ID (in network order)
 * \param[out] talker_unique_id        pointer to variable holding the matching talker index on success.
 */
int acmp_milan_get_talker_unique_id(struct entity *entity, u64 stream_id, u16 *talker_unique_id)
{
	int i;
	struct acmp_ctx *acmp = &entity->acmp;
	struct stream_output_dynamic_desc *stream_output_dynamic;

	for (i = 0; i < acmp->max_talker_streams; i++) {
		stream_output_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_OUTPUT, i, NULL);
		if (cmp_64(&stream_output_dynamic->stream_id, &stream_id) && talker_unique_id) {

			*talker_unique_id = i;
			return 0;
		}
	}

	return -1;
}

/** Check if we have valid SRP params and  needs to advertise talker attribute
 * \return                            True if all SRP params are valid and we need to advertise talker attribute.
 * \param stream_output_dynamic       pointer to the stream_output_dynamic_desc structure
 */
static bool acmp_milan_talker_has_valid_srp_params(struct stream_output_dynamic_desc *stream_output_dynamic)
{
	bool ret = false;

	/* Per MILAN Specification v1.2 (5.3.7.2 and 4.3.3.1): We need to declare the SRP talker, once SRP params are valid:
	 *     - If we already received a new valid MAAP address
	 *     - we have received a PROBE_TX_COMMAND in the last 15 sec or registered a listener attribute
	 */
	if (!is_invalid_mac_addr(stream_output_dynamic->stream_dest_mac) &&
		(stream_output_dynamic->u.milan.probe_tx_valid ||
		(stream_output_dynamic->u.milan.srp_listener_status != NO_LISTENER))) {

		ret = true;
	}

	return ret;
}

/** Save the SRP parameters of the Talker that is withdrawing
 * \return				none
 * \param stream_output_dynamic		pointer to the stream_output_dynamic_desc structure
 * \param valid 			validity flag for the SRP params, if valid save them otherwise reset them
 */
static void acmp_milan_set_stream_output_withdraw_srp_params(struct stream_output_dynamic_desc *stream_output_dynamic, bool valid)
{
	if (valid) {
		os_memcpy(&stream_output_dynamic->u.milan.srp_talker_withdraw_params.stream_id, &stream_output_dynamic->stream_id, 8);
		stream_output_dynamic->u.milan.srp_talker_withdraw_params.vlan_id = stream_output_dynamic->stream_vlan_id;
		os_memcpy(&stream_output_dynamic->u.milan.srp_talker_withdraw_params.stream_dest_mac, &stream_output_dynamic->stream_dest_mac, 6);
	} else {
		os_memset(&stream_output_dynamic->u.milan.srp_talker_withdraw_params, 0, sizeof(struct srp_talker_params));
	}
}

/** Check if we are advertising the same talker attributes we previously withdrawn
 * \return                            True if the talker attributes are the same as the ones last advertised.
 * \param stream_output_dynamic       pointer to the stream_output_dynamic_desc structure
 */
static bool acmp_milan_redeclaring_same_talker(struct stream_output_dynamic_desc *stream_output_dynamic)
{
	bool ret = false;

	if (!os_memcmp(&stream_output_dynamic->u.milan.srp_talker_withdraw_params.stream_id, &stream_output_dynamic->stream_id, 8) &&
	    !os_memcmp(&stream_output_dynamic->u.milan.srp_talker_withdraw_params.stream_dest_mac, &stream_output_dynamic->stream_dest_mac, 6) &&
	    (stream_output_dynamic->u.milan.srp_talker_withdraw_params.vlan_id == stream_output_dynamic->stream_vlan_id)) {
		ret = true;
	}

	return ret;
}

/** Checks Talker status and perform stack connection/disconnection if needed.
 * \return                      ACMP status
 * \param entity                pointer to the entity context
 * \param talker_unique_id      valid talker unique id (in host order)
 */
static u8 acmp_milan_talker_update(struct entity *entity, u16 talker_unique_id)
{
	struct stream_output_dynamic_desc *stream_output_dynamic;
	struct avdecc_ctx *avdecc = entity->avdecc;
	struct stream_descriptor *stream_output;
	u8 status = ACMP_STAT_SUCCESS;
	unsigned int port_id;

	stream_output_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_OUTPUT, talker_unique_id, NULL);
	if (!stream_output_dynamic) {
		os_log(LOG_ERR, "entity(%p) talker unique id (%u): unable to get stream output dynamic descriptor\n",
					entity, talker_unique_id);
		status = ACMP_STAT_TALKER_MISBEHAVING;
		goto exit;
	}

	stream_output = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_STREAM_OUTPUT, talker_unique_id, NULL);
	if (!stream_output) {
		os_log(LOG_ERR, "acmp(%p) cannot find descriptor type %d index %u\n",
			&entity->acmp, AEM_DESC_TYPE_STREAM_OUTPUT, talker_unique_id);
		status = ACMP_STAT_TALKER_MISBEHAVING;
		goto exit;
	}

	port_id = ntohs(stream_output->avb_interface_index);

	/* Per MILAN Specification v1.2 4.3.3.1: If no Valid SRP params, withdraw any previously declared talker.
	 * (i.e avoid declaring talker attributes for Streams that are not requested by any Listener)
	 */
	if (!acmp_milan_talker_has_valid_srp_params(stream_output_dynamic) && stream_output_dynamic->u.milan.talker_declared) {
		if (acmp_talker_withdraw(avdecc, port_id, stream_output_dynamic) < 0) {
			os_log(LOG_ERR, "acmp(%p) cannot disconnect SRP\n", &entity->acmp);
			status = ACMP_STAT_TALKER_MISBEHAVING;
			goto exit;
		}

		/* Save deregistered talker attributes */
		acmp_milan_set_stream_output_withdraw_srp_params(stream_output_dynamic, true);

		stream_output_dynamic->u.milan.talker_declared = false;

		/* start the srp talker withdraw timer */
		stream_output_dynamic->u.milan.srp_talker_withdraw_in_progress = true;
		timer_start(&stream_output_dynamic->u.milan.srp_talker_withdraw_timer, ACMP_MILAN_TALKER_TMR_SRP_WITHDRAW_MS);
	}

	/* Per MILAN Specification v1.2 5.3.7.3 Streaming state: Stream is running when declaring
	 * a Talker Advertise attribute and receiving a Listener Ready or Listener Ready Failed
	 */
	if ((!stream_output_dynamic->u.milan.talker_declared || (stream_output_dynamic->u.milan.srp_listener_status == NO_LISTENER))
	    && stream_output_dynamic->u.milan.avtp_connected) {
		if (acmp_talker_avtp_stack_disconnect(avdecc, port_id, talker_unique_id, stream_output_dynamic) < 0) {
			os_log(LOG_ERR, "acmp(%p) cannot disconnect AVTP\n", &entity->acmp);
			status = ACMP_STAT_TALKER_MISBEHAVING;
			goto exit;
		}

		stream_output_dynamic->u.milan.avtp_connected = false;
	}

	/* Valid SRP params, SRP declare if no previous withdraw in progress then check for AVTP connect */
	if (acmp_milan_talker_has_valid_srp_params(stream_output_dynamic)) {
		if (!stream_output_dynamic->u.milan.talker_declared) {
			/* Per MILAN Specification v1.2 5.3.7.5: If we have a running SRP talker withdraw timer,
			 * we should wait for the 2 LeaveALL period before re-declaring the talker attribute with different parameters.
			 */
			if (stream_output_dynamic->u.milan.srp_talker_withdraw_in_progress) {
				if (!acmp_milan_redeclaring_same_talker(stream_output_dynamic))
					goto exit;

				stream_output_dynamic->u.milan.srp_talker_withdraw_in_progress = false;
				timer_stop(&stream_output_dynamic->u.milan.srp_talker_withdraw_timer);
			}

			if (acmp_talker_declare(avdecc, port_id, stream_output, stream_output_dynamic, 0) < 0) {
				os_log(LOG_ERR, "acmp(%p) cannot connect SRP\n", &entity->acmp);
				status = ACMP_STAT_TALKER_MISBEHAVING;
				goto exit;
			}

			stream_output_dynamic->u.milan.talker_declared = true;

			/* Reset withdraw SRP talker params */
			acmp_milan_set_stream_output_withdraw_srp_params(stream_output_dynamic, false);
		}

		/* Per MILAN Specification v1.2 5.3.7.3 Streaming state: Stream is running when declaring a Talker Advertise attribute
		 * and receiving a Listener Ready or Listener Ready Failed attribute
		 */
		if (ACMP_MILAN_TALKER_HAS_ACTIVE_LISTENER(stream_output_dynamic) && !stream_output_dynamic->u.milan.avtp_connected
		    && stream_output_dynamic->u.milan.talker_declared) {
			/* If Listener attributes are registered as ACTIVE or ACTIVE_AND_FAILED, connect AVTP too */
			if (acmp_talker_avtp_stack_connect(entity, port_id, talker_unique_id, stream_output, stream_output_dynamic, 0) < 0) {
				os_log(LOG_ERR, "acmp(%p) cannot connect AVTP\n", &entity->acmp);
				status = ACMP_STAT_TALKER_MISBEHAVING;
				goto exit;
			}

			stream_output_dynamic->u.milan.avtp_connected = true;
		}
	}

exit:
	return status;
}

/** Updates the SRP talker declaration for the specific stream output
 * \return                             none
 * \param      entity                  pointer to entity struct
 * \param      talker_unique_id        talker_unique_id (host order)
 * \param      ipc_talker_declaration_status       ipc status message for talker stream declaration
 */
void acmp_milan_talker_update_declaration(struct entity *entity, u16 talker_unique_id, struct genavb_msg_talker_declaration_status *ipc_talker_declaration_status)
{
	genavb_talker_stream_declaration_type_t declaration_type = ipc_talker_declaration_status->declaration_type;
	struct stream_output_dynamic_desc *stream_output_dynamic = aem_get_descriptor(entity->aem_dynamic_descs,
											AEM_DESC_TYPE_STREAM_OUTPUT, talker_unique_id, NULL);

	if (!stream_output_dynamic) {
		os_log(LOG_ERR, "entity(%p) talker unique id (%u): unable to get stream output dynamic descriptor\n",
							entity, talker_unique_id);

		return;
	}

	stream_output_dynamic->u.milan.srp_talker_declaration_type = declaration_type;

	if (declaration_type == TALKER_FAILED) {
		os_memcpy(stream_output_dynamic->u.milan.failure.bridge_id, ipc_talker_declaration_status->failure.bridge_id, 8);
		stream_output_dynamic->u.milan.failure.failure_code = ipc_talker_declaration_status->failure.failure_code;
	} else {
		os_memset(stream_output_dynamic->u.milan.failure.bridge_id, 0, 8);
		stream_output_dynamic->u.milan.failure.failure_code = 0;
	}

	/* Register send of an asynchronous GET_STREAM_INFO unsolicited notification per MILAN Specification v1.2 5.4.5.2
	 * to notify changes in the STREAM_OUTPUT descriptor state (MSRP Talker attribute declaration state and MSRP failure information)
	 */
	acmp_milan_talker_register_async_get_stream_info_notification(stream_output_dynamic);
}

/** Updates the SRP talker stream status for the specific stream output
 * \return                             none
 * \param      entity                  pointer to entity struct
 * \param      talker_unique_id        talker_unique_id (host order)
 * \param      ipc_talker_status       ipc status message for talker stream
 */
void acmp_milan_talker_update_status(struct entity *entity, u16 talker_unique_id, struct genavb_msg_talker_status *ipc_talker_status)
{
	genavb_talker_stream_status_t status = ipc_talker_status->status;
	struct stream_output_dynamic_desc *stream_output_dynamic = aem_get_descriptor(entity->aem_dynamic_descs,
											AEM_DESC_TYPE_STREAM_OUTPUT, talker_unique_id, NULL);

	if (!stream_output_dynamic) {
		os_log(LOG_ERR, "entity(%p) talker unique id (%u): unable to get stream output dynamic descriptor\n",
							entity, talker_unique_id);

		return;
	}

	stream_output_dynamic->u.milan.srp_listener_status = status;

	/* If the listener withdraws its attribute (NO_LISTENER), check if we need to stop declaring the talker attribute. */
	acmp_milan_talker_update(entity, talker_unique_id);

	/* Register send of an asynchronous GET_STREAM_INFO unsolicited notification per MILAN Specification v1.2 5.4.5.2
	 * to notify changes in the STREAM_OUTPUT descriptor state (MSRP Listener attribute registration state)
	 */
	acmp_milan_talker_register_async_get_stream_info_notification(stream_output_dynamic);
}

/** Main ACMP MILAN talker receive function.
 * \return                      0 on success, negative otherwise
 * \param acmp                  pointer to the ACMP context
 * \param pdu                   pointer to the ACMP PDU
 * \param msg_type	        ACMP message type (8.2.1.5)
 * \param status                status from AVTP control header (8.2.1.6)
 * \param port_id               port on which the PDU is received
 */
int acmp_milan_talker_rcv(struct acmp_ctx *acmp, struct acmp_pdu *pdu, u8 msg_type, u8 status, unsigned int port_id)
{
	struct entity *entity = container_of(acmp, struct entity, acmp);
	struct avdecc_ctx *avdecc = entity->avdecc;
	struct net_tx_desc *desc_rsp;
	struct acmp_pdu *acmp_rsp;
	struct stream_descriptor *stream_output;
	struct stream_output_dynamic_desc *stream_output_dynamic;
	int rc = 0;
	u16 unique_id = ntohs(pdu->talker_unique_id);
	u64 empty_listener_entity_id = 0;
	u8 status_rsp = ACMP_STAT_SUCCESS;
	struct avdecc_port *port_rsp = &avdecc->port[port_id];

	os_log(LOG_INFO, "acmp(%p) %s: controller(%016"PRIx64") talker(%016"PRIx64", %u) listener(%016"PRIx64", %u)\n",
	       acmp, acmp_milan_msgtype2string(msg_type), ntohll(pdu->controller_entity_id),
	       ntohll(entity->desc->entity_id), unique_id,
	       ntohll(pdu->listener_entity_id), ntohs(pdu->listener_unique_id));

	/* Prepare the response PDU */
	desc_rsp = acmp_net_tx_init(acmp, port_rsp, pdu, true);
	if (!desc_rsp) {
		rc = -1;
		goto exit;
	}

	acmp_rsp = (struct acmp_pdu *)((char *)NET_DATA_START(desc_rsp) + OFFSET_TO_ACMP);

	if (!acmp_talker_unique_valid(acmp, unique_id)) {
		if (ACMP_IS_TALKER_COMMAND(msg_type) && msg_type != ACMP_GET_TX_CONNECTION_COMMAND)
			rc = acmp_send_rsp(acmp, port_rsp, desc_rsp, msg_type + 1, ACMP_STAT_TALKER_UNKNOWN_ID);
		else
			net_tx_free(desc_rsp);

		goto exit;
	}

	stream_output_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_OUTPUT, unique_id, NULL);

	switch (msg_type) {
	case ACMP_PROBE_TX_COMMAND:
		stream_output = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_STREAM_OUTPUT, unique_id, NULL);

		if (port_id != ntohs(stream_output->avb_interface_index)) {
			rc = acmp_send_rsp(acmp, port_rsp, desc_rsp, msg_type + 1, ACMP_STAT_INCOMPATIBLE_REQUEST);
			goto exit;
		}

		/* Restart PROBE_TX timer */
		timer_restart(&stream_output_dynamic->u.milan.probe_tx_reception_timer, ACMP_MILAN_TALKER_TMR_PROBE_TX_RECEPTION_MS);

		stream_output_dynamic->u.milan.probe_tx_valid = true;

		if (!acmp_milan_talker_has_valid_srp_params(stream_output_dynamic)) {

			status_rsp = ACMP_STAT_TALKER_DEST_MAC_FAIL;
		} else {
			/* We have valid SRP params and we received a PROBE_TX_COMAND, check (No SRP talker withdrawal in progress)
			 * if we need to do a talker connect. */
			status_rsp = acmp_milan_talker_update(entity, unique_id);

			acmp_rsp->flags &= ~htons(ACMP_FLAG_REGISTERING_FAILED);
			acmp_rsp->flags |= pdu->flags & htons(ACMP_FLAG_STREAMING_WAIT | ACMP_FLAG_FAST_CONNECT);

			acmp_rsp->connection_count = htons(0);

			acmp_talker_copy_common_params(acmp_rsp, stream_output_dynamic);
		}

		rc = acmp_send_rsp(acmp, port_rsp, desc_rsp, msg_type + 1, status_rsp);
		break;

	case ACMP_DISCONNECT_TX_COMMAND:
		rc = acmp_send_rsp(acmp, port_rsp, desc_rsp, msg_type + 1, ACMP_STAT_SUCCESS);
		break;

	case ACMP_GET_TX_STATE_COMMAND:

		copy_64(&acmp_rsp->listener_entity_id, &empty_listener_entity_id);
		acmp_rsp->listener_unique_id = 0;

		if (stream_output_dynamic->u.milan.srp_talker_declaration_type != NO_TALKER_DECLARATION)
			acmp_talker_copy_common_params(acmp_rsp, stream_output_dynamic);


		if (stream_output_dynamic->u.milan.srp_listener_status == FAILED_LISTENER)
			acmp_rsp->flags |= htons(ACMP_FLAG_REGISTERING_FAILED);

		rc = acmp_send_rsp(acmp, port_rsp, desc_rsp, msg_type + 1, ACMP_STAT_SUCCESS);
		break;

	case ACMP_GET_TX_CONNECTION_COMMAND:
		rc = acmp_send_rsp(acmp, port_rsp, desc_rsp, msg_type + 1, ACMP_STAT_NOT_SUPPORTED);
		break;

	default:
		os_log(LOG_ERR, "entity(%p) message type (%x) not supported\n", entity, msg_type);
		net_tx_free(desc_rsp);
		rc = -1;

		break;
	}

exit:
	return rc;
}

/** Delete the allocated MAAP range for all talkers in the entity.
 * \return              0 on success, negative otherwise.
 * \param      entity   pointer to entity struct
 */
static int acmp_milan_talkers_maap_stop(struct entity *entity)
{
	struct stream_output_dynamic_desc *stream_output_dynamic;
	struct avdecc_ctx *avdecc = entity->avdecc;
	struct stream_descriptor *stream_output;
	struct acmp_ctx *acmp = &entity->acmp;
	struct ipc_desc *desc;
	int i, rc = 0;

	for (i = 0; i < acmp->max_talker_streams; i++) {
		stream_output_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_OUTPUT, i, NULL);

		if (!stream_output_dynamic->u.milan.maap_started)
			continue;

		stream_output = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_STREAM_OUTPUT, i, NULL);

		desc = ipc_alloc(&avdecc->ipc_tx_maap, sizeof(struct genavb_msg_maap_delete));
		if (desc) {
			desc->type = GENAVB_MSG_MAAP_DELETE_RANGE;
			desc->len = sizeof(struct genavb_msg_maap_delete);
			desc->flags = 0;

			desc->u.maap_delete.port_id = ntohs(stream_output->avb_interface_index);
			desc->u.maap_delete.range_id = CFG_ACMP_DEFAULT_MAAP_BASE_RANGE_ID + i;

			rc = ipc_tx(&avdecc->ipc_tx_maap, desc);
			if (rc < 0) {
				os_log(LOG_ERR, "ipc_tx() failed(%d)\n", rc);
				ipc_free(&avdecc->ipc_tx_maap, desc);

				goto exit;
			}

			stream_output_dynamic->u.milan.maap_started = false;

		} else {
			os_log(LOG_ERR, "ipc_alloc() failed\n");
			rc = -1;
			goto exit;
		}
	}

exit:
	return rc;
}

/** Starts the MAAP address allocation for all talkers in the entity.
 * \return              0 on success, negative otherwise.
 * \param      entity   pointer to entity struct
 */
int acmp_milan_talkers_maap_start(struct entity *entity)
{
	struct stream_output_dynamic_desc *stream_output_dynamic;
	struct avdecc_ctx *avdecc = entity->avdecc;
	struct acmp_ctx *acmp = &entity->acmp;
	struct stream_descriptor *stream_output;
	struct ipc_desc *desc;
	int i, rc = 0;

	for (i = 0; i < acmp->max_talker_streams; i++) {
		stream_output_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_OUTPUT, i, NULL);

		if (stream_output_dynamic->u.milan.maap_started)
			continue;

		stream_output = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_STREAM_OUTPUT, i, NULL);

		desc = ipc_alloc(&avdecc->ipc_tx_maap, sizeof(struct genavb_msg_maap_create));
		if (desc) {
			desc->type = GENAVB_MSG_MAAP_CREATE_RANGE;
			desc->len = sizeof(struct genavb_msg_maap_create);
			desc->flags = 0;

			desc->u.maap_create.flag = 0;
			desc->u.maap_create.port_id = ntohs(stream_output->avb_interface_index);
			desc->u.maap_create.range_id = CFG_ACMP_DEFAULT_MAAP_BASE_RANGE_ID + i;
			desc->u.maap_create.count = CFG_ACMP_DEFAULT_MAAP_COUNT_PER_RANGE;

			rc = ipc_tx(&avdecc->ipc_tx_maap, desc);
			if (rc < 0) {
				os_log(LOG_ERR, "ipc_tx() failed(%d)\n", rc);
				ipc_free(&avdecc->ipc_tx_maap, desc);

				goto exit;
			}

			stream_output_dynamic->u.milan.maap_started = true;
		} else {
			os_log(LOG_ERR, "ipc_alloc() failed\n");
			rc = -1;
			goto exit;
		}
	}

exit:
	return rc;
}

/** Called when a MAAP conflict was reported for the address range.
 * \return                      none
 * \param      entity           pointer to entity struct
 * \param      port_id          port on which the conflict was reported
 * \param      range_id         id of the range reporting a conflict before restarting
 * \param      base_address     base address for the range
 * \param      count            number of mac addresses in the range
 */
void acmp_milan_talker_maap_conflict(struct entity *entity, avb_u16 port_id, avb_u32 range_id, avb_u8 *base_address, avb_u16 count)
{
	struct stream_output_dynamic_desc *stream_output_dynamic;
	struct stream_descriptor *stream_output;
	struct acmp_ctx *acmp = &entity->acmp;
	u8 invalid_mac_addr[6] = {0};
	unsigned int stream_index;

	stream_index = range_id - CFG_ACMP_DEFAULT_MAAP_BASE_RANGE_ID;

	if ((range_id < CFG_ACMP_DEFAULT_MAAP_BASE_RANGE_ID) || (stream_index >= acmp->max_talker_streams))
		goto exit;

	if (count != CFG_ACMP_DEFAULT_MAAP_COUNT_PER_RANGE) {
		os_log(LOG_ERR, "entity(%p) stream index(%u) port(%u) range(%u): received incorrect address count (%u) != (%u)\n",
			entity, stream_index, port_id, range_id, count, CFG_ACMP_DEFAULT_MAAP_COUNT_PER_RANGE);
		goto exit;
	}

	stream_output = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_STREAM_OUTPUT, stream_index, NULL);

	/* MAAP range ids are per port, process only indications on the right one */
	if (ntohs(stream_output->avb_interface_index) != port_id)
		goto exit;

	stream_output_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_OUTPUT, stream_index, NULL);

	/* MAAP conflict indication for the range id should contain the same address previously reported valid
	 * This usecase should never happen: but if so, invalid the address anyway.
	 */
	if (os_memcmp(stream_output_dynamic->stream_dest_mac, base_address, 6)) {
		os_log(LOG_ERR, "entity(%p) stream index(%u) port(%u) range(%u): received incorrect base address \
				 (%02x:%02x:%02x:%02x:%02x:%02x) != (%02x:%02x:%02x:%02x:%02x:%02x)\n",
			entity, stream_index, port_id, range_id,
			base_address[0], base_address[1], base_address[2], base_address[3], base_address[4], base_address[5],
			stream_output_dynamic->stream_dest_mac[0], stream_output_dynamic->stream_dest_mac[1], stream_output_dynamic->stream_dest_mac[2],
			stream_output_dynamic->stream_dest_mac[3], stream_output_dynamic->stream_dest_mac[4], stream_output_dynamic->stream_dest_mac[5]);
	}

	/* Invalidate the destination MAC address for the stream */
	os_memcpy(stream_output_dynamic->stream_dest_mac, invalid_mac_addr, 6);

	/* Disconnect the talker stream, if already connected */
	acmp_milan_talker_update(entity, stream_index);

	/* Register send of an asynchronous GET_STREAM_INFO unsolicited notification per MILAN Specification v1.2 5.4.5.2
	 * to notify changes in the STREAM_OUTPUT descriptor state (MAAP stream_dest_mac)
	 */
	acmp_milan_talker_register_async_get_stream_info_notification(stream_output_dynamic);

exit:
	return;
}

/** Called when a MAAP valid/success was reported for the requested address range.
 * \return                      none
 * \param      entity           pointer to entity struct
 * \param      port_id          port on which the range was allocated
 * \param      range_id         id of the allocated range
 * \param      base_address     base address for the range
 * \param      count            number of mac addresses in the range
 */
void acmp_milan_talker_maap_valid(struct entity *entity, avb_u16 port_id, avb_u32 range_id, avb_u8 *base_address, avb_u16 count)
{
	struct stream_output_dynamic_desc *stream_output_dynamic;
	struct stream_descriptor *stream_output;
	struct acmp_ctx *acmp = &entity->acmp;
	unsigned int stream_index;

	stream_index = range_id - CFG_ACMP_DEFAULT_MAAP_BASE_RANGE_ID;

	if ((range_id < CFG_ACMP_DEFAULT_MAAP_BASE_RANGE_ID) || (stream_index >= acmp->max_talker_streams))
		goto exit;

	stream_output = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_STREAM_OUTPUT, stream_index, NULL);

	/* MAAP range ids are per port, process only indications on the right one */
	if (ntohs(stream_output->avb_interface_index) != port_id)
		goto exit;

	if (count != CFG_ACMP_DEFAULT_MAAP_COUNT_PER_RANGE) {
		os_log(LOG_ERR, "entity(%p) stream index(%u) port(%u) range(%u): received incorrect address count (%u) != (%u)\n",
			entity, stream_index, port_id, range_id, count, CFG_ACMP_DEFAULT_MAAP_COUNT_PER_RANGE);
		goto exit;
	}

	stream_output_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_OUTPUT, stream_index, NULL);

	/* Save the stream destination MAC address. */
	os_memcpy(stream_output_dynamic->stream_dest_mac, base_address, 6);

	/* we have now a valid MAAP address, check if we need/can do a talker connect. */
	acmp_milan_talker_update(entity, stream_index);

	/* Register send of an asynchronous GET_STREAM_INFO unsolicited notification per MILAN Specification v1.2 5.4.5.2
	 * to notify changes in the STREAM_OUTPUT descriptor state (MAAP stream_dest_mac)
	 */
	acmp_milan_talker_register_async_get_stream_info_notification(stream_output_dynamic);

exit:
	return;
}

static void acmp_milan_srp_talker_withdraw_timer_handler(void *data)
{
	struct stream_output_dynamic_desc *stream_output_dynamic = (struct stream_output_dynamic_desc *) data;
	struct entity *entity = stream_output_dynamic->entity;

	stream_output_dynamic->u.milan.srp_talker_withdraw_in_progress = false;

	/* 2 LeaveALL timer period has passed since last talker withdraw, check if we need to do a talker connect. */
	acmp_milan_talker_update(entity, stream_output_dynamic->unique_id);
}

static void acmp_talker_probe_tx_reception_timer_handler(void *data)
{
	struct stream_output_dynamic_desc *stream_output_dynamic = (struct stream_output_dynamic_desc *) data;
	struct entity *entity = stream_output_dynamic->entity;

	stream_output_dynamic->u.milan.probe_tx_valid = false;

	/* Per MILAN Specification v1.2 (4.3.3.1 Talker attribute declaration):
	 * If after 15 seconds of last received PROBE_TX_COMMAND and No listener attribute registered, withdraw
	 * the SRP talker attribute (if already declared).
	 */
	acmp_milan_talker_update(entity, stream_output_dynamic->unique_id);
}

static void acmp_milan_talker_async_unsolicited_notification_timer_handler(void *data)
{
	struct stream_output_dynamic_desc *stream_output_dynamic = (struct stream_output_dynamic_desc *)data;
	struct entity *entity = stream_output_dynamic->entity;

	aecp_aem_send_async_unsolicited_notification(&entity->aecp, AECP_AEM_CMD_GET_STREAM_INFO, AEM_DESC_TYPE_STREAM_OUTPUT, stream_output_dynamic->unique_id);
}

__init static int acmp_milan_talker_init_timers(struct entity *entity, u16 talker_unique_id)
{
	struct stream_output_dynamic_desc *stream_output_dynamic = aem_get_descriptor(entity->aem_dynamic_descs,
										      AEM_DESC_TYPE_STREAM_OUTPUT, talker_unique_id, NULL);

	stream_output_dynamic->u.milan.probe_tx_reception_timer.func = acmp_talker_probe_tx_reception_timer_handler;
	stream_output_dynamic->u.milan.probe_tx_reception_timer.data = stream_output_dynamic;

	if (timer_create(entity->avdecc->timer_ctx, &stream_output_dynamic->u.milan.probe_tx_reception_timer, 0,
				ACMP_MILAN_TALKER_TMR_PROBE_TX_RECEPTION_GRANULARITY_MS) < 0)
		goto err_probe_tx_timer;

	/* Init the SRP talker withdraw timer */
	stream_output_dynamic->u.milan.srp_talker_withdraw_timer.func = acmp_milan_srp_talker_withdraw_timer_handler;
	stream_output_dynamic->u.milan.srp_talker_withdraw_timer.data = stream_output_dynamic;

	if (timer_create(entity->avdecc->timer_ctx, &stream_output_dynamic->u.milan.srp_talker_withdraw_timer, 0,
				ACMP_MILAN_TALKER_TMR_SRP_WITHDRAW_GRANULARITY_MS) < 0)
		goto err_srp_talker_withdraw_timer;

	stream_output_dynamic->u.milan.async_unsolicited_notification_timer.func = acmp_milan_talker_async_unsolicited_notification_timer_handler;
	stream_output_dynamic->u.milan.async_unsolicited_notification_timer.data = stream_output_dynamic;

	if (timer_create(entity->avdecc->timer_ctx, &stream_output_dynamic->u.milan.async_unsolicited_notification_timer, 0,
				ACMP_MILAN_TALKER_ASYNC_UNSOLICITED_NOTIFICATION_GRANULARITY_MS) < 0)
		goto err_notification_timer;

	return 0;

err_notification_timer:
	timer_destroy(&stream_output_dynamic->u.milan.srp_talker_withdraw_timer);
err_srp_talker_withdraw_timer:
	timer_destroy(&stream_output_dynamic->u.milan.probe_tx_reception_timer);
err_probe_tx_timer:
	return -1;
}

__exit static void acmp_milan_talker_exit_timers(struct entity *entity, u16 talker_unique_id)
{
	struct stream_output_dynamic_desc *stream_output_dynamic = aem_get_descriptor(entity->aem_dynamic_descs,
										      AEM_DESC_TYPE_STREAM_OUTPUT, talker_unique_id, NULL);

	timer_destroy(&stream_output_dynamic->u.milan.probe_tx_reception_timer);
	timer_destroy(&stream_output_dynamic->u.milan.srp_talker_withdraw_timer);
	timer_destroy(&stream_output_dynamic->u.milan.async_unsolicited_notification_timer);
}

__init int acmp_milan_init(struct acmp_ctx *acmp)
{
	struct entity *entity = container_of(acmp, struct entity, acmp);
	struct stream_input_dynamic_desc *stream_input_dynamic;
	struct stream_output_dynamic_desc *stream_output_dynamic;
	struct stream_descriptor *stream_output;
	struct avb_interface_descriptor *avb_itf;
	int i, j;

	for (i = 0; i < acmp->max_listener_streams; i++) {
		stream_input_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_INPUT, i, NULL);
		if (!stream_input_dynamic) {
			os_log(LOG_ERR, "entity(%p) listener unique id (%u): unable to get stream input dynamic descriptor\n",
					    entity, i);
			goto err_listener_sinks_init;
		}

		stream_input_dynamic->u.milan.state = ACMP_LISTENER_SINK_SM_STATE_UNBOUND;
		stream_input_dynamic->u.milan.srp_stream_status = NO_TALKER;
		stream_input_dynamic->u.milan.srp_state = ACMP_LISTENER_SINK_SRP_STATE_NOT_REGISTERING;
		stream_input_dynamic->u.milan.acmp_status = 0;
		stream_input_dynamic->u.milan.probing_status = ACMP_PROBING_STATUS_DISABLED;

		acmp_milan_set_stream_input_binding_params(stream_input_dynamic, NULL);

		if (acmp_milan_listener_sink_init_timers(entity, i) < 0) {
			os_log(LOG_ERR, "entity(%p) listener unique id (%u): unable to init timers\n",
					    entity, i);
			goto err_listener_sinks_init;
		}
	}

	for (j = 0; j < acmp->max_talker_streams; j++) {
		stream_output_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_OUTPUT, j, NULL);
		if (!stream_output_dynamic) {
			os_log(LOG_ERR, "entity(%p) talker unique id (%u): unable to get stream output dynamic descriptor\n",
					    entity, j);
			goto err_talkers_init;
		}

		stream_output_dynamic->u.milan.srp_talker_declaration_type = NO_TALKER_DECLARATION;
		stream_output_dynamic->u.milan.srp_listener_status = NO_LISTENER;

		stream_output_dynamic->presentation_time_offset = sr_class_max_transit_time(SR_CLASS_A);

		stream_output_dynamic->stream_class = SR_CLASS_A;
		/* Always use the Default Vlan ID */
		stream_output_dynamic->stream_vlan_id = MRP_DEFAULT_VID;

		stream_output = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_STREAM_OUTPUT, j, NULL);
		if (!stream_output) {
			os_log(LOG_ERR, "entity(%p) talker unique id (%u): unable to get stream output descriptor\n",
					    entity, i);
			goto err_talkers_init;
		}

		avb_itf = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_AVB_INTERFACE, ntohs(stream_output->avb_interface_index), NULL);
		if (!avb_itf) {
			os_log(LOG_ERR, "entity(%p) talker unique id (%u): unable to get avb interface with index %u\n",
					    entity, j, ntohs(stream_output->avb_interface_index));
			goto err_talkers_init;
		}

		os_memset(&stream_output_dynamic->stream_id, 0, sizeof(stream_output_dynamic->stream_id));
		os_memcpy(&stream_output_dynamic->stream_id, avb_itf->mac_address, 6);
		*(((u16 *)&stream_output_dynamic->stream_id) + 3) = stream_output->descriptor_index;

		acmp_milan_set_stream_output_withdraw_srp_params(stream_output_dynamic, false);

		stream_output_dynamic->u.milan.srp_talker_withdraw_in_progress = false;
		stream_output_dynamic->u.milan.talker_declared = false;
		stream_output_dynamic->u.milan.avtp_connected = false;
		stream_output_dynamic->u.milan.maap_started = false;
		stream_output_dynamic->u.milan.probe_tx_valid = false;

		if (acmp_milan_talker_init_timers(entity, j) < 0) {
			os_log(LOG_ERR, "entity(%p) talker unique id (%u): unable to init timers\n",
					    entity, j);
			goto err_talkers_init;
		}

	}

	return 0;

err_talkers_init:
	while (j--)
		acmp_milan_talker_exit_timers(entity, j);

err_listener_sinks_init:
	while (i--)
		acmp_milan_listener_sink_exit_timers(entity, i);

	return -1;
}

__exit int acmp_milan_exit(struct acmp_ctx *acmp)
{
	struct entity *entity = container_of(acmp, struct entity, acmp);
	struct avdecc_ctx *avdecc = entity->avdecc;
	int i;

	for (i = 0; i < acmp->max_listener_streams; i++) {
		struct stream_input_dynamic_desc *stream_input_dynamic;

		stream_input_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_INPUT, i, NULL);

		/* Disconnect (AVTP disconnect and SRP deregister) settled (connected) listener streams. */
		if (stream_input_dynamic && ACMP_MILAN_IS_LISTENER_SINK_SETTLED(stream_input_dynamic))
			acmp_listener_stack_disconnect(entity, i);

		acmp_milan_listener_sink_exit_timers(entity, i);
	}

	for (i = 0; i < acmp->max_talker_streams; i++) {
		struct stream_output_dynamic_desc *stream_output_dynamic;
		struct stream_descriptor *stream_output;
		unsigned int port_id;

		stream_output_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_OUTPUT, i, NULL);
		stream_output = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_STREAM_OUTPUT, i, NULL);

		if (stream_output_dynamic && stream_output) {
			port_id = ntohs(stream_output->avb_interface_index);

			if (stream_output_dynamic->u.milan.talker_declared) {
		 		acmp_talker_withdraw(avdecc, port_id, stream_output_dynamic);
		 		stream_output_dynamic->u.milan.talker_declared = false;
		 	}

			if (stream_output_dynamic->u.milan.avtp_connected) {
				acmp_talker_avtp_stack_disconnect(avdecc, port_id, stream_output_dynamic->unique_id, stream_output_dynamic);
				stream_output_dynamic->u.milan.avtp_connected = false;
			}
		}

		acmp_milan_talker_exit_timers(entity, i);
	}

	/* Delete the maap ranges if we have talker streams. */
	if (acmp->max_talker_streams)
		acmp_milan_talkers_maap_stop(entity);

	return 0;
}
