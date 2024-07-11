/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <inttypes.h>

#include "log.h"
#include "time.h"

#define DEFAULT_LOG_DIR "/var/log/"
#define DEFAULT_LOG_FILE "avb_media_app"

FILE *g_logfile_hdl = NULL;
char g_timestamp_str[TS_STR_LEN] = {0};

int aar_log_init(char *log_file_path)
{
#if LOG_LEVEL > VERBOSE_NONE
	if (!log_file_path) /* no log file specified, use default. */
		g_logfile_hdl = fopen(DEFAULT_LOG_DIR DEFAULT_LOG_FILE, "a");
	else
		g_logfile_hdl = fopen(log_file_path, "a");

	if (!g_logfile_hdl)
		return -1;

	setlinebuf(g_logfile_hdl);
#endif
	return 0;
}

void aar_log_exit(void)
{
	if (g_logfile_hdl) {
		// Close log file and clear file handle.
		fclose(g_logfile_hdl);
		g_logfile_hdl = NULL;
	}
}

void aar_log_update_time(genavb_clock_id_t clock_id)
{
	uint64_t now;

	if (genavb_clock_gettime64(clock_id, &now) < 0)
		return;

	snprintf(g_timestamp_str, TS_STR_LEN, "%" PRIu64 "", now / NSECS_PER_SEC);
}
