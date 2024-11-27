/*
 * Copyright 2017-2018, 2020, 2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _CRF_STREAM_H_
#define _CRF_STREAM_H_

#include "clock_domain.h"

#define MAX_CLOCK_DOMAIN 1

typedef struct {
	struct avb_stream_params stream_params;
	struct avb_stream_handle *stream_handle;

	unsigned int batch_size_ns;
	unsigned int cur_batch_size;

	bool is_static_config; /* single static stream configuration for the CRF stream supported */
} aar_crf_stream_t;

extern aar_crf_stream_t g_crf_streams[];

static inline unsigned int crf_get_clock_domain(unsigned int domain_index)
{
	return (domain_index + AVB_CLOCK_DOMAIN_0);
}

aar_crf_stream_t *crf_stream_get(unsigned int domain_index);
int crf_stream_create(unsigned int domain_index);
int crf_stream_destroy(unsigned int domain_index);
int crf_connect(media_clock_role_t role, unsigned int domain_index, struct avb_stream_params *stream_params);
void crf_disconnect(unsigned int domain_index);
int crf_configure(unsigned int domain_index, avtp_direction_t direction, unsigned int flags);

#endif /* _CLOCK_DOMAIN_H_ */
