/*
 * Copyright 2020-2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Frame preemption state machines
 @details
*/

#include "common/log.h"
#include "genavb/frame_preemption.h"

#include "frame_preemption.h"
#include "net_logical_port.h"
#include "net_port.h"

enum {
	FP_VERIFY_STATE_INIT_VERIFICATION,
	FP_VERIFY_STATE_VERIFICATION_IDLE,
	FP_VERIFY_STATE_SEND_VERIFY,
	FP_VERIFY_STATE_WAIT_FOR_RESPONSE,
	FP_VERIFY_STATE_VERIFIED,
	FP_VERIFY_STATE_VERIFY_FAIL
};

enum {
	FP_RESPONSE_STATE_RESPOND_IDLE,
	FP_RESPONSE_STATE_SEND_RESPOND
};

#define FP_VERIFY_LIMIT 3

int net_port_fp_status_verify(struct fp_verify_sm *sm)
{
	if (sm->disable_verify)
		return GENAVB_FP_STATUS_VERIFY_DISABLED;
	else if (sm->v_state == FP_VERIFY_STATE_INIT_VERIFICATION)
		return GENAVB_FP_STATUS_VERIFY_INITIAL;
	else if (sm->v_state == FP_VERIFY_STATE_VERIFIED)
		return GENAVB_FP_STATUS_VERIFY_SUCCEEDED;
	else if (sm->v_state == FP_VERIFY_STATE_VERIFY_FAIL)
		return GENAVB_FP_STATUS_VERIFY_FAILED;
	else
		return GENAVB_FP_STATUS_VERIFY_VERIFYING;
}

/*
 * 802.3br-2016, Figure 99-8, a) Verify State Diagram
 */
static void fp_verify_state_verify_fail(struct fp_verify_sm *sm)
{
	sm->v_state = FP_VERIFY_STATE_VERIFY_FAIL;
	sm->verify_fail = true;
}

static void fp_verify_state_verified(struct fp_verify_sm *sm)
{
	sm->v_state = FP_VERIFY_STATE_VERIFIED;
	sm->verified = true;
}

static void fp_verify_state_wait_for_response(struct fp_verify_sm *sm)
{
	sm->v_state = FP_VERIFY_STATE_WAIT_FOR_RESPONSE;
	sm->verify_cnt++;
	sm->timer_expired = false;
	sm->start_timer(sm->priv, sm->verify_time);
}

static void fp_verify_state_send_verify(struct fp_verify_sm *sm)
{
	sm->v_state = FP_VERIFY_STATE_SEND_VERIFY;
	sm->snd_v = true;

	sm->send_verify(sm->priv);
}

static void fp_verify_state_verification_idle(struct fp_verify_sm *sm)
{
	sm->v_state = FP_VERIFY_STATE_VERIFICATION_IDLE;
}

static void fp_verify_state_init_verification(struct fp_verify_sm *sm)
{
	sm->v_state = FP_VERIFY_STATE_INIT_VERIFICATION;
	sm->snd_v = false;
	sm->rcv_v = false;
	sm->snd_r = false;
	sm->rcv_r = false;
	sm->verified = false;
	sm->verify_fail = false;
	sm->verify_cnt = 0;
}

static void fp_update_pactive(struct fp_verify_sm *sm)
{
	bool p_active;

	p_active = (sm->p_enable && (sm->verified || sm->disable_verify));

	if (p_active != sm->p_active) {
		if (p_active)
			sm->enable(sm->priv);
		else
			sm->disable(sm->priv);

		sm->p_active = p_active;
	}
}

static void fp_verify_state_fsm(struct fp_verify_sm *sm)
{
	unsigned int state;

start:
	os_log(LOG_DEBUG, "%u\n", sm->v_state);

	state = sm->v_state;

	if (sm->begin || sm->link_fail || sm->disable_verify || !sm->p_enable) {
		fp_verify_state_init_verification(sm);
		return;
	}

	switch (sm->v_state) {
	case FP_VERIFY_STATE_INIT_VERIFICATION:
		fp_verify_state_verification_idle(sm);
		break;

	case FP_VERIFY_STATE_VERIFICATION_IDLE:
		if (sm->p_enable && !sm->disable_verify)
			fp_verify_state_send_verify(sm);

		break;

	case FP_VERIFY_STATE_SEND_VERIFY:
		if (!sm->snd_v)
			fp_verify_state_wait_for_response(sm);

		break;

	case FP_VERIFY_STATE_WAIT_FOR_RESPONSE:
		if (sm->rcv_r)
			fp_verify_state_verified(sm);
		else if (sm->timer_expired) {
			if (sm->verify_cnt < FP_VERIFY_LIMIT)
				fp_verify_state_verification_idle(sm);
			else
				fp_verify_state_verify_fail(sm);
		}

		break;

	case FP_VERIFY_STATE_VERIFIED:
		break;

	case FP_VERIFY_STATE_VERIFY_FAIL:
		break;

	default:
		break;
	}

	if (sm->v_state != state)
		goto start;
}

/*
 * 802.3br-2016, Figure 99-8, b) Respond State Diagram
 */
static void fp_response_state_respond_idle(struct fp_verify_sm *sm)
{
	sm->r_state = FP_RESPONSE_STATE_RESPOND_IDLE;
}

static void fp_response_state_send_respond(struct fp_verify_sm *sm)
{
	sm->rcv_v = false;
	sm->snd_r = true;

	sm->r_state = FP_RESPONSE_STATE_SEND_RESPOND;

	sm->send_response(sm->priv);
}

static void fp_response_state_fsm(struct fp_verify_sm *sm)
{
	unsigned int state;

start:
	os_log(LOG_DEBUG, "%u\n", sm->r_state);

	state = sm->r_state;

	if (sm->begin) {
		fp_response_state_respond_idle(sm);
		return;
	}

	switch (sm->r_state) {
	case FP_RESPONSE_STATE_RESPOND_IDLE:
		if (sm->rcv_v)
			fp_response_state_send_respond(sm);

		break;

	case FP_RESPONSE_STATE_SEND_RESPOND:
		if (!sm->snd_r)
			fp_response_state_respond_idle(sm);

		break;

	default:
		break;
	}

	if (sm->r_state != state)
		goto start;
}

void net_port_fp_schedule(struct fp_verify_sm *sm)
{
	fp_verify_state_fsm(sm);
	fp_response_state_fsm(sm);

	fp_update_pactive(sm);
}

void net_port_fp_event(struct fp_verify_sm *sm, unsigned int event)
{
	switch (event) {
	case FP_EVENT_RECEIVED_VERIFY:
		sm->rcv_v = true;
		break;

	case FP_EVENT_TRANSMITTED_RESPONSE:
		sm->snd_r = false;
		break;

	case FP_EVENT_TRANSMITTED_VERIFY:
		sm->snd_v = false;
		break;

	case FP_EVENT_RECEIVED_RESPONSE:
		sm->rcv_r = true;
		break;

	case FP_EVENT_VERIFY_ENABLE:
		sm->disable_verify = false;
		break;

	case FP_EVENT_VERIFY_DISABLE:
		sm->disable_verify = true;
		break;

	case FP_EVENT_PREEMPTION_DISABLE:
		sm->p_enable = false;
		break;

	case FP_EVENT_PREEMPTION_ENABLE:
		sm->p_enable = true;
		break;

	case FP_EVENT_LINK_FAIL:
		sm->link_fail = true;
		break;

	case FP_EVENT_LINK_OK:
		sm->link_fail = false;
		break;

	case FP_EVENT_TIMER_EXPIRED:
		sm->timer_expired = true;
		break;

	default:
		break;
	}
}

void net_port_fp_init(struct fp_verify_sm *sm, void *priv)
{
	sm->priv = priv;

	sm->p_enable = false;
	sm->disable_verify = false;
	sm->p_active = false;
	sm->verify_time = FP_VERIFY_TIME_MAX;

	sm->begin = true;
	net_port_fp_schedule(sm);
	sm->begin = false;
	net_port_fp_schedule(sm);
}

int fp_set(unsigned int port_id, unsigned int type, struct genavb_fp_config *config)
{
	struct logical_port *port;

	port = logical_port_get(port_id);
	if (!port)
		return -1;

	return port_set_fp(port->phys, type, config);
}

int fp_get(unsigned int port_id, unsigned int type, struct genavb_fp_config *config)
{
	struct logical_port *port;

	port = logical_port_get(port_id);
	if (!port)
		return -1;

	return port_get_fp(port->phys, type, config);
}
