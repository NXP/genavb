/*
 * Copyright 2017-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief RTOS specific Network service implementation
 @details
*/

#ifndef _RTOS_NET_PORT_H_
#define _RTOS_NET_PORT_H_

#include "os/clock.h"
#include "os/net.h"

#include "genavb/fdb.h"
#include "genavb/frame_preemption.h"
#include "genavb/net_types.h"
#include "genavb/ptp.h"
#include "genavb/qos.h"
#include "genavb/scheduled_traffic.h"
#include "genavb/vlan.h"

#include "rtos_abstraction_layer.h"

#include "config.h"

#define NET_PAYLOAD_SIZE_MAX		1518
#define NET_DATA_SIZE			DEFAULT_NET_DATA_SIZE

#if (NET_PAYLOAD_SIZE_MAX > (NET_DATA_SIZE - NET_DATA_OFFSET))
#error "Invalid NET_PAYLOAD_SIZE_MAX"
#endif

#define PORT_ERROR			(1 << 0)
#define PORT_SUCCESS			(1 << 1)
#define PORT_TX_SUCCESS			(1 << 2)

struct net_rx_ctx;
struct net_tx_ctx;
struct net_port;

struct net_port_status {
	uint16_t port;
	bool up;
	bool full_duplex;
	uint64_t rate;
};

struct ptp_frame_data {
	uint8_t version;
	struct ptp_port_identity src_port_id;
	uint16_t sequence_id;
	uint8_t message_type;
};

struct port_tx_ts_data {
	uint32_t priv;
	unsigned int queue;
	struct ptp_frame_data ptp_data;
};

#define TX_QUEUE_FLAGS_STRICT_PRIORITY (1 << 0)
#define TX_QUEUE_FLAGS_CREDIT_SHAPER   (1 << 1)

#define TX_QUEUE_PROP_MAX 8

struct tx_queue_properties {
	unsigned int num_queues;
	unsigned int queue_prop[TX_QUEUE_PROP_MAX];
};

#define NET_PORT_TX_TS_QUEUE_LENGTH 8

typedef enum {
	ENET_t,
	ENET_1G_t,
	ENET_QOS_t,
	ENETC_1G_t, /* ENETC stand-alone */
	ENETC_PSEUDO_1G_t, /* ENETC pseudo-endpoint */
	NETC_SW_t, /* NETC switch  */
	NETC_EMDIO_t, /* EMDIO */
	NETC_PORT_EMDIO_t, /* MAC Port MDIO */
	ENET_MAX_t
} net_driver_type_t;

struct net_port_drv_ops {
	int (*vlan_update)(struct net_port *port, uint16_t vid, bool dynamic, struct genavb_vlan_port_map *map);
	int (*fdb_update)(struct net_port *port, uint8_t *address, uint16_t vid, bool dynamic, genavb_fdb_port_control_t control);
	int (*set_pvid)(struct net_port *port, uint16_t vid);
	int (*get_pvid)(struct net_port *port, uint16_t *vid);
	int (*add_multi)(struct net_port *port, uint8_t *addr);
	int (*del_multi)(struct net_port *port, uint8_t *addr);
	int (*get_rx_frame_size)(struct net_port *port, uint32_t *length, uint32_t queue);
	int (*read_frame)(struct net_port *port, uint8_t *data, uint32_t length, unsigned int *port_index, uint64_t *ts, uint32_t queue);
	int (*send_frame)(struct net_port *port, struct net_tx_desc *desc, uint32_t queue);
	void (*tx_cleanup)(struct net_port *port);
	void (*link_up)(struct net_port *port);
	void (*link_down)(struct net_port *port);
	int (*set_tx_queue_config)(struct net_port *port, struct tx_queue_properties *cfg);
	int (*set_tx_idle_slope)(struct net_port *port, uint64_t idle_slope, uint32_t queue);
	int (*set_st_config)(struct net_port *port, struct genavb_st_config *config);
	int (*get_st_config)(struct net_port *port, genavb_st_config_type_t type, struct genavb_st_config *config, unsigned int list_length);
	int (*st_set_max_sdu)(struct net_port *port, struct genavb_st_max_sdu *queue_max_sdu, unsigned int n);
	int (*st_get_max_sdu)(struct net_port *port, struct genavb_st_max_sdu *queue_max_sdu);
	int (*st_max_entries)(struct net_port *port);
	int (*set_fp)(struct net_port *port, unsigned int type, struct genavb_fp_config *config);
	int (*get_fp)(struct net_port *port, unsigned int type, struct genavb_fp_config *config);
	int (*set_max_frame_size)(struct net_port *port, uint16_t size);
	int (*stats_get_number)(struct net_port *port);
	int (*stats_get_strings)(struct net_port *port, const char **buf, unsigned int buf_len);
	int (*stats_get)(struct net_port *port, uint64_t *buf, unsigned int buf_len);
	int (*set_priority_to_tc_map)(struct net_port *port);
	int (*init)(struct net_port *port);
	void (*exit)(struct net_port *port);
	int (*post_init)(struct net_port *port);
	void (*pre_exit)(struct net_port *port);
};

struct timer_cfg {
	unsigned int channel;
};

#define MAX_QUEUES 5

enum {
	PORT_CLOCK_GPTP_0 = 0,
	PORT_CLOCK_GPTP_1,
	PORT_CLOCK_LOCAL,
	PORT_CLOCK_MAX,
};

struct queue_stats {
	unsigned int tx;
	unsigned int tx_err;
	unsigned int tx_ts_err;
	unsigned int rx;
	unsigned int rx_err;
	unsigned int rx_alloc_err;
};

struct net_port {
	unsigned int index;
	int pps_timer_channel; /* Channel number for a pps signal, otherwise -1 */

	net_driver_type_t drv_type;
	unsigned int drv_index;
	void *drv;

	uintptr_t base;
	unsigned int mii_mode;
	unsigned int phy_index;

	struct net_port_drv_ops drv_ops;

	struct timer_cfg timers[3];
	struct timer_cfg timer_event;

	uint8_t mac_addr[6];
	bool up;		/* port up status, protected by mutex */
	bool tx_up;		/* port up status, only used by net tx task */
	struct port_qos *qos;

	bool st_enabled;
	uint64_t st_base_time;

	struct logical_port *logical_port;

	unsigned int num_rx_q;
	unsigned int num_tx_q;

	struct tx_queue_properties *tx_q_cap;

	uint8_t map[QOS_PRIORITY_MAX];

	unsigned int max_pdu;

	uint16_t pvid;

	os_clock_id_t clock[PORT_CLOCK_MAX];
	os_clock_id_t clock_st_psfp;

	unsigned int hw_clock_id;

	struct hw_clock *hw_clock;

	unsigned int phy_speed;
	unsigned int phy_duplex;

	rtos_mutex_t mutex[MAX_QUEUES];

	rtos_mutex_t config_mutex;

	rtos_event_group_t event_group;

	uint32_t rx_tstamp_latency;
	uint32_t tx_tstamp_latency;

	struct queue_stats stats[MAX_QUEUES];
};

extern struct net_port ports[CFG_PORTS];

int port_set_tx_queue_config(struct net_port *port, struct tx_queue_properties *cfg);
int port_set_tx_idle_slope(struct net_port *port, uint64_t idle_slope, unsigned int queue);
unsigned int port_tx_queue_prop_num_cbs(struct tx_queue_properties *cfg);
unsigned int port_tx_queue_prop_num_sp(struct tx_queue_properties *cfg);
uint8_t *port_get_hwaddr(struct net_port *port);
int port_status(struct net_port *port, struct net_port_status *status);
int port_add_multi(struct net_port *port, uint8_t *addr);
int port_del_multi(struct net_port *port, uint8_t *addr);
uint32_t port_gettime32(struct net_port *port);
uint64_t port_gettime64(struct net_port *port);
void port_tx_cleanup(struct net_port *port);
int port_tx_ts(struct net_port *port);
unsigned int port_rx(struct net_rx_ctx *, struct net_port *port, unsigned int n, unsigned int queue);
int port_tx(struct net_port *port, struct net_tx_desc *desc, unsigned int queue);
void port_up(struct net_port *port);
void port_down(struct net_port *port);
void port_tx_up(struct net_port *port);
void port_tx_down(struct net_port *port);
int port_set_st_config(struct net_port *port, struct genavb_st_config *config);
int port_get_st_config(struct net_port *port, genavb_st_config_type_t type,
		       struct genavb_st_config *config, unsigned int list_length);
int port_st_set_max_sdu(struct net_port *port, struct genavb_st_max_sdu *queue_max_sdu, unsigned int n);
int port_st_get_max_sdu(struct net_port *port, struct genavb_st_max_sdu *queue_max_sdu);
int port_st_max_entries(struct net_port *port);
int port_set_fp(struct net_port *port, unsigned int type, struct genavb_fp_config *config);
int port_get_fp(struct net_port *port, unsigned int type, struct genavb_fp_config *config);
int port_set_max_frame_size(struct net_port *port, uint16_t size);
int port_stats_get_number(struct net_port *port);
int port_stats_get_strings(struct net_port *port, const char **buf, unsigned int buf_len);
int port_stats_get(struct net_port *port, uint64_t *buf, unsigned int buf_len);
unsigned int port_priority_to_traffic_class(struct net_port *port, uint8_t priority);
void port_set_priority_to_traffic_class_map(struct net_port *port, unsigned int enabled_tclass, unsigned int enabled_sr_class, uint8_t *custom_map);
int port_init(void);
void port_exit(void);
int port_post_init(void);
void port_pre_exit(void);

static inline void *net_port_drv(struct net_port *port)
{
	return port->drv;
}

static inline struct logical_port *physical_to_logical_port(struct net_port *port)
{
	return port->logical_port;
}

static inline void port_tx_clean_desc(struct net_port *port, unsigned long priv)
{
	net_tx_free((struct net_tx_desc *)priv);
}

#endif /* _RTOS_NET_PORT_H_ */
