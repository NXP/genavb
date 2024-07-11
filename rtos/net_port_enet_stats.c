/*
 * Copyright 2020-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief RTOS specific Network service implementation
 @details
*/

#include "common/types.h"
#include "net_port.h"
#include "net_port_enet.h"
#include "net_port_stats.h"
#include "genavb/stats.h"

#if CFG_NUM_ENET_MAC

#define REG_OFFSET(m) offsetof(ENET_Type, m)

#define ENET_STATS_MAX ETH_STD_STATS_MAX

static const int enet_stats[ENET_STATS_MAX] = {
	[IEEE_OUT_FRAMES] = REG_OFFSET(IEEE_T_FRAME_OK),
	[IEEE_IN_FRAMES] = REG_OFFSET(IEEE_R_FRAME_OK),
	[IEEE_IN_ERROR_CRC_FRAMES] = REG_OFFSET(IEEE_R_CRC),
	[IEEE_IN_ERROR_ALIGN_FRAMES] = REG_OFFSET(IEEE_R_ALIGN),
	[IEEE_OUT_ERROR_MAC_INTERNAL_FRAMES] = REG_OFFSET(IEEE_R_MACERR),
	[IEEE_IN_ERROR_MAC_INTERNAL_FRAMES] = REG_OFFSET(IEEE_R_MACERR),
	[IEEE_OUT_MC_FRAMES] = REG_OFFSET(RMON_T_MC_PKT),
	[IEEE_OUT_BC_FRAMES] = REG_OFFSET(RMON_T_BC_PKT),
	[IEEE_IN_MC_FRAMES] = REG_OFFSET(RMON_R_MC_PKT),
	[IEEE_IN_BC_FRAMES] = REG_OFFSET(RMON_R_BC_PKT),
	[IEEE_OUT_PAUSE_FRAMES] = REG_OFFSET(IEEE_T_FDXFC),
	[IEEE_IN_PAUSE_FRAMES] = REG_OFFSET(IEEE_R_FDXFC),
	[IEEE_OUT_LPI_TIME] = -1,
	[IEEE_IN_LPI_TIME] = -1,
	[IEEE_OUT_LPI_TRANSITIONS] = -1,
	[IEEE_IN_LPI_TRANSITIONS] = -1,
	[RMON_IN_ERROR_UNDERSIZE_FRAMES] = REG_OFFSET(RMON_R_UNDERSIZE),
	[RMON_IN_ERROR_OVERSIZE_FRAMES] = REG_OFFSET(RMON_R_OVERSIZE),
	[RMON_IN_ERROR_FRAGS_FRAMES] = REG_OFFSET(RMON_R_FRAG),
	[RMON_IN_64_FRAMES] = REG_OFFSET(RMON_R_P64),
	[RMON_IN_65_127_FRAMES] = REG_OFFSET(RMON_R_P65TO127),
	[RMON_IN_128_255_FRAMES] = REG_OFFSET(RMON_R_P128TO255),
	[RMON_IN_256_511_FRAMES] = REG_OFFSET(RMON_R_P256TO511),
	[RMON_IN_512_1023_FRAMES] = REG_OFFSET(RMON_R_P512TO1023),
	[RMON_IN_1024_MAX_FRAMES] = REG_OFFSET(RMON_R_P1024TO2047),
	[RMON_OUT_64_FRAMES] = REG_OFFSET(RMON_R_P64),
	[RMON_OUT_65_127_FRAMES] = REG_OFFSET(RMON_R_P65TO127),
	[RMON_OUT_128_255_FRAMES] = REG_OFFSET(RMON_T_P128TO255),
	[RMON_OUT_256_511_FRAMES] = REG_OFFSET(RMON_T_P256TO511),
	[RMON_OUT_512_1023_FRAMES] = REG_OFFSET(RMON_T_P512TO1023),
	[RMON_OUT_1024_MAX_FRAMES] = REG_OFFSET(RMON_T_P1024TO2047),
};

static unsigned int num_stats;

static int enet_stats_get_number(struct net_port *port)
{
	return num_stats;
}

static int enet_stats_get_strings(struct net_port *port, const char **buf, unsigned int buf_len)
{
	int i;

	if (buf_len < (num_stats * sizeof(char *)))
		return -1;

	for (i = 0; i < ENET_STATS_MAX; i++) {
		if (enet_stats[i] < 0)
			continue;

		*buf = net_port_stats_str(i);
		buf++;
	}

	return 0;
}

static int enet_stats_get(struct net_port *port, uint64_t *buf, unsigned int buf_len)
{
	int i, j = 0;

	if (buf_len < (num_stats * sizeof(uint64_t)))
		return -1;

	for (i = 0; i < ENET_STATS_MAX; i++) {
		if (enet_stats[i] < 0)
			continue;

		buf[j++] = *(volatile uint32_t *)((uint8_t *)port->base + enet_stats[i]);
	}

	return 0;
}

__init void enet_stats_init(struct net_port *port)
{
	int i;

	port->drv_ops.stats_get_number = enet_stats_get_number;
	port->drv_ops.stats_get_strings = enet_stats_get_strings;
	port->drv_ops.stats_get = enet_stats_get;

	num_stats = 0;

	for (i = 0; i < ENET_STATS_MAX; i++)
		if (enet_stats[i] >= 0)
			num_stats++;
}

#endif /* CFG_NUM_ENET_MAC */
