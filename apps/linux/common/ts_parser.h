/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _TS_PARSER_H_
#define _TS_PARSER_H_

#include <sys/time.h>
#include <genavb/genavb.h>

#include "file_buffer.h"

#define PES_SIZE	188

#ifdef __cplusplus
extern "C" {
#endif

struct ts_parser {
	uint64_t t0_ns;
	unsigned long long pcr0;

	unsigned int pcr_count;
	unsigned long long buffer_pts0;

	unsigned int transport_rate;

	unsigned long long byte_count;

	unsigned long long pcr[2];
	unsigned long long count[2];
};

void ts_parser_init(struct ts_parser *p);
int ts_parser_timestamp_range(struct file_buffer *b, struct ts_parser *p, struct avb_event *event, unsigned int *event_len, unsigned int data_start, unsigned int data_len, unsigned int presentation_offset);
int ts_parser_is_pcr(void *buf, unsigned long long *pcr);

#ifdef __cplusplus
}
#endif

#endif /* _TS_PARSER_H_ */
