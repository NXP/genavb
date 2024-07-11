/*
 * Copyright 2019-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <unistd.h>
#include <sys/epoll.h>

#include <genavb/genavb.h>
#include <genavb/socket.h>
#include <genavb/qos.h>
#include <genavb/ether.h>

#include "tsn_task.h"
#include "../common/log.h"
#include "../common/time.h"
#include "../common/helpers.h"
#include "opcua/opcua_server.h"

void tsn_task_stats_init(struct tsn_task *task)
{
	stats_init(&task->stats.sched_err, 31, "sched err", NULL);
	hist_init(&task->stats.sched_err_hist, 100, 10000);

	stats_init(&task->stats.proc_time, 31, "processing time", NULL);
	hist_init(&task->stats.proc_time_hist, 100, 1000);

	stats_init(&task->stats.total_time, 31, "total time", NULL);
	hist_init(&task->stats.total_time_hist, 100, 1000);

	task->stats.sched_err_max = 0;
}

void tsn_task_stats_reset(struct tsn_task *task)
{
	stats_init(&task->stats.sched_err, 31, "sched err", NULL);
	hist_reset(&task->stats.sched_err_hist);

	stats_init(&task->stats.proc_time, 31, "processing time", NULL);
	hist_reset(&task->stats.proc_time_hist);

	stats_init(&task->stats.total_time, 31, "total time", NULL);
	hist_reset(&task->stats.total_time_hist);

	task->stats.sched = 0;
	task->stats.sched_early = 0;
	task->stats.sched_late = 0;
	task->stats.sched_missed = 0;
	task->stats.sched_timeout = 0;
	task->stats.clock_discont = 0;
	task->stats.clock_err = 0;
	task->stats.sched_err_max = 0;
}

void tsn_task_stats_start(struct tsn_task *task, int count, uint64_t now)
{
	int32_t sched_err;

	task->sched_time += (count * task->params->task_period_ns);

	task->stats.sched += count;

	if (task->stats.stats_valid) {
		sched_err = now - task->sched_time;

		if (sched_err > SCHEDULE_LATENCY_THRESHOLD)
			task->stats.sched_late++;

		if (count > 1)
			task->stats.sched_missed += (count - 1);

		if (sched_err < 0) {
			task->stats.sched_early++;
			sched_err = -sched_err;
		}

		stats_update(&task->stats.sched_err, sched_err);
		hist_update(&task->stats.sched_err_hist, sched_err);

		if (sched_err > task->stats.sched_err_max)
			task->stats.sched_err_max = sched_err;
	} else
		task->stats.stats_valid = true;

	task->sched_now = now;
}

void tsn_task_stats_end(struct tsn_task *task)
{
	uint64_t now = 0;
	int32_t proc_time;
	int32_t total_time;

	if (task->stats.stats_valid) {
		genavb_clock_gettime64(task->params->clk_id, &now);

		proc_time = now - task->sched_now;
		total_time = now - task->sched_time;

		stats_update(&task->stats.proc_time, proc_time);
		hist_update(&task->stats.proc_time_hist, proc_time);

		stats_update(&task->stats.total_time, total_time);
		hist_update(&task->stats.total_time_hist, total_time);
	}
}

void tsn_task_stats_print(struct tsn_task *task)
{
	struct tsn_task_stats *stats = &task->stats_snap;

	if (stats->pending) {
		stats_compute(&stats->sched_err);
		stats_compute(&stats->proc_time);
		stats_compute(&stats->total_time);

		INF("tsn task(%p)", task);
		INF("sched           : %u", stats->sched);
		INF("sched early     : %u", stats->sched_early);
		INF("sched late      : %u", stats->sched_late);
		INF("sched missed    : %u", stats->sched_missed);
		INF("sched timeout   : %u", stats->sched_timeout);
		INF("clock discont   : %u", stats->clock_discont);
		INF("clock err       : %u", stats->clock_err);

		stats_print(&stats->sched_err);
		hist_print(&stats->sched_err_hist);

		stats_print(&stats->proc_time);
		hist_print(&stats->proc_time_hist);

		stats_print(&stats->total_time);
		hist_print(&stats->total_time_hist);

		stats->pending = false;

		opcua_update_task_stats(stats);
	}
}

static void tsn_task_stats_dump(struct tsn_task *task)
{
	if (task->stats_snap.pending)
		return;

	memcpy(&task->stats_snap, &task->stats, sizeof(struct tsn_task_stats));
	task->stats_snap.pending = true;

	stats_reset(&task->stats.sched_err);
	stats_reset(&task->stats.proc_time);
	stats_reset(&task->stats.total_time);
}

void net_socket_stats_print(struct net_socket *sock)
{
	struct net_socket_stats *stats = &sock->stats_snap;

	if (stats->pending) {
		INF("net %s socket(%p) %d", sock->dir ? "tx" : "rx", sock, sock->id);
		INF("frames     : %u", stats->frames);
		INF("err        : %u", stats->err);

		opcua_update_net_socket_stats(sock);

		stats->pending = false;
	}
}

static void net_socket_stats_dump(struct net_socket *sock)
{
	if (sock->stats_snap.pending)
		return;

	memcpy(&sock->stats_snap, &sock->stats, sizeof(struct net_socket_stats));
	sock->stats_snap.pending = true;
}

void tsn_stats_dump(struct tsn_task *task)
{
	int i;

	tsn_task_stats_dump(task);

	for (i = 0; i < task->params->num_rx_socket; i++)
		net_socket_stats_dump(&task->sock_rx[i]);

	for (i = 0; i < task->params->num_tx_socket; i++)
		net_socket_stats_dump(&task->sock_tx[i]);
}

int tsn_net_receive_sock(struct net_socket *sock)
{
	struct tsn_task *task = container_of(sock, struct tsn_task, sock_rx[sock->id]);
	int len;
	int status;

	len = genavb_socket_rx(sock->genavb_rx, sock->buf, task->params->rx_buf_size, &sock->ts);
	if (len > 0) {
		status = NET_OK;
		sock->len = len;
		sock->stats.frames++;
	} else if (len == -GENAVB_ERR_SOCKET_AGAIN) {
		status = NET_NO_FRAME;
	} else {
		status = NET_ERR;
		sock->stats.err++;
	}

	return status;
}

int tsn_net_transmit_sock(struct net_socket *sock)
{
	int rc;
	int status;

	rc = genavb_socket_tx(sock->genavb_tx, sock->buf, sock->len);
	if (rc == GENAVB_SUCCESS) {
		status = NET_OK;
		sock->stats.frames++;
	} else {
		status = NET_ERR;
		sock->stats.err++;
	}

	return status;
}

#if 0
int tsn_net_receive_set_cb(struct net_socket *sock, void (*net_rx_cb)(void *))
{
	int rc;

	rc = genavb_socket_rx_set_callback(sock->genavb_rx, net_rx_cb, sock);
	if (rc != GENAVB_SUCCESS)
		return -1;

	return 0;
}

int tsn_net_receive_enable_cb(struct net_socket *sock)
{
	int rc;

	rc = genavb_socket_rx_enable_callback(sock->genavb_rx);
	if (rc != GENAVB_SUCCESS)
		return -1;

	return 0;
}
/*
 * Returns the complete transmit time including MAC framing and physical
 * layer overhead (802.3).
 * \return transmit time in nanoseconds
 * \param frame_size frame size without any framing
 * \param speed_mbps link speed in Mbps
 */
static unsigned int frame_tx_time_ns(unsigned int frame_size, int speed_mbps)
{
	unsigned int eth_size;

	eth_size = sizeof(struct eth_hdr) + frame_size + ETHER_FCS;

	if (eth_size < ETHER_MIN_FRAME_SIZE)
		eth_size = ETHER_MIN_FRAME_SIZE;

	eth_size += ETHER_IFG + ETHER_PREAMBLE;

	return (((1000 / speed_mbps) * eth_size * 8) + ST_TX_TIME_MARGIN);
}

static void tsn_net_st_config_enable(struct tsn_task *task, uint64_t now)
{
	struct genavb_st_config config;
	struct genavb_st_gate_control_entry gate_list[ST_LIST_LEN];
	struct net_address *addr = &task->params->tx_params[0].addr;
	unsigned int cycle_time = task->params->task_period_ns;
	uint8_t iso_traffic_prio = addr->priority;
	uint8_t tclass = priority_to_traffic_class_map(CFG_TRAFFIC_CLASS_MAX, CFG_SR_CLASS_MAX)[iso_traffic_prio];
	unsigned int iso_tx_time = frame_tx_time_ns(task->params->tx_buf_size, 1000) * ST_TX_TIME_FACTOR;
	unsigned int guard_band = frame_tx_time_ns(ETHER_MTU, 1000);

	gate_list[0].operation = GENAVB_ST_SET_GATE_STATES;
	gate_list[0].gate_states = 1 << tclass;
	gate_list[0].time_interval = iso_tx_time;

	gate_list[1].operation = GENAVB_ST_SET_GATE_STATES;
	gate_list[1].gate_states = ~(1 << tclass);
	gate_list[1].time_interval = cycle_time - iso_tx_time - guard_band;

	gate_list[2].operation = GENAVB_ST_SET_GATE_STATES;
	gate_list[2].gate_states = 0;
	gate_list[2].time_interval = guard_band;

	config.enable = 1;
	config.base_time = (now / cycle_time) * cycle_time;
	config.base_time += task->params->task_period_offset_ns + task->params->sched_traffic_offset;
	config.cycle_time_p = cycle_time;
	config.cycle_time_q = NSECS_PER_SEC;
	config.cycle_time_ext = 0;
	config.list_length = ST_LIST_LEN;
	config.control_list = gate_list;

	if (genavb_st_set_admin_config(addr->port, task->params->clk_id, &config) < 0)
		ERR("genavb_st_set_admin_config() error\n");
	else
		INF("scheduled traffic config enabled\n");
}

static void tsn_net_st_config_disable(struct tsn_task *task)
{
	struct genavb_st_config config;
	struct net_address *addr = &task->params->tx_params[0].addr;

	config.enable = 0;

	if (genavb_st_set_admin_config(addr->port, task->params->clk_id, &config) < 0)
		ERR("genavb_qos_st_set_admin_config() error\n");
	else
		INF("scheduled traffic config disabled\n");
}

static void tsn_net_st_oper_config_print(struct tsn_task *task)
{
	int i;
	struct genavb_st_config config;
	struct genavb_st_gate_control_entry gate_list[ST_LIST_LEN];
	struct net_address *addr = &task->params->tx_params[0].addr;

	config.control_list = gate_list;

	if (genavb_st_get_config(addr->port, GENAVB_ST_OPER, &config, ST_LIST_LEN) < 0) {
		ERR("genavb_qos_st_get_config() error\n");
		return;
	}

	INF("base time   : %llu\n", config.base_time);
	INF("cycle time  : %u / %u\n", config.cycle_time_p, config.cycle_time_q);
	INF("ext time    : %u\n", config.cycle_time_ext);

	for (i = 0; i < config.list_length; i++)
		INF("%u op: %u, interval: %u, gates: %b\n",
		    i, gate_list[i].operation, gate_list[i].time_interval, gate_list[i].gate_states);
}
#endif
int tsn_task_start(struct tsn_task *task)
{
	uint64_t now, start_time;

	if (genavb_clock_gettime64(task->params->clk_id, &now) != GENAVB_SUCCESS) {
		ERR("genavb_clock_gettime64() error\n");
		goto err;
	}

	/* Start time = rounded up second + 1 second */
	start_time = ((now + NSECS_PER_SEC / 2) / NSECS_PER_SEC + 1) * NSECS_PER_SEC;

	/* Align on cycle time and add offset */
	start_time = (start_time / task->params->task_period_ns) * task->params->task_period_ns + task->params->task_period_offset_ns;

	if (tsn_timer_start(task->timer, start_time, task->params->task_period_ns) < 0) {
		ERR("tsn timer start error\n");
		goto err;
	}

	/* Sched time is updated at the start of the app cycle to take account of
	 * number of expirations. So here it's set one period before */
	task->sched_time = start_time - task->params->task_period_ns;

	DBG("now(%" PRIu64 ") sched_time(%" PRIu64 ") sched_next%" PRIu64 ")", now, task->sched_time, task->nsleep.next);
	//tsn_net_st_config_enable(task, now);

	return 0;

err:
	return -1;
}

void tsn_task_stop(struct tsn_task *task)
{
	tsn_timer_stop(task->timer);
}

#ifdef SRP_RESERVATION
static struct genavb_control_handle *s_msrp_handle = NULL;

static uint8_t tsn_stream_id[8] = {0xaa, 0xaa, 0xaa, 0xaa, 0xbb, 0xbb, 0xbb, 0x00};

static int msrp_init(struct genavb_handle *s_avb_handle)
{
	int genavb_result;
	int rc;

	genavb_result = genavb_control_open(s_avb_handle, &s_msrp_handle, GENAVB_CTRL_MSRP);
	if (genavb_result != GENAVB_SUCCESS) {
		ERR("avb_control_open() failed: %s\n", genavb_strerror(genavb_result));
		rc = -1;
		goto err_control_open;
	}

	return 0;

err_control_open:
	return rc;
}

static int msrp_exit(void)
{
	genavb_control_close(s_msrp_handle);

	s_msrp_handle = NULL;

	return 0;
}

static int tsn_net_rx_srp_register(struct genavb_socket_rx_params *params)
{
	struct genavb_msg_listener_register listener_register;
	struct genavb_msg_listener_response listener_response;
	struct net_address *addr = &params->addr;
	unsigned int msg_type, msg_len;
	int rc;

	listener_register.port = addr->port;
	memcpy(listener_register.stream_id, tsn_stream_id, 8);
	listener_register.stream_id[7] = addr->u.l2.dst_mac[5];

	INF("stream_params: %p\n", listener_register.stream_id);

	msg_type = GENAVB_MSG_LISTENER_REGISTER;
	msg_len = sizeof(listener_response);
	rc = genavb_control_send_sync(s_msrp_handle, (genavb_msg_type_t *)&msg_type, &listener_register, sizeof(listener_register), &listener_response, &msg_len, 1000);
	if ((rc != GENAVB_SUCCESS) || (msg_type != GENAVB_MSG_LISTENER_RESPONSE) || (listener_response.status != GENAVB_SUCCESS)) {
		ERR(STREAM_STR_FMT " failed: %s\n", STREAM_STR(listener_register.stream_id), genavb_strerror(rc));
		return -1;
	}

	return 0;
}

static int tsn_net_rx_srp_deregister(struct genavb_socket_rx_params *params)
{
	struct genavb_msg_listener_deregister listener_deregister;
	struct genavb_msg_listener_response listener_response;
	struct net_address *addr = &params->addr;
	unsigned int msg_type, msg_len;
	int rc;

	listener_deregister.port = addr->port;
	memcpy(listener_deregister.stream_id, tsn_stream_id, 8);
	listener_deregister.stream_id[7] = addr->u.l2.dst_mac[5];

	INF("stream_params: %p\n", listener_deregister.stream_id);

	msg_type = GENAVB_MSG_LISTENER_DEREGISTER;
	msg_len = sizeof(listener_response);
	rc = genavb_control_send_sync(s_msrp_handle, (genavb_msg_type_t *)&msg_type, &listener_deregister, sizeof(listener_deregister), &listener_response, &msg_len, 1000);
	if ((rc != GENAVB_SUCCESS) || (msg_type != GENAVB_MSG_LISTENER_RESPONSE) || (listener_response.status != GENAVB_SUCCESS)) {
		ERR(STREAM_STR_FMT " failed: %s\n", STREAM_STR(listener_deregister.stream_id), genavb_strerror(rc));
		return -1;
	}

	return 0;
}

static int tsn_net_tx_srp_register(struct genavb_socket_tx_params *params)
{
	struct genavb_msg_talker_register talker_register;
	struct genavb_msg_talker_response talker_response;
	struct net_address *addr = &params->addr;
	unsigned int msg_type, msg_len;
	int rc;

	talker_register.port = addr->port;
	memcpy(talker_register.stream_id, tsn_stream_id, 8);
	talker_register.stream_id[7] = addr->u.l2.dst_mac[5];

	talker_register.params.stream_class = SR_CLASS_A;
	memcpy(talker_register.params.destination_address, addr->u.l2.dst_mac, 6);
	talker_register.params.vlan_id = ntohs(addr->vlan_id);

	talker_register.params.max_frame_size = 200;
	talker_register.params.max_interval_frames = 1;
	talker_register.params.accumulated_latency = 0;
	talker_register.params.rank = NORMAL;

	msg_type = GENAVB_MSG_TALKER_REGISTER;
	msg_len = sizeof(talker_response);

	rc = genavb_control_send_sync(s_msrp_handle, (genavb_msg_type_t *)&msg_type, &talker_register, sizeof(talker_register), &talker_response, &msg_len, 1000);
	if ((rc != GENAVB_SUCCESS) || (msg_type != GENAVB_MSG_TALKER_RESPONSE) || (talker_response.status != GENAVB_SUCCESS)) {
		ERR(STREAM_STR_FMT " failed: %s\n", STREAM_STR(talker_register.stream_id), genavb_strerror(rc));
		return -1;
	}

	return 0;
}

static int tsn_net_tx_srp_deregister(struct genavb_socket_tx_params *params)
{
	struct genavb_msg_talker_deregister talker_deregister;
	struct genavb_msg_talker_response talker_response;
	struct net_address *addr = &params->addr;
	unsigned int msg_type, msg_len;
	int rc;

	talker_deregister.port = addr->port;
	memcpy(talker_deregister.stream_id, tsn_stream_id, 8);
	talker_deregister.stream_id[7] = addr->u.l2.dst_mac[5];

	msg_type = GENAVB_MSG_TALKER_DEREGISTER;
	msg_len = sizeof(talker_response);
	rc = genavb_control_send_sync(s_msrp_handle, (genavb_msg_type_t *)&msg_type, &talker_deregister, sizeof(talker_deregister), &talker_response, &msg_len, 1000);
	if ((rc != GENAVB_SUCCESS) || (msg_type != GENAVB_MSG_TALKER_RESPONSE) || (talker_response.status != GENAVB_SUCCESS)) {
		ERR(STREAM_STR_FMT " failed: %s\n", STREAM_STR(talker_deregister.stream_id), genavb_strerror(rc));

		return -1;
	}

	return 0;
}
#endif

static int tsn_task_net_init(struct tsn_task *task)
{
	int i, j, k, rc;
	struct net_socket *sock;

#ifdef SRP_RESERVATION
	if (msrp_init(get_genavb_handle()) < 0)
		return -1;
#endif

	for (i = 0; i < task->params->num_rx_socket; i++) {
		sock = &task->sock_rx[i];
		sock->id = i;
		sock->dir = RX;

		rc = genavb_socket_rx_open(&sock->genavb_rx, GENAVB_SOCKF_NONBLOCK,
					   &task->params->rx_params[i]);
		if (rc != GENAVB_SUCCESS) {
			ERR("genavb_socket_rx_open error: %s\n", genavb_strerror(rc));
			goto close_sock_rx;
		}

		sock->buf = malloc(task->params->rx_buf_size);
		if (!sock->buf) {
			genavb_socket_rx_close(sock->genavb_rx);
			ERR("error allocating rx_buff\n");
			goto close_sock_rx;
		}

#ifdef SRP_RESERVATION
		tsn_net_rx_srp_register(&task->params->rx_params[i]);
#endif
	}

	for (j = 0; j < task->params->num_tx_socket; j++) {
		sock = &task->sock_tx[j];
		sock->id = j;
		sock->dir = TX;

		rc = genavb_socket_tx_open(&sock->genavb_tx, 0, &task->params->tx_params[j]);
		if (rc != GENAVB_SUCCESS) {
			ERR("genavb_socket_tx_open error: %s\n", genavb_strerror(rc));
			goto close_sock_tx;
		}

		sock->buf = malloc(task->params->tx_buf_size);
		if (!sock->buf) {
			genavb_socket_tx_close(sock->genavb_tx);
			ERR("error allocating tx_buff\n");
			goto close_sock_tx;
		}

#ifdef SRP_RESERVATION
		tsn_net_tx_srp_register(&task->params->tx_params[j]);
#endif
	}

#ifdef SRP_RESERVATION
	msrp_exit();
#endif

	return 0;

close_sock_tx:
	for (k = 0; k < j; k++) {
		sock = &task->sock_tx[k];
#ifdef SRP_RESERVATION
		tsn_net_tx_srp_deregister(&task->params->tx_params[k]);
#endif
		free(sock->buf);
		genavb_socket_tx_close(sock->genavb_tx);
	}

close_sock_rx:
	for (k = 0; k < i; k++) {
		sock = &task->sock_rx[k];
#ifdef SRP_RESERVATION
		tsn_net_rx_srp_deregister(&task->params->rx_params[k]);
#endif
		free(sock->buf);
		genavb_socket_rx_close(sock->genavb_rx);
	}

	return -1;
}

static void tsn_task_net_exit(struct tsn_task *task)
{
	int i;
	struct net_socket *sock;

	for (i = 0; i < task->params->num_rx_socket; i++) {
		sock = &task->sock_rx[i];
#ifdef SRP_RESERVATION
		tsn_net_rx_srp_deregister(&task->params->rx_params[i]);
#endif
		free(sock->buf);
		genavb_socket_rx_close(sock->genavb_rx);
	}

	for (i = 0; i < task->params->num_tx_socket; i++) {
		sock = &task->sock_tx[i];
#ifdef SRP_RESERVATION
		tsn_net_tx_srp_deregister(&task->params->tx_params[i]);
#endif
		free(sock->buf);
		genavb_socket_tx_close(sock->genavb_tx);
	}
}

//note only cyclic mode supported
int tsn_task_register(struct tsn_task **task, struct tsn_task_params *params,
		      int id, int (*main_loop)(void *, unsigned int), void *ctx)
{
	char task_name[16];
	char timer_name[16];

	*task = malloc(sizeof(struct tsn_task));
	if (!(*task))
		goto err;

	memset(*task, 0, sizeof(struct tsn_task));

	(*task)->id = id;
	(*task)->params = params;
	(*task)->ctx = ctx;

	sprintf(task_name, "tsn task%1d", (*task)->id);
	sprintf(timer_name, "task%1d timer", (*task)->id);

	if (tsn_task_net_init(*task) < 0) {
		ERR("tsn_task_net_init error\n");
		goto err_free;
	}

	tsn_task_stats_init(*task);

	(*task)->timer = tsn_timer_init((*task)->params->timer_type, THR_CAP_TSN_LOOP, ctx, main_loop);
	if ((*task)->timer == NULL) {
		ERR("timer registration() failed\n");
		goto net_exit;
	}

	return 0;

net_exit:
	tsn_task_net_exit(*task);

err_free:
	free(*task);

err:
	return -1;
}

void tsn_task_deregister(struct tsn_task *task)
{
	tsn_timer_exit(task->timer);

	tsn_task_net_exit(task);

	free(task);
}
