/*
 * Copyright 2020-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief RTOS specific Network service implementation
 @details
*/

#include "genavb/qos.h"
#include "common/log.h"
#include "os/stdlib.h"
#include "os/timer.h"

#include "net_port_enet_qos.h"

#if CFG_NUM_ENET_QOS_MAC

#include "net_port.h"
#include "fp.h"

#include "net_tx.h"
#include "clock.h"
#include "hw_clock.h"
#include "hw_timer.h"

#include "fsl_phy.h"

#include "ptp.h"

#ifdef BOARD_NET_TX_CACHEABLE
#include "fsl_cache.h"
#endif
#include "fsl_enet_qos.h"

/* Tx settings */

#define ENET_QOS_TXQ_MAX  5
#define ENET_QOS_CBS_MAX  2
#define ENET_QOS_TXBD_NUM 12

static enet_qos_frame_info_t tx_dirty[ENET_QOS_TXQ_MAX][ENET_QOS_TXBD_NUM];
AT_NONCACHEABLE_SECTION_ALIGN(static enet_qos_tx_bd_struct_t g_txBuffDescrip[ENET_QOS_TXQ_MAX][ENET_QOS_TXBD_NUM], ENET_QOS_BUFF_ALIGNMENT);

/* Rx settings */

#define ENET_QOS_RXQ_MAX 4

#if ENET_QOS_RXQ_MAX < (CFG_SR_CLASS_MAX + 2)
#error "Invalid ENET_QOS_RXQ_MAX"
#endif

#define ENET_QOS_BE_RXBD_NUM	20 /* Enough to support 100Mbit/s line rate small packets and 1 Gbit/s line rate big packets */
#define ENET_QOS_SR_RXBD_NUM	16 /* Enough to support 16 streams SR Class A */
#define ENET_QOS_OTHER_RXBD_NUM 8

#if CFG_SR_CLASS_MAX > 0
#define ENET_QOS_RXBD_NUM (ENET_QOS_BE_RXBD_NUM + (ENET_QOS_RXQ_MAX - 2) * ENET_QOS_OTHER_RXBD_NUM + ENET_QOS_SR_RXBD_NUM)
#else
#define ENET_QOS_RXBD_NUM (ENET_QOS_BE_RXBD_NUM + (ENET_QOS_RXQ_MAX - 1) * ENET_QOS_OTHER_RXBD_NUM)
#endif

AT_NONCACHEABLE_SECTION_ALIGN(static enet_qos_rx_bd_struct_t g_rxBuffDescrip[ENET_QOS_RXBD_NUM], ENET_QOS_BUFF_ALIGNMENT);

SDK_ALIGN(static uint8_t rx_buf[ENET_QOS_RXBD_NUM][SDK_SIZEALIGN(NET_DATA_SIZE, ENET_QOS_BUFF_ALIGNMENT)], ENET_QOS_BUFF_ALIGNMENT);
static uint32_t rx_buf_addr[ENET_QOS_RXBD_NUM];

#define PTP_TS_RX_BUF_NUM 12

#define ENET_QOS_NUM_BUF_CFG MAX(ENET_QOS_RXQ_MAX, ENET_QOS_TXQ_MAX)

#define ENET_QOS_NUM_TIMER 4

#define ENET_QOS_PPS_WIDTH_NS 1000

/* AVB CBS */
#define AVB_HI_CREDIT 0x08000000
#define AVB_LO_CREDIT 0x18000000
/* Transmit clock cycle: 40 ns for 100 Mbps and 8 ns for 1000 Mbps */
#define AVB_IDLE_SLOPE_CYCLE_FACTOR_100M 25000000 /* 1/40ns */
#define AVB_IDLE_SLOPE_CYCLE_FACTOR_1000M 125000000 /* 1/8ns */
/* Max trasmit rate */
#define AVB_MAX_PORT_TRANSIT_RATE_100M  (((uint64_t)100000000 * 1024) / AVB_IDLE_SLOPE_CYCLE_FACTOR_100M)
#define AVB_MAX_PORT_TRANSIT_RATE_1000M (((uint64_t)1000000000 * 1024) / AVB_IDLE_SLOPE_CYCLE_FACTOR_1000M)

/* RX parser helper defines */
#define INDEX_VLAN_LABEL		0
#define INDEX_VLAN_PCP			(INDEX_VLAN_LABEL + 1)
#define INDEX_MULTICAST_RESERVED	(INDEX_VLAN_PCP + 8)
#define INDEX_AVDECC			(INDEX_MULTICAST_RESERVED + 2)
#define INDEX_OTHER			(INDEX_AVDECC + 1)
#define RX_PARSER_SIZE			(INDEX_OTHER + 1)

#define ETHERTYPE_MATCH(v)	.matchData = htonl((v) << 16), \
				.matchEnable = htonl(0xffff << 16), \
				.frameOffset = 12 / 4

#define VLAN_PCP_MATCH(v)	.matchData = htonl((v) << 13), \
				.matchEnable = htonl(0x7 << 13), \
				.frameOffset = 12 / 4

#define DA_UPPER_MATCH(v, m)	.matchData = htonl((uint32_t)((v) >> 16)), \
				.matchEnable = htonl((uint32_t)((m) >> 16)), \
				.frameOffset = 0

#define DA_LOWER_MATCH(v, m)	.matchData = htonl(((uint32_t)((v) & 0xffff) << 16)), \
				.matchEnable = htonl(((uint32_t)((m) & 0xffff) << 16)), \
				.frameOffset = 4 / 4

struct enet_qos_timer {
	int id;
	struct hw_timer hw_timer;
	enet_qos_ptp_pps_instance_t instance;
};

struct enet_qos_drv {
	ENET_QOS_Type *base;
	enet_qos_handle_t handle;
	enet_qos_ptp_config_t ptp_config;
	enet_qos_config_t config;
	enet_qos_buffer_config_t buffer_config[ENET_QOS_NUM_BUF_CFG];
	enet_qos_multiqueue_config_t multi_queue;
	enet_qos_cbs_config_t cbs_config[ENET_QOS_CBS_MAX];
	enet_qos_rxp_config_t rxp_config[RX_PARSER_SIZE];

	uint32_t ptp_ref_clk_freq;	/* ptp reference clock frequency, input to system time module */
	uint32_t ptp_clk_freq;		/* ptp/system clock frequency, used for packet timestamping and system time */

	struct hw_clock clock;

	struct enet_qos_timer timer[ENET_QOS_NUM_TIMER - 1]; //one is dedicated to MCR
	struct enet_qos_timer *isr_channel_to_timer[ENET_QOS_NUM_TIMER];

	bool st_enabled;
	bool fp_enabled;
	unsigned int preemptable_mask;
	unsigned int closed_mask;
	uint32_t cycle_time_p;
	uint32_t cycle_time_q;

	struct fp_verify_sm fp;
	struct os_timer fp_timer;

	void (*clock_fix)(void *base, enet_qos_mii_speed_t miiSpeed);
};

void enet_qos_stats_init(struct net_port *);

static struct enet_qos_drv enet_driver = {
	.config = {
		.specialControl = kENET_QOS_HashMulticastEnable | kENET_QOS_StoreAndForward,
		.miiSpeed = 0,
		.miiDuplex = 0,
	},
	.buffer_config = {
		{
			.txRingLen = ENET_QOS_TXBD_NUM,
			.txDescStartAddrAlign = &g_txBuffDescrip[0][0],
			.txDescTailAddrAlign = &g_txBuffDescrip[0][ENET_QOS_TXBD_NUM],
			.txDirtyStartAddr = tx_dirty[0],
			.rxBuffSizeAlign = SDK_SIZEALIGN(NET_DATA_SIZE, ENET_QOS_BUFF_ALIGNMENT),
		},
		{
			.txRingLen = ENET_QOS_TXBD_NUM,
			.txDescStartAddrAlign = &g_txBuffDescrip[1][0],
			.txDescTailAddrAlign = &g_txBuffDescrip[1][ENET_QOS_TXBD_NUM],
			.txDirtyStartAddr = tx_dirty[1],
			.rxBuffSizeAlign = SDK_SIZEALIGN(NET_DATA_SIZE, ENET_QOS_BUFF_ALIGNMENT),
		},
		{
			.txRingLen = ENET_QOS_TXBD_NUM,
			.txDescStartAddrAlign = &g_txBuffDescrip[2][0],
			.txDescTailAddrAlign = &g_txBuffDescrip[2][ENET_QOS_TXBD_NUM],
			.txDirtyStartAddr = tx_dirty[2],
			.rxBuffSizeAlign = SDK_SIZEALIGN(NET_DATA_SIZE, ENET_QOS_BUFF_ALIGNMENT),
		},
		{
			.txRingLen = ENET_QOS_TXBD_NUM,
			.txDescStartAddrAlign = &g_txBuffDescrip[3][0],
			.txDescTailAddrAlign = &g_txBuffDescrip[3][ENET_QOS_TXBD_NUM],
			.txDirtyStartAddr = tx_dirty[3],
			.rxBuffSizeAlign = SDK_SIZEALIGN(NET_DATA_SIZE, ENET_QOS_BUFF_ALIGNMENT),
		},
		{
			.txRingLen = ENET_QOS_TXBD_NUM,
			.txDescStartAddrAlign = &g_txBuffDescrip[4][0],
			.txDescTailAddrAlign = &g_txBuffDescrip[4][ENET_QOS_TXBD_NUM],
			.txDirtyStartAddr = tx_dirty[4],
		},
	},
	.ptp_config = {
		.tsRollover = kENET_QOS_DigitalRollover,
		.fineUpdateEnable = true,
		.ptp1588V2Enable = false,
	},
	.multi_queue = {
		.burstLen = kENET_QOS_BurstLen16,
		.mtltxSche = kENET_QOS_txStrPrio,
		.mtlrxSche = kENET_QOS_rxStrPrio,
		.rxQueueConfig = {
			{
				.mapChannel = 0x0,
			},
			{
				.mapChannel = 0x1,
			},
			{
				.mapChannel = 0x2,
			},
			{
				.mapChannel = 0x3,
			},
			{
				.mapChannel = 0x4,
			},
		},
	},
	.clock = {
		.rate = NSECS_PER_SEC,
		.period = ((1ULL << 32) - 1) * NSECS_PER_SEC,
		.to_ns = {
			.shift = 0,
		},
		.to_cyc = {
			.shift = 0,
		},
	},
	.rxp_config = {
		/* VLAN label match */
		[INDEX_VLAN_LABEL] = {
			ETHERTYPE_MATCH(ETHERTYPE_VLAN),
			.nextControl = 1,
			.okIndex = INDEX_MULTICAST_RESERVED,
		},

		/* VLAN PCP matching */
		[INDEX_VLAN_PCP] = {
			VLAN_PCP_MATCH(7),
			.acceptFrame = 1,
		},
		[INDEX_VLAN_PCP + 1] = {
			VLAN_PCP_MATCH(6),
			.acceptFrame = 1,
		},
		[INDEX_VLAN_PCP + 2] = {
			VLAN_PCP_MATCH(5),
			.acceptFrame = 1,
		},
		[INDEX_VLAN_PCP + 3] = {
			VLAN_PCP_MATCH(4),
			.acceptFrame = 1,
		},
		[INDEX_VLAN_PCP + 4] = {
			VLAN_PCP_MATCH(3),
			.acceptFrame = 1,
		},
		[INDEX_VLAN_PCP + 5] = {
			VLAN_PCP_MATCH(2),
			.acceptFrame = 1,
		},
		[INDEX_VLAN_PCP + 6] = {
			VLAN_PCP_MATCH(1),
			.acceptFrame = 1,
		},
		[INDEX_VLAN_PCP + 7] = {
			VLAN_PCP_MATCH(0),
			.acceptFrame = 1,
			.nextControl = 1,
			.okIndex = INDEX_OTHER,
		},

		/* Multicast control (MSRP, MVRP, PTP, ...) 01:80:C2:00:00:xx*/
		[INDEX_MULTICAST_RESERVED] = {
			DA_UPPER_MATCH(0x0180C2000000, 0xffffffffff00),
			.nextControl = 1,
			.okIndex = INDEX_AVDECC,
		},

		/* Multicast control (MSRP, MVRP, PTP, ...) */
		[INDEX_MULTICAST_RESERVED + 1] = {
			DA_LOWER_MATCH(0x0180C2000000, 0xffffffffff00),
			.acceptFrame = 1,
			.nextControl = 1,
			.okIndex = INDEX_OTHER,
		},

		/* Avdecc ethertype */
		[INDEX_AVDECC] = {
			ETHERTYPE_MATCH(ETHERTYPE_AVTP),
			.acceptFrame = 1,
		},

		[INDEX_OTHER] = {
			.matchEnable = 0,
			.acceptFrame = 1,
		},
	},
#ifdef BOARD_ENET_QOS0_CLOCK_FIX
	.clock_fix = BOARD_ENET_QOS0_CLOCK_FIX,
#endif
};

static struct tx_queue_properties tx_q_capabilites = {
	.num_queues = 5,
	.queue_prop = {
		TX_QUEUE_FLAGS_STRICT_PRIORITY,
		TX_QUEUE_FLAGS_STRICT_PRIORITY,
		TX_QUEUE_FLAGS_STRICT_PRIORITY,
		TX_QUEUE_FLAGS_STRICT_PRIORITY | TX_QUEUE_FLAGS_CREDIT_SHAPER,
		TX_QUEUE_FLAGS_STRICT_PRIORITY | TX_QUEUE_FLAGS_CREDIT_SHAPER,
	},
};

static struct tx_queue_properties tx_q_default_config = {
	.num_queues = 5,
	.queue_prop = {
		TX_QUEUE_FLAGS_STRICT_PRIORITY,
		TX_QUEUE_FLAGS_STRICT_PRIORITY,
		TX_QUEUE_FLAGS_STRICT_PRIORITY,
		TX_QUEUE_FLAGS_CREDIT_SHAPER,
		TX_QUEUE_FLAGS_CREDIT_SHAPER,
	},
};

void *enet_qos_get_handle(unsigned int id)
{
	if (id) /* only instance 0 supported in current enet_qos */
		return NULL;

	return enet_driver.base;
}

static void enet_qos_fp_enable(void *arg)
{
	struct enet_qos_drv *drv = arg;

	ENET_QOS_FpeEnable(drv->base);
}

static void enet_qos_fp_disable(void *arg)
{
	struct enet_qos_drv *drv = arg;

	ENET_QOS_FpeDisable(drv->base);
}

static void enet_qos_fp_start_timer(void *arg, unsigned int ms)
{
	struct enet_qos_drv *drv = arg;

	if (os_timer_start(&drv->fp_timer, ms * NSECS_PER_MS, 0, 1, 0) < 0)
		os_log(LOG_ERR, "os_timer_start() failed\n");
}

static void enet_qos_fp_send_verify(void *arg)
{
	struct enet_qos_drv *drv = arg;

	drv->base->MAC_FPE_CTRL_STS |= ENET_QOS_MAC_FPE_CTRL_STS_SVER_MASK;
}

static void enet_qos_fp_send_response(void *arg)
{
	struct enet_qos_drv *drv = arg;

	drv->base->MAC_FPE_CTRL_STS |= ENET_QOS_MAC_FPE_CTRL_STS_SRSP_MASK;
}

static void enet_qos_fp_schedule(struct enet_qos_drv *drv, bool isr)
{
	struct event e;

	e.type = EVENT_FP_SCHED;
	e.data = &drv->fp;

	if (isr) {
		bool yield = false;

		rtos_mqueue_send_from_isr(&net_tx_ctx.queue, &e, RTOS_NO_WAIT, &yield);

		rtos_yield_from_isr(yield);
	} else {
		rtos_mqueue_send(&net_tx_ctx.queue, &e, RTOS_MS_TO_TICKS(10));
	}
}

static void enet_qos_fp_timer(struct os_timer *t, int count)
{
	struct enet_qos_drv *drv = container_of(t, struct enet_qos_drv, fp_timer);

	net_port_fp_event(&drv->fp, FP_EVENT_TIMER_EXPIRED);
	enet_qos_fp_schedule(drv, false);
}

static int enet_qos_fp_init(struct enet_qos_drv *drv)
{
	if (os_timer_create(&drv->fp_timer, OS_CLOCK_SYSTEM_MONOTONIC_COARSE, 0, enet_qos_fp_timer, 0) < 0)
		goto err_timer;

	drv->fp.send_verify = enet_qos_fp_send_verify;
	drv->fp.send_response = enet_qos_fp_send_response;
	drv->fp.start_timer = enet_qos_fp_start_timer;
	drv->fp.enable = enet_qos_fp_enable;
	drv->fp.disable = enet_qos_fp_disable;

	net_port_fp_init(&drv->fp, drv);

	return 0;

err_timer:
	return -1;
}

static void enet_qos_fp_exit(struct enet_qos_drv *drv)
{
	os_timer_destroy(&drv->fp_timer);
}

static uint64_t enet_qos_1588_read_counter(void *priv)
{
	struct enet_qos_drv *drv = (struct enet_qos_drv *)priv;

	uint64_t seconds;
	uint32_t nanoseconds;

	ENET_QOS_Ptp1588GetTimerNoIRQDisable(drv->base, &seconds, &nanoseconds);

	return (seconds * NSECS_PER_SEC) + nanoseconds;
}

static int enet_qos_1588_clock_adj_freq(void *priv, int32_t ppb)
{
	struct enet_qos_drv *drv = (struct enet_qos_drv *)priv;
	uint32_t addend = drv->ptp_config.defaultAddend;
	uint64_t diff;
	status_t rc;

	diff = ((uint64_t)drv->ptp_config.defaultAddend * os_abs(ppb)) / 1000000000ULL;

	if (ppb > 0)
		addend += diff;
	else
		addend -= diff;

	rc = ENET_QOS_Ptp1588CorrectTimerInFine(drv->base, addend);
	if (rc != kStatus_Success)
		return -1;

	return 0;
}

static void enet_qos_1588_timer_init(struct enet_qos_drv *drv, struct enet_qos_timer *timer)
{
	unsigned int width;

	width = (uint64_t)ENET_QOS_PPS_WIDTH_NS / ((uint64_t)NSECS_PER_SEC / drv->config.csrClock_Hz) - 1;

	/* The interval value must be greater than width value */
	ENET_QOS_Ptp1588PpsSetWidth(drv->base, timer->instance, width);
	ENET_QOS_Ptp1588PpsSetInterval(drv->base, timer->instance, 2 * width);

	drv->isr_channel_to_timer[timer->instance] = timer;
}

static int enet_qos_1588_timer_set_next_event(struct hw_timer *hw_timer, uint64_t cycles)
{
	struct enet_qos_timer *timer = container_of(hw_timer, struct enet_qos_timer, hw_timer);
	struct enet_qos_drv *drv = container_of(timer, struct enet_qos_drv,
						timer[timer->id]);
	uint32_t seconds, nanoseconds;
	status_t rc;

	seconds = cycles / NSECS_PER_SEC;
	nanoseconds = cycles - seconds * NSECS_PER_SEC;

	rc = ENET_QOS_Ptp1588PpsSetTrgtTime(drv->base, timer->instance, seconds, nanoseconds);
	if (rc != kStatus_Success)
		return -1;

	if (hw_timer_is_pps_enabled(hw_timer)) {
		if (ENET_Ptp1588PpsControl(drv->base, timer->instance, kENET_QOS_PtpPpsTrgtModeIntSt, kENET_QOS_PtpPpsCmdSSP) != kStatus_Success)
			return -1;
	} else {
		if (ENET_Ptp1588PpsControl(drv->base, timer->instance, kENET_QOS_PtpPpsTrgtModeOnlyInt, 0) != kStatus_Success)
			return -1;
	}

	return 0;
}

static int enet_qos_1588_timer_cancel_event(struct hw_timer *hw_timer)
{
	struct enet_qos_timer *timer = container_of(hw_timer, struct enet_qos_timer, hw_timer);
	struct enet_qos_drv *drv = container_of(timer, struct enet_qos_drv,
						timer[timer->id]);
	status_t rc;

	rc = ENET_QOS_Ptp1588PpsSetTrgtTime(drv->base, timer->instance, 0, 0);
	if (rc != kStatus_Success)
		return -1;

	return 0;
}

static void enet_qos_1588_interrupt(struct net_port *port, struct enet_qos_drv *drv)
{
	/* Status cleared on read */
	uint32_t status = drv->base->MAC_TIMESTAMP_STATUS;
	int id = 0;
	struct enet_qos_timer *timer;

	status >>= 1;

	while (status) {
		if (status & 0x1) {
			timer = drv->isr_channel_to_timer[id];
			if (timer) {
				if (timer->hw_timer.func)
					timer->hw_timer.func(timer->hw_timer.data);
				else
					os_log(LOG_ERR, "invalid isr, timer id: %d\n", id);
			}
		}

		if (!id)
			status >>= 3;
		else
			status >>= 2;

		id++;
	}
}

static void enet_qos_fp_interrupt(struct net_port *port, struct enet_qos_drv *drv)
{
	/* Status cleared on read */
	uint32_t status = drv->base->MAC_FPE_CTRL_STS;

	if (status & ENET_QOS_MAC_FPE_CTRL_STS_TRSP_MASK)
		net_port_fp_event(&drv->fp, FP_EVENT_TRANSMITTED_RESPONSE);

	if (status & ENET_QOS_MAC_FPE_CTRL_STS_TVER_MASK)
		net_port_fp_event(&drv->fp, FP_EVENT_TRANSMITTED_VERIFY);

	if (status & ENET_QOS_MAC_FPE_CTRL_STS_RRSP_MASK)
		net_port_fp_event(&drv->fp, FP_EVENT_RECEIVED_RESPONSE);

	if (status & ENET_QOS_MAC_FPE_CTRL_STS_RVER_MASK)
		net_port_fp_event(&drv->fp, FP_EVENT_RECEIVED_VERIFY);

	enet_qos_fp_schedule(drv, true);
}

static int enet_qos_add_multi(struct net_port *port, uint8_t *addr)
{
	struct enet_qos_drv *drv = net_port_drv(port);

	ENET_QOS_AddMulticastGroup(drv->base, addr);

	return 0;
}

static int enet_qos_del_multi(struct net_port *port, uint8_t *addr)
{
	struct enet_qos_drv *drv = net_port_drv(port);

	ENET_QOS_LeaveMulticastGroup(drv->base, addr);

	return 0;
}

static int enet_qos_get_rx_frame_size(struct net_port *port, uint32_t *length, uint32_t queue)
{
	struct enet_qos_drv *drv = net_port_drv(port);
	status_t rc;

	rc = ENET_QOS_GetRxFrameSize(drv->base, &drv->handle, length, queue);
	if (rc == kStatus_Success)
		return 1;
	else if (rc == kStatus_ENET_QOS_RxFrameEmpty)
		return 0;
	else
		return -1;
}

static int enet_qos_read_frame(struct net_port *port, uint8_t *data,
			uint32_t length, unsigned int *port_index, uint64_t *ts, uint32_t queue)
{
	struct enet_qos_drv *drv = net_port_drv(port);
	enet_qos_ptp_time_t rx_ts;
	status_t rc;

	rc = ENET_QOS_ReadFrame(drv->base, &drv->handle, data,
				length, queue, &rx_ts);
	if (rc == kStatus_Success) {
		if (ts)
			*ts = rx_ts.second * NSECS_PER_SEC + rx_ts.nanosecond;

		*port_index = port->index;

		return 0;
	} else {
		return -1;
	}
}

static int enet_qos_send_frame(struct net_port *port, uint8_t *data,
			uint32_t length, struct net_tx_desc *desc, uint32_t queue, bool need_ts)
{
	struct enet_qos_drv *drv = net_port_drv(port);
	status_t rc;

#ifdef BOARD_NET_TX_CACHEABLE
	DCACHE_CleanByRange((uintptr_t)data, length);
#endif /* BOARD_NET_TX_CACHEABLE */

#if FSL_ENET_QOS_DRIVER_VERSION < MAKE_VERSION(2, 6, 0)
	rc = ENET_QOS_SendFrame(drv->base, &drv->handle, data, length, queue, need_ts, desc);
#else
	rc = ENET_QOS_SendFrame(drv->base, &drv->handle, data, length, queue, need_ts, desc, kENET_QOS_TxOffloadDisable);
#endif
	if (rc == kStatus_Success)
		return 1;
	else
		return -1;
}

static int enet_qos_clear_dma_isr(struct enet_qos_drv *drv, unsigned int chan)
{
	uint32_t flag = drv->base->DMA_CH[chan].DMA_CHX_STAT;

	if (flag & ENET_QOS_DMA_CHX_STAT_TI_MASK) {
		/* Clear status */
		drv->base->DMA_CH[chan].DMA_CHX_STAT = ENET_QOS_DMA_CHX_STAT_TI_MASK |
							       ENET_QOS_DMA_CHX_STAT_NIS_MASK;

		return 1;
	}

	return 0;
}

static void enet_qos_tx_cleanup(struct net_port *port)
{
	struct enet_qos_drv *drv = net_port_drv(port);
	unsigned int chan;

	for (chan = 0; chan < port->num_tx_q; chan++) {
		if (enet_qos_clear_dma_isr(drv, chan))
			ENET_QOS_ReclaimTxDescriptor(drv->base, &drv->handle, chan);
	}
}

static void enet_qos_interrupt(ENET_QOS_Type *base, enet_qos_handle_t *handle)
{
	struct net_port *port = handle->userData;

	if (base->DMA_INTERRUPT_STATUS & ENET_QOS_DMA_INTERRUPT_STATUS_MACIS_MASK) {
		struct enet_qos_drv *drv = net_port_drv(port);
		uint32_t status = drv->base->MAC_INTERRUPT_STATUS;

		if (status & ENET_QOS_MAC_INTERRUPT_STATUS_TSIS_MASK)
			enet_qos_1588_interrupt(port, drv);

		if (status & ENET_QOS_MAC_INTERRUPT_STATUS_FPEIS_MASK)
			enet_qos_fp_interrupt(port, drv);
	}

#if defined __CORTEX_M && (__CORTEX_M == 4U)
	__DSB();
#endif
}

static void enet_qos_callback(ENET_QOS_Type *base, enet_qos_handle_t *handle,
			    enet_qos_event_t event, uint8_t channel, void *userData)
{
	struct net_port *port = (struct net_port *)userData;
	struct enet_qos_drv *drv = net_port_drv(port);
	enet_qos_frame_info_t sent_frame_info;
	struct net_tx_desc *desc;

	if (event == kENET_QOS_TxIntEvent) {
		if (channel >= port->num_tx_q) {
			os_log(LOG_ERR, "Invalid channel\n");
			return;
		}

		ENET_QOS_GetTxFrame(&drv->handle, &sent_frame_info, channel);

		desc = (struct net_tx_desc *)sent_frame_info.context;
		if (desc) {
			if (sent_frame_info.isTsAvail == true) {
				uint64_t cycles, ts;
				cycles = sent_frame_info.timeStamp.second * NSECS_PER_SEC + sent_frame_info.timeStamp.nanosecond;

				ts = hw_clock_cycles_to_time(port->hw_clock, cycles) + port->tx_tstamp_latency;

				ptp_tx_ts(port, ts, desc->priv);
			}
			port_tx_clean_desc(port, (unsigned long)desc);
		} else {
			os_log(LOG_ERR, "Invalid dirty tx desc\n");
		}
	}
}

static void enet_qos_link_up(struct net_port *port)
{
	struct enet_qos_drv *drv = net_port_drv(port);

	/* Use the actual speed and duplex when phy success
	 * to finish the autonegotiation.
	 */
	switch (port->phy_speed) {
	case kPHY_Speed10M:
		drv->config.miiSpeed = kENET_QOS_MiiSpeed10M;
		break;
	case kPHY_Speed100M:
		drv->config.miiSpeed = kENET_QOS_MiiSpeed100M;
		break;
	case kPHY_Speed1000M:
		drv->config.miiSpeed = kENET_QOS_MiiSpeed1000M;
		break;
	default:
		break;
	}

	if (drv->clock_fix)
		drv->clock_fix(drv->base, drv->config.miiSpeed);

	drv->config.miiDuplex = (enet_qos_mii_duplex_t)port->phy_duplex;

	ENET_QOS_Up(drv->base, &drv->config, port->mac_addr, 1, drv->ptp_ref_clk_freq);

	ENET_QOS_EnableRxParser(drv->base, true);

	ENET_QOS_DisableInterrupts(drv->base, kENET_QOS_DmaTx);

	/* Active TX/RX. */
	ENET_QOS_StartRxTx(drv->base, port->num_tx_q, port->num_rx_q);

	drv->base->MAC_INTERRUPT_ENABLE |= ENET_QOS_MAC_INTERRUPT_ENABLE_FPEIE_MASK;

	net_port_fp_event(&drv->fp, FP_EVENT_LINK_OK);
	net_port_fp_schedule(&drv->fp);
}

static void enet_qos_link_down(struct net_port *port)
{
	struct enet_qos_drv *drv = net_port_drv(port);

	net_port_fp_event(&drv->fp, FP_EVENT_LINK_FAIL);
	net_port_fp_schedule(&drv->fp);

	ENET_QOS_Down(drv->base);
}

static int enet_qos_st_fp_check_config(bool fp_enabled, bool st_enabled, unsigned int preemptable_mask, unsigned int closed_mask)
{
	if (!fp_enabled || !st_enabled)
		goto exit;

	/* q0 must be preemptable */
	if (!(preemptable_mask & 0x1))
		goto err;

	/* Preemptable queues must always be in open state */
	if (preemptable_mask & closed_mask)
		goto err;

exit:
	return 0;

err:
	return -1;
}

static int enet_qos_set_st_config(struct net_port *port, struct genavb_st_config *config)
{
	struct enet_qos_drv *drv = net_port_drv(port);
	enet_qos_est_gcl_t gcl;
	enet_qos_est_gate_op_t *drv_list = NULL, *drv_list_op;
	struct genavb_st_gate_control_entry *cfg_list = config->control_list;
	unsigned int hold = 0, closed_mask = 0;
	uint32_t cycle_time;
	uint32_t nsecs = 0, secs = 0;
	status_t rc_sdk;
	int rc = 0, i;

	if (!config->enable) {
		gcl.enable = 0;
		goto program;
	}

	gcl.enable = 1;

	if (config->list_length > ENET_QOS_EST_DEP) {
		os_log(LOG_ERR, "list_length: %u invalid (max: %u)\n",
		       config->list_length, ENET_QOS_EST_DEP);
		rc = -1;
		goto exit;
	}

	drv_list = rtos_malloc(config->list_length * sizeof(*drv_list));
	if (!drv_list) {
		os_log(LOG_ERR, "rtos_malloc() failed\n");
		rc = -1;
		goto exit;
	}

	/* The SDK driver does a sanity check on gates and time intervals, so
	 * no checks are being done here.
	 */
	drv_list_op = drv_list;

	for (i = 0; i < config->list_length; i++, drv_list_op++, cfg_list++) {

		if (cfg_list->operation == GENAVB_ST_SET_AND_HOLD_MAC) {
			hold = 1;
		} else if (cfg_list->operation == GENAVB_ST_SET_AND_RELEASE_MAC) {
			hold = 0;
		} else if (cfg_list->operation != GENAVB_ST_SET_GATE_STATES) {
			os_log(LOG_ERR, "operation not supported \n");
			rc = -1;
			goto exit;
		}

		/* Keep track of queues in closed state */
		closed_mask |= (~cfg_list->gate_states) & 0xff;

		/* When fp is enabled, q0 bit is determined by hold/release */
		if (drv->fp_enabled) {
			if (hold)
				drv_list_op->gate = cfg_list->gate_states | 0x1;
			else
				drv_list_op->gate = cfg_list->gate_states & ~0x1;
		} else {
			drv_list_op->gate = cfg_list->gate_states;
		}

		drv_list_op->interval = cfg_list->time_interval;

		os_log(LOG_DEBUG, "op: %u, gate: %x, interval: %u\n",
		       cfg_list->operation, drv_list_op->gate, drv_list_op->interval);
	}

	if (enet_qos_st_fp_check_config(drv->fp_enabled, true, drv->preemptable_mask, closed_mask) < 0) {
		os_log(LOG_ERR, "Scheduled traffic and frame preemption incompatible configuration\n");
		rc = -1;
		goto exit;
	}

	cycle_time = ((uint64_t)NSECS_PER_SEC * config->cycle_time_p) / config->cycle_time_q;
	drv->cycle_time_p = config->cycle_time_p;
	drv->cycle_time_q = config->cycle_time_q;

	secs = config->base_time / NSECS_PER_SEC;
	nsecs = config->base_time - secs * NSECS_PER_SEC;
	gcl.baseTime = ((uint64_t)secs << 32) | nsecs;
	os_log(LOG_DEBUG, "base time secs: %u, nsecs: %u\n", secs, nsecs);

	secs = cycle_time / NSECS_PER_SEC;
	nsecs = cycle_time - secs * NSECS_PER_SEC;
	gcl.cycleTime = ((uint64_t)secs << 32) | nsecs;
	os_log(LOG_DEBUG, "cycle time secs: %u, nsecs: %u\n", secs, nsecs);

	gcl.extTime = config->cycle_time_ext;
	gcl.numEntries = config->list_length;
	gcl.opList = drv_list;

program:
	rc_sdk = ENET_QOS_EstProgramGcl(drv->base, &gcl, drv->ptp_clk_freq);
	if (rc_sdk != kStatus_Success) {
		os_log(LOG_ERR, "ENET_QOS_EstProgramGcl() failed rc = %d\n", rc_sdk);
		rc = -1;
		goto exit;
	}

	if (config->enable) {
		drv->st_enabled = true;
		drv->closed_mask = closed_mask;
	} else {
		drv->st_enabled = false;
	}

exit:
	if (drv_list)
		rtos_free(drv_list);

	return rc;
}

static int enet_qos_get_st_config(struct net_port *port, genavb_st_config_type_t type,
			   struct genavb_st_config *config, unsigned int list_length)
{
	struct enet_qos_drv *drv = net_port_drv(port);
	enet_qos_est_gcl_t gcl;
	enet_qos_est_gate_op_t *drv_list = NULL, *drv_list_op;
	struct genavb_st_gate_control_entry *cfg_list = config->control_list;
	status_t rc_sdk;
	bool hw_list = true;
	int rc = 0, i;

	drv_list = rtos_malloc(list_length * sizeof(*drv_list));
	if (!drv_list) {
		os_log(LOG_ERR, "rtos_malloc() failed\n");
		rc = -1;
		goto exit;
	}
	gcl.opList = drv_list;

	if (type == GENAVB_ST_ADMIN)
		hw_list = false;

	rc_sdk = ENET_QOS_EstReadGcl(drv->base, &gcl, list_length, hw_list);
	if (rc_sdk != kStatus_Success) {
		os_log(LOG_ERR, "ENET_QOS_EstProgramGcl() failed rc = %d\n", rc_sdk);
		rc = -1;
		goto exit;
	}

	drv_list_op = drv_list;

	for (i = 0; i < gcl.numEntries; i++, drv_list_op++, cfg_list++) {
		cfg_list->gate_states = drv_list_op->gate;
		cfg_list->time_interval = drv_list_op->interval;
		if (drv->fp_enabled) {
			if (cfg_list->gate_states & 0x1)
				cfg_list->operation = GENAVB_ST_SET_AND_HOLD_MAC;
			else
				cfg_list->operation = GENAVB_ST_SET_AND_RELEASE_MAC;

			cfg_list->gate_states |= 0x1;
		} else {
			cfg_list->operation = GENAVB_ST_SET_GATE_STATES;
		}
	}

	config->enable = (drv->base->MTL_EST_CONTROL & ENET_QOS_MTL_EST_CONTROL_EEST_MASK) ? 1 : 0;
	config->base_time = (uint32_t)gcl.baseTime + ((gcl.baseTime >> 32) * NSECS_PER_SEC);
	config->cycle_time_p = drv->cycle_time_p;
	config->cycle_time_q = drv->cycle_time_q;
	config->cycle_time_ext = gcl.extTime;
	config->list_length = gcl.numEntries;

exit:
	if (drv_list)
		rtos_free(drv_list);

	return rc;
}

static int enet_qos_st_max_entries(struct net_port *port)
{
	return ENET_QOS_EST_DEP;
}

static int enet_qos_set_fp(struct net_port *port, unsigned int type, struct genavb_fp_config *config)
{
	struct enet_qos_drv *drv = net_port_drv(port);
	uint8_t mask_express = 0, mask_preemptable = 0;
	uint32_t status;
	int i;
	int rc = 0;

	switch (type) {
	case GENAVB_FP_CONFIG_802_1Q:

		for (i = 0; i < QOS_PRIORITY_MAX; i++) {
			if (config->u.cfg_802_1Q.admin_status[i] == GENAVB_FP_ADMIN_STATUS_EXPRESS) {
				mask_express |= (1 << port_priority_to_traffic_class(port, i));
			} else if (config->u.cfg_802_1Q.admin_status[i] == GENAVB_FP_ADMIN_STATUS_PREEMPTABLE) {
				mask_preemptable |= (1 << port_priority_to_traffic_class(port, i));
			} else {
				rc = -1;
				goto exit;
			}
		}

		/* return error if there are conflicting settings */
		if (mask_express & mask_preemptable) {
			rc = -1;
			break;
		}

		if (enet_qos_st_fp_check_config(drv->fp_enabled, drv->st_enabled, mask_preemptable, drv->closed_mask) < 0) {
			os_log(LOG_ERR, "Scheduled traffic and frame preemption incompatible configuration\n");
			rc = -1;
			break;
		}

		drv->preemptable_mask = mask_preemptable;

		ENET_QOS_FpeConfigPreemptable(drv->base, mask_preemptable);

		break;

	case GENAVB_FP_CONFIG_802_3:

		if ((config->u.cfg_802_3.enable_tx != drv->fp_enabled) && drv->st_enabled) {
			os_log(LOG_ERR, "Can't enable/disable frame preemption while scheduled traffic is enabled\n");
			rc = -1;
			break;
		}

		if (enet_qos_st_fp_check_config(config->u.cfg_802_3.enable_tx, drv->st_enabled, drv->preemptable_mask, drv->closed_mask) < 0) {
			os_log(LOG_ERR, "Scheduled traffic and frame preemption incompatible configuration\n");
			rc = -1;
			break;
		}

		if (config->u.cfg_802_3.add_frag_size > FP_ADD_FRAG_SIZE_MAX) {
			rc = -1;
			break;
		}

		if ((config->u.cfg_802_3.verify_time < FP_VERIFY_TIME_MIN) || (config->u.cfg_802_3.verify_time > FP_VERIFY_TIME_MAX)) {
			rc = -1;
			break;
		}

		status = drv->base->MTL_FPE_CTRL_STS;
		status &= ~ENET_QOS_MTL_FPE_CTRL_STS_AFSZ_MASK;
		status |= ENET_QOS_MTL_FPE_CTRL_STS_AFSZ(config->u.cfg_802_3.add_frag_size);
		drv->base->MTL_FPE_CTRL_STS = status;

		drv->fp.verify_time = config->u.cfg_802_3.verify_time;

		if (config->u.cfg_802_3.verify_disable_tx)
			net_port_fp_event(&drv->fp, FP_EVENT_VERIFY_DISABLE);
		else
			net_port_fp_event(&drv->fp, FP_EVENT_VERIFY_ENABLE);

		if (config->u.cfg_802_3.enable_tx) {
			drv->fp_enabled = true;
			net_port_fp_event(&drv->fp, FP_EVENT_PREEMPTION_ENABLE);
		} else {
			drv->fp_enabled = false;
			net_port_fp_event(&drv->fp, FP_EVENT_PREEMPTION_DISABLE);
		}

		enet_qos_fp_schedule(drv, false);

		break;

	default:
		rc = -1;
		break;
	}

exit:
	return rc;
}

static int enet_qos_get_fp(struct net_port *port, unsigned int type, struct genavb_fp_config *config)
{
	struct enet_qos_drv *drv = net_port_drv(port);
	uint32_t status, advance, tc_preemptable_mask;
	int i;
	int rc = 0;

	status = drv->base->MTL_FPE_CTRL_STS;

	switch (type) {
	case GENAVB_FP_CONFIG_802_1Q:

		advance = drv->base->MTL_FPE_ADVANCE;

		tc_preemptable_mask = (drv->base->MTL_FPE_CTRL_STS & ENET_QOS_MTL_FPE_CTRL_STS_PEC_MASK) >> ENET_QOS_MTL_FPE_CTRL_STS_PEC_SHIFT;

		for (i = 0; i < QOS_PRIORITY_MAX; i++) {
			if (tc_preemptable_mask & (1 << port_priority_to_traffic_class(port, i)))
				config->u.cfg_802_1Q.admin_status[i] = GENAVB_FP_ADMIN_STATUS_PREEMPTABLE;
			else
				config->u.cfg_802_1Q.admin_status[i] = GENAVB_FP_ADMIN_STATUS_EXPRESS;
		}

		config->u.cfg_802_1Q.hold_advance = (advance & ENET_QOS_MTL_FPE_ADVANCE_HADV_MASK) >> ENET_QOS_MTL_FPE_ADVANCE_HADV_SHIFT;
		config->u.cfg_802_1Q.release_advance = (advance & ENET_QOS_MTL_FPE_ADVANCE_RADV_MASK) >> ENET_QOS_MTL_FPE_ADVANCE_RADV_SHIFT;

		config->u.cfg_802_1Q.preemption_active = (drv->base->MAC_FPE_CTRL_STS & ENET_QOS_MAC_FPE_CTRL_STS_EFPE_MASK) ? true : false;

		config->u.cfg_802_1Q.hold_request = (status & ENET_QOS_MTL_FPE_CTRL_STS_HRS_MASK) ? GENAVB_FP_HOLD_REQUEST_HOLD : GENAVB_FP_HOLD_REQUEST_RELEASE;

		break;

	case GENAVB_FP_CONFIG_802_3:
		config->u.cfg_802_3.support = GENAVB_FP_SUPPORT_SUPPORTED;
		config->u.cfg_802_3.status_verify = net_port_fp_status_verify(&drv->fp);
		config->u.cfg_802_3.enable_tx = drv->fp.p_enable;
		config->u.cfg_802_3.verify_disable_tx = drv->fp.disable_verify;
		config->u.cfg_802_3.status_tx = 0; // FIXME, needs hardware support
		config->u.cfg_802_3.verify_time = drv->fp.verify_time;
		config->u.cfg_802_3.add_frag_size = (status & ENET_QOS_MTL_FPE_CTRL_STS_AFSZ_MASK) >> ENET_QOS_MTL_FPE_CTRL_STS_AFSZ_SHIFT;

		break;

	default:
		rc = -1;
		break;
	}

	return rc;
}

/*
 * ptp_clk_freq: the ptp/system time clock frequency (ssinc * ptp_clk_freq = 1sec)
 * ptp_ref_clk_freq: the ptp reference clock frequency (clk_ptp_ref_i)
 */
static uint32_t compute_default_addend(uint32_t ptp_clk_freq, uint32_t ptp_ref_clk_freq)
{
	uint64_t tmp;

	/* Default addend: 2^32 / (ptp_ref_clk_freq / ptp_clk_freq) */
	tmp = (uint64_t)ptp_clk_freq << 32;

	return (tmp / ptp_ref_clk_freq);
}

static int enet_qos_set_tx_queue_config(struct net_port *port,
				 struct tx_queue_properties *tx_q_cfg)
{
	struct enet_qos_drv *drv = net_port_drv(port);
	enet_qos_multiqueue_config_t *config = &drv->multi_queue;
	int i, cbs_idx = 0;

	if (tx_q_cfg->num_queues > ENET_QOS_TXQ_MAX)
		return -1;

	config->txQueueUse = tx_q_cfg->num_queues;

	for (i = 0; i < config->txQueueUse; i++) {
		if (tx_q_cfg->queue_prop[i] & TX_QUEUE_FLAGS_CREDIT_SHAPER) {
			config->txQueueConfig[i].mode = kENET_QOS_AVB_Mode;

			if (cbs_idx >= ENET_QOS_CBS_MAX)
				return -1;

			config->txQueueConfig[i].cbsConfig = &drv->cbs_config[cbs_idx++];

		} else if (tx_q_cfg->queue_prop[i] & TX_QUEUE_FLAGS_STRICT_PRIORITY) {
			config->txQueueConfig[i].mode = kENET_QOS_DCB_Mode;
		}
	}

	return 0;
}

static int enet_qos_set_tx_idle_slope(struct net_port *port, unsigned int idle_slope, uint32_t queue)
{
	struct enet_qos_drv *drv = net_port_drv(port);
	enet_qos_cbs_config_t config;
	uint32_t idle_slope_cycle_factor, max_transmit_rate;
	int rc = 0;

	switch (port->phy_speed) {
	case kPHY_Speed100M:
		idle_slope_cycle_factor = AVB_IDLE_SLOPE_CYCLE_FACTOR_100M;
		max_transmit_rate = AVB_MAX_PORT_TRANSIT_RATE_100M;
		break;
	case kPHY_Speed1000M:
		idle_slope_cycle_factor = AVB_IDLE_SLOPE_CYCLE_FACTOR_1000M;
		max_transmit_rate = AVB_MAX_PORT_TRANSIT_RATE_1000M;
		break;
	case kPHY_Speed10M:
	default:
		rc = -1;
		goto err;
	}

	/* Scale and round up idle slope and send slope:
	 * The software should program idle slope with computed credit in bits per cycle (40ns or 8ns) scaled by 1024
	 */
	config.idleSlope = ((((uint64_t)idle_slope * 1024) + idle_slope_cycle_factor - 1) / idle_slope_cycle_factor);
	config.sendSlope = max_transmit_rate - config.idleSlope;

	config.highCredit = AVB_HI_CREDIT;
	config.lowCredit =  AVB_LO_CREDIT;

	ENET_QOS_AVBConfigure(drv->base, &config, queue);

err:
	return rc;
}

/*
 * The MTL classification selects a MTL queue for the received frame.
 * This classification suffers from some limitations:
 * -VLAN classification is not performed if the multicast rule is enabled and the
 *  multicast rule is mandatory to prioritize PTP traffic.
 * -PTP classification doesn't work.
 * -multicast classification also matches broadcast.
 *
 * In conclusion the receive QoS is not bulletproof as broadcast traffic is mixed up
 * with prioritized traffic.
 */
static void enet_qos_setup_rx_mtl(struct enet_qos_drv *drv, const uint8_t *map)
{
	int i;

	for (i = 0; i < QOS_PRIORITY_MAX; i++) {
		/* VLAN PCP to queue mapping */
		/* Non AVTP packets, that would go to a queue in AVB mode, actually end up in queue 0 */
		/* FIXME: this classification is overriden by the multicast rule. Keep it for reference. */
		drv->multi_queue.rxQueueConfig[map[i]].priority |= (1 << i);

		/* Queue mode, AVB for SR class queues, DCB for all others */
		if (sr_pcp_class(i) != SR_CLASS_NONE) {
			drv->multi_queue.rxQueueConfig[map[i]].mode = kENET_QOS_AVB_Mode;
		} else {
			drv->multi_queue.rxQueueConfig[map[i]].mode = kENET_QOS_DCB_Mode;
		}
	}

	/* Untagged */
	drv->multi_queue.rxQueueConfig[map[QOS_BEST_EFFORT_PRIORITY]].packetRoute |= kENET_QOS_PacketUPQ;

	/* Matching Multicast/Broadcast, PTP, MSRP, MVRP, ... */
	drv->multi_queue.rxQueueConfig[map[PTPV2_DEFAULT_PRIORITY]].packetRoute |= kENET_QOS_PacketMCBCQ;

	/* AVDECC, use lowest priority AVB queue */
	for (i = 0; i < QOS_PRIORITY_MAX; i++) {
		if (sr_pcp_class(i) != SR_CLASS_NONE) {
			drv->multi_queue.rxQueueConfig[map[i]].packetRoute |= kENET_QOS_PacketAVCPQ;
			break;
		}
	}
}

/*
 * To address the MTL limitations an additional layer of classification is
 * performed by the receive parser.
 * The DMA channel assignment is performed by the receive parser
 * after the frames has been enqueued in the MTL queue. It allows to correctly
 * route the frames to the right user queue. However congestion and frame loss
 * can still happen if the MTL queue has congestions.
 */
static void enet_qos_setup_rx_dma(struct enet_qos_drv *drv, const uint8_t *map)
{
	int i;

	/* VLAN PCP to queue mapping */
	for (i = 0; i < QOS_PRIORITY_MAX; i++)
		drv->rxp_config[INDEX_VLAN_PCP + QOS_PRIORITY_MAX - 1 - i].dmaChannel = 1 << map[i];

	drv->rxp_config[INDEX_MULTICAST_RESERVED + 1].dmaChannel = 1 << map[QOS_INTERNETWORK_CONTROL_PRIORITY];
	drv->rxp_config[INDEX_AVDECC].dmaChannel = 1 << map[AVDECC_DEFAULT_PRIORITY];
	drv->rxp_config[INDEX_OTHER].dmaChannel = 1 << map[QOS_BEST_EFFORT_PRIORITY];
}

static int enet_qos_setup_rx(struct enet_qos_drv *drv, unsigned int max_queue)
{
	enet_qos_buffer_config_t *buf_cfg;
	int queue, i, desc_offset = 0, buf_offset = 0;
	const uint8_t *map;

	map = priority_to_traffic_class_map(max_queue, CFG_SR_CLASS_MAX);
	if (!map) {
		os_log(LOG_ERR, "priority_to_traffic_class_map() failed\n");
		goto err;
	}

	enet_qos_setup_rx_mtl(drv, map);
	enet_qos_setup_rx_dma(drv, map);

	/* Ring buffers configuration */
	for (queue = 0; queue < max_queue; queue++) {
		buf_cfg = &drv->buffer_config[queue];

#ifdef BOARD_NET_RX_CACHEABLE
		buf_cfg->rxBuffNeedMaintain = true;
#endif /* BOARD_NET_RX_CACHEABLE */

		if (queue == map[QOS_BEST_EFFORT_PRIORITY])
			buf_cfg->rxRingLen = ENET_QOS_BE_RXBD_NUM;
		else if (sr_class_enabled(SR_CLASS_A) && (queue == map[sr_class_pcp(SR_CLASS_A)]))
			buf_cfg->rxRingLen = ENET_QOS_SR_RXBD_NUM;
		else
			buf_cfg->rxRingLen = ENET_QOS_OTHER_RXBD_NUM;

		buf_cfg->rxDescStartAddrAlign = &g_rxBuffDescrip[desc_offset];
		buf_cfg->rxDescTailAddrAlign = &g_rxBuffDescrip[desc_offset + buf_cfg->rxRingLen];
		buf_cfg->rxBufferStartAddr = &rx_buf_addr[desc_offset];

		desc_offset += buf_cfg->rxRingLen;

		for (i = 0; i < buf_cfg->rxRingLen; i++) {
			buf_cfg->rxBufferStartAddr[i] = (uint32_t)(uintptr_t)&rx_buf[buf_offset];
			buf_offset++;
		}
	}

	return 0;

err:
	return -1;
}

static int enet_qos_post_init(struct net_port *port)
{
	struct enet_qos_drv *drv = net_port_drv(port);

	if (enet_qos_fp_init(drv) < 0)
		goto err_fp;

	return 0;

err_fp:
	return -1;
}

static void enet_qos_pre_exit(struct net_port *port)
{
	struct enet_qos_drv *drv = net_port_drv(port);

	enet_qos_fp_exit(drv);
}

__exit static void enet_qos_exit(struct net_port *port)
{
	struct enet_qos_drv *drv = net_port_drv(port);

	ENET_QOS_Deinit(drv->base);

	hw_clock_unregister(HW_CLOCK_PORT_BASE + port->hw_clock_id);
}

__init int enet_qos_init(struct net_port *port)
{
	struct enet_qos_drv *drv;
	unsigned int i;

	if (port->drv_index)
		goto err;

	port->drv_ops.add_multi = enet_qos_add_multi;
	port->drv_ops.del_multi = enet_qos_del_multi;
	port->drv_ops.get_rx_frame_size = enet_qos_get_rx_frame_size;
	port->drv_ops.read_frame = enet_qos_read_frame;
	port->drv_ops.send_frame = enet_qos_send_frame;
	port->drv_ops.tx_cleanup = enet_qos_tx_cleanup;
	port->drv_ops.link_up = enet_qos_link_up;
	port->drv_ops.link_down = enet_qos_link_down;
	port->drv_ops.set_tx_queue_config = enet_qos_set_tx_queue_config;
	port->drv_ops.set_tx_idle_slope = enet_qos_set_tx_idle_slope;
	port->drv_ops.set_st_config = enet_qos_set_st_config;
	port->drv_ops.get_st_config = enet_qos_get_st_config;
	port->drv_ops.st_max_entries = enet_qos_st_max_entries;
	port->drv_ops.set_fp = enet_qos_set_fp;
	port->drv_ops.get_fp = enet_qos_get_fp;
	port->drv_ops.exit = enet_qos_exit;
	port->drv_ops.post_init = enet_qos_post_init;
	port->drv_ops.pre_exit = enet_qos_pre_exit;

	port->num_rx_q = ENET_QOS_RXQ_MAX;

	port->drv = &enet_driver;

	drv = net_port_drv(port);
	drv->base = (ENET_QOS_Type *)port->base;

	drv->ptp_ref_clk_freq = dev_get_enet_1588_freq(drv->base);
	if (!drv->ptp_ref_clk_freq) {
		os_log(LOG_ERR, "dev_get_enet_1588_freq() failed\n");
		goto err;
	}

	drv->config.csrClock_Hz = dev_get_enet_core_freq(drv->base);
	if (!drv->config.csrClock_Hz) {
		os_log(LOG_ERR, "dev_get_enet_core_freq() failed\n");
		goto err;
	}

	/* ptp/system clock much be slightly lower than ptp reference clock,
           so that addend can be adjusted lower or higher (at least +- 100ppm) */
	drv->ptp_clk_freq = (drv->ptp_ref_clk_freq * 4ULL) / 5ULL;

	drv->ptp_config.defaultAddend = compute_default_addend(drv->ptp_clk_freq, drv->ptp_ref_clk_freq);

	drv->ptp_config.systemTimeClock_Hz = drv->ptp_clk_freq;
	drv->config.ptpConfig = &drv->ptp_config;

	drv->multi_queue.rxQueueUse = port->num_rx_q;
	drv->config.multiqueueCfg = &drv->multi_queue;

	if (enet_qos_set_tx_queue_config(port, &tx_q_default_config) < 0) {
		os_log(LOG_ERR, "enet_qos_setup_tx_queue_config() failed\n");
		goto err;
	}

	port->tx_q_cap = &tx_q_capabilites;
	port->num_tx_q = tx_q_default_config.num_queues;

	os_log(LOG_INIT, "port(%u) enet_qos(%u) core clock: %u Hz, ptp ref clock: %u Hz, ptp/system clock: %u Hz\n",
		port->index, port->drv_index, drv->config.csrClock_Hz, drv->ptp_ref_clk_freq, drv->ptp_clk_freq);

	os_log(LOG_INIT, "port(%u) enet_qos(%u) num TX queue: %u, num RX queue: %u\n",
		port->index, port->drv_index, port->num_tx_q, port->num_rx_q);

	drv->config.miiDuplex = kENET_QOS_MiiFullDuplex;
	drv->config.miiMode = port->mii_mode;

	if (enet_qos_setup_rx(drv, port->num_rx_q) < 0) {
		os_log(LOG_ERR, "enet_qos_setup_rx() failed\n");
		goto err;
	}

	if (ENET_QOS_Init(drv->base, &drv->config, port->mac_addr,
			  1, drv->ptp_ref_clk_freq) != kStatus_Success) {
		os_log(LOG_ERR, "ENET_QOS_Init() failed\n");
		goto err;
	}

	/* Initialize Descriptor. */
	if (ENET_QOS_DescriptorInit(drv->base,
				    &drv->config,
				    drv->buffer_config) != kStatus_Success) {
		os_log(LOG_ERR, "ENET_QOS_DescriptorInit() failed\n");
		goto err;
	}

	/* Create the handler. */
	ENET_QOS_CreateHandler(drv->base, &drv->handle,
			       &drv->config, drv->buffer_config,
			       enet_qos_callback, port);

	if (drv->handle.rxintEnable) {
		os_log(LOG_ERR, "Receive interrupt is set\n");
		goto err;
	}

	if (ENET_QOS_ConfigureRxParser(drv->base, drv->rxp_config, RX_PARSER_SIZE) != kStatus_Success) {
		os_log(LOG_ERR, "ENET_QOS_ConfigureRxParser() error\n");
		goto err;
	}

	ENET_QOS_SetISRHandler(drv->base, enet_qos_interrupt);

	drv->clock.read_counter = enet_qos_1588_read_counter;
	drv->clock.adj_freq = enet_qos_1588_clock_adj_freq;
	drv->clock.priv = drv;

	if (hw_clock_register(HW_CLOCK_PORT_BASE + port->hw_clock_id, &drv->clock) < 0) {
		os_log(LOG_ERR, "failed to register hw clock\n");
		goto err;
	}

	for (i = 0; i < (ENET_QOS_NUM_TIMER - 1); i++) {
		struct hw_timer *hw_timer = &drv->timer[i].hw_timer;
		unsigned int flags = 0;

		if (port->pps_timer_channel >= 0 && port->pps_timer_channel == port->timers[i].channel)
			flags |= HW_TIMER_F_PPS;

		drv->timer[i].id = i;
		drv->timer[i].instance = port->timers[i].channel;

		hw_timer->set_next_event = enet_qos_1588_timer_set_next_event;
		hw_timer->cancel_event = enet_qos_1588_timer_cancel_event;

		if (hw_timer_register(HW_CLOCK_PORT_BASE + port->hw_clock_id, hw_timer, flags) < 0) {
			os_log(LOG_ERR, "%s : failed to register hw timer\n", __func__);
			continue;
		}

		enet_qos_1588_timer_init(drv, &drv->timer[i]);
	}

	enet_qos_stats_init(port);

	ENET_QOS_EnableInterrupts(drv->base, kENET_QOS_MacTimestamp);

	return 0;

err:
	return -1;
}
#else
__init int enet_qos_init(struct net_port *port) { return -1; }
#endif /* CFG_NUM_ENET_QOS_MAC */
