/*
* Copyright 2023-2024 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#include "genavb/frer.h"


int genavb_sequence_generation_update(uint32_t index, struct genavb_sequence_generation *entry)
{
	int rc;

	rc = sequence_generation_update(index, entry, SEQG_UPDATE_OPTION_CONFIG);

	return rc;
}

int genavb_sequence_generation_reset(uint32_t index)
{
	int rc;

	rc = sequence_generation_update(index, NULL, SEQG_UPDATE_OPTION_RESET);

	return rc;
}

int genavb_sequence_generation_delete(uint32_t index)
{
	int rc;

	rc = sequence_generation_delete(index);

	return rc;
}

int genavb_sequence_generation_read(uint32_t index, struct genavb_sequence_generation *entry)
{
	int rc;

	rc = sequence_generation_read(index, entry);

	return rc;
}

int genavb_sequence_recovery_update(uint32_t index, struct genavb_sequence_recovery *entry)
{
	int rc;

	rc = sequence_recovery_update(index, entry);

	return rc;
}

int genavb_sequence_recovery_delete(uint32_t index)
{
	int rc;

	rc = sequence_recovery_delete(index);

	return rc;
}

int genavb_sequence_recovery_read(uint32_t index, struct genavb_sequence_recovery *entry)
{
	int rc;

	rc = sequence_recovery_read(index, entry);

	return rc;
}

int genavb_sequence_identification_update(unsigned int port_id, bool direction_out_facing, struct genavb_sequence_identification *entry)
{
	int rc;

	rc = sequence_identification_update(port_id, direction_out_facing, entry);

	return rc;
}

int genavb_sequence_identification_delete(unsigned int port_id, bool direction_out_facing)
{
	int rc;

	rc = sequence_identification_delete(port_id, direction_out_facing);

	return rc;
}

int genavb_sequence_identification_read(unsigned int port_id, bool direction_out_facing, struct genavb_sequence_identification *entry)
{
	int rc;

	rc = sequence_identification_read(port_id, direction_out_facing, entry);

	return rc;
}
