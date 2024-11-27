/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file		srp.h
 @brief  	SRP protocol common definitions
 @details	PDU and protocol definitions for all SRP applications
*/


#ifndef _PROTO_SRP_H_
#define _PROTO_SRP_H_

#include "genavb/srp.h"
#include "genavb/sr_class.h"
#include "common/types.h"


/*
*
* MRP common header (802.1Qat -2010 - 10.8.1.2)
*
*/

struct __attribute__ ((packed)) mvrp_pdu_header {
	u8 attribute_type;		/**<  packet type such as (MSRP) Domain, Talker Advertise or (MVRP) VLAN, etc...*/
	u8 attribute_length;		/**<  length of the first value fields */
};

struct __attribute__ ((packed)) mrp_pdu_header {
	u8 attribute_type;		/**<  packet type such as (MSRP) Domain, Talker Advertise or (MVRP) VLAN, etc...*/
	u8 attribute_length;		/**<  length of the first value fields */
	u16 attribute_list_length;
};

/**
 * MRP default Timer values from 802.1Q-2011, Table 10-7
 */

#define MRP_JOINTIMER_VAL			200	/**< join timeout in msec */
#define MRP_JOINTIMER_POINT_TO_POINT_VAL	100	/**< maximum 3 transmit opportunities in 1.5xJoinTime */
#define MRP_LVATIMER_VAL_MIN			10000	/**< Min leaveall timeout in msec (LeaveAllTime per 802.1Q-2018 10.7.4.3) */
#define MRP_LVATIMER_VAL_MAX			15000	/**< Max leaveall timeout in msec (1.5 x LeaveAllTime per 802.1Q-2018 10.7.4.3) */
#define MRP_PERIODTIMER_VAL			1000	/**< periodic timeout in msec */

/*
*
* MSRP
*
*/


/**
* MSRP PDU Direction (802.1Qat 35.2.1.2)
*/
#define MSRP_DIRECTION_TALKER	0x00
#define MSRP_DIRECTION_LISTENER	0x01


/**
* MSRP PDU Protocol Version (802.1Qat 35.2.2.3)
*/
#define MSRP_PROTO_VERSION	0x00

/**
* MSRP PDU Attribute Type definitions (802.1Qat 35.2.2.4)
*/
typedef enum
{
	MSRP_ATTR_TYPE_TALKER_ADVERTISE = 1,
	MSRP_ATTR_TYPE_TALKER_FAILED,
	MSRP_ATTR_TYPE_LISTENER,
	MSRP_ATTR_TYPE_DOMAIN,
	MSRP_ATTR_TYPE_MAX
} msrp_attribute_type_t;


/**
* MSRP Declaration Type definitions (802.1Qat 35.2.1.3)
*/
typedef enum
{
	MSRP_TALKER_DECLARATION_TYPE_ADVERTISE = 0,
	MSRP_TALKER_DECLARATION_TYPE_FAILED,
	MSRP_TALKER_DECLARATION_TYPE_NONE = 0xffff
} msrp_talker_declaration_type_t;

typedef enum
{
	/* (802.1Qat 35.1.2.2) */
	MSRP_LISTENER_DECLARATION_TYPE_ASKING_FAILED = 1,	/**< none of the listeners can receive the stream due to bandwidth or network problems  */
	MSRP_LISTENER_DECLARATION_TYPE_READY, 			/**< there is enough bandwidth for all listeners to receive the stream */
	MSRP_LISTENER_DECLARATION_TYPE_READY_FAILED,		/**< at least enough bandwidth for one listener to receive the stream */
	MSRP_LISTENER_DECLARATION_TYPE_NONE = 0xffff
} msrp_listener_declaration_type_t;


/**
* MSRP PDU Attribute Length values (802.1Qat 35.2.2.5)
*/
#define MSRP_ATTR_LEN_TALKER_ADVERTISE	0x19 /* 25 */
#define MSRP_ATTR_LEN_TALKER_FAILED	0x22 /* 34 */
#define MSRP_ATTR_LEN_LISTENER		0x08 /* 8 */
#define MSRP_ATTR_LEN_DOMAIN		0x04 /* 4 */

/**
* MSRP PDU Four Packed Events values (802.1Qat 35.2.2.7)
*/
typedef enum
{
	MSRP_FOUR_PACKED_IGNORE = 0,
	MSRP_FOUR_PACKED_ASKING_FAILED,
	MSRP_FOUR_PACKED_READY,
	MSRP_FOUR_PACKED_READY_FAILED,
	MSRP_FOUR_PACKED_MAX
} msrp_four_packed_event_t;

struct msrp_data_frame {
	u8 destination_address[6];
	u16 vlan_identifier;
};

struct msrp_tspec {
	u16 max_frame_size;
	u16 max_interval_frames;
};

/**
* MSRP PDU Listener First Value definition (802.1Qat 35.2.2.8.1)
*/
struct __attribute__ ((packed)) msrp_pdu_fv_listener {
	u64 stream_id;
};


/**
* MSRP PDU Talker Advertise First Value definition (802.1Qat 35.2.2.8.1)
*/
struct __attribute__ ((packed)) msrp_pdu_fv_talker_advertise {
	u64 stream_id;
	struct msrp_data_frame data_frame;
	struct msrp_tspec tspec;
#ifdef __BIG_ENDIAN__
	u8 priority:3;
	u8 rank:1;
	u8 reserved:4;
#else
	u8 reserved:4;
	u8 rank:1;
	u8 priority:3;
#endif
	u32 accumulated_latency;
};


/**
* MSRP PDU Talker Advertise Failed First Value definition (802.1Qat 35.2.2.8.1)
*/
struct __attribute__ ((packed)) msrp_pdu_fv_talker_failed {
	u64 stream_id;
	struct msrp_data_frame data_frame;
	struct msrp_tspec tspec;
#ifdef __BIG_ENDIAN__
	u8 priority:3;
	u8 rank:1;
	u8 reserved:4;
#else
	u8 reserved:4;
	u8 rank:1;
	u8 priority:3;
#endif
	u32 accumulated_latency;
	struct msrp_failure_information failure_info;
};


/**
* MSRP PDU Domain Discovery definition (802.1Qat 35.2.2.9.1)
*/
struct __attribute__ ((packed)) msrp_pdu_fv_domain {
	u8 sr_class_id;
	u8 sr_class_priority;
	u16 sr_class_vid;
};


/*
*
* MVRP
*
*/

#define MVRP_PROTO_VERSION	0x00

/**
 * MVRP declaration type
 */
#define MVRP_ATTR_TYPE_VID	0x01

/**
 *MVRP attribute length
 *
 */
#define MVRP_ATTR_LEN		0x02

struct __attribute__ ((packed)) mvrp_pdu_fv {
	u16 vid;	/**<  vlan ID value */
};

/**
* MVRP PDU for the case where "Leave all" is not signaled.
*/
struct __attribute__ ((packed)) mvrp_pdu_no_leave_all {
	u8 attribute_type;	/**< declaration type, should always be 1 */
	u8 attribute_length;	/**< always 2 bytes for MVRP */
	u16 vector_header;	/**< vector = leaveall only, leaveall + vector, vector only, value= number of value always 1 for MVRP*/
	struct mvrp_pdu_fv fv;	/**< vlan ID value */
	u8 attribute_event;	/**< NEW | JOININ | IN | JOINMT | MT | LV*/
	u16 end_mark;		/**< always 0x0000 */
};

/**
* MVRP PDU for the case where "Leave all" is signaled.
*/
struct __attribute__ ((packed)) mvrp_pdu_leave_all {
	u8 attribute_type;
	u8 attribute_length;
	u16 vector_header;
	struct mvrp_pdu_fv fv;
	u16 end_mark;
};


/*
*
* MMRP
*
*/


/**
* MMRP PDU Protocol Version
*/
#define MMRP_PROTO_VERSION	0x00

uint64_t srp_tspec_to_idle_slope(unsigned int logical_port, sr_class_t sr_class, unsigned int max_frame_size, unsigned int max_interval_frames);

#endif /* _PROTO_SRP_H_ */
