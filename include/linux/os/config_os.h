/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2018, 2020, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file config.h
 \brief GenAVB public API
 \details OS specific config header definitions.

 \copyright 2016 Freescale Semiconductor, Inc.
 \copyright Copyright 2016, 2018, 2020, 2023 NXP
*/
#ifndef _OS_GENAVB_PUBLIC_CONFIG_H_
#define _OS_GENAVB_PUBLIC_CONFIG_H_

#define CFG_TRAFFIC_CLASS_MAX 4

typedef enum {
	HYBRID_MODE_MIN = 0,
	HYBRID_MODE_DISABLED = HYBRID_MODE_MIN,
	HYBRID_MODE_EXTERNAL_CLOCK_SYNC, /* Hybrid mode with a single bridge gptp stack: clock synchronization (if needed) is done externally. */
	HYBRID_MODE_GPTP_CLOCK_SYNC,     /* Hybrid mode with two gptp stacks: endpoint and bridge to ensure clock synchronization. */
	HYBRID_MODE_MAX = HYBRID_MODE_GPTP_CLOCK_SYNC,
} hybrid_mode_t;

#endif /* _OS_GENAVB_PUBLIC_CONFIG_H_ */
