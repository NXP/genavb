/*
 * AVB GPT driver
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2022-2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _TPM_H_
#define _TPM_H_

#ifdef __KERNEL__

#include <linux/time.h>
#include "hw_timer.h"
#include "media_clock_rec_pll.h"

bool is_tpm_hw_timer_available(void);
int tpm_init(struct platform_driver *);
void tpm_exit(struct platform_driver *);

#endif /* __KERNEL__ */

#endif /* _TPM_H_ */
