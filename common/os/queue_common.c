/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

void queue_flush(struct queue *q, void *data)
{
	unsigned long entry;

	/* Frees all the buffers in the queue */
	/* It's only safe to dequeue from a rx queue because the user is closing it (i.e, no longer dequeing) */
	while (!queue_empty(q)) {
		entry = queue_dequeue(q);

		q->entry_free(data, entry);
	}
}
