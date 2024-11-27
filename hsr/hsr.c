/*
* Copyright 2023-2024 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief NXP HSR stack code
 @details Setup NXP HSR stack component. Implements mac learn and supervision
 process, adds timers to check nodes and send supervision frames.
 */

#include <string.h>
#include <stdlib.h>

#include "common/timer.h"
#include "common/list.h"
#include "common/ipc.h"
#include "common/log.h"
#include "genavb/hsr.h"

#include "os/stream_identification.h"
#include "os/sys_types.h"
#include "os/config.h"
#include "os/stdlib.h"
#include "os/string.h"
#include "os/timer.h"
#include "os/vlan.h"
#include "os/frer.h"
#include "os/psfp.h"
#include "os/fdb.h"
#include "os/log.h"
#include "os/net.h"

#define HSR_TABLES_ARRAY_CNT	48
#define HSR_NODE_MAX_STREAM	8

#define HSR_MAX_TIMERS		2
#define HSR_TIMER_FORGET_PERIOD	60000
#define HSR_TIMER_PERIOD	2000

#define HSR_SWITCH_PORT_MAX	5
#define HSR_SWITCH_PORT_MASK	0x1f

#define HSR_NULL_ENTRY_ID	(0xFFFFFFFF)
#define HSR_SI_MAX_ENTRIES	384

#define STREAM_VID(streamid)	(NETC_INTERNAL_VID_BASE + streamid)

struct hsr_table {
	uint8_t stream_table[HSR_TABLES_ARRAY_CNT];
	uint8_t rec_table[HSR_TABLES_ARRAY_CNT];
	uint8_t gen_table[HSR_TABLES_ARRAY_CNT];
	uint8_t drop_table[HSR_TABLES_ARRAY_CNT];
};

struct hsr_stream_drop {
	uint32_t stream_id;
	uint32_t drop_filter_id;
};

struct hsr_stream {
	uint32_t stream_id;
	uint16_t vid;
	bool tagged;
	struct hsr_stream_drop drop;
};

struct hsr_node {
	struct list_head	list;
	uint8_t			mac[6];
	uint8_t			fwdmask;
	uint8_t			stream_max;
	struct hsr_stream	stream[HSR_NODE_MAX_STREAM];
	uint32_t		moden_stream[HSR_SWITCH_PORT_MAX];
	uint32_t		gen_id;
	uint32_t		rec_id;
	bool			isring;
	uint32_t		cnt;
	uint16_t		seqnum;
};

struct hsr_port_config {
	uint8_t			cpu_port;
	uint8_t			br_mgmt_port;
	uint8_t			br_port_max;
	uint8_t			logical_br_port_list[HSR_SWITCH_PORT_MAX];
	uint32_t		ring_port_mask;
};

struct hsr_ctx {
	struct hsr_port_config	port_cfg;
	genavb_hsr_mode_t	mode;
	struct net_rx		net_rx;
	struct net_tx		net_tx;
	struct ipc_rx		ipc_rx_stack;
	struct timer_ctx	*timer_ctx;
	struct timer		lifecheck_timer;
	struct timer		nodeforget_timer;
	struct list_head	ring_list;
	struct list_head	proxy_list;
	struct hsr_table	tables;
	uint16_t		SupSequenceNumber;
};

static void hsr_node_moden_set(struct hsr_ctx *hsr, struct hsr_node *node);
static void hsr_node_moden_clear(struct hsr_ctx *hsr, struct hsr_node *node);

static int hsr_logic_port_to_bridge_port(struct hsr_port_config *port_cfg, uint32_t logical_port, uint32_t *br_port)
{
	int i;

	for (i = 0; i < port_cfg->br_port_max; i++)
		if (port_cfg->logical_br_port_list[i] == logical_port) {
			*br_port = i;

			return 0;
		}

	return -1;
}

static int hsr_table_id_alloc(uint8_t *table)
{
	int i, j;

	for (i = 0; i < HSR_TABLES_ARRAY_CNT; i++)
		if (table[i] != 0xFF)
			break;

	if (i == HSR_TABLES_ARRAY_CNT)
		return -1;

	for (j = 0; j < 8; j++)
		if (!(table[i] & (1 << j))) {
			table[i] |= (1 << j);
			return (i * 8 + j);
		}

	return -1;
}

static void hsr_table_id_free(uint8_t *table, uint32_t id)
{
	uint32_t i, j;

	if (table) {
		i = id / 8;
		j = id % 8;
		table[i] &= ((~(1 << j)) & 0xFF);
	}
}

static int hsr_node_list_add(struct list_head *head, uint8_t *mac, uint16_t vid,
			     bool tagged, uint8_t fwdmask, uint32_t streamid,
			     uint32_t gen_id, uint32_t rec_id, bool isring)
{
	struct hsr_node *entry;

	entry = os_malloc(sizeof(struct hsr_node));
	if (!entry)
		return -1;

	memset(entry, 0, sizeof(struct hsr_node));
	memcpy(entry->mac, mac, 6);
	entry->gen_id = gen_id;
	entry->rec_id = rec_id;
	entry->fwdmask = fwdmask;

	entry->stream_max = 1;
	entry->stream[0].stream_id = streamid;
	entry->stream[0].vid = vid;
	entry->stream[0].tagged = tagged;
	entry->isring = isring;

	list_add(head, &entry->list);

	return 0;
}

static void hsr_node_list_del(struct hsr_node *node)
{
	list_del(&node->list);

	os_free(node);
}

static struct hsr_node *hsr_node_list_find(struct list_head *head, uint8_t *mac)
{
	struct list_head *pos, *next;
	struct hsr_node *entry;

	for (pos = list_first(head); next = list_next(pos), pos != head; pos = next) {
		entry = container_of(pos, struct hsr_node, list);
		if (!memcmp(entry->mac, mac, 6))
			return entry;
	}

	return NULL;
}

static int hsr_node_streamid_add(struct hsr_node *node, uint32_t streamid, uint16_t vid, bool tagged)
{
	if (node->stream_max >= HSR_NODE_MAX_STREAM)
		return -1;

	node->stream[node->stream_max].stream_id = streamid;
	node->stream[node->stream_max].vid = vid;
	node->stream[node->stream_max].tagged = tagged;
	node->stream_max++;

	return 0;
}

static int hsr_node_streamid_get_index(struct hsr_node *node, uint16_t vid)
{
	int i;

	for (i = 0; i < node->stream_max; i++) {
		if (node->stream[i].vid == vid)
			return i;
	}

	return -1;
}

static void hsr_node_streamid_del(struct hsr_node *node, int index)
{
	int i;

	for (i = index; i < node->stream_max - 1; i++)
		memcpy(&node->stream[i], &node->stream[i + 1], sizeof(struct hsr_stream));

	node->stream_max--;
}

static bool hsr_fdb_entry_exist(struct hsr_port_config *port_cfg, uint8_t *mac, uint16_t vid, uint8_t *fwdmask, bool *active)
{
	struct genavb_fdb_port_map map[HSR_SWITCH_PORT_MAX];
	bool dynamic, exist = false;
	genavb_fdb_status_t status;
	uint8_t mask = 0;
	int ret, i;

	ret = fdb_read(mac, vid, &dynamic, map, &status);
	if (ret)
		return exist;

	for (i = 0; i < port_cfg->br_port_max; i++)
		if (map[i].control == GENAVB_FDB_PORT_CONTROL_FORWARDING)
			mask |= (1 << i);

	if (fwdmask)
		*fwdmask = mask;
	if (active)
		*active = (status == GENAVB_FDB_STATUS_INVALID ? 0 : 1);

	exist = true;

	return exist;
}

static int hsr_fdb_entry_add(struct hsr_port_config *port_cfg, uint8_t *mac, uint16_t vid, uint8_t fwdmask, bool dynamic)
{
	genavb_fdb_port_control_t control;
	unsigned int port_id;
	struct hsr_ctx *hsr;
	int i;

	hsr = container_of(port_cfg, struct hsr_ctx, port_cfg);
	if (hsr->mode == GENAVB_HSR_OPERATION_MODE_U)
		fwdmask = HSR_SWITCH_PORT_MASK;

	for (i = 0; i < port_cfg->br_port_max; i++) {
		port_id = port_cfg->logical_br_port_list[i];
		if (fwdmask & (1 << i))
			control = GENAVB_FDB_PORT_CONTROL_FORWARDING;
		else
			control = GENAVB_FDB_PORT_CONTROL_FILTERING;

		if (fdb_update(port_id, mac, vid, dynamic, control) < 0)
			return -1;
	}

	return 0;
}

static void hsr_fdb_del(uint8_t *mac, uint16_t vid, bool dynamic)
{
	fdb_delete(mac, vid, dynamic);
}

static int hsr_fdb_fwd_update(struct hsr_port_config *port_cfg, uint8_t *mac, uint16_t vid, uint8_t fwdmask)
{
	struct genavb_fdb_port_map map[HSR_SWITCH_PORT_MAX];
	genavb_fdb_port_control_t control;
	genavb_fdb_status_t status;
	bool dynamic;
	int ret, i;

	ret = fdb_read(mac, vid, &dynamic, map, &status);
	if (ret)
		return ret;

	for (i = 0; i < port_cfg->br_port_max; i++) {
		if (map[i].control != GENAVB_FDB_PORT_CONTROL_FORWARDING &&
		    (fwdmask & (1 << i))) {
			control = GENAVB_FDB_PORT_CONTROL_FORWARDING;
			if (fdb_update(map[i].port_id, mac, vid, dynamic, control) < 0)
				return -1;
		} else if (map[i].control != GENAVB_FDB_PORT_CONTROL_FILTERING &&
			   !(fwdmask & (1 << i))) {
			control = GENAVB_FDB_PORT_CONTROL_FILTERING;
			if (fdb_update(map[i].port_id, mac, vid, dynamic, control) < 0)
				return -1;
		}
	}

	return 0;
}

static void hsr_vlan_update(uint16_t vid, bool dynamic, struct genavb_vlan_port_map *map, uint8_t port_n)
{
	int i;

	for (i = 0; i < port_n; i++)
		vlan_update(vid, dynamic, &map[i]);
}

static int hsr_stream_identify_add(struct hsr_ctx *hsr, uint8_t *mac, uint16_t vid, bool tagged, uint32_t streamid, uint8_t fwdmask)
{
	struct genavb_stream_identity stream;
	struct genavb_si_smac_vlan *smac;
	struct genavb_si_port port[HSR_SWITCH_PORT_MAX];
	int i, n = 0;

	for (i = 0; i < hsr->port_cfg.br_port_max; i++) {
		if (!(fwdmask & (1 << i)))
			continue;

		port[n].id = hsr->port_cfg.logical_br_port_list[i];
		port[n].pos = GENAVB_SI_PORT_POS_IN_FACING_OUTPUT;
		n++;
	}

	stream.handle = streamid;
	stream.port = port;
	stream.port_n = n;

	stream.type = GENAVB_SI_SRC_MAC_VLAN;
	smac = &stream.parameters.smac_vlan;
	memcpy(smac->source_mac, mac, 6);

	smac->vlan = vid;
	if (tagged)
		smac->tagged = GENAVB_SI_TAGGED;
	else
		smac->tagged = GENAVB_SI_PRIORITY;

	if (stream_identity_update(streamid, &stream) < 0)
		return -1;

	return 0;
}

static int hsr_frer_identification_add(struct hsr_port_config *port_cfg, uint32_t streamid, uint8_t ring_port_mask)
{
	struct genavb_sequence_identification entry = {0};
	uint32_t stream[HSR_SI_MAX_ENTRIES];
	struct hsr_ctx *hsr;
	uint8_t port;
	int i;

	hsr = container_of(port_cfg, struct hsr_ctx, port_cfg);

	entry.stream = stream;

	for (i = 0; i < port_cfg->br_port_max; i++) {
		port = port_cfg->logical_br_port_list[i];
		entry.stream_n = HSR_SI_MAX_ENTRIES;
		if (sequence_identification_read(port, true, &entry) < 0)
			entry.stream_n = 0;
		else
			sequence_identification_delete(port, true);

		stream[entry.stream_n++] = streamid;
		entry.encapsulation = GENAVB_SEQI_HSR_SEQ_TAG;

		if (ring_port_mask & (1 << i))
			entry.active = 1;
		else
			entry.active = 0;

		if (hsr->mode == GENAVB_HSR_OPERATION_MODE_T)
			entry.active = 0;

		if (sequence_identification_update(port, true, &entry) < 0)
			return -1;
	}

	return 0;
}

static void hsr_frer_identification_del(struct hsr_port_config *port_cfg, uint32_t streamid)
{
	struct genavb_sequence_identification entry = {0};
	uint32_t stream[HSR_SI_MAX_ENTRIES];
	bool find = false;
	uint8_t port;
	int i, j;

	entry.stream = stream;

	for (i = 0; i < port_cfg->br_port_max; i++) {
		port = port_cfg->logical_br_port_list[i];
		entry.stream_n = HSR_SI_MAX_ENTRIES;
		if (sequence_identification_read(port, true, &entry) < 0)
			return;

		find = false;
		for (j = 0; j < entry.stream_n; j++) {
			if (stream[j] == streamid)
				find = true;

			if (find) {
				if ((j + 1) < entry.stream_n)
					stream[j] = stream[j + 1];
			}
		}

		if (find) {
			entry.stream_n--;
			sequence_identification_delete(port, true);
			sequence_identification_update(port, true, &entry);
		}
	}
}

static int hsr_frer_gen_add(uint32_t index, uint32_t streamid)
{
	struct genavb_sequence_generation entry = {0};
	uint32_t stream[HSR_NODE_MAX_STREAM];

	entry.stream = stream;
	entry.stream_n = HSR_NODE_MAX_STREAM;

	if (sequence_generation_read(index, &entry) < 0)
		entry.stream_n = 0;
	else
		sequence_generation_delete(index);

	stream[entry.stream_n++] = streamid;
	entry.direction_out_facing = 0;

	if (sequence_generation_update(index, &entry, SEQG_UPDATE_OPTION_CONFIG) < 0)
		return -1;

	return 0;
}

static int hsr_frer_gen_seq_get(uint32_t index, uint16_t *seqnum)
{
	struct genavb_sequence_generation entry = {0};
	uint32_t stream[HSR_NODE_MAX_STREAM];

	entry.stream = stream;
	entry.stream_n = HSR_NODE_MAX_STREAM;

	if (sequence_generation_read(index, &entry) < 0)
		return -1;

	*seqnum = entry.seqnum;

	return 0;
}

static int hsr_frer_rec_add(struct hsr_port_config *port_cfg, uint32_t base_rec_id, uint32_t streamid)
{
	struct genavb_sequence_recovery entry = {0};
	uint32_t stream[HSR_NODE_MAX_STREAM];
	unsigned int port[HSR_SWITCH_PORT_MAX];
	int i;

	entry.stream = stream;
	entry.port = port;

	for (i = 0; i < port_cfg->br_port_max; i++) {
		entry.stream_n = HSR_NODE_MAX_STREAM;
		if (sequence_recovery_read(base_rec_id + i, &entry) < 0)
			entry.stream_n = 0;
		else
			sequence_recovery_delete(base_rec_id + i);

		stream[entry.stream_n++] = streamid;
		entry.direction_out_facing = 1;
		entry.algorithm = GENAVB_SEQR_VECTOR;
		entry.history_length = 0x7f;
		entry.reset_timeout = 1;
		entry.port_n = 1;
		port[0] = port_cfg->logical_br_port_list[i];

		if (sequence_recovery_update(base_rec_id + i, &entry) < 0)
			return -1;
	}

	return 0;
}

static int hsr_mac_filter_drop(struct hsr_ctx *hsr, uint8_t *mac, uint16_t vid, bool tagged, uint8_t portmask)
{
	struct genavb_stream_filter_instance instance = {0};
	uint32_t streamid, drop_filter_id;
	struct hsr_node *entry;
	int index, rc;

	entry = hsr_node_list_find(&hsr->proxy_list, mac);
	if (!entry)
		return -1;

	rc = hsr_table_id_alloc(hsr->tables.stream_table);
	if (rc < 0)
		return -1;

	streamid = rc;

	if (hsr_stream_identify_add(hsr, mac, vid, tagged, streamid, portmask) < 0)
		goto stream_identify_err;

	rc = hsr_table_id_alloc(hsr->tables.drop_table);
	if (rc < 0)
		goto drop_alloc;

	drop_filter_id = rc;

	instance.stream_filter_instance_id = drop_filter_id;
	instance.stream_handle = streamid;
	instance.max_sdu_size = 1;
	instance.stream_gate_ref = HSR_NULL_ENTRY_ID;
	instance.priority_spec = GENAVB_PRIORITY_SPEC_WILDCARD;

	if (stream_filter_update(drop_filter_id, &instance) < 0)
		goto stream_filter_err;

	index = hsr_node_streamid_get_index(entry, vid);
	if (index < 0)
		goto streamid_get_err;

	entry->stream[index].drop.stream_id = streamid;
	entry->stream[index].drop.drop_filter_id = drop_filter_id;

	return 0;

streamid_get_err:
	stream_filter_delete(drop_filter_id);

stream_filter_err:
	hsr_table_id_free(hsr->tables.drop_table, drop_filter_id);

drop_alloc:
	stream_identity_delete(streamid);

stream_identify_err:
	hsr_table_id_free(hsr->tables.stream_table, streamid);

	return -1;
}

static void hsr_drop_filter_del(struct hsr_table *tables, struct hsr_stream_drop *drop)
{
	stream_filter_delete(drop->drop_filter_id);
	hsr_table_id_free(tables->drop_table, drop->drop_filter_id);

	stream_identity_delete(drop->stream_id);
	hsr_table_id_free(tables->stream_table, drop->stream_id);
}

static int hsr_node_stream_del(struct hsr_ctx *hsr, struct hsr_node *node, uint16_t vid)
{
	uint32_t streamid;
	int index;

	index = hsr_node_streamid_get_index(node, vid);
	if (index < 0)
		return -1;

	streamid = node->stream[index].stream_id;
	hsr_table_id_free(hsr->tables.stream_table, streamid);
	stream_identity_delete(streamid);

	hsr_frer_identification_del(&hsr->port_cfg, streamid);

	hsr_drop_filter_del(&hsr->tables, &node->stream[index].drop);

	hsr_node_streamid_del(node, index);

	return 0;
}

static void hsr_node_del(struct hsr_ctx *hsr, struct hsr_node *node)
{
	uint32_t streamid;
	int i;

	if (hsr->mode == GENAVB_HSR_OPERATION_MODE_N)
		hsr_node_moden_clear(hsr, node);

	for (i = 0; i < node->stream_max; i++) {
		streamid = node->stream[i].stream_id;
		stream_identity_delete(streamid);
		hsr_table_id_free(hsr->tables.stream_table, streamid);
		hsr_fdb_del(node->mac, node->stream[i].vid, 0);
		hsr_frer_identification_del(&hsr->port_cfg, streamid);
	}

	if (node->gen_id != HSR_NULL_ENTRY_ID) {
		sequence_generation_delete(node->gen_id);
		hsr_table_id_free(hsr->tables.gen_table, node->gen_id);

		for (i = 0; i < node->stream_max; i++)
			hsr_drop_filter_del(&hsr->tables, &node->stream[i].drop);
	}

	if (node->rec_id != HSR_NULL_ENTRY_ID) {
		sequence_recovery_delete(node->rec_id);
		hsr_table_id_free(hsr->tables.rec_table, node->rec_id / hsr->port_cfg.br_port_max);
	}

	hsr_node_list_del(node);
}

static void hsr_node_list_free(struct hsr_ctx *hsr, struct list_head *head)
{
	struct list_head *pos, *next;
	struct hsr_node *entry;

	for (pos = list_first(head); next = list_next(pos), pos != head; pos = next) {
		entry = container_of(pos, struct hsr_node, list);
		hsr_node_del(hsr, entry);
	}
}

static void hsr_maclearn_process(struct hsr_ctx *hsr, struct net_rx_desc *desc)
{
	uint8_t ring_port_mask, fwdmask, old_fwdmask;
	bool is_ring_port = 0, dynamic = 0, isring;
	uint32_t streamid, gen_id, rec_id;
	struct hsr_node *node;
	uint32_t port;
	uint16_t vid;
	bool tagged;
	uint8_t *mac;
	int rc;

	if (hsr_logic_port_to_bridge_port(&hsr->port_cfg, desc->port, &port) < 0)
		return;

	mac = (uint8_t *)desc + desc->l2_offset + 6;
	if (desc->vid == VLAN_VID_NONE) {
		if (vlan_port_get_default(desc->port, &vid) < 0)
			return;
		tagged = false;
	} else {
		vid = desc->vid;
		tagged = true;
	}

	ring_port_mask = hsr->port_cfg.ring_port_mask;

	if ((1 << port) & ring_port_mask)
		is_ring_port = 1;

	if (!is_ring_port)
		fwdmask = 1 << port;
	else
		fwdmask = ring_port_mask;

	if (hsr_fdb_entry_exist(&hsr->port_cfg, mac, vid, &old_fwdmask, NULL)) {
		node = hsr_node_list_find(&hsr->proxy_list, mac);
		if (old_fwdmask != fwdmask && node && !is_ring_port) {
			hsr_node_stream_del(hsr, node, vid);
			if (node->stream_max == 0)
				hsr_node_del(hsr, node);
		} else if (node || old_fwdmask == fwdmask) {
			return;
		}
	}

	if (hsr_fdb_entry_add(&hsr->port_cfg, mac, vid, fwdmask, dynamic) < 0)
		return;

	node = hsr_node_list_find(&hsr->proxy_list, mac);
	if (node && node->fwdmask == fwdmask)
		return;

	rc = hsr_table_id_alloc(hsr->tables.stream_table);
	if (rc < 0)
		return;

	streamid = rc;

	if (hsr_stream_identify_add(hsr, mac, vid, tagged, streamid, fwdmask) < 0)
		goto streamid_add_err;

	if (hsr_frer_identification_add(&hsr->port_cfg, streamid, ring_port_mask) < 0)
		goto frer_identify_err;

	if (is_ring_port) {
		gen_id = HSR_NULL_ENTRY_ID;
		node = hsr_node_list_find(&hsr->ring_list, mac);
		if (node) {
			hsr_node_streamid_add(node, streamid, vid, tagged);
			hsr_frer_rec_add(&hsr->port_cfg, node->rec_id, streamid);
			return;
		}

		rc = hsr_table_id_alloc(hsr->tables.rec_table) * hsr->port_cfg.br_port_max;
		if (rc < 0)
			goto frer_alloc_err;

		rec_id = rc;

		if (hsr_frer_rec_add(&hsr->port_cfg, rec_id, streamid) < 0)
			goto frer_add_err;

		rc = hsr_node_list_add(&hsr->ring_list, mac, vid, tagged, fwdmask, streamid, gen_id, rec_id, 0);
		if (rc < 0)
			goto list_add_err;

		if (hsr->mode == GENAVB_HSR_OPERATION_MODE_N) {
			node = hsr_node_list_find(&hsr->ring_list, mac);
			if (node)
				hsr_node_moden_set(hsr, node);
		}
	} else {
		rec_id = HSR_NULL_ENTRY_ID;
		node = hsr_node_list_find(&hsr->proxy_list, mac);
		if (node) {
			hsr_node_streamid_add(node, streamid, vid, tagged);
			hsr_frer_gen_add(node->gen_id, streamid);
			hsr_mac_filter_drop(hsr, mac, vid, tagged, ring_port_mask);
			return;
		}

		node = hsr_node_list_find(&hsr->ring_list, mac);
		if (node)
			hsr_node_del(hsr, node);

		rc = hsr_table_id_alloc(hsr->tables.gen_table);
		if (rc < 0)
			goto frer_alloc_err;

		gen_id = rc;

		if (hsr_frer_gen_add(gen_id, streamid) < 0)
			goto frer_add_err;

		isring = (port == hsr->port_cfg.br_mgmt_port);
		rc = hsr_node_list_add(&hsr->proxy_list, mac, vid, tagged, fwdmask, streamid, gen_id, rec_id, isring);
		if (rc < 0)
			goto list_add_err;

		hsr_mac_filter_drop(hsr, mac, vid, tagged, ring_port_mask);
	}

	return;

list_add_err:
	if (gen_id != HSR_NULL_ENTRY_ID)
		sequence_generation_delete(gen_id);

	if (rec_id != HSR_NULL_ENTRY_ID)
		sequence_recovery_delete(rec_id);

frer_add_err:
	if (gen_id != HSR_NULL_ENTRY_ID)
		hsr_table_id_free(hsr->tables.gen_table, gen_id);

	if (rec_id != HSR_NULL_ENTRY_ID)
		hsr_table_id_free(hsr->tables.rec_table, rec_id / hsr->port_cfg.br_port_max);

frer_alloc_err:
	hsr_frer_identification_del(&hsr->port_cfg, streamid);

frer_identify_err:
	stream_identity_delete(streamid);

streamid_add_err:
	hsr_table_id_free(hsr->tables.stream_table, streamid);
	hsr_fdb_del(mac, vid, 0);
}

static void hsr_supervision_process(struct hsr_ctx *hsr, struct net_rx_desc *desc)
{
	struct hsr_node *node;
	uint8_t *smac, *mac;
	bool isring = 0;

	smac = (uint8_t *)desc + desc->l2_offset + 6;
	mac = (uint8_t *)desc + desc->l3_offset + 6;

	if (!(memcmp(smac, mac, 6)))
		isring = 1;

	node = hsr_node_list_find(&hsr->ring_list, mac);
	if (node) {
		node->cnt++;
		if (node->cnt == 0)
			node->cnt = 1;

		node->isring = isring;
	}
}

static void hsr_rx_process(struct net_rx *rx, struct net_rx_desc *desc)
{
	struct hsr_ctx *hsr;

	hsr = container_of(rx, struct hsr_ctx, net_rx);

	if (desc->ethertype == ETHERTYPE_HSR_SUPERVISION)
		hsr_supervision_process(hsr, desc);
	else
		hsr_maclearn_process(hsr, desc);

	net_rx_free(desc);
}

static void hsr_supervision_frame(uint8_t *addr, uint32_t len, uint8_t *smac, uint8_t *mac, uint32_t seqnum)
{
	uint8_t dmac[6] = HSR_SUPERVISION_BASE;
	uint16_t ethertype = htons(ETHERTYPE_HSR_SUPERVISION);
	uint8_t suppath = 0;
	uint8_t supversion = 1;
	uint8_t tlv1_type = 23;
	uint8_t tlv1_length = 6;
	uint8_t tlv2_type = 30;
	uint8_t tlv2_length = 6;
	uint32_t offset;

	memcpy(addr, dmac, 6);
	offset = 6;
	memcpy(addr + offset, mac, 6);
	offset += 6;
	memcpy(addr + offset, &ethertype, 2);
	offset += 2;
	*(addr + offset) = suppath;
	offset++;
	*(addr + offset) = supversion;
	offset++;
	*(uint16_t *)(addr + offset) = seqnum;
	offset += 2;
	*(addr + offset) = tlv1_type;
	offset++;
	*(addr + offset) = tlv1_length;
	offset++;
	memcpy(addr + offset, smac, 6);
	offset += 6;

	*(addr + offset) = tlv2_type;
	offset++;
	*(addr + offset) = tlv2_length;
	offset++;
	memcpy(addr + offset, mac, 6);
	offset += 6;
	memset(addr + offset, 0, len - offset);
}

static void hsr_supervision_send(struct net_tx *tx, struct list_head *head, uint32_t seqnum)
{
	struct list_head *pos, *next;
	struct net_tx_desc *desc;
	uint32_t data_len = 70;
	struct hsr_node *entry;
	uint8_t local_mac[6] = {0};

	for (pos = list_first(head); next = list_next(pos), pos != head; pos = next) {
		entry = container_of(pos, struct hsr_node, list);

		desc = net_tx_alloc(tx, data_len);
		if (!desc) {
			os_log(LOG_ERR, "Cannot alloc tx descriptor\n");
			continue;
		}

		hsr_supervision_frame((uint8_t *)desc + desc->l2_offset,
				      data_len, entry->mac, local_mac, seqnum);
		desc->len = data_len;
		if (net_tx(tx, desc) < 0)
			net_tx_free(desc);
	}
}

static void hsr_nodeforget_timer_handler(void *data)
{
	struct hsr_ctx *hsr = (struct hsr_ctx *)data;
	struct list_head *pos, *next, *head;
	uint16_t vid, seqnum = 0;
	struct hsr_node *entry;
	int i;

	head = &hsr->ring_list;
	for (pos = list_first(head); next = list_next(pos), pos != head; pos = next) {
		entry = container_of(pos, struct hsr_node, list);
		if (entry->cnt)
			entry->cnt = 0;
		else
			hsr_node_del(hsr, entry);
	}

	head = &hsr->proxy_list;
	for (pos = list_first(head); next = list_next(pos), pos != head; pos = next) {
		entry = container_of(pos, struct hsr_node, list);
		hsr_frer_gen_seq_get(entry->gen_id, &seqnum);

		for (i = 0; i < entry->stream_max; i++) {
			if (seqnum != entry->seqnum)
				continue;

			vid = entry->stream[i].vid;
			hsr_fdb_del(entry->mac, vid, 0);
			hsr_node_stream_del(hsr, entry, vid);
		}
		entry->seqnum = seqnum;

		if (entry->stream_max == 0)
			hsr_node_del(hsr, entry);
	}

	timer_start(&hsr->nodeforget_timer, HSR_TIMER_FORGET_PERIOD);
}

static void hsr_lifecheck_timer_handler(void *data)
{
	struct hsr_ctx *hsr = (struct hsr_ctx *)data;

	hsr->SupSequenceNumber++;
	hsr_supervision_send(&hsr->net_tx, &hsr->proxy_list, hsr->SupSequenceNumber);

	timer_start(&hsr->lifecheck_timer, HSR_TIMER_PERIOD);
}

static void hsr_port_config_init(struct hsr_port_config *port_cfg, struct hsr_config *cfg)
{
	int br_port_max = 0;
	int i;

	for (i = 1; i < cfg->port_max; i++) {
		switch (cfg->hsr_port[i].type) {
		case HSR_HOST_PORT:
			port_cfg->cpu_port = cfg->hsr_port[i].logical_port;
			break;

		case HSR_RING_PORT:
		case HSR_QUARD_RING_PORT:
			port_cfg->ring_port_mask |= (1 << br_port_max);
			/* fall through */

		case HSR_INTERNAL_PORT:
		case HSR_EXTERNAL_PORT:
			port_cfg->logical_br_port_list[br_port_max] = cfg->hsr_port[i].logical_port;
			br_port_max++;
			break;

		default:
			break;
		}

		if (cfg->hsr_port[i].type == HSR_INTERNAL_PORT)
			port_cfg->br_mgmt_port = br_port_max - 1;
	}

	port_cfg->br_port_max = br_port_max;
}

static struct hsr_ctx *hsr_alloc(unsigned int timer_n)
{
	struct hsr_ctx *hsr;
	unsigned int size;

	size = sizeof(struct hsr_ctx);
	size += timer_pool_size(timer_n);

	hsr = os_malloc(size);
	if (!hsr)
		return NULL;

	os_memset(hsr, 0, size);

	hsr->timer_ctx = (struct timer_ctx *)((u8 *)(hsr + 1));

	return hsr;
}

static int hsr_timer_init(struct hsr_ctx *hsr)
{
	hsr->lifecheck_timer.func = hsr_lifecheck_timer_handler;
	hsr->lifecheck_timer.data = hsr;

	if (timer_create(hsr->timer_ctx, &hsr->lifecheck_timer, 0, HSR_TIMER_PERIOD) < 0)
		goto err_timer_lifecheck;

	hsr->nodeforget_timer.func = hsr_nodeforget_timer_handler;
	hsr->nodeforget_timer.data = hsr;

	if (timer_create(hsr->timer_ctx, &hsr->nodeforget_timer, 0, HSR_TIMER_PERIOD) < 0)
		goto err_timer_nodeforget;

	return 0;

err_timer_nodeforget:
	timer_destroy(&hsr->lifecheck_timer);

err_timer_lifecheck:
	return -1;
}

static void hsr_exit_timers(struct hsr_ctx *hsr)
{
	timer_destroy(&hsr->lifecheck_timer);
	timer_destroy(&hsr->nodeforget_timer);
}

static void hsr_node_moden_set(struct hsr_ctx *hsr, struct hsr_node *node)
{
	struct hsr_port_config *port_cfg = &hsr->port_cfg;
	struct genavb_vlan_port_map map[HSR_SWITCH_PORT_MAX] = {0};
	uint8_t hsr_port[HSR_SWITCH_PORT_MAX];
	uint32_t hsr_port_n = 0;
	uint32_t streamid;
	uint16_t vid;
	bool tagged;
	int i, j;
	int rc;

	for (i = 0; i < port_cfg->br_port_max; i++) {
		if (port_cfg->ring_port_mask & (1 << i)) {
			map[i].control = GENAVB_VLAN_ADMIN_CONTROL_FORBIDDEN;
			hsr_port[hsr_port_n++] = i;
		} else {
			map[i].control = GENAVB_VLAN_ADMIN_CONTROL_FIXED;
		}
		map[i].port_id = port_cfg->logical_br_port_list[i];
	}

	if (hsr_port_n == 0)
		return;

	for (i = 0; i < node->stream_max; i++) {
		streamid = node->stream[i].stream_id;
		vid = node->stream[i].vid;
		tagged = node->stream[i].tagged;

		stream_identity_delete(streamid);
		hsr_stream_identify_add(hsr, node->mac, vid, tagged, streamid, 1 << hsr_port[0]);

		map[hsr_port[0]].control = GENAVB_VLAN_ADMIN_CONTROL_FIXED;
		hsr_vlan_update(STREAM_VID(streamid), false, map, port_cfg->br_port_max);
		map[hsr_port[0]].control = GENAVB_VLAN_ADMIN_CONTROL_FORBIDDEN;

		for (j = 1; j < hsr_port_n; j++) {
			rc = hsr_table_id_alloc(hsr->tables.stream_table);
			if (rc < 0)
				continue;
			streamid = rc;

			if (hsr_stream_identify_add(hsr, node->mac, vid, tagged, streamid, 1 << hsr_port[j]) < 0)
				continue;

			if (hsr_frer_identification_add(&hsr->port_cfg, streamid, port_cfg->ring_port_mask) < 0)
				continue;

			hsr_frer_rec_add(&hsr->port_cfg, node->rec_id, streamid);
			node->moden_stream[j - 1] = streamid;
			map[hsr_port[j]].control = GENAVB_VLAN_ADMIN_CONTROL_FIXED;
			hsr_vlan_update(STREAM_VID(streamid), false, map, port_cfg->br_port_max);
			map[hsr_port[j]].control = GENAVB_VLAN_ADMIN_CONTROL_FORBIDDEN;
		}
	}
}

static void hsr_node_moden_clear(struct hsr_ctx *hsr, struct hsr_node *node)
{
	struct hsr_port_config *port_cfg = &hsr->port_cfg;
	struct genavb_vlan_port_map map[HSR_SWITCH_PORT_MAX] = {0};
	uint32_t hsr_port_n = 0;
	uint32_t streamid;
	uint16_t vid;
	bool dynamic;
	int i, j;

	for (i = 0; i < port_cfg->br_port_max; i++)
		if (port_cfg->ring_port_mask & (1 << i))
			hsr_port_n++;

	if (hsr_port_n == 0)
		return;

	for (i = 0; i < node->stream_max; i++) {
		streamid = node->stream[i].stream_id;
		vid = node->stream[i].vid;
		vlan_read(vid, &dynamic, map);
		hsr_vlan_update(STREAM_VID(streamid), false, map, port_cfg->br_port_max);

		for (j = 1; j < hsr_port_n; j++) {
			streamid = node->moden_stream[j - 1];
			hsr_table_id_free(hsr->tables.stream_table, streamid);
			stream_identity_delete(streamid);
			hsr_frer_identification_del(&hsr->port_cfg, streamid);
			node->moden_stream[j - 1] = 0;
		}

		streamid = node->stream[i].stream_id;
		stream_identity_delete(streamid);
		hsr_stream_identify_add(hsr, node->mac, node->stream[i].vid, node->stream[i].tagged, streamid, node->fwdmask);
	}
}

static void hsr_node_operation_modeu(struct hsr_ctx *hsr, bool enable)
{
	uint8_t fwdmask = HSR_SWITCH_PORT_MASK;
	struct list_head *pos, *next, *head;
	struct hsr_node *entry;
	int i;

	head = &hsr->proxy_list;
	for (pos = list_first(head); next = list_next(pos), pos != head; pos = next) {
		entry = container_of(pos, struct hsr_node, list);

		if (!enable)
			fwdmask = entry->fwdmask;

		for (i = 0; i < entry->stream_max; i++)
			hsr_fdb_fwd_update(&hsr->port_cfg, entry->mac, entry->stream[i].vid, fwdmask);
	}
}

static void hsr_node_operation_moden(struct hsr_ctx *hsr, bool enable)
{
	struct list_head *pos, *next, *head;
	struct hsr_node *entry;

	head = &hsr->ring_list;
	for (pos = list_first(head); next = list_next(pos), pos != head; pos = next) {
		entry = container_of(pos, struct hsr_node, list);

		if (enable)
			hsr_node_moden_set(hsr, entry);
		else
			hsr_node_moden_clear(hsr, entry);
	}
}

static void hsr_node_operation_modet(struct hsr_ctx *hsr, bool enable)
{
	struct hsr_port_config *port_cfg = &hsr->port_cfg;
	struct genavb_sequence_identification entry = {0};
	uint32_t stream[HSR_SI_MAX_ENTRIES];
	unsigned int portid;
	int i;

	entry.stream = stream;

	for (i = 0; i < port_cfg->br_port_max; i++) {
		portid = port_cfg->logical_br_port_list[i];
		entry.stream_n = HSR_SI_MAX_ENTRIES;
		if (sequence_identification_read(portid, true, &entry) < 0)
			entry.stream_n = 0;
		else
			sequence_identification_delete(portid, true);

		if (!enable && (port_cfg->ring_port_mask & (1 << i)))
			entry.active = 1;
		else
			entry.active = 0;

		sequence_identification_update(portid, true, &entry);
	}
}

static void hsr_node_operation_mode_select(struct hsr_ctx *hsr, genavb_hsr_mode_t mode, bool enable)
{
	switch (mode) {
	case GENAVB_HSR_OPERATION_MODE_N:
		hsr_node_operation_moden(hsr, enable);
		break;
	case GENAVB_HSR_OPERATION_MODE_T:
		hsr_node_operation_modet(hsr, enable);
		break;
	case GENAVB_HSR_OPERATION_MODE_U:
		hsr_node_operation_modeu(hsr, enable);
		break;
	default:
		break;
	}
}

static void hsr_node_operation_mode_set(struct ipc_rx const *rx, struct ipc_desc *desc)
{
	genavb_hsr_mode_t mode;
	struct hsr_ctx *hsr;

	hsr = container_of(rx, struct hsr_ctx, ipc_rx_stack);
	mode = desc->u.data[0];

	if (hsr->mode == mode)
		return;

	if (hsr->mode != GENAVB_HSR_OPERATION_MODE_H)
		hsr_node_operation_mode_select(hsr, hsr->mode, false);

	hsr_node_operation_mode_select(hsr, mode, true);

	hsr->mode = mode;
}

__init void *hsr_init(struct hsr_config *cfg, unsigned long priv)
{
	struct hsr_ctx *hsr;
	unsigned int timer_n;
	struct net_address addr;

	timer_n = HSR_MAX_TIMERS;
	hsr = hsr_alloc(timer_n);
	if (!hsr)
		goto err_malloc;

	hsr->SupSequenceNumber = 0;

	hsr_port_config_init(&hsr->port_cfg, cfg);

	list_head_init(&hsr->ring_list);
	list_head_init(&hsr->proxy_list);

	memset(&addr, 0, sizeof(addr));
	addr.ptype = PTYPE_HSR;
	addr.port = PORT_ANY;

	if (net_rx_init(&hsr->net_rx, &addr, hsr_rx_process, priv) < 0)
		goto err_rx_init;

	addr.port = hsr->port_cfg.cpu_port;
	addr.priority = 0;
	if (net_tx_init(&hsr->net_tx, &addr) < 0)
		goto err_tx_init;

	if (ipc_rx_init(&hsr->ipc_rx_stack, IPC_HSR_STACK, hsr_node_operation_mode_set, priv) < 0)
		goto err_ipc_rx_stack;

	if (timer_pool_init(hsr->timer_ctx, timer_n, priv) < 0)
		goto err_timer_pool_init;

	if (hsr_timer_init(hsr) < 0)
		goto err_timer_init;

	timer_start(&hsr->lifecheck_timer, HSR_TIMER_PERIOD);
	timer_start(&hsr->nodeforget_timer, HSR_TIMER_FORGET_PERIOD);

	bridge_software_maclearn(1);

	return hsr;

err_timer_init:
	timer_pool_exit(hsr->timer_ctx);

err_timer_pool_init:
	ipc_rx_exit(&hsr->ipc_rx_stack);

err_ipc_rx_stack:
	net_tx_exit(&hsr->net_tx);

err_tx_init:
	net_rx_exit(&hsr->net_rx);

err_rx_init:
	os_free(hsr->timer_ctx);
	os_free(hsr);

err_malloc:
	return NULL;
}

__exit int hsr_exit(void *hsr_h)
{
	struct hsr_ctx *hsr = (struct hsr_ctx *)hsr_h;

	hsr_exit_timers(hsr);

	timer_pool_exit(hsr->timer_ctx);

	os_free(hsr->timer_ctx);

	net_rx_exit(&hsr->net_rx);

	net_tx_exit(&hsr->net_tx);

	hsr_node_list_free(hsr, &hsr->ring_list);
	hsr_node_list_free(hsr, &hsr->proxy_list);

	bridge_software_maclearn(0);

	os_free(hsr_h);

	return 0;
}
