/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Media clock interface handling
 @details
 These functions are the linux specific interfaces to the media clock driver.
 The character devices nodes need to be allocated and should match the appropriate type of media clock as
 defined in the driver.
 The timestamps (for recovery and generation) are shared with the driver using a shared memory.
 The controls (start, stop, clean,...) are done using IOCTLs.
 For generation, the wake-up event is handled via epoll mechansim.
*/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <time.h>

#include "common/log.h"
#include "os/media_clock.h"
#include "modules/avbdrv.h"

#define REC_DEV		"/dev/mclk_rec_"
#define GEN_HW_DEV	"/dev/mclk_gen_"
#define GEN_PTP_DEV	"/dev/mclk_ptp_"

int media_clock_rec_init_no_ts(void)
{
	int fd;
	struct mclock_start start;

	/* Open right fd using id when more than one instance will be used */
	fd = open(REC_DEV, O_RDWR);
	if (fd < 0) {
		os_log(LOG_ERR, "open(%s) %s\n", REC_DEV, strerror(errno));
		goto err_open;
	}

	if (ioctl(fd, MCLOCK_IOC_START, &start) < 0) {
		os_log(LOG_ERR, "ioctl() %s\n", strerror(errno));
		goto err_ioctl;
	}

	os_log(LOG_INIT, "MCR init succesfull\n");

	return fd;

err_ioctl:
	close(fd);
err_open:
	return -1;
}

void media_clock_rec_exit_no_ts(int fd)
{
	if (ioctl(fd, MCLOCK_IOC_STOP) < 0)
		os_log(LOG_ERR, "ioctl() %s\n", strerror(errno));

	close(fd);
}

int os_media_clock_rec_init(struct os_media_clock_rec *rec, int domain_id)
{
	struct mclock_gconfig hw_conf;
	char dev_name[32];
	char *_dev_name;

	_dev_name = REC_DEV;

	snprintf(dev_name, sizeof(dev_name),"%s%d", _dev_name, domain_id);

	rec->fd = open(dev_name, O_RDWR);
	if (rec->fd < 0) {
		os_log(LOG_ERR, "open(%s) %s\n", dev_name, strerror(errno));
		goto err_open;
	}

	/* Get HW initial config */
	if (ioctl(rec->fd, MCLOCK_IOC_GCONFIG, &hw_conf) < 0) {
		os_log(LOG_ERR, "ioctl() %s\n", strerror(errno));
		goto err_ioctl;
	}

	rec->array_size = hw_conf.array_size;
	rec->mmap_size = hw_conf.mmap_size;

	rec->array_addr = mmap(NULL, rec->mmap_size, PROT_READ | PROT_WRITE , MAP_SHARED | MAP_LOCKED, rec->fd, 0);
	if (rec->array_addr == MAP_FAILED) {
		os_log(LOG_ERR,"mmap() %s\n", strerror(errno));
		goto err_mmap;
	}

	return 0;

err_mmap:
err_ioctl:
	close(rec->fd);
	rec->fd = -1;

err_open:
	return -1;
}

void os_media_clock_rec_exit(struct os_media_clock_rec *rec)
{
	os_media_clock_rec_stop(rec);
	munmap(rec->array_addr, rec->mmap_size);
	close(rec->fd);
}

int os_media_clock_rec_stop(struct os_media_clock_rec *rec)
{
	if (ioctl(rec->fd, MCLOCK_IOC_STOP) < 0) {
		os_log(LOG_ERR, "ioctl() %s\n", strerror(errno));
		return -1;
	}
	return 0;
}

int os_media_clock_rec_reset(struct os_media_clock_rec *rec)
{
	if (ioctl(rec->fd, MCLOCK_IOC_RESET) < 0) {
		os_log(LOG_ERR, "ioctl() %s\n", strerror(errno));
		return -1;
	}
	return 0;
}

int os_media_clock_rec_set_ts_freq(struct os_media_clock_rec *rec, unsigned int ts_freq_p,
									unsigned int ts_freq_q)
{
	struct mclock_sconfig cfg;

	cfg.cmd = MCLOCK_CFG_FREQ;
	cfg.ts_freq.p = ts_freq_p;
	cfg.ts_freq.q = ts_freq_q;

	if (ioctl(rec->fd, MCLOCK_IOC_SCONFIG, &cfg) < 0) {
		os_log(LOG_ERR, "ioctl() %s\n", strerror(errno));
		return -1;
	}
	return 0;
}

static int media_clock_rec_set_ts_src(struct os_media_clock_rec *rec, mclock_ts_src_t ts_src)
{
	struct mclock_sconfig cfg;

	cfg.cmd = MCLOCK_CFG_TS_SRC;
	cfg.ts_src = ts_src;

	if (ioctl(rec->fd, MCLOCK_IOC_SCONFIG, &cfg) < 0) {
		os_log(LOG_ERR, "ioctl() %s\n", strerror(errno));
		return -1;
	}
	return 0;
}

int os_media_clock_rec_set_ext_ts(struct os_media_clock_rec *rec)
{
	return media_clock_rec_set_ts_src(rec, TS_EXTERNAL);
}

int os_media_clock_rec_set_ptp_sync(struct os_media_clock_rec *rec)
{
	return media_clock_rec_set_ts_src(rec, TS_INTERNAL);
}

int os_media_clock_rec_start(struct os_media_clock_rec *rec, u32 ts_0, u32 ts_1)
{
	struct mclock_start start;

	start.ts_0 = ts_0;
	start.ts_1 = ts_1;

	if (ioctl(rec->fd, MCLOCK_IOC_START, &start) < 0) {
		os_log(LOG_ERR, "ioctl() %s\n", strerror(errno));
		return -1;
	}
	return 0;
}

os_media_clock_rec_state_t os_media_clock_rec_clean(struct os_media_clock_rec *rec, unsigned int *nb_clean)
{
	struct mclock_clean clean;
	os_media_clock_rec_state_t rc = OS_MCR_RUNNING;

	if (ioctl(rec->fd, MCLOCK_IOC_CLEAN, &clean) < 0) {
		os_log(LOG_ERR, "ioctl() %s\n", strerror(errno));
		return OS_MCR_ERROR;
	}

	*nb_clean = clean.nb_clean;

	if (clean.status == MCLOCK_RUNNING_LOCKED)
		rc = OS_MCR_RUNNING_LOCKED;
	else if (clean.status == MCLOCK_STOPPED)
		rc = OS_MCR_ERROR;

	return rc;
}

void os_media_clock_gen_exit(struct os_media_clock_gen *gen)
{
	if (gen->fd >= 0) {
		munmap(gen->array_addr, gen->mmap_size);
		os_media_clock_gen_stop(gen);
		close(gen->fd);
	}
}

int os_media_clock_gen_init(struct os_media_clock_gen *gen, int id, unsigned int is_hw)
{
	char dev_name[32];
	char *_dev_name;
	struct mclock_gconfig hw_conf;

	if (is_hw)
		_dev_name = GEN_HW_DEV;
	else
		_dev_name = GEN_PTP_DEV;

	snprintf(dev_name, sizeof(dev_name),"%s%d", _dev_name, id);

	gen->fd = open(dev_name, O_RDWR);
	if (gen->fd < 0) {
		os_log(LOG_ERR, "open(%s) %s\n", dev_name, strerror(errno));
		goto err_open;
	}

	/* Get HW initial config */
	if (ioctl(gen->fd, MCLOCK_IOC_GCONFIG, &hw_conf) < 0) {
		os_log(LOG_ERR, "ioctl() %s\n", strerror(errno));
		goto err_ioctl;
	}

	gen->mmap_size = hw_conf.mmap_size;
	gen->array_size = hw_conf.array_size;

	gen->ts_freq_p = hw_conf.ts_freq_p;
	gen->ts_freq_q = hw_conf.ts_freq_q;
	gen->timer_period = hw_conf.timer_period;

	if (os_media_clock_gen_reset(gen) < 0) {
		os_log(LOG_ERR,"() %s\n", strerror(errno));
		goto err_mmap;
	}

	gen->array_addr = mmap(NULL, gen->mmap_size, PROT_READ | PROT_WRITE , MAP_SHARED | MAP_LOCKED, gen->fd, 0);
	if (gen->array_addr == MAP_FAILED) {
		os_log(LOG_ERR,"mmap() %s\n", strerror(errno));
		goto err_mmap;
	}

	gen->w_idx = (unsigned int *)((char *)gen->array_addr + gen->array_size * sizeof(unsigned int));
	gen->ptp = gen->w_idx + 1;
	gen->count = gen->w_idx + 2;

	os_log(LOG_INIT, "hw_source(%p) device: %s, fd: %d, init done\n", gen, dev_name, gen->fd);

	return 0;

err_mmap:
err_ioctl:
	close(gen->fd);
	gen->fd = -1;

err_open:
	return -1;
}

void os_media_clock_gen_ts_update(struct os_media_clock_gen *gen, unsigned int *w_idx, unsigned int *count)
{
	*count = *gen->count;
	*w_idx = *gen->w_idx;
}

int os_media_clock_gen_stop(struct os_media_clock_gen *gen)
{
	if (ioctl(gen->fd, MCLOCK_IOC_STOP) < 0) {
		os_log(LOG_ERR, "ioctl() %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

int os_media_clock_gen_start(struct os_media_clock_gen *gen, u32 *write_index)
{
	struct mclock_start dummy;

	if (ioctl(gen->fd, MCLOCK_IOC_START, &dummy) < 0) {
		os_log(LOG_ERR, "ioctl() %s\n", strerror(errno));
		return -1;
	}

	*write_index = *(gen->w_idx);

	return 0;
}

int os_media_clock_gen_reset(struct os_media_clock_gen *gen)
{
	if (ioctl(gen->fd, MCLOCK_IOC_RESET) < 0) {
		os_log(LOG_ERR, "ioctl() %s\n", strerror(errno));
		return -1;
	}

	return 0;
}
