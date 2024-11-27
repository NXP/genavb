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

#include "common/log.h"

#include "net_port_netc_sw.h"
#include "net_port_netc_sw_drv.h"

#include "config.h"

#if CFG_NUM_NETC_SW

#include "fsl_netc.h"
#include "fsl_netc_timer.h"
#include "fsl_phy.h"

#include "net_port_enetc_ep.h"
#include "net_port_netc_1588.h"
#include "net_port_netc_stream_identification.h"
#include "net_port_netc_sw_frer.h"
#include "net_port_netc_psfp.h"
#include "net_port_netc_sw_dsa.h"

#include "clock.h"
#include "ether.h"
#include "ptp.h"

#include "genavb/error.h"

#define CUSTOMER_VLAN_BRIDGE	1
#define PROVIDER_BRIDGE		2

#define BRIDGE_TYPE		CUSTOMER_VLAN_BRIDGE

#define NETC_SW_BD_ALIGN 128U

/* Tx settings */

#define NETC_SW_TXBD_NUM 16

SDK_ALIGN(static netc_tx_bd_t sw_tx_desc[NETC_SW_TXBD_NUM], NETC_SW_BD_ALIGN);
static netc_tx_frame_info_t sw_tx_dirty[NETC_SW_TXBD_NUM];

#define NETC_TX_MAX_SDU 0x7D0U

/* Rx settings */

#define NETC_SW_RXBD_NUM 32 /* ENETC rx bd queue size must be a power of 2 */
#define NETC_SW_BUFF_SIZE_ALIGN 64U
#define NETC_SW_RXBUFF_SIZE 256
#define NETC_SW_RXBUFF_SIZE_ALIGN SDK_SIZEALIGN(NETC_SW_RXBUFF_SIZE, NETC_SW_BUFF_SIZE_ALIGN)

typedef uint8_t rx_buffer_t[NETC_SW_RXBUFF_SIZE_ALIGN];

SDK_ALIGN(static netc_rx_bd_t sw_rx_desc[NETC_SW_RXBD_NUM], NETC_SW_BD_ALIGN);
SDK_ALIGN(static rx_buffer_t sw_rx_buff[NETC_SW_RXBD_NUM/2], NETC_SW_BUFF_SIZE_ALIGN);
static uint64_t sw_rx_buff_addr[NETC_SW_RXBD_NUM/2];

/* Command settings */

#define NETC_SW_CMDBD_NUM 8

SDK_ALIGN(static netc_cmd_bd_t g_cmd_buff_desc[NETC_SW_CMDBD_NUM], NETC_SW_BD_ALIGN);

#define TX_TS_ID_MAX_AGE 32
#define TX_TS_ID_FREE		(1 << 0)
#define TX_TS_ID_PENDING 	(1 << 1)
#define TX_TS_ID_AGED 		(1 << 2)

uint8_t netc_sw_port_to_net_port[CFG_NUM_NETC_SW_PORTS];
const uint8_t port_to_netc_sw_port[CFG_NUM_NETC_SW_PORTS] = {kNETC_SWITCH0Port0, kNETC_SWITCH0Port1, kNETC_SWITCH0Port2, kNETC_SWITCH0Port3, kNETC_SWITCH0Port4};

#define HOST_PORT_ID 	(CFG_NUM_NETC_SW_PORTS - 1)

/* FDB / VLAN */
#define NETC_DEFAULT_MFO 	kNETC_FDBLookUpWithDiscard
#define NETC_DEFAULT_PVID_MFO	kNETC_FDBLookUpWithFlood
#define NETC_DEFAULT_MLO	kNETC_HardwareMACLearn

enum {
	NETC_SW_FP_EVENT_LINK_FAIL,
	NETC_SW_FP_EVENT_LINK_OK,
};

void netc_sw_stats_init(struct net_port *);

static struct netc_sw_drv netc_sw_drivers[CFG_NUM_NETC_SW] = {
};

static inline bool fp_is_enabled(struct netc_sw_drv *drv, unsigned int port_id)
{
	return (drv->fp_enabled & (1U << port_id));
}

static inline void fp_set_enabled(struct netc_sw_drv *drv, unsigned int port_id, bool enable)
{
	if (enable)
		drv->fp_enabled |= (1U << port_id);
	else
		drv->fp_enabled &= ~(1U << port_id);
}

static inline void st_set(struct netc_sw_drv *drv, unsigned int port_id, bool enable)
{
	if (enable)
		drv->st_enabled |= (1U << port_id);
	else
		drv->st_enabled &= ~(1U << port_id);
}

static inline bool st_is_enabled(struct netc_sw_drv *drv, unsigned int port_id)
{
	return (drv->st_enabled & (1U << port_id));
}

static inline bool st_is_same(struct netc_sw_drv *drv, unsigned int port_id, bool enable)
{
	return (enable && st_is_enabled(drv, port_id)) || (!enable && !st_is_enabled(drv, port_id));
}

void *netc_sw_get_handle(struct net_port *port)
{
	struct netc_sw_drv *drv = net_port_drv(port);

	return &drv->handle;
}

void *netc_sw_get_port_handle(struct net_port *port)
{
	struct netc_sw_drv *drv = net_port_drv(port);

	return drv->handle.hw.ports[port->base].port;
}

void *netc_sw_get_link_handle(struct net_port *port)
{
	struct netc_sw_drv *drv = net_port_drv(port);

	if (!NETC_PortIsPseudo(netc_sw_get_port_handle(port)))
		return drv->handle.hw.ports[port->base].eth;
	else
		return drv->handle.hw.ports[port->base].pseudo;
}

static int netc_sw_add_multi(struct net_port *port, uint8_t *addr)
{
	return 0;
}

static int netc_sw_del_multi(struct net_port *port, uint8_t *addr)
{
	return 0;
}

static void netc_sw_ts_free(struct tx_ts_info_t *ts_info)
{
	ts_info->state = TX_TS_ID_FREE;
	ts_info->age = 0;

	if (ts_info->desc) {
		net_tx_free(ts_info->desc);
		ts_info->desc = NULL;
	}
}

static void netc_sw_ts_flush(struct netc_sw_drv *drv)
{
	int i;

	for (i = 0; i < NUM_TX_TS; i++)
		netc_sw_ts_free(&drv->tx_ts[i]);
}

static struct tx_ts_info_t *netc_sw_ts_alloc(struct netc_sw_drv *drv, uint16_t tx_ts_id)
{
	struct tx_ts_info_t *ts_info, *oldest = NULL;
	struct tx_ts_info_t *found = NULL;
	int i, oldest_slot = 0;

	for (i = 0; i < NUM_TX_TS; i++) {
		ts_info = &drv->tx_ts[i];

		if ((!found) && (ts_info->state == TX_TS_ID_FREE)) {
			found = ts_info;
		} else {
			if ((ts_info->state == TX_TS_ID_PENDING) && (++ts_info->age >= TX_TS_ID_MAX_AGE)) {
				os_log(LOG_ERR, "slot(%u) aged ts_id(%u)\n", i, ts_info->tx_ts_id);
				ts_info->state = TX_TS_ID_AGED;
			}

			if ((!oldest) || (oldest->age < ts_info->age)) {
				oldest = ts_info;
				oldest_slot = i;
			}
		}
	}

	if (!found) {
		os_log(LOG_ERR, "tx_ts array is full ts_id(%u), replacing oldest slot(%u) ts_id(%u)\n", tx_ts_id, oldest_slot, oldest->tx_ts_id);

		netc_sw_ts_free(oldest);

		found = oldest;
	}

	found->state = TX_TS_ID_PENDING;
	found->tx_ts_id = tx_ts_id;

	return found;
}

static struct tx_ts_info_t *netc_sw_ts_find(struct netc_sw_drv *drv, uint16_t tx_ts_id)
{
	struct tx_ts_info_t *ts_info;
	int i;

	for (i = 0; i < NUM_TX_TS; i++) {
		ts_info = &drv->tx_ts[i];
		if ((ts_info->state != TX_TS_ID_FREE) && (ts_info->tx_ts_id == tx_ts_id))  {
			return ts_info;
		}
	}

	return NULL;
}

static void netc_sw_ts_done(struct netc_sw_drv *drv, struct tx_ts_info_t *ts_info)
{
	struct net_port *port = ts_info->port;
	uint64_t cycles = 0, ts;

	cycles = netc_1588_hwts_to_u64(port->hw_clock, ts_info->timestamp);

	ts = hw_clock_cycles_to_time(port->hw_clock, cycles) + port->tx_tstamp_latency;

	ptp_tx_ts(port, ts, ts_info->desc->priv);

	netc_sw_ts_free(ts_info);
}

static status_t netc_sw_reclaim_cb(swt_handle_t *handle, netc_tx_frame_info_t *frameInfo, void *userData)
{
	struct net_port *ports_array = (struct net_port *)userData;
	struct net_tx_desc *desc = (struct net_tx_desc *)frameInfo->context;
	struct net_port *port = &ports_array[netc_sw_port_to_net_port[desc->port]];
	struct netc_sw_drv *drv = net_port_drv(port);
	struct tx_ts_info_t *tx_ts_info;

	if (frameInfo->isTxTsIdAvail) {
		tx_ts_info = netc_sw_ts_find(drv, frameInfo->txtsid);
		if (tx_ts_info) {
			tx_ts_info->port = port;
			tx_ts_info->desc = desc;

			netc_sw_ts_done(drv, tx_ts_info);
		} else {
			tx_ts_info = netc_sw_ts_alloc(drv, frameInfo->txtsid);

			tx_ts_info->port = port;
			tx_ts_info->desc = desc;
		}
	} else {
		net_tx_free(desc);
	}

	return kStatus_Success;
}

static int netc_sw_get_rx_frame_size(struct net_bridge *bridge, uint32_t *length)
{
	struct netc_sw_drv *drv = net_bridge_drv(bridge);
	int rc;

	switch (SWT_GetRxFrameSize(&drv->handle, length)) {
	case kStatus_Success:
	case kStatus_NETC_RxHRZeroFrame:
		rc = BR_RX_FRAME_SUCCESS;
		break;

	case kStatus_NETC_RxTsrResp:
		rc = BR_RX_FRAME_EGRESS_TS;
		break;

	case kStatus_NETC_RxFrameEmpty:
		rc = BR_RX_FRAME_EMPTY;
		break;

	default:
		rc = BR_RX_FRAME_ERROR;
		break;
	}

	return rc;
}

static int netc_sw_read_frame(struct net_bridge *bridge, uint8_t *data, uint32_t length, uint8_t *port_index, uint64_t *ts, uint8_t *hr)
{
	struct netc_sw_drv *drv = net_bridge_drv(bridge);
	struct net_port *port;
	netc_frame_attr_t attr;

	if (SWT_ReceiveFrameCopy(&drv->handle, data, length, &attr) ==  kStatus_Success) {
		port = &ports[netc_sw_port_to_net_port[attr.srcPort]];

		if (port_index)
			*port_index = netc_sw_port_to_net_port[attr.srcPort];

		if (ts && attr.isTsAvail) {
			*ts = netc_1588_hwts_to_u64(port->hw_clock, attr.timestamp);
			os_log(LOG_DEBUG, "port(%u) attr.timestamp(%u) ts(%lu)\n", netc_sw_port_to_net_port[attr.srcPort], attr.timestamp, *ts);
		}

		if (hr) {
			switch (attr.hostReason) {
			case kNETC_MACLearning:
				*hr = BR_RX_FLAGS_MAC_LEARNING;
				break;
			default:
				*hr = 0;
			}
		}
	} else {
		goto err;
	}

	return 0;

err:
	return -1;
}

static int netc_sw_read_egress_ts_frame(struct net_bridge *bridge)
{
	struct netc_sw_drv *drv = net_bridge_drv(bridge);
	struct tx_ts_info_t *tx_ts_info;
	swt_tsr_resp_t tsr;

	if (SWT_GetTimestampRefResp(&drv->handle, &tsr) == kStatus_Success) {
		tx_ts_info = netc_sw_ts_find(drv, tsr.txtsid);
		if (tx_ts_info) {
			tx_ts_info->timestamp = tsr.timestamp;

			netc_sw_ts_done(drv, tx_ts_info);
		} else {
			tx_ts_info = netc_sw_ts_alloc(drv, tsr.txtsid);

			tx_ts_info->timestamp = tsr.timestamp;
		}
	} else {
		goto err;
	}

	return 0;

err:
	return -1;
}

static int netc_sw_send_frame(struct net_bridge *bridge, unsigned int bridge_port, struct net_tx_desc *desc, uint8_t priority)
{
	netc_buffer_struct_t tx_buff = {.buffer = (uint8_t *)desc + desc->l2_offset, .length = desc->len};
	netc_frame_struct_t tx_frame = {.buffArray = &tx_buff, .length = 1};
	swt_mgmt_tx_arg_t mgmt_tx = {.ipv = priority, .dr = 0};
	struct netc_sw_drv *drv = net_bridge_drv(bridge);
	swt_tx_opt opt;

	opt.flags = (desc->flags & NET_TX_FLAGS_HW_TS) ? kSWT_TX_OPT_DIRECT_ENQUEUE_REQ_TSR: 0;

	desc->port = bridge_port;

	if (SWT_SendFrame(&drv->handle, mgmt_tx, port_to_netc_sw_port[bridge_port], drv->use_masquerade, &tx_frame, (void *)desc, &opt) == kStatus_Success) {
		return 0;
	} else {
		return -1;
	}
}

static void netc_sw_tx_cleanup(struct net_bridge *bridge)
{
	struct netc_sw_drv *drv = net_bridge_drv(bridge);

	SWT_ReclaimTxDescriptor(&drv->handle, false, 0);
}

static void netc_sw_port_fp_event(struct net_port *port, unsigned int event)
{
	struct netc_sw_drv *drv = net_port_drv(port);

	rtos_mutex_lock(&port->config_mutex, RTOS_WAIT_FOREVER);

	if (fp_is_enabled(drv, port->base)) {
		NETC_ETH_LINK_Type *base_eth = netc_sw_get_link_handle(port);

		switch (event) {
		case NETC_SW_FP_EVENT_LINK_FAIL:
			base_eth->MAC_MERGE_MMCSR |= NETC_ETH_LINK_MAC_MERGE_MMCSR_LINK_FAIL(1U);
			break;
		case NETC_SW_FP_EVENT_LINK_OK:
			base_eth->MAC_MERGE_MMCSR &= ~(NETC_ETH_LINK_MAC_MERGE_MMCSR_LINK_FAIL(1U));
			break;
		default:
			break;
		}

	}

	rtos_mutex_unlock(&port->config_mutex);
}

static void netc_sw_link_up(struct net_port *port)
{
	struct netc_sw_drv *drv = net_port_drv(port);
	netc_hw_mii_speed_t miiSpeed;
	netc_hw_mii_duplex_t miiDuplex;
	uint16_t speed;

	/* Use the actual speed and duplex when phy success
	 * to finish the autonegotiation.
	 */

	if (port->phy_index == -1) {
		port->phy_speed = kPHY_Speed1000M;
		port->phy_duplex = kPHY_FullDuplex;
	}

	switch (port->phy_speed) {
	case kPHY_Speed10M:
	default:
		miiSpeed = kNETC_MiiSpeed10M;
		speed = 0;
		break;

	case kPHY_Speed100M:
		miiSpeed = kNETC_MiiSpeed100M;
		speed = 9;
		break;

	case kPHY_Speed1000M:
		miiSpeed = kNETC_MiiSpeed1000M;
		speed = 99;
		break;
	}

	switch (port->phy_duplex) {
	case kPHY_HalfDuplex:
	default:
		miiDuplex = kNETC_MiiHalfDuplex;
		break;

	case kPHY_FullDuplex:
		miiDuplex = kNETC_MiiFullDuplex;
		break;
	}

	SWT_SetEthPortMII(&drv->handle, port->base, miiSpeed, miiDuplex);
	SWT_SetPortSpeed(&drv->handle, port->base, speed);

	netc_sw_port_fp_event(port, NETC_SW_FP_EVENT_LINK_OK);
}

static void netc_sw_link_down(struct net_port *port)
{
	struct netc_sw_drv *drv = net_port_drv(port);

	netc_sw_ts_flush(drv);

	netc_sw_port_fp_event(port, NETC_SW_FP_EVENT_LINK_FAIL);
}

static int netc_sw_set_tx_queue_config(struct net_port *port, struct tx_queue_properties *tx_q_cfg)
{
	return 0;
}

static int netc_sw_set_tx_idle_slope(struct net_port *port, uint64_t idle_slope, uint32_t queue)
{
	struct netc_sw_drv *drv = net_port_drv(port);
	netc_hw_port_idx_t port_idx = port->base;
	netc_port_tc_cbs_config_t cbs_config;
	unsigned long port_tx_rate;
	status_t status;
	int rc = 0;

	switch (port->phy_speed) {
	case kPHY_Speed10M:
	default:
		port_tx_rate = 10000000;
		break;

	case kPHY_Speed100M:
		port_tx_rate = 100000000;
		break;

	case kPHY_Speed1000M:
		port_tx_rate = 1000000000;
		break;
	}

	/* Calculate bandwidth in percentage units of the port transmit rate (0-100%)
	 * Round up the result
	 * bw = ceil(100 * idle_slope / port_tx_rate)
	 */
	cbs_config.bwWeight = (100 * idle_slope + port_tx_rate - 1) / port_tx_rate;

	/* hiCredit (bits) = maxSizedFrame * (idle_slope / port_tx_rate) */
	cbs_config.hiCredit = ((uint64_t)port->max_pdu * 8 * idle_slope + port_tx_rate - 1) / port_tx_rate;
	/* hiCredit (credits) = hiCredit * (NETC Clock Frequency / port_tx_rate) * 100 */
	cbs_config.hiCredit = cbs_config.hiCredit * (dev_get_net_core_freq(SW0_BASE) / (port_tx_rate / 100));

	status = SWT_TxCBSConfigPort(&drv->handle, port_idx, queue, &cbs_config);
	if (status != kStatus_Success) {
		os_log(LOG_ERR, "SWT_TxCBSConfigPort failed\n");
		rc = -1;
		goto err;
	}

err:
	return rc;
}

static int netc_sw_set_priority_to_tc_map(struct net_port *port)
{
	struct netc_sw_drv *drv = net_port_drv(port);
	netc_hw_port_idx_t port_idx = port->base;

	return SWT_SetPortIPV2QMR(&drv->handle, port_idx, port->map);
}

static int netc_sw_enable_st_config(struct net_port *port, bool enable)
{
	struct netc_sw_drv *drv = net_port_drv(port);
	netc_hw_port_idx_t port_idx = port->base;
	status_t status;
	int rc = 0;

	if (st_is_same(drv, port_idx, enable))
		goto out;

	/* Port TGS affects several independent features and can "never" be disabled.
	   Just togle it, to go back to an empty gate control list */
	if (!enable) {
#if (defined(FSL_FEATURE_NETC_HAS_ERRATA_051130) && FSL_FEATURE_NETC_HAS_ERRATA_051130)
		status = SWT_TxPortTGSEnable(&drv->handle, port_idx, false, 0xff);
#else
		status = SWT_TxPortTGSEnable(&drv->handle, port_idx, false);
#endif
		if (status != kStatus_Success) {
			os_log(LOG_ERR, "SWT_TxPortTGSEnable(false) failed\n");
			rc = -1;
			goto err;
		}

#if (defined(FSL_FEATURE_NETC_HAS_ERRATA_051130) && FSL_FEATURE_NETC_HAS_ERRATA_051130)
		status = SWT_TxPortTGSEnable(&drv->handle, port_idx, true, 0xff);
#else
		status = SWT_TxPortTGSEnable(&drv->handle, port_idx, true);
#endif
		if (status != kStatus_Success) {
			os_log(LOG_ERR, "SWT_TxPortTGSEnable(true) failed\n");
			rc = -1;
			goto err;
		}
	}

	st_set(drv, port_idx, enable);

err:
out:
	return rc;
}

static int netc_sw_set_admin_config(struct net_port *port, struct genavb_st_config *config)
{
	struct netc_sw_drv *drv = net_port_drv(port);
	netc_tb_tgs_gcl_t time_gate_control_list;
	netc_tgs_gate_entry_t *gate_control_entries = NULL;
	netc_tb_tgs_entry_id_t entry_id = port->base;
	int rc = 0;
	int i;

	if (config->list_length > NETC_TB_TGS_MAX_ENTRY) {
		os_log(LOG_ERR, "list_length: %u invalid (max: %u)\n", config->list_length, NETC_TB_TGS_MAX_ENTRY);
		rc = -1;
		goto err;
	}

	time_gate_control_list.entryID = entry_id;
	time_gate_control_list.baseTime = config->base_time;
	time_gate_control_list.cycleTime = ((uint64_t)NSECS_PER_SEC * config->cycle_time_p) / config->cycle_time_q;
	time_gate_control_list.extTime = config->cycle_time_ext;
	time_gate_control_list.numEntries = config->list_length;

	if (config->list_length) {
		gate_control_entries = rtos_malloc(config->list_length * sizeof(netc_tgs_gate_entry_t));
		if (!gate_control_entries) {
			os_log(LOG_ERR, "rtos_malloc failed\n");
			rc = -1;
			goto err_alloc;
		}
		memset(gate_control_entries, 0, config->list_length * sizeof(netc_tgs_gate_entry_t));

		for (i = 0; i < config->list_length; i++) {
			gate_control_entries[i].interval = config->control_list[i].time_interval;
			gate_control_entries[i].tcGateState = config->control_list[i].gate_states;
			gate_control_entries[i].operType = config->control_list[i].operation;
		}
	}

	time_gate_control_list.gcList = gate_control_entries;

	if (SWT_TxTGSConfigAdminGcl(&drv->handle, &time_gate_control_list) != kStatus_Success) {
		os_log(LOG_ERR, "SWT_TxTGSConfigAdminGcl failed\n");
		rc = -1;
	}

	if (gate_control_entries)
		rtos_free(gate_control_entries);

err_alloc:
err:
	return rc;
}

static int netc_sw_set_st_config(struct net_port *port, struct genavb_st_config *config)
{
	int rc = 0;

	if (netc_sw_enable_st_config(port, config->enable) < 0) {
		os_log(LOG_ERR, "netc_sw_enable_st_config failed\n");
		rc = -1;
		goto err;
	}

	if (config->enable) {
		if (netc_sw_set_admin_config(port, config) < 0) {
			os_log(LOG_ERR, "netc_sw_set_admin_config failed\n");
			rc = -1;
			goto err;
		}
	}

err:
	return rc;
}

static int netc_sw_get_st_config(struct net_port *port, genavb_st_config_type_t type, struct genavb_st_config *config, unsigned int list_length)
{
	struct netc_sw_drv *drv = net_port_drv(port);
	netc_tb_tgs_gcl_t time_gate_control_list = {0};
	netc_tgs_gate_entry_t *gate_control_entries = NULL;
	status_t status;
	int rc = 0;
	int i;

	if (!config) {
		rc = -1;
		goto err;
	}

	if (type == GENAVB_ST_ADMIN) {
		os_log(LOG_ERR, "GENAVB_ST_ADMIN not supported\n");
		rc = -1;
		goto err;
	}

	if (list_length > NETC_TB_TGS_MAX_ENTRY) {
		os_log(LOG_ERR, "list_length: %u invalid (max: %u)\n", list_length, NETC_TB_TGS_MAX_ENTRY);
		rc = -1;
		goto err;
	}

	if (list_length) {
		gate_control_entries = rtos_malloc(list_length * sizeof(netc_tgs_gate_entry_t));
		if (!gate_control_entries) {
			os_log(LOG_ERR, "rtos_malloc failed\n");
			rc = -1;
			goto err_alloc;
		}
		memset(gate_control_entries, 0, list_length * sizeof(netc_tgs_gate_entry_t));
	}

	time_gate_control_list.entryID = port->base;
	time_gate_control_list.gcList = gate_control_entries;


	if (st_is_enabled(drv, port->base)) {
		status = SWT_TxtTGSGetOperGcl(&drv->handle, &time_gate_control_list, list_length);
		if (status != kStatus_Success) {
			os_log(LOG_ERR, "SWT_TxtTGSGetOperGcl failed with status %d\n", status);
			rc = -1;
			goto exit_free;
		}

		config->enable = 1;
		config->base_time = time_gate_control_list.baseTime;
		config->cycle_time_p = time_gate_control_list.cycleTime;
		config->cycle_time_q = NSECS_PER_SEC;
		config->cycle_time_ext = time_gate_control_list.extTime;
		config->list_length = time_gate_control_list.numEntries;

		if (config->list_length > list_length) {
			rc = -1;
			goto exit_free;
		}

		for (i = 0; i < config->list_length; i++) {
			config->control_list[i].operation = time_gate_control_list.gcList[i].operType;
			config->control_list[i].gate_states = time_gate_control_list.gcList[i].tcGateState;
			config->control_list[i].time_interval = time_gate_control_list.gcList[i].interval;
		}
	} else {
		memset(config, 0, sizeof(struct genavb_st_config));
	}


exit_free:
	if (gate_control_entries)
		rtos_free(gate_control_entries);

err_alloc:
err:
	return rc;
}

static int netc_sw_set_st_max_sdu(struct net_port *port, struct genavb_st_max_sdu *queue_max_sdu, unsigned int n)
{
	NETC_PORT_Type *base_port = netc_sw_get_port_handle(port);
	netc_port_tc_sdu_config_t config_sdu;
	netc_hw_tc_idx_t tc_idx;
	int rc = GENAVB_SUCCESS, i;

	for (i = 0; i < n; i++) {
		tc_idx = (netc_hw_tc_idx_t)queue_max_sdu[i].traffic_class;

		if (queue_max_sdu[i].queue_max_sdu)
			config_sdu.enTxMaxSduCheck = true;
		else
			config_sdu.enTxMaxSduCheck = false;

		config_sdu.maxSduSized = queue_max_sdu[i].queue_max_sdu;
		config_sdu.sduType = kNETC_MSDU;

		if (kStatus_Success != NETC_PortConfigTcMaxSDU(base_port, tc_idx, &config_sdu)) {
			rc = -GENAVB_ERR_ST_HW_CONFIG;
			goto err;
		}
	}

err:
	return rc;
}

static int netc_sw_get_st_max_sdu(struct net_port *port, struct genavb_st_max_sdu *queue_max_sdu)
{
	NETC_PORT_Type *base_port = netc_sw_get_port_handle(port);
	netc_port_tc_sdu_config_t config_sdu;
	netc_hw_tc_idx_t tc_idx;
	unsigned int i;
	int rc = GENAVB_SUCCESS;

	for (i = 0; i < QOS_TRAFFIC_CLASS_MAX; i++) {
		tc_idx	= (netc_hw_tc_idx_t)i;

		if (NETC_PortGetTcMaxSDU(base_port, tc_idx, &config_sdu) != kStatus_Success) {
			rc = -GENAVB_ERR_ST_HW_CONFIG;
			goto err;
		}

		queue_max_sdu[i].traffic_class = i;
		queue_max_sdu[i].queue_max_sdu = (uint32_t)config_sdu.maxSduSized;

		if (base_port->PTXDCRR0 & NETC_PORT_PTXDCRR0_MSDUEDR_MASK) {
			queue_max_sdu[i].transmission_overrun = (uint64_t)base_port->PTXDCR;
		}
	}

err:
	return rc;
}

static int netc_sw_st_max_entries(struct net_port *port)
{
	return NETC_TB_TGS_MAX_ENTRY;
}

static int netc_sw_set_fp(struct net_port *port, unsigned int type, struct genavb_fp_config *config)
{
	NETC_ETH_LINK_Type *base_eth = netc_sw_get_link_handle(port);
	NETC_PORT_Type *base_port = netc_sw_get_port_handle(port);
	struct netc_sw_drv *drv = net_port_drv(port);
	netc_hw_port_idx_t port_idx = port->base;
	uint32_t mask_preemptable = 0;
	int rc = 0, i;

	if (NETC_PortIsPseudo(base_port)) {
		rc = -1;
		goto err;
	}

	switch (type) {
	case GENAVB_FP_CONFIG_802_1Q:
		for (i = 0; i < QOS_PRIORITY_MAX; i++) {
			if (config->u.cfg_802_1Q.admin_status[i] == GENAVB_FP_ADMIN_STATUS_PREEMPTABLE)
				mask_preemptable |= (1 << port_priority_to_traffic_class(port, i));
		}

		base_port->PFPCR = mask_preemptable;
		break;

	case GENAVB_FP_CONFIG_802_3:
		base_eth->MAC_MERGE_MMCSR = (base_eth->MAC_MERGE_MMCSR & NETC_ETH_LINK_MAC_MERGE_MMCSR_LINK_FAIL(1U)) |
				NETC_ETH_LINK_MAC_MERGE_MMCSR_ME(config->u.cfg_802_3.enable_tx) |
				NETC_ETH_LINK_MAC_MERGE_MMCSR_VDIS(config->u.cfg_802_3.verify_disable_tx) |
				NETC_ETH_LINK_MAC_MERGE_MMCSR_VT(config->u.cfg_802_3.verify_time) |
				NETC_ETH_LINK_MAC_MERGE_MMCSR_RPS(config->u.cfg_802_3.enable_tx) |
				NETC_ETH_LINK_MAC_MERGE_MMCSR_RPE(config->u.cfg_802_3.enable_tx) |
				NETC_ETH_LINK_MAC_MERGE_MMCSR_RPA(config->u.cfg_802_3.enable_tx) |
				NETC_ETH_LINK_MAC_MERGE_MMCSR_RAFS(config->u.cfg_802_3.add_frag_size);

		fp_set_enabled(drv, port_idx, config->u.cfg_802_3.enable_tx);

		break;

	default:
		rc = -1;
		break;
	}

err:
		return rc;
}

static int netc_sw_get_fp(struct net_port *port, unsigned int type, struct genavb_fp_config *config)
{
	NETC_ETH_LINK_Type *base_eth = netc_sw_get_link_handle(port);
	NETC_PORT_Type *base_port = netc_sw_get_port_handle(port);
	uint32_t tc_preemptable_mask;
	int rc = 0, i;

	if (NETC_PortIsPseudo(base_port)) {
		rc = -1;
		goto err;
	}

	switch (type) {
	case GENAVB_FP_CONFIG_802_1Q:
		tc_preemptable_mask = base_port->PFPCR & 0xFF;

		for (i = 0; i < QOS_PRIORITY_MAX; i++) {
			if (tc_preemptable_mask & (1 << port_priority_to_traffic_class(port, i)))
				config->u.cfg_802_1Q.admin_status[i] = GENAVB_FP_ADMIN_STATUS_PREEMPTABLE;
			else
				config->u.cfg_802_1Q.admin_status[i] = GENAVB_FP_ADMIN_STATUS_EXPRESS;
		}

		config->u.cfg_802_1Q.hold_advance = (base_port->PTGSHAR & NETC_PORT_PTGSHAR_HOLDADVANCE_MASK) >> NETC_PORT_PTGSHAR_HOLDADVANCE_SHIFT;
		config->u.cfg_802_1Q.release_advance = (base_port->PTGSRAR & NETC_PORT_PTGSRAR_RELEASEADVANCE_MASK) >> NETC_PORT_PTGSRAR_RELEASEADVANCE_SHIFT;
		config->u.cfg_802_1Q.preemption_active = (base_eth->MAC_MERGE_MMCSR & NETC_ETH_LINK_MAC_MERGE_MMCSR_LPA_MASK) >> NETC_ETH_LINK_MAC_MERGE_MMCSR_LPA_SHIFT;
		config->u.cfg_802_1Q.hold_request = 0; /* There is no specific information on the hardware to fill this parameter */
		break;

	case GENAVB_FP_CONFIG_802_3:
		config->u.cfg_802_3.support = (base_eth->MAC_MERGE_MMCSR & NETC_ETH_LINK_MAC_MERGE_MMCSR_LPS_MASK) >> NETC_ETH_LINK_MAC_MERGE_MMCSR_LPS_SHIFT;
		config->u.cfg_802_3.status_verify = (base_eth->MAC_MERGE_MMCSR & NETC_ETH_LINK_MAC_MERGE_MMCSR_VSTS_MASK) >> NETC_ETH_LINK_MAC_MERGE_MMCSR_VSTS_SHIFT;
		config->u.cfg_802_3.enable_tx = (base_eth->MAC_MERGE_MMCSR & NETC_ETH_LINK_MAC_MERGE_MMCSR_ME_MASK) >> NETC_ETH_LINK_MAC_MERGE_MMCSR_ME_SHIFT;
		config->u.cfg_802_3.verify_disable_tx = (base_eth->MAC_MERGE_MMCSR & NETC_ETH_LINK_MAC_MERGE_MMCSR_VDIS_MASK) >> NETC_ETH_LINK_MAC_MERGE_MMCSR_VDIS_SHIFT;
		config->u.cfg_802_3.status_tx = (base_eth->MAC_MERGE_MMCSR & NETC_ETH_LINK_MAC_MERGE_MMCSR_TXSTS_MASK) >> NETC_ETH_LINK_MAC_MERGE_MMCSR_TXSTS_SHIFT;
		config->u.cfg_802_3.verify_time = (base_eth->MAC_MERGE_MMCSR & NETC_ETH_LINK_MAC_MERGE_MMCSR_VT_MASK) >> NETC_ETH_LINK_MAC_MERGE_MMCSR_VT_SHIFT;
		config->u.cfg_802_3.add_frag_size = (base_eth->MAC_MERGE_MMCSR & NETC_ETH_LINK_MAC_MERGE_MMCSR_RAFS_MASK) >> NETC_ETH_LINK_MAC_MERGE_MMCSR_RAFS_SHIFT;
		break;

	default:
		rc = -1;
		break;
	}

err:
	return rc;
}

static int netc_sw_set_max_frame_size(struct net_port *port, uint16_t size)
{
	struct netc_sw_drv *drv = net_port_drv(port);
	netc_hw_port_idx_t port_idx = port->base;
	int rc = GENAVB_SUCCESS;

	if (SWT_SetPortMaxFrameSize(&drv->handle, port_idx, size) != kStatus_Success) {
		rc = -GENAVB_ERR_INVALID;
		goto err;
	}

err:
	return rc;
}

static void netc_sw_del_ingress_filtering(struct netc_sw_drv *drv)
{
	while (drv->num_ipf_entries) {
		SWT_RxIPFDelTableEntry(&drv->handle, drv->ipf_entries_id[drv->num_ipf_entries]);
		drv->num_ipf_entries--;
	}
}

static int netc_sw_set_ingress_filtering(struct netc_sw_drv *drv)
{
#if BRIDGE_TYPE == CUSTOMER_VLAN_BRIDGE
	static netc_tb_ipf_config_t ipfEntryCfg;
	uint8_t vlan_reserved_mac[6] = C_VLAN_RESERVED_BASE;
	uint8_t vlan_reserved_mask[6] = C_VLAN_RESERVED_MASK;
	uint8_t mmrp_mvrp_mac[6] = MMRP_MVRP_BASE;
	uint8_t mmrp_mvrp_mask[6] = MMRP_MVRP_MASK;
	uint8_t hsr_supv_mac[6] = HSR_SUPERVISION_BASE;
	uint8_t hsr_supv_mask[6] = HSR_SUPERVISION_MASK;

	memset(&ipfEntryCfg, 0U, sizeof(netc_tb_ipf_config_t));
	ipfEntryCfg.cfge.fltfa = kNETC_IPFRedirectToMgmtPort;
	ipfEntryCfg.cfge.hr = kNETC_SoftwareDefHR0;

	drv->num_ipf_entries = 0;

	/* Set the default precedence value of the IPF entries */
	ipfEntryCfg.keye.precedence = NETC_IPF_PRECEDENCE_DEFAULT;

	/* IEEE 802.1Q-2018 - Table 10.1 - MRP application addresses */
	memcpy(ipfEntryCfg.keye.dmac, mmrp_mvrp_mac, 6);
	memcpy(ipfEntryCfg.keye.dmacMask, mmrp_mvrp_mask, 6);

	if (SWT_RxIPFAddTableEntry(&drv->handle, &ipfEntryCfg, &drv->ipf_entries_id[drv->num_ipf_entries]) != kStatus_Success)
		goto err_ipf;

	drv->num_ipf_entries++;

	/*  IEEE 802.1Q-2018 - Table 8.1 - C-VLAN and MAC Bridge addresses */
	memcpy(ipfEntryCfg.keye.dmac, vlan_reserved_mac, 6);
	memcpy(ipfEntryCfg.keye.dmacMask, vlan_reserved_mask, 6);

	if (SWT_RxIPFAddTableEntry(&drv->handle, &ipfEntryCfg, &drv->ipf_entries_id[drv->num_ipf_entries]) != kStatus_Success)
		goto err_ipf;

	drv->num_ipf_entries++;

	/* IEC 62439-3, section 5.7.2 - HSR supervision frame */
	ipfEntryCfg.cfge.fltfa = kNETC_IPFCopyToMgmtPort;
	memcpy(ipfEntryCfg.keye.dmac, hsr_supv_mac, 6);
	memcpy(ipfEntryCfg.keye.dmacMask, hsr_supv_mask, 6);

	if (SWT_RxIPFAddTableEntry(&drv->handle, &ipfEntryCfg, &drv->ipf_entries_id[drv->num_ipf_entries]) != kStatus_Success)
		goto err_ipf;

	drv->num_ipf_entries++;

#elif BRIDGE_TYPE == PROVIDER_BRIDGE
#error
#else
#error
#endif
	return 0;

err_ipf:
	return -1;
}

int netc_sw_add_or_update_et_table_entry(struct netc_sw_drv *drv, uint32_t et_eid, void (*update_entry)(netc_tb_et_config_t *entry, void *data), void *data)
{
	netc_tb_et_config_t ett_entry;
	status_t status;

	if (!update_entry)
		goto err;

	status = SWT_TxEPPQueryETTableEntry(&drv->handle, et_eid, &ett_entry);
	if (status == kStatus_NETC_NotFound) {
		memset(&ett_entry, 0, sizeof(netc_tb_et_config_t));

		ett_entry.entryID = et_eid;

		ett_entry.cfge.esqaTgtEID = NULL_ENTRY_ID;
		ett_entry.cfge.efmEID = NULL_ENTRY_ID;

		update_entry(&ett_entry, data);

		if (SWT_TxEPPAddETTableEntry(&drv->handle, &ett_entry) != kStatus_Success) {
			os_log(LOG_ERR, "SWT_TxEPPAddETTableEntry() failed\n");
			goto err;
		}
	} else {
		update_entry(&ett_entry, data);

		if (SWT_TxEPPUpdateETTableEntry(&drv->handle, &ett_entry) != kStatus_Success) {
			os_log(LOG_ERR, "SWT_RxPSFPUpdateISTableEntry() failed\n");
			goto err;
		}
	}

	return 0;

err:
	return -1;
}

int netc_sw_delete_et_table_entry(struct netc_sw_drv *drv, uint32_t et_eid)
{
	netc_tb_et_config_t config;
	int rc = 0;

	if (SWT_TxEPPQueryETTableEntry(&drv->handle, et_eid, &config) != kStatus_Success)
		goto err;

	if (SWT_TxEPPDelETTableEntry(&drv->handle, et_eid) != kStatus_Success) {
		os_log(LOG_ERR, "SWT_RxPSFPDelISTableEntry() failed\n");
		rc = -1;
		goto err;
	}

err:
	return rc;
}

/* FIXME */
static int netc_sw_fdb_fid_from_vid(unsigned short vid)
{
	return vid;
}

/* FIXME */
static int netc_sw_fdb_vid_from_fid(unsigned short fid)
{
	return fid;
}

int netc_sw_vft_find_entry(struct netc_sw_drv *drv, uint16_t vid, uint32_t *entry_id, netc_tb_vf_cfge_t *cfge)
{
	netc_tb_vf_rsp_data_t rsp;
	netc_tb_vf_keye_t keye = {0};
	int rc = -1;

	keye.vid = vid;

	if (SWT_BridgeQueryVFTableEntry(&drv->handle, &keye, &rsp) == kStatus_Success) {
		*entry_id = rsp.entryID;
		memcpy(cfge, &rsp.cfge, sizeof(netc_tb_vf_cfge_t));
		rc = 0;
	}

	return rc;
}

static bool netc_sw_is_pvid(struct netc_sw_drv *drv, uint16_t vid)
{
	int i;

	for (i = 0; i < CFG_NUM_NETC_SW_PORTS; i++) {
		if (drv->port[i] && (drv->port[i]->pvid == vid))
			return true;
	}

	return false;
}

static int netc_sw_vft_update_mfo(struct netc_sw_drv *drv, uint16_t vid, netc_swt_mac_forward_mode_t mfo)
{
	netc_tb_vf_config_t config;
	uint32_t entry_id;

	if (netc_sw_vft_find_entry(drv, vid, &entry_id, &config.cfge) == 0) {
		if (config.cfge.mfo != mfo) {
			config.cfge.mfo = mfo;

			if (SWT_BridgeUpdateVFTableEntry(&drv->handle, entry_id, &config.cfge) != kStatus_Success)
				goto err;
		}
	}

	return GENAVB_SUCCESS;
err:
	return -GENAVB_ERR_VLAN_HW_CONFIG;
}

static int netc_sw_set_pvid(struct net_port *port, uint16_t vid)
{
	struct netc_sw_drv *drv = net_port_drv(port);
	netc_hw_port_idx_t port_idx = port->base;
	uint16_t old_pvid;

	if (port->pvid == vid)
		goto done;

	if (SWT_BridgeConfigPortDefaultVid(&drv->handle, port_idx, vid) != kStatus_Success)
		goto err;

	old_pvid = port->pvid;
	port->pvid = vid;

	if (netc_sw_vft_update_mfo(drv, port->pvid, NETC_DEFAULT_PVID_MFO) < 0)
		goto err;

	/* if old pvid is no longer a pvid for any port(s), set NETC_DEFAULT_MFO */
	if (!netc_sw_is_pvid(drv, old_pvid)) {
		if (netc_sw_vft_update_mfo(drv, old_pvid, NETC_DEFAULT_MFO) < 0)
			goto err;
	}

done:
	return GENAVB_SUCCESS;
err:
	return -GENAVB_ERR_VLAN_HW_CONFIG;
}

static int netc_sw_get_pvid(struct net_port *port, uint16_t *vid)
{
	*vid = port->pvid;

	return GENAVB_SUCCESS;
}

static int netc_sw_vft_update(struct net_port *port, uint16_t vid, bool dynamic, struct genavb_vlan_port_map *map)
{
	struct netc_sw_drv *drv = net_port_drv(port);
	netc_hw_port_idx_t port_idx = port->base;
	netc_tb_vf_config_t config;
	uint32_t entry_id, mfo;

	if (netc_sw_is_pvid(drv, vid))
		mfo = NETC_DEFAULT_PVID_MFO;
	else
		mfo = NETC_DEFAULT_MFO;

	if (netc_sw_vft_find_entry(drv, vid, &entry_id, &config.cfge) < 0) {
		memset(&config, 0, sizeof(netc_tb_vf_config_t));

		config.cfge.mfo = mfo;
		config.cfge.mlo = drv->mlo;
		config.cfge.fid = netc_sw_fdb_fid_from_vid(vid);
		config.cfge.baseETEID = NETC_UNTAGGED_VLAN_ETT_BASE;
		config.keye.vid = vid;

		if (SWT_BridgeAddVFTableEntry(&drv->handle, &config, &entry_id) != kStatus_Success)
			goto err;
	}

	config.cfge.mfo = mfo;

	if (map->untagged)
		config.cfge.etaPortBitmap |= (1 << port_idx);
	else
		config.cfge.etaPortBitmap &= ~(1 << port_idx);

	if (map->control == GENAVB_VLAN_ADMIN_CONTROL_NORMAL ||
	    map->control == GENAVB_VLAN_ADMIN_CONTROL_FIXED ||
	    map->control == GENAVB_VLAN_ADMIN_CONTROL_REGISTERED) {
		config.cfge.portMembership |= (1 << port_idx);
	} else {
		config.cfge.portMembership &= ~(1 << port_idx);
	}

	if (SWT_BridgeUpdateVFTableEntry(&drv->handle, entry_id, &config.cfge) != kStatus_Success)
		goto err;

	return GENAVB_SUCCESS;
err:
	return -GENAVB_ERR_VLAN_HW_CONFIG;
}

static int netc_sw_vft_delete(struct net_bridge *bridge, uint16_t vid, bool dynamic)
{
	struct netc_sw_drv *drv = net_bridge_drv(bridge);
	netc_tb_vf_cfge_t cfge = {0};
	uint32_t entry_id = 0;

	if (netc_sw_vft_find_entry(drv, vid, &entry_id, &cfge))
		goto done;

	if (SWT_BridgeDelVFTableEntry(&drv->handle, entry_id) != kStatus_Success)
		goto err;

done:
	return GENAVB_SUCCESS;
err:
	return -GENAVB_ERR_VLAN_HW_CONFIG;
}

static int netc_sw_vft_read(struct net_bridge *bridge, uint16_t vid, bool *dynamic, struct genavb_vlan_port_map *map)
{
	struct netc_sw_drv *drv = net_bridge_drv(bridge);
	netc_tb_vf_cfge_t cfge = {0};
	uint32_t entry_id;
	int i, n;

	if (netc_sw_vft_find_entry(drv, vid, &entry_id, &cfge))
		goto err;

	n = 0;
	for (i = 0; i < CFG_NUM_NETC_SW_PORTS; i++) {
		if (drv->port[i]) {
			map[n].port_id = drv->port[i]->logical_port->id;

			if (cfge.portMembership & (1 << i))
				map[n].control = GENAVB_VLAN_ADMIN_CONTROL_FIXED;
			else
				map[n].control = GENAVB_VLAN_ADMIN_CONTROL_FORBIDDEN;

			if (cfge.etaPortBitmap & (1 << i))
				map[n].untagged = true;
			else
				map[n].untagged = false;

			n++;
		}
	}

	return GENAVB_SUCCESS;
err:
	return -GENAVB_ERR_VLAN_NOT_FOUND;
}

static int netc_sw_vft_dump(struct net_bridge *bridge, uint32_t *token, uint16_t *vid, bool *dynamic, struct genavb_vlan_port_map *map)
{
	struct netc_sw_drv *drv = net_bridge_drv(bridge);
	netc_tb_vf_search_criteria_t criteria = {0};
	int rc = -GENAVB_ERR_VLAN_NOT_FOUND;
	netc_tb_vf_rsp_data_t rsp;
	int i, n;

	if (*token)
		criteria.resumeEntryId = *token;
	else
		criteria.resumeEntryId = NULL_ENTRY_ID;

	if (SWT_BridgeSearchVFTableEntry(&drv->handle, &criteria, &rsp) == kStatus_Success) {
		*token = rsp.status;
		*vid = rsp.keye.vid;

		n = 0;
		for (i = 0; i < CFG_NUM_NETC_SW_PORTS; i++) {
			if (drv->port[i]) {
				map[n].port_id = drv->port[i]->logical_port->id;

				if (rsp.cfge.portMembership & (1 << i))
					map[n].control = GENAVB_VLAN_ADMIN_CONTROL_FIXED;
				else
					map[n].control = GENAVB_VLAN_ADMIN_CONTROL_FORBIDDEN;

				if (rsp.cfge.etaPortBitmap & (1 << i))
					map[n].untagged = true;
				else
					map[n].untagged = false;

				n++;
			}
		}

		rc = GENAVB_SUCCESS;
	} else
		*token = 0;

	return rc;
}

static int netc_sw_vft_update_mlo(struct net_bridge *bridge, uint16_t vid, uint8_t mlo)
{
	struct netc_sw_drv *drv = net_bridge_drv(bridge);
	netc_tb_vf_config_t config;
	uint32_t entry_id;

	if (netc_sw_vft_find_entry(drv, vid, &entry_id, &config.cfge) < 0)
		return -1;

	config.cfge.mlo = mlo;

	if (SWT_BridgeUpdateVFTableEntry(&drv->handle, entry_id, &config.cfge) != kStatus_Success)
		return -1;

	return 0;
}

static int netc_sw_software_maclearn(struct net_bridge *bridge, bool enable)
{
	struct genavb_vlan_port_map map[CFG_NUM_NETC_SW_PORTS];
	struct netc_sw_drv *drv = net_bridge_drv(bridge);
	uint32_t token, mlo;
	uint16_t vid;
	bool dynamic;
	int rc = 0;

	if (enable)
		mlo = kNETC_SeSoftwareMACLearn;
	else
		mlo = kNETC_HardwareMACLearn;

	if (drv->mlo == mlo)
		goto done;

	drv->mlo = mlo;
	drv->handle.hw.base->VFHTDECR2 &= ~NETC_SW_VFHTDECR2_MLO_MASK;
	drv->handle.hw.base->VFHTDECR2 |= NETC_SW_VFHTDECR2_MLO(mlo);

	/* Update the MLO of vlan entries in VF table */
	token = 0;
	while (!netc_sw_vft_dump(bridge, &token, &vid, &dynamic, map)) {
		if (netc_sw_vft_update_mlo(bridge, vid, mlo) < 0) {
			rc = -1;
			goto done;
		}
	}

done:
	return rc;
}

static int netc_sw_fdb_find_entry(struct netc_sw_drv *drv, uint8_t *address, uint16_t vid, uint32_t *entry_id, bool type, netc_tb_fdb_cfge_t *cfge)
{
	netc_tb_fdb_rsp_data_t rsp;
	netc_tb_fdb_keye_t keye = {0};
	int rc = -1;

	keye.fid = netc_sw_fdb_fid_from_vid(vid);
	memcpy(keye.macAddr, address, 6);

	if (SWT_BridgeQueryFDBTableEntry(&drv->handle, &keye, &rsp) == kStatus_Success) {
		*entry_id = rsp.entryID;
		memcpy(cfge, &rsp.cfge, sizeof(netc_tb_fdb_cfge_t));
		rc = 0;
	}

	return rc;
}

static int netc_sw_fdb_update(struct net_port *port, uint8_t *address, uint16_t vid, bool dynamic, genavb_fdb_port_control_t control)
{
	struct netc_sw_drv *drv = net_port_drv(port);
	netc_hw_port_idx_t port_idx = port->base;
	netc_tb_fdb_config_t config;
	uint32_t entry_id;
	int rc = 0;

	if (netc_sw_fdb_find_entry(drv, address, vid, &entry_id, dynamic, &config.cfge) < 0) {
		memset(&config, 0, sizeof(netc_tb_fdb_config_t));

		config.cfge.dynamic = dynamic;
		config.cfge.etEID = NULL_ENTRY_ID;
		config.keye.fid = netc_sw_fdb_fid_from_vid(vid);
		memcpy(config.keye.macAddr, address, 6);

		if (SWT_BridgeAddFDBTableEntry(&drv->handle, &config, &entry_id) != kStatus_Success) {
			rc = -1;
			goto err;
		}
	}

	if (control == GENAVB_FDB_PORT_CONTROL_FORWARDING) {
		if (config.cfge.portBitmap & (1 << port_idx))
			goto done;

		config.cfge.portBitmap |= (1 << port_idx);
	} else {
		if (! (config.cfge.portBitmap & (1 << port_idx)))
			goto done;

		config.cfge.portBitmap &= ~(1 << port_idx);
	}

	if (SWT_BridgeUpdateFDBTableEntry(&drv->handle, entry_id, &config.cfge) != kStatus_Success)
		rc = -1;

err:
done:
	return rc;
}

static int netc_sw_fdb_delete(struct net_bridge *bridge, uint8_t *address, uint16_t vid, bool dynamic)
{
	struct netc_sw_drv *drv = net_bridge_drv(bridge);
	netc_tb_fdb_cfge_t cfge = {0};
	uint32_t entry_id;
	int rc = 0;

	if (netc_sw_fdb_find_entry(drv, address, vid, &entry_id, dynamic, &cfge) < 0)
		goto err;

	if (SWT_BridgeDelFDBTableEntry(&drv->handle, entry_id) != kStatus_Success)
		rc = -1;

err:
	return rc;
}

static int netc_sw_fdb_read(struct net_bridge *bridge, uint8_t *address, uint16_t vid, bool *dynamic, struct genavb_fdb_port_map *map, genavb_fdb_status_t *status)
{
	struct netc_sw_drv *drv = net_bridge_drv(bridge);
	netc_tb_fdb_cfge_t cfge;
	netc_tb_fdb_acte_t acte;
	uint32_t entry_id;
	int i, n;
	int rc = 0;

	if (netc_sw_fdb_find_entry(drv, address, vid, &entry_id, dynamic, &cfge) < 0) {
		rc = -1;
		goto err;
	}

	if (SWT_BridgeGetFDBActivityState(&drv->handle, entry_id, &acte) != kStatus_Success) {
		rc = -1;
		goto err;
	}

	n = 0;
	for (i = 0; i < CFG_NUM_NETC_SW_PORTS; i++) {
		if (drv->port[i]) {
			map[n].port_id = drv->port[i]->logical_port->id;

			if (cfge.portBitmap & (1 << i))
				map[n].control = GENAVB_FDB_PORT_CONTROL_FORWARDING;
			else
				map[n].control = GENAVB_FDB_PORT_CONTROL_FILTERING;

			n++;
		}
	}

	*dynamic = cfge.dynamic;
	*status = acte.actFlag; /* FIXME, improve handling of activity flag to determine invalid/learned/... */

err:
	return rc;
}

static int netc_sw_fdb_dump(struct net_bridge *bridge, uint32_t *token, uint8_t *address, uint16_t *vid, bool *dynamic, struct genavb_fdb_port_map *map, genavb_fdb_status_t *status)
{
	struct netc_sw_drv *drv = net_bridge_drv(bridge);
	netc_tb_fdb_search_criteria_t criteria = {0};
	netc_tb_fdb_rsp_data_t rsp;
	int i, n;
	int rc = -1;

	if (*token)
		criteria.resumeEntryId = *token;
	else
		criteria.resumeEntryId = NULL_ENTRY_ID;

	criteria.keyeMc = kNETC_FDBKeyeMacthAny;
	criteria.cfgeMc = kNETC_FDBCfgeMacthAny;
	criteria.acteMc = kNETC_FDBActeMacthAny;

	if (SWT_BridgeSearchFDBTableEntry(&drv->handle, &criteria, &rsp) == kStatus_Success) {
		memcpy(address, rsp.keye.macAddr, 6);
		*token = rsp.status;
		*vid = netc_sw_fdb_vid_from_fid(rsp.keye.fid);
		*dynamic = rsp.cfge.dynamic;
		*status = rsp.acte.actFlag;

		n = 0;
		for (i = 0; i < CFG_NUM_NETC_SW_PORTS; i++) {
			if (drv->port[i]) {
				map[n].port_id = drv->port[i]->logical_port->id;

				if (rsp.cfge.portBitmap & (1 << i))
					map[n].control = GENAVB_FDB_PORT_CONTROL_FORWARDING;
				else
					map[n].control = GENAVB_FDB_PORT_CONTROL_FILTERING;

				n++;
			}
		}

		rc = 0;
	} else
		*token = 0;

	return rc;
}

static int netc_sw_post_init(struct net_port *port)
{
	return 0;
}

static void netc_sw_pre_exit(struct net_port *port)
{

}

__exit static void netc_sw_port_exit(struct net_port *port)
{

}

__init int netc_sw_port_init(struct net_port *port)
{
	if (port->drv_index >= CFG_NUM_NETC_SW)
		return -1;

	if (port->base >= CFG_NUM_NETC_SW_PORTS)
		return -1;

	port->drv_ops.add_multi = netc_sw_add_multi;
	port->drv_ops.del_multi = netc_sw_del_multi;
	port->drv_ops.link_up = netc_sw_link_up;
	port->drv_ops.link_down = netc_sw_link_down;
	port->drv_ops.set_tx_queue_config = netc_sw_set_tx_queue_config;
	port->drv_ops.set_tx_idle_slope = netc_sw_set_tx_idle_slope;
	port->drv_ops.set_priority_to_tc_map = netc_sw_set_priority_to_tc_map;
	port->drv_ops.set_st_config = netc_sw_set_st_config;
	port->drv_ops.get_st_config = netc_sw_get_st_config;
	port->drv_ops.st_set_max_sdu = netc_sw_set_st_max_sdu;
	port->drv_ops.st_get_max_sdu = netc_sw_get_st_max_sdu;
	port->drv_ops.st_max_entries = netc_sw_st_max_entries;
	port->drv_ops.set_fp = netc_sw_set_fp;
	port->drv_ops.get_fp = netc_sw_get_fp;
	port->drv_ops.set_max_frame_size = netc_sw_set_max_frame_size;
	port->drv_ops.exit = netc_sw_port_exit;
	port->drv_ops.post_init = netc_sw_post_init;
	port->drv_ops.pre_exit = netc_sw_pre_exit;

	port->drv_ops.vlan_update = netc_sw_vft_update;
	port->drv_ops.fdb_update = netc_sw_fdb_update;

	port->drv_ops.set_pvid = netc_sw_set_pvid;
	port->drv_ops.get_pvid = netc_sw_get_pvid;

	port->drv = &netc_sw_drivers[port->drv_index];

	port->num_tx_q = 1;
	port->num_rx_q = 1;

	if (netc_1588_freerunning_available(port->hw_clock_id)) {
		clock_to_hw_clock_set(port->clock[PORT_CLOCK_LOCAL], HW_CLOCK_PORT_BASE + port->hw_clock_id + 1);
		clock_to_hw_clock_set(port->clock[PORT_CLOCK_GPTP_1], HW_CLOCK_PORT_BASE + port->hw_clock_id + 1);

		clock_set_flags(port->clock[PORT_CLOCK_GPTP_0], OS_CLOCK_FLAGS_HW_RATIO | OS_CLOCK_FLAGS_HW_OFFSET);
		clock_set_flags(port->clock[PORT_CLOCK_GPTP_1], 0);
	}

	((struct netc_sw_drv *)port->drv)->port[port->base] = port;

	os_log(LOG_INFO, "port(%u) driver handle(%p)\n", port->index, port->drv);

	return 0;
}

__exit void netc_sw_free(struct net_bridge *bridge)
{

}

__exit static void netc_sw_exit(struct net_bridge *bridge)
{
	struct netc_sw_drv *drv = net_bridge_drv(bridge);

	netc_sw_ts_flush(drv);

	netc_sw_del_ingress_filtering(drv);

	netc_sw_psfp_exit(bridge);

	SWT_Deinit(&drv->handle);

	netc_sw_free(bridge);

	bridge->drv = NULL;
}

__init static void port_pcp_to_queue_init(struct net_port *port, swt_config_t *config, unsigned int bridge_port_id)
{
	int i;

	port_set_priority_to_traffic_class_map(port, QOS_TRAFFIC_CLASS_MAX, QOS_SR_CLASS_MAX, NULL);

	port->max_pdu = QOS_MAX_PDU_BYTES;

	for (i = 0; i < QOS_PRIORITY_MAX; i++) {
		config->ports[bridge_port_id].ipvToTC[i] = port->map[i];
		config->ports[bridge_port_id].txTcCfg[i].sduCfg.maxSduSized = QOS_MAX_SDU_BYTES;
	}
}

__init static void port_remove_vlan_tag_egress_treatment(netc_tb_et_config_t *entry, void *data)
{
	/* Egress Frame Modification: Delete Outer VLAN Tag */
	entry->cfge.efmLenChange = -4;
	entry->cfge.efmEID = NETC_FM_DEL_VLAN_TAG;
}

__init static int netc_sw_egress_treatment_init(struct netc_sw_drv *drv)
{
	unsigned int port;
	int rc = 0;

	/**
	 * Add Egress Treatment group for VLAN untagged frame modification
	 * One Egress Treatment entry per egress port
	 * All VLANs use the same group, activating the set of entries/ports where transmission is untagged
	 */
	for (port = 0; port < CFG_NUM_NETC_SW_PORTS; port++) {
		if (netc_sw_add_or_update_et_table_entry(drv, NETC_UNTAGGED_VLAN_ETT_BASE + port, port_remove_vlan_tag_egress_treatment, NULL) < 0) {
			os_log(LOG_ERR, "netc_sw_add_or_update_et_table_entry failed\n");
			rc = -1;
			goto err;
		}
	}

err:
	return rc;
}

__init static void port_vlan_init(struct net_port *port, swt_config_t *config, unsigned int bridge_port_id)
{
	config->ports[bridge_port_id].bridgeCfg.isRxVlanAware = true;

	config->ports[bridge_port_id].bridgeCfg.txVlanAction = kNETC_TxDelOuterVlan;

	port->pvid = VLAN_PVID_DEFAULT;

	config->ports[bridge_port_id].bridgeCfg.defaultVlan.vid = port->pvid;
	config->ports[bridge_port_id].bridgeCfg.defaultVlan.dei = VLAN_DEI_DEFAULT;
	config->ports[bridge_port_id].bridgeCfg.defaultVlan.pcp = 0;

#if BRIDGE_TYPE == CUSTOMER_VLAN_BRIDGE
	config->ports[bridge_port_id].bridgeCfg.defaultVlan.tpid = 0;
#elif BRIDGE_TYPE == PROVIDER_BRIDGE
	config->ports[bridge_port_id].bridgeCfg.defaultVlan.tpid = 1;
#else
#error
#endif
}

__init static void switch_port_init(struct net_port *port, swt_config_t *config, unsigned int bridge_port_id)
{
	config->ports[bridge_port_id].enTxRx = true;

	/* time gate scheduling is required for CBS */
	config->ports[bridge_port_id].enableTg = true;

	config->ports[bridge_port_id].ethMac.miiMode = port->mii_mode;
	config->ports[bridge_port_id].ethMac.miiSpeed = kNETC_MiiSpeed100M;
	config->ports[bridge_port_id].ethMac.miiDuplex = kNETC_MiiFullDuplex;

	if (netc_1588_freerunning_available(port->hw_clock_id)) {
		config->ports[bridge_port_id].ethMac.txTsSelect = kNETC_FreeRunningTime;
		config->ports[bridge_port_id].commonCfg.rxTsSelect = kNETC_FreeRunningTime;
	}

	config->ports[bridge_port_id].commonCfg.qosMode.vlanQosMap = 0;
	config->ports[bridge_port_id].commonCfg.qosMode.defaultIpv = 0;
	config->ports[bridge_port_id].commonCfg.qosMode.defaultDr = 2;
	config->ports[bridge_port_id].commonCfg.qosMode.enVlanInfo = true;
	config->ports[bridge_port_id].commonCfg.qosMode.vlanTagSelect = true;

	/* Map each priority to a dedicated buffer pool */
	config->ports[bridge_port_id].ipvToBP[0] = 0;
	config->ports[bridge_port_id].ipvToBP[1] = 1;
	config->ports[bridge_port_id].ipvToBP[2] = 2;
	config->ports[bridge_port_id].ipvToBP[3] = 3;
	config->ports[bridge_port_id].ipvToBP[4] = 4;
	config->ports[bridge_port_id].ipvToBP[5] = 5;
	config->ports[bridge_port_id].ipvToBP[6] = 6;
	config->ports[bridge_port_id].ipvToBP[7] = 7;

	config->ports[bridge_port_id].commonCfg.ipfCfg.enIPFTable = true;

	config->ports[bridge_port_id].bridgeCfg.enMacStationMove = true;

	port_vlan_init(port, config, bridge_port_id);

	port_pcp_to_queue_init(port, config, bridge_port_id);

	netc_sw_port_to_net_port[bridge_port_id] = port->index;
}

#define NETC_QUEUE_MAX_M	256
#define NETC_QUEUE_MAX_N	32

#define NETC_POOL_MAX_M		256
#define NETC_POOL_MAX_N		16

__init static unsigned int get_m_n(unsigned int val, unsigned int max_m, unsigned int max_n, unsigned int *m, unsigned int *n)
{
	*m = val;
	*n = 0;

	while (*m >= max_m) {
		(*n)++;
		(*m) >>= 1;
	}

	if (*n >= max_n)
		*n = max_n ? max_n - 1 : 0;

	return (*m) * (1 << (*n));
}

__init static unsigned int get_pool_m_n(unsigned int val, unsigned int *m, unsigned int *n)
{
	return get_m_n(val, NETC_POOL_MAX_M, NETC_POOL_MAX_N, m, n);
}

__init static unsigned int get_queue_m_n(unsigned int val, unsigned int *m, unsigned int *n)
{
	return get_m_n(val, NETC_QUEUE_MAX_M, NETC_QUEUE_MAX_N, m, n);
}

__init static void etm_congestion_group_init(struct netc_sw_drv *drv, unsigned int port_id)
{
	netc_tb_etmcg_config_t config;
	unsigned int m, n;
	int i;

	/* ETM Congestion group table */
	/* One entry per port and group */
	/* Same threshold for all DR */
	/* Never drop frames for DR=0 */
	config.cfge.tdDr0En = 0;
	config.cfge.tdDr1En = 1;
	config.cfge.tdDr2En = 1;
	config.cfge.tdDr3En = 1;
	config.cfge.oal = 0;

	for (i = 0; i < NUM_TC; i++) {
		config.entryID = (port_id << 4) | i;

		get_queue_m_n(drv->tc_bytes[i], &m, &n);

		config.cfge.tdDRThresh[0].ta = m;
		config.cfge.tdDRThresh[1].ta = m;
		config.cfge.tdDRThresh[2].ta = m;
		config.cfge.tdDRThresh[3].ta = m;

		config.cfge.tdDRThresh[0].tn = n;
		config.cfge.tdDRThresh[1].tn = n;
		config.cfge.tdDRThresh[2].tn = n;
		config.cfge.tdDRThresh[3].tn = n;

		if (SWT_TxETMConfigCongestionGroup(&drv->handle, &config) != kStatus_Success)
			os_log(LOG_ERR, "SWT_TxETMConfigCongestionGroup(%u, %u) failed\n",
			       port_id, i);
		else
			os_log(LOG_INIT, "port(%u), queue(%u), bytes: %u\n",
			port_id, i, m * (1 << n));
	}
}

__init static void etm_class_init(struct netc_sw_drv *drv, unsigned int port_id)
{
	/* ETM Class queue table */
	/* One entry per port and queue */
	/* By default each queue is associated to a separate congestion group */
	/* All ports are configured the same */
}

__init static void buffer_shared_pool_init(struct netc_sw_drv *drv, unsigned int pool_id, unsigned int words)
{
	netc_tb_sbp_config_t config;
	unsigned int m, n;

	get_pool_m_n(words, &m, &n);

	config.entryID = pool_id;
	config.cfge.maxThresh = NETC_TB_BP_THRESH(m, n);
	config.cfge.fcOnThresh = NETC_TB_BP_THRESH(0, 0);
	config.cfge.fcOffThresh = NETC_TB_BP_THRESH(0, 0);

	if (SWT_UpdateSBPTableEntry(&drv->handle, &config) != kStatus_Success)
		os_log(LOG_ERR, "SWT_UpdateSBPTableEntry(%u) failed\n", pool_id);
	else
		os_log(LOG_INIT, "buffer_shared_pool(%u), words: %u\n",
			pool_id, m * (1 << n));
}

__init static void buffer_pool_init(struct netc_sw_drv *drv, unsigned int pool_id, unsigned int words, unsigned int shared_pool_id, unsigned int shared_words)
{
	netc_tb_bp_config_t config;
	unsigned int m, n, shared_m, shared_n;

	config.entryID = pool_id;

	config.cfge.gcCfg = kNETC_FlowCtrlDisable;

	get_pool_m_n(words, &m, &n);
	config.cfge.maxThresh = NETC_TB_BP_THRESH(m, n);

	if (shared_words) {
		config.cfge.sbpEn = true;

		get_pool_m_n(shared_words, &shared_m, &shared_n);
		config.cfge.sbpThresh = NETC_TB_BP_THRESH(shared_m, shared_n);

		config.cfge.sbpEid = shared_pool_id;
	} else {
		config.cfge.sbpEn = false;
		config.cfge.sbpThresh = NETC_TB_BP_THRESH(0, 0);
		config.cfge.sbpEid = 0;
	}

	config.cfge.fcOnThresh = NETC_TB_BP_THRESH(0, 0);
	config.cfge.fcOffThresh = NETC_TB_BP_THRESH(0, 0);
	config.cfge.fcPorts = 0;

	if (SWT_UpdateBPTableEntry(&drv->handle, &config) != kStatus_Success)
		os_log(LOG_ERR, "SWT_UpdateBPTableEntry(%u) failed\n", pool_id);
	else {
		if (shared_words) {
			os_log(LOG_INIT, "buffer_pool(%u), words: %u, shared_pool(%u), words: %u\n",
			pool_id, m * (1 << n), shared_pool_id, (m * (1 << n) - shared_m * (1 << shared_n)));
		} else {
			os_log(LOG_INIT, "buffer_pool(%u), words: %u\n",
			pool_id, m * (1 << n));
		}
	}
}

__init static void buffering_init(struct netc_sw_drv *drv, swt_config_t *swt_config)
{
	unsigned int total_bytes, word_size, tc_min_bytes;
	unsigned int words, tc_min_words, pool_min_words, pool_max_words, shared_words;
	int i;

	words = (drv->handle.hw.base->SMBCAPR & NETC_SW_SMBCAPR_NUM_WORDS_MASK) >> NETC_SW_SMBCAPR_NUM_WORDS_SHIFT;

	switch ((drv->handle.hw.base->SMBCAPR & NETC_SW_SMBCAPR_WORD_SIZE_MASK) >> NETC_SW_SMBCAPR_WORD_SIZE_SHIFT) {
	case 0:
	default:
		word_size = 24;
		break;
	}

	total_bytes = words * word_size;

	os_log(LOG_INIT, "words: %u, size: %u, total: %u KiB\n",
	       words, word_size, total_bytes / 1024);

	/* Buffering scheme
	 * ================
	 * - i.MX RT1180 hardware: 8 buffer pools + 2 shared pools
	 * - One buffer pool per priority, common to all ports
	 * - One shared pool for the lowest priorities and all ports
	 * - All buffer pools have the same fixed minimum number of words, to support
	 *   the port standard MTU
	 * - The shared pool, with all remaining words, is used to support
	 *   best effort/backgroud traffic high bandwidth
	 * - All traffic class queues also have the same minimal size
	 * - Best Effort/Background traffic class queues have double the size
	 */

	tc_min_bytes = QOS_MAX_PDU_BYTES;
	tc_min_words = tc_min_bytes / word_size;

	pool_min_words = tc_min_words * drv->ports;
	pool_max_words = 2 * pool_min_words;
	shared_words = words - pool_min_words * NUM_TC;

	drv->tc_bytes[0] = 2 * tc_min_bytes;
	drv->tc_bytes[1] = 2 * tc_min_bytes;
	drv->tc_bytes[2] = tc_min_bytes;
	drv->tc_bytes[3] = tc_min_bytes;
	drv->tc_bytes[4] = tc_min_bytes;
	drv->tc_bytes[5] = tc_min_bytes;
	drv->tc_bytes[6] = tc_min_bytes;
	drv->tc_bytes[7] = tc_min_bytes;

	/* Initialize shared buffer pool */
	buffer_shared_pool_init(drv, 0, shared_words);

	/* Initialize buffer pools */
	for (i = 0; i < 2; i++)
		buffer_pool_init(drv, i, pool_max_words, 0, pool_min_words);

	for (i = 2; i < NUM_TC; i++)
		buffer_pool_init(drv, i, pool_min_words, 0, 0);

	for (i = 0; i < CFG_NUM_NETC_SW_PORTS; i++) {
		if (!swt_config->ports[i].enTxRx)
			continue;

		/* Initialize traffic classes */
		etm_class_init(drv, i);

		/* Initialize queue sizes */
		etm_congestion_group_init(drv, i);
	}
}

__init int netc_sw_alloc(struct net_bridge *bridge)
{
	return 0;
}

__init int netc_sw_init(struct net_bridge *bridge)
{
	struct netc_sw_drv *drv;
	swt_config_t swt_config;
	swt_transfer_config_t tx_rx_config;
	struct net_port *ep_port = NULL;
	struct net_port *sw_port = NULL;
	unsigned int net_port_idx, bridge_port_id;

	if (bridge->drv_index >= CFG_NUM_NETC_SW)
		goto err_drv_index;

	bridge->drv_ops.get_rx_frame_size = netc_sw_get_rx_frame_size;
	bridge->drv_ops.read_frame = netc_sw_read_frame;
	bridge->drv_ops.read_egress_ts_frame = netc_sw_read_egress_ts_frame;
	bridge->drv_ops.send_frame = netc_sw_send_frame;
	bridge->drv_ops.tx_cleanup = netc_sw_tx_cleanup;
	bridge->drv_ops.exit = netc_sw_exit;
	bridge->drv_ops.fdb_delete = netc_sw_fdb_delete;
	bridge->drv_ops.fdb_dump = netc_sw_fdb_dump;
	bridge->drv_ops.fdb_read = netc_sw_fdb_read;
	bridge->drv_ops.vlan_delete = netc_sw_vft_delete;
	bridge->drv_ops.vlan_dump = netc_sw_vft_dump;
	bridge->drv_ops.vlan_read = netc_sw_vft_read;
	bridge->drv_ops.software_maclearn = netc_sw_software_maclearn;

	netc_sw_frer_init(bridge);

	bridge->drv = &netc_sw_drivers[bridge->drv_index];

	drv = net_bridge_drv(bridge);

	drv->ports = 0;
	drv->use_masquerade = false;
	drv->st_enabled = 0;

	netc_sw_ts_flush(drv);

	/* Endpoint PSI may already be initialized through the EP driver.
	Parse the whole ports table and find the EP pseudo entry and at
	least on switch port */
	for (net_port_idx = 0; net_port_idx < CFG_PORTS; net_port_idx ++) {
		if ((!ep_port) && (ports[net_port_idx].drv_type == ENETC_PSEUDO_1G_t)) {
			ep_port = &ports[net_port_idx];
			continue;
		}

		if ((!sw_port) && (ports[net_port_idx].drv_type == NETC_SW_t)) {
			sw_port = &ports[net_port_idx];
			continue;
		}
	}

	/* No speudo MAC endpoint or switch port found in the configuration, the switch cannot be initialized */
	if ((ep_port == NULL) || (sw_port == NULL))
		goto err_port_config;

	drv->ep_handle = (ep_handle_t *)enetc_ep_get_handle(ep_port);

	/* Switch configuration. */
	SWT_GetDefaultConfig(&swt_config);

#ifdef BOARD_NET_RX_CACHEABLE
	swt_config.rxCacheMaintain = true;
#endif
#ifdef BOARD_NET_TX_CACHEABLE
	swt_config.txCacheMaintain = true;
#endif

	/* Setup default behaviour to discard */
	swt_config.bridgeCfg.dVFCfg.mfo = NETC_DEFAULT_MFO;
	drv->mlo = swt_config.bridgeCfg.dVFCfg.mlo;

	swt_config.bridgeCfg.dVFCfg.portMembership = 0;

	/* Disable all ports by default */
	for (bridge_port_id = 0; bridge_port_id < CFG_NUM_NETC_SW_PORTS; bridge_port_id++)
		swt_config.ports[bridge_port_id].enTxRx = false;

	/* Enable ports found in the configuration */
	for (net_port_idx = 0; net_port_idx < CFG_PORTS; net_port_idx++) {
		struct net_port *port = &ports[net_port_idx];

		if (port->drv_type == NETC_SW_t) {
			switch_port_init(port, &swt_config, port->base);
			drv->ports++;
		}
	}

	swt_config.cmdRingUse = 1U;
	swt_config.cmdBdrCfg[0].bdBase = &g_cmd_buff_desc[0];
	swt_config.cmdBdrCfg[0].bdLength = NETC_SW_CMDBD_NUM;

	if (SWT_Init(&drv->handle, &swt_config) != kStatus_Success)
		goto err_swt_init;

	if (netc_sw_alloc(bridge) < 0)
		goto err_alloc;

	for (net_port_idx = 0; net_port_idx < CFG_PORTS; net_port_idx++) {
		if (ports[net_port_idx].drv_type == NETC_SW_t)
			netc_sw_stats_init(&ports[net_port_idx]);
	}

	buffering_init(drv, &swt_config);

	for (uint8_t index = 0U; index < NETC_SW_RXBD_NUM/2; index++)
		sw_rx_buff_addr[index] = (uintptr_t)&sw_rx_buff[index];

	memset(&tx_rx_config, 0, sizeof(tx_rx_config));

	tx_rx_config.rxZeroCopy = false;
	tx_rx_config.reclaimCallback = netc_sw_reclaim_cb;
	tx_rx_config.userData = (void *)ports;
	tx_rx_config.enUseMgmtRxBdRing = true;
	tx_rx_config.mgmtRxBdrConfig.bdArray = &sw_rx_desc[0];
	tx_rx_config.mgmtRxBdrConfig.len = NETC_SW_RXBD_NUM;
	tx_rx_config.mgmtRxBdrConfig.extendDescEn = true;
	tx_rx_config.mgmtRxBdrConfig.buffAddrArray = &sw_rx_buff_addr[0];
	tx_rx_config.mgmtRxBdrConfig.buffSize = NETC_SW_RXBUFF_SIZE_ALIGN;

	tx_rx_config.enUseMgmtTxBdRing = true;
	tx_rx_config.mgmtTxBdrConfig.bdArray = &sw_tx_desc[0],
	tx_rx_config.mgmtTxBdrConfig.priority = 2;
	tx_rx_config.mgmtTxBdrConfig.len = NETC_SW_TXBD_NUM;
	tx_rx_config.mgmtTxBdrConfig.dirtyArray = &sw_tx_dirty[0];

	/* Config switch transfer resource */
	if (SWT_ManagementTxRxConfig(&drv->handle, drv->ep_handle, &tx_rx_config) != kStatus_Success)
		goto err_mgmt_config;

	if (netc_sw_set_ingress_filtering(drv) < 0) {
		os_log(LOG_ERR, "failed to set ingress filtering\n");
		goto err_ipf;
	}

	if (netc_sw_egress_treatment_init(drv) < 0) {
		os_log(LOG_ERR, "netc_sw_egress_treatment_init failed\n");
		goto err_et;
	}

	if (netc_si_init(bridge) < 0) {
		os_log(LOG_ERR, "netc_si_init failed\n");
		goto err_si;
	}

	if (netc_sw_psfp_init(bridge) < 0) {
		os_log(LOG_ERR, "netc_sw_psfp_init failed\n");
		goto err_psfp;
	}

	netc_sw_dsa_init(bridge);

	os_log(LOG_INFO, "driver handle(%p)\n", drv);

	return 0;

err_psfp:
err_si:
err_et:
	netc_sw_del_ingress_filtering(drv);
err_ipf:
err_mgmt_config:
	netc_sw_free(bridge);
err_alloc:
	SWT_Deinit(&drv->handle);

err_swt_init:
err_port_config:
err_drv_index:
	return -1;
}
#else
__init int netc_sw_init(struct net_bridge *bridge) { return -1; }
__init int netc_sw_port_init(struct net_port *port) { return -1; }
#endif /* CFG_NUM_NETC_SW */
