/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2020, 2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _POOL_H_
#define _POOL_H_

#ifdef __KERNEL__
#include <linux/spinlock.h>
#include <linux/errno.h>

#define POOL_COUNT_MAX	(1 << 15)
#define POOL_BUFFER_FREE	(1 << 16) /* must be outside valid range, above POOL_COUNT_MAX */
#define POOL_BUFFER_NULL	(1 << 17) /* must be outside valid range, above POOL_COUNT_MAX */

struct buffer_list {
	unsigned int next;
};

struct pool {
	unsigned int first;
	raw_spinlock_t lock;
	void *baseaddr;
	void *end;
	unsigned int count_total;
	unsigned int obj_order;

	struct buffer_list *list;
};

int pool_init(struct pool *pool, void *baseaddr, unsigned int size, unsigned int obj_order);
void pool_exit(struct pool *);

void *pool_alloc(struct pool *);
int pool_alloc_shmem(struct pool *pool, unsigned long *addr_shmem);

int pool_free(struct pool *, void *);
void pool_free_shmem(struct pool *pool, unsigned long addr_shmem);
void pool_free_virt(void *pool, unsigned long entry);

/**
 * pool_shmem_to_virt() - Convert shared memory relative to kernel virtual address
 * @pool: pointer to the pool handle
 * @addr_shmem: shared memory relative address
 *
 * Return: kernel virtual address.
 */
static inline void *pool_shmem_to_virt(struct pool *pool, unsigned long addr_shmem)
{
	return pool->baseaddr + addr_shmem;
}

/**
 * pool_user_shmem_to_virt() - Convert user shared memory relative to kernel virtual address
 * @pool: pointer to the pool handle
 * @addr_shmem: shared memory relative address
 *
 * Return: kernel virtual address in case of success or NULL if the address is invalid.
 */
void *pool_user_shmem_to_virt(struct pool *pool, unsigned long addr_shmem);

/**
 * pool_virt_to_shmem() - Convert kernel virtual to shared memory relative address
 * @pool: pointer to the pool handle
 * @addr: kernel virtual address
 *
 * Return: shared memory relative address.
 */
static inline unsigned long pool_virt_to_shmem(struct pool *pool, void *addr)
{
	return addr - pool->baseaddr;
}

static inline unsigned int addr_to_index(struct pool *pool, void *addr)
{
	return (addr - pool->baseaddr) >> pool->obj_order;
}

static inline void *index_to_addr(struct pool *pool, unsigned int i)
{
	return pool->baseaddr + (i << pool->obj_order);
}

static inline unsigned int addr_error(struct pool *pool, void *addr)
{
	/* Sanity checks */
	if (unlikely((addr < pool->baseaddr) || (addr >= pool->end) || ((unsigned long)addr & ((1 << pool->obj_order) - 1)))) {
		pr_err("%s: pool(%p) free error, buffer(%p) out of range\n", __func__, pool, addr);
		return 1;
	}

	return 0;
}

/**
 * __pool_alloc_locked() - Allocates one buffer from the pool
 * @pool: pointer to the pool handle
 *
 * The caller needs to hold the pool lock
 *
 * Return: kernel virtual buffer address, or NULL if the pool is empty.
 */
static inline void *__pool_alloc_locked(struct pool *pool)
{
	unsigned int first = pool->first;
	void *addr;

	if (unlikely(first == POOL_BUFFER_NULL)) {
		pr_info("%s: pool(%p) empty\n", __func__, pool);
		return NULL;
	}

	addr = index_to_addr(pool, first);
	pool->first = pool->list[first].next;
	pool->list[first].next = POOL_BUFFER_FREE;

	return addr;
}

static inline void *__pool_alloc(struct pool *pool)
{
	unsigned long flags;
	void *addr;

	raw_spin_lock_irqsave(&pool->lock, flags);

	addr = __pool_alloc_locked(pool);

	raw_spin_unlock_irqrestore(&pool->lock, flags);

	return addr;
}

static inline int __pool_alloc_array(struct pool *pool, void **addr, unsigned int n)
{
	unsigned long flags;
	int i;

	raw_spin_lock_irqsave(&pool->lock, flags);

	for (i = 0; i < n; i++) {
		addr[i] = __pool_alloc_locked(pool);
		if (unlikely(!addr[i])) {
			if (!i)
				i = -ENOMEM;

			break;
		}
	}

	raw_spin_unlock_irqrestore(&pool->lock, flags);

	return i;
}


/**
 * __pool_free_locked() - Frees one buffer to the pool
 * @pool: pointer to the pool handle
 * @addr: kernel virtual buffer address to free
 *
 * The caller needs to hold the pool lock. The function performs some sanity checks
 * to determine if the buffer belongs to the pool.
 *
 */
static inline int __pool_free_locked(struct pool *pool, void *addr)
{
	unsigned int index;

	if (unlikely(addr_error(pool, addr)))
		return -EFAULT;

	index = addr_to_index(pool, addr);

	if (unlikely(pool->list[index].next != POOL_BUFFER_FREE)) {
		pr_err("%s: pool(%p) double free error, buffer(%p)\n", __func__, pool, addr);
		return -EFAULT;
	}

	pool->list[index].next = pool->first;
	pool->first = index;

	return 0;
}

static inline int __pool_free(struct pool *pool, void *addr)
{
	int rc;
	unsigned long flags;

	raw_spin_lock_irqsave(&pool->lock, flags);

	rc = __pool_free_locked(pool, addr);

	raw_spin_unlock_irqrestore(&pool->lock, flags);

	return rc;
}

static inline void __pool_free_array(struct pool *pool, void **addr, unsigned int n)
{
	unsigned long flags;
	int i;

	raw_spin_lock_irqsave(&pool->lock, flags);

	for (i = 0; i < n; i++)
		__pool_free_locked(pool, addr[i]);

	raw_spin_unlock_irqrestore(&pool->lock, flags);
}

#endif /* __KERNEL__ */


#endif /* _POOL_H_ */
