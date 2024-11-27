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

#ifndef _RTOS_NET_TX_H_
#define _RTOS_NET_TX_H_

#include "rtos_abstraction_layer.h"

#include "genavb/sr_class.h"
#include "genavb/config.h"
#include "genavb/ether.h"

#include "os/sys_types.h"

#include "net_port.h"
#include "rational.h"
#include "pi.h"
#include "avb_queue.h"

#define FCS_LEN		4
#define IFG_LEN		12
#define PREAMBLE_LEN	8
#define PORT_OVERHEAD	(IFG_LEN + PREAMBLE_LEN + FCS_LEN)

#define NET_TX_EVENT_QUEUE_LENGTH	16
#define PTP_TX_TS_QUEUE_LENGTH		16

struct socket_tx_ts_data {
	uint32_t priv;
	uint64_t ts;
};

struct ptp_ts_ctx {
	rtos_mqueue_t tx_ts_queue; /* Queue containing transmit timestamps */
	uint8_t tx_ts_queue_buffer[PTP_TX_TS_QUEUE_LENGTH * sizeof(struct socket_tx_ts_data)];
};

struct net_sr_config {
	uint16_t port;
	uint16_t vlan_id;	/* Host Endianess */
	uint8_t priority;
	unsigned int idle_slope;	/* bits/s */
	uint8_t stream_id[8];
};

struct ptp_grid {
	unsigned int now;
	unsigned int last;

	unsigned int ptp_last;

	unsigned int count;
	unsigned long long total;

	unsigned int period;
	unsigned int period_frac;
	unsigned int period_frac_cur;
	int d;

	unsigned int reset;
	struct pi pi;

#ifdef PORT_TRACE
	unsigned int trace_count;
	struct ptp_trace {
		unsigned int ptp;
		unsigned int now;
		unsigned int period;
		unsigned int period_frac;
		long long integral;
		unsigned long long total;
		int err;
	} trace[PORT_TRACE_SIZE];
#endif
};

struct shaper {
	int credit;		/* bits x scale */
	int credit_min;		/* bits x scale */
	unsigned int tlast;	/* in interval units */
	unsigned int rate;	/* bits/interval */
};


#define QOS_QUEUE_FLAG_CONNECTED	(1 << 0)
#define QOS_QUEUE_FLAG_ENABLED		(1 << 1)

struct qos_queue {
	struct traffic_class *tc;

	struct stream_queue *stream;

	unsigned int index;

	struct queue *queue;

	unsigned int flags;
	unsigned long atomic_flags;

	unsigned int tx;
	unsigned int dropped;
	unsigned int full;
	unsigned int disabled;
};

#define STREAM_FLAGS_CONNECTED	(1 << 0)
#define STREAM_FLAGS_CONFIGURED	(1 << 1)
#define STREAM_FLAGS_USED	(STREAM_FLAGS_CONNECTED | STREAM_FLAGS_CONFIGURED)

struct stream_queue {
	struct shaper shaper;
	unsigned int idle_slope;		/* bits/s */
	struct sr_class *sr_class;
	struct qos_queue *qos_queue;

	uint16_t vlan_label;		/* In Big Endian */
	uint8_t id[8];
	unsigned int flags;

#ifdef PORT_TRACE
	unsigned int burst;
	unsigned int burst_max;
#endif
};

#define TC_FLAGS_HW_CBS	(1 << 0) /* HW CBS queue */
#define TC_FLAGS_HW_SP	(1 << 1) /* dedicated HW queue */

struct sr_class {
	/* Private to tx context */
	struct shaper shaper;

	unsigned int flags;

	sr_class_t class;
	struct traffic_class *tc;
	unsigned int stream_max;
	unsigned int streams;

	unsigned int idle_slope;		/* bits/s */

	struct rational interval;		/* class scheduling interval (in nanoseconds) */
	struct rational interval_ratio;		/* class to port scheduling interval ratio */
	struct rational tnext;			/* class next scheduling interval (in nanoseconds) */
	unsigned int tnext_gptp;
	unsigned int sched_offset;
	unsigned int interval_n;		/* class interval count */
	unsigned int scale;			/* class subintervals, for software scheduling */

	rtos_atomic_t pending_mask;

	struct stream_queue stream[CFG_SR_CLASS_STREAM_MAX];
};


struct traffic_class {
	struct sr_class *sr_class;		/* Set only if the traffic class is an SR class */

	unsigned int index;
	unsigned int hw_queue_id;
	unsigned int flags;

	struct qos_queue qos_queue[CFG_TRAFFIC_CLASS_QUEUE_MAX];	/* array of queues assigned to this traffic class */

	rtos_atomic_t scheduled_mask;		/* bit mask of queues that can transmit packets */
	rtos_atomic_t disabled_mask;
	unsigned long slast;

	unsigned int tx;

	/* Shared with enqueue code */
	rtos_atomic_t shared_pending_mask;	/* bit mask of queues with pending packets */
};

struct port_qos {
	struct net_qos *net;

	unsigned int tnow;		/* in nanoseconds */
	unsigned int interval;		/* port scheduling interval (in nanoseconds) */
	unsigned int interval_n;	/* port interval count */
	unsigned int transmit_event;

	struct net_port *net_port;

	struct shaper shaper;

	unsigned int streams;
	unsigned int used_rate;		/* bits/s */
	unsigned int max_rate;		/* bits/s */

	unsigned int tx;
	unsigned int tx_drop;

	unsigned int traffic_class_max;
	struct sr_class sr_class[CFG_SR_CLASS_MAX];
	struct traffic_class traffic_class[CFG_TRAFFIC_CLASS_MAX];

	struct ptp_grid ptp_grid;
#if 0
	struct jitter_stats jitter_stats;
#endif

#ifdef PORT_TRACE
	struct port_trace trace[PORT_TRACE_SIZE];
	unsigned int trace_w;
	unsigned int trace_freeze;
#endif
};

struct net_qos {
	struct port_qos port[CFG_ENDPOINT_NUM];
};

struct net_tx_ctx {
	rtos_thread_t task;

	rtos_mqueue_t queue;
	uint8_t queue_buffer[NET_TX_EVENT_QUEUE_LENGTH * sizeof(struct event)];

	struct net_qos net_qos;

	struct ptp_ts_ctx ptp_ctx[CFG_PORTS];
};

extern struct net_tx_ctx net_tx_ctx;

static inline void traffic_class_update_stats(struct traffic_class *tc, struct qos_queue *qos_q, int tx_success)
{
	if (tx_success) {
		qos_q->tx++;
		tc->tx++;
	} else {
		qos_q->dropped++;
	}
}

struct qos_queue *qos_queue_connect(struct port_qos *port, uint8_t priority, struct queue *q, unsigned int is_sr);
void qos_queue_disconnect(struct port_qos *port, struct qos_queue *qos_q);

struct net_socket;
struct qos_queue *net_qos_stream_connect(struct port_qos *port, uint8_t class, uint8_t *stream_id, struct queue *queue);
void net_qos_stream_disconnect(struct port_qos *port, struct qos_queue *qos_q);
void net_qos_sr_config_event(struct net_socket *sock);
int net_qos_sr_config(struct net_qos *net, struct net_sr_config *sr_config);

int net_qos_init(struct net_qos *net);
void port_scheduler_event(void *data);

void port_qos_up(struct net_port *port);
void port_qos_down(struct net_port *port);

#endif /* _RTOS_NET_TX_H_ */
