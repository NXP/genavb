/*
* Copyright 2022-2024 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief RTOS specific Network service implementation
 @details
*/

#ifndef _RTOS_NET_BRIDGE_H_
#define _RTOS_NET_BRIDGE_H_

#include "os/net.h"
#include "genavb/frer.h"
#include "genavb/net_types.h"
#include "genavb/stream_identification.h"
#include "genavb/psfp.h"

#include "net_port.h"

struct net_bridge;
struct net_rx_ctx;

#define BR_RX_FRAME_ERROR	-1
#define BR_RX_FRAME_EMPTY	0
#define BR_RX_FRAME_SUCCESS	1
#define BR_RX_FRAME_EGRESS_TS	2

#define DEFAULT_BRIDGE_ID 0

#define BR_RX_FLAGS_MAC_LEARNING 1

struct net_bridge_drv_ops {
	int (*vlan_read)(struct net_bridge *bridge, uint16_t vid, bool *dynamic, struct genavb_vlan_port_map *map);
	int (*vlan_delete)(struct net_bridge *bridge, uint16_t vid, bool dynamic);
	int (*vlan_dump)(struct net_bridge *bridge, uint32_t *token, uint16_t *vid, bool *dynamic, struct genavb_vlan_port_map *map);
	int (*software_maclearn)(struct net_bridge *bridge, bool enable);

	int (*fdb_delete)(struct net_bridge *bridge, uint8_t *address, uint16_t vid, bool dynamic);
	int (*fdb_dump)(struct net_bridge *bridge, uint32_t *token, uint8_t *address, uint16_t *vid, bool *dynamic, struct genavb_fdb_port_map *map, genavb_fdb_status_t *status);
	int (*fdb_read)(struct net_bridge *bridge, uint8_t *address, uint16_t vid, bool *dynamic, struct genavb_fdb_port_map *map, genavb_fdb_status_t *status);

	/* Stream Identification */
	int (*si_update)(struct net_bridge *bridge, uint32_t index, struct genavb_stream_identity *entry);
	int (*si_delete)(struct net_bridge *bridge, uint32_t index);
	int (*si_read)(struct net_bridge *bridge, uint32_t index, struct genavb_stream_identity *entry);

	/* FRER */
	int (*seqg_update)(struct net_bridge *bridge, uint32_t index, struct genavb_sequence_generation *entry, unsigned int option);
	int (*seqg_delete)(struct net_bridge *bridge, uint32_t index);
	int (*seqg_read)(struct net_bridge *bridge, uint32_t index, struct genavb_sequence_generation *entry);

	int (*seqr_update)(struct net_bridge *bridge, uint32_t index, struct genavb_sequence_recovery *entry);
	int (*seqr_delete)(struct net_bridge *bridge, uint32_t index);
	int (*seqr_read)(struct net_bridge *bridge, uint32_t index, struct genavb_sequence_recovery *entry);

	int (*seqi_update)(struct net_bridge *bridge, struct net_port *port, struct genavb_sequence_identification *entry);
	int (*seqi_delete)(struct net_bridge *bridge, struct net_port *port);
	int (*seqi_read)(struct net_bridge *bridge, struct net_port *port, struct genavb_sequence_identification *entry);

	/* PSFP */
	int (*stream_filter_update)(struct net_bridge *bridge, uint32_t index, struct genavb_stream_filter_instance *instance);
	int (*stream_filter_delete)(struct net_bridge *bridge, uint32_t index);
	int (*stream_filter_read)(struct net_bridge *bridge, uint32_t index, struct genavb_stream_filter_instance *instance);
	unsigned int (*stream_filter_get_max_entries)(struct net_bridge *bridge);

	int (*stream_gate_update)(struct net_bridge *bridge, uint32_t index, struct genavb_stream_gate_instance *instance, unsigned int option);
	int (*stream_gate_delete)(struct net_bridge *bridge, uint32_t index);
	int (*stream_gate_read)(struct net_bridge *bridge, uint32_t index, genavb_sg_config_type_t type, struct genavb_stream_gate_instance *instance);
	unsigned int (*stream_gate_get_max_entries)(struct net_bridge *bridge);
	unsigned int (*stream_gate_control_get_max_entries)(struct net_bridge *bridge);

	int (*flow_meter_update)(struct net_bridge *bridge, uint32_t index, struct genavb_flow_meter_instance *instance, unsigned int option);
	int (*flow_meter_delete)(struct net_bridge *bridge, uint32_t index);
	int (*flow_meter_read)(struct net_bridge *bridge, uint32_t index, struct genavb_flow_meter_instance *instance);
	unsigned int (*flow_meter_get_max_entries)(struct net_bridge *bridge);

	/* DSA */
	int (*dsa_add)(struct net_bridge *bridge, unsigned int cpu_port, uint8_t *mac_addr, unsigned int slave_port);
	int (*dsa_delete)(struct net_bridge *bridge, unsigned int slave_port);

	int (*get_rx_frame_size)(struct net_bridge *bridge, uint32_t *length);
	int (*read_frame)(struct net_bridge *bridge, uint8_t *data, uint32_t length, uint8_t *port_index, uint64_t *ts, uint8_t *hr);
	int (*read_egress_ts_frame)(struct net_bridge *bridge);
	int (*send_frame)(struct net_bridge *bridge, unsigned int port_index, struct net_tx_desc *desc, uint8_t priority);
	void (*tx_cleanup)(struct net_bridge *bridge);
	int (*init)(struct net_bridge *bridge);
	void (*exit)(struct net_bridge *bridge);
};

struct net_bridge_stats {
	unsigned int rx_err;
	unsigned int rx_alloc_err;
	unsigned int tx_ts_err;
};

struct net_bridge {
	unsigned int index;
	net_driver_type_t drv_type;
	unsigned int drv_index;
	void *drv;
	struct net_bridge_drv_ops drv_ops;
	struct net_bridge_stats stats;
	uint64_t *sg_base_time;

	rtos_mutex_t mutex;
};

extern struct net_bridge bridges[CFG_BRIDGE_NUM];

int bridge_init(void);
void bridge_exit(void);
unsigned int bridge_rx(struct net_rx_ctx *net, struct net_bridge *bridge, unsigned int n);
int bridge_port_tx(struct net_port *port, uint8_t priority, struct net_tx_desc *desc);
void bridge_tx_cleanup(struct net_bridge *bridge);
struct net_bridge *bridge_get(unsigned int bridge_id);

static inline void *net_bridge_drv(struct net_bridge *bridge)
{
	return bridge->drv;
}

#endif /* _RTOS_NET_BRIDGE_H_ */
