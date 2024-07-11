/*
 * Copyright 2021-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @brief ADP_MILAN common defitions
*/

#ifndef _ADP_MILAN_H_
#define _ADP_MILAN_H_

#include "common/types.h"
#include "common/ipc.h"
#include "common/adp.h"

#define ADP_MILAN_ADV_TMR_ADVERTISE_MS 5000
#define ADP_MILAN_ADV_TMR_DELAY_MS_INIT_MIN 100
#define ADP_MILAN_ADV_TMR_DELAY_MS_INIT_MAX 2000
#define ADP_MILAN_ADV_TMR_DELAY_MS_MIN 100
#define ADP_MILAN_ADV_TMR_DELAY_MS_MAX 4000
#define ADP_MILAN_TMR_GRANULARITY_MS 100

/* STATES */
/* Advertise states as defined in MILAN Discovery,connection and control specification [9.3.2] */
typedef enum {
	ADP_MILAN_ADV_NOT_STARTED,
	ADP_MILAN_ADV_DOWN,
	ADP_MILAN_ADV_WAITING,
	ADP_MILAN_ADV_DELAY,
} adp_milan_advertise_state_t;

/* Discovery states as defined in MILAN Discovery,connection and control specification [9.4.2] */
typedef enum {
	ADP_LISTENER_SINK_TALKER_STATE_NOT_DISCOVERED = 0,
	ADP_LISTENER_SINK_TALKER_STATE_DISCOVERED,
} adp_milan_listener_sink_talker_state_t;

/* EVENTS */
/* Advertise events as defined in MILAN Discovery,connection and control specification [9.3.3] */
typedef enum {
	ADP_MILAN_ADV_START,
	ADP_MILAN_ADV_RCV_ADP_DISCOVER,
	ADP_MILAN_ADV_TMR_ADVERTISE,
	ADP_MILAN_ADV_TMR_DELAY,
	ADP_MILAN_ADV_LINK_UP,
	ADP_MILAN_ADV_LINK_DOWN,
	ADP_MILAN_ADV_GM_CHANGE,
	ADP_MILAN_ADV_SHUTDOWN,
} adp_milan_advertise_event_t;

/* Discovery events as defined in MILAN Discovery,connection and control specification [9.4.3] */
typedef enum {
	ADP_MILAN_LISTENER_SINK_RCV_ADP_AVAILABLE,
	ADP_MILAN_LISTENER_SINK_RCV_ADP_DEPARTING,
	ADP_MILAN_LISTENER_SINK_TMR_NO_ADP,
	ADP_MILAN_LISTENER_SINK_RESET,
} adp_milan_listener_sink_event_t;


/* STATE MACHINE */
/* Advertise and Listener Discovery states [9.3.4 and 9.4.4] are tracked in the dynamic descriptors */

/* CONTEXT */
/* Advertise and Listener Discovery contexts are managed in the dynamic descriptors */

struct avdecc_ctx;
struct adp_ctx;
struct entity;

int adp_milan_advertise_init(struct adp_ctx *adp);
int adp_milan_advertise_exit(struct adp_ctx *adp);
int adp_milan_listener_sink_discovery_init(struct adp_ctx *adp);
int adp_milan_listener_sink_discovery_exit(struct adp_ctx *adp);

void adp_milan_advertise_start(struct adp_ctx *adp);

int adp_milan_advertise_sm(struct entity *entity, unsigned int port_id, adp_milan_advertise_event_t event);
int adp_milan_listener_sink_discovery_sm(struct entity *entity, u16 listener_unique_id, adp_milan_listener_sink_event_t event, u8 valid_time, struct adp_pdu *pdu);

void adp_milan_listener_rcv(struct entity *entity, u8 msg_type, struct adp_pdu *pdu, u8 valid_time);

#endif /* _ADP_MILAN_H_ */
