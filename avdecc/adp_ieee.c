/*
 * Copyright 2014 Freescale Semiconductor, Inc.
 * Copyright 2020-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @brief ADP common code
 @details Handles ADP stack
*/

#include "os/stdlib.h"

#include "common/log.h"
#include "common/timer.h"
#include "common/net.h"

#include "genavb/aem.h"

#include "adp.h"
#include "aem.h"

#include "avdecc.h"

#define ADP_IEEE_ADV_TMR_DELAY_MS_MIN 100

static const char *adp_ieee_advertise_entity_event2string(adp_ieee_advertise_entity_event_t event)
{
	switch (event) {
	case2str(ADP_ENTITY_ADV_EVENT_BEGIN);
	case2str(ADP_ENTITY_ADV_EVENT_ADVERTISE);
	case2str(ADP_ENTITY_ADV_EVENT_REANNOUNCE_TIMEOUT);
	case2str(ADP_ENTITY_ADV_EVENT_DELAY_TIMEOUT);
	case2str(ADP_ENTITY_ADV_EVENT_TERMINATE);
	case2str(ADP_ENTITY_ADV_EVENT_RUN);
	default:
		return (char *) "Unknown ADP advertise entity event";
	}
}

static const char *adp_ieee_advertise_entity_state2string(adp_ieee_advertise_entity_state_t state)
{
	switch (state) {
	case2str(ADP_ENTITY_ADV_NOT_STARTED);
	case2str(ADP_ENTITY_ADV_WAITING);
	case2str(ADP_ENTITY_ADV_ADVERTISE);
	case2str(ADP_ENTITY_ADV_DELAY);
	case2str(ADP_ENTITY_ADV_RUN);
	default:
		return (char *) "Unknown ADP advertise entity state";
	}
}

static const char *adp_ieee_advertise_interface_state2string(adp_ieee_advertise_interface_state_t state)
{
	switch (state) {
	case2str(ADP_INTERFACE_ADV_NOT_STARTED);
	case2str(ADP_INTERFACE_ADV_WAITING);
	case2str(ADP_INTERFACE_ADV_ADVERTISE);
	case2str(ADP_INTERFACE_ADV_DEPARTING);
	case2str(ADP_INTERFACE_ADV_RECEIVED_DISCOVER);
	case2str(ADP_INTERFACE_ADV_UPDATE_GM);
	case2str(ADP_INTERFACE_ADV_LINK_DOWN);
	default:
		return (char *) "Unknown ADP advertise interface state";
	}
}

static const char *adp_ieee_advertise_interface_event2string(adp_ieee_advertise_interface_event_t event)
{
	switch (event) {
	case2str(ADP_INTERFACE_ADV_EVENT_BEGIN);
	case2str(ADP_INTERFACE_ADV_EVENT_ADVERTISE);
	case2str(ADP_INTERFACE_ADV_EVENT_RCV_DISCOVER);
	case2str(ADP_INTERFACE_ADV_EVENT_GM_CHANGE);
	case2str(ADP_INTERFACE_ADV_EVENT_LINK_UP);
	case2str(ADP_INTERFACE_ADV_EVENT_LINK_DOWN);
	case2str(ADP_INTERFACE_ADV_EVENT_RUN);
	default:
		return (char *) "Unknown ADP advertise interface event";
	}
}

void adp_ieee_advertise_start(struct adp_ctx *adp)
{
	struct entity *entity = container_of(adp, struct entity, adp);

	adp_ieee_advertise_entity_sm(entity, ADP_ENTITY_ADV_EVENT_BEGIN);
}

/**
 * Advertise entity state machine (6.2.4.3)
 * \return 0 on success, negative otherwise
 * \param entity	pointer to entity context
 * \param event		an adp_ieee_advertise_entity_event_t event
 */
int adp_ieee_advertise_entity_sm(struct entity *entity, adp_ieee_advertise_entity_event_t event)
{
	adp_ieee_advertise_entity_state_t state;
	struct adp_ieee_advertise_entity_ctx *adv = &entity->adp.ieee.advertise;
	unsigned int num_interfaces, port_num;

	num_interfaces = aem_get_descriptor_max(entity->aem_dynamic_descs, AEM_DESC_TYPE_AVB_INTERFACE);

start:
	state = adv->state;

	switch (state) {
	case ADP_ENTITY_ADV_NOT_STARTED:
		switch (event) {
		case ADP_ENTITY_ADV_EVENT_BEGIN:
			entity->desc->available_index = htonl(0);

			for (port_num = 0; port_num < num_interfaces; port_num++)
				adp_ieee_advertise_interface_sm(entity, port_num, ADP_INTERFACE_ADV_EVENT_BEGIN);

			timer_start(&adv->delay_timer, random_range(ADP_IEEE_ADV_TMR_DELAY_MS_MIN, max(1, (entity->valid_time / 5)) * MS_PER_S));

			adv->state = ADP_ENTITY_ADV_DELAY;
			break;

		default:
			break;
		}
		break;

	case ADP_ENTITY_ADV_DELAY:
		switch (event) {
		case ADP_ENTITY_ADV_EVENT_DELAY_TIMEOUT:

			adv->state = ADP_ENTITY_ADV_ADVERTISE;
			break;

		case ADP_ENTITY_ADV_EVENT_TERMINATE:
			timer_stop(&adv->delay_timer);

			adv->state = ADP_ENTITY_ADV_NOT_STARTED;
			break;

		default:
			break;
		}
		break;

	case ADP_ENTITY_ADV_ADVERTISE:
		for (port_num = 0; port_num < num_interfaces; port_num++)
			adp_ieee_advertise_interface_sm(entity, port_num, ADP_INTERFACE_ADV_EVENT_ADVERTISE);

		/* Per IEEE1722.1-2013 6.2.1.6:
		* entity->valid_time is the configured valid_time in units of seconds
		* and we need to send advertisement every 1/4 that period
		* (which is 1/2 valid_time status field, in units of 2s, in the sent ENTITY_AVAILABLE)
		*/
		timer_start(&adv->reannounce_timer, max(1, (entity->valid_time / 4)) * MS_PER_S);

		adv->state = ADP_ENTITY_ADV_WAITING; //UCT
		break;

	case ADP_ENTITY_ADV_WAITING:
		switch (event) {
		case ADP_ENTITY_ADV_EVENT_TERMINATE:

			timer_stop(&adv->reannounce_timer);

			/* Per IEEE1722.1-2013 6.2.1.16 */
			entity->desc->available_index = htonl(0);

			for (port_num = 0; port_num < num_interfaces; port_num++)
				adp_ieee_advertise_interface_sm(entity, port_num, ADP_INTERFACE_ADV_EVENT_TERMINATE);

			adv->state = ADP_ENTITY_ADV_NOT_STARTED;
			break;

		case ADP_ENTITY_ADV_EVENT_ADVERTISE:
			timer_stop(&adv->reannounce_timer);

			/* fallthrough */
		case ADP_ENTITY_ADV_EVENT_REANNOUNCE_TIMEOUT:
			entity->desc->available_index = htonl(ntohl(entity->desc->available_index) + 1);

			timer_start(&adv->delay_timer, random_range(ADP_IEEE_ADV_TMR_DELAY_MS_MIN, max(1, (entity->valid_time / 5)) * MS_PER_S));

			adv->state = ADP_ENTITY_ADV_DELAY;
			break;

		default:
			break;
		}
		break;

	default:
		os_log(LOG_ERR, "Received event %s in unknown state %d\n", adp_ieee_advertise_entity_event2string(event), state);
		goto err;
	}

	os_log(LOG_DEBUG, "entity(%p): event %s, state from %s to %s\n", entity,
			adp_ieee_advertise_entity_event2string(event), adp_ieee_advertise_entity_state2string(state),
			adp_ieee_advertise_entity_state2string(adv->state));

	if (state != adv->state) {
		event = ADP_ENTITY_ADV_EVENT_RUN;
		goto start;
	}

	return 0;

err:
	return -1;
}

/**
 * Advertise interface state machine (6.2.5.3)
 * \return 0 on success, negative otherwise
 * \param entity	pointer to entity context
 * \param port_id	port index
 * \param event		an adp_ieee_advertise_interface_event_t event
 */
int adp_ieee_advertise_interface_sm(struct entity *entity, unsigned int port_id, adp_ieee_advertise_interface_event_t event)
{
	adp_ieee_advertise_interface_state_t state;
	struct avb_interface_dynamic_desc *avb_itf_dynamic;

	avb_itf_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_AVB_INTERFACE, port_id, NULL);

	if (!avb_itf_dynamic)
		goto err;

start:
	state = avb_itf_dynamic->u.ieee.advertise_state;

	switch (state) {
	case ADP_INTERFACE_ADV_NOT_STARTED:
		switch (event) {
		case ADP_INTERFACE_ADV_EVENT_BEGIN:
			if (!(avb_itf_dynamic->operational_state))
				avb_itf_dynamic->u.ieee.advertise_state = ADP_INTERFACE_ADV_LINK_DOWN;
			else
				avb_itf_dynamic->u.ieee.advertise_state = ADP_INTERFACE_ADV_WAITING;

			break;

		default:
			break;
		}
		break;

	case ADP_INTERFACE_ADV_WAITING:
		switch (event) {
		case ADP_INTERFACE_ADV_EVENT_TERMINATE:
			avb_itf_dynamic->u.ieee.advertise_state = ADP_INTERFACE_ADV_DEPARTING;
			break;

		case ADP_INTERFACE_ADV_EVENT_ADVERTISE:
			avb_itf_dynamic->u.ieee.advertise_state = ADP_INTERFACE_ADV_ADVERTISE;
			break;

		case ADP_INTERFACE_ADV_EVENT_RCV_DISCOVER:
			avb_itf_dynamic->u.ieee.advertise_state = ADP_INTERFACE_ADV_RECEIVED_DISCOVER;
			break;

		case ADP_INTERFACE_ADV_EVENT_GM_CHANGE:
			avb_itf_dynamic->u.ieee.advertise_state = ADP_INTERFACE_ADV_UPDATE_GM;
			break;

		case ADP_INTERFACE_ADV_EVENT_LINK_DOWN:
			avb_itf_dynamic->u.ieee.advertise_state = ADP_INTERFACE_ADV_LINK_DOWN;
			break;

		default:
			break;
		}
		break;

	case ADP_INTERFACE_ADV_DEPARTING:
		if (adp_advertise_send_packet(&entity->adp, ADP_ENTITY_DEPARTING, port_id) < 0)
			os_log(LOG_ERR, "entity(%p) port_id(%d) Couldn't send ENTITY DEPARTING message\n",
				entity, port_id);

		avb_itf_dynamic->u.ieee.advertise_state = ADP_INTERFACE_ADV_NOT_STARTED; //UCT
		break;

	case ADP_INTERFACE_ADV_ADVERTISE:
		if (adp_advertise_send_packet(&entity->adp, ADP_ENTITY_AVAILABLE, port_id) < 0)
			os_log(LOG_ERR, "entity(%p) port_id(%d) Couldn't send ENTITY AVAILABLE message\n",
				entity, port_id);

		avb_itf_dynamic->u.ieee.advertise_state = ADP_INTERFACE_ADV_WAITING; //UCT
		break;

	case ADP_INTERFACE_ADV_RECEIVED_DISCOVER:

		adp_ieee_advertise_entity_sm(entity, ADP_ENTITY_ADV_EVENT_ADVERTISE);

		avb_itf_dynamic->u.ieee.advertise_state = ADP_INTERFACE_ADV_WAITING; //UCT
		break;

	case ADP_INTERFACE_ADV_UPDATE_GM:
		adp_ieee_advertise_entity_sm(entity, ADP_ENTITY_ADV_EVENT_ADVERTISE);

		avb_itf_dynamic->u.ieee.advertise_state = ADP_INTERFACE_ADV_WAITING; //UCT
		break;

	case ADP_INTERFACE_ADV_LINK_DOWN:
		switch (event) {
		case ADP_INTERFACE_ADV_EVENT_LINK_UP:
			adp_ieee_advertise_entity_sm(entity, ADP_ENTITY_ADV_EVENT_ADVERTISE);
			avb_itf_dynamic->u.ieee.advertise_state = ADP_INTERFACE_ADV_WAITING;
			break;

		default:
			break;
		}
		break;

	default:
		os_log(LOG_ERR, "Received event %s in unknown state %d\n", adp_ieee_advertise_interface_event2string(event), state);
		goto err;
	}

	os_log(LOG_DEBUG, "entity(%p) port(%u) : event %s, state from %s to %s\n", entity, port_id,
			adp_ieee_advertise_interface_event2string(event), adp_ieee_advertise_interface_state2string(state),
			adp_ieee_advertise_interface_state2string(avb_itf_dynamic->u.ieee.advertise_state));

	if (state != avb_itf_dynamic->u.ieee.advertise_state) {
		event = ADP_INTERFACE_ADV_EVENT_RUN;
		goto start;
	}

	return 0;

err:
	return -1;
}

static void adp_ieee_advertise_entity_reannounce_timeout(void *data)
{
	struct adp_ieee_advertise_entity_ctx *adv = data;
	struct entity *entity = container_of(adv, struct entity, adp.ieee.advertise);

	if (entity_ready(entity)) {
		adp_ieee_advertise_entity_sm(entity, ADP_ENTITY_ADV_EVENT_REANNOUNCE_TIMEOUT);
		os_log(LOG_DEBUG, "adv(%p)\n", adv);
	}
}

static void adp_ieee_advertise_entity_delay_timeout(void *data)
{
	struct adp_ieee_advertise_entity_ctx *adv= data;
	struct entity *entity = container_of(adv, struct entity, adp.ieee.advertise);

	if (entity_ready(entity)) {
		adp_ieee_advertise_entity_sm(entity, ADP_ENTITY_ADV_EVENT_DELAY_TIMEOUT);
		os_log(LOG_DEBUG, "adv(%p)\n", adv);
	}
}

__init static int adp_ieee_init_timers(struct avdecc_ctx *avdecc, struct adp_ieee_advertise_entity_ctx *adv)
{
	adv->reannounce_timer.func = adp_ieee_advertise_entity_reannounce_timeout;
	adv->reannounce_timer.data = adv;

	if (timer_create(avdecc->timer_ctx, &adv->reannounce_timer, TIMER_TYPE_SYS, 0) < 0)
		goto err_reannounce_timer;

	adv->delay_timer.func = adp_ieee_advertise_entity_delay_timeout;
	adv->delay_timer.data = adv;

	if (timer_create(avdecc->timer_ctx, &adv->delay_timer, TIMER_TYPE_SYS, 0) < 0)
		goto err_delay_timer;

	os_log(LOG_INIT, "adv(%p)\n", adv);

	return 0;

err_delay_timer:
	timer_destroy(&adv->reannounce_timer);
err_reannounce_timer:
	return -1;
}

__exit static int adp_ieee_exit_timers(struct avdecc_ctx *avdecc, struct adp_ieee_advertise_entity_ctx *adv)
{
	timer_destroy(&adv->reannounce_timer);
	timer_destroy(&adv->delay_timer);

	os_log(LOG_INIT, "adv(%p) done\n", adv);

	return 0;
}

__init int adp_ieee_advertise_init(struct adp_ieee_advertise_entity_ctx *adv)
{
	struct entity *entity = container_of(adv, struct entity, adp.ieee.advertise);
	struct avb_interface_dynamic_desc *avb_itf_dynamic;
	int num_interfaces, i;

	if (adp_ieee_init_timers(entity->avdecc, adv) < 0) {
		os_log(LOG_CRIT, "Cannot initialize timers\n");
		goto err;
	}

	adv->state = ADP_ENTITY_ADV_NOT_STARTED;

	num_interfaces = aem_get_descriptor_max(entity->aem_dynamic_descs, AEM_DESC_TYPE_AVB_INTERFACE);

	for (i = 0; i < num_interfaces; i++) {
		avb_itf_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_AVB_INTERFACE, i, NULL);

		avb_itf_dynamic->interface_index = i;

		avb_itf_dynamic->entity = entity;

		avb_itf_dynamic->u.ieee.advertise_state = ADP_INTERFACE_ADV_NOT_STARTED;

		os_log(LOG_DEBUG, "entity(%p) avb_interface(%p, %d)\n", entity, avb_itf_dynamic, i);
	}

	os_log(LOG_INIT, "adv(%p) num interfaces(%u) done\n", adv, num_interfaces);

	return 0;

err:
	return -1;
}

__exit int adp_ieee_advertise_exit(struct adp_ieee_advertise_entity_ctx *adv)
{
	struct entity *entity = container_of(adv, struct entity, adp.ieee.advertise);

	if (entity_ready(entity))
		adp_ieee_advertise_entity_sm(entity, ADP_ENTITY_ADV_EVENT_TERMINATE);

	adp_ieee_exit_timers(entity->avdecc, adv);

	os_log(LOG_INIT, "done\n");

	return 0;
}
