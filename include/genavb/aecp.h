/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2019, 2021-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file aecp.h
 \brief GenAVB/TSN public API
 \details 1722.1 AECP format definition and helper functions.

 \copyright Copyright 2014-2016 Freescale Semiconductor, Inc.
 \copyright Copyright 2016-2017, 2019, 2021-2024 NXP
*/
#ifndef _GENAVB_PUBLIC_AECP_H_
#define _GENAVB_PUBLIC_AECP_H_

#include "types.h"


#define AVDECC_AECP_MAX_SIZE	(524 + sizeof(avb_u64))  /**< Max size in bytes of an AECP message, including entity id field (based on section 9.2.1.1.7 of 1722.1-2013) */


/**
 * \ingroup aem
 * From section 7.4.15 of IEEE 1722.1-2013.
 */
#define AECP_STREAM_FLAG_CLASS_B                       (1 << 0) /**< Indicates that the Stream is Class B instead of Class A (default 0 is class A). */
#define AECP_STREAM_FLAG_FAST_CONNECT                  (1 << 1) /**< Fast Connect Mode, the connection is being attempted in fast connect mode. */
#define AECP_STREAM_FLAG_SAVED_STATE                   (1 << 2) /**< Connection has saved state (used in Get State only). */
#define AECP_STREAM_FLAG_STREAMING_WAIT                (1 << 3) /**< The AVDECC Talker does not start streaming until explicitly being told to by the control protocol. */
#define AECP_STREAM_FLAG_SUPPORTS_ENCRYPTED            (1 << 4) /**< Indicates that the Stream supports streaming with encrypted PDUs. */
#define AECP_STREAM_FLAG_ENCRYPTED_PDU                 (1 << 5) /**< Indicates that the Stream is using encrypted PDUs. */
#define AECP_STREAM_FLAG_TALKER_FAILED                 (1 << 6) /**< Indicates that the listener has registered an SRP Talker Failed attribute for the Stream. */
#define AECP_STREAM_FLAG_STREAM_VLAN_ID_VALID          (1 << 25) /**< Indicates that the stream_vlan_id field is valid. */
#define AECP_STREAM_FLAG_CONNECTED                     (1 << 26) /**< The Stream has been connected with ACMP. This may only be set in a response. */
#define AECP_STREAM_FLAG_MSRP_FAILURE_VALID            (1 << 27) /**< he values in the msrp_failure_code and msrp_failure_bridge_id fields are valid. */
#define AECP_STREAM_FLAG_STREAM_DEST_MAC_VALID         (1 << 28) /**< The value in the stream_dest_mac field is valid.*/
#define AECP_STREAM_FLAG_MSRP_ACC_LAT_VALID            (1 << 29) /**< The value in the msrp_accumulated_latency field is valid*/
#define AECP_STREAM_FLAG_STREAM_ID_VALID               (1 << 30) /**< The value in the stream_id field is valid*/
#define AECP_STREAM_FLAG_STREAM_FORMAT_VALID           (1 << 31) /**< The value in stream_format field is valid and is to be used to change the Stream format if it is a SET_STREAM_INFO command. */

#define AECP_STREAM_FLAG_BOUND                         AECP_STREAM_FLAG_CONNECTED /**< AVNU.IO.CONTROL 7.3.10: Milan use: Rename AECP_STREAM_FLAG_CONNECTED to AECP_STREAM_FLAG_BOUND. */
/** AVNU.IO.CONTROL 7.3.10: Milan use: Rename AECP_STREAM_FLAG_TALKER_FAILED to AECP_STREAM_FLAG_REGISTERING_FAILED
 * For a STREAM_INPUT:  indicates that the STREAM_INPUT is registering either a matching Talker Advertise or a matching Talker Failed attribute.
 * For a STREAM_OUTPUT: indicates that the STREAM_OUTPUT is declaring a Talker Advertise or a Talker Failed attribute and registering a matching Listener attribute (Listener Ready,
 *                      Listener Ready Failed or Listener Asking Failed).
 */
#define AECP_STREAM_FLAG_REGISTERING_FAILED            AECP_STREAM_FLAG_TALKER_FAILED

#define AECP_STREAM_FLAG_EX_REGISTERING            (1 << 0) /**< Extended flag for Milan 7.3.10 */

/**
 * \ingroup aem
 * From section 9.2.1 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_pdu {
	avb_u64 entity_id;			/**< ID of the entity targeted by (if the message is a command), or sending (if the message is a response) the message. */
	avb_u64 controller_entity_id;		/**< ID of the controller sending (if the message is a command), or receiving (if the message is a response) the message. */
	avb_u16 sequence_id;			/**< Sequence ID of the message, determined by the controller that sent the command (if any). */
};

/**
 * \ingroup aem
 * From section 9.2.1.2 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_pdu {
	avb_u64 entity_id;			/**< See ::aecp_pdu. */
	avb_u64 controller_entity_id;		/**< See ::aecp_pdu. */
	avb_u16 sequence_id;			/**< See ::aecp_pdu. */
	avb_u16 u_command_type;			/**< Bit field of U bit (1 for unsolicited responses, 0 in all other cases) and ::aecp_aem_command_type_t. */
};

/**
 * \ingroup aem
 * From section 9.2.1.5 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_vuf_pdu {
	avb_u64 entity_id;			/**< See ::aecp_pdu. */
	avb_u64 controller_entity_id;		/**< See ::aecp_pdu. */
	avb_u16 sequence_id;			/**< See ::aecp_pdu. */
	avb_u8 protocol_id[6];			/**< 48 Bits Vendor protocol ID. */
};

#define MILAN_VENDOR_UNIQUE_PROTOCOL_ID		{0x00, 0x1B, 0xC5, 0x0A, 0xC1, 0x00}	/* AVNU.IO.CONTROL 7.2.2.1 */

/**
 * \ingroup aem
 * From section 7.2.2 of AVNU.IO.CONTROL
 */
struct __attribute__ ((packed)) aecp_mvu_pdu {
	avb_u64 entity_id;			/**< See ::aecp_pdu. */
	avb_u64 controller_entity_id;		/**< See ::aecp_pdu. */
	avb_u16 sequence_id;			/**< See ::aecp_pdu. */
	avb_u8 protocol_id[6];			/**< See ::aecp_vuf_pdu. */
	avb_u16 r_command_type;			/**< Bit field of R bit (reserved to 0) and ::aecp_mvu_command_type_t. */
};

/**
 * \ingroup aem
 * Extracts the command type from an AECP AEM PDU.
 * \return 	command_type from ::aecp_aem_command_type_t.
 * \param hdr	pointer to struct ::aecp_aem_pdu.
 */
#define AECP_AEM_GET_CMD_TYPE(hdr) \
	(ntohs((hdr)->u_command_type) & 0x7FFF)

/**
 * \ingroup aem
 * Extracts the command type from an AECP MVU PDU.
 * \return 	command_type from ::aecp_mvu_command_type_t.
 * \param hdr	pointer to struct ::aecp_mvu_pdu.
 */
#define AECP_MVU_GET_CMD_TYPE(hdr) \
	(ntohs((hdr)->r_command_type) & 0x7FFF)

/**
 * \ingroup aem
 * Extracts the u bit from an AECP AEM PDU.
 * \return 	u bit (1 for unsolicited responses, 0 otherwise).
 * \param hdr	pointer to struct ::aecp_aem_pdu.
 */
#define AECP_AEM_GET_U(hdr) \
	((ntohs((hdr)->u_command_type) >> 15) & 0x1)

/**
 * \ingroup aem
 * Updates the u_command_type field in an aecp_aem_pdu structure.
 * \return 	none.
 * \param hdr	pointer to struct ::aecp_aem_pdu to be updated.
 * \param u	u bit value (1 for unsolicited responses, 0 otherwise).
 * \param ct	value of AECP AEM command_type (from ::aecp_aem_command_type_t).
 */
#define AECP_AEM_SET_U_CMD_TYPE(hdr, u, ct) \
	((hdr)->u_command_type = htons(((u & 0x1) << 15) | (ct & 0x7FFF)))

#define AECP_AEM_MAX_CONTROL_VALUES	1
#define AECP_AEM_NB_COUNTERS		32


/**
 * \ingroup aem
 * From section 7.4.5 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_read_desc_cmd_pdu {
	avb_u16 configuration_index;	/**< Configuration from which the descriptor is to be read. */
	avb_u16 reserved;
	avb_u16 descriptor_type;	/**< Type of the descriptor to be read. */
	avb_u16 descriptor_index;	/**< Index of the descriptor to be read. */
};

/**
 * \ingroup aem
 * From section 7.4.5 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_read_desc_rsp_pdu {
	avb_u16 configuration_index;	/**< Configuration from which the descriptor was read. */
	avb_u16 reserved;
};

/**
 * \ingroup aem
 * From section 7.4.1 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_acquire_entity_pdu {
	avb_u32 flags;			/**< Bitmask of ::AECP_AEM_ACQUIRE_PERSISTENT, ::AECP_AEM_ACQUIRE_RELEASE. */
	avb_u64 owner_id;		/**< Set to 0 for a command, and to the Entity ID of the AVDECC Controller which owns the AVDECC Entity for a response. */
	avb_u16 descriptor_type;	/**< Type of the descriptor being acquired. */
	avb_u16 descriptor_index;	/**< Index of the descriptor being acquired. */
};

/**
 * \ingroup aem
 * From section 7.4.2 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_lock_entity_pdu {
	avb_u32 flags;			/**< Bitmask of ::AECP_AEM_LOCK_UNLOCK. */
	avb_u64 locked_id;		/**< Set to 0 for a command, and to the Entity ID of the AVDECC Controller that is holding the lock for a response. */
	avb_u16 descriptor_type;	/**< Type of the descriptor being acquired. */
	avb_u16 descriptor_index;	/**< Index of the descriptor being acquired. */
};

/**
 * \ingroup aem
 * From section 7.4.7 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_set_configuration_pdu {
	avb_u16 reserved;
	avb_u16 configuration_index;		/**< Configuration from which the descriptor was read. */
};

/**
 * \ingroup aem
 * From section 7.4.8 of IEEE 1722.1-2013.
 */
#define aecp_aem_get_configuration_rsp_pdu	aecp_aem_set_configuration_pdu

/**
 * \ingroup aem
 * From section 7.4.9 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_set_stream_format_pdu {
	avb_u16 descriptor_type;	/**< Type of the descriptor to be read. */
	avb_u16 descriptor_index;	/**< Index of the descriptor to be read. */
	avb_u64 stream_format;
};

/**
 * \ingroup aem
 * From section 7.4.10 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_get_stream_format_cmd_pdu {
	avb_u16 descriptor_type;	/**< Type of the descriptor to be read. */
	avb_u16 descriptor_index;	/**< Index of the descriptor to be read. */
};

/**
 * \ingroup aem
 * From section 7.4.10 of IEEE 1722.1-2013.
 */
#define aecp_aem_get_stream_format_rsp_pdu	aecp_aem_set_stream_format_pdu

/**
 * \ingroup aem
 * From section 7.4.15 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_set_stream_info_pdu {
	avb_u16 descriptor_type;	/**< Type of the descriptor to be read. */
	avb_u16 descriptor_index;	/**< Index of the descriptor to be read. */
	avb_u32 flags;
	avb_u64 stream_format;
	avb_u64 stream_id;
	avb_u32 msrp_accumulated_latency;
	avb_u8 stream_dest_mac[6];
	avb_u8 msrp_failure_code;
	avb_u8 reserved_1;
	avb_u64 msrp_failure_bridge_id;
	avb_u16 stream_vlan_id;
	avb_u16 reserved_2;
};

/**
 * \ingroup aem
 * From section 7.4.16 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_get_stream_info_cmd_pdu {
	avb_u16 descriptor_type;	/**< Type of the descriptor to be read. */
	avb_u16 descriptor_index;	/**< Index of the descriptor to be read. */
};

/**
 * \ingroup aem
 * From section 7.4.16 of IEEE 1722.1-2013.
 */
#define aecp_aem_get_stream_info_rsp_pdu	aecp_aem_set_stream_info_pdu

/**
 * \ingroup aem
 * From section 7.3.10 of AVNU.IO.CONTROL
 */

struct __attribute__ ((packed)) aecp_aem_milan_get_stream_info_rsp_pdu {
	avb_u16 descriptor_type;	/**< Type of the descriptor to be read. */
	avb_u16 descriptor_index;	/**< Index of the descriptor to be read. */
	avb_u32 flags;
	avb_u64 stream_format;
	avb_u64 stream_id;
	avb_u32 msrp_accumulated_latency;
	avb_u8 stream_dest_mac[6];
	avb_u8 msrp_failure_code;
	avb_u8 reserved_1;
	avb_u64 msrp_failure_bridge_id;
	avb_u16 stream_vlan_id;
	avb_u16 reserved_2;
	avb_u32 flags_ex;
#ifdef __BIG_ENDIAN__
	avb_u32 pbsta:3;
	avb_u32 acmpsta:5;
	avb_u32 reserved_3:24;
#else
	avb_u32 acmpsta:5;
	avb_u32 pbsta:3;
	avb_u32 reserved_3:24;
#endif
};

/**
 * \ingroup aem
 * From section 7.4.17 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_set_name_pdu {
	avb_u16 descriptor_type;	/**< Type of the descriptor to be read. */
	avb_u16 descriptor_index;	/**< Index of the descriptor to be read. */
	avb_u16 name_index;	/**< Index of the name being set. */
	avb_u16 configuration_index;	/**< Index of the configuration containing the descriptor to be read. */
	avb_u8 name[64];
};

/**
 * \ingroup aem
 * From section 7.4.18 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_get_name_cmd_pdu {
	avb_u16 descriptor_type;	/**< Type of the descriptor to be read. */
	avb_u16 descriptor_index;	/**< Index of the descriptor to be read. */
	avb_u16 name_index;	/**< Index of the name to be read. */
	avb_u16 configuration_index;	/**< Index of the configuration containing the descriptor to be read. */
};

/**
 * \ingroup aem
 * From section 7.4.18 of IEEE 1722.1-2013.
 */
#define aecp_aem_get_name_rsp_pdu	aecp_aem_set_name_pdu

/**
 * \ingroup aem
 * From section 7.4.21 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_set_sampling_rate_pdu {
	avb_u16 descriptor_type;	/**< Type of the descriptor to be read. */
	avb_u16 descriptor_index;	/**< Index of the descriptor to be read. */
	avb_u32 sampling_rate;
};

/**
 * \ingroup aem
 * From section 7.4.22 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_get_sampling_rate_cmd_pdu {
	avb_u16 descriptor_type;	/**< Type of the descriptor to be read. */
	avb_u16 descriptor_index;	/**< Index of the descriptor to be read. */
};

/**
 * \ingroup aem
 * From section 7.4.22 of IEEE 1722.1-2013.
 */
#define aecp_aem_get_sampling_rate_rsp_pdu	aecp_aem_set_sampling_rate_pdu

/**
 * \ingroup aem
 * From section 7.4.23 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_set_clock_source_pdu {
	avb_u16 descriptor_type;	/**< Type of the descriptor to be read. */
	avb_u16 descriptor_index;	/**< Index of the descriptor to be read. */
	avb_u16 clock_source_index;
	avb_u16 reserved;
};

/**
 * \ingroup aem
 * From section 7.4.24 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_get_clock_source_cmd_pdu {
	avb_u16 descriptor_type;	/**< Type of the descriptor to be read. */
	avb_u16 descriptor_index;	/**< Index of the descriptor to be read. */
};

/**
 * \ingroup aem
 * From section 7.4.24 of IEEE 1722.1-2013.
 */
#define aecp_aem_get_clock_source_rsp_pdu	aecp_aem_set_clock_source_pdu

/**
 * \ingroup aem
 * From section 7.4.35 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_start_streaming_cmd_pdu {
	avb_u16 descriptor_type;	/**< Type of the descriptor to be read. */
	avb_u16 descriptor_index;	/**< Index of the descriptor to be read. */
};

/**
 * \ingroup aem
 * From section 7.4.36 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_stop_streaming_cmd_pdu {
	avb_u16 descriptor_type;	/**< Type of the descriptor to be read. */
	avb_u16 descriptor_index;	/**< Index of the descriptor to be read. */
};

/**
 * \ingroup aem
 * From section 7.4.40 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_get_avb_info_cmd_pdu {
	avb_u16 descriptor_type;	/**< Type of the descriptor to be read. */
	avb_u16 descriptor_index;	/**< Index of the descriptor to be read. */
};

/**
 * \ingroup aem
 * From section 7.4.40 of IEEE 1722.1-2013.
 */

#define AECP_AEM_AVB_INFO_AS_CAPABLE		(1 << 0) /**< The IEEE Std 802.1AS-2011 variable asCapable is set on this interface. */
#define AECP_AEM_AVB_INFO_GPTP_ENABLED		(1 << 1) /**< Indicates that the interface has the IEEE Std 802.1AS-2011 functionality enabled. */
#define AECP_AEM_AVB_INFO_SRP_ENABLED		(1 << 2) /**< Indicates that the interface has the IEEE Std 802.1Q-2011 Clause 35, “Stream Reservation Protocol (SRP)” functionality enabled.*/

struct __attribute__ ((packed)) aecp_aem_get_avb_info_rsp_pdu {
	avb_u16 descriptor_type;	/**< Type of the descriptor to be read. */
	avb_u16 descriptor_index;	/**< Index of the descriptor to be read. */
	avb_u64 gptp_grandmaster_id;	/**< ClockIdentity of the current IEEE Std 802.1AS-2011 grandmaster as elected on this AVB Interface. */
	avb_u32 propagation_delay;	/**< The propagation delay in nanoseconds as reported by the IEEE Std 802.1AS-2011 pDelay mechanism. */
	avb_u8 gptp_domain_number;	/**< The domainNumber of the current IEEE Std 802.1AS-2011 grandmaster as elected on this AVB Interface. */
	avb_u8 flags;			/**< Bitmask of ::AECP_AEM_AVB_INFO_AS_CAPABLE, ::AECP_AEM_AVB_INFO_GPTP_ENABLED, ::AECP_AEM_AVB_INFO_SRP_ENABLED. */
	avb_u16 msrp_mappings_count;	/**< The number of mappings ::aecp_aem_get_avb_info_msrp_mappings_format present in the msrp_mappings field. */
};

/**
 * \ingroup aem
 * From section 7.4.40 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_get_avb_info_msrp_mappings_format {
	avb_u8 traffic_class;		/**< The traffic class. */
	avb_u8 priority;		/**< The priority for this traffic class. */
	avb_u16 vlan_id;		/**< The VLAN ID for this traffic class. */
};

/**
 * \ingroup aem
 * From section 7.4.41 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_get_as_path_cmd_pdu {
	avb_u16 descriptor_index;	/**< Index of the AVB Interface descriptor to be read. */
	avb_u16 reserved;
};

/**
 * \ingroup aem
 * From section 7.4.41 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_get_as_path_rsp_pdu {
	avb_u16 descriptor_index;	/**< Index of the AVB Interface descriptor to be read. */
	avb_u16 count;
	/* path sequence as a variable length field. */
};

/**
 * \ingroup aem
 * From section 7.4.1 of AVNU.IO.CONTROL
 */
struct __attribute__ ((packed)) aecp_mvu_get_milan_info_cmd_pdu {
	avb_u16 reserved;
};

/**
 * \ingroup aem
 * From section 7.4.1 of AVNU.IO.CONTROL
 */
struct __attribute__ ((packed)) aecp_mvu_get_milan_info_rsp_pdu {
	avb_u16 reserved;
	avb_u32 protocol_version;
	avb_u32 features_flags;
	avb_u32 certification_version;
};

/**
 * \ingroup aem
 * Acquire the AVDECC Entity and disable the CONTROLLER_AVAILABLE test for future ACQUIRE_ENTITY commands until released.
 * The AVDECC Entity returns an ENTITY_ACQUIRED response immediately to any other Controller.
 */
#define AECP_AEM_ACQUIRE_PERSISTENT	(1 << 0)
/**
 * \ingroup aem
 * Release the AVDECC Entity from the acquisition.
 */
#define AECP_AEM_ACQUIRE_RELEASE	(1 << 31)
/**
 * \ingroup aem
 * Unlock the AVDECC Entity.
 */
#define AECP_AEM_LOCK_UNLOCK		(1 << 0)


/**
 * \ingroup aem
 * From section 7.4.25/26 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_set_get_control_pdu {
	avb_u16 descriptor_type;	/**< Set to AEM_DESC_TYPE_CONTROL. */
	avb_u16 descriptor_index;	/**< Index of the Control for which the current values are being set. */
};

/**
 * \ingroup aem
 * From section 7.4.42 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_get_counters_cmd_pdu {
	avb_u16 descriptor_type;	/**< ENTITY, AVB_INTERFACE, CLOCK_DOMAIN or STREAM_INPUT */
	avb_u16 descriptor_index;	/**< Index of the descriptor whose counters are to be fetched */
};

#define AECP_AEM_COUNTER_ENTITY_ENTITY_SPECIFIC_8		24
#define AECP_AEM_COUNTER_ENTITY_ENTITY_SPECIFIC_7		25
#define AECP_AEM_COUNTER_ENTITY_ENTITY_SPECIFIC_6		26
#define AECP_AEM_COUNTER_ENTITY_ENTITY_SPECIFIC_5		27
#define AECP_AEM_COUNTER_ENTITY_ENTITY_SPECIFIC_4		28
#define AECP_AEM_COUNTER_ENTITY_ENTITY_SPECIFIC_3		29
#define AECP_AEM_COUNTER_ENTITY_ENTITY_SPECIFIC_2		30
#define AECP_AEM_COUNTER_ENTITY_ENTITY_SPECIFIC_1		31

#define AECP_AEM_COUNTER_AVB_INTERFACE_LINK_UP			0
#define AECP_AEM_COUNTER_AVB_INTERFACE_LINK_DOWN		1
#define AECP_AEM_COUNTER_AVB_INTERFACE_FRAMES_TX		2
#define AECP_AEM_COUNTER_AVB_INTERFACE_FRAMES_RX		3
#define AECP_AEM_COUNTER_AVB_INTERFACE_RX_CRC_ERROR		4
#define AECP_AEM_COUNTER_AVB_INTERFACE_GPTP_GM_CHANGED		5
#define AECP_AEM_COUNTER_AVB_INTERFACE_ENTITY_SPECIFIC_8	24
#define AECP_AEM_COUNTER_AVB_INTERFACE_ENTITY_SPECIFIC_7	25
#define AECP_AEM_COUNTER_AVB_INTERFACE_ENTITY_SPECIFIC_6	26
#define AECP_AEM_COUNTER_AVB_INTERFACE_ENTITY_SPECIFIC_5	27
#define AECP_AEM_COUNTER_AVB_INTERFACE_ENTITY_SPECIFIC_4	28
#define AECP_AEM_COUNTER_AVB_INTERFACE_ENTITY_SPECIFIC_3	29
#define AECP_AEM_COUNTER_AVB_INTERFACE_ENTITY_SPECIFIC_2	30
#define AECP_AEM_COUNTER_AVB_INTERFACE_ENTITY_SPECIFIC_1	31

#define AECP_AEM_COUNTER_CLOCK_DOMAIN_LOCKED			0
#define AECP_AEM_COUNTER_CLOCK_DOMAIN_UNLOCKED			1
#define AECP_AEM_COUNTER_CLOCK_DOMAIN_ENTITY_SPECIFIC_1		24
#define AECP_AEM_COUNTER_CLOCK_DOMAIN_ENTITY_SPECIFIC_2		25
#define AECP_AEM_COUNTER_CLOCK_DOMAIN_ENTITY_SPECIFIC_3		26
#define AECP_AEM_COUNTER_CLOCK_DOMAIN_ENTITY_SPECIFIC_4		27
#define AECP_AEM_COUNTER_CLOCK_DOMAIN_ENTITY_SPECIFIC_5		28
#define AECP_AEM_COUNTER_CLOCK_DOMAIN_ENTITY_SPECIFIC_6		29
#define AECP_AEM_COUNTER_CLOCK_DOMAIN_ENTITY_SPECIFIC_7		30
#define AECP_AEM_COUNTER_CLOCK_DOMAIN_ENTITY_SPECIFIC_8		31

#define AECP_AEM_COUNTER_STREAM_INPUT_MEDIA_LOCKED		0
#define AECP_AEM_COUNTER_STREAM_INPUT_MEDIA_UNLOCKED		1
#define AECP_AEM_COUNTER_STREAM_INPUT_STREAM_RESET		2
#define AECP_AEM_COUNTER_STREAM_INPUT_SEQ_NUM_MISMATCH		3
#define AECP_AEM_COUNTER_STREAM_INPUT_MEDIA_RESET		4
#define AECP_AEM_COUNTER_STREAM_INPUT_TIMESTAMP_UNCERTAIN	5
#define AECP_AEM_COUNTER_STREAM_INPUT_TIMESTAMP_VALID		6
#define AECP_AEM_COUNTER_STREAM_INPUT_TIMESTAMP_NOT_VALID	7
#define AECP_AEM_COUNTER_STREAM_INPUT_UNSUPPORTED_FORMAT	8
#define AECP_AEM_COUNTER_STREAM_INPUT_LATE_TIMESTAMP		9
#define AECP_AEM_COUNTER_STREAM_INPUT_EARLY_TIMESTAMP		10
#define AECP_AEM_COUNTER_STREAM_INPUT_FRAMES_RX			11
#define AECP_AEM_COUNTER_STREAM_INPUT_FRAMES_TX			12
#define AECP_AEM_COUNTER_STREAM_INPUT_ENTITY_SPECIFIC_8		24
#define AECP_AEM_COUNTER_STREAM_INPUT_ENTITY_SPECIFIC_7		25
#define AECP_AEM_COUNTER_STREAM_INPUT_ENTITY_SPECIFIC_6		26
#define AECP_AEM_COUNTER_STREAM_INPUT_ENTITY_SPECIFIC_5		27
#define AECP_AEM_COUNTER_STREAM_INPUT_ENTITY_SPECIFIC_4		28
#define AECP_AEM_COUNTER_STREAM_INPUT_ENTITY_SPECIFIC_3		29
#define AECP_AEM_COUNTER_STREAM_INPUT_ENTITY_SPECIFIC_2		30
#define AECP_AEM_COUNTER_STREAM_INPUT_ENTITY_SPECIFIC_1		31

#define AECP_AEM_COUNTER_STREAM_OUTPUT_STREAM_START		0
#define AECP_AEM_COUNTER_STREAM_OUTPUT_STREAM_STOP		1
#define AECP_AEM_COUNTER_STREAM_OUTPUT_MEDIA_RESET		2
#define AECP_AEM_COUNTER_STREAM_OUTPUT_TIMESTAMP_UNCERTAIN	3
#define AECP_AEM_COUNTER_STREAM_OUTPUT_FRAMES_TX		4
/**
 * \ingroup aem
 * From section 7.4.42 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_get_counters_rsp_pdu {
	avb_u16 descriptor_type;	/**< ENTITY, AVB_INTERFACE, CLOCK_DOMAIN or STREAM_INPUT */
	avb_u16 descriptor_index;	/**< Index of the descriptor whose counters are fetched */
	avb_u32 counters_valid;
	avb_u32 counters_block[AECP_AEM_NB_COUNTERS];
};

/**
 * \ingroup aem
 * From section 7.4.44 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_get_audio_map_cmd_pdu {
	avb_u16 descriptor_type;	/**< Type of the descriptor to be read. */
	avb_u16 descriptor_index;	/**< Index of the descriptor to be read. */
	avb_u16 map_index; /**< Index of the map subset to fetch. */
	avb_u16 reserved;
};

/**
 * \ingroup aem
 * From section 7.4.44 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_get_audio_map_rsp_pdu {
	avb_u16 descriptor_type;	/**< Type of the descriptor to be read. */
	avb_u16 descriptor_index;	/**< Index of the descriptor to be read. */
	avb_u16 map_index; /**< Index of the map subset to fetch. */
	avb_u16 number_of_maps;
	avb_u16 number_of_mappings;
	avb_u16 reserved;
};

/**
 * \ingroup aem
 * From section 7.4.44 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_get_audio_map_mappings_format {
	avb_u16 mapping_stream_index;		/**< The stream index. */
	avb_u16 mapping_stream_channel;		/**< The stream channel. */
	avb_u16 mapping_cluster_offset;		/**< The offset from the base_cluster of the AUDIO_PORT_INPUT or AUDIO_PORT_OUTPUT . */
	avb_u16 mapping_cluster_channel;		/**< The channel within the cluster. */
};

/**
 * \ingroup aem
 * From section 7.4.45 of IEEE 1722.1-2013.
 */
struct __attribute__ ((packed)) aecp_aem_add_audio_mappings_cmd_pdu {
	avb_u16 descriptor_type;	/**< Type of the descriptor to be read. */
	avb_u16 descriptor_index;	/**< Index of the descriptor to be read. */
	avb_u16 number_of_mappings;
	avb_u16 reserved;
};

/**
 * \ingroup aem
 * From section 7.4.46 of IEEE 1722.1-2013.
 */
#define aecp_aem_remove_audio_mappings_cmd_pdu aecp_aem_add_audio_mappings_cmd_pdu
#define aecp_aem_modify_audio_mappings_cmd_pdu aecp_aem_add_audio_mappings_cmd_pdu

/**
 * \ingroup aem
 * AECP message types, from table 9.1 of IEEE 1722.1-2013.
 */
typedef enum {
	AECP_AEM_COMMAND		= 0x00,		/**< AECP message type, AEM command. */
	AECP_AEM_RESPONSE		= 0x01,		/**< AECP message type, AEM response. */
	AECP_ADDRESS_ACCESS_COMMAND	= 0x02,
	AECP_ADDRESS_ACCESS_RESPONSE	= 0x03,
	AECP_AVC_COMMAND		= 0x04,
	AECP_AVC_RESPONSE		= 0x05,
	AECP_VENDOR_UNIQUE_COMMAND	= 0x06,
	AECP_VENDOR_UNIQUE_RESPONSE	= 0x07,
	AECP_HDCP_APM_COMMAND		= 0x08,
	AECP_HDCP_APM_RESPONSE		= 0x09,
	AECP_EXTENDED_COMMAND		= 0x0E,
	AECP_EXTENDED_RESPONSE		= 0x0F
} aecp_message_type_t;

/**
 * \ingroup aem
 * AECP AEM commands, from table 7.125 of IEEE 1722.1-2013.
 */
typedef enum {
	AECP_AEM_CMD_ACQUIRE_ENTITY			= 0x0000,	/**< ACQUIRE_ENTITY command/response */
	AECP_AEM_CMD_LOCK_ENTITY			= 0x0001,
	AECP_AEM_CMD_ENTITY_AVAILABLE			= 0x0002,
	AECP_AEM_CMD_CONTROLLER_AVAILABLE		= 0x0003,
	AECP_AEM_CMD_READ_DESCRIPTOR			= 0x0004,	/**< READ_DESCRIPTOR command/response */
	AECP_AEM_CMD_WRITE_DESCRIPTOR			= 0x0005,
	AECP_AEM_CMD_SET_CONFIGURATION			= 0x0006,
	AECP_AEM_CMD_GET_CONFIGURATION			= 0x0007,
	AECP_AEM_CMD_SET_STREAM_FORMAT			= 0x0008,
	AECP_AEM_CMD_GET_STREAM_FORMAT			= 0x0009,
	AECP_AEM_CMD_SET_VIDEO_FORMAT			= 0x000a,
	AECP_AEM_CMD_GET_VIDEO_FORMAT			= 0x000b,
	AECP_AEM_CMD_SET_SENSOR_FORMAT			= 0x000c,
	AECP_AEM_CMD_GET_SENSOR_FORMAT			= 0x000d,
	AECP_AEM_CMD_SET_STREAM_INFO			= 0x000e,
	AECP_AEM_CMD_GET_STREAM_INFO			= 0x000f,
	AECP_AEM_CMD_SET_NAME				= 0x0010,
	AECP_AEM_CMD_GET_NAME				= 0x0011,
	AECP_AEM_CMD_SET_ASSOCIATION_ID			= 0x0012,
	AECP_AEM_CMD_GET_ASSOCIATION_ID			= 0x0013,
	AECP_AEM_CMD_SET_SAMPLING_RATE			= 0x0014,
	AECP_AEM_CMD_GET_SAMPLING_RATE			= 0x0015,
	AECP_AEM_CMD_SET_CLOCK_SOURCE			= 0x0016,
	AECP_AEM_CMD_GET_CLOCK_SOURCE			= 0x0017,
	AECP_AEM_CMD_SET_CONTROL			= 0x0018,	/**< SET_CONTROL command/response */
	AECP_AEM_CMD_GET_CONTROL			= 0x0019,	/**< GET_CONTROL command/response */
	AECP_AEM_CMD_INCREMENT_CONTROL			= 0x001a,
	AECP_AEM_CMD_DECREMENT_CONTROL			= 0x001b,
	AECP_AEM_CMD_SET_SIGNAL_SELECTOR		= 0x001c,
	AECP_AEM_CMD_GET_SIGNAL_SELECTOR		= 0x001d,
	AECP_AEM_CMD_SET_MIXER				= 0x001e,
	AECP_AEM_CMD_GET_MIXER				= 0x001f,
	AECP_AEM_CMD_SET_MATRIX				= 0x0020,
	AECP_AEM_CMD_GET_MATRIX				= 0x0021,
	AECP_AEM_CMD_START_STREAMING			= 0x0022,	/**< START_STREAMING command/response */
	AECP_AEM_CMD_STOP_STREAMING			= 0x0023,	/**< STOP_STREAMING command/response */
	AECP_AEM_CMD_REGISTER_UNSOLICITED_NOTIFICATION	= 0x0024,	/**< REGISTER_UNSOLICITED_NOTIFICATION command/response */
	AECP_AEM_CMD_DEREGISTER_UNSOLICITED_NOTIFICATION = 0x0025,	/**< DEREGISTER_UNSOLICITED_NOTIFICATION command/response */
	AECP_AEM_CMD_IDENTIFY_NOTIFICATION		= 0x0026,
	AECP_AEM_CMD_GET_AVB_INFO			= 0x0027,
	AECP_AEM_CMD_GET_AS_PATH			= 0x0028,
	AECP_AEM_CMD_GET_COUNTERS			= 0x0029,
	AECP_AEM_CMD_REBOOT				= 0x002a,
	AECP_AEM_CMD_GET_AUDIO_MAP			= 0x002b,
	AECP_AEM_CMD_ADD_AUDIO_MAPPINGS			= 0x002c,
	AECP_AEM_CMD_REMOVE_AUDIO_MAPPINGS		= 0x002d,
	AECP_AEM_CMD_GET_VIDEO_MAP			= 0x002e,
	AECP_AEM_CMD_ADD_VIDEO_MAPPINGS			= 0x002f,
	AECP_AEM_CMD_REMOVE_VIDEO_MAPPINGS		= 0x0030,
	AECP_AEM_CMD_GET_SENSOR_MAP			= 0x0031,
	AECP_AEM_CMD_ADD_SENSOR_MAPPINGS		= 0x0032,
	AECP_AEM_CMD_REMOVE_SENSOR_MAPPINGS		= 0x0033,
	AECP_AEM_CMD_START_OPERATION			= 0x0034,
	AECP_AEM_CMD_ABORT_OPERATION			= 0x0035,
	AECP_AEM_CMD_OPERATION_STATUS			= 0x0036,
	AECP_AEM_CMD_AUTH_ADD_KEY			= 0x0037,
	AECP_AEM_CMD_AUTH_DELETE_KEY			= 0x0038,
	AECP_AEM_CMD_AUTH_GET_KEY_LIST			= 0x0039,
	AECP_AEM_CMD_AUTH_GET_KEY			= 0x003a,
	AECP_AEM_CMD_AUTH_ADD_KEY_TO_CHAIN		= 0x003b,
	AECP_AEM_CMD_AUTH_DELETE_KEY_FROM_CHAIN		= 0x003c,
	AECP_AEM_CMD_AUTH_GET_KEYCHAIN_LIST		= 0x003d,
	AECP_AEM_CMD_AUTH_GET_IDENTITY			= 0x003e,
	AECP_AEM_CMD_AUTH_ADD_TOKEN			= 0x003f,
	AECP_AEM_CMD_AUTH_DELETE_TOKEN			= 0x0040,
	AECP_AEM_CMD_AUTHENTICATE			= 0x0041,
	AECP_AEM_CMD_DEAUTHENTICATE			= 0x0042,
	AECP_AEM_CMD_ENABLE_TRANSPORT_SECURITY		= 0x0043,
	AECP_AEM_CMD_DISABLE_TRANSPORT_SECURITY		= 0x0044,
	AECP_AEM_CMD_ENABLE_STREAM_ENCRYPTION		= 0x0045,
	AECP_AEM_CMD_DISABLE_STREAM_ENCRYPTION		= 0x0046,
	AECP_AEM_CMD_SET_MEMORY_OBJECT_LENGTH		= 0x0047,
	AECP_AEM_CMD_GET_MEMORY_OBJECT_LENGTH		= 0x0048,
	AECP_AEM_CMD_SET_STREAM_BACKUP			= 0x0049,
	AECP_AEM_CMD_GET_STREAM_BACKUP			= 0x004a,
	AECP_AEM_CMD_EXPANSION				= 0x7fff
} aecp_aem_command_type_t;

/**
 * \ingroup aem
 * AECP MVU commands, from table 7.2.2.3 of AVNU.IO.CONTROL
 */
typedef enum {
	AECP_MVU_CMD_GET_MILAN_INFO			= 0x0000
} aecp_mvu_command_type_t;

/**
 * \ingroup aem
 * AECP and AECP AEM status codes, from section 9.2.11.6 and table 7.126 of IEEE 1722.1-2013.
 */
typedef enum {
	AECP_SUCCESS			= 0,  /**< The AVDECC Entity successfully performed the command and has valid results. */
	AECP_AEM_SUCCESS		= 0,  /**< The AVDECC Entity successfully performed the command and has valid results. */
	AECP_NOT_IMPLEMENTED		= 1,  /**< The AVDECC Entity does not support the command type. */
	AECP_AEM_NOT_IMPLEMENTED	= 1,  /**< The AVDECC Entity does not support the command type. */
	AECP_AEM_NO_SUCH_DESCRIPTOR	= 2,  /**< A descriptor with the descriptor_type and descriptor_index specified does not exist. */
	AECP_AEM_ENTITY_LOCKED		= 3,  /**< The AVDECC Entity has been locked by another AVDECC Controller. */
	AECP_AEM_ENTITY_ACQUIRED	= 4,  /**< The AVDECC Entity has been acquired by another AVDECC Controller. */
	AECP_AEM_NOT_AUTHENTICATED	= 5,  /**< The AVDECC Controller is not authenticated with the AVDECC Entity. */
	AECP_AEM_AUTHENTICATION_DISABLED = 6,  /**< The AVDECC Controller is trying to use an authentication command when authentication isn’t enable on the AVDECC Entity. */
	AECP_AEM_BAD_ARGUMENTS		= 7,  /**< One or more of the values in the fields of the frame were deemed to be bad by the AVDECC Entity (unsupported, incorrect combination, etc.). */
	AECP_AEM_NO_RESOURCES		= 8,  /**< The AVDECC Entity cannot complete the command because it does not have the resources to support it. */
	AECP_AEM_IN_PROGRESS		= 9,  /**< The AVDECC Entity is processing the command and will send a second response at a later time with the result of the command. */
	AECP_AEM_ENTITY_MISBEHAVING	= 10, /**< The AVDECC Entity is generated an internal error while trying to process the command. */
	AECP_AEM_NOT_SUPPORTED		= 11, /**< The command is implemented but the target of the command is not supported. For example trying to set the value of a read-only Control. */
	AECP_AEM_STREAM_IS_RUNNING	= 12, /**< The Stream is currently streaming and the command is one which cannot be executed on an Active Stream. */
	AECP_AEM_TIMEOUT		= 13 /**< Added for communication with GenAVB/TSN controller application, to signal a timeout waiting for a response from the target entity */
} aecp_status_t;

#endif /* _GENAVB_PUBLIC_AECP_H_ */
