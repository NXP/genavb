/*
 * AVB GPT driver
 * Copyright 2016-2023 NXP
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _GPT_H_
#define _GPT_H_

#ifdef __KERNEL__

bool is_gpt_hw_timer_available(void);
int gpt_init(struct platform_driver *);
void gpt_exit(struct platform_driver *);

#endif /* __KERNEL__ */

#endif /* _GPT_H_ */
