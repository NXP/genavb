/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2019-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Linux specific shared memory allocator
 @details
*/

#ifndef _LINUX_SHMEM_H_
#define _LINUX_SHMEM_H_

#include <string.h>
#include <errno.h>

#include "os_config.h"
#include "common/log.h"

int shmem_init(struct os_net_config *config);
void shmem_exit(void);
void *shmem_alloc(void);
void shmem_free(void *);

extern void *shmem_baseaddr;
extern int shmem_fd;

static inline void *shmem_to_virt(unsigned long addr)
{
	return (char *)shmem_baseaddr + addr;
}

static inline unsigned long virt_to_shmem(void *addr)
{
	return (char *)addr - (char *)shmem_baseaddr;
}

static inline int shmem_alloc_multi(void **buf, unsigned int n)
{
	int rc;
	unsigned int i;

	rc = read(shmem_fd, buf, n * sizeof(unsigned long));
	if (rc < 0) {
		os_log(LOG_ERR, "read() %s\n", strerror(errno));
		return rc;
	}

	rc /= sizeof(unsigned long);

	for (i = 0; i < rc; i++)
		buf[i] = shmem_to_virt(((unsigned long *)buf)[i]);

	return rc;
}

static inline void shmem_free_multi(void **buf, unsigned int n)
{
	int i;
	int rc;

	for (i = 0; i < n; i++)
		((unsigned long *)buf)[i] = virt_to_shmem(buf[i]);

	rc = write(shmem_fd, buf, n * sizeof(unsigned long));
	if (rc < (int)(n * sizeof(unsigned long))) {
		if (rc < 0)
			os_log(LOG_ERR, "write() %s\n", strerror(errno));
		else
			os_log(LOG_ERR, "write() incomplete\n");
	}
}

#endif /* _LINUX_SHMEM_H_ */
