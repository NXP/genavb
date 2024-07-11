/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2020, 2022-2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <linux/kernel.h>
#include <linux/fec.h>
#include <linux/math64.h>

#include "genavb/ether.h"
#include "genavb/avtp.h"
#include "genavb/sr_class.h"
#include "genavb/qos.h"

#include "avbdrv.h"
#include "net_tx.h"
#include "net_socket.h"
#include "net_port.h"
#include "debugfs.h"
#include "hw_timer.h"



/**
 * DOC: Transmit QoS (FQTSS)
 *
 * ** Shaper logic **
 *
 * One shaper per stream
 * One shaper per SR class
 *
 * Shaper uses credit, credit_min, rate, pending variables.
 * credit_min is an optimization, and allows us to know if the credit would become positive sometime in
 * the current scheduling interval. If so, the stream can still transmit.
 *
 * Pending - packets are available
 * Scheduled - packets are available and credit is >= credit_min
 *
 * Stream can transmit if associated shaper credit is >= credit_min
 * Credit is decremented by amount of bytes transmitted
 * Credit increments based on SR class interval timer and shaper rate
 * Credit is incremented once, at the start of the interval
 * If packets are pending, credit can go above zero
 * If packets are not pending, maximum credit is 0
 * Transitions between pending/!pending need to be tracked to correctly account positive credit
 * We only care about detecting !pending -> pending transitions, since the credit is not used otherwise
 *
 * Class shapping, similar to above, but class is pending only if at least one of it's streams is scheduled.
 * !pending -> pending transitions can happen not only when packets are enqueued/dequeued, but also
 * when timer increases queue credits. We should also determine precisely when the first queue of a class
 * becomes active.
 *
 * ** Scheduler logic **
 *
 * Queues from the same class are scheduled in round-robin order
 *
 * Queues from different classes are scheduled in strict priority order
 *
 * ** Design details **
 *
 * To avoid too much overhead, in a single interval we make several scheduling decisions based
 * on the state of the different queues/shapers/schedulers at the beginning of the interval. The only
 * limit is to not queue more than 125us worth of data. This is done by using a minimal credit per shaper
 * such that all possible packets are scheduled in the interval.
 *
 * This introduces two type of errors:
 * - If new packets arrive during the 125us period they will not be taken into account in the schedule
 * decisions.
 * - Packets are not transmitted uniformly in the interval. All packets that would be scheduled inside the interval
 * (by a prefect shaper with byte granularity) are transmitted in a burst.
 *
 */

#define SCALING_FACTOR	1024	/* Used to get sub nanosecond precision in the calculated period */
#define DEFAULT_ki	3
#define DEFAULT_kp	1

#define PTP_MAX_ERROR_NS	50000 /* based on expected measurement jitter */
#define PI_MAX_ERROR_NS		1000 /* based on clock accuracy of 100ppm */

static int priority_to_tclass(uint8_t priority)
{
	const u8 *map;

	if (priority >= QOS_PRIORITY_MAX) {
		goto err;
	}

	map = priority_to_traffic_class_map(CFG_TRAFFIC_CLASS_MAX, CFG_SR_CLASS_MAX);
	if (!map) {
		goto err;
	}

	if (map[priority] >= CFG_TRAFFIC_CLASS_MAX) {
		goto err;
	}

	return map[priority];

err:
	return -1;
}

static void port_ptp_grid_init(struct ptp_grid *g)
{
	g->reset = 0;
	g->count = 0;
	g->total = 0;

	g->period = HW_TIMER_PERIOD_NS;
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
	g->period = HW_TIMER_PERIOD_NS;
	g->period_frac = 0;
	g->period_frac_cur = 0;

	pi_reset(&g->pi, g->period * SCALING_FACTOR);

	g->reset++;
}

static void port_ptp_grid_update(struct ptp_grid *g, unsigned int ptp_now)
{
	unsigned int period = ptp_now - g->ptp_last;

	if ((period > (HW_TIMER_PERIOD_NS + PTP_MAX_ERROR_NS)) || (period < (HW_TIMER_PERIOD_NS - PTP_MAX_ERROR_NS))) {
		/* Error in measured ptp time */
		port_ptp_grid_reset(g);

		g->now = ptp_now;
	} else if ((g->period > (HW_TIMER_PERIOD_NS + PI_MAX_ERROR_NS)) || (g->period < (HW_TIMER_PERIOD_NS - PI_MAX_ERROR_NS))) {
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

void port_jitter_stats_init(struct jitter_stats *s)
{
	s->count = 0;

	s->dt_mean = 0;
	s->dt_mean2 = 0;

	s->dt_mean_cur = 0;
	s->dt_mean2_cur = 0;

	s->dt_min = 0xffffffff;
	s->dt_max = 0;
}

void port_jitter_stats(struct jitter_stats *s, unsigned int ptp_now)
{
	if (s->count) {
		unsigned int dt = ptp_now - s->ptp_last;

		s->dt_mean_cur += dt;
		s->dt_mean2_cur += dt *dt;

		if (dt < s->dt_min)
			s->dt_min = dt;
		else if (dt > s->dt_max)
			s->dt_max = dt;
	}

	s->ptp_last = ptp_now;

	s->count++;
	if (!(s->count % (1 << 8))) {
		s->dt_mean = s->dt_mean_cur >> 8;
		s->dt_mean2 = s->dt_mean2_cur >> 8;
		s->dt_mean2 -= (s->dt_mean_cur * s->dt_mean_cur) >> (2 * 8);

		s->dt_mean_cur = 0;
		s->dt_mean2_cur = 0;
	}
}

/* Return number of leading zeros in a BITS_PER_LONG-bit word */
static inline unsigned long leading_zeros(unsigned long x)
{
	unsigned long ret;

	asm("clz\t%0, %1" : "=r" (ret) : "r" (x));

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

	print_debug("%20s: (%1u, %2u) %u %d\n", __func__, stream->sr_class->index, stream->index, tnow, stream->shaper.credit);
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

	print_debug("%s error %d %lx %d\n", __func__, class->index, class->scheduled_mask, class->slast);

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
	return div64_u64((u64)idle_slope * sr_class_interval_p(sr_class->class), (u64)NSEC_PER_SEC * sr_class_interval_q(sr_class->class));
}

static inline void sr_class_incr_credit(struct sr_class *class, unsigned int tnow)
{
	incr_credit(&class->shaper.credit, tnow - class->shaper.tlast, class->shaper.rate);

	class->shaper.tlast = tnow;

	print_debug("%20s: (%u) %u %d\n", __func__, class->index, tnow, class->shaper.credit);
}

static inline unsigned int queue_tx_ready(struct sr_class *class, struct stream_queue *stream)
{
	struct queue *queue = stream->qos_queue->queue;
	struct avb_tx_desc *desc;

	if (unlikely(!queue_pending(queue)))
		return 0;

	desc = (struct avb_tx_desc *)queue_peek(queue);
	if (likely(desc->common.flags & AVB_TX_FLAG_TS)) {
		if (avtp_before(desc->common.ts, class->tnext_gptp))
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
	clear_bit(qos_q->index, &tc->shared_pending_mask);
	if (!queue_tx_ready(class, stream)) {
		if (queue_pending(queue))
			set_bit(qos_q->index, &tc->shared_pending_mask);

		class->pending_mask &= ~(1UL << qos_q->index);
		tc->scheduled_mask &= ~(1UL << qos_q->index);
	} else {
		set_bit(qos_q->index, &tc->shared_pending_mask);

		if (!shaper_ready(&stream->shaper))
			tc->scheduled_mask &= ~(1UL << qos_q->index);
	}

	print_debug("%20s: (%1u, %2u) %10u %4d %d %d\n", __func__, class->index, stream->index, tnow, len, class->shaper.credit, stream->shaper.credit);
}

#ifdef PORT_TRACE
static void port_trace_init(struct port_qos *port)
{
	struct sr_class *sr_class;
	struct stream_queue *stream;
	int i, j;

	for (i = 0; i < CFG_SR_CLASS_MAX; i++) {
		sr_class = &port->sr_class[i];

		for (j = 0; j < sr_class->stream_max; j++) {
			stream = &sr_class->stream[j];

			stream->burst_max = 0;
		}
	}
}

static inline void port_trace_init_period(struct port_qos *port)
{
	struct sr_class *sr_class;
	struct stream_queue *stream;
	int i, j;

	for (i = 0; i < CFG_SR_CLASS_MAX; i++) {
		sr_class = &port->sr_class[i];

		for (j = 0; j < sr_class->stream_max; j++) {
			stream = &sr_class->stream[j];

			stream->burst = 0;
		}
	}
}

static inline void port_trace_freeze(struct port_qos *port)
{
	port->trace_freeze = 1;
}

static void port_trace(struct port_qos *port, struct traffic_class *tclass, struct qos_queue *qos_q, unsigned int tnow)
{
	struct port_trace *trace = &port->trace[port->trace_w];

	trace->port_credit = port->shaper.credit;

	trace->class = tclass->index;

	trace->scheduled_mask = tclass->scheduled_mask;

	if (tclass->sr_class) {
		trace->pending_mask = tclass->sr_class->pending_mask;
		trace->class_credit = tclass->sr_class->shaper.credit;
		trace->ptp = port->ptp_grid.now - tclass->sr_class->sched_offset;
	} else {
		trace->class_credit = 0;
		trace->scheduled_mask = 0;
		trace->ptp = port->ptp_grid.now;
	}

	if (qos_q->stream) {
		struct stream_queue *stream = qos_q->stream;

		trace->ready = queue_tx_ready(tclass->sr_class, qos_q->stream);

		trace->pending = queue_pending(qos_q->queue);
		if ((trace->pending) && (((struct avb_tx_desc *)queue_peek(qos_q->queue))->common.flags & AVB_TX_FLAG_TS))
			trace->ts = ((struct avb_tx_desc *)queue_peek(qos_q->queue))->common.ts;
		else
			trace->ts = 0;

		trace->queue = qos_q->index;
		trace->queue_credit = stream->shaper.credit;

		stream->burst++;
		if (stream->burst > stream->burst_max)
			stream->burst_max = stream->burst;

		if (stream->burst >= 4)
			port_trace_freeze(port);
	} else {
		trace->ts = 0;
		trace->queue = 0;
		trace->queue_credit = 0;
	}

	trace->tnow = tnow;

	port->trace_w++;
	if (port->trace_w >= PORT_TRACE_SIZE) {
		if (port->trace_freeze)
			port->trace_w = PORT_TRACE_SIZE - 1;
		else
			port->trace_w = 0;
	}
}

#else
static void port_trace_init(struct port_qos *port)
{

}

static inline void port_trace_init_period(struct port_qos *port)
{

}

static inline void port_trace_freeze(struct port_qos *port)
{

}

static inline void port_trace(struct port_qos *port, struct traffic_class *tclass, struct qos_queue *qos_q, unsigned int tnow)
{

}
#endif

static int sr_class_tx(struct port_qos *port, struct traffic_class *tc, struct qos_queue *qos_q)
{
	struct queue *queue = qos_q->queue;
	struct avb_tx_desc *desc;
	unsigned int len;
	u32 read;
	int rc;

	queue_dequeue_init(queue, &read);

	desc = (void *)queue_dequeue_next(queue, &read);
	len = desc->common.len;
	desc->queue_id = tc->hw_queue_id;

	rc = fec_enet_start_xmit_avb(port->fec_data, desc);

	if (rc < 0) {
		/* If packet was not added to the hw ring buffer don't finish the dequeing */
		if (rc == -1) {
			queue_dequeue_done(queue, read);
			port_dec_credit(port, len);
			sr_class_dec_credit(tc, qos_q, len);
		} else
			port->tx_full++;

		/* ring buffer is full, exit */
		goto out;
	}

	queue_dequeue_done(queue, read);
	port_dec_credit(port, len);
	sr_class_dec_credit(tc, qos_q, len);

out:
	return rc;
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

		print_debug("%20s: %d %d %d %lx %lx\n", __func__, class->tnext, port->shaper.credit, port->shaper.credit_min,
				class->pending_mask, class->scheduled_mask);

		qos_q = round_robin_scheduler(tc);
		if (!qos_q) {
			rc = -1;
			goto exit;
		}

		/* Stream credit hasn't been updated since the stream was scheduled, do it now */
		stream_incr_credit(qos_q->stream, class->interval_n);

		port_trace(port, tc, qos_q, tnow);

		rc = sr_class_tx(port, tc, qos_q);

		port_trace(port, tc, qos_q, tnow);

		if (rc < 0)
			break;

		if (!port->transmit_event) {
			if (test_bit(SOCKET_ATOMIC_FLAGS_SOCKET_WAITING_EVENT, &qos_q->atomic_flags) && (queue_available(qos_q->queue) >= (qos_q->queue->size >> 2)))
				port->transmit_event = 1;
		}
	}

	rational_add(&class->tnext, &class->tnext, &class->interval);
	class->interval_n++;

exit:
	return rc;
}


static void traffic_class_update_queue(struct traffic_class *tc, struct qos_queue *qos_q)
{
	struct queue *queue = qos_q->queue;

	qos_q->tx++;
	tc->tx++;

	/* Modifying shared_pending_mask races with queueing code and it may leave the bit clear with packets pending */
	/* To work around this race we clear the bit first and then _re-check_ for pending packets. If any are pending
	 * we set the bit again */
	clear_bit(qos_q->index, &tc->shared_pending_mask);
	if (queue_pending(queue))
		set_bit(qos_q->index, &tc->shared_pending_mask);
	else
		tc->scheduled_mask &= ~(1UL << qos_q->index);
}


static int traffic_class_tx(struct port_qos *port, struct traffic_class *tc, struct qos_queue *qos_q)
{
	struct queue *queue = qos_q->queue;
	struct avb_tx_desc *desc;
	unsigned int len;
	u32 read;
	int rc;

	queue_dequeue_init(queue, &read);

	desc = (void *)queue_dequeue_next(queue, &read);
	len = desc->common.len;
	desc->queue_id = tc->hw_queue_id;

	rc = fec_enet_start_xmit_avb(port->fec_data, desc);

	if (rc < 0) {
		/* If packet was not added to the hw ring buffer don't finish the dequeing */
		if (rc == -1) {
			queue_dequeue_done(queue, read);
			port_dec_credit(port, len);
			traffic_class_update_queue(tc, qos_q);
		} else
			port->tx_full++;

		/* ring buffer is full, exit */
		goto out;
	}

	queue_dequeue_done(queue, read);
	port_dec_credit(port, len);
	traffic_class_update_queue(tc, qos_q);

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

		if (queue_pending(qos_q->queue))
			tc->scheduled_mask |= (1UL << qos_q->index);
	}
}


static int traffic_class_scheduler(struct port_qos *port, struct traffic_class *tc, unsigned int tnow)
{
	struct qos_queue *qos_q;
	int rc = 0;

	traffic_class_update(tc);

	/* Transmit traffic class traffic in round robin */
	while (shaper_ready(&port->shaper) && tc->scheduled_mask) {

		print_debug("%20s: %d %d %d %lx %lx\n", __func__, class->tnext, port->shaper.credit, port->shaper.credit_min,
				class->pending_mask, class->scheduled_mask);

		qos_q = round_robin_scheduler(tc);
		if (!qos_q) {
			rc = -1;
			break;
		}

		port_trace(port, tc, qos_q, tnow);

		rc = traffic_class_tx(port, tc, qos_q);

		port_trace(port, tc, qos_q, tnow);

		if (rc < 0)
			break;
	}

	return rc;
}

unsigned int port_scheduler(struct port_qos *port, unsigned int ptp_now)
{
	struct traffic_class *tc;
	unsigned int tnow = port->tnow;
	int i;

	port_ptp_grid_update(&port->ptp_grid, ptp_now);
	port_jitter_stats(&port->jitter_stats, ptp_now);

	port_trace_init_period(port);

	port_incr_credit(port, port->interval_n);

	port->transmit_event = 0;

	/* priority scheduler */
	for (i = CFG_TRAFFIC_CLASS_MAX - 1; i >= 0; i--) {
		tc = &port->traffic_class[i];

		if (tc->sr_class)
			sr_class_scheduler(port, tc, tnow);
		else
			traffic_class_scheduler(port, tc, tnow);
	}

	/* FIXME, needs to be updated to properly support hardware transmit multi queues */
	fec_enet_finish_xmit_avb(port->fec_data, 0);

	port->tnow += port->interval;
	port->interval_n++;

	return port->transmit_event;
}

static void qos_queue_flush(struct port_qos *port, struct qos_queue *qos_q)
{
	struct avb_drv *avb = container_of(port->eth->buf_pool, struct avb_drv, buf_pool);
	struct traffic_class *tc = qos_q->tc;

	if (!(qos_q->flags & QOS_QUEUE_FLAG_CONNECTED))
		return;

	clear_bit(qos_q->index, &tc->shared_pending_mask);
	clear_bit(qos_q->index, &tc->scheduled_mask);

	queue_flush(qos_q->queue, port->eth->buf_pool);

	if (test_bit(SOCKET_ATOMIC_FLAGS_SOCKET_WAITING_EVENT, &qos_q->atomic_flags)) {
		set_bit(ETH_AVB_TX_AVAILABLE_THREAD_BIT, &avb->timer.scheduled_threads);
		wake_up_process(avb->timer.kthread);
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
}

static void qos_queue_disable(struct qos_queue *qos_q)
{
	qos_q->flags &= ~QOS_QUEUE_FLAG_ENABLED;
}

static void qos_queue_init(struct qos_queue *qos_q, struct traffic_class *tc, unsigned int index)
{
	qos_q->tc = tc;
	qos_q->index = index;

	qos_q->queue = NULL;
	qos_q->flags = 0;

	qos_queue_reset(qos_q);
}

struct qos_queue *qos_queue_connect(struct port_qos *port, u8 priority, struct queue *q, unsigned int is_sr)
{
	struct traffic_class *tc;
	struct qos_queue *qos_q;
	int i, tclass;

	tclass = priority_to_tclass(priority);
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
	qos_queue_flush(port, qos_q);

	qos_q->queue = NULL;

	qos_q->flags &= ~QOS_QUEUE_FLAG_CONNECTED;
}

struct stream_queue *net_qos_stream_get(struct port_qos *port, u8 priority, u8 *stream_id)
{
	struct sr_class *sr_class;
	struct stream_queue *free_stream = NULL;
	struct stream_queue *match_stream;
	int i, tclass;

	tclass = priority_to_tclass(priority);
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

struct qos_queue *net_qos_stream_connect(struct port_qos *port, u8 class, u8 *stream_id, struct queue *queue)
{
	struct stream_queue *stream;
	u8 priority;

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

	clear_bit(stream->qos_queue->index, &stream->sr_class->pending_mask);

	qos_queue_flush(port, stream->qos_queue);
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
	int rc;

	if ((port->used_rate + idle_slope - stream->idle_slope) > port->max_rate)
		return -EBUSY;

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

	if (sr_class->tc->flags & SR_FLAGS_HW_CBS) {
		rc = fec_enet_set_idle_slope(port->fec_data, sr_class->tc->hw_queue_id, sr_class->idle_slope);
		if (rc)
			pr_err("%s: could not set idle-slope (%i, %u, %u)\n",
					__func__, rc, sr_class->tc->hw_queue_id, sr_class->idle_slope);
	}

	return 0;
}

int net_qos_sr_config(struct net_qos *net, struct net_sr_config *sr_config)
{
	struct port_qos *port;
	struct sr_class *sr_class;
	struct stream_queue *stream;
	unsigned long flags;
	int tclass, rc = 0;

	if (sr_config->port >= CFG_PORTS) {
		rc = -EINVAL;
		goto err;
	}

	tclass = priority_to_tclass(sr_config->priority);
	if (tclass < 0) {
		rc = -EINVAL;
		goto err;
	}

	port = &net->port[sr_config->port];

	sr_class = port->traffic_class[tclass].sr_class;
	if (!sr_class) {
		rc = -EINVAL;
		goto err;
	}

	raw_spin_lock_irqsave(&port->eth->lock, flags);

	stream = net_qos_stream_get(port, sr_config->priority, sr_config->stream_id);
	if (!stream) {
		rc  = -ENOMEM;
		goto err_unlock;
	}

	if (sr_config->idle_slope) {
		stream->vlan_label = VLAN_LABEL(sr_config->vlan_id, sr_config->priority, 0);

		if (stream->flags & STREAM_FLAGS_CONNECTED) {
			struct net_socket *sock = container_of(stream->qos_queue->queue, struct net_socket, queue);

			sock->vlan_label = stream->vlan_label;
		}
	}

	rc = net_qos_stream_configure(port, stream, sr_config->idle_slope);
	if (rc < 0)
		goto err_unlock;

	raw_spin_unlock_irqrestore(&port->eth->lock, flags);

	return 0;

err_unlock:
	raw_spin_unlock_irqrestore(&port->eth->lock, flags);
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

static void net_qos_sr_class_init(struct sr_class *sr_class, sr_class_t class, unsigned int tnow)
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

		qos_queue_flush(port, qos_q);
	}
}

static void traffic_class_init(struct traffic_class *tc, unsigned int index)
{
	struct qos_queue *qos_q;
	int i;

	tc->index = index;
	tc->hw_queue_id = 0;
	tc->flags = 0;
	tc->sr_class = NULL;
	tc->shared_pending_mask = 0;
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

	for (i = 0; i < CFG_TRAFFIC_CLASS_MAX; i++) {
		tc = &port->traffic_class[i];

		traffic_class_flush(port, tc);
	}

	for (i = 0; i < CFG_SR_CLASS_MAX; i++) {
		class = &port->sr_class[i];

		sr_class_flush(port, class);
	}
}

static int net_qos_port_init(struct net_qos *net, struct port_qos *port)
{
	struct traffic_class *tc;
	struct sr_class *class;
	int i, tclass, rc = 0;

	port->net = net;
	port->streams = 0;
	port->tnow = 0;
	port->interval_n = 0;

	port->interval = HW_TIMER_PERIOD_NS;

	shaper_init(&port->shaper, 0);
	port->shaper.credit_min = 0;

	port->max_rate = 0;
	port->used_rate = 0;

	for (i = 0; i < CFG_TRAFFIC_CLASS_MAX; i++) {
		tc = &port->traffic_class[i];

		traffic_class_init(tc, i);
	}

	for (i = 0; i < CFG_SR_CLASS_MAX; i++) {
		class = &port->sr_class[i];

		tclass = priority_to_tclass(sr_prio_pcp(i));
		if (tclass < 0){
			rc = -EINVAL;
			goto exit;
		}

		net_qos_sr_class_init(class, sr_prio_class(i), port->tnow);

		rational_int_div(&class->interval_ratio, &class->interval, port->interval);

		tc = &port->traffic_class[tclass];
		tc->sr_class = class;
		class->tc = tc;
	}

	port_trace_init(port);
	port_ptp_grid_init(&port->ptp_grid);
	port_jitter_stats_init(&port->jitter_stats);

exit:
	return rc;
}

int net_qos_sr_class_configure(struct avb_drv *avb, struct net_sr_class_cfg *net_sr_class_cfg)
{
	struct net_qos *net = &avb->qos;
	struct traffic_class *tc;
	struct logical_port *logical_port;
	struct port_qos *port;
	struct sr_class *class;
	unsigned long flags;
	int i, tclass, rc = 0;

	logical_port = logical_port_get(avb, net_sr_class_cfg->port);
	if (!logical_port)
		goto err;

	port = &net->port[logical_port->eth->port];

	raw_spin_lock_irqsave(&port->eth->lock, flags);

	if (port->streams > 0) {
		rc = -EINVAL;
		goto err_unlock;
	}

	if (sr_class_config(net_sr_class_cfg->sr_class) < 0) {
		rc = -EINVAL;
		goto err_unlock;
	}

	for (i = 0; i < CFG_SR_CLASS_MAX; i++) {
		class = &port->sr_class[i];

		tclass = priority_to_tclass(sr_prio_pcp(i));
		if (tclass < 0) {
			rc = -EINVAL;
			goto err_unlock;
		}

		net_qos_sr_class_init(class, sr_prio_class(i), port->tnow);

		rational_int_div(&class->interval_ratio, &class->interval, port->interval);

		tc = &port->traffic_class[tclass];
		tc->sr_class = class;
		class->tc = tc;
	}

	raw_spin_unlock_irqrestore(&port->eth->lock, flags);

	return 0;

err_unlock:
	raw_spin_unlock_irqrestore(&port->eth->lock, flags);

err:
	return rc;
}

int net_qos_map_traffic_class_to_hw_queues(struct port_qos *port)
{
	struct tx_queue_properties prop;
	struct traffic_class *tc;
	struct sr_class *class;
	unsigned int be_queue = 0;
	int rc = 0;
	int i;

	/* Considering the very limited number of available
	 * hardware, a few asumptions are made here to keep things simple:
	 *  -maximum number of SR classes is 2
	 *  -only one best-effort queue (all non-SR traffic classes are mapped to it)
	 */
	if (fec_enet_get_tx_queue_properties(port->eth->ifindex, &prop) < 0) {
		rc = -1;
		goto err;
	}

	if (!prop.num_queues) {
		pr_err("%s invalid number of queues\n", __func__);
		rc = -1;
		goto err;
	}

	/* Find best-effort queue */
	for (i = 0; i < prop.num_queues; i++) {
		if (prop.queue[i].priority < prop.queue[be_queue].priority)
			be_queue = i;
	}

	if (!(prop.queue[be_queue].flags & TX_QUEUE_FLAGS_STRICT_PRIORITY)) {
		pr_err("%s lowest priority queue doesn't support strict priority\n", __func__);
		rc = -1;
		goto err;
	}

	/* Map all traffic classes to best effort queue for now */
	for (i = 0; i < CFG_TRAFFIC_CLASS_MAX; i++) {
		tc = &port->traffic_class[i];
		tc->hw_queue_id = be_queue;
	}

	/* If more than one queue we are expecting AVB-capable
	 * hardware which is enough to support current config.
	 */
	if (prop.num_queues > 1) {
		unsigned int num_sr_queue = 0;
		unsigned int sr_high_queue = 0;
		unsigned int sr_low_queue = 0;

		for (i = 0; i < prop.num_queues; i++) {
			if (prop.queue[i].flags & TX_QUEUE_FLAGS_CREDIT_SHAPER) {
				if (!num_sr_queue) {
					sr_high_queue = i;
					sr_low_queue = i;
				}

				if (prop.queue[sr_high_queue].priority < prop.queue[i].priority)
					sr_high_queue = i;

				if (prop.queue[sr_low_queue].priority > prop.queue[i].priority)
					sr_low_queue = i;

				num_sr_queue++;
			}
		}

		if ((num_sr_queue < CFG_SR_CLASS_MAX)
		|| ((num_sr_queue > 1) && (sr_high_queue == sr_low_queue))) {
			pr_err("%s hw tx queues properties don't match SR config\n", __func__);
			rc = -1;
			goto err;
		}

		for (i = 0; i < CFG_SR_CLASS_MAX; i++) {
			class = &port->sr_class[i];

			if (sr_prio_class(i) == sr_class_high())
				class->tc->hw_queue_id = sr_high_queue;
			else
				class->tc->hw_queue_id = sr_low_queue;

			class->tc->flags |= SR_FLAGS_HW_CBS;

			pr_info("%s SR class: %u -> %u\n", __func__, i, class->tc->hw_queue_id);
		}
	}

err:
	return rc;
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
			//FIXME notify upper layers if streams cannot be configured anymore
			if (stream->flags & STREAM_FLAGS_CONFIGURED) {
				unsigned int idle_slope = stream->idle_slope;

				stream->flags &= ~STREAM_FLAGS_CONFIGURED;
				stream->idle_slope = 0;
				net_qos_stream_configure(port, stream, idle_slope);
			}
		}
	}
}

int net_qos_init(struct net_qos *net, struct dentry *avb_dentry)
{
	int i;

	for (i = 0; i < CFG_PORTS; i++) {
		struct port_qos *port = &net->port[i];

		if (net_qos_port_init(net, port) < 0)
			goto err;
	}

	net_qos_debugfs_init(net, avb_dentry);

	return 0;

err:
	return -1;
}

void net_qos_exit(struct net_qos *net)
{

}
