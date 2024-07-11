/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _COMMON_ACMP_H_
#define _COMMON_ACMP_H_

#include <genavb/genavb.h>


int acmp_connect_stream(struct avb_control_handle *ctrl_h, avb_u64 talker_entity_id, avb_u16 talker_unique_id, avb_u64 listener_entity_id, avb_u16 listener_unique_id, avb_u16 flags, struct avb_acmp_response *acmp_rsp);
int acmp_disconnect_stream(struct avb_control_handle *ctrl_h, avb_u64 talker_entity_id, avb_u16 talker_unique_id, avb_u64 listener_entity_id, avb_u16 listener_unique_id, struct avb_acmp_response *acmp_rsp);
int acmp_get_rx_state(struct avb_control_handle *ctrl_h, avb_u64 listener_entity_id, avb_u16 listener_unique_id, struct avb_acmp_response *acmp_rsp);
int acmp_get_tx_state(struct avb_control_handle *ctrl_h, avb_u64 talker_entity_id, avb_u16 talker_unique_id, struct avb_acmp_response *acmp_rsp);
int acmp_get_tx_connection(struct avb_control_handle *ctrl_h, avb_u64 talker_entity_id, avb_u16 talker_unique_id, avb_u16 connection_count, struct avb_acmp_response *acmp_rsp);

#endif /* _COMMON_ACMP_H_ */
