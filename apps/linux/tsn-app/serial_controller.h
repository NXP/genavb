/*
 * Copyright 2020-2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _SERIAL_CONTROLLER_H_
#define _SERIAL_CONTROLLER_H_

#include <stdint.h>
#include <semaphore.h>

#define MAX_SERIAL_COMMAND_LEN 50

struct pt_cmd {
	int avail;
	int abort;
	unsigned int len;
	unsigned int rem;
	char buf[MAX_SERIAL_COMMAND_LEN];
	unsigned int buf_size;
};

struct serial_controller_stats {
	unsigned int net_cmds;
	unsigned int net_null_cmds;
	unsigned int net_invalid_msg_id;
	unsigned int pt_read_bytes;
	unsigned int pt_read_cmds;
	unsigned int pt_read_abort_cmds;
	unsigned int pt_write_errors;
	unsigned int pt_write_incomplete;
	bool pending;
};

struct serial_controller_ctx {
	struct cyclic_task *c_task;
	void *pt_thread;

	int pt_fd;
	struct pt_cmd pt_cmd;
	sem_t cmd_sem;

	struct serial_controller_stats stats;
	struct serial_controller_stats stats_snap;
};

struct msg_serial {
	uint16_t cmd_len;
	uint8_t cmd[MAX_SERIAL_COMMAND_LEN];
};

struct serial_controller_ctx *serial_controller_init(unsigned int period_ns, unsigned int num_peers, int pt_fd, unsigned int timer_type);
void serial_controller_exit(void *data);
void serial_controller_stats_handler(void *data);

#endif /* _SERIAL_CONTROLLER_H_ */
