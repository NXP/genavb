/*
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2020-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @brief ACMP common definitions
*/

#ifndef _ACMP_H_
#define _ACMP_H_

#include "common/types.h"
#include "common/acmp.h"
#include "common/timer.h"
#include "common/ipc.h"

#include "genavb/aem.h"

#include "acmp_ieee.h"
#include "acmp_milan.h"

struct acmp_ctx {
	u16 sequence_id;
	struct list_head inflight;
	unsigned int max_listener_streams;    /* total number of listener streams in the entity */
	unsigned int max_talker_streams;      /* total number of talker streams in the entity */

	union {
		struct {
			struct listener_stream_info *listener_info; /* AVDECC 1722.1 use: array of listeners stream information */
			struct talker_stream_info *talker_info;     /* AVDECC 1722.1 use: array of talkers stream information */
			unsigned int max_listener_pairs;            /* AVDECC 1722.1 use: Maximum number of connected listeners per talker */
		} ieee;
	} u;
};

struct avdecc_port;
struct entity;

int acmp_init(struct acmp_ctx *acmp, void *data, struct avdecc_entity_config *cfg);
int acmp_exit(struct acmp_ctx *acmp);
unsigned int acmp_data_size(struct avdecc_entity_config *cfg);
struct net_tx_desc *acmp_net_tx_init(struct acmp_ctx *acmp, struct avdecc_port *port, struct acmp_pdu *pdu, bool is_resp);
struct net_tx_desc *acmp_net_tx_alloc(struct acmp_ctx *acmp, struct avdecc_port *port);
int acmp_net_rx(struct avdecc_port *port, struct acmp_pdu *pdu, u8 msg_type, u8 status);
int acmp_ipc_rx(struct entity *entity, struct ipc_acmp_command *acmp_command, u32 len,struct ipc_tx *ipc, unsigned int ipc_dst);
void acmp_listener_copy_common_params(struct acmp_pdu *pdu, struct stream_input_dynamic_desc *listener_params);
void acmp_talker_copy_common_params(struct acmp_pdu *pdu, struct stream_output_dynamic_desc *talker_params);
u8 acmp_listener_stack_connect(struct entity *entity, u16 listener_unique_id, u16 flags);
u8 acmp_listener_stack_disconnect(struct entity *entity, u16 unique_id);
int acmp_talker_declare(struct avdecc_ctx *avdecc, unsigned int port_id, struct stream_descriptor *stream_output, struct stream_output_dynamic_desc *stream_output_dynamic, u16 flags);
int acmp_talker_withdraw(struct avdecc_ctx *avdecc, unsigned int port_id, struct stream_output_dynamic_desc *stream_output_dynamic);
int acmp_talker_avtp_stack_connect(struct entity *entity, unsigned int port_id, u16 talker_unique_id, struct stream_descriptor *stream_output, struct stream_output_dynamic_desc *stream_output_dynamic, u16 flags);
int acmp_talker_avtp_stack_disconnect(struct avdecc_ctx *avdecc, unsigned int port_id, u16 talker_unique_id, struct stream_output_dynamic_desc *stream_output_dynamic);
int acmp_send_cmd(struct acmp_ctx *acmp, struct avdecc_port *port, struct acmp_pdu *pdu, struct net_tx_desc *desc, u8 msg_type, u8 retried, u16 orig_seq_id, struct ipc_tx *ipc, unsigned int ipc_dst);
int acmp_send_rsp(struct acmp_ctx *acmp, struct avdecc_port *port, struct net_tx_desc *desc, u8 msg_type, u16 status);
bool acmp_is_stream_running(struct entity *entity, u16 stream_desc_type, u16 stream_desc_index);
int acmp_start_streaming(struct entity *entity, u16 stream_desc_type, u16 stream_desc_index);
int acmp_stop_streaming(struct entity *entity, u16 stream_desc_type, u16 stream_desc_index);
int acmp_talker_unique_valid(struct acmp_ctx *acmp, u16 unique_id);
int acmp_listener_unique_valid(struct acmp_ctx *acmp, u16 unique_id);

#endif /* _ACMP_H_ */
