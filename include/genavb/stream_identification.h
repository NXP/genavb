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

#ifndef _GENAVB_PUBLIC_STREAM_IDENTIFICATION_H_
#define _GENAVB_PUBLIC_STREAM_IDENTIFICATION_H_

#include "genavb/config.h"
#include "genavb/types.h"

/**
 * \ingroup stream_identification
 * Stream identification type
 * 802.1CB-2017, section 9.1.1.6
 * 802.1CBdb-2021, section 9.1.1.6
 */
typedef enum {
	GENAVB_SI_NULL = 1,
	GENAVB_SI_SRC_MAC_VLAN,
	GENAVB_SI_DST_MAC_VLAN,
	GENAVB_SI_IP,
	GENAVB_SI_MASK_AND_MATCH
} genavb_si_t;

/**
 * \ingroup stream_identification
 * Stream identification VLAN tag type
 * 802.1CB-2017, section 9.1.2.2
 */
typedef enum {
	GENAVB_SI_TAGGED = 1,	/**< frame must have a VLAN tag to be recognized as belonging to the Stream */
	GENAVB_SI_PRIORITY,	/**< frame must be untagged, or have a VLAN tag with a VLAN ID = 0 */
	GENAVB_SI_ALL		/**< A frame is recognized as belonging to the Stream whether tagged or not */
} genavb_si_vlan_tag_t;

/**
 * \ingroup stream_identification
 * Stream identification port position
 */
typedef enum {
	GENAVB_SI_PORT_POS_IN_FACING_INPUT,
	GENAVB_SI_PORT_POS_IN_FACING_OUTPUT,
	GENAVB_SI_PORT_POS_OUT_FACING_INPUT,
	GENAVB_SI_PORT_POS_OUT_FACING_OUTPUT
} genavb_si_port_pos_t;

/**
 * \ingroup stream_identification
 * Null stream identification type parameters
 * 802.1CB-2017, section 9.1.2
 */
struct genavb_si_null {
	uint8_t destination_mac[6];
	genavb_si_vlan_tag_t tagged;
	uint16_t vlan;
};

/**
 * \ingroup stream_identification
 * Source MAC and VLAN stream identification type parameters
 * 802.1CB-2017, section 9.1.3
 */
struct genavb_si_smac_vlan {
	uint8_t source_mac[6];
	genavb_si_vlan_tag_t tagged;
	uint16_t vlan;
};

/**
 * \ingroup stream_identification
 * Active Destination MAC and VLAN stream identification type parameters
 * 802.1CB-2017, section 9.1.4
 */
struct genavb_si_dmac_vlan {
	struct {
		uint8_t destination_mac[6];	/**< Specifies the destination address that identifies an input packet */
		genavb_si_vlan_tag_t tagged;
		uint16_t vlan;
		uint8_t priority;
	} down;
	struct {
		uint8_t destination_mac[6];	/**< This address replaces the address that was used to identify the packet */
		genavb_si_vlan_tag_t tagged;
		uint16_t vlan;
		uint8_t priority;
	} up;
};

/**
 * \ingroup stream_identification
 * Stream identification port
 */
struct genavb_si_port {
	unsigned int id;
	genavb_si_port_pos_t pos;
};

/**
 * \ingroup stream_identification
 * Stream identity table entry
 * 802.1CB-2017, section 9.1
 */
struct genavb_stream_identity {
	uint32_t handle;
	unsigned int port_n;
	struct genavb_si_port *port;
	genavb_si_t type;
	union {
		struct genavb_si_null null;
		struct genavb_si_smac_vlan smac_vlan;
		struct genavb_si_dmac_vlan dmac_vlan;
	} parameters;
};

/* OS specific headers */
#include "os/stream_identification.h"

#endif /* _GENAVB_PUBLIC_STREAM_IDENTIFICATION_H_ */
