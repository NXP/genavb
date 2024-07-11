/*
 * Copyright 2018, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief GenAVB public control API
 \details Clock domain control API definition for the GenAVB library

 \copyright Copyright 2018, 2023 NXP
*/

#ifndef _GENAVB_PUBLIC_CONTROL_CLOCK_API_H_
#define _GENAVB_PUBLIC_CONTROL_CLOCK_API_H_

#include "control.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "types.h"
#include "avdecc.h"

/**
 * \ingroup stream
 */
typedef enum {
	GENAVB_CLOCK_DOMAIN_DEFAULT = 0,
	GENAVB_MEDIA_CLOCK_DOMAIN_PTP,		/**< Clock domain based on gPTP clock, should be used for talker media streaming (DEPRECATED) */
	GENAVB_MEDIA_CLOCK_DOMAIN_STREAM,		/**< Clock domain based on stream recovered clock, should be use for listener media playback (DEPRECATED) */
	GENAVB_MEDIA_CLOCK_DOMAIN_MASTER_CLK,	/**< Clock domain based on the master clock driving the domain (codec) (DEPRECATED) */

	GENAVB_CLOCK_DOMAIN_0 = 10,		/**< Clock domain 0 */
	GENAVB_CLOCK_DOMAIN_1,			/**< Clock domain 1 */
	GENAVB_CLOCK_DOMAIN_2,			/**< Clock domain 2 */
	GENAVB_CLOCK_DOMAIN_3,			/**< Clock domain 3 */
	GENAVB_CLOCK_DOMAIN_MAX
} genavb_clock_domain_t;

/**
 * \ingroup control
 */
typedef enum {
	GENAVB_CLOCK_SOURCE_TYPE_INTERNAL = 0,	/**< Internal clock source (master) */
	GENAVB_CLOCK_SOURCE_TYPE_INPUT_STREAM	/**< Stream clock source (slave) */
} genavb_clock_source_type_t;

/**
 * \ingroup control
 */
typedef enum {
	GENAVB_CLOCK_SOURCE_AUDIO_CLK = 0,		/**< Hardware audio clock source, driving the audio domain (codecs) */
	GENAVB_CLOCK_SOURCE_PTP_CLK,		/**< gPTP clock source, can be used for talker media streaming */
} genavb_clock_source_local_id_t;

/**
 * \ingroup control
 * Clock Domain set source command
 * Sent by the application to set the clock source of a domain
 */
struct genavb_msg_clock_domain_set_source {
	genavb_clock_domain_t domain;				/**< Clock Domain */
	genavb_clock_source_type_t source_type;
	union {
		avb_u8 stream_id[8];				/**< Only valid for clock sources of type INPUT_STREAM */
		genavb_clock_source_local_id_t local_id;		/**< Only valid for clock sources of type INTERNAL */
	};
};

/**
 * \ingroup control
 * Clock Domain set source response
 * Sent by the GENAVB stack in response to a set clock source command. Status field contains the
 * result of the command execution.
 */
struct genavb_msg_clock_domain_response {
	genavb_clock_domain_t domain;				/**< Clock Domain */
	avb_u32 status;
};

/**
 * \ingroup control
 */
typedef enum {
	GENAVB_CLOCK_DOMAIN_STATUS_UNLOCKED = 0,			/**< Clock domain is not locked to any source */
	GENAVB_CLOCK_DOMAIN_STATUS_LOCKED,				/**< Clock domain is locked to source */
	GENAVB_CLOCK_DOMAIN_STATUS_FREE_WHEELING,			/**< Clock domain is free-wheeling (local clock is valid but not syntonized to source) */
	GENAVB_CLOCK_DOMAIN_STATUS_HW_ERROR			/**< Clock domain hardware error */
} genavb_clock_domain_status_t;

/**
 * \ingroup control
 * Clock Domain get status
 * Sent by the application to retrieve the current clock domain status
 */
struct genavb_msg_clock_domain_get_status {
	genavb_clock_domain_t domain;				/**< Clock Domain */
};

/**
 * \ingroup control
 * Clock Domain status indication
 * Sent by the GENAVB stack in response to a get status command or as an indication when
 * listening to CLOCK_DOMAIN_STATUS. Status field contains the
 * domain's status.
 */
struct genavb_msg_clock_domain_status {
	genavb_clock_domain_t domain;				/**< Clock Domain */
	genavb_clock_source_type_t source_type;
	union {
		avb_u8 stream_id[8];				/**< Only valid for clock sources of type INPUT_STREAM */
		genavb_clock_source_local_id_t local_id;		/**< Only valid for clock sources of type INTERNAL */
	};
	genavb_clock_domain_status_t status;
};

/**
 * \ingroup control
 * Clock Domain channel message type.
 */
union genavb_msg_clock_domain {
	struct genavb_msg_clock_domain_set_source set_source;
	struct genavb_msg_clock_domain_response response;
	struct genavb_msg_clock_domain_get_status get_status;
	struct genavb_msg_clock_domain_status status;
	struct genavb_msg_error_response error_response;
};

#ifdef __cplusplus
}
#endif

#endif /* _GENAVB_PUBLIC_CONTROL_CLOCK_API_H_ */
