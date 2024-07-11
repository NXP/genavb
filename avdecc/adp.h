/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2018-2019, 2021-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @brief ADP common defitions
*/

#ifndef _ADP_H_
#define _ADP_H_

#include "common/types.h"
#include "common/ipc.h"
#include "common/adp.h"
#include "adp_ieee.h"
#include "adp_milan.h"

/**
 * Discovered entity main context
 */
struct entity_discovery {
	struct entity_info info;
	struct timer timeout;
	int in_use;
	struct adp_discovery_ctx *disc;
};

typedef enum {
	ADP_DISC_WAITING = 0,
	ADP_DISC_DISCOVER,
	ADP_DISC_AVAILABLE,
	ADP_DISC_DEPARTING,
	ADP_DISC_TIMEOUT
} adp_discovery_states;

/**
 * ADP common controller discovery
 */
struct adp_discovery_ctx {
	int num_discovered_entities;
	unsigned int max_entities_discovery;
	struct entity_discovery *entities;  /* Array of the discovered entities */
};

struct adp_ctx {
	struct {
		struct adp_ieee_advertise_entity_ctx advertise;
	} ieee;
};

#define discovery_to_avdecc_port(disc)	container_of(disc, struct avdecc_port, discovery)

struct avdecc_port;

int adp_init(struct adp_ctx *adp);
void adp_exit(struct adp_ctx *adp);

void adp_update(struct adp_ctx *adp);

/* ADP common controller discovery */
int adp_discovery_init(struct adp_discovery_ctx *disc, void *data, struct avdecc_config *cfg);
void adp_discovery_exit(struct adp_discovery_ctx *disc);
unsigned int adp_discovery_data_size(unsigned int max_entities_discovery);
void adp_discovery_update(struct adp_discovery_ctx *disc, struct adp_pdu *pdu, u8 valid_time, u8 *mac_src);
void adp_discovery_remove(struct adp_discovery_ctx *disc, struct adp_pdu *pdu);

int adp_discovery_send_packet(struct adp_discovery_ctx *disc, u8 *entity_id);
int adp_advertise_send_packet(struct adp_ctx *adp, u8 message_type, unsigned int port_id);
int adp_net_rx(struct avdecc_port *port, struct adp_pdu *pdu, u8 msg_type, u8 valid_time, u8 *mac_src);
int adp_ipc_rx(struct entity *entity, struct ipc_adp_msg *adp_msg, u32 len, struct ipc_tx *ipc, unsigned int ipc_dst);
struct entity_discovery *adp_find_entity_discovery(struct avdecc_ctx *avdecc, unsigned int port_id, u64 entity_id);
struct entity_discovery *adp_find_entity_discovery_any(struct avdecc_ctx *avdecc, u64 entity_id, unsigned int num_interfaces);

#endif /* _ADP_H_ */
