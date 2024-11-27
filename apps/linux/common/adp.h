/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _COMMON_ADP_H_
#define _COMMON_ADP_H_

#include <genavb/genavb.h>

int adp_start_dump_entities(struct avb_control_handle *ctrl_h);
int adp_dump_entities(struct avb_control_handle *ctrl_h, struct entity_info **entities);

#endif /* _COMMON_ADP_H_ */
