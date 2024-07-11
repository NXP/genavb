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

#include "common/log.h"
#include "common/ptp.h"

#include "os/net.h"
#include "os/clock.h"

#include "genavb/error.h"

#include "clock.h"
#include "net_mdio.h"
#include "net_phy.h"
#include "net_port.h"
#include "net_port_enet.h"
#include "net_port_enet_qos.h"
#include "net_port_enetc_ep.h"
#include "net_port_netc_sw.h"
#include "net_rx.h"
#include "ptp.h"

struct net_port ports[CFG_PORTS] = {
#if CFG_PORTS > 0
	[0] = {
		.index = 0,
		.drv_type = BOARD_NET_PORT0_DRV_TYPE,
		.drv_index = BOARD_NET_PORT0_DRV_INDEX,
		.base = BOARD_NET_PORT0_DRV_BASE,
		.phy_index = BOARD_NET_PORT0_PHY_INDEX,
		.mii_mode = BOARD_NET_PORT0_MII_MODE,
		.up = false,
#ifdef BOARD_NET_PORT0_HW_CLOCK
		.hw_clock_id = BOARD_NET_PORT0_HW_CLOCK,
#else
		.hw_clock_id = 0,
#endif
		.timers = {
			[0] = {
				.channel = BOARD_NET_PORT0_1588_TIMER_CHANNEL_0,
			},
			[1] = {
				.channel = BOARD_NET_PORT0_1588_TIMER_CHANNEL_1,
			},
			[2] = {
				.channel = BOARD_NET_PORT0_1588_TIMER_CHANNEL_2,
			},
		},
		.timer_event = {
			.channel = BOARD_NET_PORT0_1588_TIMER_EVENT_CHANNEL,
		},
#ifdef BOARD_NET_PORT0_1588_TIMER_PPS
		.pps_timer_channel = BOARD_NET_PORT0_1588_TIMER_PPS,
#else
		.pps_timer_channel = -1,
#endif
	},
#endif
#if CFG_PORTS > 1
	[1] = {
		.index = 1,
		.drv_type = BOARD_NET_PORT1_DRV_TYPE,
		.drv_index = BOARD_NET_PORT1_DRV_INDEX,
		.base = BOARD_NET_PORT1_DRV_BASE,
		.phy_index = BOARD_NET_PORT1_PHY_INDEX,
		.mii_mode = BOARD_NET_PORT1_MII_MODE,
		.up = false,
#ifdef BOARD_NET_PORT1_HW_CLOCK
		.hw_clock_id = BOARD_NET_PORT1_HW_CLOCK,
#else
		.hw_clock_id = 1,
#endif
		.timers = {
			[0] = {
				.channel = BOARD_NET_PORT1_1588_TIMER_CHANNEL_0,
			},
			[1] = {
				.channel = BOARD_NET_PORT1_1588_TIMER_CHANNEL_1,
			},
			[2] = {
				.channel = BOARD_NET_PORT1_1588_TIMER_CHANNEL_2,
			},
		},
		.timer_event = {
			.channel = BOARD_NET_PORT1_1588_TIMER_EVENT_CHANNEL,
		},
#ifdef BOARD_NET_PORT1_1588_TIMER_PPS
		.pps_timer_channel = BOARD_NET_PORT1_1588_TIMER_PPS,
#else
		.pps_timer_channel = -1,
#endif
	},
#endif
#if CFG_PORTS > 2
	[2] = {
		.index = 2,
		.drv_type = BOARD_NET_PORT2_DRV_TYPE,
		.drv_index = BOARD_NET_PORT2_DRV_INDEX,
		.base = BOARD_NET_PORT2_DRV_BASE,
		.phy_index = BOARD_NET_PORT2_PHY_INDEX,
		.mii_mode = BOARD_NET_PORT2_MII_MODE,
		.up = false,
#ifdef BOARD_NET_PORT2_HW_CLOCK
		.hw_clock_id = BOARD_NET_PORT2_HW_CLOCK,
#else
		.hw_clock_id = 2,
#endif
		.timers = {
			[0] = {
				.channel = BOARD_NET_PORT2_1588_TIMER_CHANNEL_0,
			},
			[1] = {
				.channel = BOARD_NET_PORT2_1588_TIMER_CHANNEL_1,
			},
			[2] = {
				.channel = BOARD_NET_PORT2_1588_TIMER_CHANNEL_2,
			},
		},
		.timer_event = {
			.channel = BOARD_NET_PORT2_1588_TIMER_EVENT_CHANNEL,
		},
#ifdef BOARD_NET_PORT2_1588_TIMER_PPS
		.pps_timer_channel = BOARD_NET_PORT2_1588_TIMER_PPS,
#else
		.pps_timer_channel = -1,
#endif
	},
#endif
#if CFG_PORTS > 3
	[3] = {
		.index = 3,
		.drv_type = BOARD_NET_PORT3_DRV_TYPE,
		.drv_index = BOARD_NET_PORT3_DRV_INDEX,
		.base = BOARD_NET_PORT3_DRV_BASE,
		.phy_index = BOARD_NET_PORT3_PHY_INDEX,
		.mii_mode = BOARD_NET_PORT3_MII_MODE,
		.up = false,
#ifdef BOARD_NET_PORT3_HW_CLOCK
		.hw_clock_id = BOARD_NET_PORT3_HW_CLOCK,
#else
		.hw_clock_id = 3,
#endif
		.timers = {
			[0] = {
				.channel = BOARD_NET_PORT3_1588_TIMER_CHANNEL_0,
			},
			[1] = {
				.channel = BOARD_NET_PORT3_1588_TIMER_CHANNEL_1,
			},
			[2] = {
				.channel = BOARD_NET_PORT3_1588_TIMER_CHANNEL_2,
			},
		},
		.timer_event = {
			.channel = BOARD_NET_PORT3_1588_TIMER_EVENT_CHANNEL,
		},
#ifdef BOARD_NET_PORT3_1588_TIMER_PPS
		.pps_timer_channel = BOARD_NET_PORT3_1588_TIMER_PPS,
#else
		.pps_timer_channel = -1,
#endif
	},
#endif
#if CFG_PORTS > 4
	[4] = {
		.index = 4,
		.drv_type = BOARD_NET_PORT4_DRV_TYPE,
		.drv_index = BOARD_NET_PORT4_DRV_INDEX,
		.base = BOARD_NET_PORT4_DRV_BASE,
		.phy_index = BOARD_NET_PORT4_PHY_INDEX,
		.mii_mode = BOARD_NET_PORT4_MII_MODE,
		.up = false,
#ifdef BOARD_NET_PORT4_HW_CLOCK
		.hw_clock_id = BOARD_NET_PORT4_HW_CLOCK,
#else
		.hw_clock_id = 4,
#endif
		.timers = {
			[0] = {
				.channel = BOARD_NET_PORT4_1588_TIMER_CHANNEL_0,
			},
			[1] = {
				.channel = BOARD_NET_PORT4_1588_TIMER_CHANNEL_1,
			},
			[2] = {
				.channel = BOARD_NET_PORT4_1588_TIMER_CHANNEL_2,
			},
		},
		.timer_event = {
			.channel = BOARD_NET_PORT4_1588_TIMER_EVENT_CHANNEL,
		},
#ifdef BOARD_NET_PORT4_1588_TIMER_PPS
		.pps_timer_channel = BOARD_NET_PORT4_1588_TIMER_PPS,
#else
		.pps_timer_channel = -1,
#endif
	},
#endif
#if CFG_PORTS > 5
	[5] = {
		.index = 5,
		.drv_type = BOARD_NET_PORT5_DRV_TYPE,
		.drv_index = BOARD_NET_PORT5_DRV_INDEX,
		.base = BOARD_NET_PORT5_DRV_BASE,
		.phy_index = BOARD_NET_PORT5_PHY_INDEX,
		.mii_mode = BOARD_NET_PORT5_MII_MODE,
		.up = false,
#ifdef BOARD_NET_PORT5_HW_CLOCK
		.hw_clock_id = BOARD_NET_PORT5_HW_CLOCK,
#else
		.hw_clock_id = 5,
#endif
		.timers = {
			[0] = {
				.channel = BOARD_NET_PORT5_1588_TIMER_CHANNEL_0,
			},
			[1] = {
				.channel = BOARD_NET_PORT5_1588_TIMER_CHANNEL_1,
			},
			[2] = {
				.channel = BOARD_NET_PORT5_1588_TIMER_CHANNEL_2,
			},
		},
		.timer_event = {
			.channel = BOARD_NET_PORT5_1588_TIMER_EVENT_CHANNEL,
		},
#ifdef BOARD_NET_PORT5_1588_TIMER_PPS
		.pps_timer_channel = BOARD_NET_PORT5_1588_TIMER_PPS,
#else
		.pps_timer_channel = -1,
#endif
	},
#endif
#if CFG_PORTS > 6
	[6] = {
		.index = 6,
		.drv_type = BOARD_NET_PORT6_DRV_TYPE,
		.drv_index = BOARD_NET_PORT6_DRV_INDEX,
		.base = BOARD_NET_PORT6_DRV_BASE,
		.phy_index = BOARD_NET_PORT6_PHY_INDEX,
		.mii_mode = BOARD_NET_PORT6_MII_MODE,
		.up = false,
#ifdef BOARD_NET_PORT6_HW_CLOCK
		.hw_clock_id = BOARD_NET_PORT6_HW_CLOCK,
#else
		.hw_clock_id = 6,
#endif
		.timers = {
			[0] = {
				.channel = BOARD_NET_PORT6_1588_TIMER_CHANNEL_0,
			},
			[1] = {
				.channel = BOARD_NET_PORT6_1588_TIMER_CHANNEL_1,
			},
			[2] = {
				.channel = BOARD_NET_PORT6_1588_TIMER_CHANNEL_2,
			},
		},
		.timer_event = {
			.channel = BOARD_NET_PORT6_1588_TIMER_EVENT_CHANNEL,
		},
#ifdef BOARD_NET_PORT6_1588_TIMER_PPS
		.pps_timer_channel = BOARD_NET_PORT6_1588_TIMER_PPS,
#else
		.pps_timer_channel = -1,
#endif
	},
#endif
};

static unsigned int port_tx_queue_prop_num(struct tx_queue_properties *cfg, unsigned int flag)
{
	unsigned int i, num = 0;

	for (i = 0; i < cfg->num_queues; i++)
		if (cfg->queue_prop[i] & flag)
			num++;

	return num;
}

unsigned int port_tx_queue_prop_num_cbs(struct tx_queue_properties *cfg)
{
	return port_tx_queue_prop_num(cfg, TX_QUEUE_FLAGS_CREDIT_SHAPER);
}

unsigned int port_tx_queue_prop_num_sp(struct tx_queue_properties *cfg)
{
	return port_tx_queue_prop_num(cfg, TX_QUEUE_FLAGS_STRICT_PRIORITY);
}

static int port_tx_queue_config_check(struct tx_queue_properties *cap,
				      struct tx_queue_properties *cfg)
{
	int i;

	if (cfg->num_queues > cap->num_queues)
		return -1;

	for (i = 0; i < cfg->num_queues; i++) {
		if (cfg->queue_prop[i] && !(cfg->queue_prop[i] & cap->queue_prop[i]))
			return -1;
	}

	return 0;
}

int port_set_tx_queue_config(struct net_port *port, struct tx_queue_properties *cfg)
{
	if (port_tx_queue_config_check(port->tx_q_cap, cfg) < 0) {
		os_log(LOG_ERR, "port(%u) invalid config\n", port->index);
		goto err;
	}

	if (port->drv_ops.set_tx_queue_config(port, cfg) < 0) {
		os_log(LOG_ERR, "port(%u) port_set_tx_queue_config() error \n", port->index);
		goto err;
	}

	port->num_tx_q = cfg->num_queues;

	return 0;

err:
	return -1;
}

int port_set_tx_idle_slope(struct net_port *port, unsigned int idle_slope, unsigned int queue)
{
	if (port->drv_ops.set_tx_idle_slope(port, idle_slope, queue) < 0) {
		os_log(LOG_ERR, "port(%u) set_tx_idle_slope() error\n", port->index);
		goto err;
	}

	return 0;

err:
	return -1;
}

uint8_t *port_get_hwaddr(struct net_port *port)
{
	return port->mac_addr;
}

int port_status(struct net_port *port, struct net_port_status *status)
{
	if (port->up) {
		status->up = true;
		phy_port_status(port->phy_index, status);
	} else {
		status->up = false;
		status->full_duplex = false;
		status->rate = 0;
	}

	return GENAVB_SUCCESS;
}

int port_add_multi(struct net_port *port, uint8_t *addr)
{
	return port->drv_ops.add_multi(port, addr);
}

int port_del_multi(struct net_port *port, uint8_t *addr)
{
	return port->drv_ops.del_multi(port, addr);
}

static inline void ptp_hdr_to_data(struct ptp_hdr *ptp, struct ptp_frame_data *ptp_data)
{
	if (ptp) {
		ptp_data->version = ptp->version_ptp;
		memcpy(&ptp_data->src_port_id, &ptp->source_port_id, sizeof(struct ptp_port_identity));
		ptp_data->sequence_id = ntohs(ptp->sequence_id);
		ptp_data->message_type = ptp->msg_type;
	} else {
		ptp_data->version = 0;
		memset(&ptp_data->src_port_id, 0, sizeof(struct ptp_port_identity));
		ptp_data->sequence_id = 0;
		ptp_data->message_type = 0;
	}
}

unsigned int port_rx(struct net_rx_ctx *net, struct net_port *port, unsigned int n, unsigned int queue)
{
	struct net_port *port_rx;
	uint32_t length;
	uint64_t cycles;
	struct net_rx_desc *desc;
	unsigned int read = 0;
	unsigned int port_index;
	int rc;

	if (!port->up)
		return 0;

	while (read < n) {
		rc = port->drv_ops.get_rx_frame_size(port, &length, queue);
		if (rc == 1) {

			desc = net_pool_rx_alloc(length);
			if (!desc) {
				port->stats[queue].rx_alloc_err++;
				break;

			}
			desc->len = length;

			if (!port->drv_ops.read_frame(port, (uint8_t *)desc + desc->l2_offset,
						      length, &port_index, &cycles, queue)) {

				port_rx = &ports[port_index];

				desc->ts64 = hw_clock_cycles_to_time(port->hw_clock, cycles) -
					     port->rx_tstamp_latency;
				desc->ts = (uint32_t)desc->ts64;
				port->stats[queue].rx++;
				eth_rx(net, desc, port_rx);
			} else {
				port->stats[queue].rx_err++;
				net_rx_free(desc);
			}

		} else if (rc < 0) {
			port->stats[queue].rx_err++;
			port->drv_ops.read_frame(port, NULL, 0, &port_index, NULL, queue);
		} else {
			break;
		}

		read++;

		rtos_mutex_unlock(&net->mutex);
		/* Explicit preemption point, to allow socket synchronous receive */
		rtos_mutex_lock(&net->mutex, RTOS_WAIT_FOREVER);
	}

	return read;
}

void port_tx_cleanup(struct net_port *port)
{
	port->drv_ops.tx_cleanup(port);
}

static inline void port_tx_lock(struct net_port *port, unsigned int queue)
{
	rtos_mutex_lock(&port->mutex[queue], RTOS_WAIT_FOREVER);
}

static inline void port_tx_unlock(struct net_port *port, unsigned int queue)
{
	rtos_mutex_unlock(&port->mutex[queue]);
}

static void port_tx_lock_all(struct net_port *port)
{
	int i;

	for (i = 0; i < port->num_tx_q; i++)
		port_tx_lock(port, i);
}

static void port_tx_unlock_all(struct net_port *port)
{
	int i;

	for (i = port->num_tx_q - 1; i >= 0; i--)
		port_tx_unlock(port, i);
}


int port_tx(struct net_port *port, struct net_tx_desc *desc, unsigned int queue)
{
	int rc;

	port_tx_lock(port, queue);

	if (port->up) {

		port->stats[queue].tx++;
		rc = port->drv_ops.send_frame(port, (uint8_t *)desc + desc->l2_offset,
					      desc->len, desc, queue, desc->flags & NET_TX_FLAGS_HW_TS);
		if (rc < 0) {
			port->stats[queue].tx_err++;
			rc = -1;
		}
	} else {
		rc = -1;
	}

	port_tx_unlock(port, queue);

	/* Free descriptor now only if buffer is copied and successfully transmitted */
	if (!rc)
		net_tx_free(desc);

	return rc;
}

void port_tx_up(struct net_port *port)
{
	port->tx_up = true;

	if (port->logical_port->is_bridge)
		return;

	port_qos_up(port);

	rtos_event_group_set(&port->event_group, PORT_TX_SUCCESS);
}

void port_tx_down(struct net_port *port)
{
	port->tx_up = false;

	if (port->logical_port->is_bridge)
		return;

	port_qos_down(port);

	rtos_event_group_set(&port->event_group, PORT_TX_SUCCESS);
}

void port_up(struct net_port *port)
{
	port_tx_lock_all(port);

	if (port->up)
		goto out;

	port->drv_ops.link_up(port);

	port->up = true;

	if (port->phy_index != -1)
		phy_get_ts_latency(port->phy_index, &port->rx_tstamp_latency, &port->tx_tstamp_latency);

out:
	port_tx_unlock_all(port);

	os_log(LOG_INFO, "port(%u) up, speed:%x, duplex:%x\n", port->index, port->phy_speed, port->phy_duplex);

	rtos_event_group_set(&port->event_group, PORT_SUCCESS);
}

int port_set_st_config(struct net_port *port, struct genavb_st_config *config)
{
	int rc;

	if (port->drv_ops.set_st_config)
		rc = port->drv_ops.set_st_config(port, config);
	else
		rc = -1;

	return rc;
}

int port_get_st_config(struct net_port *port, genavb_st_config_type_t type,
		       struct genavb_st_config *config, unsigned int list_length)
{
	int rc;

	if (port->drv_ops.get_st_config)
		rc = port->drv_ops.get_st_config(port, type, config, list_length);
	else
		rc = -1;

	return rc;
}

int port_st_set_max_sdu(struct net_port *port, struct genavb_st_max_sdu *queue_max_sdu, unsigned int n)
{
	int rc;

	if (port->drv_ops.st_set_max_sdu)
		rc = port->drv_ops.st_set_max_sdu(port, queue_max_sdu, n);
	else
		rc = -GENAVB_ERR_ST_MAX_SDU_NOT_SUPPORTED;

	return rc;
}

int port_st_get_max_sdu(struct net_port *port, struct genavb_st_max_sdu *queue_max_sdu)
{
	int rc;

	if (port->drv_ops.st_get_max_sdu)
		rc = port->drv_ops.st_get_max_sdu(port, queue_max_sdu);
	else
		rc = -GENAVB_ERR_ST_MAX_SDU_NOT_SUPPORTED;

	return rc;
}

int port_st_max_entries(struct net_port *port)
{
	int rc;

	if (port->drv_ops.st_max_entries)
		rc = port->drv_ops.st_max_entries(port);
	else
		rc = -1;

	return rc;
}

int port_set_fp(struct net_port *port, unsigned int type, struct genavb_fp_config *config)
{
	int rc;

	rtos_mutex_lock(&port->config_mutex, RTOS_WAIT_FOREVER);

	if (port->drv_ops.set_fp)
		rc = port->drv_ops.set_fp(port, type, config);
	else
		rc = -1;

	rtos_mutex_unlock(&port->config_mutex);

	return rc;
}

int port_get_fp(struct net_port *port, unsigned int type, struct genavb_fp_config *config)
{
	int rc;

	rtos_mutex_lock(&port->config_mutex, RTOS_WAIT_FOREVER);

	if (port->drv_ops.get_fp)
		rc = port->drv_ops.get_fp(port, type, config);
	else
		rc = -1;

	rtos_mutex_unlock(&port->config_mutex);

	return rc;
}

int port_set_max_frame_size(struct net_port *port, uint16_t size)
{
	int rc;

	if (port->drv_ops.set_max_frame_size)
		rc = port->drv_ops.set_max_frame_size(port, size);
	else
		rc = -GENAVB_ERR_NOT_SUPPORTED;

	return rc;
}

int port_stats_get_number(struct net_port *port)
{
	return port->drv_ops.stats_get_number(port);
}

int port_stats_get_strings(struct net_port *port, const char **buf, unsigned int buf_len)
{
	return port->drv_ops.stats_get_strings(port, buf, buf_len);
}

int port_stats_get(struct net_port *port, uint64_t *buf, unsigned int buf_len)
{
	return port->drv_ops.stats_get(port, buf, buf_len);
}

unsigned int port_priority_to_traffic_class(struct net_port *port, uint8_t priority)
{
	return port->map[priority & 0x7U];
}

void port_set_priority_to_traffic_class_map(struct net_port *port, unsigned int enabled_tclass, unsigned int enabled_sr_class)
{
	const uint8_t *map;

	map = priority_to_traffic_class_map(enabled_tclass, enabled_sr_class);
	if (!map)
		return;

	memcpy(port->map, map, sizeof(port->map));
}

void port_down(struct net_port *port)
{
	port_tx_lock_all(port);

	if (!port->up)
		goto out;

	port->drv_ops.link_down(port);

	port->up = false;

out:
	port_tx_unlock_all(port);

	os_log(LOG_INFO, "port(%u) down\n", port->index);

	rtos_event_group_set(&port->event_group, PORT_SUCCESS);
}

static void port_phy_callback(void *data, unsigned int type, unsigned int speed, unsigned int duplex)
{
	struct net_port *port = data;
	struct event e;

	port->phy_speed = speed;
	port->phy_duplex = duplex;

	e.type = type;
	e.data = port;

	if (type == EVENT_PHY_DOWN) {
		/* Stop software tx first */
		rtos_mqueue_send(&net_tx_ctx.queue, &e, RTOS_MS_TO_TICKS(10));
		rtos_event_group_wait(&port->event_group, PORT_TX_SUCCESS, true, RTOS_MS_TO_TICKS(10));

		/* Stop driver rx/tx and flush software tx */
		rtos_mqueue_send(&net_rx_ctx.queue, &e, RTOS_MS_TO_TICKS(10));
		rtos_event_group_wait(&port->event_group, PORT_SUCCESS, true, RTOS_MS_TO_TICKS(10));
	} else {
		rtos_mqueue_send(&net_rx_ctx.queue, &e, RTOS_MS_TO_TICKS(10));
		rtos_event_group_wait(&port->event_group, PORT_SUCCESS, true, RTOS_MS_TO_TICKS(10));

		rtos_mqueue_send(&net_tx_ctx.queue, &e, RTOS_MS_TO_TICKS(10));
		rtos_event_group_wait(&port->event_group, PORT_TX_SUCCESS, true, RTOS_MS_TO_TICKS(10));
	}
}

__exit static void __port_pre_exit(struct net_port *port)
{
	if (port->phy_index != -1)
		phy_disconnect(port->phy_index);

	if (port->drv_ops.pre_exit)
		port->drv_ops.pre_exit(port);
}

__init static int __port_post_init(struct net_port *port)
{
	port->hw_clock = hw_clock_get(clock_to_hw_clock(port->clock_local));
	if (!port->hw_clock)
		goto err_clock;

	if (port->drv_ops.post_init) {
		if (port->drv_ops.post_init(port) < 0)
			goto err_init;
	}

	if (port->phy_index != -1) {
		if (phy_connect(port->phy_index, port_phy_callback, port) < 0) {
			os_log(LOG_ERR, "port(%u) phy connect failed\n", port->index);
			goto err_connect;
		}
	} else {
		/* Phy less MAC support */
		port_phy_callback(port, EVENT_PHY_UP, port->phy_speed, port->phy_duplex);
	}

	/* Actual MAC setup is done on phy up (polled from net rx task) */

	return 0;

err_connect:
	__port_pre_exit(port);

err_init:
err_clock:
	return -1;
}

__init int port_post_init(void)
{
	int i, j;

	for (i = 0; i < CFG_PORTS; i++)
		if (__port_post_init(&ports[i]) < 0) {
			os_log(LOG_ERR, "__port_post_init() port(%u) failed\n",
			       ports[i].index);
			goto err;
		}

	return 0;

err:
	for (j = 0; j < i; j++)
		__port_pre_exit(&ports[j]);

	return -1;
}

__exit void port_pre_exit(void)
{
	int i;

	for (i = 0; i < CFG_PORTS; i++)
		__port_pre_exit(&ports[i]);
}

__init static int __port_init(struct net_port *port)
{
	int i;

	os_log(LOG_INIT, "port(%u): %p\n", port->index, port);

	switch (port->drv_type) {
	case ENET_t:
	case ENET_1G_t:
		port->drv_ops.init = enet_init;
		break;

	case ENET_QOS_t:
		port->drv_ops.init = enet_qos_init;
		break;

	case ENETC_1G_t:
	case ENETC_PSEUDO_1G_t:
		port->drv_ops.init = enetc_ep_init;
		break;

	case NETC_SW_t:
		port->drv_ops.init = netc_sw_port_init;
		break;

	default:
		os_log(LOG_ERR, "port(%u) invalid driver type: %u\n", port->index, port->drv_type);
		goto err_type;
		break;
	}

	if (BOARD_NetPort_Get_MAC(port->logical_port->id, port->mac_addr) < 0) {
		os_log(LOG_ERR, "port(%u) get MAC failed\n", port->index);
		goto err_get_mac;
	}

	memset(&port->stats, 0, sizeof(port->stats));

	if (rtos_mutex_init(&port->config_mutex) < 0) {
		os_log(LOG_ERR, "rtos_mutex_init() port(%u) failed\n", port->index);
		goto err_semaphore_create;
	}

	for (i = 0; i < MAX_QUEUES; i++) {
		if (rtos_mutex_init(&port->mutex[i]) < 0) {
			os_log(LOG_ERR, "rtos_mutex_init() port(%u) failed\n", port->index);
			goto err_semaphore_create;
		}
	}

	if (rtos_event_group_init(&port->event_group) < 0) {
		os_log(LOG_ERR, "rtos_event_group_init() port(%u) failed\n", port->index);
		goto err_group_create;
	}

	port->up = false;
	port->tx_up = false;

	port->clock_gptp[0] = logical_port_to_gptp_clock(port->logical_port->id, 0);
	port->clock_gptp[1] = logical_port_to_gptp_clock(port->logical_port->id, 1);
	port->clock_local = logical_port_to_local_clock(port->logical_port->id);
	port->clock_st_psfp = port->clock_gptp[0];

	/* Default mapping of clocks to hardware clocks */
	/* Individual drivers may override it in the driver specific port init function */
	clock_to_hw_clock_set(port->clock_local, HW_CLOCK_PORT_BASE + port->hw_clock_id);
	clock_to_hw_clock_set(port->clock_gptp[0], HW_CLOCK_PORT_BASE + port->hw_clock_id);
	clock_to_hw_clock_set(port->clock_gptp[1], HW_CLOCK_PORT_BASE + port->hw_clock_id);

	if (port->drv_ops.init(port) < 0) {
		os_log(LOG_ERR, "port drv_ops->init() port(%u) failed\n", port->index);
		goto err_drv_init;
	}

	return 0;

err_drv_init:
err_group_create:
err_semaphore_create:
err_get_mac:
err_type:
	return -1;
}

__exit static void __port_exit(struct net_port *port)
{
	port->drv_ops.exit(port);
}

__init int port_init(void)
{
	int i, j;

	logical_port_init();
	phy_init();

	for (i = 0; i < CFG_PORTS; i++)
		if (__port_init(&ports[i]) < 0) {
			os_log(LOG_ERR, "__port_init() port(%u) failed\n",
			       ports[i].index);
			goto err;
		}

	mdio_init();

	return 0;

err:
	for (j = 0; j < i; j++)
		__port_exit(&ports[j]);

	return -1;
}

__exit void port_exit(void)
{
	int i;

	mdio_exit();

	for (i = 0; i < CFG_PORTS; i++)
		__port_exit(&ports[i]);

	phy_exit();
}
