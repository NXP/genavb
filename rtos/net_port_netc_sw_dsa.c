/*
* Copyright 2023-2024 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

#include "net_bridge.h"

#if defined(CONFIG_DSA)

#include "fsl_netc.h"

#include "common/log.h"
#include "genavb/error.h"
#include "genavb/ether.h"
#include "net_logical_port.h"
#include "net_port_netc_sw.h"
#include "net_port_netc_sw_drv.h"

#include "net_port_dsa.h"

/* hard-coded entry ID of Ingress Stream Table (IS_EID) */
#define IS_EID_INGRESS_BRIDGE_FORWARD_BASE 350
#define IS_EID_INGRESS_BRIDGE_FORWARD_SW_PORT_0 (IS_EID_INGRESS_BRIDGE_FORWARD_BASE + 0)
#define IS_EID_INGRESS_BRIDGE_FORWARD_SW_PORT_1 (IS_EID_INGRESS_BRIDGE_FORWARD_BASE + 1)
#define IS_EID_INGRESS_BRIDGE_FORWARD_SW_PORT_2 (IS_EID_INGRESS_BRIDGE_FORWARD_BASE + 2)
#define IS_EID_INGRESS_BRIDGE_FORWARD_SW_PORT_3 (IS_EID_INGRESS_BRIDGE_FORWARD_BASE + 3)
#define IS_EID_INGRESS_BRIDGE_FORWARD_SW_PORT_4 (IS_EID_INGRESS_BRIDGE_FORWARD_BASE + 4)

#define IS_EID_INGRESS_STREAM_FORWARD_BASE 355
#define IS_EID_INGRESS_STREAM_FORWARD_SW_PORT_0 (IS_EID_INGRESS_STREAM_FORWARD_BASE + 0)
#define IS_EID_INGRESS_STREAM_FORWARD_SW_PORT_1 (IS_EID_INGRESS_STREAM_FORWARD_BASE + 1)
#define IS_EID_INGRESS_STREAM_FORWARD_SW_PORT_2 (IS_EID_INGRESS_STREAM_FORWARD_BASE + 2)
#define IS_EID_INGRESS_STREAM_FORWARD_SW_PORT_3 (IS_EID_INGRESS_STREAM_FORWARD_BASE + 3)
#define IS_EID_INGRESS_STREAM_FORWARD_SW_PORT_4 (IS_EID_INGRESS_STREAM_FORWARD_BASE + 4)

#define IS_EID_EGRESS_STREAM_FORWARD_BASE 360
#define IS_EID_EGRESS_STREAM_FORWARD_SW_PORT_0 (IS_EID_EGRESS_STREAM_FORWARD_BASE + 0)
#define IS_EID_EGRESS_STREAM_FORWARD_SW_PORT_1 (IS_EID_EGRESS_STREAM_FORWARD_BASE + 1)
#define IS_EID_EGRESS_STREAM_FORWARD_SW_PORT_2 (IS_EID_EGRESS_STREAM_FORWARD_BASE + 2)
#define IS_EID_EGRESS_STREAM_FORWARD_SW_PORT_3 (IS_EID_EGRESS_STREAM_FORWARD_BASE + 3)
#define IS_EID_EGRESS_STREAM_FORWARD_SW_PORT_4 (IS_EID_EGRESS_STREAM_FORWARD_BASE + 4)

/* hard-coded entry ID of Egress Treatment Table (ET_EID) */
#define ET_EID_INGRESS_SW_PORT_0_BASE 350
#define ET_EID_INGRESS_SW_PORT_1_BASE 351
#define ET_EID_INGRESS_SW_PORT_2_BASE 352
#define ET_EID_INGRESS_SW_PORT_3_BASE 353
#define ET_EID_INGRESS_SW_PORT_4_BASE 354

#if defined(CFG_DSA_CPU_PORT_ENETC)
#define ET_EID_EGRESS_SW_PORT_0_BASE 355
#define ET_EID_EGRESS_SW_PORT_1_BASE 356
#define ET_EID_EGRESS_SW_PORT_2_BASE 357
#define ET_EID_EGRESS_SW_PORT_3_BASE 358
#define ET_EID_EGRESS_SW_PORT_4_BASE 359
#endif

enum {
	NETC_DSA_IPF_PRECEDENCE_DEFAULT = NETC_IPF_PRECEDENCE_DEFAULT,
	NETC_DSA_IPF_PRECEDENCE_DSA,
	NETC_DSA_IPF_PRECEDENCE_PTP
};

const static uint32_t is_entry_id_ingress_bridge_forward[CFG_NUM_NETC_SW_PORTS] = {
	IS_EID_INGRESS_BRIDGE_FORWARD_SW_PORT_0,
	IS_EID_INGRESS_BRIDGE_FORWARD_SW_PORT_1,
	IS_EID_INGRESS_BRIDGE_FORWARD_SW_PORT_2,
	IS_EID_INGRESS_BRIDGE_FORWARD_SW_PORT_3,
	IS_EID_INGRESS_BRIDGE_FORWARD_SW_PORT_4
};

const static uint32_t is_entry_id_ingress_stream_forward[CFG_NUM_NETC_SW_PORTS] = {
	IS_EID_INGRESS_STREAM_FORWARD_SW_PORT_0,
	IS_EID_INGRESS_STREAM_FORWARD_SW_PORT_1,
	IS_EID_INGRESS_STREAM_FORWARD_SW_PORT_2,
	IS_EID_INGRESS_STREAM_FORWARD_SW_PORT_3,
	IS_EID_INGRESS_STREAM_FORWARD_SW_PORT_4
};

const static uint32_t is_entry_id_egress_stream_forward[CFG_NUM_NETC_SW_PORTS] = {
	IS_EID_EGRESS_STREAM_FORWARD_SW_PORT_0,
	IS_EID_EGRESS_STREAM_FORWARD_SW_PORT_1,
	IS_EID_EGRESS_STREAM_FORWARD_SW_PORT_2,
	IS_EID_EGRESS_STREAM_FORWARD_SW_PORT_3,
	IS_EID_EGRESS_STREAM_FORWARD_SW_PORT_4
};

const static uint32_t et_entry_id_ingress[CFG_NUM_NETC_SW_PORTS] = {
	ET_EID_INGRESS_SW_PORT_0_BASE,
	ET_EID_INGRESS_SW_PORT_1_BASE,
	ET_EID_INGRESS_SW_PORT_2_BASE,
	ET_EID_INGRESS_SW_PORT_3_BASE,
	ET_EID_INGRESS_SW_PORT_4_BASE,
};

#if defined(CFG_DSA_CPU_PORT_ENETC)
const static uint32_t et_entry_id_egress[CFG_NUM_NETC_SW_PORTS] = {
	ET_EID_EGRESS_SW_PORT_0_BASE,
	ET_EID_EGRESS_SW_PORT_1_BASE,
	ET_EID_EGRESS_SW_PORT_2_BASE,
	ET_EID_EGRESS_SW_PORT_3_BASE,
	ET_EID_EGRESS_SW_PORT_4_BASE,
};
#endif

/* store the hardware generated entry ID of Ingress Port Filter Table */
static uint32_t ipf_entry_id_ingress_bridge_forward[CFG_NUM_NETC_SW_PORTS] = {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff};
static uint32_t ipf_entry_id_ingress_stream_forward[CFG_NUM_NETC_SW_PORTS] = {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff};
static uint32_t ipf_entry_id_egress_stream_forward[CFG_NUM_NETC_SW_PORTS] = {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff};


/* Find an entry in Egress Treatment Table based on entry ID */
static int netc_sw_et_find_entry(uint32_t entry_id)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	struct netc_sw_drv *drv = net_bridge_drv(bridge);
	netc_tb_et_config_t et_entry_cfg = { 0 };
	int rc = GENAVB_SUCCESS;

	if (SWT_TxEPPQueryETTableEntry(&drv->handle, entry_id, &et_entry_cfg) != kStatus_Success)
		rc = -GENAVB_ERR_DSA_NOT_FOUND;

	return rc;
}

/* Delete an entry found in Egress Treatment Table based on entry ID */
static int netc_sw_et_delete_entry(uint32_t entry_id)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	struct netc_sw_drv *drv = net_bridge_drv(bridge);

	return netc_sw_delete_et_table_entry(drv, entry_id);
}

/* Add an entry in Egress Treatment Table based on the provided config.
 * 
 * Note: The entry ID of the Egress Treatment Table entry to be added should be
 * assigned by software as the member 'entryID' of struct netc_tb_et_config_t.
 */
static int netc_sw_et_add_entry(netc_tb_et_config_t *config)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	struct netc_sw_drv *drv = net_bridge_drv(bridge);
	int rc = GENAVB_SUCCESS;

	if (netc_sw_et_find_entry(config->entryID) < 0) {
		if (SWT_TxEPPAddETTableEntry(&drv->handle, config) != kStatus_Success) {
			os_log(LOG_ERR, "SWT_TxEPPAddETTableEntry() failed\n");
			rc = -GENAVB_ERR_DSA_HW_CONFIG;
		}
	}

	return rc;
}

/* Find an entry in Ingress Port Filter Table based on entry ID */
static int netc_sw_ipf_find_entry(uint32_t entry_id)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	struct netc_sw_drv *drv = net_bridge_drv(bridge);
	netc_tb_ipf_config_t ipf_entry_cfg = { 0 };
	int rc = GENAVB_SUCCESS;

	if (SWT_RxIPFQueryTableEntry(&drv->handle, entry_id, &ipf_entry_cfg) != kStatus_Success)
		rc = -GENAVB_ERR_DSA_NOT_FOUND;

	return rc;
}

/* Delete an entry found in Ingress Port Filter Table based on entry ID */
static int netc_sw_ipf_delete_entry(uint32_t entry_id)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	struct netc_sw_drv *drv = net_bridge_drv(bridge);
	int rc = GENAVB_SUCCESS;

	if (netc_sw_ipf_find_entry(entry_id) < 0)
		goto err;

	if (SWT_RxIPFDelTableEntry(&drv->handle, entry_id) != kStatus_Success) {
		os_log(LOG_ERR, "SWT_RxIPFDelTableEntry() failed\n");
		rc = -GENAVB_ERR_DSA_HW_CONFIG;
	}

err:
	return rc;
}

/* Add an entry in Ingress Port Filter Table based on the provided config */
static int netc_sw_ipf_add_entry(netc_tb_ipf_config_t *config, uint32_t *entry_id)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	struct netc_sw_drv *drv = net_bridge_drv(bridge);
	int rc = GENAVB_SUCCESS;

	if (netc_sw_ipf_find_entry(*entry_id) < 0) {
		if (SWT_RxIPFAddTableEntry(&drv->handle, config, entry_id) != kStatus_Success) {
			os_log(LOG_ERR, "SWT_RxIPFAddTableEntry() failed\n");
			rc = -GENAVB_ERR_DSA_HW_CONFIG;
		}
	}

	return rc;
}

static int netc_sw_dsa_set_ptp_filter(struct net_bridge *bridge)
{
	struct netc_sw_drv *drv = net_bridge_drv(bridge);
	static netc_tb_ipf_config_t ipfEntryCfg;
	uint8_t ptp_mac[6] = MC_ADDR_PTP;
	uint8_t ptp_mask[6] = MC_ADDR_PTP_MASK;

	memset(&ipfEntryCfg, 0U, sizeof(netc_tb_ipf_config_t));
	ipfEntryCfg.cfge.fltfa = kNETC_IPFRedirectToMgmtPort;
	ipfEntryCfg.cfge.hr = kNETC_SoftwareDefHR0;

	ipfEntryCfg.keye.precedence = NETC_DSA_IPF_PRECEDENCE_PTP;

	memcpy(ipfEntryCfg.keye.dmac, ptp_mac, 6);
	memcpy(ipfEntryCfg.keye.dmacMask, ptp_mask, 6);

	if (SWT_RxIPFAddTableEntry(&drv->handle, &ipfEntryCfg, &drv->ipf_entries_id[drv->num_ipf_entries]) != kStatus_Success)
		return -1;

	drv->num_ipf_entries++;

	return 0;
}

/* Find an entry in Ingress Stream Table based on entry ID */
static int netc_sw_is_find_entry(uint32_t entry_id)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	struct netc_sw_drv *drv = net_bridge_drv(bridge);
	netc_tb_is_config_t is_entry_cfg = { 0 };
	int rc = GENAVB_SUCCESS;

	if (SWT_RxPSFPQueryISTableEntry(&drv->handle, entry_id, &is_entry_cfg) != kStatus_Success)
		rc = -GENAVB_ERR_DSA_NOT_FOUND;

	return rc;
}

/* Delete an entry found in Ingress Stream Table based on entry ID */
static int netc_sw_is_delete_entry(uint32_t entry_id)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	struct netc_sw_drv *drv = net_bridge_drv(bridge);
	int rc = GENAVB_SUCCESS;

	if (netc_sw_is_find_entry(entry_id) < 0)
		goto err;

	if (SWT_RxPSFPDelISTableEntry(&drv->handle, entry_id) != kStatus_Success) {
		os_log(LOG_ERR, "SWT_RxPSFPDelISTableEntry() failed\n");
		rc = -GENAVB_ERR_DSA_HW_CONFIG;
	}

err:
	return rc;
}

/* Add an entry in Ingress Stream Table based on the provided config.
 * 
 * Note: The entry ID of the Ingress Stream Table entry to be added should be
 * assigned by software as the member 'entryID' of struct netc_tb_is_config_t.
 */
static int netc_sw_is_add_entry(netc_tb_is_config_t *config)
{
	struct net_bridge *bridge = bridge_get(DEFAULT_BRIDGE_ID);
	struct netc_sw_drv *drv = net_bridge_drv(bridge);
	int rc = GENAVB_SUCCESS;

	if (netc_sw_is_find_entry(config->entryID) < 0) {
		if (SWT_RxPSFPAddISTableEntry(&drv->handle, config) != kStatus_Success) {
			os_log(LOG_ERR, "SWT_RxIPFAddTableEntry() failed\n");
			rc = -GENAVB_ERR_DSA_HW_CONFIG;
		}
	}

	return rc;
}

/* Ingress packet flow:
 *
 * When using NETC switch port as CPU port:
 *     switch ingress port -> switch CPU port -> MPU
 *
 * When using ENETC0 port as CPU port:
 *     switch ingress port -> switch management port -> ENETC1 -> software virtual switch -> ENETC0 -> MPU
 */
static int netc_sw_dsa_ingress_bridge_forward_add(unsigned int bridge_cpu_port, unsigned int bridge_ingress_port)
{
	netc_tb_ipf_config_t ipf_entry_cfg = { 0 };
	netc_tb_is_config_t is_entry_cfg = { 0 };
	netc_tb_et_config_t et_entry_cfg = { 0 };
	/* Problem: An extra VLAN tag added by PVID of switch ingress port
	 * still exists on egress when forwarding action is set to 802.1Q
	 * bridge forwarding in Ingress Stream Table entry.
	 *
	 * Solution: When using NETC switch port as CPU port, have to replace
	 * the extra VLAN tag instead of the expected action to add VLAN tag.
	 * When using ENETC0 port as CPU port, have to delete the extra VLAN
	 * tag.
	 */
#if defined(CFG_DSA_CPU_PORT_NETC_SWITCH)
	netc_fm_vlan_ar_act_t vara = kNETC_ReplVidOnly;
	uint16_t dsa_tag;
#endif
#if defined(CFG_DSA_CPU_PORT_ENETC)
	netc_fm_sqt_act_t sqta = kNETC_NoSqtAction;
	netc_fm_vlan_ud_act_t vuda = kNETC_DelVlan;
#endif
	int rc;

	/* Add one Ingress Port Filter Table entry to filter on SRC_PORT (ingress
	 * switch port), with filter action set to ingress stream identification
	 * action.
	 */
	ipf_entry_cfg.keye.srcPort = bridge_ingress_port;
	ipf_entry_cfg.keye.srcPortMask = 0x1f;
	ipf_entry_cfg.keye.precedence = NETC_DSA_IPF_PRECEDENCE_DSA;
	ipf_entry_cfg.cfge.fltfa = kNETC_IPFForwardPermit;
	ipf_entry_cfg.cfge.flta = kNETC_IPFWithIngressStream;
	ipf_entry_cfg.cfge.fltaTgt = is_entry_id_ingress_bridge_forward[bridge_ingress_port];
	ipf_entry_cfg.cfge.rpr = 2;	/* RRR = 10b : enKC0 > enKC1 > IPF > defaultISEID */

	rc = netc_sw_ipf_add_entry(&ipf_entry_cfg, &ipf_entry_id_ingress_bridge_forward[bridge_ingress_port]);
	if (rc < 0)
		goto err;

	/* Add one Ingress Stream Table entry to use 802.1Q bridge forwarding and
	 * add egress packet processing action on CPU port.
	 */
	is_entry_cfg.entryID = is_entry_id_ingress_bridge_forward[bridge_ingress_port];
	is_entry_cfg.cfge.fa = kNETC_ISBridgeForward;
	is_entry_cfg.cfge.eport = bridge_cpu_port;
	is_entry_cfg.cfge.oETEID = kNETC_SinglePortETAccess;
	is_entry_cfg.cfge.isqEID = NULL_ENTRY_ID;
	is_entry_cfg.cfge.rpEID = NULL_ENTRY_ID;
	is_entry_cfg.cfge.sgiEID = NULL_ENTRY_ID;
	is_entry_cfg.cfge.ifmEID = NULL_ENTRY_ID;
	is_entry_cfg.cfge.etEID = et_entry_id_ingress[bridge_ingress_port];
	is_entry_cfg.cfge.iscEID = NULL_ENTRY_ID;

	rc = netc_sw_is_add_entry(&is_entry_cfg);
	if (rc < 0)
		goto err_is_add_entry;

	/* Add one Egress Treatment Table entry
	 *
	 * When using NETC switch port as CPU port, replace the extra VLAN tag
	 * with the outer VLAN tag used as DSA tag for packets egressing on CPU
	 * port.
	 *
	 * When using ENETC0 port as CPU port, delete the extra VLAN tag for
	 * packets egressing on switch management port.
	 */
	et_entry_cfg.entryID = et_entry_id_ingress[bridge_ingress_port];
#if defined(CFG_DSA_CPU_PORT_NETC_SWITCH)
	et_entry_cfg.cfge.efmLenChange = 4;
	dsa_tag = DSA_TAG_8021Q_RSV | bridge_ingress_port;
	et_entry_cfg.cfge.efmEID = NETC_FD_EID_ENCODE_OPTION_2(vara, dsa_tag); /* Replace VLAN tag */
#endif
#if defined(CFG_DSA_CPU_PORT_ENETC)
	et_entry_cfg.cfge.efmLenChange = -4;
	et_entry_cfg.cfge.efmEID = NETC_FD_EID_ENCODE_OPTION_1(sqta, vuda); /* Remove VLAN tag */
#endif
	et_entry_cfg.cfge.ecEID = NULL_ENTRY_ID;
	et_entry_cfg.cfge.esqaTgtEID = NULL_ENTRY_ID;

	rc = netc_sw_et_add_entry(&et_entry_cfg);
	if (rc < 0)
		goto err_et_add_entry;

	return GENAVB_SUCCESS;

err_et_add_entry:
	netc_sw_is_delete_entry(is_entry_id_ingress_bridge_forward[bridge_ingress_port]);

err_is_add_entry:
	netc_sw_ipf_delete_entry(ipf_entry_id_ingress_bridge_forward[bridge_ingress_port]);

err:
	return rc;
}

static int netc_sw_dsa_ingress_bridge_forward_del(unsigned int bridge_ingress_port)
{
	int rc;

	rc = netc_sw_ipf_delete_entry(ipf_entry_id_ingress_bridge_forward[bridge_ingress_port]);
	if (rc < 0)
		goto err;

	rc = netc_sw_is_delete_entry(is_entry_id_ingress_bridge_forward[bridge_ingress_port]);
	if (rc < 0)
		goto err;

	rc = netc_sw_et_delete_entry(et_entry_id_ingress[bridge_ingress_port]);
	if (rc < 0)
		goto err;

	return GENAVB_SUCCESS;

err:
	return rc;
}

/* Problem: for packets with destination MAC address in FDB, when using
 * 802.1Q bridge forwarding in Ingress Stream Table, the egress packet
 * processing action doesn't take effect.
 *
 * Solution: When the destination MAC address is the same as that of the
 * master Ethernet interface connected to the CPU port, use stream forward in
 * Ingress Stream Table to add outer VLAN tag on ingress switch port.
 */
static int netc_sw_dsa_ingress_stream_forward_add(unsigned int bridge_cpu_port, uint8_t *mac_addr, unsigned int bridge_ingress_port)
{
	netc_tb_ipf_config_t ipf_entry_cfg = { 0 };
	netc_tb_is_config_t is_entry_cfg = { 0 };
#if defined(CFG_DSA_CPU_PORT_NETC_SWITCH)
	netc_fm_vlan_ar_act_t vara = kNETC_AddCVlanPcpAndDei;
	uint16_t dsa_tag;
#endif
	int rc;

	if (!mac_addr) {
		rc = -GENAVB_ERR_INVALID_PARAMS;
		goto err;
	}

	/* Add one Ingress Port Filter Table entry to filter on both SRC_PORT
	 * (ingress switch port) and destination MAC address of master Ethernet
	 * interface, with filter action set to ingress stream identification
	 * action.
	 */
	ipf_entry_cfg.keye.srcPort = bridge_ingress_port;
	ipf_entry_cfg.keye.srcPortMask = 0x1f;
	ipf_entry_cfg.keye.precedence = NETC_DSA_IPF_PRECEDENCE_DSA;
	memcpy(ipf_entry_cfg.keye.dmac, mac_addr, 6);
	memset(ipf_entry_cfg.keye.dmacMask, 0xff, 6);
	ipf_entry_cfg.cfge.fltfa = kNETC_IPFForwardPermit;
	ipf_entry_cfg.cfge.flta = kNETC_IPFWithIngressStream;
	ipf_entry_cfg.cfge.fltaTgt = is_entry_id_ingress_stream_forward[bridge_ingress_port];
	ipf_entry_cfg.cfge.rpr = 2;	/* RRR = 10b : enKC0 > enKC1 > IPF > defaultISEID */

	rc = netc_sw_ipf_add_entry(&ipf_entry_cfg, &ipf_entry_id_ingress_stream_forward[bridge_ingress_port]);
	if (rc < 0)
		goto err;

	/* Add one Ingress Stream Table entry to use stream forwarding
	 *
	 * When using NETC switch port as CPU port, add ingress frame modification
	 * action to add outer VLAN tag, and then forward the packet to CPU port.
	 *
	 * When using ENETC0 port as CPU port, do not add any frame modification
	 * action, just use stream forwarding to bypass bridge forwarding (VLAN
	 * processing in particular) and forward the packet to switch management
	 * port. The outer VLAN tag is later added by software virtual switch.
	 */
	is_entry_cfg.entryID = is_entry_id_ingress_stream_forward[bridge_ingress_port];
	is_entry_cfg.cfge.fa = kNETC_ISStreamForward;
	is_entry_cfg.cfge.isqEID = NULL_ENTRY_ID;
	is_entry_cfg.cfge.rpEID = NULL_ENTRY_ID;
	is_entry_cfg.cfge.sgiEID = NULL_ENTRY_ID;
#if defined(CFG_DSA_CPU_PORT_NETC_SWITCH)
	dsa_tag = DSA_TAG_8021Q_RSV | bridge_ingress_port;
	is_entry_cfg.cfge.ifmEID = NETC_FD_EID_ENCODE_OPTION_2(vara, dsa_tag); /* Add outer VLAN tag */
#endif
#if defined(CFG_DSA_CPU_PORT_ENETC)
	is_entry_cfg.cfge.ifmEID = NULL_ENTRY_ID;
#endif
	is_entry_cfg.cfge.etEID = NULL_ENTRY_ID;
	is_entry_cfg.cfge.iscEID = NULL_ENTRY_ID;
	is_entry_cfg.cfge.ePortBitmap = (1 << bridge_cpu_port);

	rc = netc_sw_is_add_entry(&is_entry_cfg);
	if (rc < 0)
		goto err_is_add_entry;

	return GENAVB_SUCCESS;

err_is_add_entry:
	netc_sw_ipf_delete_entry(ipf_entry_id_ingress_stream_forward[bridge_ingress_port]);

err:
	return rc;
}

static int netc_sw_dsa_ingress_stream_forward_del(unsigned int bridge_ingress_port)
{
	int rc;

	rc = netc_sw_ipf_delete_entry(ipf_entry_id_ingress_stream_forward[bridge_ingress_port]);
	if (rc < 0)
		goto err;

	rc = netc_sw_is_delete_entry(is_entry_id_ingress_stream_forward[bridge_ingress_port]);
	if (rc < 0)
		goto err;

	return GENAVB_SUCCESS;

err:
	return rc;
}

/* Egress packet flow:
 *
 * When using NETC switch port as CPU port:
 *     MPU -> switch CPU port -> switch egress port
 *
 * When using ENETC0 port as CPU port: 
 *     MPU -> ENETC0 -> software virtual switch -> ENETC1 -> switch management port -> switch egress port
 */
static int netc_sw_dsa_egress_stream_forward_add(unsigned int bridge_cpu_port, unsigned int bridge_egress_port)
{
	netc_tb_ipf_config_t ipf_entry_cfg = { 0 };
	netc_tb_is_config_t is_entry_cfg = { 0 };
#if defined(CFG_DSA_CPU_PORT_ENETC)
	netc_tb_et_config_t et_entry_cfg = { 0 };
#endif
	netc_fm_sqt_act_t sqta = kNETC_NoSqtAction;
	netc_fm_vlan_ud_act_t vuda = kNETC_DelVlan;
	int rc;

	/* Add one Ingress Port Filter Table entry to filter on SRC_PORT (switch CPU
	 * port) and outer VLAN TCI, with filter action set to ingress stream
	 * identification action.
	 */
	ipf_entry_cfg.keye.srcPort = bridge_cpu_port;
	ipf_entry_cfg.keye.srcPortMask = 0x1f;
	ipf_entry_cfg.keye.frameAttr.outerVlan = 1;
	ipf_entry_cfg.keye.frameAttrMask = kNETC_IPFOuterVlanMask;
	ipf_entry_cfg.keye.outerVlanTCI = VLAN_LABEL(DSA_TAG_8021Q_RSV | bridge_egress_port, 0, 0);
	ipf_entry_cfg.keye.outerVlanTCIMask = VLAN_LABEL(VLAN_TCI_VID_MASK, 0, 0); /* The lower 4 bits in VID is used as port number */
	ipf_entry_cfg.keye.precedence = NETC_DSA_IPF_PRECEDENCE_DSA;
	ipf_entry_cfg.cfge.fltfa = kNETC_IPFForwardPermit;
	ipf_entry_cfg.cfge.flta = kNETC_IPFWithIngressStream;
	ipf_entry_cfg.cfge.fltaTgt = is_entry_id_egress_stream_forward[bridge_egress_port];
	ipf_entry_cfg.cfge.rpr = 2;	/* RRR = 10b : enKC0 > enKC1 > IPF > defaultISEID */

	rc = netc_sw_ipf_add_entry(&ipf_entry_cfg, &ipf_entry_id_egress_stream_forward[bridge_egress_port]);
	if (rc < 0)
		goto err;

	/* Add one Ingress Stream Table entry to use stream forwarding
	 *
	 * When using NETC switch port as CPU port, add ingress frame modification
	 * action to remove outer VLAN tag, and then forward the packet to egress
	 * switch port.
	 *
	 * When using ENETC0 port as CPU port, add egress packet processing action
	 * on egress switch port to remove outer VLAN tag after forwarding the
	 * packet to egress switch port.
	 */
	is_entry_cfg.entryID = is_entry_id_egress_stream_forward[bridge_egress_port];
	is_entry_cfg.cfge.fa = kNETC_ISStreamForward;
#if defined(CFG_DSA_CPU_PORT_ENETC)
	is_entry_cfg.cfge.eport = bridge_egress_port;
	is_entry_cfg.cfge.oETEID = kNETC_SinglePortETAccess;
#endif
	is_entry_cfg.cfge.isqEID = NULL_ENTRY_ID;
	is_entry_cfg.cfge.rpEID = NULL_ENTRY_ID;
	is_entry_cfg.cfge.sgiEID = NULL_ENTRY_ID;
#if defined(CFG_DSA_CPU_PORT_NETC_SWITCH)
	is_entry_cfg.cfge.ifmEID = NETC_FD_EID_ENCODE_OPTION_1(sqta, vuda); /* Remove outer VLAN tag */
	is_entry_cfg.cfge.etEID = NULL_ENTRY_ID;
#endif
#if defined(CFG_DSA_CPU_PORT_ENETC)
	is_entry_cfg.cfge.ifmEID = NULL_ENTRY_ID;
	is_entry_cfg.cfge.etEID = et_entry_id_egress[bridge_egress_port];
#endif
	is_entry_cfg.cfge.iscEID = NULL_ENTRY_ID;
	is_entry_cfg.cfge.ePortBitmap = (1 << bridge_egress_port);

	rc = netc_sw_is_add_entry(&is_entry_cfg);
	if (rc < 0)
		goto err_is_add_entry;

#if defined(CFG_DSA_CPU_PORT_ENETC)
	/* When using ENETC0 port as CPU port, add one Egress Treatment Table entry
	 * to delete outer VLAN tag for packets leaving from egress switch port.
	 */
	et_entry_cfg.entryID = et_entry_id_egress[bridge_egress_port];
	et_entry_cfg.cfge.efmLenChange = -4;
	et_entry_cfg.cfge.efmEID = NETC_FD_EID_ENCODE_OPTION_1(sqta, vuda); /* Remove outer VLAN tag */
	et_entry_cfg.cfge.ecEID = NULL_ENTRY_ID;
	et_entry_cfg.cfge.ecEID = NULL_ENTRY_ID;

	rc = netc_sw_et_add_entry(&et_entry_cfg);
	if (rc < 0)
		goto err_et_add_entry;
#endif

	return GENAVB_SUCCESS;

#if defined(CFG_DSA_CPU_PORT_ENETC)
err_et_add_entry:
	netc_sw_is_delete_entry(is_entry_id_egress_stream_forward[bridge_egress_port]);
#endif

err_is_add_entry:
	netc_sw_ipf_delete_entry(ipf_entry_id_egress_stream_forward[bridge_egress_port]);

err:
	return rc;
}

static int netc_sw_dsa_egress_stream_forward_del(unsigned int bridge_egress_port)
{
	int rc;

	rc = netc_sw_ipf_delete_entry(ipf_entry_id_egress_stream_forward[bridge_egress_port]);
	if (rc < 0)
		goto err;

	rc = netc_sw_is_delete_entry(is_entry_id_egress_stream_forward[bridge_egress_port]);
	if (rc < 0)
		goto err;

#if defined(CFG_DSA_CPU_PORT_ENETC)
	rc = netc_sw_et_delete_entry(et_entry_id_egress[bridge_egress_port]);
	if (rc < 0)
		goto err;
#endif

	return GENAVB_SUCCESS;

err:
	return rc;
}

static int netc_sw_dsa_add(struct net_bridge *bridge, unsigned int cpu_port, uint8_t *mac_addr, unsigned int slave_port)
{
	struct netc_sw_drv *drv = net_bridge_drv(bridge);
	uint8_t bridge_cpu_port = logical_port_to_bridge_port(cpu_port);
	uint8_t bridge_slave_port = logical_port_to_bridge_port(slave_port);
	int rc;

	rc = netc_sw_dsa_ingress_bridge_forward_add(bridge_cpu_port, bridge_slave_port);
	if (rc < 0)
		goto err;

	rc = netc_sw_dsa_ingress_stream_forward_add(bridge_cpu_port, mac_addr, bridge_slave_port);
	if (rc < 0)
		goto err;

	rc = netc_sw_dsa_egress_stream_forward_add(bridge_cpu_port, bridge_slave_port);
	if (rc < 0)
		goto err;

	drv->dsa_cpu_port = bridge_cpu_port;

	return GENAVB_SUCCESS;

err:
	return rc;
}

static int netc_sw_dsa_del(struct net_bridge *bridge, unsigned int slave_port)
{
	uint8_t bridge_slave_port = logical_port_to_bridge_port(slave_port);
	int rc;

	rc = netc_sw_dsa_ingress_bridge_forward_del(bridge_slave_port);
	if (rc < 0)
		goto err;

	rc = netc_sw_dsa_ingress_stream_forward_del(bridge_slave_port);
	if (rc < 0)
		goto err;

	rc = netc_sw_dsa_egress_stream_forward_del(bridge_slave_port);
	if (rc < 0)
		goto err;

	return GENAVB_SUCCESS;

err:
	return rc;
}

static void netc_sw_dsa_efm_init(struct net_bridge *bridge)
{
	struct netc_sw_drv *drv = net_bridge_drv(bridge);
	netc_tb_fm_config_t fm_conf = {0};

	fm_conf.cfge.vlanHdrAct = kNETC_ReplOuterVlan;
	fm_conf.cfge.outerVidAct = kNETC_UseSpecVid;
	fm_conf.cfge.sqtAct = kNETC_ReomveRTag;
	fm_conf.cfge.outerTpidAct = kNETC_UseFrameTpid;
	fm_conf.cfge.outerPcpAct = kNETC_UseFramePcp;
	fm_conf.cfge.outerDeiAct = kNETC_UseFrameDei;

	for (int port = 0; port < CFG_NUM_NETC_SW_PORTS; port++) {
		fm_conf.entryID = DSA_NETC_EFM_DEL_SQ_TAG_CHANGE_VLAN_BASE + port;
		fm_conf.cfge.outerVlanID = DSA_TAG_8021Q_RSV | port;
		SWT_FMAddTableEntry(&drv->handle, &fm_conf);
	}
}

void netc_sw_dsa_init(struct net_bridge *bridge)
{
	bridge->drv_ops.dsa_add = netc_sw_dsa_add;
	bridge->drv_ops.dsa_delete = netc_sw_dsa_del;
	netc_sw_dsa_set_ptp_filter(bridge);
	netc_sw_dsa_efm_init(bridge);
}

#else
void netc_sw_dsa_init(struct net_bridge *bridge) { }
#endif
