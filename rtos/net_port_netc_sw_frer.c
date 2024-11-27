/*
* Copyright 2023-2024 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 @file
 @brief RTOS specific Network service implementation
 @details
*/

#include "genavb/error.h"

#include "net_port_netc_sw_frer.h"
#include "net_port_netc_stream_identification.h"
#include "net_port_netc_sw_drv.h"
#include "net_logical_port.h"

#include "config.h"
#include "log.h"

#if CFG_NUM_NETC_SW

#define API_TO_DRIVER_RTAG {0, kNETC_SqRTag, kNETC_SqHsrTag, kNETC_SqDraftRTag}
#define NETC_SEQR_MAX_HIST_LEN 127

static bool netc_sw_sequence_identification_get(struct netc_sw_drv *drv, uint32_t stream_handle, unsigned int *next_port, genavb_seqi_encapsulation_t *encapsulation, bool *active)
{
	int p, s;

	if (next_port)
		p = *next_port;
	else
		p = 0;

	for (; p < CFG_NUM_NETC_SW_PORTS; p++) {
		for (s = 0; s < drv->seqi_table[p].stream_n; s++) {
			if (drv->seqi_table[p].stream_handle[s] == stream_handle) {
				if (next_port)
					*next_port = p;
				if (encapsulation)
					*encapsulation = drv->seqi_table[p].tag;
				if (active)
					*active = drv->seqi_table[p].active;
				return true;
			}
		}
	}

	return false;
}

static int netc_sw_sequence_identification_get_port_map(struct netc_sw_drv *drv, uint32_t stream_handle, uint32_t *port_map, uint32_t *active_port_map, genavb_seqi_encapsulation_t *encapsulation)
{
	unsigned int port_idx = 0;
	genavb_seqi_encapsulation_t enc;
	bool active;
	int i = 0;

	while (netc_sw_sequence_identification_get(drv, stream_handle, &port_idx, &enc, &active)) {
		if (port_map)
			*port_map |= (1 << port_idx);
		if (active && active_port_map)
			*active_port_map |= (1 << port_idx);

		port_idx++;

		/* Sequence tag must be same for all ports */
		if (encapsulation) {
			if (i == 0) {
				*encapsulation = enc;
			} else {
				if (*encapsulation != enc)
					return -1;
			}
			i++;
		}
	}

	return 0;
}

static bool netc_sw_sequence_generation_get(struct netc_sw_drv *drv, uint32_t stream_handle, unsigned int *seqg_index, unsigned int *stream_index)
{
	int i, j;

	for (i = 0; i < SEQG_MAX_ENTRIES; i++) {
		for (j = 0; j < drv->seqg_table[i].stream_n; j++) {
			if (drv->seqg_table[i].stream_handle[j] == stream_handle) {
				if (seqg_index)
					*seqg_index = i;
				if (stream_index)
					*stream_index = j;

				return true;
			}
		}
	}

	return false;
}

static bool netc_sw_sequence_recovery_get(struct netc_sw_drv *drv, uint32_t stream_handle, unsigned int port_idx, unsigned int *seqr_index)
{
	int i, j;

	for (i = 0; i < SEQR_MAX_ENTRIES; i++) {
		for (j = 0; j < drv->seqr_table[i].stream_n; j++) {
			if (drv->seqr_table[i].stream_handle[j] == stream_handle) {
				if (drv->seqr_table[i].port_map & (1 << port_idx)) {
					if (seqr_index)
						*seqr_index = i;
					return true;
				}
			}
		}
	}

	return false;
}

int netc_sw_frer_get_eid(void *drv, uint32_t stream_handle, uint32_t *isqg_eid, uint32_t *et_eid, uint32_t *et_port_map)
{
	struct netc_sw_drv *sw_drv = drv;
	unsigned int seqg_index;
	uint32_t tmp_et_port_map = 0;
	uint32_t tmp_et_eid;
	unsigned int port_idx;

	if (netc_sw_sequence_generation_get(sw_drv, stream_handle, &seqg_index, NULL)) {
		if (isqg_eid) {
			if (sw_drv->seqg_table[seqg_index].programmed)
				*isqg_eid = seqg_index;
			else
				*isqg_eid = NULL_ENTRY_ID;
		}
	}

	for (port_idx = 0; port_idx < CFG_NUM_NETC_SW_PORTS; port_idx++) {
		if (netc_sw_sequence_recovery_get(sw_drv, stream_handle, port_idx, NULL)) {
			tmp_et_port_map |= (1 << port_idx);
		}
	}

	if (tmp_et_port_map)
		tmp_et_eid = (stream_handle * CFG_NUM_NETC_SW_PORTS);
	else
		tmp_et_eid = NULL_ENTRY_ID;

	if (et_eid)
		*et_eid = tmp_et_eid;

	if (et_port_map)
		*et_port_map = tmp_et_port_map;

	return 0;
}

static int netc_sw_add_iseqg_table_entry(struct netc_sw_drv *drv, uint32_t isqg_eid, genavb_seqi_encapsulation_t encapsulation)
{
	netc_tb_iseqg_sqtag_t api_to_driver_rtag[4] = API_TO_DRIVER_RTAG;
	netc_tb_iseqg_config_t config = {0};

	config.entryID = isqg_eid;
	config.cfge.sqTag = api_to_driver_rtag[encapsulation];

	if (SWT_FRERAddISEQGTableEntry(&drv->handle, &config) != kStatus_Success) {
		os_log(LOG_ERR, "SWT_FRERAddISEQGTableEntry failed\n");
		goto err;
	}

	return 0;
err:
	return -1;
}

static int netc_sw_delete_iseqg_table_entry(struct netc_sw_drv *drv, uint32_t isqg_eid)
{
	if (SWT_FRERDelISEQGTableEntry(&drv->handle, isqg_eid) != kStatus_Success) {
		os_log(LOG_ERR, "SWT_FRERDelISEQGTableEntry failed\n");
		goto err;
	}

	return 0;
err:
	return -1;
}

static void netc_sw_et_update_esqa_params(netc_tb_et_config_t *entry, void *data)
{
	uint32_t esqr_eid = *(uint32_t *)data;

	entry->cfge.esqaTgtEID = esqr_eid; /* Egress Sequence Actions Target ID */
	if (esqr_eid != NULL_ENTRY_ID)
		entry->cfge.esqa = kNETC_HasEsqAction; /* Egress Sequence Recovery Action */
	else
		entry->cfge.esqa = kNETC_NoEsqAction;
}

static int netc_sw_et_update_eseqr(struct netc_sw_drv *drv, uint32_t stream_handle, uint32_t esqr_eid, unsigned int port_idx)
{
	uint32_t et_eid = netc_sw_get_stream_et_eid(stream_handle, port_idx);

	return netc_sw_add_or_update_et_table_entry(drv, et_eid, netc_sw_et_update_esqa_params, &esqr_eid);
}

static int netc_sw_del_rtag(struct netc_sw_drv *drv, uint32_t stream_handle, uint32_t port_map)
{
	unsigned int port;
	bool del_rtag;

	for (port = 0; port < CFG_NUM_NETC_SW_PORTS; port++) {
		if (port_map & (1 << port))
			del_rtag = true;
		else
			del_rtag = false;

		if (netc_si_update_et_rtag(drv, stream_handle, port, del_rtag) < 0)
			goto err;
	}

	return 0;
err:
	return -1;
}

static int netc_sw_sequence_generation_program(struct netc_sw_drv *drv, uint32_t seqg_index, genavb_seqi_encapsulation_t encapsulation)
{
	uint32_t isqg_eid;

	if (!drv->seqg_table[seqg_index].programmed) {
		/* ISQG_EID == sequence generation table index */
		isqg_eid = seqg_index;

		if (netc_sw_add_iseqg_table_entry(drv, isqg_eid, encapsulation) < 0)
			goto err;

		drv->seqg_table[seqg_index].programmed = true;
	}

	return 0;
err:
	return -1;
}

static int netc_sw_frer_stream_seqi_apply(struct netc_sw_drv *drv, uint32_t stream_handle, genavb_seqi_encapsulation_t *encapsulation)
{
	uint32_t seqi_port_map = 0;
	uint32_t seqi_active_port_map = 0;
	uint32_t del_rtag_port_map;

	if (netc_sw_sequence_identification_get_port_map(drv, stream_handle, &seqi_port_map, &seqi_active_port_map, encapsulation) < 0)
		goto err;

	if (seqi_port_map) {
#if 1
		/* delete Redundancy Tag on all ports except active */
		del_rtag_port_map = ~seqi_active_port_map;
#else
		/* delete Redundancy Tag only on non active ports */
		del_rtag_port_map = seqi_port_map & ~seqi_active_port_map;
#endif
		if (netc_sw_del_rtag(drv, stream_handle, del_rtag_port_map) < 0)
			goto err;

		return 1;
	}

	return 0;
err:
	return -1;
}

static int netc_sw_frer_stream_replicate_apply(struct netc_sw_drv *drv, uint32_t stream_handle)
{
	genavb_seqi_encapsulation_t encapsulation;
	unsigned int seqg_index;
	int rc;

	if (netc_sw_sequence_generation_get(drv, stream_handle, &seqg_index, NULL)) {
		rc = netc_sw_frer_stream_seqi_apply(drv, stream_handle, &encapsulation);
		if (rc < 0) {
			goto err;
		} else if (rc == 1) {
			/* Apply Generation only if seqi exists */

			/* Add seqg HW entry */
			if (netc_sw_sequence_generation_program(drv, seqg_index, encapsulation) < 0)
				goto err;
			/* Link Stream Identification and Sequence Generation */
			if (netc_si_update_frer_ref(drv, stream_handle) < 0)
				goto err;
		}
	}

	return 0;
err:
	return -1;
}

static int netc_sw_frer_stream_recovery_apply(struct netc_sw_drv *drv, uint32_t stream_handle, unsigned int port_idx)
{
	unsigned int seqr_index;

	if (netc_sw_frer_stream_seqi_apply(drv, stream_handle, NULL) < 0)
		goto err;

	if (netc_sw_sequence_recovery_get(drv, stream_handle, port_idx, &seqr_index)) {
		if (netc_sw_et_update_eseqr(drv, stream_handle, seqr_index, port_idx) < 0)
			goto err;
	}

	return 0;
err:
	return -1;
}

static int netc_sw_sequence_generation_update(struct net_bridge *bridge, uint32_t index, struct genavb_sequence_generation *entry, unsigned int option)
{
	struct netc_sw_drv *drv = net_bridge_drv(bridge);
	int i, rc;

	if (index >= SEQG_MAX_ENTRIES) {
		rc = -GENAVB_ERR_FRER_MAX_ENTRIES;
		goto err;
	}

	switch (option) {
	case SEQG_UPDATE_OPTION_RESET:
		if (drv->seqg_table[index].stream_handle == NULL) {
			rc = -GENAVB_ERR_FRER_ENTRY_NOT_FOUND;
			goto err;
		}

		if (SWT_FRERResetSEQGTableEntry(&drv->handle, index) != kStatus_Success) {
			rc = -GENAVB_ERR_FRER_SEQG_RESET;
			goto err;
		}
		break;

	case SEQG_UPDATE_OPTION_CONFIG:
		if (entry->stream_n > SI_MAX_ENTRIES) {
			rc = -GENAVB_ERR_FRER_MAX_STREAM;
			goto err;
		}

		if (drv->seqg_table[index].stream_handle != NULL) {
			if (drv->seqg_table[index].stream_n != entry->stream_n) {
				rc = -GENAVB_ERR_FRER_ENTRY_USED;
				goto err;
			}

			for (i = 0; i < entry->stream_n; i++) {
				if (drv->seqg_table[index].stream_handle[i] != entry->stream[i]) {
					rc = -GENAVB_ERR_FRER_ENTRY_USED;
					goto err;
				}
			}
		} else {
			drv->seqg_table[index].stream_handle = rtos_malloc(entry->stream_n * sizeof(uint16_t));
			if (!drv->seqg_table[index].stream_handle) {
				rc = -GENAVB_ERR_NO_MEMORY;
				goto err;
			}

			/* prepare */
			for (i = 0; i < entry->stream_n; i++) {
				drv->seqg_table[index].stream_handle[i] = entry->stream[i];
			}

			drv->seqg_table[index].stream_n = entry->stream_n;
		}

		/* apply */
		for (i = 0; i < entry->stream_n; i++) {
			if (netc_sw_frer_stream_replicate_apply(drv, entry->stream[i]) < 0) {
				rc = -GENAVB_ERR_FRER_HW_CONFIG;
				goto err_apply;
			}
		}
		break;

	default:
		rc = -GENAVB_ERR_FRER_NOT_SUPPORTED;
		goto err;	
	}

	return GENAVB_SUCCESS;

err_apply:
	drv->seqg_table[index].stream_n = 0;
	rtos_free(drv->seqg_table[index].stream_handle);
err:
	return rc;
}

static int netc_sw_sequence_generation_delete(struct net_bridge *bridge, uint32_t index)
{
	struct netc_sw_drv *drv = net_bridge_drv(bridge);
	uint32_t stream_handle;
	uint32_t isqg_eid;
	int i, rc;

	if (index >= SEQG_MAX_ENTRIES) {
		rc = -GENAVB_ERR_FRER_MAX_ENTRIES;
		goto err;
	}

	if (drv->seqg_table[index].stream_handle == NULL) {
		rc = -GENAVB_ERR_FRER_ENTRY_NOT_FOUND;
		goto err;
	}

	if (drv->seqg_table[index].programmed) {

		drv->seqg_table[index].programmed = false;

		for (i = 0; i < drv->seqg_table[index].stream_n; i++) {
			stream_handle = drv->seqg_table[index].stream_handle[i];

			/* Remove Stream Identification to Sequence Generation Link */
			if (netc_si_update_frer_ref(drv, stream_handle) < 0) {
				rc = -GENAVB_ERR_FRER_HW_CONFIG;
				goto err;
			}

			/* Stop removing HSR Tag on all ports */
			if (netc_sw_del_rtag(drv, stream_handle, 0) < 0) {
				rc = -GENAVB_ERR_FRER_HW_CONFIG;
				goto err;
			}
		}

		/* Delete ISQG Table entry */
		/* ISQG_EID == sequence generation table index */
		isqg_eid = index;
		if (netc_sw_delete_iseqg_table_entry(drv, isqg_eid) < 0) {
			rc = -GENAVB_ERR_FRER_HW_CONFIG;
			goto err;
		}
	}

	rtos_free(drv->seqg_table[index].stream_handle);
	memset(&drv->seqg_table[index], 0, sizeof(struct netc_sw_seqg));

	return GENAVB_SUCCESS;
err:
	return rc;
}

static int netc_sw_sequence_generation_read(struct net_bridge *bridge, uint32_t index, struct genavb_sequence_generation *entry)
{
	struct netc_sw_drv *drv = net_bridge_drv(bridge);
	netc_tb_iseqg_sgse_t state;
	int i, rc;

	if (index >= SEQG_MAX_ENTRIES) {
		rc = -GENAVB_ERR_FRER_MAX_ENTRIES;
		goto err;
	}

	if (drv->seqg_table[index].stream_handle == NULL) {
		rc = -GENAVB_ERR_FRER_ENTRY_NOT_FOUND;
		goto err;
	}

	if (entry->stream == NULL) {
		rc = -GENAVB_ERR_FRER_PARAMS;
		goto err;
	}

	entry->stream_n = drv->seqg_table[index].stream_n;

	for (i = 0; i < entry->stream_n; i++) {
		entry->stream[i] = drv->seqg_table[index].stream_handle[i];
	}

	entry->direction_out_facing = false;
	entry->reset = false;

	if (SWT_FRERGetISEQGState(&drv->handle, index, &state) != kStatus_Success) {
		rc = -GENAVB_ERR_FRER_HW_READ;
		goto err;
	}

	entry->seqnum = state.sqgNum;

	return GENAVB_SUCCESS;
err:
	return rc;
}

static int netc_sw_add_esqr_table_entry(struct netc_sw_drv *drv, uint32_t esqr_eid, struct genavb_sequence_recovery *entry)
{
	netc_tb_eseqr_config_t eseqr_config = {0};

	eseqr_config.entryID = esqr_eid;

	if (entry->algorithm == GENAVB_SEQR_VECTOR) {
		if ((entry->history_length < 1) ||
		    (entry->history_length > NETC_SEQR_MAX_HIST_LEN))
			goto err;
	}

	eseqr_config.cfge.sqTag = kNETC_AcceptAnyTag;
	eseqr_config.cfge.sqrTnsq = entry->take_no_sequence;
	eseqr_config.cfge.sqrAlg = entry->algorithm;
	eseqr_config.cfge.sqrType = entry->individual_recovery;
	eseqr_config.cfge.sqrHl = entry->history_length;
	/*
	 * Note this field is not part of the 802.1CB standard, to achieve behavior compliant to the
	 * standard the user should set SQR_FWL = SQR_HL.
	 */
	eseqr_config.cfge.sqrFwl = entry->history_length;
	/* recovery timeout period in increments of 1.048576 milliseconds*/
	eseqr_config.cfge.sqrTp = entry->reset_timeout;

	if (SWT_FRERConfigESEQRTableEntry(&drv->handle, &eseqr_config) != kStatus_Success) {
		os_log(LOG_ERR, "SWT_FRERConfigESEQRTableEntry failed\n");
		goto err;
	}

	return 0;
err:
	return -1;
}

static int netc_sw_sequence_recovery_update(struct net_bridge *bridge, uint32_t index, struct genavb_sequence_recovery *entry)
{
	struct netc_sw_drv *drv = net_bridge_drv(bridge);
	unsigned int port_idx;
	uint32_t esqr_eid;
	int s, p, rc;

	if (index >= SEQR_MAX_ENTRIES) {
		rc = -GENAVB_ERR_FRER_MAX_ENTRIES;
		goto err;
	}

	if (drv->seqr_table[index].stream_handle != NULL) {
		rc = -GENAVB_ERR_FRER_ENTRY_USED;
		goto err;
	}

	if (entry->stream_n > SI_MAX_ENTRIES) {
		rc = -GENAVB_ERR_FRER_MAX_STREAM;
		goto err;
	}

	drv->seqr_table[index].stream_handle = rtos_malloc(entry->stream_n * sizeof(uint16_t));
	if (!drv->seqr_table[index].stream_handle) {
		rc = -GENAVB_ERR_NO_MEMORY;
		goto err;
	}

	/* ESQR_EID = sequence recovery index */
	esqr_eid = index;
	if (netc_sw_add_esqr_table_entry(drv, esqr_eid, entry) < 0) {
		rc = -GENAVB_ERR_FRER_HW_CONFIG;
		goto err;
	}

	/* prepare */
	for (s = 0; s < entry->stream_n; s++) {
		drv->seqr_table[index].stream_handle[s] = entry->stream[s];
		for (p = 0; p < entry->port_n; p++) {
			port_idx = __logical_port_get(entry->port[p])->phys->base;
			drv->seqr_table[index].port_map |= (1 << port_idx);
		}
	}
	drv->seqr_table[index].stream_n = entry->stream_n;

	/* apply */
	for (s = 0; s < entry->stream_n; s++) {
		for (p = 0; p < entry->port_n; p++) {
			port_idx = __logical_port_get(entry->port[p])->phys->base;
			if (netc_sw_frer_stream_recovery_apply(drv, entry->stream[s], port_idx) < 0) {
				rc = -GENAVB_ERR_FRER_HW_CONFIG;
				goto err_apply;
			}
		}
	}

	return GENAVB_SUCCESS;

err_apply:
	rtos_free(drv->seqr_table[index].stream_handle);
	memset(&drv->seqr_table[index], 0, sizeof(struct netc_sw_seqr));
err:
	return rc;
}

static int netc_sw_sequence_recovery_delete(struct net_bridge *bridge, uint32_t index)
{
	struct netc_sw_drv *drv = net_bridge_drv(bridge);
	uint32_t stream_handle;
	int s, p, rc;

	if (index >= SEQR_MAX_ENTRIES) {
		rc = -GENAVB_ERR_FRER_MAX_ENTRIES;
		goto err;
	}

	if (!drv->seqr_table[index].stream_n) {
		rc = -GENAVB_ERR_FRER_ENTRY_NOT_FOUND;
		goto err;
	}

	for (s = 0; s < drv->seqr_table[index].stream_n; s++) {
		stream_handle = drv->seqr_table[index].stream_handle[s];

		for (p = 0; p < CFG_NUM_NETC_SW_PORTS; p++) {
			if (drv->seqr_table[index].port_map & (1 << p)) {
				if (netc_sw_et_update_eseqr(drv, stream_handle, NULL_ENTRY_ID, p) < 0) {
					rc = -GENAVB_ERR_FRER_HW_CONFIG;
					goto err;
				}
			}
		}
	}

	/* No Delete for Egress Sequence Recovery Table */

	rtos_free(drv->seqr_table[index].stream_handle);
	memset(&drv->seqr_table[index], 0, sizeof(struct netc_sw_seqr));

	return GENAVB_SUCCESS;
err:
	return rc;
}

static int netc_sw_sequence_recovery_read(struct net_bridge *bridge, uint32_t index, struct genavb_sequence_recovery *entry)
{
	struct netc_sw_drv *drv = net_bridge_drv(bridge);
	netc_tb_eseqr_stse_t statistic;
	netc_tb_eseqr_srse_t state;
	netc_tb_eseqr_cfge_t cfge;
	unsigned int port_n = 0;
	uint32_t esqr_eid;
	int s, p, rc;

	if (index >= SEQR_MAX_ENTRIES) {
		rc = -GENAVB_ERR_FRER_MAX_ENTRIES;
		goto err;
	}

	if (drv->seqr_table[index].stream_handle == NULL) {
		rc = -GENAVB_ERR_FRER_ENTRY_NOT_FOUND;
		goto err;
	}

	if (entry->stream == NULL) {
		rc = -GENAVB_ERR_FRER_PARAMS;
		goto err;
	}

	if (entry->port == NULL) {
		rc = -GENAVB_ERR_FRER_PARAMS;
		goto err;
	}

	for (s = 0; (s < drv->seqr_table[index].stream_n) && (s < entry->stream_n); s++) {
		entry->stream[s] = drv->seqr_table[index].stream_handle[s];
	}

	for (p = 0; p < CFG_NUM_NETC_SW_PORTS; p++) {
		if (drv->seqr_table[index].port_map & (1 << p)) {
			if (drv->port[p]) {
				entry->port[port_n] = drv->port[p]->logical_port->id;
				port_n++;
				if (port_n >= entry->port_n)
					break;
			}
		}
	}

	/* ESQR_EID = sequence recovery index */
	esqr_eid = index;
	if (SWT_FRERQueryESEQRTableEntry(&drv->handle, esqr_eid, &statistic, &cfge, &state) != kStatus_Success) {
		rc = -GENAVB_ERR_FRER_HW_READ;
		goto err;
	}

	entry->stream_n = drv->seqr_table[index].stream_n;
	entry->port_n = port_n;
	entry->algorithm = cfge.sqrAlg;
	entry->history_length = cfge.sqrHl;
	entry->reset_timeout = cfge.sqrTp;

	return GENAVB_SUCCESS;
err:
	return rc;
}

static int netc_sw_sequence_identification_update(struct net_bridge *bridge, struct net_port *port, struct genavb_sequence_identification *entry)
{
	struct netc_sw_drv *drv = net_bridge_drv(bridge);
	unsigned int port_idx = port->base;
	int i, rc;

	if (drv->seqi_table[port_idx].stream_n) {
		rc = -GENAVB_ERR_FRER_ENTRY_USED;
		goto err;
	}

	if (entry->stream_n > SI_MAX_ENTRIES) {
		rc = -GENAVB_ERR_FRER_MAX_STREAM;
		goto err;
	}

	/* prepare */
	for (i = 0; i < entry->stream_n; i++) {
		drv->seqi_table[port_idx].stream_handle[i] = entry->stream[i];
	}

	drv->seqi_table[port_idx].tag = entry->encapsulation;
	drv->seqi_table[port_idx].active = entry->active;
	drv->seqi_table[port_idx].stream_n = entry->stream_n;

	/* apply */
	for (i = 0; i < entry->stream_n; i++) {
		if (netc_sw_frer_stream_replicate_apply(drv, entry->stream[i]) < 0) {
			rc = -GENAVB_ERR_FRER_HW_CONFIG;
			goto err_apply;
		}

		if (netc_sw_frer_stream_recovery_apply(drv, entry->stream[i], port_idx) < 0) {
			rc = -GENAVB_ERR_FRER_HW_CONFIG;
			goto err_apply;
		}
	}

	return GENAVB_SUCCESS;

err_apply:
	memset(&drv->seqi_table[port_idx], 0, sizeof(struct netc_sw_seqi));
err:
	return rc;
}

static int netc_sw_sequence_identification_delete(struct net_bridge *bridge, struct net_port *port)
{
	struct netc_sw_drv *drv = net_bridge_drv(bridge);
	unsigned int port_idx = port->base;
	uint32_t stream_handle;
	bool del_rtag;
	int i, rc;

	if (!drv->seqi_table[port_idx].stream_n) {
		rc = -GENAVB_ERR_FRER_ENTRY_NOT_FOUND;
		goto err;
	}

	for (i = 0; i < drv->seqi_table[port_idx].stream_n; i++) {
		stream_handle = drv->seqi_table[port_idx].stream_handle[i];

		/* keep generated or received rtag */
		del_rtag = false;

		if (netc_si_update_et_rtag(drv, stream_handle, port_idx, del_rtag) < 0) {
			rc = -GENAVB_ERR_FRER_HW_CONFIG;
			goto err;
		}
	}

	memset(&drv->seqi_table[port_idx], 0, sizeof(struct netc_sw_seqi));

	return GENAVB_SUCCESS;
err:
	return rc;
}

static int netc_sw_sequence_identification_read(struct net_bridge *bridge, struct net_port *port, struct genavb_sequence_identification *entry)
{
	struct netc_sw_drv *drv = net_bridge_drv(bridge);
	unsigned int port_idx = port->base;
	int i, rc;

	if (!drv->seqi_table[port_idx].stream_n) {
		rc = -GENAVB_ERR_FRER_ENTRY_NOT_FOUND;
		goto err;
	}

	if (!entry->stream) {
		rc = -GENAVB_ERR_FRER_PARAMS;
		goto err;
	}

	entry->stream_n = drv->seqi_table[port_idx].stream_n;

	for (i = 0; i < drv->seqi_table[port_idx].stream_n; i++) {
		entry->stream[i] = drv->seqi_table[port_idx].stream_handle[i];
	}

	entry->active = drv->seqi_table[port_idx].active;
	entry->encapsulation = drv->seqi_table[port_idx].tag;
	entry->path_id_lan_id = 0;

	return GENAVB_SUCCESS;
err:
	return rc;
}

void netc_sw_frer_init(struct net_bridge *bridge)
{
	bridge->drv_ops.seqg_update = netc_sw_sequence_generation_update;
	bridge->drv_ops.seqg_delete = netc_sw_sequence_generation_delete;
	bridge->drv_ops.seqg_read = netc_sw_sequence_generation_read;

	bridge->drv_ops.seqr_update = netc_sw_sequence_recovery_update;
	bridge->drv_ops.seqr_delete = netc_sw_sequence_recovery_delete;
	bridge->drv_ops.seqr_read = netc_sw_sequence_recovery_read;

	bridge->drv_ops.seqi_update = netc_sw_sequence_identification_update;
	bridge->drv_ops.seqi_delete = netc_sw_sequence_identification_delete;
	bridge->drv_ops.seqi_read = netc_sw_sequence_identification_read;
}
#endif
