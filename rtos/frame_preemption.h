/*
 * Copyright 2020-2021, 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Frame preemption state machines
 @details
*/

#ifndef _RTOS_FRAME_PREEMPTION_H_
#define _RTOS_FRAME_PREEMPTION_H_

#include <stdbool.h>

struct fp_verify_sm {
	/* configuration */
	bool disable_verify;
	bool p_enable;
	unsigned int verify_time;

	/* response state machine */
	unsigned int r_state;

	/* response events */
	bool rcv_v;
	bool snd_r;

	/* verify state machine */
	unsigned int v_state;

	unsigned int verify_cnt;
	bool verify_fail;
	bool verified;
	bool p_active;

	/* verify events */
	bool begin;
	bool link_fail;
	bool snd_v;
	bool rcv_r;
	bool timer_expired;

	void *priv;

	void (*send_response)(void *priv);
	void (*send_verify)(void *priv);
	void (*start_timer)(void *priv, unsigned int ms);
	void (*enable)(void *priv);
	void (*disable)(void *priv);
};

enum {
	FP_EVENT_RECEIVED_RESPONSE,
	FP_EVENT_TRANSMITTED_RESPONSE,
	FP_EVENT_RECEIVED_VERIFY,
	FP_EVENT_TRANSMITTED_VERIFY,
	FP_EVENT_VERIFY_ENABLE,
	FP_EVENT_VERIFY_DISABLE,
	FP_EVENT_PREEMPTION_ENABLE,
	FP_EVENT_PREEMPTION_DISABLE,
	FP_EVENT_LINK_FAIL,
	FP_EVENT_LINK_OK,
	FP_EVENT_TIMER_EXPIRED
};

#define FP_ADD_FRAG_SIZE_MIN	0
#define FP_ADD_FRAG_SIZE_MAX	3

#define FP_VERIFY_TIME_MIN	1
#define FP_VERIFY_TIME_MAX	128

int net_port_fp_status_verify(struct fp_verify_sm *sm);
void net_port_fp_event(struct fp_verify_sm *sm, unsigned int event);
void net_port_fp_schedule(struct fp_verify_sm *sm);
void net_port_fp_init(struct fp_verify_sm *sm, void *priv);

#endif /* _RTOS_FRAME_PREEMPTION_H_ */
