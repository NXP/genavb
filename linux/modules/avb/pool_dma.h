/*
 * AVB DMA buffer pool
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
 * Copyright 2020, 2022-2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _POOL_DMA_H_
#define _POOL_DMA_H_

#ifdef __KERNEL__
#include <linux/dma-mapping.h>

#include "pool.h"

#define	DMA_MAX_DEVICES		2

struct pool_dma {
	struct pool pool;

	struct device *dma_device[DMA_MAX_DEVICES];
	dma_addr_t dma_baseaddr[DMA_MAX_DEVICES];
};

int pool_dma_init(struct pool_dma *pool, void *baseaddr, unsigned int size, unsigned int obj_order);
void pool_dma_exit(struct pool_dma *);
int pool_dma_map(struct pool_dma *pool, struct device *dev, unsigned int dev_id);
void pool_dma_unmap(struct pool_dma *pool, unsigned int dev_id);

void *pool_dma_alloc(struct pool_dma *pool);
int pool_dma_alloc_array(struct pool_dma *pool, void **addr, unsigned int count);
int pool_dma_alloc_array_shmem(struct pool_dma *pool, unsigned long *addr_shmem, unsigned int count);

int pool_dma_free(struct pool_dma *pool, void *buf);
void pool_dma_free_array(struct pool_dma *pool, void **addr, unsigned int count);
void pool_dma_free_array_shmem(struct pool_dma *pool, unsigned long *addr_shmem, unsigned int count);

void pool_dma_free_virt(void *pool, unsigned long entry);

/**
 * pool_dma_shmem_to_virt() - Convert shared memory relative to kernel virtual address
 * @pool: pointer to the pool dma handle
 * @addr_shmem: shared memory relative address
 *
 * Return: kernel virtual address.
 */
static inline void *pool_dma_shmem_to_virt(struct pool_dma *pool, unsigned long addr_shmem)
{
	return pool_shmem_to_virt(&pool->pool, addr_shmem);
}

/**
 * pool_dma_virt_to_shmem() - Convert kernel virtual to shared memory relative address
 * @pool: pointer to the pool dma handle
 * @addr: kernel virtual address
 *
 * Return: shared memory relative address.
 */
static inline unsigned long pool_dma_virt_to_shmem(struct pool_dma *pool, void *addr)
{
	return pool_virt_to_shmem(&pool->pool, addr);
}

/**
 * pool_dma_virt_to_dma() - Convert kernel virtual to dma address
 * @pool: pointer to the pool dma handle
 * @addr: kernel virtual address
 * @dev_id: dma device id
 *
 * Return: dma address.
 */
static inline dma_addr_t pool_dma_virt_to_dma(struct pool_dma *pool, void *addr, unsigned int dev_id)
{
	return pool->dma_baseaddr[dev_id] + (addr - pool->pool.baseaddr);
}

#endif /* __KERNEL__ */


#endif /* _POOL_DMA_H_ */
