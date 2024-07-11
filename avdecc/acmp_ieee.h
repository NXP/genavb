/*
 * Copyright 2014 Freescale Semiconductor, Inc.
 * Copyright 2020-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @brief ACMP IEEE 1722.1 common definitions
*/

#ifndef _ACMP_IEEE_H_
#define _ACMP_IEEE_H_

#include "common/types.h"
#include "common/acmp.h"
#include "common/timer.h"
#include "common/ipc.h"

#define ACMP_LISTENER_FL_FAST_CONNECT 		(1 << 0)	/* Stream is configured for fast connect */
#define ACMP_LISTENER_FL_FAST_CONNECT_BTB	(1 << 1)	/* Stream is configured for fast connect back-to-back */
#define ACMP_LISTENER_FL_FAST_CONNECT_PENDING	(1 << 2)	/* All fast connect information is available */

/**
 * Listener stream information context
 * 8.2.2.2.2
 */
#define listener_stream_info stream_input_dynamic_desc

/**
 * Listener pair context
 * 8.2.2.2.3
 */
struct listener_pair {
	u64 listener_entity_id;
	u16 listener_unique_id;
	u8 connected;
};

/**
 * Talker stream information context
 * 8.2.2.2.4
 */
#define talker_stream_info stream_output_dynamic_desc

struct avdecc_ctx;
struct acmp_ctx;
struct entity;
struct acmp_ctx;

int acmp_ieee_init(struct acmp_ctx *acmp, void *data, struct avdecc_entity_config *cfg);
int acmp_ieee_exit(struct acmp_ctx *acmp);
unsigned int acmp_ieee_data_size(struct avdecc_entity_config *cfg);
void acmp_ieee_listener_fast_connect(struct acmp_ctx *acmp, u64 entity_id, unsigned int port_id);
void acmp_ieee_listener_fast_connect_btb(struct acmp_ctx *acmp, u64 entity_id, unsigned int port_id);
void acmp_ieee_listener_talker_left(struct acmp_ctx *acmp, u64 entity_id);
bool acmp_ieee_is_stream_running(struct entity *entity, u16 stream_desc_type, u16 stream_desc_index);
const char *acmp_ieee_msgtype2string(acmp_message_type_t msg_type);
int acmp_ieee_get_command_timeout_ms(acmp_message_type_t msg_type);
int acmp_ieee_listener_rcv(struct acmp_ctx *acmp, struct acmp_pdu *pdu, u8 msg_type, u8 status, unsigned int port_id);
int acmp_ieee_talker_rcv(struct acmp_ctx *acmp, struct acmp_pdu *pdu, u8 msg_type, u8 status, unsigned int port_id);

#endif /* _ACMP_IEEE_H_ */
