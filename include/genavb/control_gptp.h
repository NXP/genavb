/*
 * Copyright 2018, 2020-2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file control_gptp.h
 \brief GenAVB/TSN public control API
 \details GPTP control API definition for the GenAVB/TSN library

 \copyright Copyright 2018, 2020-2021, 2023-2024 NXP
*/

#ifndef _GENAVB_PUBLIC_CONTROL_GPTP_API_H_
#define _GENAVB_PUBLIC_CONTROL_GPTP_API_H_

#include "control.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "types.h"
#include "ptp.h"

/**
 * \ingroup control
 * GPTP grand master get status command
 *
 */
struct genavb_msg_gm_get_status {
	uint8_t domain;
};

/**
 * \ingroup control
 *
 */
struct genavb_msg_gm_status {
	uint8_t domain;
	uint64_t gm_id;
	uint16_t num_ptlv;
	struct ptp_clock_identity path_sequence[];
};

/**
 * \ingroup control
 * GPTP port params status
 *
 */
struct genavb_msg_gptp_port_params {
	uint8_t domain;
	uint16_t port_id;
	uint32_t pdelay;
	bool as_capable;
};

/**
 * \ingroup control
 *
 */
union genavb_msg_gptp {
	struct genavb_msg_gm_get_status gm_get_status;
	struct genavb_msg_gm_status gm_status;
	struct genavb_msg_gptp_port_params gptp_port_params;
};

#ifdef __cplusplus
}
#endif

#endif /* _GENAVB_PUBLIC_CONTROL_GPTP_API_H_ */
