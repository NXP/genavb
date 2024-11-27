/*
 * Copyright 2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVTP stack component entry points
 @details
*/

#ifndef _AVTP_ENTRY_H_
#define _AVTP_ENTRY_H_

#include "os/sys_types.h"
#include "genavb/init.h"
#include "common/types.h"
#include "common/stats.h"
#include "common/avtp.h"

struct process_stats {
	struct stats events;
	struct stats sched_intvl;
	struct stats processing_time;
};

void *avtp_init(struct avtp_config *cfg, unsigned long priv);
int avtp_exit(void *avtp_ctx);
void avtp_stats_dump(void *avtp_ctx, struct process_stats *stats);
void avtp_media_event(void *data);
void avtp_net_tx_event(void *data);
void stats_ipc_rx(struct ipc_rx const *rx, struct ipc_desc *desc);
void avtp_ipc_rx(void *avtp_ctx);
void avtp_stream_free(void *avtp_ctx, u64 current_time);

#endif /* _AVTP_ENTRY_H_ */
