/*
* Copyright 2023-2024 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief FRER OS abstraction
 @details
*/

#ifndef _OS_FRER_H_
#define _OS_FRER_H_

#include "os/sys_types.h"
#include "genavb/frer.h"

int sequence_generation_update(uint32_t index, struct genavb_sequence_generation *entry, unsigned int option);
int sequence_generation_delete(uint32_t index);
int sequence_generation_read(uint32_t index, struct genavb_sequence_generation *entry);

int sequence_recovery_update(uint32_t index, struct genavb_sequence_recovery *entry);
int sequence_recovery_delete(uint32_t index);
int sequence_recovery_read(uint32_t index, struct genavb_sequence_recovery *entry);

int sequence_identification_update(unsigned int port_id, bool direction_out_facing, struct genavb_sequence_identification *entry);
int sequence_identification_delete(unsigned int port_id, bool direction_out_facing);
int sequence_identification_read(unsigned int port_id, bool direction_out_facing, struct genavb_sequence_identification *entry);

#define SEQG_UPDATE_OPTION_CONFIG 0x1
#define SEQG_UPDATE_OPTION_RESET 0x2

#endif /* _OS_FRER_H_ */
