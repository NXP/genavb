/*
 * Copyright 2020-2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _NETWORK_ONLY_H_
#define _NETWORK_ONLY_H_

#include <stdint.h>
#include <semaphore.h>

struct network_only_ctx {
	struct cyclic_task *c_task;
};

struct network_only_ctx *network_only_init(unsigned int role, unsigned int period_ns, unsigned int num_peers, unsigned int timer_type);
void network_only_exit(void *data);
void network_only_stats_handler(void *data);

#endif /* _NETWORK_ONLY_H_ */
