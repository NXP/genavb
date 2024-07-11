/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _COMMON_AVDECC_H_
#define _COMMON_AVDECC_H_

#include <genavb/genavb.h>

int avdecc_nvm_bindings_init(struct avb_control_handle *s_avdecc_handle, const char *binding_filename);
void avdecc_nvm_bindings_update(const char *binding_filename, struct genavb_msg_media_stack_bind *binding_params);
void avdecc_nvm_bindings_remove(const char *binding_filename, struct genavb_msg_media_stack_unbind *unbinding_params);
#endif /* _COMMON_AVDECC_H_ */
