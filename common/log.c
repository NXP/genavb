/*
 * Copyright 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2018-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief logging services
 @details print logging messages to the standard output
*/

#include "common/log.h"

#include "os/config.h"
#include "avtp/config.h"
#include "avdecc/config.h"
#include "srp/config.h"
#include "maap/config.h"
#include "gptp/config.h"
#include "api/config.h"
#include "management/config.h"

#include "os/clock.h"

const char *log_lvl_string[] = {
	[LOG_CRIT] =	"CRIT",
	[LOG_ERR] =	"ERR",
	[LOG_INIT] =	"INIT",
	[LOG_INFO] =	"INFO",
	[LOG_DEBUG] =	"DBG"
};


/* default log level per component */
log_level_t log_component_lvl[max_COMPONENT_ID] = {
#if defined(avtp_CFG_LOG)
	[avtp_COMPONENT_ID] =	avtp_CFG_LOG,
#endif
#if defined(avdecc_CFG_LOG)
	[avdecc_COMPONENT_ID] =	avdecc_CFG_LOG,
#endif
#if defined(srp_CFG_LOG)
	[srp_COMPONENT_ID] =	srp_CFG_LOG,
#endif
#if defined(maap_CFG_LOG)
	[maap_COMPONENT_ID] =	maap_CFG_LOG,
#endif
#if defined(gptp_CFG_LOG)
	[gptp_COMPONENT_ID] =	gptp_CFG_LOG,
#endif
#if defined(common_CFG_LOG)
	[common_COMPONENT_ID] =	common_CFG_LOG,
#endif
#if defined(os_CFG_LOG)
	[os_COMPONENT_ID] =	os_CFG_LOG,
#endif
#if defined(api_CFG_LOG)
	[api_COMPONENT_ID] =	api_CFG_LOG,
#endif
#if defined(management_CFG_LOG)
	[management_COMPONENT_ID] =	management_CFG_LOG,
#endif
};

u64 log_time_s;
u64 log_time_ns;

int log_level_set(unsigned int id, log_level_t level)
{
	if (id >= max_COMPONENT_ID)
		return -1;

	log_component_lvl[id] = level;

	return 0;
}

void log_update_time(os_clock_id_t clk_id)
{
	u64 log_time, log_time_s_local, log_time_ns_local;

	if (os_clock_gettime64(clk_id, &log_time) < 0)
		return;

	log_time_s_local = log_time / NSECS_PER_SEC;
	log_time_ns_local = log_time - log_time_s_local * NSECS_PER_SEC;

	log_time_s = log_time_s_local;
	log_time_ns = log_time_ns_local;
}
