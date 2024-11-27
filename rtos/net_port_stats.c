/*
 * Copyright 2020-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief RTOS specific Network service implementation
 @details
*/

#include "net_port_stats.h"
#include "common/types.h"

static const char *eth_std_stats_str[ETH_STD_STATS_MAX] = {
	[IEEE_OUT_FRAMES] = "ieee-out-frames",
	[IEEE_IN_FRAMES] = "ieee-in-frames",
	[IEEE_IN_ERROR_CRC_FRAMES] = "ieee-in-error-crc-frames",
	[IEEE_IN_ERROR_ALIGN_FRAMES] = "ieee-in-error-align-frames",
	[IEEE_OUT_ERROR_MAC_INTERNAL_FRAMES] = "ieee-out-error-mac-int-frames",
	[IEEE_IN_ERROR_MAC_INTERNAL_FRAMES] = "ieee-in-error-mac-int-frames",
	[IEEE_OUT_MC_FRAMES] = "ieee-out-mc-frames",
	[IEEE_OUT_BC_FRAMES] = "ieee-out-bc-frames",
	[IEEE_IN_MC_FRAMES] = "ieee-in-mc-frames",
	[IEEE_IN_BC_FRAMES] = "ieee-in-bc-frames",
	[IEEE_OUT_PAUSE_FRAMES] = "ieee-out-pause-frames",
	[IEEE_IN_PAUSE_FRAMES] = "ieee-in-pause-frames",
	[IEEE_OUT_LPI_TIME] = "ieee-out-lpi-time",
	[IEEE_IN_LPI_TIME] = "ieee-in-lpi-time",
	[IEEE_OUT_LPI_TRANSITIONS] = "ieee-out-lpi-transitions",
	[IEEE_IN_LPI_TRANSITIONS] = "ieee-in-lpi-transitions",
	[RMON_IN_ERROR_UNDERSIZE_FRAMES] = "rmon-in-error-undersize-frames",
	[RMON_IN_ERROR_OVERSIZE_FRAMES] = "rmon-in-error-oversize-frames",
	[RMON_IN_ERROR_FRAGS_FRAMES] = "rmon-in-error-frags-frames",
	[RMON_IN_64_FRAMES] = "rmon-in-64-frames",
	[RMON_IN_65_127_FRAMES] = "rmon-in-65-127-frames",
	[RMON_IN_128_255_FRAMES] = "rmon-in-128-255-frames",
	[RMON_IN_256_511_FRAMES] = "rmon-in-256-511-frames",
	[RMON_IN_512_1023_FRAMES] = "rmon-in-512-1023-frames",
	[RMON_IN_1024_MAX_FRAMES] = "rmon-in-1024-max-frames",
	[RMON_OUT_64_FRAMES] = "rmon-out-64-frames",
	[RMON_OUT_65_127_FRAMES] = "rmon-out-65-127-frames",
	[RMON_OUT_128_255_FRAMES] = "rmon-out-128-255-frames",
	[RMON_OUT_256_511_FRAMES] = "rmon-out-256-511-frames",
	[RMON_OUT_512_1023_FRAMES] = "rmon-out-512-1023-frames",
	[RMON_OUT_1024_MAX_FRAMES] = "rmon-out-1024-max-frames",
};

const char *net_port_stats_str(unsigned int id)
{
	if (id >= ETH_STD_STATS_MAX)
		return NULL;

	return eth_std_stats_str[id];
}
