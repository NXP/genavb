/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <string.h>

#include "rtos_mqueue.h"

int rtos_mqueue_init(rtos_mqueue_t *mq, uint32_t nb_items, size_t item_size, uint8_t *storage)
{
    mq->handle = xQueueCreateStatic(nb_items, item_size, storage, &mq->storage);

    mq->is_static = true;

    return (mq->handle != NULL)? 0 : -1;
}

rtos_mqueue_t *rtos_mqueue_alloc_init(uint32_t nb_items, size_t item_size)
{
    rtos_mqueue_t *mq;
    uint8_t *storage;
    int ret;

    mq = (rtos_mqueue_t *)pvPortMalloc(sizeof(rtos_mqueue_t) + (nb_items*item_size));
    if (!mq)
        goto err_alloc;

    memset(mq, 0, sizeof(rtos_mqueue_t) + (nb_items*item_size));
    storage = (uint8_t *)(mq + 1);

    ret = rtos_mqueue_init(mq, nb_items, item_size, storage);
    if (ret < 0)
        goto err_init;

    mq->is_static = false;

    return mq;

err_init:
    vPortFree(mq);

err_alloc:
    return NULL;
}

int rtos_mqueue_destroy(rtos_mqueue_t *mq)
{
    vQueueDelete(mq->handle);

    if (!mq->is_static)
        vPortFree(mq);

    return 0;
}
