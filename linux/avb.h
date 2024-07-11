/*
 * Copyright 2014 Freescale Semiconductor, Inc.
 * Copyright 2017-2018, 2021-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Linux specific AVB implementation
 @details
*/

#ifndef _LINUX_AVB_H_
#define _LINUX_AVB_H_

#include <pthread.h>

#include "genavb/init.h"

struct avb_ctx {
	void *avtp;
	void *maap;
	void *avdecc;

	struct avdecc_config avdecc_cfg;
	struct avtp_config avtp_cfg;
	struct maap_config maap_cfg;

	uint8_t sr_class[CFG_SR_CLASS_MAX];

	pthread_t avtp_thread;
	int avtp_status;
	pthread_cond_t avtp_cond;

	pthread_t maap_thread;
	int maap_status;
	pthread_cond_t maap_cond;

	pthread_t avdecc_thread;
	int avdecc_status;
	pthread_cond_t avdecc_cond;

	pthread_mutex_t status_mutex;
};

#endif /* _LINUX_AVB_H_ */
