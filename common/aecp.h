/*
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
 * Copyright 2017-2018, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file		aecp.h
 @brief  	AECP protocol common definitions
 @details	PDU and protocol definitions for all AECP applications
*/

#ifndef _PROTO_AECP_H_
#define _PROTO_AECP_H_

#include "common/types.h"
#include "common/avtp.h"
#include "genavb/aecp.h"

#define OFFSET_TO_AECP (sizeof(struct eth_hdr) + sizeof(struct avtp_ctrl_hdr))
#define OFFSET_TO_AECP_SPECIFIC (sizeof(struct eth_hdr) + sizeof(struct avtp_ctrl_hdr) + sizeof(struct aecp_aem_pdu))

/* 250 ms timeout for all AECP commands (1722.1-2013 section 9.2.1.2.5) */
#define AECP_COMMANDS_TIMEOUT	250
#define AECP_IN_PROGRESS_TIMEOUT	120


#endif /* _PROTO_AECP_H_ */
