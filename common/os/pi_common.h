/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PI_COMMON_H_
#define _PI_COMMON_H_

struct pi {
	unsigned int ki;
	unsigned int kp;
	int err;
	int64_t integral;
	int u;
};

void pi_reset(struct pi *p, int u);

void pi_init(struct pi *p, unsigned int ki, unsigned int kp);

int pi_update(struct pi *p, int err);

#endif /* _PI_COMMON_H_ */
