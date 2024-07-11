/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _QUEUE_COMMON_H_
#define _QUEUE_COMMON_H_

/**
 * DOC: Queue implementation
 *
 * Basic queue implementation supporting lockless single reader/single writer.
 * To support multiple readers or multiple writers the user of the queue
 * needs to add it's own reader/write side locking.
 * The queue entries are the size of a pointer and usually store pointers to buffers.
 */

#ifndef QUEUE_ENTRIES_MAX
#error "OS specific code must define queue size"
#endif

/**
 * struct queue - Queue structure
 * @size - queue size (in number of entries)
 * @flags - queue flags
 * @read - atomic queue read pointer
 * @write - atomic queue write pointer
 * @entry - queue entry storage
 *
 * It's possible to use a bigger queue size at run time by allocating a bigger memory area
 * for the queue structure.
 *
 */
struct queue {
	unsigned int size;
	unsigned int max_size;
	rtos_atomic_t read;
	rtos_atomic_t write;
	void (*entry_free)(void *data, unsigned long entry);
	unsigned long entry[QUEUE_ENTRIES_MAX]; /* Placed last so that queue user can allocate bigger size */
};

void queue_flush(struct queue *q, void *data);

static inline void queue_init(struct queue *q, void (*entry_free)(void *data, unsigned long entry), unsigned int extra)
{
	q->max_size = QUEUE_ENTRIES_MAX + extra;
	q->size = q->max_size;
	q->entry_free = entry_free;
	rtos_atomic_set(&q->read, 0);
	rtos_atomic_set(&q->write, 0);
}

static inline int queue_set_size(struct queue *q, unsigned int size)
{
	if ((q->size < 1U) || (q->size > q->max_size))
		return -1;

	q->size = size;

	return 0;
}

/**
 * queue_pending() -
 * @q - pointer to queue structure
 *
 * Return: number of pending entries (that can be dequeued)
 *
 */
static inline unsigned int queue_pending(struct queue *q)
{
	u32 read = rtos_atomic_read(&q->read);
	u32 write = rtos_atomic_read(&q->write);

	if (write >= read)
		return write - read;
	else
		return (write + q->max_size) - read;
}

/**
 * queue_available() -
 * @q - pointer to queue structure
 *
 * Return: number of available free entries (that can be queued)
 *
 */
static inline unsigned int queue_available(struct queue *q)
{
	int available = q->size - 1U - queue_pending(q);

	if (likely(available >= 0))
		return available;

	return 0;
}

static inline int queue_full(struct queue *q)
{
	return !queue_available(q);
}

static inline int queue_empty(struct queue *q)
{
	return !queue_pending(q);
}

static inline void __queue_incr(struct queue *q, u32 *index)
{
	(*index)++;
	if (*index >= q->max_size)
		*index = 0;
}

/**
 * queue_enqueue_init() - optimized queueing init
 * @q - pointer to queue structure
 * @write - caller provided storage to track optimized queueing operation
 *
 * queue_enqueue_init()/queue_enqueue_next()/queue_enqueue_done() provide
 * an optimized interface to queue several entries in a single go.
 * The functions must be called in that order. queue_enqueue_init()/queue_enqueue_done()
 * must be called once, queue_enqueue_next() may be called several times.
 * The @write variable must not be modified by the caller between the
 * queue_enqueue_init() and queue_enqueue_done() calls (it is used
 * internally by the enqueue functions)
 * At the beginning of the sequence the caller should verify how many entries are
 * available to avoid a queue overflow.
 *
 */
static inline void queue_enqueue_init(struct queue *q, u32 *write)
{
	*write = rtos_atomic_read(&q->write);
}

static inline void queue_enqueue_next(struct queue *q, u32 *write, unsigned long entry)
{
	q->entry[*write] = entry;

	__queue_incr(q, write);
}

static inline void queue_enqueue_done(struct queue *q, u32 write)
{
	smp_wmb();
	rtos_atomic_set(&q->write, write);
}

/**
 * queue_dequeue_init() - optimized dequeueing init
 * @q - pointer to queue structure
 * @read - caller provided storage to track optimized dequeueing operation
 *
 * queue_dequeue_init()/queue_dequeue_next()/queue_dequeue_done() provide
 * an optimized interface to dequeue several entries in a single go.
 * The functions must be called in that order. queue_dequeue_init()/queue_dequeue_done()
 * must be called once, queue_dequeue_next() may be called several times.
 * The @read variable must not be modified by the caller between the
 * queue_dequeue_init() and queue_dequeue_done() calls (it is used
 * internally by the dequeue functions)
 * At the beginning of the sequence the caller should verify how many entries are
 * pending to avoid a queue underflow.
 *
 */
static inline void queue_dequeue_init(struct queue *q, u32 *read)
{
	*read = rtos_atomic_read(&q->read);
}

static inline unsigned long queue_dequeue_next(struct queue *q, u32 *read)
{
	unsigned long entry = q->entry[*read];

	__queue_incr(q, read);

	return entry;
}

static inline void queue_dequeue_done(struct queue *q, u32 read)
{
	smp_wmb();
	rtos_atomic_set(&q->read, read);
}

static inline unsigned long queue_peek(struct queue *q)
{
	if (queue_empty(q))
		return -1; /* 0 is a valid entry value */
	else
		return q->entry[rtos_atomic_read(&q->read)];
}

static inline int queue_enqueue(struct queue *q, unsigned long entry)
{
	u32 write;

	if (queue_full(q))
		return -1;

	queue_enqueue_init(q, &write);

	queue_enqueue_next(q, &write, entry);

	queue_enqueue_done(q, write);

	return 0;
}

static inline unsigned long queue_dequeue(struct queue *q)
{
	u32 read;
	unsigned long entry;

	if (queue_empty(q))
		return -1; /* 0 is a valid entry value */

	queue_dequeue_init(q, &read);

	entry = queue_dequeue_next(q, &read);

	queue_dequeue_done(q, read);

	return entry;
}

#endif /* _QUEUE_COMMON_H_ */
