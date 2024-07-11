/*
 * Copyright 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief NXP GPTP linux specific code
 @details Setups linux thread for NXP GPTP stack component. Implements main loop and event handling.
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/epoll.h>

#include "common/types.h"

#include "linux/tsn.h"
#include "linux/cfgfile.h"
#include "linux/log.h"

#include "common/net.h"
#include "common/timer.h"
#include "common/ptp.h"
#include "common/types.h"
#include "common/filter.h"
#include "common/log.h"
#include "common/ipc.h"

#include "gptp/gptp_entry.h"

#define EPOLL_MAX_EVENTS	8

#define NVRAM_FILE_NAME_LEN		128
#define NVRAM_ENTRY_LEN			64
#define NVRAM_PDELAY_DATA_PER_ENTRY	2
static ptp_double pdelay_array[CFG_GPTP_MAX_NUM_PORT];

struct gptp_linux_ctx {
	struct gptp_ctx *gptp;
	char nvram_file[256];						/* path to the vram file */
};

/*******************************************************************************
* @function_name nvram_update
* @brief updates pdelay values in nvram file
*
*/
static int nvram_update_pdelay_entry(struct gptp_linux_ctx *gptp, FILE *tmp_fp)
{
	int i;
	int rc = 0;

	/* writting back fresh pdelay values to temporary nvram file */
	for (i = 0; i < CFG_GPTP_MAX_NUM_PORT; i++) {
		fprintf(tmp_fp, "initial_neighborPropDelay %d %.2f\n", i, pdelay_array[i]);
		os_log(LOG_DEBUG, "add initial_neighborPropDelay %d %.2f\n", i, pdelay_array[i]);
	}

	return rc;
}


/*******************************************************************************
* @function_name nvram_update
* @brief updates nvram contents on exit
*
*/
static int nvram_update(struct gptp_linux_ctx *gptp)
{
	char tmp_nvram_file[NVRAM_FILE_NAME_LEN];
	FILE *tmp_nvram_fp;
	int rc = 0;

	/* create new temporary nvram file,  create it if does not exists */
	if (snprintf(tmp_nvram_file, NVRAM_FILE_NAME_LEN, "%s.tmp", gptp->nvram_file) < 0) {
		rc= -1;
		goto err;
	}

	tmp_nvram_fp = fopen(tmp_nvram_file, "w+");
	if (tmp_nvram_fp == NULL) {
		rc = -1;
		goto err;
	}

	/* update pdelay values in nvram */
	nvram_update_pdelay_entry(gptp, tmp_nvram_fp);

	fflush(tmp_nvram_fp);
	fclose(tmp_nvram_fp);

	/* replace previous nvram file */
	if (rename(tmp_nvram_file, gptp->nvram_file) < 0) {
		os_log(LOG_ERR, "rename: %s\n", strerror(errno));
		rc = -1;
	}

err:
	return rc;
}



/*******************************************************************************
* @function_name nvram_load_pdelay
* @brief read pdelay value for a given port from the nvram file and apply to current configuration
*
*/
static int nvram_load_pdelay(struct gptp_linux_ctx *gptp, struct fgptp_config *cfg, FILE *fp)
{
	char pdelay_key[NVRAM_ENTRY_LEN];
	char *nvram_entry = NULL;
	unsigned int port_id, read_char;
	ptp_double initial_neighborPropDelay;
	int rc = 0;
	size_t len = 0;

	/* looking only for pdelay entries */
	if(snprintf(pdelay_key, NVRAM_ENTRY_LEN, "initial_neighborPropDelay") < 0) {
		rc = -1;
		goto err;
	}

	/* read all entries in the nvram file one by one. If an entry matches
	the pdelay pattern and if found value/port  are valid then update
	pdelay in memory. Upon any error the default initial pdelay value is kept */
	while ((read_char = getline(&nvram_entry, &len, fp)) != -1) {
		if (read_char != strlen(nvram_entry)) {
			os_log(LOG_ERR, "Unexpected embedded null byte(s)\n");
			goto err;
		}

		if((strstr(nvram_entry, pdelay_key)) != NULL) {
			/* a pdelay entry has been found in nvram, now get port and value */
			os_log(LOG_DEBUG, "found %s", nvram_entry);
			if (sscanf(nvram_entry, "%*s %u %lf", &port_id, &initial_neighborPropDelay) == NVRAM_PDELAY_DATA_PER_ENTRY) {
				/* some sanity check on pdelay value and port number */
				os_log(LOG_DEBUG, "checking initial_neighborPropDelay %u %.2f\n", port_id, initial_neighborPropDelay);
				if ((port_id < CFG_GPTP_MAX_NUM_PORT) && (initial_neighborPropDelay >= CFG_GPTP_DEFAULT_PDELAY_VALUE_MIN)) {
					cfg->initial_neighborPropDelay[port_id] = initial_neighborPropDelay;
					os_log(LOG_INFO, "loading pdelay %.2f for port(%u)\n", cfg->initial_neighborPropDelay[port_id], port_id);
				} else {
					os_log(LOG_ERR, "pdelay entry is not valid %s", nvram_entry);
				}
			} else
				os_log(LOG_ERR, "can not read data from nvram\n");
		}
	}

err:
	free(nvram_entry);
	return rc;
}


/*******************************************************************************
* @function_name nvram_load
* @brief open nvram file and retrieve gptp persistents
*
*/
static int nvram_load(struct gptp_linux_ctx *gptp, struct fgptp_config *cfg)
{
	FILE *nvram_fp;
	int rc = 0;

	nvram_fp = fopen(gptp->nvram_file, "r");
	if(nvram_fp == NULL) {
		rc = -1;
		goto err_open;
	}

	printf("FGPTP: fetching nvram parameters from %s\n", gptp->nvram_file);

	/* get previously computed pdelay values for each port */
	nvram_load_pdelay(gptp, cfg, nvram_fp);

	fclose(nvram_fp);

err_open:
	return rc;
}


/*******************************************************************************
* @function_name sync_indication_handler
* @brief called back by the gptp stack upon synchronization state change
*
*/
static void sync_indication_handler(struct gptp_sync_info *info)
{
	if (info->state == SYNC_STATE_SYNCHRONIZED)
		os_log(LOG_INFO, "Port(%u) domain(%u) %s -- synchronization time (ms): %llu\n", info->port_id, info->domain, PTP_SYNC_STATE(info->state), info->sync_time_ms);
	else
		os_log(LOG_INFO, "Port(%u) domain(%u) %s\n", info->port_id, info->domain, PTP_SYNC_STATE(info->state));
}


/*******************************************************************************
* @function_name gm_indication_handler
* @brief called back by the gptp stack upon grand master selection
*
*/
static void gm_indication_handler(struct gptp_gm_info *info)
{
}


/*******************************************************************************
* @function_name pdelay_indication_handler
* @brief called back by the gptp stack upon pdelay computation
*
*/
static void pdelay_indication_handler(struct gptp_pdelay_info *info)
{
	os_log(LOG_INFO, "port(%d) -- computed PDelay (ns) is %.2f\n", info->port_id, info->pdelay);

	/* update current configuration for the port. Will be written back to nvram upon exit.
	Some sanity checks are done to ensure no weird pdelay value will be used at next start */
	if ((info->port_id < CFG_GPTP_MAX_NUM_PORT) && (info->pdelay >= CFG_GPTP_DEFAULT_PDELAY_VALUE_MIN)) {
		pdelay_array[info->port_id] = info->pdelay;
	}
}


static void gptp_thread_cleanup(void *arg)
{
	struct gptp_linux_ctx *gptp_linux = arg;
	struct gptp_ctx *gptp = gptp_linux->gptp;

	gptp_exit(gptp);

	nvram_update(gptp_linux);

	os_log(LOG_INIT, "done\n");
}

static void gptp_status(struct tsn_ctx *tsn, int status)
{
	pthread_mutex_lock(&tsn->status_mutex);

	tsn->gptp_status = status;

	pthread_cond_signal(&tsn->gptp_cond);

	pthread_mutex_unlock(&tsn->status_mutex);
}

void *gptp_thread_main(void *arg)
{
	struct tsn_ctx *tsn = arg;
	struct gptp_linux_ctx gptp_linux = {0};
	struct gptp_ctx *gptp;
	struct fgptp_config *cfg = &tsn->gptp_linux_cfg.gptp_cfg;
	int epoll_fd;
	struct epoll_event event[EPOLL_MAX_EVENTS];
	int i;
	struct sched_param param = {
		.sched_priority = GPTP_CFG_PRIORITY,
	};
	int rc;

	rc = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
	if (rc) {
		os_log(LOG_ERR, "pthread_setschedparam(), %s\n", strerror(rc));
		goto err_setschedparam;
	}

	memcpy(gptp_linux.nvram_file, tsn->gptp_linux_cfg.nvram_file, 256);

	/*
	* Linux event polling setup
	*/
	epoll_fd = epoll_create(1);
	if (epoll_fd < 0) {
		os_log(LOG_CRIT, "epoll_create(): %s\n", strerror(errno));
		goto err_epoll_create;
	}

	/* overwrite default configuration with persistant parameters if any */
	nvram_load(&gptp_linux, cfg);
	cfg->sync_indication = sync_indication_handler;
	cfg->gm_indication = gm_indication_handler;
	cfg->pdelay_indication = pdelay_indication_handler;

	/*
	* Intialize gptp stack and apply configuration
	*/
	gptp = gptp_init(cfg, epoll_fd);
	if (!gptp)
		goto err_gptp_init;

	gptp_linux.gptp = gptp;

	pthread_cleanup_push(gptp_thread_cleanup, &gptp_linux);

	os_log(LOG_INIT, "started\n");

	gptp_status(tsn, 1);

	while (1) {
		int ready;
		struct linux_epoll_data *epoll_data;

		log_update_time(tsn->gptp_linux_cfg.clock_log);
		log_update_monotonic();

		pthread_testcancel();

		ready = epoll_wait(epoll_fd, event, EPOLL_MAX_EVENTS, -1);
		if (ready < 0) {
			if (errno == EINTR)
				continue;

			os_log(LOG_ERR, "epoll_wait(): %s\n", strerror(errno));
			break;
		}

		for (i = 0; i < ready; i++) {
			if (event[i].events & (EPOLLHUP | EPOLLRDHUP))
				os_log(LOG_ERR, "event error: %x\n", event[i].events);

			if (event[i].events & (EPOLLIN | EPOLLERR)) {
				epoll_data = (struct linux_epoll_data *)event[i].data.ptr;

				switch (epoll_data->type) {
				case EPOLL_TYPE_NET_RX:
					net_rx((struct net_rx *)epoll_data->ptr);
					break;

				case EPOLL_TYPE_TIMER:
					os_timer_process((struct os_timer *)epoll_data->ptr);
					break;

				case EPOLL_TYPE_NET_TX_TS:
					net_tx_ts_process((struct net_tx *)epoll_data->ptr);
					break;

				case EPOLL_TYPE_IPC:
					ipc_rx((struct ipc_rx *)epoll_data->ptr);
					break;

				default:
					break;
				}
			}
		}
	}

	pthread_cleanup_pop(1);

	close(epoll_fd);

	return (void *)0;

err_gptp_init:
	close(epoll_fd);

err_epoll_create:
err_setschedparam:
	gptp_status(tsn, -1);

	return (void *)-1;
}
