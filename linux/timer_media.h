/*
 * Copyright 2019, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Linux specific timer media service implementation
 @details
*/

#ifndef _LINUX_TIMER_MEDIA_H_
#define _LINUX_TIMER_MEDIA_H_

#if defined(CONFIG_AVTP)
void timer_media_process(struct os_timer *t);

int timer_media_start(struct os_timer *t, u64 value, u64 interval_p, u64 interval_q, unsigned int flags);

void timer_media_stop(struct os_timer *t);

int timer_media_create(struct os_timer *t, os_clock_id_t id);

void timer_media_destroy(struct os_timer *t);
#else
static inline void timer_media_process(struct os_timer *t) { return; }

static inline int timer_media_start(struct os_timer *t, u64 value, u64 interval_p, u64 interval_q, unsigned int flags) { return -1; }

static inline void timer_media_stop(struct os_timer *t) { return; }

static inline int timer_media_create(struct os_timer *t, os_clock_id_t id) { return -1; }

static inline void timer_media_destroy(struct os_timer *t) { return; }
#endif

#endif /* _LINUX_TIMER_MEDIA_H_ */
