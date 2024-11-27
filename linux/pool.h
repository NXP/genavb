/*
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
 * Copyright 2020-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _POOL_H_
#define _POOL_H_

#include <pthread.h>
#include <errno.h>

#include "common/log.h"

#define POOL_COUNT_MAX	(1 << 15)
#define POOL_BUFFER_FREE	(1 << 16) /* must be outside valid range, above POOL_COUNT_MAX */
#define POOL_BUFFER_NULL	(1 << 17) /* must be outside valid range, above POOL_COUNT_MAX */

struct buffer_list {
	unsigned int next;
	unsigned int tag;
	bool tag_valid;
};

struct pool {
	unsigned int first;
	pthread_mutex_t lock;
	void *baseaddr;
	void *end;
	unsigned int count_total;
	unsigned int obj_order;

	struct buffer_list *list;
};

int pool_init(struct pool *, void *, unsigned int, unsigned int);
void pool_exit(struct pool *);

void *pool_alloc(struct pool *);
void *pool_alloc_with_tag(struct pool *, unsigned int);
int pool_alloc_shmem(struct pool *, unsigned long *);
int pool_alloc_shmem_with_tag(struct pool *, unsigned long *, unsigned int);

int pool_set_tag(struct pool *, void *, unsigned int);

int pool_free(struct pool *, void *);
void pool_free_all_with_tag(struct pool *, unsigned int);
void pool_free_shmem(struct pool *, unsigned long);
void pool_free_virt(void *, unsigned long);

/**
 * pool_align() - Align an address to the nearest previous pool object boundary
 * @pool: pointer to the pool handle
 * @addr: memory address
 *
 * Return: aligned memory address
 */
static inline unsigned long pool_align(struct pool *pool, unsigned long addr)
{
	return addr & ~((1 << pool->obj_order) - 1);
}

/**
 * pool_shmem_to_virt() - Convert shared memory relative to virtual address
 * @pool: pointer to the pool handle
 * @addr_shmem: shared memory relative address
 *
 * Return: kernel virtual address.
 */
static inline void *pool_shmem_to_virt(struct pool *pool, unsigned long addr_shmem)
{
	return (char *)pool->baseaddr + addr_shmem;
}

/**
 * pool_virt_to_shmem() - Convert virtual to shared memory relative address
 * @pool: pointer to the pool handle
 * @addr: kernel virtual address
 *
 * Return: shared memory relative address.
 */
static inline unsigned long pool_virt_to_shmem(struct pool *pool, void *addr)
{
	return (char *)addr - (char *)pool->baseaddr;
}

static inline unsigned int addr_to_index(struct pool *pool, void *addr)
{
	return ((char *)addr - (char *)pool->baseaddr) >> pool->obj_order;
}

static inline void *index_to_addr(struct pool *pool, unsigned int i)
{
	return (char *)pool->baseaddr + (i << pool->obj_order);
}

static inline unsigned int addr_error(struct pool *pool, void *addr)
{
	/* Sanity checks */
	if (unlikely((addr < pool->baseaddr) || (addr >= pool->end) || ((unsigned long)addr & ((1 << pool->obj_order) - 1)))) {
		os_log(LOG_ERR, "pool(%p) free error, buffer(%p) out of range\n", pool, addr);
		return 1;
	}

	return 0;
}

/**
 * __pool_alloc_locked() - Allocates one buffer from the pool
 * @pool:     pointer to the pool handle
 * @set_tag:  If true, set buffer tag to specified argument.
 * @tag:      buffer tag on successfull allocation
 *
 * The caller needs to hold the pool lock
 *
 * Return: kernel virtual buffer address, or NULL if the pool is empty.
 */
static inline void *__pool_alloc_locked(struct pool *pool, bool set_tag, unsigned int tag)
{
	unsigned int first = pool->first;
	void *addr;

	if (unlikely(first == POOL_BUFFER_NULL)) {
		os_log(LOG_INFO, "pool(%p) empty\n", pool);
		return NULL;
	}

	addr = index_to_addr(pool, first);
	pool->first = pool->list[first].next;
	pool->list[first].next = POOL_BUFFER_FREE;
	pool->list[first].tag_valid = set_tag;
	if (set_tag)
		pool->list[first].tag = tag;

	return addr;
}

static inline void *__pool_alloc(struct pool *pool, bool set_tag, unsigned int tag)
{
	void *addr;

	pthread_mutex_lock(&pool->lock);

	addr = __pool_alloc_locked(pool, set_tag, tag);

	pthread_mutex_unlock(&pool->lock);

	return addr;
}

static inline int __pool_alloc_array(struct pool *pool, void **addr, unsigned int n, bool set_tag, unsigned int tag)
{
	int i;

	pthread_mutex_lock(&pool->lock);

	for (i = 0; i < n; i++) {
		addr[i] = __pool_alloc_locked(pool, set_tag, tag);
		if (unlikely(!addr[i])) {
			if (!i)
				i = -ENOMEM;

			break;
		}
	}

	pthread_mutex_unlock(&pool->lock);

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
		os_log(LOG_ERR, "pool(%p) double free error, buffer(%p)\n", pool, addr);
		return -EFAULT;
	}

	pool->list[index].next = pool->first;
	pool->list[index].tag_valid = false;
	pool->first = index;

	return 0;
}

static inline int __pool_free(struct pool *pool, void *addr)
{
	int rc;

	pthread_mutex_lock(&pool->lock);

	rc = __pool_free_locked(pool, addr);

	pthread_mutex_unlock(&pool->lock);

	return rc;
}

static inline void __pool_free_array(struct pool *pool, void **addr, unsigned int n)
{
	int i;

	pthread_mutex_lock(&pool->lock);

	for (i = 0; i < n; i++)
		__pool_free_locked(pool, addr[i]);

	pthread_mutex_unlock(&pool->lock);
}


#endif /* _POOL_H_ */
