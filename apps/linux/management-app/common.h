/*
 * Copyright 2020-2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _MANAGEMENT_APP_COMMON_H_
#define _MANAGEMENT_APP_COMMON_H_
void usage (void);
int managed_set(struct genavb_control_handle *ctrl_h, void *cmd, unsigned int cmd_len, void *response, unsigned int response_len);
int managed_get(struct genavb_control_handle *ctrl_h, void *cmd, unsigned int cmd_len, void *response, unsigned int response_len);
int check_response(uint16_t *expected, uint16_t *response, unsigned int len);
uint8_t *get_response_header(uint8_t *buf, uint16_t *id, uint16_t *length, uint16_t *status);
uint8_t *get_node_next(uint8_t *buf, uint16_t length);
uint8_t *get_node_header(uint8_t *buf, uint16_t *id, uint16_t *length, uint16_t *status);

#endif /* _MANAGEMENT_APP_COMMON_H_ */
