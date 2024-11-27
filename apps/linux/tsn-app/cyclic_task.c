/*
 * Copyright 2019-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <unistd.h>
#include <signal.h>

#include "cyclic_task.h"
#include "tsn_tasks_config.h"
#include "opcua/opcua_server.h"

#include "../common/log.h"
#include "../common/time.h"

#ifdef TRACE_SNAPSHOT
#include <fcntl.h>
#define TRACE_SNAPSHOT_FILE "/sys/kernel/debug/tracing/snapshot"
#define TRACE_SNAPSHOT_TAG "1"
#define TRACE_SNAPSHOT_THRESHOLD	50000
#endif

static bool reset_stats;

void cyclic_stats_reset_handler(void)
{
    reset_stats = true;
}

void socket_stats_print(struct socket *sock)
{
	if (sock->stats_snap.pending) {
		stats_compute(&sock->stats_snap.traffic_latency);

		INF("cyclic rx socket(%p) net_sock(%p) peer id: %d", sock, sock->net_sock, sock->peer_id);
		INF("valid frames  : %u", sock->stats_snap.valid_frames);
		INF("err id        : %u", sock->stats_snap.err_id);
		INF("err ts        : %u", sock->stats_snap.err_ts);
		INF("err underflow : %u", sock->stats_snap.err_underflow);
		INF("link %s", sock->stats_snap.link_status ? "up" : "down");

		stats_print(&sock->stats_snap.traffic_latency);
		hist_print(&sock->stats_snap.traffic_latency_hist);

		opcua_update_cyclic_socket(sock);

		sock->stats_snap.pending = false;
	}
}

static void socket_stats_dump(struct socket *sock)
{
	if (sock->stats_snap.pending)
		return;

	memcpy(&sock->stats_snap, &sock->stats, sizeof(struct socket_stats));
	sock->stats_snap.pending = true;

	stats_reset(&sock->stats.traffic_latency);
}

void cyclic_stats_print(struct cyclic_task *c_task)
{
	struct socket *sock;
	struct tsn_task *task = c_task->task;
	int i;

	tsn_task_stats_print(task);

	/* RX sockets */
	for (i = 0; i < c_task->num_peers; i++) {
		sock = &c_task->rx_socket[i];
		socket_stats_print(sock);
		net_socket_stats_print(sock->net_sock);
	}

	/* TX socket */
	sock = &c_task->tx_socket;
	socket_stats_print(sock);
	net_socket_stats_print(sock->net_sock);
}

static void cyclic_stats_dump(struct cyclic_task *c_task)
{
	int i;

	for (i = 0; i < c_task->num_peers; i++)
		socket_stats_dump(&c_task->rx_socket[i]);
}

static void cyclic_task_stats_reset(struct cyclic_task *c_task)
{
	int i;

	tsn_task_stats_reset(c_task->task);

	for (i = 0; i < c_task->num_peers; i++) {
		c_task->rx_socket[i].stats.traffic_latency_min = 0xffffffff;
		c_task->rx_socket[i].stats.traffic_latency_max = 0;
		c_task->rx_socket[i].stats.err_id = 0;
		c_task->rx_socket[i].stats.err_ts = 0;
		c_task->rx_socket[i].stats.err_underflow = 0;
		c_task->rx_socket[i].stats.valid_frames = 0;

		stats_init(&c_task->rx_socket[i].stats.traffic_latency, 31, "traffic latency", NULL);
		hist_reset(&c_task->rx_socket[i].stats.traffic_latency_hist);
	}
}

#ifdef TRACE_SNAPSHOT
void cyclic_task_init_trace_snapshot(struct cyclic_task *c_task)
{
       c_task->trace_snapshot_fd = open(TRACE_SNAPSHOT_FILE, O_WRONLY);
}

static void cyclic_task_take_trace_snapshot(struct cyclic_task *c_task)
{
	if (c_task->trace_snapshot_fd > 0)
				if (write(c_task->trace_snapshot_fd, TRACE_SNAPSHOT_TAG, sizeof(TRACE_SNAPSHOT_TAG)) < 0)
				 	ERR("Could not write to trace snapshot file %s\n", TRACE_SNAPSHOT_FILE);
}

#endif

static void cyclic_net_receive(struct cyclic_task *c_task)
{
	int i;
	int status;
	struct socket *sock;
	struct tsn_task *task = c_task->task;
	struct tsn_common_hdr *hdr;
	int rx_frame;
	uint32_t traffic_latency;

	for (i = 0; i < c_task->num_peers; i++) {
		sock = &c_task->rx_socket[i];
		rx_frame = 0;
	retry:
		status = tsn_net_receive_sock(sock->net_sock);
		if (status == NET_NO_FRAME && !rx_frame) {
			sock->stats.err_underflow++;
#ifdef TRACE_SNAPSHOT
			cyclic_task_take_trace_snapshot(c_task);
#endif
		}

		if (status != NET_OK) {
			sock->stats.link_status = 0;
			continue;
		}

		rx_frame = 1;

		hdr = tsn_net_sock_buf(sock->net_sock);
		if (hdr->sched_time != (tsn_task_get_time(task) - task->params->transfer_time_ns)) {
			sock->stats.err_ts++;
			goto retry;
		}

		if (hdr->src_id != sock->peer_id) {
			sock->stats.err_id++;
			goto retry;
		}

		traffic_latency = sock->net_sock->ts - hdr->sched_time;

		stats_update(&sock->stats.traffic_latency, traffic_latency);
		hist_update(&sock->stats.traffic_latency_hist, traffic_latency);

		if (traffic_latency > sock->stats.traffic_latency_max)
			sock->stats.traffic_latency_max = traffic_latency;

		if (traffic_latency < sock->stats.traffic_latency_min)
			sock->stats.traffic_latency_min = traffic_latency;

		sock->stats.valid_frames++;
		sock->stats.link_status = 1;

		if (c_task->net_rx_func)
			c_task->net_rx_func(c_task->ctx, hdr->msg_id, hdr->src_id,
					    hdr + 1, hdr->len);
	}
}

int cyclic_net_transmit(struct cyclic_task *c_task, int msg_id, void *buf, int len)
{
	struct tsn_task *task = c_task->task;
	struct socket *sock = &c_task->tx_socket;
	struct tsn_common_hdr *hdr = tsn_net_sock_buf(sock->net_sock);
	int total_len = (len + sizeof(*hdr));
	int status;

	if (total_len >= task->params->tx_buf_size)
		goto err;

	hdr->msg_id = msg_id;
	hdr->src_id = c_task->id;
	hdr->sched_time = tsn_task_get_time(task);
	hdr->len = len;

	if (len)
		memcpy(hdr + 1, buf, len);

	sock->net_sock->len = total_len;

	status = tsn_net_transmit_sock(sock->net_sock);
	if (status != NET_OK)
		goto err;

	return 0;

err:
	return -1;
}

static int main_cyclic(void *data, unsigned int events)
{
	struct cyclic_task *c_task = data;
	struct tsn_task *task = c_task->task;
	unsigned int num_sched_stats = CYCLIC_STAT_PERIOD_SEC * (NSECS_PER_SEC / task->params->task_period_ns);
	uint64_t n_time;
	uint64_t now;
	int rc = -1;

	if (genavb_clock_gettime64(task->params->clk_id, &now) < 0) {
		ERR("genavb_clock_gettime64() error\n");
		goto timer_err;
	}

	if (reset_stats) {
		cyclic_task_stats_reset(c_task);
		reset_stats = false;
	}
	
	rc = tsn_timer_check(task->timer, now, &n_time);
	if (rc < 0)
		goto timer_err;

	tsn_task_stats_start(task, (int)n_time, now);

#ifdef TRACE_SNAPSHOT
	if ((now - task->sched_time) > TRACE_SNAPSHOT_THRESHOLD) {
		cyclic_task_take_trace_snapshot(c_task);
	}
#endif

	/*
	 * Receive, frames should be available
	 */
	cyclic_net_receive(c_task);

	/*
	 * Main loop
	 */
	if (c_task->loop_func)
		c_task->loop_func(c_task->ctx, 0);

	tsn_task_stats_end(task);

	if (!(task->stats.sched % num_sched_stats)) {
		tsn_stats_dump(task);
		cyclic_stats_dump(c_task);
	}

	return 0;

timer_err:
	if (c_task->loop_func)
		c_task->loop_func(c_task->ctx, -1);

	cyclic_task_stop(c_task);
	cyclic_task_start(c_task);

#ifdef ECANCELED
	if (rc == ECANCELED)
		task->stats.clock_discont++;
	else
#endif
		task->stats.clock_err++;

	return rc;
}

void cyclic_task_set_period(struct cyclic_task *c_task, unsigned int period_ns)
{
	struct tsn_task_params *params = &c_task->params;

	/* Use default config */
	if (!period_ns)
		return;

	params->task_period_ns = period_ns;
	params->transfer_time_ns = period_ns / 2;

	if (c_task->type == CYCLIC_CONTROLLER)
		params->task_period_offset_ns = 0;
	else
		params->task_period_offset_ns = period_ns / 2;
}

int cyclic_task_set_num_peers(struct cyclic_task *c_task, unsigned int num_peers)
{
	/* Use default config */
	if (!num_peers)
		return 0;

	if (num_peers > MAX_PEERS)
		return -1;

	c_task->num_peers = num_peers;

	return 0;
}

void cyclic_task_get_monitoring(struct cyclic_task *task, uint32_t *sched_err_max, uint32_t *traffic_latency_max,
				uint32_t *traffic_latency_min, uint32_t num_socket_monitored)
{
	uint32_t i;

	*sched_err_max = task->task->stats.sched_err_max;
	task->task->stats.sched_err_max = 0;
	for (i = 0; i < task->num_peers; i++) {
		traffic_latency_max[i] = task->rx_socket[i].stats.traffic_latency_max;
		traffic_latency_min[i] = task->rx_socket[i].stats.traffic_latency_min;

		task->rx_socket[i].stats.traffic_latency_max = 0;
		task->rx_socket[i].stats.traffic_latency_min = 0xffffffff;

		if (i >= num_socket_monitored)
			return;
	}
}

int cyclic_task_start(struct cyclic_task *c_task)
{
	unsigned int period_ns = c_task->params.task_period_ns;

	if (!period_ns || ((NSECS_PER_SEC / period_ns) * period_ns != NSECS_PER_SEC)) {
		ERR("invalid task period(%u ns), needs to be an integer divider of 1 second\n", period_ns);
		return -1;
	}

	return tsn_task_start(c_task->task);
}

void cyclic_task_stop(struct cyclic_task *c_task)
{
	tsn_task_stop(c_task->task);
}

int cyclic_task_init(struct cyclic_task *c_task,
		     void (*net_rx_func)(void *ctx, int msg_id, int src_id, void *buf, int len),
		     void (*loop_func)(void *ctx, int timer_status), void *ctx)
{
	struct tsn_task_params *params = &c_task->params;
	struct tsn_stream *rx_stream, *tx_stream;
	int i;
	int rc;

	INF("cyclic task type: %d, id: %d, num peers: %d\n",
	    c_task->type, c_task->id, c_task->num_peers);
	INF("task params");
	INF("clk_id                : %u", params->clk_id);
	INF("task_period_ns        : %u", params->task_period_ns);
	INF("task_period_offset_ns : %u", params->task_period_offset_ns);
	INF("transfer_time_ns      : %u", params->transfer_time_ns);

	tx_stream = tsn_conf_get_stream(c_task->tx_socket.stream_id);
	if (!tx_stream)
		goto err;

	memcpy(&params->tx_params[0].addr, &tx_stream->address,
	       sizeof(struct net_address));
	params->num_tx_socket = 1;
	params->tx_params[0].addr.port = 0;

	for (i = 0; i < c_task->num_peers; i++) {
		c_task->rx_socket[i].id = i;
		rx_stream = tsn_conf_get_stream(c_task->rx_socket[i].stream_id);
		if (!rx_stream)
			goto err;

		memcpy(&params->rx_params[i].addr, &rx_stream->address,
		       sizeof(struct net_address));
		params->rx_params[i].addr.port = 0;
		params->num_rx_socket++;
	}

	c_task->net_rx_func = net_rx_func;
	c_task->loop_func = loop_func;
	c_task->ctx = ctx;

	rc = tsn_task_register(&c_task->task, params, c_task->id, main_cyclic, c_task);
	if (rc < 0) {
		ERR("tsn_task_register() failed rc = %d\n", rc);
		goto err;
	}

	for (i = 0; i < c_task->num_peers; i++) {
		c_task->rx_socket[i].net_sock = tsn_net_sock_rx(c_task->task, i);
		c_task->rx_socket[i].stats.traffic_latency_min = 0xffffffff;

		stats_init(&c_task->rx_socket[i].stats.traffic_latency, 31, "traffic latency", NULL);
		hist_init(&c_task->rx_socket[i].stats.traffic_latency_hist, 100, 1000);
	}

	c_task->tx_socket.net_sock = tsn_net_sock_tx(c_task->task, 0);

	INF("success\n");

	opcua_init_params(c_task);

	return 0;

err:
	return -1;
}

void cyclic_task_exit(struct cyclic_task *c_task)
{
	tsn_task_deregister(c_task->task);
}
