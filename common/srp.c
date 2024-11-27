/*
 * Copyright 2020-2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
  @file		srp.c
  @brief	SRP protocol common definitions
  @details	PDU and protocol definitions for all SRP applications
*/

#include "genavb/ether.h"

#include "common/srp.h"
#include "os/clock.h"


/** Returns total frame size for the specific MAC and physical layer (802.1Q-2018, section 34.4)
* For now assumes 802.3 ethernet
* \return total frame size
* \param logical_port	logical port id
* \param frame_size 	frame size without any MAC framing
*/
static int mac_frame_size(unsigned int logical_port, unsigned int frame_size)
{
	unsigned int ether_frame_size;

	ether_frame_size = sizeof(struct eth_hdr) + frame_size + ETHER_FCS;

	if (ether_frame_size < ETHER_MIN_FRAME_SIZE)
		ether_frame_size = ETHER_MIN_FRAME_SIZE;

	return ETHER_IFG + ETHER_PREAMBLE + ether_frame_size;
}

/** Calculates idle slope given a tspec (802.1Q-2018, section 35.2.4.2)
* \return	calculated idle slope
* \param logical_port	logical port id
* \param sr_class	SR class
* \param max_frame_size max_frame_size (in bytes)
* \param max_interval_frames max_interval_frames
*/
uint64_t srp_tspec_to_idle_slope(unsigned int logical_port, sr_class_t sr_class, unsigned int max_frame_size, unsigned int max_interval_frames)
{
	uint64_t max_frame_rate, idle_slope;

	max_frame_rate = ((uint64_t)max_interval_frames * sr_class_interval_q(sr_class) * NSECS_PER_SEC) / sr_class_interval_p(sr_class);

	idle_slope = (mac_frame_size(logical_port, max_frame_size + sizeof(struct vlanhdr) + 1)) * 8 * max_frame_rate;

	return idle_slope;
}
