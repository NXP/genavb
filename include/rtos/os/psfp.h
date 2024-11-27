/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief GenAVB/TSN public API
 \details 802.1Q PSFP definitions.
*/

#ifndef _OS_GENAVB_PUBLIC_PSFP_API_H_
#define _OS_GENAVB_PUBLIC_PSFP_API_H_

/** Updates entry in the Stream Filter table (or creates an entry if one doesn't exist)
 * \ingroup psfp_sf
 *
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param       index
 * \param       instance
 */
int genavb_stream_filter_update(uint32_t index, struct genavb_stream_filter_instance *instance);

/** Deletes entry from the Stream Filter table.
 * \ingroup psfp_sf
 *
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param       index
 */
int genavb_stream_filter_delete(uint32_t index);


/** Reads an entry from the Stream Filter table
 * \ingroup psfp_sf
 *
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param       index
 * \param       instance
 */
int genavb_stream_filter_read(uint32_t index, struct genavb_stream_filter_instance *instance);

/** Retrieves the maximum number of entries for the Stream Filter table.
 * \ingroup psfp_sf
 *
 * \return		:: maximum number of entries.
 */
unsigned int genavb_stream_filter_get_max_entries(void);

/** Updates entry in the Stream Gate table (or creates an entry if one doesn't exist)
 * \ingroup psfp_sg
 *
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param       index
 * \param       clk_id
 * \param       instance
 */
int genavb_stream_gate_update(uint32_t index, genavb_clock_id_t clk_id, struct genavb_stream_gate_instance *instance);

/** Reset Invalid Receive field entry in the Stream Gate entry
 * \ingroup psfp_sg
 *
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param       index
 */
int genavb_stream_gate_reset_rx_invalid(uint32_t index);

/** Reset Octets Exceeded field in the Stream Gate entry 
 * \ingroup psfp_sg
 *
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param       index
 */
int genavb_stream_gate_reset_octets_exceeded(uint32_t index);

/** Deletes entry from the Stream Gate table.
 * \ingroup psfp_sg
 *
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param       index
 */
int genavb_stream_gate_delete(uint32_t index);


/** Reads an entry from the Stream Gate table
 * \ingroup psfp_sg
 *
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param       index
 * \param       type
 * \param       instance
 */
int genavb_stream_gate_read(uint32_t index, genavb_sg_config_type_t type, struct genavb_stream_gate_instance *instance);

/** Retrieves the maximum number of entries for the Stream Gate table.
 * \ingroup psfp_sg
 *
 * \return		:: maximum number of entries.
 */
unsigned int genavb_stream_gate_get_max_entries(void);

/** Retrieves the maximum number of entries for the Stream Gate Control List table.
 * \ingroup psfp_sg
 *
 * \return		:: maximum number of entries.
 */
unsigned int genavb_stream_gate_control_get_max_entries(void);

/** Updates entry in the Stream Flow Meter table (or creates an entry if one doesn't exist)
 * \ingroup psfp_fm
 *
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param       index
 * \param       instance
 */
int genavb_flow_meter_update(uint32_t index, struct genavb_flow_meter_instance *instance);

/** Reset markAllFramesRed parameter in the Stream Flow Meter entry
 * \ingroup psfp_fm
 *
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param       index
 */
int genavb_flow_meter_mark_red_reset(uint32_t index);

/** Deletes entry from the Stream Flow Meter table.
 * \ingroup psfp_fm
 *
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param       index
 */
int genavb_flow_meter_delete(uint32_t index);

/** Reads an entry from the Stream Flow Meter table
 * \ingroup psfp_fm
 *
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param       index
 * \param       instance
 */
int genavb_flow_meter_read(uint32_t index, struct genavb_flow_meter_instance *instance);

/** Retrieves the maximum number of entries for the Stream Flow Meter table.
 * \ingroup psfp_fm
 *
 * \return		:: maximum number of entries.
 */
unsigned int genavb_flow_meter_get_max_entries(void);

#endif /* _OS_GENAVB_PUBLIC_PSFP_API_H_ */
