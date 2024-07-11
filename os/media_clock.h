/*
 * Copyright 2014 Freescale Semiconductor, Inc.
 * Copyright 2016-2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Media clock interface handling
 @details
*/
#ifndef _OS_MEDIA_CLOCK_H_
#define _OS_MEDIA_CLOCK_H_

#include "osal/media_clock.h"

typedef enum  {
	OS_MCR_ERROR 		= -1,
	OS_MCR_RUNNING 		= 0,
	OS_MCR_RUNNING_LOCKED 	= 1
} os_media_clock_rec_state_t;

int os_media_clock_rec_init(struct os_media_clock_rec *rec, int domain_id);
void os_media_clock_rec_exit(struct os_media_clock_rec *rec);
int os_media_clock_rec_start(struct os_media_clock_rec *rec, u32 ts_0, u32 ts_1);
int os_media_clock_rec_stop(struct os_media_clock_rec *rec);
int os_media_clock_rec_reset(struct os_media_clock_rec *rec);
os_media_clock_rec_state_t os_media_clock_rec_clean(struct os_media_clock_rec *rec, unsigned int *nb_clean);
int os_media_clock_rec_set_ts_freq(struct os_media_clock_rec *rec, unsigned int ts_freq_p, unsigned int ts_freq_q);
int os_media_clock_rec_set_ext_ts(struct os_media_clock_rec *rec);
int os_media_clock_rec_set_ptp_sync(struct os_media_clock_rec *rec);

void os_media_clock_gen_exit(struct os_media_clock_gen *gen);
int os_media_clock_gen_init(struct os_media_clock_gen *gen, int id, unsigned int is_hw);
int os_media_clock_gen_start(struct os_media_clock_gen *gen, u32 *write_index);
int os_media_clock_gen_stop(struct os_media_clock_gen *gen);
int os_media_clock_gen_reset(struct os_media_clock_gen *gen);
void os_media_clock_gen_ts_update(struct os_media_clock_gen *gen, unsigned int *w_idx, unsigned int *count);

#endif /* _OS_MEDIA_CLOCK_H_ */
