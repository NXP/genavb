/*
 * Copyright 2021-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @brief ACMP Milan common definitions
*/

#ifndef _ACMP_MILAN_H_
#define _ACMP_MILAN_H_

#include "common/acmp.h"
#include "genavb/control_srp.h"
#include "genavb/control_avdecc.h"

typedef enum {
	ACMP_LISTENER_SINK_SM_EVENT_UNKNOWN = -1,
	ACMP_LISTENER_SINK_SM_EVENT_TMR_NO_RESP = 0,
	ACMP_LISTENER_SINK_SM_EVENT_TMR_RETRY,
	ACMP_LISTENER_SINK_SM_EVENT_TMR_DELAY,
	ACMP_LISTENER_SINK_SM_EVENT_TMR_NO_TK,
	ACMP_LISTENER_SINK_SM_EVENT_RCV_BIND_RX_CMD,
	ACMP_LISTENER_SINK_SM_EVENT_RCV_PROBE_TX_RESP,
	ACMP_LISTENER_SINK_SM_EVENT_RCV_GET_RX_STATE,
	ACMP_LISTENER_SINK_SM_EVENT_RCV_UNBIND_RX_CMD,
	ACMP_LISTENER_SINK_SM_EVENT_EVT_TK_DISCOVERED,
	ACMP_LISTENER_SINK_SM_EVENT_EVT_TK_DEPARTED,
	ACMP_LISTENER_SINK_SM_EVENT_EVT_TK_REGISTERED,
	ACMP_LISTENER_SINK_SM_EVENT_EVT_TK_UNREGISTERED,
	ACMP_LISTENER_SINK_SM_EVENT_SAVED_BINDING_PARAMS
} acmp_milan_listener_sink_sm_event_t;

typedef enum {
	ACMP_LISTENER_SINK_SM_STATE_UNBOUND = 0,
	ACMP_LISTENER_SINK_SM_STATE_PRB_W_AVAIL,
	ACMP_LISTENER_SINK_SM_STATE_PRB_W_DELAY,
	ACMP_LISTENER_SINK_SM_STATE_PRB_W_RESP,
	ACMP_LISTENER_SINK_SM_STATE_PRB_W_RESP2,
	ACMP_LISTENER_SINK_SM_STATE_PRB_W_RETRY,
	ACMP_LISTENER_SINK_SM_STATE_SETTLED_NO_RSV,
	ACMP_LISTENER_SINK_SM_STATE_SETTLED_RSV_OK
} acmp_milan_listener_sink_sm_state_t;

typedef enum {
	ACMP_LISTENER_SINK_SRP_STATE_NOT_REGISTERING = 0,
	ACMP_LISTENER_SINK_SRP_STATE_REGISTERING,
} acmp_milan_listener_sink_srp_state_t;

/**
 * ACMP Probing Status codes about the probing status of a STREAM_INPUT
 * Follows definition of section 6.8.6 from MILAN Discovery, connection and control specification for talkers and listeners rev1.1a */
typedef enum{
	ACMP_PROBING_STATUS_DISABLED	= 0,	/**< The sink is not probing because it is not bound. */
	ACMP_PROBING_STATUS_PASSIVE	= 1,	/**< The sink is probing passively. It waits until the bound talker has been discovered. */
	ACMP_PROBING_STATUS_ACTIVE	= 2,	/**< The sink is probing actively. It is querying the stream parameters to the talker. */
	ACMP_PROBING_STATUS_COMPLETED	= 3,	/**< The sink is not probing because it is settled. */
} acmp_milan_probing_status_t;

#define ACMP_MILAN_IS_LISTENER_SINK_BOUND(stream_input_dynamic) (((stream_input_dynamic)->u.milan.state != ACMP_LISTENER_SINK_SM_STATE_UNBOUND))
#define ACMP_MILAN_IS_LISTENER_SINK_SETTLED(stream_input_dynamic) (((stream_input_dynamic)->u.milan.state == ACMP_LISTENER_SINK_SM_STATE_SETTLED_NO_RSV) || \
									((stream_input_dynamic)->u.milan.state == ACMP_LISTENER_SINK_SM_STATE_SETTLED_RSV_OK))

#define ACMP_MILAN_TALKER_HAS_ACTIVE_LISTENER(stream_output_dynamic) (((stream_output_dynamic)->u.milan.srp_listener_status == ACTIVE_LISTENER) || \
									((stream_output_dynamic)->u.milan.srp_listener_status == ACTIVE_AND_FAILED_LISTENERS))

struct avdecc_ctx;
struct acmp_ctx;
struct entity;

int acmp_milan_talker_rcv(struct acmp_ctx *acmp, struct acmp_pdu *pdu, u8 msg_type, u8 status, unsigned int port_id);
int acmp_milan_listener_sink_event(struct entity *entity, u16 listener_unique_id, acmp_milan_listener_sink_sm_event_t event);
int acmp_milan_listener_rcv(struct acmp_ctx *acmp, struct acmp_pdu *pdu, u8 msg_type, u8 status, unsigned int port_id);
int acmp_milan_get_listener_unique_id(struct entity *entity, u64 stream_id, u16 *listener_unique_id);
int acmp_milan_get_talker_unique_id(struct entity *entity, u64 stream_id, u16 *talker_unique_id);
void acmp_milan_listener_srp_state_sm(struct entity *entity, u16 listener_unique_id, struct genavb_msg_listener_status *ipc_listener_status);
void acmp_milan_talker_update_status(struct entity *entity, u16 talker_unique_id, struct genavb_msg_talker_status *ipc_talker_status);
void acmp_milan_talker_update_declaration(struct entity *entity, u16 talker_unique_id, struct genavb_msg_talker_declaration_status *ipc_talker_declaration_status);
int acmp_milan_init(struct acmp_ctx *acmp);
int acmp_milan_exit(struct acmp_ctx *acmp);
int acmp_milan_get_command_timeout_ms(acmp_message_type_t msg_type);
bool acmp_milan_is_stream_running(struct entity *entity, u16 stream_desc_type, u16 stream_desc_index);
int acmp_milan_start_streaming(struct entity *entity, u16 stream_desc_type, u16 stream_desc_index);
int acmp_milan_stop_streaming(struct entity *entity, u16 stream_desc_type, u16 stream_desc_index);
const char *acmp_milan_msgtype2string(acmp_message_type_t msg_type);
int acmp_milan_talkers_maap_start(struct entity *entity);
void acmp_milan_talker_maap_conflict(struct entity *entity, avb_u16 port_id, avb_u32 range_id, avb_u8 *base_address, avb_u16 count);
void acmp_milan_talker_maap_valid(struct entity *entity, avb_u16 port_id, avb_u32 range_id, avb_u8 *base_address, avb_u16 count);
int acmp_milan_listener_sink_rcv_binding_params(struct entity *entity, struct genavb_msg_media_stack_bind *binding_params);

#endif /* _ACMP_MILAN_H_ */
