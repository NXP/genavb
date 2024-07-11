/*
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
 * Copyright 2020-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @brief ADP common defitions
*/

#ifndef _ADP_IEEE_H_
#define _ADP_IEEE_H_

#include "common/types.h"
#include "common/ipc.h"
#include "common/adp.h"

/* IEEE 1722.1 2013-Cor1-2018 6.2.4.3 */
typedef enum {
	ADP_ENTITY_ADV_NOT_STARTED = 0,
	ADP_ENTITY_ADV_WAITING,
	ADP_ENTITY_ADV_ADVERTISE,
	ADP_ENTITY_ADV_DELAY,
	ADP_ENTITY_ADV_RUN
} adp_ieee_advertise_entity_state_t;

typedef enum {
	ADP_ENTITY_ADV_EVENT_BEGIN = 0,
	ADP_ENTITY_ADV_EVENT_ADVERTISE,
	ADP_ENTITY_ADV_EVENT_REANNOUNCE_TIMEOUT,
	ADP_ENTITY_ADV_EVENT_DELAY_TIMEOUT,
	ADP_ENTITY_ADV_EVENT_TERMINATE,
	ADP_ENTITY_ADV_EVENT_RUN
} adp_ieee_advertise_entity_event_t;

struct avdecc_ctx;

struct adp_ieee_advertise_entity_ctx {
	adp_ieee_advertise_entity_state_t state;
	struct timer reannounce_timer;
	struct timer delay_timer;
};

/* IEEE 1722.1 2013 6.2.5.3 */
typedef enum {
	ADP_INTERFACE_ADV_NOT_STARTED = 0,
	ADP_INTERFACE_ADV_WAITING,
	ADP_INTERFACE_ADV_ADVERTISE,
	ADP_INTERFACE_ADV_DEPARTING,
	ADP_INTERFACE_ADV_RECEIVED_DISCOVER,
	ADP_INTERFACE_ADV_UPDATE_GM,
	ADP_INTERFACE_ADV_LINK_DOWN
} adp_ieee_advertise_interface_state_t;

typedef enum {
	ADP_INTERFACE_ADV_EVENT_BEGIN = 0,
	ADP_INTERFACE_ADV_EVENT_ADVERTISE,
	ADP_INTERFACE_ADV_EVENT_RCV_DISCOVER,
	ADP_INTERFACE_ADV_EVENT_GM_CHANGE,
	ADP_INTERFACE_ADV_EVENT_LINK_UP,
	ADP_INTERFACE_ADV_EVENT_LINK_DOWN,
	ADP_INTERFACE_ADV_EVENT_TERMINATE,
	ADP_INTERFACE_ADV_EVENT_RUN
} adp_ieee_advertise_interface_event_t;

struct avdecc_ctx;
struct adp_ctx;
struct entity;

void adp_ieee_advertise_start(struct adp_ctx *adp);

int adp_ieee_advertise_init(struct adp_ieee_advertise_entity_ctx *adv);
int adp_ieee_advertise_exit(struct adp_ieee_advertise_entity_ctx *adv);
int adp_ieee_advertise_sm(struct adp_ieee_advertise_entity_ctx *adv);

int adp_ieee_advertise_entity_sm(struct entity *entity, adp_ieee_advertise_entity_event_t event);
int adp_ieee_advertise_interface_sm(struct entity *entity, unsigned int port_id, adp_ieee_advertise_interface_event_t event);

#endif /* _ADP_IEEE_H_ */
