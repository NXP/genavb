/*
 * Copyright 2020-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Frame Preemption OS abstraction
 @details
*/

#ifndef _OS_FRAME_PREEMPTION_H_
#define _OS_FRAME_PREEMPTION_H_

#include "genavb/frame_preemption.h"

int fp_set(unsigned int port_id, unsigned int type, struct genavb_fp_config *config);

int fp_get(unsigned int port_id, unsigned int type, struct genavb_fp_config *config);

#endif /* _OS_FRAME_PREEMPTION_H_ */
