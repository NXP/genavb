/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2019, 2021-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVDECC main header file
 @details Definition of AVDECC stack component entry point functions and global context structure.
*/

#ifndef _AVDECC_H_
#define _AVDECC_H_

#include "common/net.h"
#include "common/ipc.h"
#include "common/timer.h"
#include "common/avtp.h"
#include "common/avdecc.h"
#include "common/srp.h"
#include "common/log.h"

#include "genavb/aem.h"

#include "adp.h"
#include "aecp.h"
#include "acmp.h"
#include "aem.h"

#define AVDECC_INFLIGHT_TIMER_RESTART	0
#define AVDECC_INFLIGHT_TIMER_STOP	1

/* The Presentation time offset can be changed to any value in the range between 0x0 and 0x7FFFFFFF ns
 * as per MILAN Specification v1.2 5.3.7.6
 */
#define STREAM_PRESENTATION_TIME_OFFSET_MAX	0x7FFFFFFF
#define STREAM_PRESENTATION_TIME_OFFSET_INVALID	(STREAM_PRESENTATION_TIME_OFFSET_MAX + 1U)

struct inflight_data {
	u8 retried;
	u8 msg_type;
	u16 sequence_id;					/**< Sequence ID to match the entry against AECP responses, in host byte order. */
	u16 orig_seq_id;
	uintptr_t priv[2];
	u8 mac_dst[6];
	u16 len;
	u16 port_id; /* avdecc port information */
	union {
		struct acmp_pdu acmp;
		struct aecp_aem_pdu aem;
		u8 buf[AVDECC_AECP_MAX_SIZE];
		//other commands
	} pdu;
};

struct inflight_ctx {
	struct timer timeout;
	unsigned int timeout_ms;
	struct inflight_data data;
	int(*cb)(struct inflight_ctx *);
	struct list_head list;
	struct list_head *list_head;
	struct entity *entity;
};

struct entity {
	unsigned int index;
	unsigned int flags;
	struct aem_desc_hdr *aem_descs;
	struct aem_desc_hdr *aem_dynamic_descs;
	struct aecp_ctx aecp;
	struct acmp_ctx acmp;
	struct adp_ctx adp;
	struct entity_descriptor *desc;
	struct avdecc_ctx *avdecc;
	struct inflight_ctx *inflight_storage;
	struct list_head free_inflight;
	unsigned int channel_openmask;
	unsigned int channel_waitmask;
	unsigned int valid_time;			/**< Valid time is in units of seconds. */
	unsigned int max_inflights;
	bool milan_mode;
};

struct avdecc_port {
	unsigned int port_id; //maps directly to AVB interface index
	unsigned int logical_port;

	u8 local_physical_mac[6];
	bool initialized;

	struct adp_discovery_ctx discovery;

	struct net_rx net_rx;
	struct net_tx net_tx;

	struct ipc_tx ipc_tx_srp;
	struct ipc_tx ipc_tx_mac_service;
	struct ipc_tx ipc_tx_gptp;

	struct ipc_rx ipc_rx_gptp;
	struct ipc_rx ipc_rx_srp;
	struct ipc_rx ipc_rx_mac_service;
};

/* AVDECC global context structure */
struct avdecc_ctx {
	struct ipc_tx ipc_tx_media_stack;
	struct ipc_tx ipc_tx_maap;
	struct ipc_tx ipc_tx_controlled;
	struct ipc_tx ipc_tx_controller;
	struct ipc_tx ipc_tx_controller_sync;

	struct ipc_rx ipc_rx_controller;
	struct ipc_rx ipc_rx_controlled;
	struct ipc_rx ipc_rx_media_stack;
	struct ipc_rx ipc_rx_maap;

	void *adp_discovery_data;
	struct timer_ctx *timer_ctx;
	struct entity *entities[CFG_AVDECC_NUM_ENTITIES]; //make this an array to pointer to avoid saving the dynamic allocations pointers (for later free) and keep the allocated space starting with the parent.
	unsigned int num_entities;
	unsigned int port_max;
	bool srp_enabled;
	bool management_enabled;
	bool milan_mode;
	bool use_gptp_bridge_stack; /* If true, use gptp bridge stack instance, instead of endpoint stack, to retrieve gptp information */

	/* variable size array */
	struct avdecc_port port[];
};

static inline unsigned int entity_ready(struct entity *entity)
{
	return (entity->channel_openmask & entity->channel_waitmask) == entity->channel_waitmask;
}

/* An invalid mac address is all zeroed. */
static inline bool is_invalid_mac_addr(const u8 *mac_addr)
{
	return !(mac_addr[0] | mac_addr[1] | mac_addr[2] | mac_addr[3] | mac_addr[4] | mac_addr[5]);
}

#define avdecc_port_to_context(port_)	container_of(port_, struct avdecc_ctx, port[port_->port_id])

void avdecc_net_rx(struct net_rx *, struct net_rx_desc *);
void avdecc_ipc_rx_gptp(struct ipc_rx const *, struct ipc_desc *);
void avdecc_ipc_rx_controller(struct ipc_rx const *rx, struct ipc_desc *desc);
void avdecc_ipc_rx_controlled(struct ipc_rx const *rx, struct ipc_desc *desc);
void avdecc_ipc_rx_media_stack(struct ipc_rx const *rx, struct ipc_desc *desc);
int avdecc_net_tx(struct avdecc_port *port, struct net_tx_desc *desc);
size_t avdecc_add_common_header(void *buf, u8 subtype, u8 msg_type, u16 length, u8 status);
struct entity * avdecc_get_entity(struct avdecc_ctx *avdecc, u64 entity_id);
bool avdecc_entity_port_valid(struct entity *entity, unsigned int port_id);
struct inflight_ctx *avdecc_inflight_get(struct entity *entity);
int avdecc_inflight_start(struct list_head *inflight, struct inflight_ctx *entry, unsigned int timeout);
void avdecc_inflight_restart(struct inflight_ctx *entry);
struct inflight_ctx *avdecc_inflight_find(struct list_head *inflight_head, u16 sequence_id);
struct inflight_ctx *aem_inflight_find_controller(struct list_head *inflight_head, u16 sequence_id, u64 controller_id);
void avdecc_inflight_remove(struct entity *entity, struct inflight_ctx *entry);
int avdecc_inflight_cancel(struct entity *entity, struct list_head *inflight_head, u16 sequence_id, u16 *orig_seq_id, void **priv0, void **priv1);
struct entity *avdecc_get_local_controller_any(struct avdecc_ctx *avdecc);
struct entity *avdecc_get_local_controller(struct avdecc_ctx *avdecc, unsigned int port_id);
struct entity *avdecc_get_local_controlled_any(struct avdecc_ctx *avdecc);
struct entity *avdecc_get_local_listener(struct avdecc_ctx *avdecc, unsigned int port_id);
struct entity *avdecc_get_local_listener_any(struct avdecc_ctx *avdecc, unsigned int port_id);
struct entity *avdecc_get_local_talker(struct avdecc_ctx *avdecc, unsigned int port_id);
bool avdecc_entity_is_locked(struct entity *entity, u64 controller_id);
bool avdecc_entity_is_acquired(struct entity *entity, u64 controller_id);
struct avdecc_port *logical_to_avdecc_port(struct avdecc_ctx *avdecc, unsigned int logical_port);
unsigned int avdecc_port_to_logical(struct avdecc_ctx *avdecc, unsigned int port_id);

#endif /* _AVDECC_H_ */
