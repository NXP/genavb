/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2019-2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _COMMON_AECP_H_
#define _COMMON_AECP_H_

#include <genavb/genavb.h>

int aecp_aem_send_set_control(struct avb_control_handle *ctrl_h, avb_u64 *entity_id, int unsolicited, avb_u16 descriptor_index, void *value, avb_u16 *len, avb_u8 *status, int sync);
int aecp_aem_send_set_control_single_u8(struct avb_control_handle *ctrl_h, avb_u64 *entity_id, avb_u16 descriptor_index, avb_u8 value);
int aecp_aem_send_set_control_single_u8_command(struct avb_control_handle *ctrl_h, avb_u64 *entity_id, avb_u16 descriptor_index, avb_u8 *value, avb_u8 *status, int sync);
int aecp_aem_send_set_control_single_u8_response(struct avb_control_handle *ctrl_h, avb_u16 descriptor_index, avb_u8 *value);
int aecp_aem_send_set_control_single_u8_unsolicited_response(struct avb_control_handle *ctrl_h, avb_u16 descriptor_index, avb_u8 *value);
int aecp_aem_send_set_control_utf8(struct avb_control_handle *ctrl_h, avb_u64 *entity_id, avb_u16 descriptor_index, const char *value);
int aecp_aem_send_set_control_utf8_command(struct avb_control_handle *ctrl_h, avb_u64 *entity_id, avb_u16 descriptor_index, const char *value, avb_u8 *status, int sync);
int aecp_aem_send_set_control_utf8_response(struct avb_control_handle *ctrl_h, avb_u16 descriptor_index, const char *value);
int aecp_aem_send_set_control_utf8_unsolicited_response(struct avb_control_handle *ctrl_h, avb_u16 descriptor_index, const char *value);
int aecp_aem_send_get_control(struct avb_control_handle *ctrl_h, avb_u64 *entity_id, avb_u16 descriptor_index, void *value, avb_u16 *len, avb_u8 *status, int sync);
int aecp_aem_acquire_entity(struct avb_control_handle *ctrl_h, avb_u64 *entity_id, int acquire);
int aecp_aem_send_read_descriptor(struct avb_control_handle *ctrl_h, avb_u64 *entity_id, avb_u16 configuration_index, avb_u16 descriptor_type, avb_u16 descriptor_index, void *buffer, avb_u16 *len, avb_u8 *status, int sync);
int aecp_aem_send_start_streaming(struct avb_control_handle *ctrl_h, avb_u64 *entity_id, avb_u16 descriptor_type, avb_u16 descriptor_index, avb_u8 *status, int sync);
int aecp_aem_send_stop_streaming(struct avb_control_handle *ctrl_h, avb_u64 *entity_id, avb_u16 descriptor_type, avb_u16 descriptor_index, avb_u8 *status, int sync);
int avdecc_fmt_pretty_printf(const struct avdecc_format *format, char *str, size_t size);

#endif /* _COMMON_AECP_H_ */
