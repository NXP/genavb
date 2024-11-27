/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2020, 2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <linux/slab.h>

#include "pool.h"

#define ROUND_DOWN(p, a)	(typeof(p))((((unsigned long)p) / (a)) * (a))
#define ROUND_UP(p, a)		ROUND_DOWN(p + a - 1, a)


void *pool_user_shmem_to_virt(struct pool *pool, unsigned long addr_shmem)
{
	void *addr = pool->baseaddr + addr_shmem;

	if (addr_error(pool, addr))
		return NULL;

	return addr;
}

/**
 * pool_init() - initializes a buffer pool
 * @pool: pointer to the pool handle to be initialized
 * @baseaddr: base address of the memory range the pool will use
 * @size: size of the memory range the pool will use
 * @obj_order: log2 of the pool buffer size
 *
 * The pool will contain N buffers of fixed size 2^@obj_order and aligned on buffer size.
 * The memory area used for the pool is specified by the caller using @baseaddr and @size.
 * The @pool handle is initialized in this function and must be passed to all other pool functions.
 * The pool maintains a simple linked list of free buffers.
 *
 * Return: 0 on success, -1 on error.
 */
int pool_init(struct pool *pool, void *baseaddr, unsigned int size, unsigned int obj_order)
{
	unsigned int obj_size = (1 << obj_order);
	int i;

	if (obj_order > 20)
		goto err;

	if (obj_order < 5)
		goto err;

	obj_size = (1 << obj_order);

	pool->baseaddr = ROUND_UP(baseaddr, obj_size);
	size = ROUND_DOWN(size, obj_size);
	pool->end = pool->baseaddr + size;
	pool->obj_order = obj_order;

	pool->count_total = size >> pool->obj_order;
	if (!pool->count_total || (pool->count_total > POOL_COUNT_MAX))
		goto err;

	pool->list = kmalloc(pool->count_total * sizeof(struct buffer_list), GFP_KERNEL);
	if (!pool->list)
		goto err;

	for (i = 0; i < pool->count_total; i++)
		pool->list[i].next = i + 1;

	pool->first = 0;
	pool->list[i - 1].next = POOL_BUFFER_NULL;

//	pr_info("%s: pool(%p) [%p-%p], %u %u\n", __func__, pool, pool->baseaddr, pool->end - 1, 1 << pool->obj_order, pool->count_total);

	raw_spin_lock_init(&pool->lock);

	return 0;

err:
	return -1;
}

/**
 * pool_exit() - deinitializes a buffer pool
 * @pool: pointer to the pool handle to be deinitialized
 *
 */
void pool_exit(struct pool *pool)
{
	int i;
	unsigned long flags;

//	pr_info("%s: pool(%p)\n", __func__, pool);

	raw_spin_lock_irqsave(&pool->lock, flags);

	for (i = 0; i < pool->count_total; i++)
		if (pool->list[i].next == POOL_BUFFER_FREE)
			pr_err("%s: pool(%p) buffer(%p) %d\n", __func__, pool, index_to_addr(pool, i), i);

	pool->first = POOL_BUFFER_NULL;

	raw_spin_unlock_irqrestore(&pool->lock, flags);

	kfree(pool->list);
}

/**
 * pool_alloc() - Allocates one buffer from the pool
 * @pool: pointer to the pool handle
 *
 *
 * Return: kernel virtual buffer address, or NULL if the pool is empty.
 */
void *pool_alloc(struct pool *pool)
{
	return __pool_alloc(pool);
}

int pool_alloc_shmem(struct pool *pool, unsigned long *addr_shmem)
{
	void *addr = __pool_alloc(pool);

	if (!addr)
		return -ENOMEM;

	*addr_shmem = pool_virt_to_shmem(pool, addr);

	return 0;
}

/**
 * pool_free() - Frees one buffer to the pool
 * @pool: pointer to the pool handle
 * @addr: kernel virtual buffer address to free
 *
 */
int pool_free(struct pool *pool, void *addr)
{
	return __pool_free(pool, addr);
}

void pool_free_shmem(struct pool *pool, unsigned long addr_shmem)
{
	void *addr = pool_shmem_to_virt(pool, addr_shmem);

	__pool_free(pool, addr);
}

void pool_free_virt(void *pool, unsigned long entry)
{
	__pool_free((struct pool *)pool, (void *)entry);
}
