/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Clock Domain interface handling
 @details
*/
#ifndef _CLOCK_DOMAIN_H_
#define _CLOCK_DOMAIN_H_

#include "common/ipc.h"
#include "common/list.h"
#include "common/timer.h"

#include "clock_grid.h"

#include "clock_source.h"

struct avtp_ctx;

struct clock_domain {
	unsigned int id;
	unsigned int source_type;
	union {
		u8 stream_id[8];
		u16 local_id;
	};
	unsigned int source_flags;
	unsigned int status;
	unsigned int state;
	unsigned int locked_count;
	unsigned int ts_update_n;
	struct clock_source *source;
	struct list_head grids;				/**< List of existing grids for the clock_domain. Each grid may be used by one or more consumers. */
	struct list_head sched_streams[CFG_SR_CLASS_MAX];		/**< FIXME WAKEUP list of consumer streams to schedule for this domain (will be removed when switching to media and net event wake-up). */
	struct media_clock_rec *hw_sync;
	struct clock_source hw_source;
	struct clock_source stream_source;
};

#define CLOCK_DOMAIN_SOURCE_FLAG_USER	(1 << 0)

typedef enum {
	CLOCK_DOMAIN_STATE_LOCKED = (1 << 0),
	CLOCK_DOMAIN_STATE_FREE_WHEELING = (1 << 1)
} clock_domain_state_t;


struct ipc_avtp_clock_domain_stats {
	void *domain;
	unsigned int domain_id;
	unsigned int domain_status;
	unsigned int domain_locked_count;
};

void clock_domain_stats_print(struct ipc_avtp_clock_domain_stats *msg);
void clock_domain_ipc_rx_media_stack(struct ipc_rx const *rx, struct ipc_desc *desc);
int clock_domain_set_source(struct clock_domain *domain, struct clock_source *source, void *data);
int clock_domain_set_wakeup(struct clock_domain *domain, unsigned int freq_p, unsigned int freq_q);
void clock_domain_clear_state(struct clock_domain *domain, clock_domain_state_t state);
void clock_domain_set_state(struct clock_domain *domain, clock_domain_state_t state);
unsigned int clock_domain_is_locked(struct clock_domain *domain);
struct clock_domain * clock_domain_get(struct avtp_ctx *avtp, unsigned int id);
unsigned int clock_domain_is_source_stream(struct clock_domain *domain, void *stream_id);
int __clock_domain_update_source(struct clock_domain *domain, struct clock_source *new_source, void *data);
int clock_domain_set_source_legacy(struct clock_domain *domain, struct avtp_ctx *avtp, struct ipc_avtp_connect *ipc);
void clock_domain_stats_dump(struct clock_domain *domain, struct ipc_tx *tx);
void clock_domain_sched(struct clock_domain *domain, unsigned int prio);
int clock_domain_init_consumer(struct clock_domain *domain, struct clock_grid_consumer *consumer, unsigned int offset, u32 nominal_freq_p, u32 nominal_freq_q, unsigned int alignment, unsigned int prio);
void clock_domain_exit_consumer(struct clock_grid_consumer *consumer);
int clock_domain_init_consumer_wakeup(struct clock_domain *domain, struct clock_grid_consumer *consumer, unsigned int wake_freq_p, unsigned int wake_freq_q);
void clock_domain_exit_consumer_wakeup(struct clock_grid_consumer *consumer);
struct clock_grid *clock_domain_find_grid(struct clock_domain *domain, u32 nominal_freq_p, u32 nominal_freq_q);
void clock_domain_add_grid(struct clock_domain *domain, struct clock_grid *grid);
void clock_domain_remove_grid(struct clock_grid *grid);
void clock_domain_init(struct clock_domain *domain, unsigned int id, unsigned long priv);
void clock_domain_exit(struct clock_domain *domain);

#endif /* _CLOCK_DOMAIN_H_ */
