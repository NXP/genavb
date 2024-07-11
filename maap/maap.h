/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2021-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief MAAP main header file
 @details Definition of MAAP stack component entry point functions and global context structure.
*/

#ifndef _MAAP_H_
#define _MAAP_H_

#include "common/net.h"
#include "common/ipc.h"
#include "common/timer.h"

#define MAAP_VERSION	1

#define MAC_VALUE(addr) ((u64)((u64)addr[5] | (u64)addr[4] << 8 | (u64)addr[3] << 16 | (u64)addr[2] << 24 | (u64)addr[1] << 32 | (u64)addr[0] << 40))
#define REVERSE_MAC_VALUE(addr) ((u64)((u64)addr[0] | (u64)addr[1] << 8 | (u64)addr[2] << 16 | (u64)addr[3] << 24 | (u64)addr[4] << 32 | (u64)addr[5] << 40))

/**
 * MAAP Reserved MAC addresses from IEEE Std 1722-2016, Table B.9 and B.10
 */
#define MAAP_MULTICAST_ADDR {0x91, 0xE0, 0xF0, 0x00, 0xFF, 0x00}
#define MAAP_DYNAMIC_POOL_MIN {0x91, 0xE0, 0xF0, 0x00, 0x00, 0x00}
#define MAAP_DYNAMIC_POOL_MAX {0x91, 0xE0, 0xF0, 0x00, 0xFD, 0xFF}
#define MAAP_LOCAL_POOL_MIN {0x91, 0xE0, 0xF0, 0x00, 0xFE, 0x00}
#define MAAP_LOCAL_POOL_MAX {0x91, 0xE0, 0xF0, 0x00, 0xFE, 0xFF}
#define MAAP_RESERVED_POOL_MIN {0x91, 0xE0, 0xF0, 0x00, 0xFF, 0x01}
#define MAAP_RESERVED_POOL_MAX {0x91, 0xE0, 0xF0, 0x00, 0xFF, 0xFF}

#define MAAP_DYNAMIC_POOL_SIZE (0xFDFF - 0x0000 + 1)
#define MAAP_PORT_MAX_RANGES_ALLOCATION 128

/**
 * MAAP probe constant values from IEEE Std 1722-2016, Table B.8
 */
#define MAAP_NUMBER_OF_TIMERS		2 /* Probe timer and Announce timer */
#define MAAP_SOFTWARE_TIMER_JITTER		10 /* Software jitter 10ms */
#define MAAP_PROBE_RETRANSMITS_VAL		3 /* Number of probe to send before assuming there is no conflict */

#define MAAP_PROBE_INTERVAL_VARIATION		100 /* Variations in the probe messages interval in msec */
#define MAAP_PROBE_TIMER_GRANULARITY		20 /* Software timer granularity */
/* Interval between probe messages in msec (500ms to 600ms) */
#define MAAP_PROBE_INTERVAL_MIN		500
#define MAAP_PROBE_INTERVAL_MAX		(MAAP_PROBE_INTERVAL_MIN + MAAP_PROBE_INTERVAL_VARIATION - MAAP_PROBE_TIMER_GRANULARITY - MAAP_SOFTWARE_TIMER_JITTER)


#define MAAP_ANNOUNCE_INTERVAL_VARIATION		2000 /* Variations in the announce messages interval in msec (2 sec) */
#define MAAP_ANNOUNCE_TIMER_GRANULARITY		100 /* Software timer granularity */
 /* Interval min between announce messages in msec (30 to 32 sec) */
#define MAAP_ANNOUNCE_INTERVAL_MIN		30000
#define MAAP_ANNOUNCE_INTERVAL_MAX		(MAAP_ANNOUNCE_INTERVAL_MIN + MAAP_ANNOUNCE_INTERVAL_VARIATION - MAAP_ANNOUNCE_TIMER_GRANULARITY - MAAP_SOFTWARE_TIMER_JITTER)

/**
 * MAAP message types from 1722-2016, Table B.1
 */
#define MAAP_PROBE 0x1 /* Probe MAC address(es) PDU */
#define MAAP_DEFEND 0x2 /* Defend address(es) response PDU */
#define MAAP_ANNOUNCE 0x3 /* Announce MAC address(es) acquired PDU */

/**
 * MAAP protocol events from IEEE Std 1722-2016, Table B.3
 */
typedef enum {
	MAAP_EVENT_BEGIN = 0, /* Initialize state machine */
	MAAP_EVENT_RELEASE, /* Release an address range */
	MAAP_EVENT_RESTART, /* Restart the state machine */
	MAAP_EVENT_RESERVE_ADDRESS, /* Reserve an address */
	MAAP_EVENT_RPROBE, /* Receive a MAAP PROBE PDU */
	MAAP_EVENT_RDEFEND, /* Receive a MAAP DEFEND PDU */
	MAAP_EVENT_RANNOUNCE, /* Receive a MAAP ANNOUNCE PDU */
	MAAP_EVENT_PROBE_COUNT, /* maap_probe_count has decremented to zero */
	MAAP_EVENT_ANNOUNCE_TIMER, /* announce_timer has expired */
	MAAP_EVENT_PROBE_TIMER, /* probe_timer has expired */
	MAAP_EVENT_PORT_OPERATIONAL, /* The port transitions to an operational state */

	MAAP_EVENT_ERR = 0xFF
} maap_event_t;

/**
 * MAAP protocol actions from IEEE 1722-2016, Table B.5
 */
typedef enum {
	MAAP_ACTION_NONE = 0,
	MAAP_ACTION_GENERATE_ADDRESS, /* Generate a random address range */
	MAAP_ACTION_INIT_MAAP_PROBE_COUNT, /* Initialize maap_probe_count */
	MAAP_ACTION_DEC_MAAP_PROBE_COUNT, /* Decrement maap_probe_count */
	MAAP_ACTION_COMPARE_MAC, /* Compare the received and local MAC address */
	MAAP_ACTION_PROBE_TIMER, /* Probe period timer */
	MAAP_ACTION_ANNOUNCE_TIMER, /* Announce period timer */
	MAAP_ACTION_SPROBE, /* Send a MAAP_PROBE PDU */
	MAAP_ACTION_SDEFEND, /* Send a MAAP_DEFEND PDU */
	MAAP_ACTION_SANNOUNCE /* Send a MAAP_ANNOUNCE PDU */
} maap_action_t;

/**
 * MAAP protocol states from IEEE 1722-2016, Table B.6
 */
typedef enum {
	MAAP_STATE_INITIAL = 0, /* Initialize the state machine */
	MAAP_STATE_PROBE, /* Probe the availability of an address range */
	MAAP_STATE_DEFEND /* Defend an acquired address range */
} maap_state_t;

/**
 * MAAP PDU structure
 */
struct __attribute__ ((packed)) maap_pdu {
#ifdef __BIG_ENDIAN__
	u32 subtype:8;

	u32 sv:1;
	u32 version:3;
	u32 message_type:4;

	u32 maap_version:5;
	u32 control_data_length:11;
#else
	u8 subtype;

	u8 message_type:4;
	u8 version:3;
	u8 sv:1;

	u8 control_data_length_msb:3;
	u8 maap_version:5;

	u8 control_data_length_lsb;
#endif

	u64 stream_id; /* 8 bytes */
	u8 requested_start_address[6]; /* 1 byte x 6 */
	u16 requested_count;
	u8 conflict_start_address[6]; /* 1 byte x 6 */
	u16 conflict_count;
};

#ifdef __BIG_ENDIAN__
#define MAAP_CONTROL_DATA_LENGTH(pdu)			((pdu)->control_data_length)
#define MAAP_CONTROL_DATA_LENGTH_SET(pdu, val)		((pdu)->control_data_length = (val))
#else
#define MAAP_CONTROL_DATA_LENGTH(pdu)			(((pdu)->control_data_length_msb << 8) | (pdu)->control_data_length_lsb)
#define MAAP_CONTROL_DATA_LENGTH_SET(pdu, val)	do {(pdu)->control_data_length_msb = (val) >> 8; (pdu)->control_data_length_lsb = (val) & 0xff; } while(0)
#endif

/**
 *	MAAP state machine
 */
struct maap_sm {
	/* State machine */
	maap_state_t state;
	maap_action_t action;

	/* Timers */
	struct timer announce_timer;
	struct timer probe_timer;

	/* Counters */
	unsigned int maap_probe_count; /* Counter that decrements from MAAP_PROBE_RETRANSMITS to zero from IEEE 1722-2016, Table B.4 */

	bool send_conflict_indication; /* Set to true upon sending MAAP_STATUS_SUCCESS, so next address conflict should send MAAP_STATUS_CONFLICT */
};

/**
 * MAAP MAC address range
 */
struct maap_range {
	struct list_head list;

	/* MAAP port where the range is allocated */
	struct maap_port *port;

	u32 range_id;

	/* Applied to MAC addresses range, end = start address + (mac_count-1) */
	unsigned int mac_count;
	/* Start MAC address */
	u8 start_mac_addr[6];

	/* MAAP state machine per range, 1..1 */
	struct maap_sm sm;
};

/**
 * MAAP port
 */
struct maap_port {
	unsigned int port_id;
	unsigned int logical_port;

	unsigned char local_physical_mac[6];
	bool initialized;

	struct net_rx net_rx;
	struct net_tx net_tx;

	struct ipc_tx ipc_tx_mac_service;
	struct ipc_rx ipc_rx_mac_service;

	unsigned int allocated_ranges_count;

	/* MAC address range, 0..n */
	struct list_head range;
};

/**
 * MAAP global context structure
 */
struct maap_ctx {
	/* Context timer */
	struct timer_ctx *timer_ctx;

	/* Global IPC channel */
	struct ipc_rx ipc_rx;
	struct ipc_tx ipc_tx;
	struct ipc_tx ipc_tx_sync;

	unsigned int port_max;
	bool management_enabled;

	/* variable size array, multiple ports MAAP device, 0..n */
	struct maap_port port[];
};

/**
 * MAAP range info
 */
struct maap_range_info {
	u8 src_physical_mac[6];
	u8 local_physical_mac[6];

	u8 requested_start_addr[6];
	unsigned int requested_count;

	struct maap_conflict_info {
		u8 start_addr[6];
		unsigned int count;
	} conflict;
};

/* MAAP Interface */
/**
 * Entry point for external use (AVDEEC).
 * Allows to start the process of attributing a MAC address range to a port and start the corresponding state machine
 * \return	0 on success, -1 otherwise
 * \param maap	pointer to the MAAP context
 * \param port, pointer to the port
 * \param range_id, chosen id for the range
 * \param addr, a chosen initial MAC address if needed otherwise NULL
 * \param n_addr int, number of MAC addresses requested for the range
 */
struct maap_range *maap_new_range_on_port(struct maap_ctx *ctx, struct maap_port *port, u32 range_id, const u8 *addr, unsigned int n_addr);

/**
 * Entry point for external use (AVDEEC).
 * Allows to remove all ranges attributed on a port
 * \return	0 on success, -1 otherwise
 * \param maap	pointer to the MAAP context
 * \param port, pointer to the port
 */
int maap_remove_all_range_on_port(struct maap_ctx *ctx, struct maap_port *port);

/**
 * Entry point for external use (AVDEEC).
 * Allows to remove a ranges attributed on a port by its range_id
 * \return	0 on success, -1 otherwise
 * \param maap	pointer to the MAAP context
 * \param port, pointer to the port
 * \param range_id, id of the range to delete
 * \param addr, OUTPUT, pointer to an u8 array to save the first address of the deleted range to send a response
 * \param count, OUTPUT, pointer to an unsigned int to save the number of addresses in the deleted range to send a response
 */
int maap_remove_range_on_port(struct maap_ctx *maap, struct maap_port *port, u32 range_id, u8 *addr, unsigned int *count);

/**
 * Get maap port from its logical port id
 * \return the maap port with the corresponding logical port id
 * \param maap, maap context
 * \param port, logical port id
 */
struct maap_port *logical_to_maap_port(struct maap_ctx *maap, unsigned int port_id);

/**
 * Get logical port
 * \return logical port id
 * \param maap_ctx, maap context
 * \param port_id
 */
unsigned int maap_port_to_logical(struct maap_ctx *maap, unsigned int port_id);

#endif /* _MAAP_H_ */
