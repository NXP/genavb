/*
 * AVB DMA buffer pool
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
 * Copyright 2020, 2022-2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "avbdrv.h"
#include "pool_dma.h"

/**
 * pool_dma_init() - initializes a buffer pool dma
 * @pool: pointer to the pool dma handle to be initialized
 * @baseaddr: base address of the memory range the pool will use
 * @size: size of the memory range the pool will use
 * @obj_order: log2 of the pool buffer size
 *
 * The pool will contain N buffers of fixed size 2^@obj_order and aligned on buffer size.
 * The memory area used for the pool is specified by the caller using @baseaddr and @size.
 * The @pool handle is initialized in this function and must be passed to all other pool functions.
 * The pool maintains a simple linked list of free buffers.
 * All buffers in the pool are cache clean
 * The pool dma free functions receive a hint of the minimum amount of dirty cache data in a buffer,
 *   and invalidates it before adding to the pool
 *
 * Return: 0 on success, -1 on error.
 */
int pool_dma_init(struct pool_dma *pool, void *baseaddr, unsigned int size, unsigned int obj_order)
{
	while ((1 << obj_order) < L1_CACHE_BYTES)
		obj_order++;

	return pool_init(&pool->pool, baseaddr, size, obj_order);
}

void pool_dma_exit(struct pool_dma *pool)
{
	pool_exit(&pool->pool);
}

int pool_dma_map(struct pool_dma *pool, struct device *dev, unsigned int dev_id)
{
	if (dev_id >= DMA_MAX_DEVICES) {
		pr_err("%s : Unsupported dma device id %u\n", __func__, dev_id);
		goto err;
	}

	if (!dev) {
		pr_err("%s : Can not map pool with a NULL device \n", __func__);
		goto err;
	}

	if (pool->dma_device[dev_id]) {
		pr_err("%s : pool already mapped on this port %u with device %p\n", __func__, dev_id, pool->dma_device[dev_id]);
		goto err;
	}

	/* Save the device struct to use for DMA API */
	pool->dma_device[dev_id] = dev;

	pool->dma_baseaddr[dev_id] = dma_map_single(pool->dma_device[dev_id],
							pool->pool.baseaddr, L1_CACHE_ALIGN(pool->pool.end - pool->pool.baseaddr), DMA_BIDIRECTIONAL);
	if (dma_mapping_error(pool->dma_device[dev_id], pool->dma_baseaddr[dev_id])) {
		pr_err("%s: pool(%p) dev_id %d dma_map_single failed \n", __func__, pool, dev_id);
		goto err;
	}

	 pr_info("%s: pool(%p) dma_map_single: dev_id %d pool->dma_baseaddr 0x%pad  pool->baseaddr %p size %ld \n", __func__,
			pool, dev_id, &pool->dma_baseaddr[dev_id], pool->pool.baseaddr, (long int) (pool->pool.end - pool->pool.baseaddr));

	return 0;

err:
	return -1;
}

void pool_dma_unmap(struct pool_dma *pool, unsigned int dev_id)
{
	if (dev_id >= DMA_MAX_DEVICES) {
		pr_err("%s : Unsupported dma device id %u\n", __func__, dev_id);
		return;
	}

	if (!pool->dma_device[dev_id]) {
		pr_err("%s : Device id %u was not mapped\n", __func__, dev_id);
		return;
	}

	dma_unmap_single(pool->dma_device[dev_id], pool->dma_baseaddr[dev_id], L1_CACHE_ALIGN(pool->pool.end - pool->pool.baseaddr), DMA_BIDIRECTIONAL);

	pr_info("%s: pool(%p) dma_unmap_single: dev_id %d pool->dma_baseaddr 0x%pad size %ld \n", __func__,
			pool, dev_id, &pool->dma_baseaddr[dev_id], (long int) (pool->pool.end - pool->pool.baseaddr));

	pool->dma_device[dev_id] = NULL;
}

static inline void pool_dma_invalidate(struct pool_dma *pool, struct avb_desc *desc)
{
	unsigned long offset;

	if (unlikely(addr_error(&pool->pool, desc)))
		return;

	offset = (void *)desc + NET_DATA_OFFSET - pool->pool.baseaddr;

	//FIXME: The goal here is to invalidate the cache, so using a fixed single dma_device for the sync operation should be sufficient.
	if ((desc->len + desc->offset) < (1 << pool->pool.obj_order)) {
		if (desc->offset > NET_DATA_OFFSET)
			dma_sync_single_for_device(pool->dma_device[0], pool->dma_baseaddr[0] + offset, L1_CACHE_ALIGN(desc->len + (desc->offset - NET_DATA_OFFSET)), DMA_FROM_DEVICE);
		else
			dma_sync_single_for_device(pool->dma_device[0], pool->dma_baseaddr[0] + offset, L1_CACHE_ALIGN(desc->len), DMA_FROM_DEVICE);
	} else {
		pr_info("%s: pool(%p) desc(%p) invalid offset(%d)/length(%d) on free\n", __func__, pool, desc, desc->offset, desc->len);

		dma_sync_single_for_device(pool->dma_device[0], pool->dma_baseaddr[0] + offset, L1_CACHE_ALIGN((1 << pool->pool.obj_order) - NET_DATA_OFFSET), DMA_FROM_DEVICE);
	}
}

void *pool_dma_alloc(struct pool_dma *pool)
{
	return __pool_alloc(&pool->pool);
}


/**
 * pool_dma_alloc_array() - Allocates an array of buffers from the pool
 * @pool: pointer to the pool dma handle
 * @addr: pointer to array that will contain allocated buffer addresses
 * @n: number of buffers to allocate/size of array
 *
 * Allocates @n buffers from the pool and places their kernel virtual addresses
 * in @addr array.
 *
 * Return: number of buffers allocated, or negative error code.
 */
int pool_dma_alloc_array(struct pool_dma *pool, void **addr, unsigned int n)
{
	return __pool_alloc_array(&pool->pool, addr, n);
}

/**
 * pool_dma_alloc_array_shmem() - Allocates an array of buffers from the pool
 * @pool: pointer to the pool dma handle
 * @addr_shmem: pointer to array that will contain allocated buffer addresses
 * @n: number of buffers to allocate/size of array
 *
 * Allocates @n buffers from the pool and places their shared memory relative addresses
 * in @addr_shmem array.
 *
 * Return: number of buffers allocated, or negative error code.
 */
int pool_dma_alloc_array_shmem(struct pool_dma *pool, unsigned long *addr_shmem, unsigned int n)
{
	int i, rc;

	rc = pool_dma_alloc_array(pool, (void **)addr_shmem, n);
	if (rc < 0)
		return rc;

	for (i = 0; i < rc; i++)
		addr_shmem[i] = pool_dma_virt_to_shmem(pool, ((void **)addr_shmem)[i]);

	return rc;
}

/**
 * pool_dma_free() - Frees one buffer to the pool
 * @pool: pointer to the pool dma handle
 * @addr: kernel virtual buffer address to free
 *
 * The function will invalidate the cache for the buffer
 *
 */
int pool_dma_free(struct pool_dma *pool, void *addr)
{
	pool_dma_invalidate(pool, (struct avb_desc *)addr);

	return __pool_free(&pool->pool, addr);
}

void pool_dma_free_virt(void *pool, unsigned long entry)
{
	pool_dma_free(pool, (void *)entry);
}

/**
 * pool_dma_free_array() - Frees an array of buffers to the pool
 * @pool: pointer to the pool dma handle
 * @addr: pointer to array containing addresses of buffers to free
 * @n: number of buffers to free/size of array
 *
 * The @addr array contains kernel virtual addresses of the buffers to free.
 *
 */
void pool_dma_free_array(struct pool_dma *pool, void **addr, unsigned int n)
{
	int i;

	for (i = 0; i < n; i++)
		pool_dma_invalidate(pool, (struct avb_desc *)addr[i]);

	__pool_free_array(&pool->pool, addr, n);
}

/**
 * pool_dma_free_array_shmem() - Frees an array of buffers to the pool
 * @pool: pointer to the pool dma handle
 * @addr: pointer to array containing addresses of buffers to free
 * @n: number of buffers to free/size of array
 *
 * The @addr array contains shared memory relative addresses of the buffers to free.
 *
 */
void pool_dma_free_array_shmem(struct pool_dma *pool, unsigned long *addr_shmem, unsigned int n)
{
	int i;

	for (i = 0; i < n; i++)
		addr_shmem[i] = (unsigned long) pool_dma_shmem_to_virt(pool, addr_shmem[i]);

	pool_dma_free_array(pool, (void **)addr_shmem, n);
}
