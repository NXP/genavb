/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief GenAVB public API
 \details 802.1CB stream identification definitions.

 \copyright Copyright 2023 NXP
*/

#ifndef _OS_GENAVB_PUBLIC_STREAM_IDENTIFICATION_H_
#define _OS_GENAVB_PUBLIC_STREAM_IDENTIFICATION_H_

/** Updates Stream Identity Table entry
 * \ingroup stream_identification
 *
 * \return	::GENAVB_SUCCESS or negative error code.
 * \param index	table entry index
 * \param entry	entry value
 */
int genavb_stream_identification_update(uint32_t index, struct genavb_stream_identity *entry);

/** Delete Stream Identity Table entry
 * \ingroup stream_identification
 *
 * \return	::GENAVB_SUCCESS or negative error code.
 * \param index	table entry index
 */
int genavb_stream_identification_delete(uint32_t index);

/** Read Stream Identity Table entry
 * \ingroup stream_identification
 *
 * \return	::GENAVB_SUCCESS or negative error code.
 * \param index	table entry index
 * \param entry	entry value read
 */
int genavb_stream_identification_read(uint32_t index, struct genavb_stream_identity *entry);

#endif /* _OS_GENAVB_PUBLIC_STREAM_IDENTIFICATION_H_ */
