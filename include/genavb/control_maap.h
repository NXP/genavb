/*
 * Copyright 2021-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file control_maap.h
 \brief GenAVB/TSN public control API
 \details MAAP control API definition for the GenAVB/TSN library

 \copyright Copyright 2021-2024 NXP
*/

#ifndef _GENAVB_PUBLIC_CONTROL_MAAP_API_H_
#define _GENAVB_PUBLIC_CONTROL_MAAP_API_H_

#include "control.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "types.h"

/**
 * \ingroup control
 * MAAP IPC status
 */
typedef enum {
	MAAP_STATUS_SUCCESS,
	MAAP_STATUS_FREE,
	MAAP_STATUS_CONFLICT,
	MAAP_STATUS_ERROR,
} genavb_maap_status_t;

/**
 * \ingroup control
 * MAAP IPC response status
 */
typedef enum {
	MAAP_RESPONSE_SUCCESS,
	MAAP_RESPONSE_ERROR,
} genavb_maap_response_status_t;

/**
 * \ingroup control
 * MAAP generic command
 */
struct genavb_msg_maap_create {
	avb_u8 flag;		/**< Prefered range flag. 1 to allocate our range to a port, 0 to let the stack choose the range */
	avb_u16 port_id;
	avb_u32 range_id;
	avb_u16 count;		/**< Number of consecutive addresses in the range. max = 65024 */
	avb_u8 base_address[6];		/**< First address of the range */
};

/**
 * \ingroup control
 * MAAP generic command
 */
struct genavb_msg_maap_delete {
	avb_u16 port_id;
	avb_u32 range_id;
};

/**
 * \ingroup control
 * MAAP generic response
 */
struct genavb_msg_maap_create_response {
	avb_u16 status;		/**< ::genavb_maap_response_status_t */
	avb_u16 port_id;
	avb_u32 range_id;
	avb_u16 count;		/**< Number of consecutive addresses in the range. max = 65024 */
	avb_u8 base_address[6];		/**< First address of the range */
};

/**
 * \ingroup control
 * MAAP generic response
 */
struct genavb_msg_maap_delete_response {
	avb_u16 status;		/**< ::genavb_maap_response_status_t */
	avb_u16 port_id;
	avb_u32 range_id;
};

/**
 * \ingroup control
 * MAAP generic indication
 */
struct genavb_maap_status {
	avb_u16 status;		/**< ::genavb_maap_status_t */
	avb_u16 port_id;
	avb_u32 range_id;
	avb_u16 count;		/**< Number of consecutive addresses in the range. max = 65024 */
	avb_u8 base_address[6];		/**< First address of the range */
};

#ifdef __cplusplus
}
#endif

#endif /* _GENAVB_PUBLIC_CONTROL_MAAP_API_H_ */
