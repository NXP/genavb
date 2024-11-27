/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2020, 2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _NET_TX_H_
#define _NET_TX_H_

#include "linux/types.h"
#include "genavb/sr_class.h"

#include "rational.h"

struct net_sr_class_cfg {
	u16 port;
	u8 sr_class[CFG_SR_CLASS_MAX];
};

struct net_sr_config {
	u16 port;
	u16 vlan_id;	/* Host Endianess */
	u8 priority;
	unsigned int idle_slope;	/* bits/s */
	u8 stream_id[8];
};

#ifdef __KERNEL__

#include <linux/types.h>
#include <linux/dcache.h>

#include "pi.h"
#include "queue.h"
#include "port_config.h"
#include "genavb/sr_class.h"
#include "genavb/config.h"


//#define PORT_TRACE	1
#define PORT_TRACE_SIZE	128

#define PORT_RATE_bps		100000000 /* 100 Mbps */

#define FCS_LEN		4
#define IFG_LEN		12
#define PREAMBLE_LEN	8
#define PORT_OVERHEAD	(IFG_LEN + PREAMBLE_LEN + FCS_LEN)

#if 1
#define print_debug(...)	;
#else
#define print_debug(...)	pr_info(__VA_ARGS__)
#endif

struct jitter_stats {
	unsigned int ptp_last;

	unsigned int count;

	unsigned int dt_min;
	unsigned int dt_max;
	unsigned long long dt_mean_cur;
	unsigned long long dt_mean2_cur;

	unsigned int dt_mean;
	unsigned int dt_mean2;
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

	u16 vlan_label;		/* In Big Endian */
	u8 id[8];
	unsigned int flags;

#ifdef PORT_TRACE
	unsigned int burst;
	unsigned int burst_max;
#endif
};

#define SR_FLAGS_HW_CBS	(1 << 0)

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

	unsigned long int pending_mask;

	struct stream_queue stream[CFG_SR_CLASS_STREAM_MAX];
};


struct traffic_class {
	struct sr_class *sr_class;		/* Set only if the traffic class is an SR class */

	unsigned int index;
	unsigned int hw_queue_id;
	unsigned int flags;

	struct qos_queue qos_queue[CFG_TRAFFIC_CLASS_QUEUE_MAX];	/* array of queues assigned to this traffic class */

	unsigned long scheduled_mask;		/* bit mask of queues that can transmit packets */
	unsigned long slast;

	unsigned int tx;

	/* Shared with enqueue code */
	unsigned long shared_pending_mask;	/* bit mask of queues with pending packets */
};

#ifdef PORT_TRACE

struct port_trace {
	unsigned int tnow;
	unsigned int ptp;
	unsigned int ts;
	unsigned int class;
	unsigned int queue;
	int port_credit;
	int class_credit;
	int queue_credit;
	unsigned int scheduled_mask;
	unsigned int pending_mask;
	unsigned int ready;
	unsigned int pending;
};

#endif /* PORT_TRACE */

struct port_qos {
	struct net_qos *net;

	unsigned int tnow;		/* in nanoseconds */
	unsigned int interval;		/* port scheduling interval (in nanoseconds) */
	unsigned int interval_n;	/* port interval count */
	unsigned int transmit_event;

	void *fec_data;

	struct eth_avb *eth;

	struct shaper shaper;

	unsigned int streams;
	unsigned int used_rate;		/* bits/s */
	unsigned int max_rate;		/* bits/s */

	unsigned int tx;
	unsigned int tx_full;

	struct sr_class sr_class[CFG_SR_CLASS_MAX];
	struct traffic_class traffic_class[CFG_TRAFFIC_CLASS_MAX];

	struct ptp_grid ptp_grid;
	struct jitter_stats jitter_stats;

#ifdef PORT_TRACE
	struct port_trace trace[PORT_TRACE_SIZE];
	unsigned int trace_w;
	unsigned int trace_freeze;
#endif
};

struct net_qos {
	struct port_qos port[CFG_PORTS];
};

struct avb_drv;

struct qos_queue *qos_queue_connect(struct port_qos *port, u8 priority, struct queue *q, unsigned int is_sr);
void qos_queue_disconnect(struct port_qos *port, struct qos_queue *qos_q);

struct qos_queue *net_qos_stream_connect(struct port_qos *port, u8 class, u8 *stream_id, struct queue *queue);
void net_qos_stream_disconnect(struct port_qos *port, struct qos_queue *qos_q);
int net_qos_sr_config(struct net_qos *net, struct net_sr_config *sr_config);
void net_qos_port_flush(struct port_qos *port);
void net_qos_port_reset(struct port_qos *port, int port_rate_bps);
int net_qos_map_traffic_class_to_hw_queues(struct port_qos *port);

int net_qos_init(struct net_qos *net, struct dentry *avb_dentry);
int net_qos_sr_class_configure(struct avb_drv *avb, struct net_sr_class_cfg *net_sr_class_cfg);
void net_qos_exit(struct net_qos *net);
unsigned int port_scheduler(struct port_qos *port, unsigned int ptp_now);

void port_jitter_stats_init(struct jitter_stats *s);
void port_jitter_stats(struct jitter_stats *s, unsigned int ptp_now);

#endif /* __KERNEL__ */

#endif /* _NET_TX_H_ */
