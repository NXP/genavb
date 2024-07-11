/*
 * Copyright 2019-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Linux specific TSN (gPTP + SRP) implementation
 @details
*/

#ifndef _LINUX_TSN_H_
#define _LINUX_TSN_H_

#include <pthread.h>

#include "genavb/init.h"
#include "genavb/sr_class.h"
#include "os/clock.h"

struct gptp_linux_config {
	struct fgptp_config gptp_cfg;
	os_clock_id_t clock_log;
	char nvram_file[256];
};

struct tsn_ctx {
	struct gptp_linux_config gptp_linux_cfg;
	struct management_config management_cfg;
	struct srp_config srp_cfg;

	uint8_t sr_class[CFG_SR_CLASS_MAX];

	int gptp_status;
	pthread_cond_t gptp_cond;

	int management_status;
	pthread_cond_t management_cond;

	int srp_status;
	pthread_cond_t srp_cond;

	pthread_mutex_t status_mutex;
};

#endif /* _LINUX_TSN_H_ */
