/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief GenAVB/TSN RTOS public API
 \details 802.1CB-2017 FRER definitions.

 \copyright Copyright 2023-2024 NXP
*/
#ifndef _OS_GENAVB_PUBLIC_FRER_API_H_
#define _OS_GENAVB_PUBLIC_FRER_API_H_

/** Creates or updates a Sequence Generation table entry
 * \ingroup frer
 *
 * \return	::GENAVB_SUCCESS or negative error code.
 * \param index	table entry index
 * \param entry	entry value
 */
int genavb_sequence_generation_update(uint32_t index, struct genavb_sequence_generation *entry);

/** Reset Sequence Generation in Sequence Generation table entry
 * \ingroup frer
 *
 * \return	::GENAVB_SUCCESS or negative error code.
 * \param index	table entry index
 */
int genavb_sequence_generation_reset(uint32_t index);

/** Deletes a Sequence Generation table entry
 * \ingroup frer
 *
 * \return	::GENAVB_SUCCESS or negative error code.
 * \param index	table entry index
 */
int genavb_sequence_generation_delete(uint32_t index);

/** Reads a Sequence Generation table entry
 * \ingroup frer
 *
 * \return	::GENAVB_SUCCESS or negative error code.
 * \param index	table entry index
 * \param entry	entry value read
 */
int genavb_sequence_generation_read(uint32_t index, struct genavb_sequence_generation *entry);

/** Creates or updates a Sequence Recovery table entry
 * \ingroup frer
 *
 * \return	::GENAVB_SUCCESS or negative error code.
 * \param index	table entry index
 * \param entry	entry value
 */
int genavb_sequence_recovery_update(uint32_t index, struct genavb_sequence_recovery *entry);

/** Deletes a Sequence Recovery table entry
 * \ingroup frer
 *
 * \return	::GENAVB_SUCCESS or negative error code.
 * \param index	table entry index
 */
int genavb_sequence_recovery_delete(uint32_t index);

/** Reads a Sequence Recovery table entry
 * \ingroup frer
 *
 * \return	::GENAVB_SUCCESS or negative error code.
 * \param index	table entry index
 * \param entry	entry value read
 */
int genavb_sequence_recovery_read(uint32_t index, struct genavb_sequence_recovery *entry);

/** Creates or updates a Sequence Identification table entry
 * \ingroup frer
 *
 * \return			::GENAVB_SUCCESS or negative error code.
 * \param port_id		logical port id
 * \param direction_out_facing	direction
 * \param entry			entry value
 */
int genavb_sequence_identification_update(unsigned int port_id, bool direction_out_facing, struct genavb_sequence_identification *entry);

/** Deletes a Sequence Identification table entry
 * \ingroup frer
 *
 * \return			::GENAVB_SUCCESS or negative error code.
 * \param port_id		logical port id
 * \param direction_out_facing	direction
 */
int genavb_sequence_identification_delete(unsigned int port_id, bool direction_out_facing);

/** Reads a Sequence Identification table entry
 * \ingroup frer
 *
 * \return			::GENAVB_SUCCESS or negative error code.
 * \param port_id		logical port id
 * \param direction_out_facing	direction
 * \param entry			entry value read
 */
int genavb_sequence_identification_read(unsigned int port_id, bool direction_out_facing, struct genavb_sequence_identification *entry);

#endif /* _OS_GENAVB_PUBLIC_FRER_API_H_ */
