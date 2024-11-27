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

#include "common/log.h"
#include "genavb/avtp.h"
#include "genavb/qos.h"
#include "os/clock.h"
#include "net_tx.h"
#include "ptp.h"
#include "net_socket.h"

#define PORT_RATE_BPS	100000000 // FIXME move to freertos_avb/../common/system_config ??

struct net_tx_ctx net_tx_ctx;

#define SCALING_FACTOR	1024	/* Used to get sub nanosecond precision in the calculated period */
#define DEFAULT_ki	3
#define DEFAULT_kp	1

#define PTP_MAX_ERROR_NS	50000 /* based on expected measurement jitter */
#define PI_MAX_ERROR_NS		1000 /* based on clock accuracy of 100ppm */

#define BITS_PER_BYTE	8
#ifndef BITS_PER_LONG
#define BITS_PER_LONG (sizeof(long) * BITS_PER_BYTE)
#endif /* !BITS_PER_LONG */

static int priority_to_tclass(struct port_qos *port, uint8_t priority)
{
	if (priority >= QOS_PRIORITY_MAX)
		goto err;

	return port_priority_to_traffic_class(port->net_port, priority);

err:
	return -1;
}

static void port_ptp_grid_init(struct ptp_grid *g)
{
	g->reset = 0;
	g->count = 0;
	g->total = 0;

	g->period = NET_RX_TX_PERIOD;
	g->period_frac = 0;
	g->period_frac_cur = 0;

	pi_init(&g->pi, DEFAULT_ki, DEFAULT_kp);
	pi_reset(&g->pi, g->period * SCALING_FACTOR);

#ifdef PORT_TRACE
	g->trace_count = 0;
#endif
}

static void port_ptp_grid_reset(struct ptp_grid *g)
{
	g->count = 0;
	g->total = 0;
	g->period = NET_RX_TX_PERIOD;
	g->period_frac = 0;
	g->period_frac_cur = 0;

	pi_reset(&g->pi, g->period * SCALING_FACTOR);

	g->reset++;
}

static void port_ptp_grid_update(struct ptp_grid *g, unsigned int ptp_now)
{
	unsigned int period = ptp_now - g->ptp_last;

	if ((period > (NET_RX_TX_PERIOD + PTP_MAX_ERROR_NS)) || (period < (NET_RX_TX_PERIOD - PTP_MAX_ERROR_NS))) {
		/* Error in measured ptp time */
		port_ptp_grid_reset(g);

		g->now = ptp_now;
	} else if ((g->period > (NET_RX_TX_PERIOD + PI_MAX_ERROR_NS)) || (g->period < (NET_RX_TX_PERIOD - PI_MAX_ERROR_NS))) {
		/* Error in PI controller */
		port_ptp_grid_reset(g);

		g->now = ptp_now;
	} else {

		g->now = g->last + g->period;

		/* Apply fractional period, uniformly over the scaling interval.
		 * Check Bresenham's line algorithm for details (with dx = SCALING_FACTOR, dy = g->period_frac and SCALING_FACTOR even) */
		if (g->period_frac_cur) {
			g->d += g->period_frac;

			if (g->d > 0) {
				g->now++;
				g->d -= SCALING_FACTOR;
				g->period_frac_cur--;
			}
		}

		g->count++;

		/* Apply a filter to the sampling to reduce jitter.
		   Measurement jitter is of the order of tens of microseconds, this filter should bring it
		down to tens of nanoseconds */

		if (g->count >= SCALING_FACTOR) {
			int period;

			if (g->period_frac_cur)
				g->now += g->period_frac_cur;

			period = pi_update(&g->pi, ptp_now - g->now);

			g->count = 0;
			g->period = period / SCALING_FACTOR;
			g->period_frac = period - g->period * SCALING_FACTOR;
			g->d = g->period_frac - SCALING_FACTOR / 2;
			g->period_frac_cur = g->period_frac;

#ifdef PORT_TRACE
			if (g->trace_count < PORT_TRACE_SIZE) {
				g->trace[g->trace_count].now = g->now;
				g->trace[g->trace_count].ptp = ptp_now;
				g->trace[g->trace_count].period = g->period;
				g->trace[g->trace_count].period_frac = g->period_frac;
				g->trace[g->trace_count].integral = g->pi.integral;
				g->trace[g->trace_count].err = g->pi.err;
				g->trace[g->trace_count].total = g->total;
				g->trace_count++;
			}
#endif
		}
	}

	g->ptp_last = ptp_now;
	g->last = g->now;

	g->total++;
}

#if 0
void port_jitter_stats_init(struct jitter_stats *s) {}
void port_jitter_stats(struct jitter_stats *s, unsigned int ptp_now) {}
#endif

/* Return number of leading zeros in a BITS_PER_LONG-bit word */
static inline unsigned long leading_zeros(unsigned long x)
{
	unsigned long ret;

	__asm("clz\t%0, %1" : "=r" (ret) : "r" (x));

	return ret;
}

static inline void incr_credit(int *credit, unsigned int dt, unsigned int rate)
{
	/* Given a maximum rate of ~15625 bytes/125us, this guarantees the credit never overflows */
	if ((dt > 0x10000) || (*credit >= 0x40000000))
		*credit = 0x40000000;
	else
		*credit += dt * rate;
}

static void stream_incr_credit(struct stream_queue *stream, unsigned int tnow)
{
	incr_credit(&stream->shaper.credit, tnow - stream->shaper.tlast, stream->shaper.rate);

	stream->shaper.tlast = tnow;
}

static inline struct qos_queue *round_robin_scheduler(struct traffic_class *tc)
{
	unsigned long smask = tc->scheduled_mask;
	unsigned long slast = tc->slast;
	unsigned long i;

	if (likely(slast)) {
		smask <<= BITS_PER_LONG - slast;
		i = leading_zeros(smask);
		if (i < BITS_PER_LONG) {
			i = slast - i - 1;
			goto found;
		}
	}

	smask = tc->scheduled_mask >> slast;
	i = leading_zeros(smask);
	if (i < BITS_PER_LONG) {
		i = slast + (BITS_PER_LONG - i - 1);
		goto found;
	}

	return NULL;

found:
	tc->slast = i;

	return &tc->qos_queue[i];
}

/* Port credit acccounting */
static void port_incr_credit(struct port_qos *port, unsigned int tnow)
{
	if (port->shaper.credit < 0) {
		incr_credit(&port->shaper.credit, tnow - port->shaper.tlast, port->shaper.rate);
		if (port->shaper.credit > 0)
			port->shaper.credit = 0;
	}

	port->shaper.tlast = tnow;
}

static void port_dec_credit(struct port_qos *port, unsigned int len)
{
	if (unlikely(len < (ETHER_MIN_FRAME_SIZE - FCS_LEN)))
		len = ETHER_MIN_FRAME_SIZE - FCS_LEN;

	port->shaper.credit -= (len + PORT_OVERHEAD) * BITS_PER_BYTE; /* bits */
	port->tx++;
}

static void shaper_init(struct shaper *s, unsigned int rate)
{
	s->credit = 0;
	s->tlast = 0;
	s->rate = rate;
	s->credit_min = -rate;
}

static void shaper_add(struct shaper *s, int rate)
{
	s->rate += rate;
	s->credit_min -= rate;
}

static void shaper_set(struct shaper *s, unsigned int rate)
{
	s->rate = rate;
	s->credit_min = -rate;
}

static inline int shaper_ready(struct shaper *s)
{
	return (s->credit >= s->credit_min);
}

static unsigned int sr_class_scale_idle_slope(struct sr_class *sr_class, unsigned int idle_slope)
{
	return ((uint64_t)idle_slope * sr_class_interval_p(sr_class->class)) / ((uint64_t)NSECS_PER_SEC * sr_class_interval_q(sr_class->class));
}

static inline void sr_class_incr_credit(struct sr_class *class, unsigned int tnow)
{
	incr_credit(&class->shaper.credit, tnow - class->shaper.tlast, class->shaper.rate);

	class->shaper.tlast = tnow;
}

static inline unsigned int queue_tx_ready(struct sr_class *class, struct stream_queue *stream)
{
	struct queue *queue = stream->qos_queue->queue;
	struct net_tx_desc *desc;

	if (unlikely(!queue_pending(queue)))
		return 0;

	desc = (struct net_tx_desc *)queue_peek(queue);
	if (likely(desc->flags & NET_TX_FLAGS_TS)) {
		if (avtp_before(desc->ts, class->tnext_gptp))
			return 1;
		else
			return 0;
	}

	return 1;
}

static void sr_class_dec_credit(struct traffic_class *tc, struct qos_queue *qos_q, unsigned int len)
{
	struct sr_class *class = tc->sr_class;
	struct stream_queue *stream = qos_q->stream;
	struct queue *queue = qos_q->queue;

	if (unlikely(len < (ETHER_MIN_FRAME_SIZE - FCS_LEN)))
		len = ETHER_MIN_FRAME_SIZE - FCS_LEN;

	stream->shaper.credit -= (len + PORT_OVERHEAD) * class->scale * BITS_PER_BYTE;
	class->shaper.credit -= (len + PORT_OVERHEAD) * class->scale * BITS_PER_BYTE;

	qos_q->tx++;
	tc->tx++;

	/* Modifying shared_pending_mask races with queueing code and it may leave the bit clear with packets pending */
	/* To work around this race we clear the bit first and then _re-check_ for pending packets. If any are pending
	 * we set the bit again */
	rtos_atomic_clear_bit(qos_q->index, &tc->shared_pending_mask);
	if (!queue_tx_ready(class, stream)) {
		if (queue_pending(queue))
			rtos_atomic_set_bit(qos_q->index, &tc->shared_pending_mask);

		class->pending_mask &= ~(1UL << qos_q->index);
		tc->scheduled_mask &= ~(1UL << qos_q->index);
	} else {
		rtos_atomic_set_bit(qos_q->index, &tc->shared_pending_mask);

		if (!shaper_ready(&stream->shaper))
			tc->scheduled_mask &= ~(1UL << qos_q->index);
	}
}

static int sr_class_tx(struct port_qos *port, struct traffic_class *tc, struct qos_queue *qos_q)
{
	struct queue *queue = qos_q->queue;
	struct net_tx_desc *desc;
	unsigned int len;
	u32 read;
	int rc;

	queue_dequeue_init(queue, &read);

	desc = (void *)queue_dequeue_next(queue, &read);
	len = desc->len;

	rc = port_tx(port->net_port, desc, tc->hw_queue_id);

	if (rc < 0) {
		queue_dequeue_done(queue, read);
		net_tx_free(desc);
		port->tx_drop++;

		/* ring buffer is full, exit */
		goto out;
	}

	queue_dequeue_done(queue, read);
	port_dec_credit(port, len);
	sr_class_dec_credit(tc, qos_q, len);

out:
	return rc;
}

static void qos_queue_flush(struct qos_queue *qos_q)
{
	struct traffic_class *tc = qos_q->tc;

	if (!(qos_q->flags & QOS_QUEUE_FLAG_CONNECTED))
		return;

	rtos_atomic_clear_bit(qos_q->index, &tc->shared_pending_mask);
	rtos_atomic_clear_bit(qos_q->index, &tc->scheduled_mask);

	queue_flush(qos_q->queue, NULL);
}

static void inline qos_queue_flush_disabled(struct traffic_class *tc)
{
	struct qos_queue *qos_q;
	unsigned long mask;
	int i;

	mask = tc->shared_pending_mask & tc->disabled_mask;

	while ((i = leading_zeros(mask)) < BITS_PER_LONG) {
		qos_q = &tc->qos_queue[BITS_PER_LONG - 1 - i];
		mask &= ~(1UL << (BITS_PER_LONG - 1 - i));

		qos_queue_flush(qos_q);

		qos_q->dropped++;
		qos_q->disabled++;
	}
}

static void inline sr_class_update(struct traffic_class *tc, unsigned int tnow)
{
	struct sr_class *class = tc->sr_class;
	struct qos_queue *qos_q;
	struct stream_queue *stream;
	unsigned long mask;
	int i;

	/* Update the status of all pending SR streams. We are interested in:
	 * - skipping streams that are no longer connected
	 * - correctly account for stream idle time when updating it's credit
	 * - correctly account for class idle time when updating it's credit
	 */

	qos_queue_flush_disabled(tc);

	if (tc->scheduled_mask) {
		/* Class was never idle */

		/* Update all new pending streams */
		mask = tc->shared_pending_mask & (~class->pending_mask);

		/* loop over all streams with corresponding bit set in mask */
		while ((i = leading_zeros(mask)) < BITS_PER_LONG) {
			qos_q = &tc->qos_queue[BITS_PER_LONG - 1 - i];
			stream = qos_q->stream;
			mask &= ~(1UL << (BITS_PER_LONG - 1 - i));

			if (queue_tx_ready(class, stream)) {
				class->pending_mask |= (1UL << qos_q->index);

				stream_incr_credit(stream, tnow);

				if (stream->shaper.credit > 0)
					stream->shaper.credit = 0;

				if (shaper_ready(&stream->shaper))
					tc->scheduled_mask |= (1UL << qos_q->index);
			}
		}

		/* Update all streams already pending, but not scheduled yet */
		mask = class->pending_mask & (~tc->scheduled_mask);

		/* loop over all streams with corresponding bit set in mask */
		while ((i = leading_zeros(mask)) < BITS_PER_LONG) {
			qos_q = &tc->qos_queue[BITS_PER_LONG - 1 - i];
			stream = qos_q->stream;
			mask &= ~(1UL << (BITS_PER_LONG - 1 - i));

			stream_incr_credit(stream, tnow);

			if (shaper_ready(&stream->shaper))
				tc->scheduled_mask |= (1UL << qos_q->index);
		}

		sr_class_incr_credit(class, tnow);

	} else {
		/* Complex case, the class was idle for a while
		 * need to determine when the first stream became active */

		/* Update all new pending streams */
		mask = tc->shared_pending_mask & (~class->pending_mask);

		/* loop over all streams with corresponding bit set in mask */
		while ((i = leading_zeros(mask)) < BITS_PER_LONG) {
			qos_q = &tc->qos_queue[BITS_PER_LONG - 1 - i];
			stream = qos_q->stream;
			mask &= ~(1UL << (BITS_PER_LONG - 1 - i));

			if (queue_tx_ready(class, stream)) {
				class->pending_mask |= (1UL << qos_q->index);

				stream_incr_credit(stream, tnow);

				if (stream->shaper.credit > 0)
					stream->shaper.credit = 0;

				if (shaper_ready(&stream->shaper))
					tc->scheduled_mask |= (1UL << qos_q->index);
			}
		}

		/* Update all streams already pending, but not scheduled yet */
		mask = class->pending_mask & (~tc->scheduled_mask);

		/* loop over all streams with corresponding bit set in mask */
		while ((i = leading_zeros(mask)) < BITS_PER_LONG) {
			qos_q = &tc->qos_queue[BITS_PER_LONG - 1 - i];
			stream = qos_q->stream;
			mask &= ~(1UL << (BITS_PER_LONG - 1 - i));

			stream_incr_credit(stream, tnow);

			if (shaper_ready(&stream->shaper))
				tc->scheduled_mask |= (1UL << qos_q->index);
		}

		/* Update class credit, if it's no longer idle */
		if (tc->scheduled_mask) {
			sr_class_incr_credit(class, tnow);

			if (class->shaper.credit > 0)
				class->shaper.credit = 0;
		}
	}
}

static int sr_class_scheduler(struct port_qos *port, struct traffic_class *tc, unsigned int tnow)
{
	struct sr_class *class = tc->sr_class;
	struct qos_queue *qos_q;
	int rc = 0;

	if (rational_int_cmp(tnow, &class->tnext) < 0)
		goto exit;

	class->sched_offset = (tnow - class->tnext.i);
	class->tnext_gptp = port->ptp_grid.now - class->sched_offset + rational_int_mul(port->ptp_grid.period, &class->interval_ratio);

	/* Credits are only incremented once per scheduling interval */
	sr_class_update(tc, class->interval_n);

	/* Transmit sr class traffic, highest priority first */
	while (shaper_ready(&port->shaper) && shaper_ready(&class->shaper) && tc->scheduled_mask) {

		qos_q = round_robin_scheduler(tc);
		if (!qos_q) {
			rc = -1;
			goto exit;
		}

		/* Stream credit hasn't been updated since the stream was scheduled, do it now */
		stream_incr_credit(qos_q->stream, class->interval_n);

		rc = sr_class_tx(port, tc, qos_q);

		if (rc < 0)
			break;

		if (!port->transmit_event) {
			if (/*rtos_atomic_test_bit(SOCKET_ATOMIC_FLAGS_SOCKET_WAITING_EVENT, &qos_q->atomic_flags) && */(queue_available(qos_q->queue) >= (qos_q->queue->size >> 2)))
				port->transmit_event = 1;
		}
	}

	rational_add(&class->tnext, &class->tnext, &class->interval);
	class->interval_n++;

exit:
	return rc;
}

static void traffic_class_update_queue(struct traffic_class *tc,
				       struct qos_queue *qos_q, int tx_success)
{
	struct queue *queue = qos_q->queue;

	traffic_class_update_stats(tc, qos_q, tx_success);

	/* Modifying shared_pending_mask races with queueing code and it may leave the bit clear with packets pending */
	/* To work around this race we clear the bit first and then _re-check_ for pending packets. If any are pending
	 * we set the bit again */
	rtos_atomic_clear_bit(qos_q->index, &tc->shared_pending_mask);
	if (queue_pending(queue))
		rtos_atomic_set_bit(qos_q->index, &tc->shared_pending_mask);
	else
		tc->scheduled_mask &= ~(1UL << qos_q->index);
}

static int traffic_class_tx(struct port_qos *port, struct traffic_class *tc, struct qos_queue *qos_q)
{
	struct queue *queue = qos_q->queue;
	struct net_tx_desc *desc;
	unsigned int len;
	u32 read;
	int rc;

	queue_dequeue_init(queue, &read);

	desc = (void *)queue_dequeue_next(queue, &read);
	len = desc->len;

	rc = port_tx(port->net_port, desc, tc->hw_queue_id);

	if (rc < 0) {
		queue_dequeue_done(queue, read);
		traffic_class_update_queue(tc, qos_q, 0);
		net_tx_free(desc);
		port->tx_drop++;

		/* ring buffer is full, exit */
		goto out;
	}

	queue_dequeue_done(queue, read);
	port_dec_credit(port, len);
	traffic_class_update_queue(tc, qos_q, 1);

out:
	return rc;
}

static void inline traffic_class_update(struct traffic_class *tc)
{
	struct qos_queue *qos_q;
	unsigned long mask;
	int i;

	/* Update all new pending queues */
	mask = tc->shared_pending_mask & (~tc->scheduled_mask);

	/* loop over all queues with corresponding bit set in mask */
	while ((i = leading_zeros(mask)) < BITS_PER_LONG) {
		qos_q = &tc->qos_queue[BITS_PER_LONG - 1 - i];
		mask &= ~(1UL << (BITS_PER_LONG - 1 - i));

		if (queue_pending(qos_q->queue)) {
			tc->scheduled_mask |= (1UL << qos_q->index);
		}
	}
}

static int traffic_class_scheduler(struct port_qos *port, struct traffic_class *tc, unsigned int tnow)
{
	struct qos_queue *qos_q;
	int rc = 0;

	traffic_class_update(tc);

	/* Transmit traffic class traffic in round robin */
	while (shaper_ready(&port->shaper) && tc->scheduled_mask) {

		qos_q = round_robin_scheduler(tc);
		if (!qos_q) {
			rc = -1;
			break;
		}

		rc = traffic_class_tx(port, tc, qos_q);

		if (rc < 0)
			break;
	}

	return rc;
}

static unsigned int port_scheduler(struct port_qos *port, unsigned int ptp_now)
{
	struct traffic_class *tc;
	unsigned int tnow = port->tnow;
	int i;

	port_ptp_grid_update(&port->ptp_grid, ptp_now);
#if 0
	port_jitter_stats(&port->jitter_stats, ptp_now);
#endif

	port_incr_credit(port, port->interval_n);

	port->transmit_event = 0;

	/* priority scheduler */
	for (i = port->traffic_class_max - 1; i >= 0; i--) {
		tc = &port->traffic_class[i];

		if (tc->sr_class)
			sr_class_scheduler(port, tc, tnow);
		else
			traffic_class_scheduler(port, tc, tnow);
	}

	port->tnow += port->interval;
	port->interval_n++;

	return port->transmit_event;
}

void port_scheduler_event(void *data)
{
	int i;
	struct net_port *port;
	uint32_t ptp_now;
	struct port_qos *qos;

	for (i = 0; i < CFG_PORTS; i++) {
		port = &ports[i];

		if (port->logical_port->is_bridge)
			continue;

		qos = port->qos;

		if (port->tx_up) {
			if (os_clock_gettime32(port->clock[PORT_CLOCK_GPTP_0], &ptp_now) < 0)
				ptp_now = 0;

			port_scheduler(qos, ptp_now);
		}
	}
}

static void qos_queue_reset(struct qos_queue *qos_q)
{
	qos_q->tx = 0;
	qos_q->dropped = 0;
	qos_q->full = 0;
	qos_q->disabled = 0;
}

static void qos_queue_enable(struct qos_queue *qos_q)
{
	qos_q->flags |= QOS_QUEUE_FLAG_ENABLED;
	rtos_atomic_clear_bit(qos_q->index, &qos_q->tc->disabled_mask);
}

static void qos_queue_disable(struct qos_queue *qos_q)
{
	qos_q->flags &= ~QOS_QUEUE_FLAG_ENABLED;
	rtos_atomic_set_bit(qos_q->index, &qos_q->tc->disabled_mask);
}

__init static void qos_queue_init(struct qos_queue *qos_q, struct traffic_class *tc, unsigned int index)
{
	qos_q->tc = tc;
	qos_q->index = index;

	qos_q->queue = NULL;
	qos_q->flags = 0;

	qos_queue_reset(qos_q);
}

struct qos_queue *qos_queue_connect(struct port_qos *port, uint8_t priority, struct queue *q, unsigned int is_sr)
{
	struct traffic_class *tc;
	struct qos_queue *qos_q;
	int i, tclass;

	tclass = priority_to_tclass(port, priority);
	if (tclass < 0)
		return NULL;

	tc = &port->traffic_class[tclass];

	if ((is_sr && (!tc->sr_class))
	|| (!is_sr && tc->sr_class))
		return NULL;

	for (i = 0; i < CFG_TRAFFIC_CLASS_QUEUE_MAX; i++) {
		qos_q = &tc->qos_queue[i];

		if (!(qos_q->flags & QOS_QUEUE_FLAG_CONNECTED))
			goto found;
	}

	return NULL;

found:
	qos_q->flags |= QOS_QUEUE_FLAG_CONNECTED;

	qos_queue_enable(qos_q);

	qos_q->queue = q;

	qos_queue_reset(qos_q);

	return qos_q;
}

void qos_queue_disconnect(struct port_qos *port, struct qos_queue *qos_q)
{
	qos_queue_flush(qos_q);

	qos_q->queue = NULL;

	qos_q->flags &= ~QOS_QUEUE_FLAG_CONNECTED;
}

static struct stream_queue *net_qos_stream_get(struct port_qos *port, uint8_t priority, uint8_t *stream_id)
{
	struct sr_class *sr_class;
	struct stream_queue *free_stream = NULL;
	struct stream_queue *match_stream;
	int i, tclass;

	tclass = priority_to_tclass(port, priority);
	if (tclass < 0)
		return NULL;

	sr_class = port->traffic_class[tclass].sr_class;
	if (!sr_class)
		return NULL;

	for (i = 0; i < sr_class->stream_max; i++) {
		struct stream_queue *stream = &sr_class->stream[i];

		if (stream->flags & STREAM_FLAGS_USED) {
			if (!memcmp(stream->id, stream_id, 8)) {
				match_stream = stream;
				goto out;
			}
		} else
			if (!free_stream)
				free_stream = stream;
	}

	if (free_stream)
		memcpy(free_stream->id, stream_id, 8);

	return free_stream;

out:
	return match_stream;
}

struct qos_queue *net_qos_stream_connect(struct port_qos *port, uint8_t class, uint8_t *stream_id, struct queue *queue)
{
	struct stream_queue *stream;
	uint8_t priority;

	if (!sr_class_enabled(class))
		return NULL;

	priority = sr_class_pcp(class);

	stream = net_qos_stream_get(port, priority, stream_id);
	if (!stream)
		return NULL;

	if (stream->flags & STREAM_FLAGS_CONNECTED)
		return NULL;

	stream->qos_queue = qos_queue_connect(port, priority, queue, 1);
	if (!stream->qos_queue)
		return NULL;

	if (!(stream->flags & STREAM_FLAGS_CONFIGURED))
		qos_queue_disable(stream->qos_queue);

	stream->flags |= STREAM_FLAGS_CONNECTED;
	stream->qos_queue->stream = stream;

	return stream->qos_queue;
}

static void net_qos_stream_flush(struct port_qos *port, struct stream_queue *stream)
{
	if (!(stream->flags & STREAM_FLAGS_CONNECTED))
		return;

	rtos_atomic_clear_bit(stream->qos_queue->index, &stream->sr_class->pending_mask);

	qos_queue_flush(stream->qos_queue);
}

void net_qos_stream_disconnect(struct port_qos *port, struct qos_queue *qos_q)
{
	struct stream_queue *stream = qos_q->stream;

	qos_q->stream = NULL;

	qos_queue_disconnect(port, qos_q);

	net_qos_stream_flush(port, stream);

	stream->qos_queue = NULL;
	stream->flags &= ~STREAM_FLAGS_CONNECTED;
}

static int net_qos_stream_configure(struct port_qos *port, struct stream_queue *stream,
					unsigned int idle_slope)
{
	struct sr_class *sr_class = stream->sr_class;

	if ((port->used_rate + idle_slope - stream->idle_slope) > port->max_rate)
		return -1;

	if (stream->flags & STREAM_FLAGS_CONFIGURED) {

		shaper_add(&sr_class->shaper, -stream->shaper.rate);

		sr_class->idle_slope -= stream->idle_slope;
		port->used_rate -= stream->idle_slope;

		sr_class->streams--;
		port->streams--;
	}

	if (idle_slope) {
		unsigned int rate = sr_class_scale_idle_slope(sr_class, idle_slope);	/* bits/interval */

		stream->flags |= STREAM_FLAGS_CONFIGURED;

		shaper_set(&stream->shaper, rate);
		shaper_add(&sr_class->shaper, stream->shaper.rate);

		stream->idle_slope = idle_slope;

		sr_class->idle_slope += idle_slope;
		port->used_rate += idle_slope;

		sr_class->streams++;
		port->streams++;

		if (stream->flags & STREAM_FLAGS_CONNECTED)
			qos_queue_enable(stream->qos_queue);
	} else {
		net_qos_stream_flush(port, stream);

		stream->flags &= ~STREAM_FLAGS_CONFIGURED;
		stream->idle_slope = 0;
		shaper_set(&stream->shaper, 0);

		if (stream->flags & STREAM_FLAGS_CONNECTED)
			qos_queue_disable(stream->qos_queue);
	}

	if (sr_class->tc->flags & TC_FLAGS_HW_CBS) {
		if (port_set_tx_idle_slope(port->net_port, sr_class->idle_slope, sr_class->tc->hw_queue_id) < 0)
			os_log(LOG_ERR, "port(%u) could not set idle-slope, queue: %u, idle-slope: %u\n",
					port->net_port->index, sr_class->tc->hw_queue_id, sr_class->idle_slope);
	}

	return 0;
}

void net_qos_sr_config_event(struct net_socket *sock)
{
	if (net_qos_sr_config(&net_tx_ctx.net_qos, sock->handle_data) < 0)
		goto err;

	rtos_event_group_set(&sock->event_group, SOCKET_SUCCESS);

	return;
err:
	rtos_event_group_set(&sock->event_group, SOCKET_ERROR);
}

int net_qos_sr_config(struct net_qos *net, struct net_sr_config *sr_config)
{
	struct logical_port *logical_port = __logical_port_get(sr_config->port);
	struct port_qos *port;
	struct sr_class *sr_class;
	struct stream_queue *stream;
	int tclass, rc = 0;

	port = logical_port->phys->qos;

	tclass = priority_to_tclass(port, sr_config->priority);
	if (tclass < 0) {
		rc = -1;
		goto err;
	}

	sr_class = port->traffic_class[tclass].sr_class;
	if (!sr_class) {
		rc = -1;
		goto err;
	}

	stream = net_qos_stream_get(port, sr_config->priority, sr_config->stream_id);
	if (!stream) {
		rc  = -1;
		goto err;
	}

	if (sr_config->idle_slope) {
		stream->vlan_label = VLAN_LABEL(sr_config->vlan_id, sr_config->priority, 0);

		if (stream->flags & STREAM_FLAGS_CONNECTED) {
			struct net_socket *sock = container_of(stream->qos_queue->queue, struct net_socket, tx.queue);

			sock->tx.vlan_label = stream->vlan_label;
		}
	}

	rc = net_qos_stream_configure(port, stream, sr_config->idle_slope);
	if (rc < 0)
		goto err;

	return 0;

err:
	return rc;
}

unsigned int sr_class_interval_scale[SR_CLASS_MAX] = {
	[SR_CLASS_A] = 1,
	[SR_CLASS_B] = 2,
	[SR_CLASS_C] = 8,
	[SR_CLASS_D] = 8,
	[SR_CLASS_E] = 8
};

static void sr_class_flush(struct port_qos *port, struct sr_class *class)
{
	struct stream_queue *stream;
	int i;

	for (i = 0; i < class->stream_max; i++) {
		stream = &class->stream[i];

		net_qos_stream_flush(port, stream);
	}
}

__init static void net_qos_sr_class_init(struct sr_class *sr_class, sr_class_t class, unsigned int tnow)
{
	struct stream_queue *stream;
	int i;

	sr_class->scale = sr_class_interval_scale[class];
	rational_init(&sr_class->interval, sr_class_interval_p(class), sr_class_interval_q(class) * sr_class->scale);
	rational_init(&sr_class->tnext, 0, sr_class->interval.q);

	sr_class->tnext.i = tnow;

	if (sr_class_prio(class) == SR_PRIO_HIGH)
		sr_class->stream_max = CFG_SR_CLASS_HIGH_STREAM_MAX;
	else
		sr_class->stream_max = CFG_SR_CLASS_LOW_STREAM_MAX;

	if (sr_class->stream_max > CFG_SR_CLASS_STREAM_MAX)
		sr_class->stream_max = CFG_SR_CLASS_STREAM_MAX;

	sr_class->streams = 0;
	sr_class->pending_mask = 0;
	sr_class->flags = 0;
	sr_class->class = class;

	for (i = 0; i < sr_class->stream_max; i++) {
		stream = &sr_class->stream[i];
		stream->sr_class = sr_class;
	}
}

static void traffic_class_flush(struct port_qos *port, struct traffic_class *tc)
{
	struct qos_queue *qos_q;
	int i;

	for (i = 0; i < CFG_TRAFFIC_CLASS_QUEUE_MAX; i++) {
		qos_q = &tc->qos_queue[i];

		qos_queue_flush(qos_q);
	}
}

__init static void traffic_class_init(struct traffic_class *tc, unsigned int index)
{
	struct qos_queue *qos_q;
	int i;

	tc->index = index;
	tc->hw_queue_id = 0;
	tc->flags = 0;
	tc->sr_class = NULL;
	tc->shared_pending_mask = 0;
	tc->disabled_mask = 0xffffffff;
	tc->scheduled_mask = 0;
	tc->slast = 0;

	for (i = 0; i < CFG_TRAFFIC_CLASS_QUEUE_MAX; i++) {
		qos_q = &tc->qos_queue[i];

		qos_queue_init(qos_q, tc, i);
	}
}

void net_qos_port_flush(struct port_qos *port)
{
	struct traffic_class *tc;
	struct sr_class *class;
	int i;

	for (i = 0; i < port->traffic_class_max; i++) {
		tc = &port->traffic_class[i];

		traffic_class_flush(port, tc);
	}

	for (i = 0; i < CFG_SR_CLASS_MAX; i++) {
		class = &port->sr_class[i];

		sr_class_flush(port, class);
	}
}

static int net_qos_map_traffic_class_to_hw_queues(struct port_qos *port)
{
	struct tx_queue_properties *tx_q_cap = port->net_port->tx_q_cap;
	struct tx_queue_properties tx_q_cfg;
	struct traffic_class *tc;
	int i, j;
	unsigned int num_tc = port->traffic_class_max;
	unsigned int num_sr = CFG_SR_CLASS_MAX;
	unsigned int num_tx_q = tx_q_cap->num_queues;

	os_log(LOG_INFO, "port(%u) num tc: %u, num sr: %u, num hw queues: %u\n",
	       port->net_port->index, num_tc, num_sr, num_tx_q);

	/*
	 * The full SW mapping uses a single HW queue and all traffic classes are
	 * mapped to it.
	 *
	 * If the net port have multiple queues, the following mappings are possible:
	 *  - 1 to 1 for strict prio and SR classes
	 *  - 1 to 1 for SR classes and 1 queue for all other strict prio traffic classes
	 *  - 1 to 1 for strict prio when no SR class required
	 *
	 * SR classes always require a dedicated HW queue with CBS support. Otherwise,
	 * full SW mapping is used.
	 */

	if (num_tx_q > 1) {
		unsigned int num_cbs_q = port_tx_queue_prop_num_cbs(tx_q_cap);
		unsigned int hw_queue, flags;
		int full_hw = 0;

		os_log(LOG_INFO, "num hw queues: %u, num cbs: %u\n", num_tx_q, num_cbs_q);

		/* Either there is a one-to-one mapping */
		if (num_tc <= num_tx_q) {
			full_hw = 1;
			tx_q_cfg.num_queues = num_tc;
		/* Or there are some CBS queues that can be used for shaping */
		} else {
			/* If no SR classes or not enough CBS HW queues go full sw */
			if (!num_sr || num_cbs_q < num_sr)
				goto sw_config;

			/* Check that it remains at least on queue for mapping SP tc */
			if (num_tx_q <= num_sr)
				goto sw_config;

			tx_q_cfg.num_queues = num_sr + 1;
		}

		for (i = num_tc - 1, j = tx_q_cfg.num_queues - 1; i >= 0; i--, j--) {
			tc = &port->traffic_class[i];
			hw_queue = j;
			flags = 0;

			if (tc->sr_class) {
				if (tx_q_cap->queue_prop[hw_queue] & TX_QUEUE_FLAGS_CREDIT_SHAPER) {
					flags = TC_FLAGS_HW_CBS;
					tx_q_cfg.queue_prop[hw_queue] = TX_QUEUE_FLAGS_CREDIT_SHAPER;
				} else if (tx_q_cap->queue_prop[hw_queue] & TX_QUEUE_FLAGS_STRICT_PRIORITY) {
					tx_q_cfg.queue_prop[hw_queue] = TX_QUEUE_FLAGS_STRICT_PRIORITY;
				} else {
					goto sw_config;
				}
			} else {
				if (full_hw)
					flags = TC_FLAGS_HW_SP;
				else
					hw_queue = 0;

				if (tx_q_cap->queue_prop[hw_queue] & TX_QUEUE_FLAGS_STRICT_PRIORITY)
					tx_q_cfg.queue_prop[hw_queue] = TX_QUEUE_FLAGS_STRICT_PRIORITY;
				else
					goto sw_config;
			}

			tc->hw_queue_id = hw_queue;
			tc->flags |= flags;
		}
		goto exit;
	}

sw_config:
	/* Full SW mapping */
	for (i = 0; i < num_tc; i++) {
		tc = &port->traffic_class[i];

		tc->hw_queue_id = 0;
		tc->flags &= ~(TC_FLAGS_HW_SP | TC_FLAGS_HW_CBS);
	}

	tx_q_cfg.num_queues = 1;
	tx_q_cfg.queue_prop[0] = TX_QUEUE_FLAGS_STRICT_PRIORITY;

exit:
	if (port_set_tx_queue_config(port->net_port, &tx_q_cfg) < 0) {
		os_log(LOG_ERR, "port_set_tx_queue_config() error\n");
		return -1;
	}

	for (i = 0; i < port->traffic_class_max; i++) {
		tc = &port->traffic_class[i];
		os_log(LOG_INFO, "tc(%u)->hw_queue_id: %u, flags: %x, hw queue prop: %x\n",
		       i, tc->hw_queue_id, tc->flags, tx_q_cfg.queue_prop[tc->hw_queue_id]);
	}

	return 0;
}

__init static void net_qos_set_traffic_class_max(struct port_qos *port)
{
	struct tx_queue_properties *tx_q_cap = port->net_port->tx_q_cap;

	if ((tx_q_cap->num_queues >= 4) && (port->traffic_class_max > tx_q_cap->num_queues))
		port->traffic_class_max = tx_q_cap->num_queues;
}

__init static int net_qos_port_init(struct net_qos *net, struct port_qos *port)
{
	struct traffic_class *tc;
	struct sr_class *class;
	int i, tclass;

	port->net = net;
	port->streams = 0;
	port->tnow = 0;
	port->interval_n = 0;

	port->interval = NET_RX_TX_PERIOD;

	shaper_init(&port->shaper, 0);
	port->shaper.credit_min = 0;

	port->max_rate = 0;
	port->used_rate = 0;

	port->traffic_class_max = CFG_TRAFFIC_CLASS_MAX;

	net_qos_set_traffic_class_max(port);

	port_set_priority_to_traffic_class_map(port->net_port, port->traffic_class_max, CFG_SR_CLASS_MAX, NULL);

	for (i = 0; i < port->traffic_class_max; i++) {
		tc = &port->traffic_class[i];

		traffic_class_init(tc, i);
	}

	for (i = 0; i < CFG_SR_CLASS_MAX; i++) {
		class = &port->sr_class[i];

		tclass = priority_to_tclass(port, sr_prio_pcp(i));
		if (tclass < 0)
			goto err;

		net_qos_sr_class_init(class, sr_prio_class(i), port->tnow);

		rational_int_div(&class->interval_ratio, &class->interval, port->interval);

		tc = &port->traffic_class[tclass];
		tc->sr_class = class;
		class->tc = tc;
	}

	port_ptp_grid_init(&port->ptp_grid);

	return 0;

err:
	return -1;
}

void net_qos_port_reset(struct port_qos *port, int port_rate_bps)
{
	struct sr_class* class;
	struct stream_queue *stream;
	unsigned int rate = ((port_rate_bps / 1000000) * port->interval) / 1000; /* bits/interval */
	int i, j;

	/* Port reset */
	/* Allow for a 200ppm drift between the hardware timer and ethernet transmit clocks */
	shaper_init(&port->shaper, rate - (rate + 4999) / 5000);
	port->max_rate = (port_rate_bps / 100) * 75; /* 75%, in bits/s */
	port->streams = 0;
	port->used_rate = 0;
	port->interval_n = 0;

	for (i = 0; i < CFG_SR_CLASS_MAX; i++) {
		class = &port->sr_class[i];

		/* Class reset */
		shaper_init(&class->shaper, 0);
		class->interval_n = 0;
		class->streams = 0;
		class->idle_slope = 0;

		for (j = 0; j < class->stream_max; j++) {
			stream = &class->stream[j];

			/* Re-configure previously configured streams */
			// FIXME notify upper layers if streams cannot be configured anymore
			if (stream->flags & STREAM_FLAGS_CONFIGURED) {
				unsigned int idle_slope = stream->idle_slope;

				stream->flags &= ~STREAM_FLAGS_CONFIGURED;
				stream->idle_slope = 0;
				net_qos_stream_configure(port, stream, idle_slope);
			}
		}
	}
}

void port_qos_up(struct net_port *port)
{
	net_qos_port_reset(port->qos, PORT_RATE_BPS);
}

void port_qos_down(struct net_port *port)
{
	net_qos_port_flush(port->qos);
}

__init int net_qos_init(struct net_qos *net)
{
	int i, j;

	for (i = 0, j = 0; i < CFG_PORTS; i++) {
		struct net_port *net_port = &ports[i];
		struct port_qos *port = &net->port[j];

		if (net_port->logical_port->is_bridge)
			continue;

		port->net_port = net_port;
		net_port->qos = port;

		if (net_qos_port_init(net, port) < 0) {
			os_log(LOG_ERR, "net_qos_port_init() error\n");
			goto err;
		}

		net_qos_port_reset(port, PORT_RATE_BPS);

		if (net_qos_map_traffic_class_to_hw_queues(port) < 0) {
			os_log(LOG_ERR, "net_qos_map_traffic_class_to_hw_queues() error\n");
			goto err;
		}

		j++;
	}

	return 0;

err:
	return -1;
}

__exit void net_qos_exit(struct net_qos *net)
{
}
