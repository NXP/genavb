/*
 * Copyright 2017-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief RTOS specific Network service implementation
 @details
*/
#include <stdbool.h>

#include "common/log.h"

#include "media_clock.h"
#include "net_rx.h"
#include "net_tx.h"
#include "net_socket.h"
#include "net_port.h"
#include "net_logical_port.h"
#include "net_bridge.h"
#include "frame_preemption.h"

#define NET_TX_STACK_DEPTH	(RTOS_MINIMAL_STACK_SIZE + 256)
#define NET_TX_PRIORITY		(RTOS_MAX_PRIORITY - 2)
#define NET_RX_STACK_DEPTH	(RTOS_MINIMAL_STACK_SIZE + 150)
#define NET_RX_PRIORITY		(RTOS_MAX_PRIORITY - 4)

#define NET_TX_TASK_NAME	"net tx"
#define NET_RX_TASK_NAME	"net rx"

extern struct net_port ports[CFG_PORTS];

static void net_tx_cleanup(void)
{
	struct net_port *port;
	int i;

	for (i = 0; i < CFG_PORTS; i++) {
		port = &ports[i];

		if (port->logical_port->is_bridge)
			continue;

		if (port->tx_up) {
			port_tx_cleanup(port);
		}
	}

#if CFG_BRIDGE_NUM
	for (i = 0; i < CFG_BRIDGE_NUM; i++)
		bridge_tx_cleanup(&bridges[i]);
#endif
}

static void net_tx_task(void *pvParameters)
{
	struct net_tx_ctx *net = pvParameters;
	struct event e;

	os_log(LOG_INIT, "networking(%p) tx task started\n", net);

	for (;;) {
		if (rtos_mqueue_receive(&net->queue, &e, RTOS_WAIT_FOREVER) < 0)
			continue;

		switch (e.type) {
		case EVENT_QOS_SCHED:
			port_scheduler_event(e.data);

			net_tx_cleanup();

			break;

		case EVENT_SOCKET_CONNECT:
			socket_connect_event(e.data);
			break;

		case EVENT_SOCKET_DISCONNECT:
			socket_disconnect_event(e.data);
			break;

		case EVENT_PHY_UP:
			port_tx_up(e.data);
			break;

		case EVENT_PHY_DOWN:
			port_tx_down(e.data);
			break;

		case EVENT_SR_CONFIG:
			net_qos_sr_config_event(e.data);
			break;

		case EVENT_FP_SCHED:
			net_port_fp_schedule(e.data);
			break;

		case EVENT_SOCKET_CALLBACK:
			socket_set_callback_event(e.data);
			break;

		default:
			break;
		}
	}

	os_log(LOG_INIT, "networking(%p) tx task(%p) exited\n", net);

	rtos_thread_abort(NULL);
}

bool static inline net_rx_skip(void)
{
#if defined(NET_RX_PERIOD_MUL) && (NET_RX_PERIOD_MUL > 1)
	static int n = 0;

	n++;
	if (n < NET_RX_PERIOD_MUL)
		return true;

	n = 0;
#endif
	return false;
}

static void net_rx_task(void *pvParameters)
{
	struct net_rx_ctx *net = pvParameters;
	struct event e;
	struct net_port *port;
	unsigned int rx_now;
	int i, queue;

	os_log(LOG_INIT, "networking(%p) rx task started\n", net);

	if (os_timer_start(&net->timer.handle, 0, net->timer.period, 1, 0) < 0)
		os_log(LOG_ERR, "Net rx timer failed to start\n");

	for (;;) {
		if (rtos_mqueue_receive(&net->queue, &e, RTOS_WAIT_FOREVER) < 0)
			continue;

		rtos_mutex_lock(&net->mutex, RTOS_WAIT_FOREVER);

		switch (e.type) {
		case EVENT_RX_TIMER:
			net->time += NET_RX_TX_PERIOD;

			if (net_rx_skip())
				break;

			for (i = 0; i < CFG_PORTS; i++) {
				port = &ports[i];

				if (port->logical_port->is_bridge)
					continue;

				if (!port->up)
					continue;

				for (queue = port->num_rx_q - 1, rx_now = NET_RX_PACKETS; queue >= 0 && rx_now; queue--)
					rx_now -= port_rx(net, port, rx_now, queue);
			}

#if CFG_BRIDGE_NUM
			for (i = 0; i < CFG_BRIDGE_NUM; i++)
				bridge_rx(net, &bridges[i], NET_RX_BRIDGE_PACKETS);
#endif

			socket_poll_all(net);

			break;

		case EVENT_PHY_UP:
			port = e.data;

			port_up(port);


			break;

		case EVENT_PHY_DOWN:
			port = e.data;

			port_down(port);

			net_rx_flush(net, port);
			break;

		/* Control path events */
		case EVENT_SOCKET_BIND:
			socket_bind_event(e.data);
			break;

		case EVENT_SOCKET_UNBIND:
			socket_unbind_event(e.data);
			break;

		case EVENT_SOCKET_CALLBACK:
			socket_set_callback_event(e.data);
			break;

		case EVENT_SOCKET_OPT:
			socket_set_option_event(e.data);
			break;

		default:
			break;
		}

		rtos_mutex_unlock(&net->mutex);
	}

	os_timer_destroy(&net->timer.handle);

	os_log(LOG_INIT, "networking(%p) rx task exited\n", net);

	rtos_thread_abort(NULL);
}

static void net_tx_qos_sched(bool *wake)
{
	struct event e;

	e.type = EVENT_QOS_SCHED;
	e.data = NULL;

	if (rtos_mqueue_send_from_isr(&net_tx_ctx.queue, &e, RTOS_NO_WAIT, wake) < 0) {
		os_log(LOG_ERR, "rtos_mqueue_send_from_isr() failed\n");
		return;
	}
}

static void net_rx_timer(bool *wake)
{
	struct event e;

	e.type = EVENT_RX_TIMER;
	e.data = NULL;

	rtos_mqueue_send_from_isr(&net_rx_ctx.queue, &e, RTOS_NO_WAIT, wake);
}

static void net_timer(struct os_timer *t, int count)
{
	bool wake = false;

	net_tx_qos_sched(&wake);

	net_rx_timer(&wake);

	mclock_interrupt(count, &wake);

	rtos_yield_from_isr(wake);
}

__init static int net_timer_create(struct net_rx_ctx *net)
{
	net->timer.period = NET_RX_TX_PERIOD;

	if (!os_timer_create(&net->timer.handle, OS_CLOCK_SYSTEM_MONOTONIC_1, OS_TIMER_FLAGS_RECOVERY, net_timer, 0))
		goto out;

	if (!os_timer_create(&net->timer.handle, OS_CLOCK_SYSTEM_MONOTONIC, OS_TIMER_FLAGS_RECOVERY, net_timer, 0))
		goto out;

	if (os_timer_create(&net->timer.handle, OS_CLOCK_SYSTEM_MONOTONIC, 0, net_timer, 0) < 0) {
		os_log(LOG_ERR, "os_timer_create() failed\n");
		goto err;
	}

out:
	return 0;

err:
	return -1;
}

__init int net_task_init(void)
{
	int i;
	struct ptp_ts_ctx *ptp_ctx;

	memset(&net_tx_ctx, 0, sizeof(net_tx_ctx));
	memset(&net_rx_ctx, 0, sizeof(net_rx_ctx));

	net_qos_init(&net_tx_ctx.net_qos);

	if (rtos_mutex_init(&net_rx_ctx.mutex) < 0) {
		os_log(LOG_ERR, "rtos_mutex_init() failed\n");
		goto err_net_rx_mutex;
	}

	if (rtos_mqueue_init(&net_rx_ctx.queue, NET_RX_EVENT_QUEUE_LENGTH, sizeof(struct event),
						net_rx_ctx.queue_buffer) < 0) {
		os_log(LOG_ERR, "rtos_mqueue_init(net rx) failed\n");

		goto err_net_rx_queue;
	}

	for (i = 0; i < CFG_PORTS; i++) {
		ptp_ctx = &net_tx_ctx.ptp_ctx[i];

		if (rtos_mqueue_init(&ptp_ctx->tx_ts_queue, PTP_TX_TS_QUEUE_LENGTH, sizeof(struct socket_tx_ts_data),
								ptp_ctx->tx_ts_queue_buffer) < 0) {
			os_log(LOG_ERR, "rtos_mqueue_init(tx ts) failed\n");

			goto err_net_tx_ts_queue;
		}
	}

	if (rtos_mqueue_init(&net_tx_ctx.queue, NET_TX_EVENT_QUEUE_LENGTH, sizeof(struct event),
						net_tx_ctx.queue_buffer) < 0) {
		os_log(LOG_ERR, "rtos_mqueue_init(net tx) failed\n");

		goto err_net_tx_queue;
	}

	if (net_timer_create(&net_rx_ctx) < 0)
		goto err_net_timer;

	if (rtos_thread_create(&net_tx_ctx.task, NET_TX_PRIORITY, 0, NET_TX_STACK_DEPTH, NET_TX_TASK_NAME, net_tx_task, &net_tx_ctx) < 0) {
		os_log(LOG_ERR, "rtos_thread_create(%s) failed\n", NET_TX_TASK_NAME);
		goto err_net_tx;
	}

	if (rtos_thread_create(&net_rx_ctx.task, NET_RX_PRIORITY, 0, NET_RX_STACK_DEPTH, NET_RX_TASK_NAME, net_rx_task, &net_rx_ctx) < 0) {
		os_log(LOG_ERR, "rtos_thread_create(%s) failed\n", NET_RX_TASK_NAME);
		goto err_net_rx;
	}

	os_log(LOG_INIT, "networking started\n");

	return 0;

err_net_rx:
	rtos_thread_abort(&net_tx_ctx.task);

err_net_tx:
	os_timer_destroy(&net_rx_ctx.timer.handle);

err_net_timer:
err_net_tx_queue:
err_net_tx_ts_queue:
err_net_rx_queue:
err_net_rx_mutex:

	return -1;
}

__exit void net_task_exit(void)
{
	rtos_thread_abort(&net_rx_ctx.task);

	rtos_thread_abort(&net_tx_ctx.task);

	os_timer_destroy(&net_rx_ctx.timer.handle);

	os_log(LOG_INIT, "networking stopped\n");
}
