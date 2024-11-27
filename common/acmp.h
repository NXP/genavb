/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file     	acmp.h
 @brief 	ACMP protocol common definitions
 @details	PDU and protocol definitions for all ACMP applications
*/

#ifndef _PROTO_ACMP_H_
#define _PROTO_ACMP_H_

#include "genavb/acmp.h"
#include "common/types.h"
#include "common/avtp.h"

struct __attribute__ ((packed)) acmp_pdu {
	u64 stream_id;
	u64 controller_entity_id;
	u64 talker_entity_id;
	u64 listener_entity_id;
	u16 talker_unique_id;
	u16 listener_unique_id;
	u8 stream_dest_mac[6];
	u16 connection_count;
	u16 sequence_id;
	u16 flags;
	u16 stream_vlan_id;
	u16 rsvd;
};

#define OFFSET_TO_ACMP (sizeof(struct eth_hdr) + sizeof(struct avtp_ctrl_hdr))
#define ACMP_NET_DATA_SIZE (OFFSET_TO_ACMP + sizeof(struct acmp_pdu))
#define ACMP_IS_LISTENER_COMMAND(msg_type) ((msg_type == ACMP_CONNECT_RX_COMMAND) || (msg_type == ACMP_GET_RX_STATE_COMMAND) || (msg_type == ACMP_DISCONNECT_RX_COMMAND))
#define ACMP_IS_TALKER_COMMAND(msg_type) ((msg_type == ACMP_CONNECT_TX_COMMAND) || (msg_type == ACMP_GET_TX_STATE_COMMAND) || (msg_type == ACMP_DISCONNECT_TX_COMMAND) || (msg_type == ACMP_GET_TX_CONNECTION_COMMAND))
#define ACMP_IS_COMMAND(msg_type) (!(msg_type & 0x1))

#define ACMP_PDU_LEN 44

#endif /* _PROTO_ACMP_H_ */
