/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2022-2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _EPIT_H_
#define _EPIT_H_

#ifdef __KERNEL__

bool is_epit_hw_timer_available(void);
int epit_init(struct platform_driver *);
void epit_exit(struct platform_driver *);

#endif /* __KERNEL__ */

#endif /* _EPIT_H_ */
