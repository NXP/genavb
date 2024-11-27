/*
 * Copyright 2020-2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file frame_preemption.c
 \brief frame preemption public API for rtos
 \details
 \copyright Copyright 2020-2021, 2023-2024 NXP
*/

#include "genavb/error.h"
#include "genavb/frame_preemption.h"

#include "os/frame_preemption.h"

int genavb_fp_set(unsigned int port_id, genavb_fp_config_type_t type, struct genavb_fp_config *config)
{
	int rc = GENAVB_SUCCESS;

	if (!config) {
		rc = -GENAVB_ERR_INVALID;
		goto out;
	}

	if (fp_set(port_id, type, config) < 0) {
		rc = -GENAVB_ERR_INVALID;
		goto out;
	}

out:
	return rc;
}

int genavb_fp_get(unsigned int port_id, genavb_fp_config_type_t type, struct genavb_fp_config *config)
{
	int rc = GENAVB_SUCCESS;

	if (!config) {
		rc = -GENAVB_ERR_INVALID;
		goto out;
	}

	if (fp_get(port_id, type, config) < 0) {
		rc = -GENAVB_ERR_INVALID;
		goto out;
	}

out:
	return rc;
}
