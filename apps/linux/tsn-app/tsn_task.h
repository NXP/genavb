/*
 * Copyright 2019-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _TSN_TASK_H_
#define _TSN_TASK_H_

#include "../common/stats.h"
#include "../common/thread.h"
#include "tsn_timer.h"

#include <genavb/clock.h>
#include <genavb/socket.h>

#define MAX_RX_SOCKET 2
#define MAX_TX_SOCKET 2

#define RX 0
#define TX 1

#define ST_TX_TIME_MARGIN 1000 /* Additional margin to account for drift between MAC and gPTP clocks */
#define ST_TX_TIME_FACTOR 2    /* Factor applied to critical time interval, to avoid frames getting stuck */
#define ST_LIST_LEN	  3

#define SCHEDULE_LATENCY_THRESHOLD (250000) /* packets will not be sent if app is woken up more than threshold ns after expected time */

enum net_flags {
	NET_OK,
	NET_NO_FRAME,
	NET_ERR,
};

struct tsn_task_params {
	genavb_clock_id_t clk_id;
	unsigned int task_period_ns;
	unsigned int task_period_offset_ns; //modulo 1 second
	unsigned int transfer_time_ns;
	int sched_traffic_offset;

	unsigned int timer_type;

	int num_rx_socket;
	int rx_buf_size;
	struct genavb_socket_rx_params rx_params[MAX_RX_SOCKET];

	int num_tx_socket;
	int tx_buf_size;
	struct genavb_socket_tx_params tx_params[MAX_TX_SOCKET];
};

struct net_socket_stats {
	unsigned int frames;
	unsigned int err;
	bool pending;
};

struct tsn_task_stats {
	struct stats sched_err;
	struct hist sched_err_hist;
	struct stats proc_time;
	struct hist proc_time_hist;
	struct stats total_time;
	struct hist total_time_hist;
	bool stats_valid;
	unsigned int sched;
	unsigned int sched_early;
	unsigned int sched_late;
	unsigned int sched_missed;
	unsigned int sched_timeout;
	unsigned int clock_discont;
	unsigned int clock_err;
	unsigned int sched_err_max;
	bool pending;
};

struct net_socket {
	int id;
	int dir;
	union {
		struct genavb_socket_rx *genavb_rx;
		struct genavb_socket_tx *genavb_tx;
	};
	void *buf;
	int len;
	uint64_t ts;

	struct net_socket_stats stats;
	struct net_socket_stats stats_snap;
};

struct tsn_task {
	int id;
	struct tsn_timer *timer;
	struct tsn_task_params *params;
	void *ctx;
	unsigned int clock_discont;

	struct net_socket sock_rx[MAX_RX_SOCKET];
	struct net_socket sock_tx[MAX_TX_SOCKET];

	struct tsn_task_stats stats;
	struct tsn_task_stats stats_snap;

	uint64_t sched_time;
	uint64_t sched_now;
};

enum msg_type_id {
	MSG_MOTOR_SET_IQ = 0x0,
	MSG_MOTOR_FEEDBACK = 0x1,
	MSG_SERIAL = 0x2,
};

struct tsn_common_hdr {
	uint16_t msg_id;
	uint16_t len;
	uint16_t src_id;
	uint64_t sched_time;
};

static inline uint64_t tsn_task_get_time(struct tsn_task *task)
{
	return task->sched_time;
}

static inline uint64_t tsn_task_get_now(struct tsn_task *task)
{
	return task->sched_now;
}

static inline struct net_socket *tsn_net_sock_rx(struct tsn_task *task, int id)
{
	return &task->sock_rx[id];
}

static inline struct net_socket *tsn_net_sock_tx(struct tsn_task *task, int id)
{
	return &task->sock_tx[id];
}

static inline void *tsn_net_sock_buf(struct net_socket *socket)
{
	return socket->buf;
}

int tsn_task_register(struct tsn_task **task, struct tsn_task_params *params,
		      int id, int (*main_loop)(void *, unsigned int), void *ctx);
void tsn_task_deregister(struct tsn_task *task);
int tsn_task_start(struct tsn_task *task);
void tsn_task_stop(struct tsn_task *task);
int tsn_net_receive_set_cb(struct net_socket *sock,
			   void (*net_rx_cb)(void *));
int tsn_net_receive_enable_cb(struct net_socket *sock);
int tsn_net_receive_sock(struct net_socket *sock);
int tsn_net_transmit_sock(struct net_socket *sock);
void tsn_stats_dump(struct tsn_task *task);
void tsn_task_stats_start(struct tsn_task *task, int count, uint64_t now);
void tsn_task_stats_end(struct tsn_task *task);
void tsn_task_stats_get_monitoring(struct tsn_task *task, uint32_t *sched_err_max, uint32_t *transmit_time_max, uint32_t nb_socket_rx);
void tsn_task_stats_print(struct tsn_task *task);
void tsn_task_stats_reset(struct tsn_task *task);
void net_socket_stats_print(struct net_socket *sock);

#endif /* _TSN_TASK_H_ */
