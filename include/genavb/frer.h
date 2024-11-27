/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief GenAVB/TSN public API
 \details 802.1CB-2017 FRER definitions.

 \copyright Copyright 2023-2024 NXP
*/
#ifndef _GENAVB_PUBLIC_FRER_H_
#define _GENAVB_PUBLIC_FRER_H_

#include <stdbool.h>
#include <stdint.h>

/** \cond RTOS */

/**
 * \defgroup frer			802.1CB Frame Replication and Elimination for Reliability
 * \ingroup library
 *
 * This API is used to manage Frame Replication and Elimination for Reliability (FRER) of a given network bridge or port (as defined in IEEE 802.1CB-2017 Sections 7, 8 and 10).
 *
 * \defgroup frer_seqg       802.1CB Sequence Generation
 * \ingroup frer
 *
 * This API is used to manage Sequence Generation entries of a given network bridge or port (as defined in IEEE 802.1CB-2017 Sections 7.4.1 and 10.3).
 *
 * \defgroup frer_seqr       802.1CB Sequence Recovery
 * \ingroup frer
 *
 * This API is used to manage Sequence Recovery entries of a given network bridge or port (as defined in IEEE 802.1CB-2017 Sections 7.4.2 and 10.4).
 *
 * \defgroup frer_seqi       802.1CB Sequence Identification
 * \ingroup frer
 *
 * This API is used to manage Sequence Identification entries of a given network bridge or port (as defined in IEEE 802.1CB-2017 Sections 7.6 and 10.5).
 *
 */

/**
 * \ingroup frer_seqg
 * FRER sequence generation
 * 802.1CB-2017 - 10.3
 */
struct genavb_sequence_generation {
	unsigned int stream_n;		/**< Number of streams in the stream handle array */
	uint32_t *stream;		/**< Stream handle array */
	bool direction_out_facing;
	bool reset;
	uint16_t seqnum;		/**< Sequence Generation Number */
};

/**
 * \ingroup frer_seqr
 * FRER sequence recovery
 * 802.1CB-2017 - 10.4.1.5
 */
typedef enum {
	GENAVB_SEQR_VECTOR = 0,
	GENAVB_SEQR_MATCH,
} genavb_seqr_algorithm_t;

/**
 * \ingroup frer_seqr
 * FRER sequence recovery
 * 802.1CB-2017 - 10.4
 */
struct genavb_sequence_recovery {
	unsigned int stream_n;		/**< Number of streams in the stream handle array */
	uint32_t *stream;		/**< Stream handle array */

	unsigned int port_n;		/**< Number of ports in the port array */
	unsigned int *port;		/**< Port array (logical port id)*/

	bool direction_out_facing;
	bool reset;

	genavb_seqr_algorithm_t algorithm;

	uint32_t history_length;

	uint32_t reset_timeout;
	uint32_t invalid_sequence_value;

	bool take_no_sequence;
	bool individual_recovery;

	bool latent_error_detection;

	struct {
		int32_t difference;
		uint32_t period;
		uint16_t paths;
		uint32_t reset_period;
	} latent_error_parameters;
};

/**
 * \ingroup frer_seqi
 * FRER sequence identification
 * 802.1CB-2017 - 10.5.1.5
 */
typedef enum {
	GENAVB_SEQI_RTAG = 1,
	GENAVB_SEQI_HSR_SEQ_TAG,
	GENAVB_SEQI_PRP_SEQ_TRAILER,
} genavb_seqi_encapsulation_t;

/**
 * \ingroup frer_seqi
 * FRER sequence identification
 * 802.1CB-2017 - 10.5
 */
struct genavb_sequence_identification {
	unsigned int stream_n;		/**< Number of streams in the stream handle array */
	uint32_t *stream;		/**< Stream handle array */

	bool active;

	genavb_seqi_encapsulation_t encapsulation;

	int8_t path_id_lan_id;
};

/** \endcond */

/* OS specific headers */
#include "os/frer.h"

#endif /* _GENAVB_PUBLIC_FRER_H_ */
