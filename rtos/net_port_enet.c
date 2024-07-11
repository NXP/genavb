/*
 * Copyright 2019-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief RTOS specific Network service implementation
 @details
*/

#include "common/log.h"

#include "net_port_enet.h"
#include "net_port.h"

#if CFG_NUM_ENET_MAC

#include "clock.h"
#include "hw_clock.h"
#include "hw_timer.h"

#include "ptp.h"

#include "fsl_enet.h"

#define NUM_ENET_TYPE 2

#define ENET_RXBD_NUM 20 /* Enough to support 100Mbit/s line rate small packets */
#define ENET_TXBD_NUM 12

AT_NONCACHEABLE_SECTION_ALIGN(static enet_rx_bd_struct_t rx_desc[CFG_NUM_ENET_MAC][ENET_RXBD_NUM], ENET_BUFF_ALIGNMENT);
AT_NONCACHEABLE_SECTION_ALIGN(static enet_tx_bd_struct_t tx_desc[CFG_NUM_ENET_MAC][ENET_TXBD_NUM], ENET_BUFF_ALIGNMENT);

static enet_frame_info_t txFrameInfoArray[CFG_NUM_ENET_MAC][ENET_TXBD_NUM];

SDK_ALIGN(static uint8_t rx_buf[CFG_NUM_ENET_MAC][ENET_RXBD_NUM][SDK_SIZEALIGN(NET_DATA_SIZE, ENET_BUFF_ALIGNMENT)], ENET_BUFF_ALIGNMENT);

#define ENET_NUM_TIMER 3

#define ENET_RING_ID 0

struct enet_timer {
	int id;
	enet_ptp_timer_channel_t channel;
	struct hw_timer hw_timer;
};

struct enet_drv {
	ENET_Type *base;
	enet_handle_t handle;
	enet_ptp_config_t ptp_config;
	enet_config_t config;
	enet_buffer_config_t buffer_config;

	uint32_t enet_clk;

	struct hw_clock clock;
	struct enet_timer timer[ENET_NUM_TIMER];

	unsigned int ptp_inc;
	unsigned int ptp_adj_max;
	struct enet_timer *isr_channel_to_timer[4];
};

void enet_stats_init(struct net_port *);

static struct enet_drv enet_drivers[CFG_NUM_ENET_MAC] = {
	[0] = {
		.config = {
			.macSpecialConfig = kENET_ControlRxPayloadCheckEnable | kENET_ControlVLANTagEnable,
			.interrupt = 0,
			.rxMaxFrameLen = NET_DATA_SIZE - NET_DATA_OFFSET,
			.miiSpeed = 0,
			.miiDuplex = 0,
			.rxAccelerConfig = 0,
		},
		.buffer_config = {
			.rxBdNumber = ENET_RXBD_NUM,
			.txBdNumber = ENET_TXBD_NUM,
			.rxBuffSizeAlign = SDK_SIZEALIGN(NET_DATA_SIZE, ENET_BUFF_ALIGNMENT),
			.txBuffSizeAlign = SDK_SIZEALIGN(NET_DATA_SIZE, ENET_BUFF_ALIGNMENT),
			.rxBdStartAddrAlign = &rx_desc[0][0],
			.txBdStartAddrAlign = &tx_desc[0][0],
			.rxBufferAlign = &rx_buf[0][0][0],
			.txBufferAlign = NULL,
			.rxMaintainEnable = false,
			.txMaintainEnable = false,
			.txFrameInfo = &txFrameInfoArray[0][0],
		},
		.clock = {
			.rate = NSECS_PER_SEC,
			.period = NSECS_PER_SEC,
			.to_ns = {
				.shift = 0,
			},
			.to_cyc = {
				.shift = 0,
			},
		},
	},
#if (CFG_NUM_ENET_MAC > 1)
	[1] = {
		.config = {
			.macSpecialConfig = kENET_ControlRxPayloadCheckEnable | kENET_ControlVLANTagEnable,
			.interrupt = 0,
			.rxMaxFrameLen = NET_DATA_SIZE - NET_DATA_OFFSET,
			.miiSpeed = 0,
			.miiDuplex = 0,
			.rxAccelerConfig = 0,
		},
		.buffer_config = {
			.rxBdNumber = ENET_RXBD_NUM,
			.txBdNumber = ENET_TXBD_NUM,
			.rxBuffSizeAlign = SDK_SIZEALIGN(NET_DATA_SIZE, ENET_BUFF_ALIGNMENT),
			.txBuffSizeAlign = SDK_SIZEALIGN(NET_DATA_SIZE, ENET_BUFF_ALIGNMENT),
			.rxBdStartAddrAlign = &rx_desc[1][0],
			.txBdStartAddrAlign = &tx_desc[1][0],
			.rxBufferAlign = &rx_buf[1][0][0],
			.txBufferAlign = NULL,
			.rxMaintainEnable = false,
			.txMaintainEnable = false,
			.txFrameInfo = &txFrameInfoArray[1][0],
		},
		.clock = {
			.rate = NSECS_PER_SEC,
			.period = NSECS_PER_SEC,
			.to_ns = {
				.shift = 0,
			},
			.to_cyc = {
				.shift = 0,
			},
		},
	},
#endif
};

static struct tx_queue_properties tx_q_capabilites[NUM_ENET_TYPE] = {
	[ENET_t] = {
		.num_queues = 1,
		.queue_prop = {
			TX_QUEUE_FLAGS_STRICT_PRIORITY,
		},
	},
	[ENET_1G_t] = {
		.num_queues = 1,
		.queue_prop = {
			TX_QUEUE_FLAGS_STRICT_PRIORITY,
		},
	},
};

/* Default configuration */
static struct tx_queue_properties tx_q_default_config[NUM_ENET_TYPE] = {
	[ENET_t] = {
		.num_queues = 1,
		.queue_prop = {
			TX_QUEUE_FLAGS_STRICT_PRIORITY,
		},
	},
	[ENET_1G_t] = {
		.num_queues = 1,
		.queue_prop = {
			TX_QUEUE_FLAGS_STRICT_PRIORITY,
		},
	},
};

void *enet_get_handle(unsigned int id)
{
	if (id >= CFG_NUM_ENET_MAC)
		return NULL;

	return enet_drivers[id].base;
}

static uint64_t enet_1588_read_counter(void *priv)
{
	struct enet_drv *drv = (struct enet_drv *)priv;
	enet_ptp_time_t time;

	/*
	 * The nanosecond field of time struct matches the counter value.
	 */
	ENET_Ptp1588GetTimerNoIrqDisable(drv->base, &drv->handle, &time);

	return time.nanosecond;
}

static int enet_1588_clock_adj_freq(void *priv, int32_t ppb)
{
	struct enet_drv *drv = (struct enet_drv *)priv;
	uint32_t pc, inc, cor;

	inc = drv->ptp_inc;
	if (ppb == 0) {
		cor = 0;
		pc = 0;
	} else {
		if (ppb < 0) {
			ppb = -ppb;
			cor = inc - 1;
		} else
			cor = inc + 1;

		if (ppb > drv->ptp_adj_max) {
			return -1;
		}
		/* MCUX SDK drivers configures 1588 counter to wrap every second.
		 * Hence the hardware bugs relative to 1588 counter overflow do not happen
		 * here in comparison to the Linux version.
		 */
		pc = (drv->ptp_config.ptp1588ClockSrc_Hz / ppb) - 1;
	}

	os_log(LOG_DEBUG, "ptp_adjfreq: ppb = %d pc = 0x%x cor = %d\n", cor > inc ? ppb : -ppb, pc, cor);

	ENET_Ptp1588AdjustTimer(drv->base, cor, pc);

	return 0;
}

static int enet_1588_timer_set_next_event(struct hw_timer *hw_timer, uint64_t cycles)
{
	struct enet_timer *enet_timer = container_of(hw_timer, struct enet_timer, hw_timer);
	struct enet_drv *drv = container_of(enet_timer, struct enet_drv, timer[enet_timer->id]);

	ENET_Ptp1588SetChannelCmpValue(drv->base, enet_timer->channel, cycles);

	if (hw_timer_is_pps_enabled(hw_timer))
		ENET_Ptp1588SetChannelMode(drv->base, enet_timer->channel, kENET_PtpChannelToggleCompare, 1);
	else
		ENET_Ptp1588SetChannelMode(drv->base, enet_timer->channel, kENET_PtpChannelSoftCompare, 1);

	return 0;
}

static int enet_1588_timer_cancel_event(struct hw_timer *hw_timer)
{
	struct enet_timer *enet_timer = container_of(hw_timer, struct enet_timer, hw_timer);
	struct enet_drv *drv = container_of(enet_timer, struct enet_drv, timer[enet_timer->id]);

	ENET_Ptp1588SetChannelMode(drv->base, enet_timer->channel, 0, 1);
	ENET_Ptp1588ClearChannelStatus(drv->base, enet_timer->channel);

	return 0;
}

static int enet_1588_isr_register_timer(struct enet_drv *drv, struct enet_timer *timer)
{
	drv->isr_channel_to_timer[timer->channel] = timer;

	return 0;
}

static void enet_1588_interrupt(ENET_Type *base, enet_handle_t *handle)
{
	struct net_port *port = handle->userData;
	struct enet_drv *drv = net_port_drv(port);
	int id = 0;
	struct enet_timer *timer;
	uint32_t status = ENET_Ptp1588GetGlobalStatus(drv->base) &
			  (ENET_TGSR_TF0_MASK |
			   ENET_TGSR_TF1_MASK |
			   ENET_TGSR_TF2_MASK |
			   ENET_TGSR_TF3_MASK);

	while (status) {
		if (status & 0x1) {
			timer = drv->isr_channel_to_timer[id];
			if (timer) {
				ENET_Ptp1588SetChannelMode(drv->base,
							   timer->channel, 0, 1);
				ENET_Ptp1588ClearChannelStatus(drv->base,
							       timer->channel);
				if (timer->hw_timer.func)
					timer->hw_timer.func(timer->hw_timer.data);
				else
					os_log(LOG_ERR, "invalid isr, timer id: %d\n", id);
			}
		}
		status >>= 1;
		id++;
	};
}

static int enet_add_multi(struct net_port *port, uint8_t *addr)
{
	struct enet_drv *drv = net_port_drv(port);

	ENET_AddMulticastGroup(drv->base, addr);

	return 0;
}

static int enet_del_multi(struct net_port *port, uint8_t *addr)
{
	struct enet_drv *drv = net_port_drv(port);

	ENET_LeaveMulticastGroup(drv->base, addr);

	return 0;
}

static int enet_get_rx_frame_size(struct net_port *port, uint32_t *length, uint32_t queue)
{
	struct enet_drv *drv = net_port_drv(port);
	status_t rc;

	rc = ENET_GetRxFrameSize(&drv->handle, length, queue);
	if (rc == kStatus_Success)
		return 1;
	else if (rc == kStatus_ENET_RxFrameEmpty)
		return 0;
	else
		return -1;
}

static int enet_read_frame(struct net_port *port, uint8_t *data, uint32_t length, unsigned int *port_index, uint64_t *ts, uint32_t queue)
{
	struct enet_drv *drv = net_port_drv(port);
	uint32_t rx_ts;
	status_t rc;

	rc = ENET_ReadFrame(drv->base, &drv->handle, data, length, queue, &rx_ts);
	if (rc == kStatus_Success) {
		if (ts)
			*ts = rx_ts;

		*port_index = port->index;

		return 0;
	} else {
		return -1;
	}
}

static int enet_send_frame(struct net_port *port, uint8_t *data, uint32_t length, struct net_tx_desc *desc, uint32_t queue, bool need_ts)
{
	struct enet_drv *drv = net_port_drv(port);
	enet_buffer_struct_t txBuffArray = {
		.buffer = data,
		.length = length,
	};

	enet_tx_frame_struct_t txFrame = {
		.txBuffArray = &txBuffArray,
		.txBuffNum = 1,
		.context = desc,
		.txConfig = {
			.intEnable = true,
			.tsEnable = need_ts,
			.AVBFrameType = queue,
		},
	};

	status_t rc;

	rc = ENET_StartTxFrame(drv->base, &drv->handle, &txFrame, queue);
	if (rc == kStatus_Success)
		return 1;
	else
		return -1;
}

static int enet_clear_dma_isr(struct enet_drv *drv)
{
	uint32_t irq;
	uint32_t mask = kENET_TxBufferInterrupt | kENET_TxFrameInterrupt;

	irq = drv->base->EIR;

	if (irq & (uint32_t)kENET_TxFrameInterrupt) {

		drv->base->EIR = mask;

		return 1;
	}

	return 0;
}

static void enet_tx_cleanup(struct net_port *port)
{
	struct enet_drv *drv = net_port_drv(port);

	/* FIXME: For multiqueue replace ENET_RING_ID by queue id */
	if (enet_clear_dma_isr(drv))
		ENET_ReclaimTxDescriptor(drv->base, &drv->handle, ENET_RING_ID);

}

#if FSL_FEATURE_ENET_QUEUE > 1
static void enet_callback(ENET_Type *base, enet_handle_t *handle, uint32_t ringId, enet_event_t event, enet_frame_info_t *frameInfo, void *userData)
#else
static void enet_callback(ENET_Type *base, enet_handle_t *handle, enet_event_t event, enet_frame_info_t *frameInfo, void *userData)
#endif
{
	struct net_port *port = (struct net_port *)userData;
	struct net_tx_desc *desc;

	if (event == kENET_TxEvent) {
		if (!frameInfo) {
			os_log(LOG_ERR, "Invalid frame info\n");
			return;
		}

		desc = (struct net_tx_desc *)frameInfo->context;
		if (frameInfo->isTsAvail == true) {
			uint64_t cycles, ts;

			cycles = frameInfo->timeStamp.nanosecond;

			ts = hw_clock_cycles_to_time(port->hw_clock, cycles) + port->tx_tstamp_latency;

			ptp_tx_ts(port, ts, desc->priv);
		}
		port_tx_clean_desc(port, (unsigned long)desc);
	}
}

static void enet_link_up(struct net_port *port)
{
	struct enet_drv *drv = net_port_drv(port);

	drv->config.miiSpeed = port->phy_speed;
	drv->config.miiDuplex = port->phy_duplex;

	ENET_Up(drv->base, &drv->handle, &drv->config, &drv->buffer_config, port->mac_addr, drv->enet_clk);
	ENET_Ptp1588ConfigureHandler(drv->base, &drv->handle, &drv->ptp_config);
	ENET_Set1588TimerISRHandler(drv->base, enet_1588_interrupt);
	ENET_DisableInterrupts(drv->base, ENET_TS_INTERRUPT | ENET_TX_INTERRUPT | ENET_ERR_INTERRUPT);
	ENET_SetTxReclaim(&drv->handle, true, ENET_RING_ID);

	ENET_ActiveRead(drv->base);
}

static void enet_link_down(struct net_port *port)
{
	struct enet_drv *drv = net_port_drv(port);

	ENET_Down(drv->base);
}

static int enet_set_tx_queue_config(struct net_port *port,
			     struct tx_queue_properties *tx_q_cfg)
{
	return 0;
}

static int enet_set_tx_idle_slope(struct net_port *port, unsigned int idle_slope, uint32_t queue)
{
	return -1;
}

__exit static void enet_exit(struct net_port *port)
{
	struct enet_drv *drv = net_port_drv(port);

	ENET_Deinit(drv->base);

	hw_clock_unregister(HW_CLOCK_PORT_BASE + port->hw_clock_id);
}

__init int enet_init(struct net_port *port)
{
	struct enet_drv *drv;
	unsigned int i;

	if (port->drv_index >= CFG_NUM_ENET_MAC)
		goto err;

	port->drv_ops.add_multi = enet_add_multi;
	port->drv_ops.del_multi = enet_del_multi;
	port->drv_ops.get_rx_frame_size = enet_get_rx_frame_size;
	port->drv_ops.read_frame = enet_read_frame;
	port->drv_ops.send_frame = enet_send_frame;
	port->drv_ops.tx_cleanup = enet_tx_cleanup;
	port->drv_ops.link_up = enet_link_up;
	port->drv_ops.link_down = enet_link_down;
	port->drv_ops.set_tx_queue_config = enet_set_tx_queue_config;
	port->drv_ops.set_tx_idle_slope = enet_set_tx_idle_slope;
	port->drv_ops.exit = enet_exit;

	port->drv = &enet_drivers[port->drv_index];

	if (enet_set_tx_queue_config(port, &tx_q_default_config[port->drv_type]) < 0) {
		os_log(LOG_ERR, "enet_setup_tx_queue_config() failed\n");
		goto err;
	}

	port->tx_q_cap = &tx_q_capabilites[port->drv_type];
	port->num_tx_q = tx_q_default_config[port->drv_type].num_queues;
	port->num_rx_q = 1;

	drv = net_port_drv(port);
	drv->base = (ENET_Type *)port->base;

	drv->ptp_config.ptp1588ClockSrc_Hz = dev_get_enet_1588_freq(drv->base);
	drv->ptp_inc = NSECS_PER_SEC / drv->ptp_config.ptp1588ClockSrc_Hz;
	drv->ptp_adj_max = drv->ptp_config.ptp1588ClockSrc_Hz / 2;

	os_log(LOG_INIT, "port(%u) enet(%u) ptp 1588 clock: %u Hz, inc: %u\n",
	       port->index, port->drv_index, drv->ptp_config.ptp1588ClockSrc_Hz, drv->ptp_inc);

#if defined(FSL_FEATURE_ENET_HAS_AVB) && FSL_FEATURE_ENET_HAS_AVB
	if (port->drv_type == ENET_1G_t)
		drv->config.miiSpeed = kENET_MiiSpeed1000M;
	else
#endif
		drv->config.miiSpeed = kENET_MiiSpeed100M;

	drv->config.miiMode = port->mii_mode;
	drv->config.miiDuplex = kENET_MiiFullDuplex;
	drv->config.ringNum = 1;
	drv->config.callback = enet_callback;
	drv->config.userData = port;

	drv->enet_clk = dev_get_enet_core_freq(drv->base);

#ifdef BOARD_NET_RX_CACHEABLE
	drv->buffer_config.rxMaintainEnable = true;
#endif
#ifdef BOARD_NET_TX_CACHEABLE
	drv->buffer_config.txMaintainEnable = true;
#endif

	ENET_Init(drv->base, &drv->handle, &drv->config, &drv->buffer_config, port->mac_addr, drv->enet_clk);
	ENET_Ptp1588Configure(drv->base, &drv->handle, &drv->ptp_config);
	ENET_Set1588TimerISRHandler(drv->base, enet_1588_interrupt);
	ENET_SetSMI(drv->base, drv->enet_clk, false);
	if (ENET_SetTxReclaim(&drv->handle, true, ENET_RING_ID) != kStatus_Success) {
		os_log(LOG_ERR, "ENET_SetTxReclaim failed\n");
		goto err;
	}

	drv->clock.read_counter = enet_1588_read_counter;
	drv->clock.adj_freq = enet_1588_clock_adj_freq;
	drv->clock.priv = drv;

	if (hw_clock_register(HW_CLOCK_PORT_BASE + port->hw_clock_id, &drv->clock) < 0) {
		os_log(LOG_ERR, "failed to register hw clock\n");
		goto err;
	}

	for (i = 0; i < ENET_NUM_TIMER; i++) {
		struct hw_timer *hw_timer = &drv->timer[i].hw_timer;
		unsigned int flags = 0;

		if (port->pps_timer_channel >= 0 && port->pps_timer_channel == port->timers[i].channel)
			os_log(LOG_ERR, "pps mode not supported, forced to disabled\n");

		drv->timer[i].id = i;
		drv->timer[i].channel = port->timers[i].channel;

		hw_timer->set_next_event = enet_1588_timer_set_next_event;
		hw_timer->cancel_event = enet_1588_timer_cancel_event;

		if (hw_timer_register(HW_CLOCK_PORT_BASE + port->hw_clock_id, hw_timer, flags) < 0) {
			os_log(LOG_ERR, "%s : failed to register hw timer\n", __func__);
			continue;
		}

		enet_1588_isr_register_timer(drv, &drv->timer[i]);
	}

	/* Enable MIB counters */
	drv->base->MIBC = ENET_MIBC_MIB_DIS(0);

	enet_stats_init(port);

	return 0;

err:
	return -1;
}
#else
__init int enet_init(struct net_port *port) { return -1; }
#endif /* CFG_NUM_ENET_MAC */
