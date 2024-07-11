/*
 * Copyright 2017-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief PTP management functions
 @details
*/

#include "ptp.h"
#include "net_bridge.h"
#include "net_port.h"
#include "net_tx.h"
#include "net_logical_port.h"
#include "clock.h"

#include "os/net.h"

#include "genavb/ether.h"

static struct generic_rx_hdlr ptp_rx_hdlr[CFG_LOGICAL_NUM_PORT];

static int ptp_tx(struct net_socket *sock, struct logical_port *port, unsigned long *addr, unsigned int *n);

void ptp_socket_unbind(struct net_socket *sock)
{
	generic_unbind(ptp_rx_hdlr, sock, CFG_LOGICAL_NUM_PORT);
}

int ptp_socket_bind(struct net_socket *sock, struct net_address *addr)
{
	if (addr->u.ptp.version != 2)
		return -1;

	return generic_bind(ptp_rx_hdlr, sock, addr->port, CFG_LOGICAL_NUM_PORT);
}

void ptp_socket_disconnect(struct logical_port *port, struct net_socket *sock)
{
	struct net_port *net_port = port->phys;

	if (sock->flags & SOCKET_FLAGS_TX_TS_ENABLED) {
		struct socket_tx_ts_data ts_data;

		while (rtos_mqueue_receive_from_isr(sock->tx.ts_queue_handle, &ts_data, RTOS_NO_WAIT, NULL) == 0)
			;

		sock->flags &= ~SOCKET_FLAGS_TX_TS_ENABLED;
		port->ptp_sock = NULL;
	}

	sock->tx.func = NULL;
	sock->tx.ts_queue_handle = NULL;

	if (!port->is_bridge)
		qos_queue_disconnect(net_port->qos, sock->tx.qos_queue);
}


int ptp_socket_connect(struct logical_port *port, struct net_socket *sock, struct net_address *addr)
{
	struct net_port *net_port = port->phys;

	if (port->ptp_sock)
		goto err;

	sock->tx.func = ptp_tx;
	sock->tx.ts_queue_handle = &net_tx_ctx.ptp_ctx[port->phys->index].tx_ts_queue;

	if (!port->is_bridge) {
		sock->tx.qos_queue = qos_queue_connect(net_port->qos, addr->priority, &sock->tx.queue, 0);
		if (!sock->tx.qos_queue)
			goto err;
	}

	sock->flags |= SOCKET_FLAGS_TX_TS_ENABLED;
	port->ptp_sock = sock;

	return 0;

err:
	return -1;
}

void ptp_tx_ts_wakeup(struct net_rx_ctx *net)
{

}

void ptp_tx_ts(struct net_port *phys_port, u64 ts, u32 priv)
{
	struct logical_port *port = physical_to_logical_port(phys_port);
	struct socket_tx_ts_data ts_data;
	struct net_socket *sock = port->ptp_sock;
	uint64_t local_time = 0;
	struct event e;

	clock_time_from_hw(phys_port->clock_local, ts, &local_time);

	ts_data.priv = priv;
	ts_data.ts = local_time;
	rtos_mqueue_send_from_isr(port->ptp_sock->tx.ts_queue_handle, &ts_data, RTOS_NO_WAIT, NULL);

	if (sock->callback) {
		if (rtos_atomic_test_and_clear_bit(SOCKET_ATOMIC_FLAGS_CALLBACK_ENABLED, &sock->atomic_flags))
			sock->callback(sock->callback_data);
	} else {
		e.type = EVENT_TYPE_NET_TX_TS;
		e.data = port->ptp_sock->handle_data;
		rtos_mqueue_send(port->ptp_sock->handle, &e, RTOS_NO_WAIT);
	}
}

static int ptp_tx(struct net_socket *sock, struct logical_port *port, unsigned long *addr, unsigned int *n)
{
	struct net_tx_desc *desc;
	int rc = 0, i;

	for (i = 0; i < *n; i++) {
		desc = (struct net_tx_desc *)addr[i];

		if (!port->is_bridge) {
			if ((rc = socket_qos_enqueue(sock, port, sock->tx.qos_queue, desc)) < 0)
				break;
		} else {
			if ((rc = bridge_port_tx(port->phys, sock->addr.priority, desc)) < 0)
				break;
		}
	}

	*n = i;

	return rc;
}

int ptp_rx(struct net_rx_ctx *net, struct logical_port *port, struct net_rx_desc *desc, void *hdr)
{
	struct ptp_hdr *ptp = hdr;
	struct net_rx_stats *stats = &net->ptype_hdlr[PTYPE_PTP].stats[desc->port];
	struct net_socket *sock;
	uint64_t local_time = 0;
	int rc;

	if (!port)
		goto slow;

	if (ptp->version_ptp != 0x2)
		goto slow;

	if ((ptp->transport_specific != 0x1) && (ptp->transport_specific != 0x2))
		goto slow;

	sock = ptp_rx_hdlr[port->id].sock;
	if (!sock)
		goto slow;

	clock_time_from_hw(port->phys->clock_local, desc->ts64, &local_time);
	desc->ts = local_time;
	desc->ts64 = local_time;

	rc = socket_rx(net, sock, desc, stats);

	return rc;

slow:
	return net_rx_slow(net, port, desc, stats);
}
