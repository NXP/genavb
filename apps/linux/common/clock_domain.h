/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _CLOCK_DOMAIN_H_
#define _CLOCK_DOMAIN_H_

typedef enum {
	MEDIA_CLOCK_MASTER,
	MEDIA_CLOCK_SLAVE
}media_clock_role_t;

const char* get_genavb_clock_domain_t_name(genavb_clock_domain_t type);

const char* get_genavb_clock_source_type_t_name(genavb_clock_source_type_t type);

const char* get_genavb_clock_source_local_id_t_name(genavb_clock_source_local_id_t type);

const char* get_genavb_clock_domain_status_t_name(genavb_clock_domain_status_t type);

int get_clk_domain_validity(avb_clock_domain_t clk_domain);

void set_clk_domain_validity(avb_clock_domain_t clk_domain, int validity);

int handle_clock_domain_event(void);

void clock_domain_get_status(avb_clock_domain_t domain);

int get_audio_clk_sync(avb_clock_domain_t clk_domain);

void set_audio_clk_sync(avb_clock_domain_t clk_domain, int clk_sync);

int clock_domain_set_source_stream(avb_clock_domain_t domain, struct avb_stream_params *stream_params);

int clock_domain_set_source_internal(avb_clock_domain_t domain, avb_clock_source_local_id_t local_id);

int clock_domain_set_role(media_clock_role_t role, avb_clock_domain_t domain, struct avb_stream_params *stream_params);

int clock_domain_init(struct avb_handle *s_avb_handle, int *clk_fd);

int clock_domain_exit(void);

#endif /* _CLOCK_DOMAIN_H_ */
