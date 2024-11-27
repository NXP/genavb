/*
 * Copyright 2021-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @brief ADP_MILAN common code
 @details Handles ADP_MILAN stack
*/

#include "os/stdlib.h"

#include "common/log.h"
#include "common/timer.h"
#include "common/net.h"
#include "common/random.h"

#include "genavb/aem.h"

#include "adp.h"
#include "aem.h"
#include "acmp_milan.h"

#include "avdecc.h"

/* _COMMON_ */
#define ADP_MILAN_ADV_TMR_DELAY_MS    (random_range(ADP_MILAN_ADV_TMR_DELAY_MS_MIN, ADP_MILAN_ADV_TMR_DELAY_MS_MAX))  /* Random timer between 0 and 4 sec per spec: Make it between 0.1 and 4 sec to avoid 0 period timers */

static const char *adp_milan_advertise_state2string(adp_milan_advertise_state_t state)
{
	switch (state) {
	case2str(ADP_MILAN_ADV_NOT_STARTED);
	case2str(ADP_MILAN_ADV_DOWN);
	case2str(ADP_MILAN_ADV_WAITING);
	case2str(ADP_MILAN_ADV_DELAY);
	default:
		return (char *) "Unknown adp advertise state";
	}
}

static const char *adp_milan_advertise_event2string(adp_milan_advertise_event_t event)
{
	switch (event) {
	case2str(ADP_MILAN_ADV_START);
	case2str(ADP_MILAN_ADV_RCV_ADP_DISCOVER);
	case2str(ADP_MILAN_ADV_TMR_ADVERTISE);
	case2str(ADP_MILAN_ADV_TMR_DELAY);
	case2str(ADP_MILAN_ADV_LINK_UP);
	case2str(ADP_MILAN_ADV_LINK_DOWN);
	case2str(ADP_MILAN_ADV_GM_CHANGE);
	case2str(ADP_MILAN_ADV_SHUTDOWN);
	default:
		return (char *) "Unknown adp advertise event";
	}
}

static const char *adp_milan_listener_sink_talker_state2string(adp_milan_listener_sink_talker_state_t state)
{
	switch (state) {
	case2str(ADP_LISTENER_SINK_TALKER_STATE_NOT_DISCOVERED);
	case2str(ADP_LISTENER_SINK_TALKER_STATE_DISCOVERED);
	default:
		return (char *) "Unknown adp listener sink talker state";
	}
}

static const char *adp_milan_listener_sink_event2string(adp_milan_listener_sink_event_t event)
{
	switch (event) {
	case2str(ADP_MILAN_LISTENER_SINK_RCV_ADP_AVAILABLE);
	case2str(ADP_MILAN_LISTENER_SINK_RCV_ADP_DEPARTING);
	case2str(ADP_MILAN_LISTENER_SINK_TMR_NO_ADP);
	default:
		return (char *) "Unknown adp listener sink event";
	}
}

/** Sends an ADP ENTITY AVAILABLE PDU to the network and increments the entity's available_index counter.
 * \return		0 on success, -1 if error
 * \param entity	pointer to the entity struct
 * \param port_id	port index on which the packet will be sent
 */
static int adp_milan_advertise_send_available(struct entity *entity, unsigned int port_id)
{
	int rc;

	rc = adp_advertise_send_packet(&entity->adp, ADP_ENTITY_AVAILABLE, port_id);

	entity->desc->available_index = htonl(ntohl(entity->desc->available_index) + 1);

	return rc;
}

/**
 * Check the gptp_grandmaster_id and gptp_domain_number fields of the received ADP ENTITY_AVAILABLE PDU
 * \return true if the current gPTP configuration and state of the PAAD on this port match those in the PDU, false otherwise
 * \param entity, pointer to the entity context
 * \param listener_unique_id
 * \param pdu, pointer to ADP PDU
 */
static bool adp_milan_check_gptp(struct entity *entity, u16 listener_unique_id, struct adp_pdu *pdu)
{
	struct avb_interface_dynamic_desc *avb_interface_dynamic;
	struct stream_descriptor *stream_input;
	struct avb_interface_descriptor *avb_interface;

	stream_input = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_STREAM_INPUT, listener_unique_id, NULL);

	if (!stream_input)
		goto exit;

	avb_interface_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_AVB_INTERFACE, ntohs(stream_input->avb_interface_index), NULL);

	if (!avb_interface_dynamic)
		goto exit;

	avb_interface = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_AVB_INTERFACE, ntohs(stream_input->avb_interface_index), NULL);

	if (!avb_interface)
		goto exit;

	if (cmp_64(&avb_interface_dynamic->gptp_grandmaster_id, &pdu->gptp_grandmaster_id) && (avb_interface->domain_number == pdu->gptp_domain_number))
		return true;

exit:
	return false;
}

/**
 * Update discovery state machine upon reception of an ADP ENTITY_AVAILABLE/ENTITY_DEPARTING message
 * \return none
 * \param avdecc, pointer to AVDECC context
 * \param msg_type, ENTITY_AVAILABLE/ENTITY_DEPARTING
 * \param pdu, pointer to ADP PDU
 * \param mac_src, pointer to source MAC address
 */
void adp_milan_listener_rcv(struct entity *entity, u8 msg_type, struct adp_pdu *pdu, u8 valid_time)
{
	int num_stream_in, i;
	struct stream_input_dynamic_desc *stream_input_dynamic;
	adp_milan_listener_sink_event_t event;

	if (msg_type == ADP_ENTITY_AVAILABLE)
		event = ADP_MILAN_LISTENER_SINK_RCV_ADP_AVAILABLE;
	else if (msg_type == ADP_ENTITY_DEPARTING)
		event = ADP_MILAN_LISTENER_SINK_RCV_ADP_DEPARTING;
	else
		return;

	num_stream_in = aem_get_descriptor_max(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_INPUT);
	for (i = 0; i < num_stream_in; i++) {
		stream_input_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_INPUT, i, NULL);
		if (ACMP_MILAN_IS_LISTENER_SINK_BOUND(stream_input_dynamic) && cmp_64(&stream_input_dynamic->talker_entity_id, &pdu->entity_id)) {
			adp_milan_listener_sink_discovery_sm(entity, i, event, valid_time, pdu);
		}
	}

	os_log(LOG_DEBUG, "entity(%p) msg(%d) done\n", entity, msg_type);
}

/* _TIMERS_ */
/* Advertise */
static void adp_milan_adv_delay_timer_timeout(void *data)
{
	struct avb_interface_dynamic_desc *avb_interface_dynamic = (struct avb_interface_dynamic_desc *)data;

	adp_milan_advertise_sm(avb_interface_dynamic->entity, avb_interface_dynamic->interface_index, ADP_MILAN_ADV_TMR_DELAY);
}

static void adp_milan_adv_timer_timeout(void *data)
{
	struct avb_interface_dynamic_desc *avb_interface_dynamic = (struct avb_interface_dynamic_desc *)data;

	adp_milan_advertise_sm(avb_interface_dynamic->entity, avb_interface_dynamic->interface_index, ADP_MILAN_ADV_TMR_ADVERTISE);
}

/* Discovery */
static void adp_milan_listener_sink_disc_timer_timeout(void *data)
{
	struct stream_input_dynamic_desc *stream_input_dynamic = (struct stream_input_dynamic_desc *)data;

	adp_milan_listener_sink_discovery_sm(stream_input_dynamic->entity, stream_input_dynamic->unique_id, ADP_MILAN_LISTENER_SINK_TMR_NO_ADP, 0, NULL);
}
/* _TIMERS_ */


/* _STATE_MACHINES_ */
/* Advertise */
/**
 * Advertise state machine handler
 * \return 		0 on success, negative otherwise
 * \param entity	pointer to entity context
 * \param port_id	port index
 * \param event		an adp_milan_advertise_event_t event
 */
int adp_milan_advertise_sm(struct entity *entity, unsigned int port_id, adp_milan_advertise_event_t event)
{
	adp_milan_advertise_state_t state;
	struct avb_interface_dynamic_desc *avb_interface_dynamic;

	avb_interface_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_AVB_INTERFACE, port_id, NULL);

	if (!avb_interface_dynamic)
		goto err;

	state = avb_interface_dynamic->u.milan.state;

	switch (state) {
	case ADP_MILAN_ADV_NOT_STARTED:
		switch (event) {
		case ADP_MILAN_ADV_START:
			/* 9.3.5.1/9.3.5.2 */
			if (!(avb_interface_dynamic->operational_state)) {
				avb_interface_dynamic->u.milan.state = ADP_MILAN_ADV_DOWN;
			} else {
				timer_start(&avb_interface_dynamic->u.milan.adp_delay_timer, random_range(ADP_MILAN_ADV_TMR_DELAY_MS_INIT_MIN, ADP_MILAN_ADV_TMR_DELAY_MS_INIT_MAX)); /* random between 0.1 s and 2 sec */
				avb_interface_dynamic->u.milan.state = ADP_MILAN_ADV_DELAY;
			}

			break;

		default:
			break;
		}
		break;

	case ADP_MILAN_ADV_DOWN:
		switch (event) {
		case ADP_MILAN_ADV_LINK_UP:
			/* 9.3.5.2/3 */
			timer_start(&avb_interface_dynamic->u.milan.adp_delay_timer, ADP_MILAN_ADV_TMR_DELAY_MS);

			avb_interface_dynamic->u.milan.state = ADP_MILAN_ADV_DELAY;
			break;

		default:
			break;
		}
		break;

	case ADP_MILAN_ADV_WAITING:
		switch (event) {
		case ADP_MILAN_ADV_RCV_ADP_DISCOVER:
			/* 9.3.5.4 */
			timer_stop(&avb_interface_dynamic->u.milan.adp_advertise_timer);
			timer_start(&avb_interface_dynamic->u.milan.adp_delay_timer, ADP_MILAN_ADV_TMR_DELAY_MS);

			avb_interface_dynamic->u.milan.state = ADP_MILAN_ADV_DELAY;
			break;

		case ADP_MILAN_ADV_TMR_ADVERTISE:
			/* 9.3.5.5 */
			timer_start(&avb_interface_dynamic->u.milan.adp_delay_timer, ADP_MILAN_ADV_TMR_DELAY_MS);

			avb_interface_dynamic->u.milan.state = ADP_MILAN_ADV_DELAY;
			break;

		case ADP_MILAN_ADV_LINK_DOWN:
			/* 9.3.5.6 */
			timer_stop(&avb_interface_dynamic->u.milan.adp_advertise_timer);

			avb_interface_dynamic->u.milan.state = ADP_MILAN_ADV_DOWN;
			break;

		case ADP_MILAN_ADV_GM_CHANGE:
			/* 9.3.5.7 */
			/* Stopping the TMR_ADVERTISE is not mentioned in the spec, but it needs to be stopped before going into
			* the DELAY state as it will be restarted there.
			*/
			timer_stop(&avb_interface_dynamic->u.milan.adp_advertise_timer);
			timer_start(&avb_interface_dynamic->u.milan.adp_delay_timer, ADP_MILAN_ADV_TMR_DELAY_MS);

			avb_interface_dynamic->u.milan.state = ADP_MILAN_ADV_DELAY;
			break;

		case ADP_MILAN_ADV_SHUTDOWN:
			/* 9.3.5.8 */
			timer_stop(&avb_interface_dynamic->u.milan.adp_advertise_timer);

			/* Per IEEE1722.1-2013 6.2.1.16 */
			entity->desc->available_index = htonl(0);

			if (adp_advertise_send_packet(&entity->adp, ADP_ENTITY_DEPARTING, port_id) < 0) {
				os_log(LOG_ERR, "entity(%p) port_id(%d) Couldn't send ENTITY DEPARTING message\n", entity, port_id);
			}

			avb_interface_dynamic->u.milan.state = ADP_MILAN_ADV_NOT_STARTED;
			break;

		default:
			break;
		}
		break;

	case ADP_MILAN_ADV_DELAY:
		switch (event) {
		case ADP_MILAN_ADV_TMR_DELAY:
			/* 9.3.5.9 */
			if (adp_milan_advertise_send_available(entity, port_id) < 0) {
				os_log(LOG_ERR, "entity(%p) port_id(%d) Couldn't send ENTITY AVAILABLE message\n", entity, port_id);
			}

			timer_start(&avb_interface_dynamic->u.milan.adp_advertise_timer, ADP_MILAN_ADV_TMR_ADVERTISE_MS); /* 5 sec */

			avb_interface_dynamic->u.milan.state = ADP_MILAN_ADV_WAITING;
			break;

		case ADP_MILAN_ADV_LINK_DOWN:
			/* 9.3.5.10 */
			timer_stop(&avb_interface_dynamic->u.milan.adp_delay_timer);

			avb_interface_dynamic->u.milan.state = ADP_MILAN_ADV_DOWN;
			break;

		case ADP_MILAN_ADV_SHUTDOWN:
			/* 9.3.5.11 */
			timer_stop(&avb_interface_dynamic->u.milan.adp_delay_timer);

			/* Per IEEE1722.1-2013 6.2.1.16 */
			entity->desc->available_index = htonl(0);

			if (adp_advertise_send_packet(&entity->adp, ADP_ENTITY_DEPARTING, port_id) < 0) {
				os_log(LOG_ERR, "entity(%p) port_id(%d) Couldn't send ENTITY DEPARTING message\n", entity, port_id);
			}

			avb_interface_dynamic->u.milan.state = ADP_MILAN_ADV_NOT_STARTED;
			break;

		default:
			break;
		}
		break;

	default:
		os_log(LOG_ERR, "Received event %s in unknown state %d\n", adp_milan_advertise_event2string(event), state);
		goto err;
	}

	os_log(LOG_DEBUG, "entity(%p) port(%u) : event %s, state from %s to %s\n", entity, port_id,
			adp_milan_advertise_event2string(event), adp_milan_advertise_state2string(state),
			adp_milan_advertise_state2string(avb_interface_dynamic->u.milan.state));

	return 0;

err:
	return -1;
}

/* Discovery */
/**
 * Discovery state machine handler
 * \return 0 on success, negative otherwise
 * \param entity, pointer to entity context
 * \param listener_unique_id
 * \param event, an adp_milan_listener_sink_event_t event
 * \param pdu, pointer to ADP PDU
 */
int adp_milan_listener_sink_discovery_sm(struct entity *entity, u16 listener_unique_id, adp_milan_listener_sink_event_t event, u8 valid_time, struct adp_pdu *pdu)
{
	adp_milan_listener_sink_talker_state_t state;
	struct stream_input_dynamic_desc *stream_input_dynamic;

	stream_input_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_INPUT, listener_unique_id, NULL);

	if (!stream_input_dynamic)
		goto err;

	state = stream_input_dynamic->u.milan.talker_state;

	switch (state) {
	case ADP_LISTENER_SINK_TALKER_STATE_NOT_DISCOVERED:
		switch (event) {
		case ADP_MILAN_LISTENER_SINK_RCV_ADP_AVAILABLE:
			/* 9.4.5.1 */
			if (!adp_milan_check_gptp(entity, listener_unique_id, pdu)) {
				os_log(LOG_DEBUG, "entity(%p) listener_unique_id(%d) gPTP configuration of current PAAD don't match pdu's configuration\n", entity, listener_unique_id);
				break;
			}

			stream_input_dynamic->u.milan.available_index = pdu->available_index;
			stream_input_dynamic->u.milan.interface_index = pdu->interface_index;

			timer_start(&stream_input_dynamic->u.milan.adp_discovery_timer, max(1, valid_time) * MS_PER_S);

			acmp_milan_listener_sink_event(entity, listener_unique_id, ACMP_LISTENER_SINK_SM_EVENT_EVT_TK_DISCOVERED);

			stream_input_dynamic->u.milan.talker_state = ADP_LISTENER_SINK_TALKER_STATE_DISCOVERED;
			break;

		default:
			break;
		}
		break;

	case ADP_LISTENER_SINK_TALKER_STATE_DISCOVERED:
		switch(event) {
		case ADP_MILAN_LISTENER_SINK_RCV_ADP_AVAILABLE:
			/* 9.4.5.2 */
			if (ntohs(stream_input_dynamic->u.milan.interface_index) != ntohs(pdu->interface_index)) {
				os_log(LOG_DEBUG, "entity(%p) listener_unique_id(%d) pdu's interface index (%d) don't match current interface index (%d)\n",
						entity, listener_unique_id, ntohs(pdu->interface_index), ntohs(stream_input_dynamic->u.milan.interface_index));

				break;
			}

			if ((s32)(ntohl(pdu->available_index) - ntohl(stream_input_dynamic->u.milan.available_index)) <= 0) {
				acmp_milan_listener_sink_event(entity, listener_unique_id, ACMP_LISTENER_SINK_SM_EVENT_EVT_TK_DEPARTED);

				if (!adp_milan_check_gptp(entity, listener_unique_id, pdu)) {
					timer_stop(&stream_input_dynamic->u.milan.adp_discovery_timer);

					stream_input_dynamic->u.milan.talker_state = ADP_LISTENER_SINK_TALKER_STATE_NOT_DISCOVERED;
					break;
				}

				acmp_milan_listener_sink_event(entity, listener_unique_id, ACMP_LISTENER_SINK_SM_EVENT_EVT_TK_DISCOVERED);
			}

			stream_input_dynamic->u.milan.available_index = pdu->available_index;

			timer_stop(&stream_input_dynamic->u.milan.adp_discovery_timer);
			timer_start(&stream_input_dynamic->u.milan.adp_discovery_timer, max(1, valid_time) * MS_PER_S);

			break;

		case ADP_MILAN_LISTENER_SINK_RCV_ADP_DEPARTING:
			/* 9.4.5.3 */
			if (ntohs(stream_input_dynamic->u.milan.interface_index) != ntohs(pdu->interface_index)) {
				os_log(LOG_DEBUG, "entity(%p) listener_unique_id(%d) pdu's interface index (%d) don't match current interface index (%d)\n",
						entity, listener_unique_id, ntohs(pdu->interface_index), ntohs(stream_input_dynamic->u.milan.interface_index));

				break;
			}

			timer_stop(&stream_input_dynamic->u.milan.adp_discovery_timer);

			acmp_milan_listener_sink_event(entity, listener_unique_id, ACMP_LISTENER_SINK_SM_EVENT_EVT_TK_DEPARTED);

			stream_input_dynamic->u.milan.talker_state = ADP_LISTENER_SINK_TALKER_STATE_NOT_DISCOVERED;

			break;

		case ADP_MILAN_LISTENER_SINK_TMR_NO_ADP:
			/* 9.4.5.4 */
			acmp_milan_listener_sink_event(entity, listener_unique_id, ACMP_LISTENER_SINK_SM_EVENT_EVT_TK_DEPARTED);

			stream_input_dynamic->u.milan.talker_state = ADP_LISTENER_SINK_TALKER_STATE_NOT_DISCOVERED;

			break;

		case ADP_MILAN_LISTENER_SINK_RESET:
			/* Not mentioned in the spec but used by the ACMP listener sink SM to reset this one. */
			timer_stop(&stream_input_dynamic->u.milan.adp_discovery_timer);

			stream_input_dynamic->u.milan.talker_state = ADP_LISTENER_SINK_TALKER_STATE_NOT_DISCOVERED;

			break;

		default:
			break;
		}
		break;

	default:
		os_log(LOG_ERR, "Received event %s in unknown state %d\n", adp_milan_listener_sink_event2string(event), state);
		goto err;
	}

	os_log(LOG_DEBUG, "entity(%p) listener_unique_id(%u) : event %s, state from %s to %s\n", entity, listener_unique_id,
			adp_milan_listener_sink_event2string(event), adp_milan_listener_sink_talker_state2string(state),
			adp_milan_listener_sink_talker_state2string(stream_input_dynamic->u.milan.talker_state));

	return 0;

err:
	return -1;
}

void adp_milan_advertise_start(struct adp_ctx *adp)
{
	int num_interfaces, i;
	struct entity *entity = container_of(adp, struct entity, adp);

	num_interfaces = aem_get_descriptor_max(entity->aem_dynamic_descs, AEM_DESC_TYPE_AVB_INTERFACE);

	for (i = 0; i < num_interfaces; i++) {
		adp_milan_advertise_sm(entity, i, ADP_MILAN_ADV_START);
	}
}
/* _STATE_MACHINES_ */


/* _EXIT_ */
/* Timers */
__exit static void adp_milan_advertise_exit_timers(struct entity *entity, unsigned int port_id)
{
	struct avb_interface_dynamic_desc *avb_interface_dynamic;

	avb_interface_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_AVB_INTERFACE, port_id, NULL);

	timer_destroy(&avb_interface_dynamic->u.milan.adp_delay_timer);
	timer_destroy(&avb_interface_dynamic->u.milan.adp_advertise_timer);
}

__exit static void adp_milan_listener_sink_exit_timers(struct entity *entity, int stream_input_index)
{
	struct stream_input_dynamic_desc *stream_input_dynamic;

	stream_input_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_INPUT, stream_input_index, NULL);

	timer_destroy(&stream_input_dynamic->u.milan.adp_discovery_timer);
}

/* Advertise */
__exit int adp_milan_advertise_exit(struct adp_ctx *adp)
{
	int num_interfaces, i;
	struct entity *entity = container_of(adp, struct entity, adp);

	num_interfaces = aem_get_descriptor_max(entity->aem_dynamic_descs, AEM_DESC_TYPE_AVB_INTERFACE);

	if (entity_ready(entity)) {
		for (i = 0; i < num_interfaces; i++) {
			adp_milan_advertise_sm(entity, i, ADP_MILAN_ADV_SHUTDOWN);

			adp_milan_advertise_exit_timers(entity, i);
		}
	}

	os_log(LOG_DEBUG, "entity(%p) num_interf(%d)\n", entity, num_interfaces);

	os_log(LOG_INIT, "done\n");
	return 0;
}

/* Discovery */
__exit int adp_milan_listener_sink_discovery_exit(struct adp_ctx *adp)
{
	int num_streams_input, i;
	struct entity *entity = container_of(adp, struct entity, adp);

	num_streams_input = aem_get_descriptor_max(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_INPUT);

	for (i = 0; i < num_streams_input; i++) {
		adp_milan_listener_sink_exit_timers(entity, i);
	}

	os_log(LOG_DEBUG, "entity(%p) num_strin(%d)\n", entity, num_streams_input);

	os_log(LOG_INIT, "done\n");
	return 0;
}
/* _EXIT_ */


/* _INIT_ */
/* Timers */
__init static int adp_milan_advertise_init_timers(struct entity *entity, unsigned int port_id)
{
	struct avb_interface_dynamic_desc *avb_interface_dynamic;

	avb_interface_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_AVB_INTERFACE, port_id, NULL);

	avb_interface_dynamic->u.milan.adp_delay_timer.func = adp_milan_adv_delay_timer_timeout;
	avb_interface_dynamic->u.milan.adp_delay_timer.data = avb_interface_dynamic;

	if (timer_create(entity->avdecc->timer_ctx, &avb_interface_dynamic->u.milan.adp_delay_timer, 0, ADP_MILAN_TMR_GRANULARITY_MS) < 0) {
		os_log(LOG_ERR, "entity(%p) avb_interface(%p, %d) advertise timer_create failed\n", entity, avb_interface_dynamic, port_id);
		goto err_delay_timer;
	}

	avb_interface_dynamic->u.milan.adp_advertise_timer.func = adp_milan_adv_timer_timeout;
	avb_interface_dynamic->u.milan.adp_advertise_timer.data = avb_interface_dynamic;

	if (timer_create(entity->avdecc->timer_ctx, &avb_interface_dynamic->u.milan.adp_advertise_timer, 0, ADP_MILAN_TMR_GRANULARITY_MS) < 0) {
		os_log(LOG_ERR, "entity(%p) avb_interface(%p, %d) advertise timer_create failed\n", entity, avb_interface_dynamic, port_id);
		goto err_advertise_timer;
	}

	return 0;

err_advertise_timer:
	timer_destroy(&avb_interface_dynamic->u.milan.adp_delay_timer);

err_delay_timer:
	return -1;
}

__init static int adp_milan_listener_sink_init_timers(struct entity *entity, int stream_input_index)
{
	struct stream_input_dynamic_desc *stream_input_dynamic;

	stream_input_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_INPUT, stream_input_index, NULL);

	stream_input_dynamic->u.milan.adp_discovery_timer.func = adp_milan_listener_sink_disc_timer_timeout;
	stream_input_dynamic->u.milan.adp_discovery_timer.data = stream_input_dynamic;

	if (timer_create(entity->avdecc->timer_ctx, &stream_input_dynamic->u.milan.adp_discovery_timer, 0, ADP_MILAN_TMR_GRANULARITY_MS) < 0) {
		os_log(LOG_ERR, "entity(%p) stream_input(%p, %d) discovery timer_create failed\n", entity, stream_input_dynamic, stream_input_index);
		goto err_timer;
	}

	return 0;

err_timer:
	return -1;
}

/* Advertise */
__init int adp_milan_advertise_init(struct adp_ctx *adp)
{
	int num_interfaces, i;
	struct avb_interface_dynamic_desc *avb_interface_dynamic;
	struct entity *entity = container_of(adp, struct entity, adp);

	num_interfaces = aem_get_descriptor_max(entity->aem_dynamic_descs, AEM_DESC_TYPE_AVB_INTERFACE);

	os_log(LOG_DEBUG, "entity(%p) num_interf(%d)\n", entity, num_interfaces);

	for (i = 0; i < num_interfaces; i++) {
		avb_interface_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_AVB_INTERFACE, i, NULL);

		if (adp_milan_advertise_init_timers(entity, i) < 0) {
			os_log(LOG_ERR, "init timer failed\n");
			goto err_timer;
		}

		avb_interface_dynamic->u.milan.state = ADP_MILAN_ADV_NOT_STARTED;

		os_log(LOG_DEBUG, "entity(%p) avb_interface(%p %d)\n", entity, avb_interface_dynamic, i);
	}

	os_log(LOG_INIT, "done\n");
	return 0;

err_timer:
	while(i--)
		adp_milan_advertise_exit_timers(entity, i);

	os_log(LOG_ERR, "failed\n");
	return -1;
}

/* Discovery */
__init int adp_milan_listener_sink_discovery_init(struct adp_ctx *adp)
{
	int num_stream_in, i;
	struct stream_input_dynamic_desc *stream_input_dynamic;
	struct entity *entity = container_of(adp, struct entity, adp);

	num_stream_in = aem_get_descriptor_max(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_INPUT);

	os_log(LOG_DEBUG, "entity(%p) num_strin(%d)\n", entity, num_stream_in);

	for (i = 0; i < num_stream_in; i++) {
		stream_input_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_STREAM_INPUT, i, NULL);

		stream_input_dynamic->u.milan.interface_index = 0;
		stream_input_dynamic->u.milan.available_index = 0;

		if (adp_milan_listener_sink_init_timers(entity, i) < 0) {
			os_log(LOG_ERR, "init timer failed\n");
			goto err_timer;
		}

		stream_input_dynamic->u.milan.talker_state = ADP_LISTENER_SINK_TALKER_STATE_NOT_DISCOVERED;

		os_log(LOG_DEBUG, "entity(%p) stream_input(%p, %d)\n", entity, stream_input_dynamic, i);
	}

	os_log(LOG_INIT, "done\n");
	return 0;

err_timer:
	while(i--)
		adp_milan_listener_sink_exit_timers(entity, i);

	os_log(LOG_ERR, "failed\n");
	return -1;
}
/* _INIT_ */
