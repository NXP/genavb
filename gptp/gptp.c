/*
 * Copyright 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief NXP GPTP core functions
 @details Handles all GPTP
*/

#include "os/clock.h"
#include "os/sys_types.h"

#include "gptp.h"
#include "md_fsm_802_3.h"
#include "port_fsm.h"
#include "clock_sl_fsm.h"
#include "clock_ms_fsm.h"
#include "target_clock_adj.h"
#include "common/ptp_time_ops.h"
#include "bmca.h"
#include "site_fsm.h"
#include "port_fsm.h"

#include "genavb/ptp.h"
#include "genavb/qos.h"

extern void ptp_time_ops_unit_test(void);

static const u8 ptp_dst_mac[6] = MC_ADDR_PTP;

static struct gptp_instance *instance_from_domain(struct gptp_ctx *gptp, u8 domain_number)
{
	int i;

	for (i = 0; i < gptp->domain_max; i++) {
		if (gptp->instances[i]->domain.domain_number == domain_number)
			return gptp->instances[i];
	}

	return NULL;
}

unsigned int net_port_to_logical(struct gptp_net_port *net_port)
{
	return net_port->logical_port;
}

static struct gptp_port *net_port_to_gptp(struct gptp_net_port *net_port, u8 domain_index)
{
	struct gptp_ctx *gptp = container_of(net_port, struct gptp_ctx, net_ports[net_port->port_id]);

	if (domain_index >= gptp->domain_max)
		return NULL;

	return &gptp->instances[domain_index]->ports[net_port->port_id];
}

struct gptp_net_port *net_port_from_gptp(struct gptp_port *port)
{
	return &port->instance->gptp->net_ports[port->port_id];
}

struct gptp_net_port *net_port_from_gptp_common(struct gptp_port_common *port_common)
{
	return &port_common->gptp->net_ports[port_common->port_id];
}

static struct gptp_net_port *logical_to_net_port(struct gptp_ctx *gptp, unsigned int logical_port)
{
	struct gptp_net_port *net_port = NULL;
	int i;

	for (i = 0; i < gptp->port_max; i++) {
		if (logical_port == net_port_to_logical(&gptp->net_ports[i])) {
			if (gptp->net_ports[i].initialized)
				net_port = &gptp->net_ports[i];
			break;
		}
	}

	return net_port;
}


static struct gptp_port_common *get_cmlds_port_common(struct gptp_ctx *gptp, u16 link_id)
{
	return &gptp->cmlds.link_ports[link_id].c;
}


static const char * gptp_delay_mechanism2string(ptp_delay_mechanism_t delay_mechanism)
{
	switch (delay_mechanism) {
	case P2P:
		return "P2P";
		break;

	case COMMON_P2P:
		return "COMMON_P2P";
		break;

	case SPECIAL:
		return "SPECIAL";
		break;

	default:
		return "Unknown delay mechanism";
		break;
	}
}


static bool is_cmlds_link_port_enabled(struct gptp_ctx *gptp, unsigned int link_id)
{
	if (gptp->cmlds.link_ports[link_id].cmlds_link_port_enabled)
		return true;

	return false;
}

/*
Return 1 if the port uses the COMMON_P2P delay mechanism  else return 0
*/
static bool is_common_p2p(struct gptp_port *port)
{
	if (port->params.delay_mechanism == COMMON_P2P)
		return true;

	return false;
}

bool get_as_capable_accross_domains(struct gptp_port *port)
{
	struct gptp_port_common *c;

	if (is_common_p2p(port))
		c = get_cmlds_port_common(port->instance->gptp, port->link_id);
	else
		c = port->c;

	return c->params.as_capable_across_domains;
}

bool get_port_asymmetry (struct gptp_port *port)
{
	struct gptp_net_port *net_port = net_port_from_gptp( port);

	return net_port->asymmetry_measurement_mode;
}

bool get_port_oper(struct gptp_port *port)
{
	struct gptp_net_port *net_port = net_port_from_gptp( port);

	return net_port->port_oper;
}

struct ptp_port_identity *get_port_identity(struct gptp_port *port)
{
	struct gptp_port_common *c;

	if (is_common_p2p(port))
		c = get_cmlds_port_common(port->instance->gptp, port->link_id);
	else
		c = port->instance->gptp->instances[0]->ports[port->port_id].c;

	return &c->identity;
}

u16 get_port_identity_number(struct gptp_port *port)
{
	struct ptp_port_identity *identity = get_port_identity(port);

	return identity->port_number;
}

u8 *get_port_identity_clock_id(struct gptp_port *port)
{
	struct ptp_port_identity *identity = get_port_identity(port);

	return identity->clock_identity;
}

/*
Retrieve either the per instance specific mean delay or the cmlds's link port one associated to the port
*/
struct ptp_u_scaled_ns *get_mean_link_delay(struct gptp_port *port)
{
	struct gptp_port_common *c;

	if (is_common_p2p(port))
		c = get_cmlds_port_common(port->instance->gptp, port->link_id);
	else
		c = port->instance->gptp->instances[0]->ports[port->port_id].c;

	return &c->params.mean_link_delay;
}

/*
Retrieve either the per instance specific neighbor rate ratio or the cmlds's link port one associated to the port
*/
ptp_double get_neighbor_rate_ratio(struct gptp_port *port)
{
	struct gptp_port_common *c;

	if (is_common_p2p(port))
		c = get_cmlds_port_common(port->instance->gptp, port->link_id);
	else
		c = port->instance->gptp->instances[0]->ports[port->port_id].c;

	return c->params.neighbor_rate_ratio;
}


/*
Retrieve either the per instance specific delay asymmetry or the cmlds's link port one associated to the port
*/
struct ptp_scaled_ns *get_delay_asymmetry(struct gptp_port *port)
{
	struct gptp_port_common *c;

	if (is_common_p2p(port))
		c = get_cmlds_port_common(port->instance->gptp, port->link_id);
	else
		c = port->instance->gptp->instances[0]->ports[port->port_id].c;

	return &c->params.delay_asymmetry;
}


static const char * gptp_msgtype2string(u8 msg_type)
{
	switch (msg_type) {
	case PTP_MSG_TYPE_SYNC:
		return "SYNC";
		break;
	case PTP_MSG_TYPE_PDELAY_REQ:
		return "PDELAY_REQ";
		break;
	case PTP_MSG_TYPE_PDELAY_RESP:
		return "PDELAY_RESP";
		break;
	case PTP_MSG_TYPE_PDELAY_RESP_FUP:
		return "PDELAY_RESP_FUP";
		break;
	case PTP_MSG_TYPE_FOLLOW_UP:
		return "FOLLOW_UP";
		break;
	case PTP_MSG_TYPE_ANNOUNCE:
		return "ANNOUNCE";
		break;
	case PTP_MSG_TYPE_SIGNALING:
		return "SIGNALING";
		break;
	default:
		return "Unknown message type";
		break;
	}
}

/* GPTP GM status
 * Propagates gm_id and path_trace info for a given domain
 */
void gptp_ipc_gm_status(struct gptp_instance *instance, struct ipc_tx *ipc, unsigned int ipc_dst)
{
	struct ipc_desc *desc;
	int rc;
	unsigned int size, num_ptlv;

	if (instance->params.num_ptlv > MAX_PTLV_ENTRIES) {
		os_log(LOG_ERR, "Invalid number of path trace entries(%u)\n", instance->params.num_ptlv);
		return;
	}

	size = sizeof(struct genavb_msg_gm_status) + instance->params.num_ptlv * sizeof(struct ptp_clock_identity);

	if (size > DEFAULT_IPC_DATA_SIZE)
		size = DEFAULT_IPC_DATA_SIZE;

	/* Number of entries to copy.*/
	num_ptlv = (size - sizeof(struct genavb_msg_gm_status)) / sizeof(struct ptp_clock_identity);

	desc = ipc_alloc(ipc, size);
	if (desc) {
		desc->dst = ipc_dst;
		desc->type = GENAVB_MSG_GM_STATUS;

		desc->u.gm_status.gm_id = instance->cfg.gm_id;
		desc->u.gm_status.domain = instance->domain.domain_number;

		desc->u.gm_status.num_ptlv = num_ptlv;

		os_memcpy(&desc->u.gm_status.path_sequence, &instance->params.path_trace, num_ptlv * sizeof(struct ptp_clock_identity));

		desc->len = size;

		rc = ipc_tx(ipc, desc);
		if (rc < 0) {
			if (rc != -IPC_TX_ERR_NO_READER)
				os_log(LOG_ERR, "ipc_tx() failed\n");

			ipc_free(ipc, desc);
		}
	} else
		os_log(LOG_ERR, "ipc_alloc() failed\n");
}

/* GPTP Port Params indication
 * Propagate as_capable and pdelay info for a given port
 */
void gptp_ipc_gptp_port_params(struct gptp_instance *instance, struct gptp_port *port, struct ipc_tx *ipc, unsigned int ipc_dst)
{
	struct ipc_desc *desc;
	struct gptp_net_port *net_port;
	u64 pdelay;
	int rc;

	desc = ipc_alloc(ipc, sizeof(struct genavb_msg_gptp_port_params));
	if (desc) {
		desc->dst = ipc_dst;
		desc->type = GENAVB_MSG_GPTP_PORT_PARAMS;
		desc->len = sizeof(struct genavb_msg_gptp_port_params);

		os_memset(&desc->u.gptp_port_params, 0, sizeof(struct genavb_msg_gptp_port_params));

		desc->u.gptp_port_params.domain = instance->domain.domain_number;

		net_port = net_port_from_gptp(port);
		desc->u.gptp_port_params.port_id = net_port_to_logical(net_port);

		desc->u.gptp_port_params.as_capable = port->params.as_capable;

		u_scaled_ns_to_u64(&pdelay, get_mean_link_delay(port));
		desc->u.gptp_port_params.pdelay = (u32)pdelay;

		rc = ipc_tx(ipc, desc);
		if (rc < 0) {
			if (rc != -IPC_TX_ERR_NO_READER)
				os_log(LOG_ERR, "ipc_tx() failed\n");

			ipc_free(ipc, desc);
		}
	} else
		os_log(LOG_ERR, "ipc_alloc() failed\n");
}

static void gptp_ipc_managed_get(struct gptp_ctx *gptp, struct ipc_tx *ipc, unsigned int ipc_dst, uint8_t *in, uint8_t *in_end)
{
	struct ipc_desc *desc;
	int rc;

	desc = ipc_alloc(ipc, GENAVB_MAX_MANAGED_SIZE);
	if (desc) {

		desc->len = gptp_managed_objects_get(&gptp->module, in, in_end, desc->u.data, desc->u.data + GENAVB_MAX_MANAGED_SIZE);

		desc->dst = ipc_dst;
		desc->type = GENAVB_MSG_MANAGED_GET_RESPONSE;

		rc = ipc_tx(ipc, desc);
		if (rc < 0) {
			if (rc != -IPC_TX_ERR_NO_READER)
				os_log(LOG_ERR, "ipc_tx() failed\n");

			ipc_free(ipc, desc);
		}
	} else
		os_log(LOG_ERR, "ipc_alloc() failed\n");
}

static void gptp_ipc_managed_set(struct gptp_ctx *gptp, struct ipc_tx *ipc, unsigned int ipc_dst, uint8_t *in, uint8_t *in_end)
{
	struct ipc_desc *desc;
	int rc;

	desc = ipc_alloc(ipc, GENAVB_MAX_MANAGED_SIZE);
	if (desc) {

		desc->len = gptp_managed_objects_set(&gptp->module, in, in_end, desc->u.data, desc->u.data + GENAVB_MAX_MANAGED_SIZE);

		desc->dst = ipc_dst;
		desc->type = GENAVB_MSG_MANAGED_SET_RESPONSE;

		rc = ipc_tx(ipc, desc);
		if (rc < 0) {
			if (rc != -IPC_TX_ERR_NO_READER)
				os_log(LOG_ERR, "ipc_tx() failed\n");

			ipc_free(ipc, desc);
		}
	} else
		os_log(LOG_ERR, "ipc_alloc() failed\n");
}

static void gptp_ipc_error_response(struct gptp_ctx *gptp, struct ipc_tx *ipc, unsigned int ipc_dst, u32 type, u32 len, u32 status)
{
	struct ipc_desc *desc;
	struct genavb_msg_error_response *error;
	int rc;

	desc = ipc_alloc(ipc, sizeof(struct ipc_error_response));
	if (desc) {
		desc->dst = ipc_dst;
		desc->type = GENAVB_MSG_ERROR_RESPONSE;
		desc->len = sizeof(struct ipc_error_response);
		desc->flags = 0;

		error = &desc->u.error;

		error->type = type;
		error->len = len;
		error->status = status;

		rc = ipc_tx(ipc, desc);
		if (rc < 0) {
			if (rc != -IPC_TX_ERR_NO_READER)
				os_log(LOG_ERR, "ipc_tx() failed\n");

			ipc_free(ipc, desc);
		}
	} else
		os_log(LOG_ERR, "ipc_alloc() failed\n");
}

static void gptp_ipc_rx_media_stack(struct ipc_rx const *rx, struct ipc_desc *desc)
{
	struct gptp_ctx *gptp = container_of(rx, struct gptp_ctx, ipc_rx);
	struct gptp_instance *instance;
	struct ipc_tx *ipc_tx;

	if (desc->flags & IPC_FLAGS_AVB_MSG_SYNC)
		ipc_tx = &gptp->ipc_tx_sync;
	else
		ipc_tx = &gptp->ipc_tx;

	switch (desc->type) {
	case GENAVB_MSG_GM_GET_STATUS:
		os_log(LOG_INFO, "GENAVB_MSG_GM_GET_STATUS\n");
		instance = instance_from_domain(gptp, desc->u.gm_get_status.domain);
		if (instance)
			gptp_ipc_gm_status(instance, ipc_tx, desc->src);
		else
			//FIXME log per time-aware system error counter
			gptp_ipc_error_response(gptp, ipc_tx, desc->src, desc->type, desc->len, GENAVB_ERR_PTP_DOMAIN_INVALID);

		break;

	case GENAVB_MSG_MANAGED_SET:
		os_log(LOG_DEBUG, "GENAVB_MSG_MANAGED_SET\n");
		gptp_ipc_managed_set(gptp, ipc_tx, desc->src, desc->u.data, desc->u.data + desc->len);
		break;

	case GENAVB_MSG_MANAGED_GET:
		os_log(LOG_DEBUG, "GENAVB_MSG_MANAGED_GET\n");
		gptp_ipc_managed_get(gptp, ipc_tx, desc->src, desc->u.data, desc->u.data + desc->len);
		break;

	default:
		gptp_ipc_error_response(gptp, ipc_tx, desc->src, desc->type, desc->len, GENAVB_ERR_CTRL_INVALID);
		break;
	}

	ipc_free(rx, desc);
}

void gptp_gm_indication(struct gptp_instance *instance, struct ptp_priority_vector *gm_vector)
{
	struct gptp_ctx *gptp = instance->gptp;

	/* send indication only if GM has changed */
	if (!os_memcmp(&instance->gm_info.vector.u.s.root_system_identity.u.s.clock_identity, &gm_vector->u.s.root_system_identity.u.s.clock_identity, sizeof(struct ptp_clock_identity)))
		return;

	dump_priority_vector(gm_vector, instance->index, instance->domain.domain_number, "grand master", LOG_INFO);

	os_memcpy(&instance->gm_info.vector, gm_vector, sizeof(struct ptp_priority_vector));

	os_memcpy(&instance->cfg.gm_id, &gm_vector->u.s.root_system_identity.u.s.clock_identity, sizeof(struct ptp_clock_identity));

	instance->gm_info.is_grandmaster = instance->is_grandmaster;
	instance->gm_info.domain = instance->domain.domain_number;

	if (instance->is_grandmaster)
		os_log(LOG_INFO, "domain(%u, %u) acting as GRAND MASTER\n", instance->index, instance->domain.domain_number);
	else
		os_log(LOG_INFO, "domain(%u, %u) GRAND MASTER is %"PRIx64"\n", instance->index, instance->domain.domain_number, htonll(instance->cfg.gm_id));

	gptp_ipc_gm_status(instance, &gptp->ipc_tx, IPC_DST_ALL);

	if (gptp->gm_indication)
		gptp->gm_indication(&instance->gm_info);
}

void gptp_sync_indication(struct gptp_port *port)
{
	struct gptp_instance *instance = port->instance;
	struct gptp_ctx *gptp = instance->gptp;
	u64 pdelayreq_interval;
	u64 sync_receipt_timeout_time_interval;
	struct ptp_md_entity *md;

	if (gptp->sync_indication)
		gptp->sync_indication(&port->sync_info);

	/* if  synchronization state is reached then we can slow down peer master link interval for sync and pdelay transmit
	and also ourself */
	if (port->sync_info.state == SYNC_STATE_SYNCHRONIZED) {
		/* Avnu AutomotiveCDSFunctionalSpecs_1.4 - 6.2.3.1 / 6.2.3.2 */
		md_link_delay_sync_transmit_sm(port, LINK_TRANSMIT_SM_EVENT_OPER);

		/* since we are changing the peer intervals for sync we don't want to trigger timeout
		on previous (likely shorters) interval values, so just restart pending timer with new timeout */

		/*
		802.1.AS - 10.5.4.3.7 timeSyncInterval
		When a signaling message that contains this TLV is sent by a port, the value of syncReceiptTimeoutTimeInterval for that port
		shall be set equal to syncReceiptTimeout multiplied by the value of the interval, in seconds, reflected by timeSyncInterval.
		*/
		sync_receipt_timeout_time_interval = (u64)port->sync_receipt_timeout * log_to_ns(port->cfg.operLogSyncInterval);
		timer_stop(&port->sync_receive_timeout_timer);
		timer_start(&port->sync_receive_timeout_timer, sync_receipt_timeout_time_interval / NS_PER_MS);

		/*
		Avnu AutomotiveCDSFunctionalSpecs_1.4 - 6.2.1.5 operLogPdelayReqInterval
		operLogPdelayReqInterval is the operational Pdelay request interval. A device moves to this value on all slave
		ports once the measured values have stabilized.
		*/
		if (port->c)
			md = &port->c->md;
		else
			md = &port->instance->gptp->cmlds.link_ports[port->port_id].c.md;

		if (port->cfg.operLogPdelayReqInterval != md->globals.initialLogPdelayReqInterval) {
			pdelayreq_interval = log_to_ns(port->cfg.operLogPdelayReqInterval);
			u64_to_u_scaled_ns(&md->globals.pdelayReqInterval, pdelayreq_interval);
			os_log(LOG_INFO, "Port(%u): operLogPdelayReqInterval %d (%"PRIu64" ms)\n", port->port_id, port->cfg.operLogPdelayReqInterval, pdelayreq_interval / NS_PER_MS);
		}
	} else {
		/*
		FIXME: what actions should be taken upon transition from SYNCHRONIZED to NOT SYNCHRONIZED ?
		Shall we request the peer to switch back to the 'initial' values ?
		*/
	}
}

int gptp_system_time(struct gptp_ctx *gptp, u64 *ns)
{
	/* System time is not well defined, for now just get monotonic clock */
	return os_clock_gettime64(gptp->clock_monotonic, ns);
}

void gptp_ratio_invalid(struct gptp_port *port)
{
	u32 num_tx_pdelayresp;

	if (port->c)
		num_tx_pdelayresp = port->c->stats.num_tx_pdelayresp;
	else
		num_tx_pdelayresp = port->instance->gptp->cmlds.link_ports[port->port_id].c.stats.num_tx_pdelayresp;

	port->ratio_is_valid = 0;
	port->ratio_last_num_tx_pdelayresp = num_tx_pdelayresp;
	md_link_delay_sync_transmit_sm(port, LINK_TRANSMIT_SM_EVENT_RATIO_NOT_VALID);
}


void gptp_ratio_valid(struct gptp_port *port)
{
	u32 num_tx_pdelayresp;

	if (port->c)
		num_tx_pdelayresp = port->c->stats.num_tx_pdelayresp;
	else
		num_tx_pdelayresp = port->instance->gptp->cmlds.link_ports[port->port_id].c.stats.num_tx_pdelayresp;

	if ((num_tx_pdelayresp - port->ratio_last_num_tx_pdelayresp) >= 2) {
		port->ratio_is_valid = 1;
		md_link_delay_sync_transmit_sm(port, LINK_TRANSMIT_SM_EVENT_RATIO_VALID);
	}
}

static void gptp_dump_header(struct ptp_hdr *hdr)
{
	u64 cid = get_64(hdr->source_port_id.clock_identity);

	os_log(LOG_DEBUG, "msg_type             %s\n", gptp_msgtype2string(hdr->msg_type));
	os_log(LOG_DEBUG, "transport_specific   %u\n", hdr->transport_specific);
	os_log(LOG_DEBUG, "version_ptp          %u\n", hdr->version_ptp);
	os_log(LOG_DEBUG, "minor_version_ptp    %u\n", hdr->minor_version_ptp);
	os_log(LOG_DEBUG, "msg_length           %u\n", ntohs(hdr->msg_length));
	os_log(LOG_DEBUG, "domain_number        %u\n", hdr->domain_number);
	os_log(LOG_DEBUG, "flags                0x%02x\n", ntohs(hdr->flags));
	os_log(LOG_DEBUG, "correction_field     %"PRId64"\n", hdr->correction_field);
	os_log(LOG_DEBUG, "clock_identity       0x%016"PRIx64"\n", ntohll(cid));
	os_log(LOG_DEBUG, "port_number          %u\n", ntohs(hdr->source_port_id.port_number));
	os_log(LOG_DEBUG, "sequence_id          %u\n", ntohs(hdr->sequence_id));
	os_log(LOG_DEBUG, "control              %u\n", hdr->control);
	os_log(LOG_DEBUG, "log_msg_interval     %d\n\n", hdr->log_msg_interval);
}


static void gptp_rsync_common_header(struct gptp_port *port, void *msg)
{
	struct ptp_hdr *header = (struct ptp_hdr *)(msg);

	os_memset(header, 0, sizeof(struct ptp_hdr));

	header->transport_specific = PTP_DOMAIN_MAJOR_SDOID;
	header->version_ptp = PTP_VERSION;
	header->source_port_id.port_number = htons(get_port_identity_number(port));
	copy_64(header->source_port_id.clock_identity, get_port_identity_clock_id(port));
	header->domain_number = port->instance->domain.domain_number;
	header->correction_field = htonll(0);
}

static void gptp_rsync_send_sync(struct gptp_port *port)
{
	struct ptp_sync_pdu *msg = &port->sync_tx;

	os_log(LOG_DEBUG, "Port(%u)\n", port->port_id);

	gptp_rsync_common_header(port, msg);

	msg->header.msg_type = PTP_MSG_TYPE_SYNC;
	msg->header.msg_length = htons(sizeof(struct ptp_sync_pdu));
	msg->header.flags = htons((PTP_FLAG_TWO_STEP << 8));
	msg->header.sequence_id = htons(port->sync_tx_seqnum);
	msg->header.control = PTP_CONTROL_SYNC;
	msg->header.log_msg_interval = port->params.current_log_sync_interval;
	if (port->instance->gptp->force_2011)
		msg->header.flags |= htons(PTP_FLAG_PTP_TIMESCALE);
	else
		msg->header.minor_version_ptp = PTP_MINOR_VERSION;

	os_memset(&msg->timestamp, 0, sizeof(struct ptp_timestamp));

	gptp_net_tx(net_port_from_gptp(port), msg, sizeof(struct ptp_sync_pdu), 1, port->instance->domain.domain_number, PTP_DOMAIN_MAJOR_SDOID);
}

static void gptp_rsync_send_follow_up(struct gptp_port *port)
{
	struct ptp_follow_up_pdu *msg = &port->follow_up_tx;
	struct gptp_instance *instance = port->instance;
	u64 rsync_slave_time_ns;

	os_log(LOG_DEBUG, "Port(%u) domain(%u, %u): ts %"PRIu64" rsync_running %d\n", port->port_id,
		port->instance->index, port->instance->domain.domain_number,
		port->sync_tx_ts, port->rsync_running);

	gptp_rsync_common_header(port, msg);

	msg->header.msg_type = PTP_MSG_TYPE_FOLLOW_UP;
	msg->header.msg_length = htons(sizeof(struct ptp_follow_up_pdu));
	msg->header.sequence_id = htons(port->sync_tx_seqnum);
	msg->header.log_msg_interval = 0;
	if (port->instance->gptp->force_2011) {
		msg->header.flags |= htons(PTP_FLAG_PTP_TIMESCALE);
		msg->header.control = PTP_CONTROL_FOLLOW_UP_2011;
	} else {
		msg->header.minor_version_ptp = PTP_MINOR_VERSION;
		msg->header.control = PTP_CONTROL_FOLLOW_UP;
	}

	os_clock_convert(instance->gptp->clock_local, port->sync_tx_ts, instance->clock_target, &rsync_slave_time_ns);

	u64_to_pdu_ptp_timestamp(&msg->precise_origin_timestamp, rsync_slave_time_ns);

	os_log(LOG_DEBUG, "Port(%u) domain(%u, %u): RSYNC sent - slave ptp time %"PRIu64" ns (%"PRIu64" ns)\n", port->port_id,
		port->instance->index, port->instance->domain.domain_number,
		rsync_slave_time_ns, port->sync_tx_ts);

	os_memset(&msg->tlv, 0, sizeof(struct ptp_follow_up_tlv));
	/* 11.4.4.3 */
	msg->tlv.tlv_type = htons(PTP_TLV_TYPE_ORGANIZATION_EXTENSION);
	msg->tlv.length_field = htons(28);
	msg->tlv.organization_id[0] = 0x00;
	msg->tlv.organization_id[1] = 0x80;
	msg->tlv.organization_id[2] = 0xC2;
	msg->tlv.organization_sub_type[0] = 0x00;
	msg->tlv.organization_sub_type[1] = 0x00;
	msg->tlv.organization_sub_type[2] = 0x01;

	gptp_net_tx(net_port_from_gptp(port), msg, sizeof(struct ptp_follow_up_pdu), 0, port->instance->domain.domain_number, PTP_DOMAIN_MAJOR_SDOID);

	/* schedule next transmit */
	if (port->rsync_running)
		timer_start(&port->rsync_timer, instance->gptp->cfg.rsync_interval);
}

static void gptp_rsync_timer_handler(void *data)
{
	struct gptp_port *port = (struct gptp_port *)data;

	os_log(LOG_DEBUG, "Port(%u) domain(%u, %u):: RSYNC sent\n", port->port_id, port->instance->index, port->instance->domain.domain_number);

	gptp_rsync_send_sync(port);
}

/** Transmitted packet timestamp handler. Called by platform specific code upon tx timestamp notification
* \return	none
* \param tx	pointer to the network transmit context
* \param ts	64bit timestamp in ns
* \param ts_info	32bits timestamp information (bit0-7: ptp message type, bit8-15 ptp domain, bit16-31: timestamp identifier)
*/
static void cmlds_hwts_handler(struct gptp_net_port *net_port, uint64_t ts, u8 type)
{
	struct gptp_ctx *gptp = container_of(net_port, struct gptp_ctx, net_ports[net_port->port_id]);
	struct gptp_port_common *c;

	os_log(LOG_DEBUG, "Port(%d) CMLDS: ts %"PRIu64", type %u %s\n", net_port->port_id, ts, type, gptp_msgtype2string(type));

	c = get_cmlds_port_common(gptp, net_port->port_id);

	switch (type) {
	case PTP_MSG_TYPE_PDELAY_REQ:
		c->md.pdelay_req_sm.req_tx_ts = ts;
		c->md.pdelay_req_sm.rcvdMDTimestampReceive = true;
		md_pdelay_req_sm(c, PDELAY_REQ_SM_EVENT_RUN);
		break;

	case PTP_MSG_TYPE_PDELAY_RESP:
		c->md.pdelay_resp_sm.resp_tx_ts = ts;
		c->md.pdelay_resp_sm.rcvdMDTimestampReceive = true;
		md_pdelay_resp_sm(c, PDELAY_RESP_SM_EVENT_RUN);
		break;

	default:
		os_log(LOG_ERR, "Port(%u): Unknown message type (%u)\n", net_port->port_id, type);
		break;
	}
}

/** Transmitted packet timestamp handler. Called by platform specific code upon tx timestamp notification
* \return	none
* \param tx	pointer to the network transmit context
* \param ts	64bit timestamp in ns
* \param ts_info	32bits timestamp information (bit0-7: ptp message type, bit8-15 ptp domain, bit16-31: timestamp identifier)
*/
static void gptp_instance_hwts_handler(struct gptp_net_port *net_port, uint64_t ts, u8 type, u8 domain_number)
{
	struct gptp_ctx *gptp = container_of(net_port, struct gptp_ctx, net_ports[net_port->port_id]);
	struct gptp_instance *instance;
	struct gptp_port *port;

	instance = instance_from_domain(gptp, domain_number);
	if (!instance) {
		os_log(LOG_ERR, " Unknown domain(%u) for message type (%u)\n", domain_number, type);
		net_port->stats.num_err_domain_unknown++;
		goto err;
	}

	os_log(LOG_DEBUG, "Port(%d) domain(%u, %u): ts %"PRIu64", type %u %s\n", net_port->port_id, instance->index, instance->domain.domain_number, ts, type, gptp_msgtype2string(type));

	port = net_port_to_gptp(net_port, instance->index);
	if (!port) {
		os_log(LOG_ERR, "Can not retrieve gptp_port from net_port(%d) \n", net_port->port_id);
		goto err;
	}

	switch (type) {
	case PTP_MSG_TYPE_PDELAY_REQ:
		port->c->md.pdelay_req_sm.req_tx_ts = ts;
		port->c->md.pdelay_req_sm.rcvdMDTimestampReceive = true;
		md_pdelay_req_sm(port->c, PDELAY_REQ_SM_EVENT_RUN);
		break;

	case PTP_MSG_TYPE_PDELAY_RESP:
		port->c->md.pdelay_resp_sm.resp_tx_ts = ts;
		port->c->md.pdelay_resp_sm.rcvdMDTimestampReceive = true;
		md_pdelay_resp_sm(port->c, PDELAY_RESP_SM_EVENT_RUN);
		break;

	case PTP_MSG_TYPE_SYNC:
		port->sync_tx_ts = ts;
		port->md.sync_send_sm.rcvdMDTimestampReceive = true;
		port->md.sync_send_sm.rcvdMDTimestampReceivePtr = (struct md_timestamp_receive *)&port->sync_tx_ts;

		if (gptp->cfg.rsync && !instance->is_grandmaster)
			gptp_rsync_send_follow_up(port); /* not really acting as a master, handling sync ts for rsync feature */
		else
			md_sync_send_sm(port, SYNC_SND_SM_EVENT_RUN);

		break;

	default:
		os_log(LOG_ERR, "Port(%u): Unknown message type (%u)\n", net_port->port_id, type);
		break;
	}

err:
	return;
}


/** Transmitted packet timestamp handler. Called by platform specific code upon tx timestamp notification
* \return	none
* \param tx	pointer to the network transmit context
* \param ts	64bit timestamp in ns
* \param ts_info	32bits timestamp information (bit0-7: ptp message type, bit8-15 ptp domain, bit16-31: timestamp identifier)
*/
static void gptp_hwts_handler(struct net_tx *tx, uint64_t ts, unsigned int ts_info)
{
	struct gptp_net_port *net_port = container_of(tx, struct gptp_net_port, net_tx);
	u8 type = gptp_ts_info_msgtype(ts_info);
	u8 domain_number = gptp_ts_info_domain(ts_info);
	u8 sdoid = gptp_ts_info_sdoid(ts_info);

	net_port->stats.num_hwts_handler++;

	/* Compensate transmit timetamp */
	ts += net_port->tx_delay_compensation;

	if (sdoid == PTP_CMLDS_MAJOR_SDOID) {
		if (domain_number == PTP_DOMAIN_0)
			cmlds_hwts_handler(net_port, ts, type);
		else
			os_log(LOG_ERR, "Domain not supported for CMLDS\n");
	} else {
		gptp_instance_hwts_handler(net_port, ts, type, domain_number);
	}
}


static int gptp_compute_clock_identity(struct gptp_ctx *gptp, u8 *clock_identity)
{
	u8 bridge_mac[6];
	struct gptp_net_port *net_port = &gptp->net_ports[0];

	os_memset(clock_identity, 0, 8);

	/* 802.1Q-2014 - 8.13.8, use port 1 mac address for the bridge mac address */
	if (net_get_local_addr(net_port_to_logical(net_port), bridge_mac) < 0)
		goto err;

	/*
	* Per 802.1AS - 8.5.2.2
	*/
	net_eui64_from_mac(clock_identity, bridge_mac, 0);

	return 0;
err:
	return -1;
}

void gptp_instance_priority_vector_update(void *data)
{
	struct gptp_instance *instance = data;

	/* construct this time aware system priority vector */

	/* 8.6.2.1 priority1
		The value of priority1 shall be 255 for a time-aware system that is not grandmaster-capable.
		The value of priority1 shall be less than 255 for a time-aware system that is grandmaster-capable
	*/

	/* 8.6.2.2 clockClass
		a) If the Default Parameter Data Set member gmCapable is TRUE, then
			1) clockClass is set to the value that reflects the combination of the LocalClock and ClockSource entities; else
			2) if the value that reflects the LocalClock and ClockSource entities is not specified or not known,clockClass is set to 248;
		b) If the Default Parameter Data Set member gmCapable is FALSE (see 8.6.2.1), clockClass is set to 255.
	*/

	if (instance->gmCapable) {
		if (instance->params.system_priority.u.s.root_system_identity.u.s.priority_1 == 255) {
			instance->params.system_priority.u.s.root_system_identity.u.s.priority_1 = 254;
			os_log(LOG_ERR, "gptp(%p) priority1=255 while gmCapable=TRUE. Forcing priority1=254\n", instance);
		}

		if (instance->params.system_priority.u.s.root_system_identity.u.s.clock_quality.clock_class == 255) {
			instance->params.system_priority.u.s.root_system_identity.u.s.clock_quality.clock_class = 248;
			os_log(LOG_ERR, "gptp(%p) clock_class=255 while gmCapable=TRUE. Forcing clock_class=248\n", instance);
		}
	} else {
		instance->params.system_priority.u.s.root_system_identity.u.s.priority_1 = 255;
		instance->params.system_priority.u.s.root_system_identity.u.s.clock_quality.clock_class = 255;
	}

	os_memcpy(&instance->params.system_priority.u.s.root_system_identity.u.s.clock_identity, &instance->params.this_clock, sizeof(struct ptp_clock_identity));
	os_memcpy(&instance->params.system_priority.u.s.source_port_identity.clock_identity, &instance->params.this_clock, sizeof(struct ptp_clock_identity));

	dump_priority_vector(&instance->params.system_priority, instance->index, instance->domain.domain_number, "system priority vector", LOG_INFO);
}


/** Transmit PTPv2 packets to the network. Allocate packet, add ethernet header, copy gptp PDU
 * \return	0 on success, negative value on failure
 * \param port	pointer to the gptp port context
 * \param msg	pointer to the gptp PDU to transmit
 * \param msg_len	length in bytes of the gptp PDU to transmit
 * \param ts_required	Boolean, specifies if a trasmint hw timestamp is required for this packet
 */
int gptp_net_tx(struct gptp_net_port *net_port, void *msg, int msg_len, unsigned char ts_required, u8 domain_number, u8 sdoid)
{
	struct ptp_hdr *hdr = msg;
	u8 msg_type = hdr->msg_type;
	unsigned int port_id = net_port->port_id;
	struct net_tx_desc *desc;
	void *pdu;

	os_log(LOG_DEBUG, "Port(%u): %s len %d sdoid %u\n", port_id, gptp_msgtype2string(msg_type), msg_len, sdoid);

	gptp_dump_header(hdr);

	desc = net_tx_alloc(&net_port->net_tx, sizeof(struct eth_hdr) + msg_len);
	if (!desc) {
		net_port->stats.num_tx_err_alloc++;
		goto err_alloc;
	}

	if (ts_required) {
		desc->priv = gptp_ts_info(sdoid, domain_number, msg_type);
		desc->flags = NET_TX_FLAGS_HW_TS;
		net_port->stats.num_hwts_request++;
		os_log(LOG_DEBUG,"Port(%u): ts_required - desc->priv 0x%08x\n", port_id, desc->priv);
	}

	pdu = NET_DATA_START(desc);
	desc->len += net_add_eth_header(pdu, ptp_dst_mac, ETHERTYPE_PTP);

	os_memcpy(((char *)pdu + desc->len), msg, msg_len);
	desc->len += msg_len;

	if (net_tx(&net_port->net_tx, desc) < 0) {
		os_log(LOG_ERR,"Port(%u): cannot transmit packet\n", port_id);
		net_port->stats.num_tx_err++;
		goto err_tx;
	}

	net_port->stats.num_tx_pkts++;

	return 0;

err_tx:
	net_tx_free(desc);

err_alloc:
	return -1;
}


static void gptp_announce_transmit_timer_handler(void *data)
{
	struct gptp_port *port = (struct gptp_port *)data;

	os_log(LOG_DEBUG, "Port(%u)\n", port->port_id);

	port_announce_transmit_sm(port, PORT_ANNOUNCE_TRANSMIT_SM_EVENT_TRANSMIT_INTERVAL);
}


static void gptp_announce_receive_timeout_timer_handler(void *data)
{
	struct gptp_port *port = (struct gptp_port *)data;

	os_log(LOG_DEBUG, "Port(%u)\n", port->port_id);

	port->stats.num_rx_announce_timeout++;

	port_announce_info_sm(port, PORT_ANNOUNCE_INFO_SM_EVENT_ANNOUNCE_TIMEOUT);
}

static void gptp_sync_receive_timeout_timer_handler(void *data)
{
	struct gptp_port *port = (struct gptp_port *)data;

	os_log(LOG_DEBUG, "Port(%u)\n", port->port_id);

	port->stats.num_rx_sync_timeout++;

	port_announce_info_sm(port, PORT_ANNOUNCE_INFO_SM_EVENT_SYNC_TIMEOUT);
}


static void gptp_follow_up_receive_timeout_timer_handler(void *data)
{
	struct gptp_port *port = (struct gptp_port *)data;

	os_log(LOG_DEBUG, "Port(%u)\n", port->port_id);

	port->stats.num_rx_fup_timeout++;

	md_sync_rcv_sm(port, SYNC_RCV_SM_EVENT_FUP_TIMEOUT);
}

static void gptp_capable_receive_timeout_timer_handler (void *data)
{
	struct gptp_port *port = (struct gptp_port *)data;

	port_gptp_capable_receive_sm(port, GPTP_CAPABLE_RECEIVE_SM_EVENT_TIMEOUT);
}

static void gptp_capable_transmit_timeout_timer_handler (void *data)
{
	struct gptp_port *port = (struct gptp_port *)data;

	port_gptp_capable_transmit_sm(port, GPTP_CAPABLE_TRANSMIT_SM_EVENT_INTERVAL);
}

static void gptp_clock_master_sync_send_timer_handler (void *data)
{
	struct gptp_instance *instance = (struct gptp_instance *)data;

	clock_master_sync_send_sm(instance, CLOCK_MASTER_SYNC_SEND_SM_EVENT_INTERVAL);
}


static void gptp_port_sync_sync_transmit_timer_handler(void *data)
{
	struct gptp_port *port = (struct gptp_port *)data;

	os_log(LOG_DEBUG, "Port(%u) domain(%u, %u)\n", port->port_id, port->instance->index, port->instance->domain.domain_number);

	port_sync_sync_send_sm(port, PORT_SYNC_SYNC_SEND_SM_EVENT_RUN);
}

static void gptp_port_sync_sync_receipt_timeout_handler(void *data)
{
	struct gptp_port *port = (struct gptp_port *)data;

	os_log(LOG_DEBUG, "Port(%u) domain(%u, %u)\n", port->port_id, port->instance->index, port->instance->domain.domain_number);

	port_sync_sync_send_sm(port, PORT_SYNC_SYNC_SEND_SM_EVENT_PSSYNC_TIMEOUT);
}


static void gptp_pdelay_req_timer_handler(void *data)
{
	struct gptp_port_common *port = (struct gptp_port_common *)data;

	os_log(LOG_DEBUG, "Port(%u)\n", port->port_id);

	md_pdelay_req_sm(port, PDELAY_REQ_SM_EVENT_REQ_INTERVAL);
}


static void gptp_handle_sync(struct gptp_instance *instance, struct gptp_port *port, struct ptp_sync_pdu *msg, u64 ts)
{
	struct ptp_instance_md_entity *md = &port->md;

	os_log(LOG_DEBUG, "Port(%u): seq_id=%d is_grandmaster=%u\n", port->port_id, ntohs(msg->header.sequence_id), instance->is_grandmaster);

	port->stats.num_rx_sync++;

	port->sync_rx_ts = ts;

	/* make sure this endpoint when acting as grandmaster (sending SYNC) does not get fooled by
	any reverse sync feature packets */
	if ((instance->is_grandmaster) && (instance->gptp->cfg.rsync))
		return;

	os_memcpy(&port->sync_rx, msg, sizeof(struct ptp_sync_pdu));
	md->sync_rcv_sm.rcvdSyncPtr = &port->sync_rx;
	md->sync_rcv_sm.rcvdSync = true;

	md_sync_rcv_sm(port, SYNC_RCV_SM_EVENT_RUN);
}


static void gptp_handle_follow_up(struct gptp_instance *instance, struct gptp_port *port, struct ptp_follow_up_pdu *msg)
{
	struct ptp_instance_md_entity *md = &port->md;
	u64 slave_time_ns, master_time_ns;
	u64 pdelay_ns;

	os_log(LOG_DEBUG, "Port(%u): seq_id=%d\n", port->port_id, ntohs(msg->header.sequence_id));

	port->stats.num_rx_fup++;

	/* make sure this endpoint when acting as grandmaster (sending SYNC) does not get fooled by
	any reverse sync feature packets */
	if ((instance->is_grandmaster) && (instance->gptp->cfg.rsync)) {
		slave_time_ns = pdu_ptp_timestamp_to_u64(msg->precise_origin_timestamp);
		os_clock_convert(instance->gptp->clock_local, port->sync_rx_ts, instance->clock_source, &master_time_ns);
		u_scaled_ns_to_u64(&pdelay_ns, get_mean_link_delay(port));
		os_log(LOG_INFO, "Port(%u) domain(%u,%u): RSYNC received - slave origin time %" PRIu64 " ns - GM current time %" PRIu64 " ns - offset %" PRId64 " ns\n",
			port->port_id, port->instance->index, port->instance->domain.domain_number,
			slave_time_ns, master_time_ns,
			(master_time_ns - slave_time_ns) - pdelay_ns);
	} else {
		os_memcpy(&port->follow_up_rx, msg, sizeof(struct ptp_follow_up_pdu));
		md->sync_rcv_sm.rcvdFollowUpPtr = &port->follow_up_rx;
		md->sync_rcv_sm.rcvdFollowUp = true;

		md_sync_rcv_sm(port, SYNC_RCV_SM_EVENT_RUN);
	}

	if ((instance->gptp->cfg.rsync) && (port->sync_info.state == SYNC_STATE_SYNCHRONIZED)) {
		if (!port->rsync_running) {
			port->rsync_running = 1;
			gptp_rsync_send_sync(port); /* initial send of rsync packet */
		}
	} else
		port->rsync_running = 0;
}


static int gptp_validate_sdoid(struct gptp_ctx *gptp, u8 link_id, u8 msg_type, u8 transport_specific, bool *is_cmlds)
{
	if ((transport_specific & 0x0F) > PTP_CMLDS_MAJOR_SDOID)
		goto not_valid;

	*is_cmlds = (transport_specific & PTP_CMLDS_MAJOR_SDOID) ? true : false;

	if ((*is_cmlds) && (msg_type != PTP_MSG_TYPE_SIGNALING) && (msg_type != PTP_MSG_TYPE_PDELAY_REQ) && (msg_type != PTP_MSG_TYPE_PDELAY_RESP) && (msg_type != PTP_MSG_TYPE_PDELAY_RESP_FUP))
		goto not_valid;

	if (!is_cmlds_link_port_enabled(gptp, link_id) && *is_cmlds)
		goto not_valid;

	return 1;

not_valid:
	return 0;
}


static void gptp_handle_pdelay_req(struct gptp_port_common *port, struct ptp_pdelay_req_pdu *msg, u64 ts)
{
	os_log(LOG_DEBUG, "Port(%u): seq_id=%d ts %" PRIu64 "\n", port->port_id, ntohs(msg->header.sequence_id), ts);

	port->stats.num_rx_pdelayreq++;

	port->md.pdelay_resp_sm.req_rx_ts = ts;

	os_memcpy(&port->md.pdelay_resp_sm.req_rx, msg, sizeof(struct ptp_pdelay_req_pdu));
	port->md.pdelay_resp_sm.rcvdPdelayReq = true;

	md_pdelay_resp_sm(port, PDELAY_RESP_SM_EVENT_REQ_RECEIVED);
}


static void gptp_handle_pdelay_resp(struct gptp_port_common *port, struct ptp_pdelay_resp_pdu *msg, u64 ts)
{
	os_log(LOG_DEBUG, "Port(%u): seq_id=%d ts %" PRIu64 " correction %"PRId64"\n", port->port_id, ntohs(msg->header.sequence_id), ts, ntohll(msg->header.correction_field));

	port->stats.num_rx_pdelayresp++;

	port->md.pdelay_req_sm.resp_rx_ts = ts;

	/* according to the pdelay req state machine description (802.1AS-11.2) pdelay response payload
	is also required upon timestamp event handling, so just store it */
	os_memcpy(&port->md.pdelay_req_sm.resp_rx, msg, sizeof(struct ptp_pdelay_resp_pdu));
	port->md.pdelay_req_sm.rcvdPdelayRespPtr = &port->md.pdelay_req_sm.resp_rx;
	port->md.pdelay_req_sm.rcvdPdelayResp = true;

	md_pdelay_req_sm(port, PDELAY_REQ_SM_EVENT_RUN);
}


static void gptp_handle_pdelay_resp_fup(struct gptp_port_common *port, struct ptp_pdelay_resp_follow_up_pdu *msg)
{
	os_log(LOG_DEBUG, "Port(%u): seq_id=%d sec_msb %u sec_lsb %u ns %u correction %"PRId64"\n", port->port_id, ntohs(msg->header.sequence_id), msg->response_origin_timestamp.seconds_msb, msg->response_origin_timestamp.seconds_lsb, msg->response_origin_timestamp.nanoseconds, ntohll(msg->header.correction_field));

	port->stats.num_rx_pdelayrespfup++;

	os_memcpy(&port->md.pdelay_req_sm.resp_fup_rx, msg, sizeof(struct ptp_pdelay_resp_follow_up_pdu));
	port->md.pdelay_req_sm.rcvdPdelayRespFollowUp = true;
	port->md.pdelay_req_sm.rcvdPdelayRespFollowUpPtr = &port->md.pdelay_req_sm.resp_fup_rx;

	md_pdelay_req_sm(port, PDELAY_REQ_SM_EVENT_RUN);
}

void gptp_dump_announcer_params(struct ptp_announce_pdu *msg)
{
	os_log(LOG_DEBUG, "gm identity %016"PRIx64"\n", get_ntohll(msg->grandmaster_identity.identity));
	os_log(LOG_DEBUG, "priority1 %u\tpriority2 %u\n", msg->grandmaster_priority1,  msg->grandmaster_priority2);
	os_log(LOG_DEBUG, "class %u\taccuracy %u\n", msg->grandmaster_clock_quality.clock_class, msg->grandmaster_clock_quality.clock_accuracy);
	os_log(LOG_DEBUG, "steps_removed %u\n", ntohs(msg->steps_removed));
}


static void gptp_handle_announce(struct gptp_instance *instance, struct gptp_port *port, struct ptp_announce_pdu *msg, u16 len)
{
	os_log(LOG_DEBUG, "Port(%u): seq_id=%d\n", port->port_id, ntohs(msg->header.sequence_id));

	port->stats.num_rx_announce++;

	/* Avnu AutoCDSFunctionalSpecs v1.1 - section 6.3
	* BMCA shall not execute. Conceptually, each device should be preconfigured with the result that BMCA
	* would have arrived at if it had been previously running and had reached a quiescent operating state.
	* Announce messages are neither required nor expected in an automotive network since BMCA is not being
	* executed. With no BMCA there will always be exactly one device in the network that is configured as GM
	* and no other device will ever become the GM.
	*/
	if (!port->gm_id_static) {
		gptp_dump_announcer_params(msg);

		/* Note: Per the specifications the announce message are passed from the media dependant layer to
		the port state machine whithout being processed at all by any of the MD state machines or functions.
		So to simplify the flow here we are passing the announce message directly to the Port state machine
		*/

		/* An announce message does not have fixed length (ref. path trace tlv) so we are checking we can handle the
		received PDU */
		if (len <= sizeof (struct ptp_announce_pdu)) {
			os_memcpy(&port->announce_rx, msg, len);
			port->port_sync.announce_receive_sm.rcvd_announce_ptr = &port->announce_rx;
			port->port_sync.announce_receive_sm.rcvd_announce_par = true;

			port_announce_rcv_sm(port);
		} else
			port->stats.num_rx_announce_dropped++;
	} else
		port->stats.num_rx_announce_dropped++;
}

static bool gptp_organization_id_equal(struct ptp_signaling_pdu *msg, u8 byte1, u8 byte2, u8 byte3)
{
	if ((msg->organization_id[0] == byte1) && (msg->organization_id[1] == byte2) && (msg->organization_id[2] == byte3))
		return true;

	return false;
}

static bool gptp_organization_subtype_equal(struct ptp_signaling_pdu *msg, u8 byte1, u8 byte2, u8 byte3)
{
	if ((msg->organization_sub_type[0] == byte1) && (msg->organization_sub_type[1] == byte2) && (msg->organization_sub_type[2] == byte3))
		return true;

	return false;
}

static void cmlds_handle_signaling(struct gptp_ctx *gptp, u16 link_id, struct ptp_signaling_pdu *msg)
{
	struct gptp_port_common *c;

	os_log(LOG_DEBUG, "Port(%u): seq_id=%d\n", link_id, ntohs(msg->header.sequence_id));

	/* checking organization id for interval requests and gptp capable signaling messages only */
	if (!gptp_organization_id_equal(msg, 0x00, 0x80, 0xC2))
		return;

	c = get_cmlds_port_common(gptp, link_id);

	switch (ntohs(msg->tlv_type)) {
	case PTP_TLV_TYPE_ORGANIZATION_EXTENSION:
		if (gptp_organization_subtype_equal(msg, 0, 0, PTP_TLV_SUBTYPE_INTERVAL_REQUEST)) {
			os_memcpy(&c->signaling_rx, msg, sizeof(struct ptp_signaling_pdu));

			/* IEEE 802.1AS-2020 section 10.6.4.3 */
			c->md.link_interval_sm.rcvd_signaling_ptr_ldis = &c->signaling_rx;
			c->md.link_interval_sm.rcvd_signaling_msg1 = true;
			md_link_delay_interval_setting_sm(c);
		}

		break;

	default:
		break;
	}
}


static void gptp_instance_handle_signaling(struct gptp_instance *instance, struct gptp_port *port, struct ptp_signaling_pdu *msg)
{
	os_log(LOG_DEBUG, "Port(%u) domain(%u, %u): seq_id=%d\n", port->port_id, instance->index, instance->domain.domain_number, ntohs(msg->header.sequence_id));

	port->stats.num_rx_sig++;

	/* checking organization id for interval requests and gptp capable signaling messages only */
	if (!gptp_organization_id_equal(msg, 0x00, 0x80, 0xC2))
		return;

	switch (ntohs(msg->tlv_type)) {
	case PTP_TLV_TYPE_ORGANIZATION_EXTENSION:
		if (gptp_organization_subtype_equal(msg, 0, 0, PTP_TLV_SUBTYPE_INTERVAL_REQUEST)) {
			os_memcpy(&port->signaling_rx, msg, sizeof(struct ptp_signaling_pdu));

			/*
			AutoCDSFunctionalSpecs-1_4 - 6.2.4
			When processing a received gPTP signaling Message on a master role port, and AED shall update the syncInterval
			for the receiving port accordingly and shall ignore values for linkDelayInterval and announceInterval
			*/
			if ((instance->gptp->cfg.profile != CFG_GPTP_PROFILE_AUTOMOTIVE) || (instance->params.selected_role[get_port_identity_number(port)] == SLAVE_PORT)) {
				/* IEEE 802.1AS-2020 section 10.6.4.3 */
				/* FIXME signaling interval setting only support for DOMAIN0 */
				if (instance->domain.domain_number == PTP_DOMAIN_0) {
					port->c->md.link_interval_sm.rcvd_signaling_ptr_ldis = &port->signaling_rx;
					port->c->md.link_interval_sm.rcvd_signaling_msg1 = true;
					md_link_delay_interval_setting_sm(port->c);
				}

				port->announce_interval_sm.rcvd_signaling_ptr_ais = &port->signaling_rx;
				port->announce_interval_sm.rcvd_signaling_msg2 = true;
				port_announce_interval_setting_sm(port);
			}

			port->sync_interval_sm.rcvd_signaling_ptr_sis = &port->signaling_rx;
			port->sync_interval_sm.rcvd_signaling_msg3 = true;
			port_sync_interval_setting_sm(port);
		}

		break;

	case PTP_TLV_TYPE_ORGANIZATION_EXTENSION_DO_NOT_PROPAGATE:
		/* Do not trigger gPTP Capable state machines with received signaling messages if the IEEE 802.1AS-2011 mode only is enabled */
		if (instance->gptp->force_2011)
			break;

		if (gptp_organization_subtype_equal(msg, 0, 0, PTP_TLV_SUBTYPE_GPTP_CAPABLE_MESSAGE)) {
			os_memcpy(&port->signaling_rx, msg, sizeof(struct ptp_signaling_pdu));

			/* IEEE 802.1AS-2020 section 10.6.4.4 */
			port->gptp_capable_receive_sm.rcvd_signaling_msg_ptr = &port->signaling_rx;
			port->gptp_capable_receive_sm.rcvd_gptp_capable_tlv = true;
			port_gptp_capable_receive_sm(port, GPTP_CAPABLE_RECEIVE_SM_EVENT_RUN);
		} else if (gptp_organization_subtype_equal(msg, 0, 0, PTP_TLV_SUBTYPE_GPTP_CAPABLE_INTERVAL)) {
			os_memcpy(&port->signaling_rx, msg, sizeof(struct ptp_signaling_pdu));

			/* IEEE 802.1AS-2020 section 10.6.4.5 */
			port->gptp_capable_interval_sm.rcvd_signaling_ptr_gis = &port->signaling_rx;
			port->gptp_capable_interval_sm.rcvd_signaling_msg4 = true;
			port_gptp_capable_interval_setting_sm(port);
		}

		break;

	default:
		break;
	}
}

static int gptp_validate_domain(struct gptp_ctx *gptp, u8 domain_number, u16 msg_type)
{
	/* If 802.1AS-2011 interoperability mode is enabled only domain0 is supported */
	if (gptp->force_2011 && (domain_number != PTP_DOMAIN_0))
		goto not_valid;

	if (domain_number > PTP_DOMAIN_NUMBER_MAX)
		goto not_valid;

	if ((domain_number != PTP_DOMAIN_0) && ((msg_type == PTP_MSG_TYPE_PDELAY_REQ) || (msg_type == PTP_MSG_TYPE_PDELAY_RESP) || (msg_type == PTP_MSG_TYPE_PDELAY_RESP_FUP)))
		goto not_valid;

	return 0;

not_valid:
	return -1;
}


static void cmlds_net_rx(struct gptp_ctx *gptp, struct gptp_net_port *net_port, struct net_rx_desc *desc)
{
	void *data = (char *)desc + desc->l3_offset;
	struct ptp_hdr *header = (struct ptp_hdr *)data;
	struct gptp_port_common *c;

	c = get_cmlds_port_common(gptp, net_port->port_id);

	os_log(LOG_DEBUG, "Port(%u) CMLDS: %s desc port %d, len %d, ts %"PRIu64"\n", c->port_id, gptp_msgtype2string(header->msg_type), desc->port, desc->len, desc->ts64);

	switch (header->msg_type) {
	case PTP_MSG_TYPE_PDELAY_REQ:
		c->stats.peer_clock_id = ntohll(get_64(header->source_port_id.clock_identity));
		gptp_handle_pdelay_req(c, data, desc->ts64);

		/* FIXME code below not supported with CMLDS, need to be revisited */
#if 0
		/* An offset adjustment has been done recently, and signaling message
		has been sent to remote GM to hold ratio and pdelay calculation. Now
		checking if ratio and pdelay calculation can be resumed on GM side
		(enough pdelay request intervals have elapsed since offset adjusment, i.e.
		this slave node synchronization has stabilized) */
		if (!port->ratio_is_valid)
			gptp_ratio_valid(port);
#endif
		break;

	case PTP_MSG_TYPE_PDELAY_RESP:
		gptp_handle_pdelay_resp(c, data, desc->ts64);
		break;

	case PTP_MSG_TYPE_PDELAY_RESP_FUP:
		gptp_handle_pdelay_resp_fup(c, data);
		break;

	case PTP_MSG_TYPE_SIGNALING:
		cmlds_handle_signaling(gptp, c->port_id, data);
		break;

	default:
		os_log(LOG_ERR, "Port(%u): unknown message type(%d)\n", c->port_id, header->msg_type);
		break;
	}
}

static void gptp_instance_net_rx(struct gptp_ctx *gptp, struct gptp_net_port *net_port, struct net_rx_desc *desc)
{
	void *data = (char *)desc + desc->l3_offset;
	struct ptp_hdr *header = (struct ptp_hdr *)data;
	struct gptp_instance *instance;
	struct gptp_port *port;

	instance = instance_from_domain(gptp, header->domain_number);
	if (!instance) {
		net_port->stats.num_err_domain_unknown++;
		return;
	}

	port = net_port_to_gptp(net_port, instance->index);
	if (!port) {
		return;
	}

	os_log(LOG_DEBUG, "Port(%u) domain(%u, %u): %s desc port %d, len %d, ts %"PRIu64"\n", port->port_id, instance->index, header->domain_number, gptp_msgtype2string(header->msg_type), desc->port, desc->len, desc->ts64);

	switch (header->msg_type) {
	case PTP_MSG_TYPE_PDELAY_REQ:
		port->c->stats.peer_clock_id = ntohll(get_64(header->source_port_id.clock_identity));
		gptp_handle_pdelay_req(port->c, data, desc->ts64);

		/* FIXME code below not supported with CMLDS, need to be revisited */
	#if 0
		/* An offset adjustment has been done recently, and signaling message
		has been sent to remote GM to hold ratio and pdelay calculation. Now
		checking if ratio and pdelay calculation can be resumed on GM side
		(enough pdelay request intervals have elapsed since offset adjusment, i.e.
		this slave node synchronization has stabilized) */
		if (!port->ratio_is_valid)
			gptp_ratio_valid(port);
	#endif
		break;

	case PTP_MSG_TYPE_PDELAY_RESP:
		gptp_handle_pdelay_resp(port->c, data, desc->ts64);
		break;

	case PTP_MSG_TYPE_PDELAY_RESP_FUP:
		gptp_handle_pdelay_resp_fup(port->c, data);
		break;

	case PTP_MSG_TYPE_SYNC:
		gptp_handle_sync(instance, port, data, desc->ts64);
		break;

	case PTP_MSG_TYPE_FOLLOW_UP:
		gptp_handle_follow_up(instance, port, data);
		break;

	case PTP_MSG_TYPE_ANNOUNCE:
		gptp_handle_announce(instance, port, data, desc->len);
		break;

	case PTP_MSG_TYPE_SIGNALING:
		gptp_instance_handle_signaling(instance, port, data);
		break;

	default:
		os_log(LOG_ERR, "Port(%u): unknown message type(%d)\n", port->port_id, header->msg_type);
		break;
	}
}


/** Decode and handle PTPv2 packets received from the network
* \return	none
* \param rx	pointer to the network receive context
* \param desc	pointer to the received packet descriptor
*/
static void gptp_net_rx(struct net_rx *rx, struct net_rx_desc *desc)
{
	struct gptp_net_port *net_port = container_of(rx, struct gptp_net_port, net_rx);
	struct gptp_ctx *gptp = container_of(net_port, struct gptp_ctx, net_ports[net_port->port_id]);
	void *data = (char *)desc + desc->l3_offset;
	struct ptp_hdr *header = (struct ptp_hdr *)data;
	bool is_cmlds;

	if (desc->ethertype != ETHERTYPE_PTP) {
		net_port->stats.num_rx_err_etype++;
		goto err;
	}

	if (!gptp_validate_sdoid(gptp, net_port->port_id, header->msg_type, header->transport_specific, &is_cmlds)) {
		net_port->stats.num_rx_err_sdoid++;
		goto err;
	}

	if (gptp_validate_domain(gptp, header->domain_number, header->msg_type) < 0) {
		net_port->stats.num_rx_err_domain++;
		goto err;
	}

	gptp_dump_header(header);

	net_port->stats.num_rx_pkts++;

	/* Compensate receive timetamp */
	desc->ts64 -= net_port->rx_delay_compensation;

	if (is_cmlds)
		cmlds_net_rx(gptp, net_port, desc);
	else
		gptp_instance_net_rx(gptp, net_port, desc);

err:
	net_rx_free(desc);
}


static int gptp_port_common_init_timers(struct gptp_port_common *port)
{
	/* pdelay request transmit timer only required if pdelay 'silent' mode is not used */
	if (port->pdelay_transmit_enabled) {
		port->md.pdelay_req_sm.req_timer.func = gptp_pdelay_req_timer_handler;
		port->md.pdelay_req_sm.req_timer.data = port;
		if (timer_create(port->gptp->timer_ctx, &port->md.pdelay_req_sm.req_timer, TIMER_TYPE_SYS, 0) < 0)
			goto err_pdelay_req;
	}

	return 0;

err_pdelay_req:
	return -1;
}

__init static int gptp_port_init_timers(struct gptp_port *port, unsigned int port_index)
{
	struct gptp_ctx *gptp = port->instance->gptp;

	os_log(LOG_INIT, "Port(%u)\n", port_index);

	/*
	* master only timers (announce transmit)
	*/
	if (!port->gm_id_static) {
		/* announce always sent every announce_interval */
		port->announce_transmit_sm.transmit_timer.func = gptp_announce_transmit_timer_handler;
		port->announce_transmit_sm.transmit_timer.data = port;
		if (timer_create(gptp->timer_ctx, &port->announce_transmit_sm.transmit_timer, TIMER_TYPE_SYS, 0) < 0)
			goto err_announce;
	}

	port->port_sync.sync_send_sm.sync_transmit_timer.func = gptp_port_sync_sync_transmit_timer_handler;
	port->port_sync.sync_send_sm.sync_transmit_timer.data = port;
	if (timer_create(gptp->timer_ctx, &port->port_sync.sync_send_sm.sync_transmit_timer, TIMER_TYPE_SYS, 0) < 0)
		goto err_sync;

	port->port_sync.sync_send_sm.sync_receipt_timeout_timer.func = gptp_port_sync_sync_receipt_timeout_handler;
	port->port_sync.sync_send_sm.sync_receipt_timeout_timer.data = port;
	if (timer_create(gptp->timer_ctx, &port->port_sync.sync_send_sm.sync_receipt_timeout_timer, TIMER_TYPE_SYS, 0) < 0)
		goto err_sync_timeout;

	/*
	* master and slave timers ( sync receive, announce receive)
	*/

	/* sync follow up receipt timeout */
	port->follow_up_receive_timeout_timer.func = gptp_follow_up_receive_timeout_timer_handler;
	port->follow_up_receive_timeout_timer.data = port;
	if (timer_create(gptp->timer_ctx, &port->follow_up_receive_timeout_timer, TIMER_TYPE_SYS, 0) < 0)
		goto err_follow_up;

	/* sync receipt timeout */
	port->sync_receive_timeout_timer.func = gptp_sync_receive_timeout_timer_handler;
	port->sync_receive_timeout_timer.data = port;
	if (timer_create(gptp->timer_ctx, &port->sync_receive_timeout_timer, TIMER_TYPE_SYS, 0) < 0)
		goto err_sync_receive;

	/* announce receipt timeout */
	port->port_sync.announce_receive_sm.timeout_timer.func = gptp_announce_receive_timeout_timer_handler;
	port->port_sync.announce_receive_sm.timeout_timer.data = port;
	if (timer_create(gptp->timer_ctx, &port->port_sync.announce_receive_sm.timeout_timer, TIMER_TYPE_SYS, 0) < 0)
		goto err_announce_receive;

	if (!gptp->force_2011) {
		/* gptp capable receipt timeout */
		port->gptp_capable_receive_sm.timeout_timer.func = gptp_capable_receive_timeout_timer_handler;
		port->gptp_capable_receive_sm.timeout_timer.data = port;
		if (timer_create(gptp->timer_ctx, &port->gptp_capable_receive_sm.timeout_timer, TIMER_TYPE_SYS, 0) < 0)
			goto err_gptp_capable_receive;

		/* gptp capable transmit timeout */
		port->gptp_capable_transmit_sm.timeout_timer.func = gptp_capable_transmit_timeout_timer_handler;
		port->gptp_capable_transmit_sm.timeout_timer.data = port;
		if (timer_create(gptp->timer_ctx, &port->gptp_capable_transmit_sm.timeout_timer, TIMER_TYPE_SYS, 0) < 0)
			goto err_gptp_capable_transmit;
	}

	/* sync reverse transmit timer */
	if (gptp->cfg.rsync) {
		port->rsync_timer.func = gptp_rsync_timer_handler;
		port->rsync_timer.data = port;
		if (timer_create(gptp->timer_ctx, &port->rsync_timer, TIMER_TYPE_SYS, 0) < 0)
			goto err_sync_reverse;
	}

	return 0;

err_sync_reverse:
	if (!gptp->force_2011)
		timer_destroy(&port->gptp_capable_transmit_sm.timeout_timer);

err_gptp_capable_transmit:
	if (!gptp->force_2011)
		timer_destroy(&port->gptp_capable_receive_sm.timeout_timer);

err_gptp_capable_receive:
	timer_destroy(&port->port_sync.announce_receive_sm.timeout_timer);

err_announce_receive:
	timer_destroy(&port->sync_receive_timeout_timer);

err_sync_receive:
	timer_destroy(&port->follow_up_receive_timeout_timer);

err_follow_up:
	timer_destroy(&port->port_sync.sync_send_sm.sync_receipt_timeout_timer);

err_sync_timeout:
	timer_destroy(&port->port_sync.sync_send_sm.sync_transmit_timer);

err_sync:
	if (!port->gm_id_static)
		timer_destroy(&port->announce_transmit_sm.transmit_timer);

err_announce:
	return -1;

}


static void gptp_stats_timer_handler(void *data)
{
	struct gptp_ctx *gptp = (struct gptp_ctx *)data;

	gptp_stats_dump(gptp);

	timer_restart(&gptp->stats_timer, gptp->cfg.statsInterval * MS_PER_S);
}

__init static int gptp_stats_init_timers(struct gptp_ctx *gptp)
{
	if (gptp->cfg.statsInterval) {
		gptp->stats_timer.func = gptp_stats_timer_handler;
		gptp->stats_timer.data = gptp;
		if (timer_create(gptp->timer_ctx, &gptp->stats_timer, TIMER_TYPE_SYS, 0) < 0)
			goto err_stats_timer;

		timer_start(&gptp->stats_timer, gptp->cfg.statsInterval * MS_PER_S);
	}

	return 0;

err_stats_timer:
	return -1;
}

static void gptp_stats_exit_timers(struct gptp_ctx *gptp)
{
	if (gptp->cfg.statsInterval)
		timer_destroy(&gptp->stats_timer);
}


__init static int gptp_instance_init_timers(struct gptp_instance *instance)
{
	struct gptp_ctx *gptp = instance->gptp;

	instance->clock_master_sync_send_timer.func = gptp_clock_master_sync_send_timer_handler;
	instance->clock_master_sync_send_timer.data = instance;
	if (timer_create(gptp->timer_ctx, &instance->clock_master_sync_send_timer, TIMER_TYPE_SYS, 0) < 0)
		goto err_clock_master;

	return 0;

err_clock_master:
	return -1;
}

static void gptp_port_common_exit_timers(struct gptp_port_common *port)
{
	if (port->pdelay_transmit_enabled)
		timer_destroy(&port->md.pdelay_req_sm.req_timer);
}

__exit static void gptp_port_exit_timers(struct gptp_port *port)
{
	timer_destroy(&port->rsync_timer);

	if (!port->instance->gptp->force_2011) {
		timer_destroy(&port->gptp_capable_transmit_sm.timeout_timer);

		timer_destroy(&port->gptp_capable_receive_sm.timeout_timer);
	}

	timer_destroy(&port->port_sync.announce_receive_sm.timeout_timer);

	timer_destroy(&port->sync_receive_timeout_timer);

	timer_destroy(&port->follow_up_receive_timeout_timer);

	if ((port->instance->domain.domain_number == PTP_DOMAIN_0) && (!is_common_p2p(port)))
		gptp_port_common_exit_timers(port->c);

	timer_destroy(&port->port_sync.sync_send_sm.sync_receipt_timeout_timer);

	timer_destroy(&port->port_sync.sync_send_sm.sync_transmit_timer);

	if (!port->gm_id_static)
		timer_destroy(&port->announce_transmit_sm.transmit_timer);
}

__exit static void gptp_instance_exit_timers(struct gptp_instance *instance)
{
	timer_destroy(&instance->clock_master_sync_send_timer);
}

static int gptp_port_common_set_profile_parameters(struct gptp_port_common *port, struct fgptp_config *cfg)
{
	int rc = 0;

	switch (cfg->profile) {
	case CFG_GPTP_PROFILE_STANDARD:
		port->as_capable_static = false;
		port->pdelay_transmit_enabled = true;

		break;

	case CFG_GPTP_PROFILE_AUTOMOTIVE:
		port->as_capable_static = true;

		if (cfg->neighborPropDelay_mode == CFG_GPTP_PDELAY_MODE_SILENT)
			port->pdelay_transmit_enabled = false;
		else
			port->pdelay_transmit_enabled = true;

		break;

	default:
		os_log(LOG_INFO, "Port(%u): profile %d not supported\n", port->port_id, cfg->profile);
		rc = -1;

		break;
	}

	return rc;
}

/** Apply gPTP profile parameters to a given gptp port
* \return		0 on success, negative value on failure
* \param instance	pointer to the gptp instance
* \param port		pointer to the gptp port context to configure
* \param port_index	index of the port to configure
* \param cfg		pointer to the gptp configuration
*/
__init static int gptp_port_set_profile_parameters(struct gptp_instance *instance, struct gptp_port *port, unsigned int port_index, struct fgptp_config *cfg)
{
	int rc = 0;

	switch (cfg->profile) {
	case CFG_GPTP_PROFILE_STANDARD:
		port->gm_id_static = false;
		port->link_up_static = false;

		/* Note: port default role assignement may be changed by BMCA */
		instance->params.selected_role[port_index + 1] = SLAVE_PORT;
		break;

	case CFG_GPTP_PROFILE_AUTOMOTIVE:
		port->gm_id_static = true;
		port->link_up_static = true;

		/* in automotive profile port's role is statically defined */
		instance->params.selected_role[port_index + 1] = cfg->port_cfg[port_index].portRole;

		switch (cfg->port_cfg[port_index].portRole) {
		case DISABLED_PORT:
			/* DISABLED role implies no time-synchronization and bmca */
			port->params.ptp_port_enabled = false;
			break;
		case SLAVE_PORT:
			/* SLAVE role on at least one port implies not able to act as grand master */
			instance->is_grandmaster = false;
			break;
		default:
			break;
		}

		break;

	default:
		os_log(LOG_INFO, "Port(%u): profile %d not supported\n", port_index, instance->gptp->cfg.profile);
		rc = -1;
		break;
	}

	return rc;
}


static void gptp_port_common_set_config_parameters(struct gptp_port_common *port, struct fgptp_config *cfg, u8 *clock_identity)
{
	os_memcpy(port->identity.clock_identity, clock_identity, sizeof(struct ptp_clock_identity));

	port->identity.port_number = port->port_id + 1;

	stats_init(&port->pdelay_stats, 31, "Pdelay (ns)", NULL);

	port->md.globals.allowedLostResponses = cfg->port_cfg[port->port_id].allowedLostResponses;
	port->md.globals.allowedFaults = cfg->port_cfg[port->port_id].allowedFaults;
	port->md.globals.initialLogPdelayReqInterval = cfg->port_cfg[port->port_id].initialLogPdelayReqInterval;
	u64_to_u_scaled_ns(&port->md.globals.meanLinkDelayThresh, cfg->neighborPropDelayThreshold);

	/* 802.1AS-2020 section 14.8.31, 14.8.35 */
	port->params.initial_compute_neighbor_rate_ratio = true;
	port->params.initial_compute_mean_link_delay = true;

	port->mgt_settable_log_pdelayreq_interval = false;
	port->mgt_settable_compute_mean_link_delay = false;
	port->mgt_settable_compute_neighbor_rate_ratio = false;

	/*802.1AS-2020 sections 14.8.26, 14.8.29, 14.8.33 */
	port->use_mgt_settable_log_pdelayreq_interval = false;
	port->use_mgt_settable_compute_mean_link_delay = false;
	port->use_mgt_settable_compute_neighbor_rate_ratio = false;

	port->cfg.neighborPropDelay_mode = cfg->neighborPropDelay_mode;
	port->cfg.initial_neighborPropDelay = cfg->initial_neighborPropDelay[port->port_id];
	port->cfg.neighborPropDelay_sensitivity = cfg->neighborPropDelay_sensitivity;
}

/** Apply gPTP profile parameters to a given gptp port
* \return		0 on success, negative value on failure
* \param instance	pointer to the gptp instance
* \param port		pointer to the gptp port context to configure
* \param port_index	index of the port to configure
* \param cfg		pointer to the gptp configuration
*/
__init static int gptp_port_set_config_parameters(struct gptp_instance *instance, struct gptp_port *port, int port_index, struct fgptp_config *cfg)
{
	u64 announce_interval;
	int rc;

	port->cfg.operLogPdelayReqInterval = cfg->port_cfg[port_index].operLogPdelayReqInterval;
	port->cfg.operLogSyncInterval = cfg->port_cfg[port_index].operLogSyncInterval;

	port->sync_info.state = SYNC_STATE_UNDEFINED;
	port->ratio_is_valid = 1;

	/* timeout values for sync, announce and gPTP-capable state machines */
	port->sync_receipt_timeout = SYNC_RECEIPT_TIMEOUT;
	port->announce_receipt_timeout = ANNOUNCE_RECEIPT_TIMEOUT;
	port->gptp_capable_receipt_timeout = GPTP_CAPABLE_RECEIPT_TIMEOUT;

	port->params.as_capable = false;
	port->params.initial_log_sync_interval = cfg->port_cfg[port_index].initialLogSyncInterval;
	port->params.initial_log_announce_interval = cfg->port_cfg[port_index].initialLogAnnounceInterval;
	port->params.initial_log_gptp_capable_message_interval = 0;

	/*Avnu AutoCDSFunctionalSpec-1_4 - 6.2.1.5 / 6.2.1.6 */
	port->params.oper_log_pdelay_req_interval = cfg->port_cfg[port_index].operLogPdelayReqInterval; /* A device moves to this value on all slave ports once the measured values have stabilized*/
	port->params.oper_log_sync_interval = cfg->port_cfg[port_index].operLogSyncInterval; /*operLogSyncInterval is the Sync interval that a device moves to and signals on a slave port once it has achieved synchronization*/

	port->mgt_settable_log_sync_interval = cfg->port_cfg[port_index].initialLogSyncInterval;
	port->mgt_settable_log_gptp_capable_message_interval = 0;

	/*802.1AS-2020 section  14.8.37 */
	port->use_mgt_settable_log_gptp_capable_message_interval = false;

	/*802.1AS-2020 section 14.8.19 */
	if (instance->domain.domain_number == PTP_DOMAIN_0)
		port->use_mgt_settable_log_sync_interval = false;
	else
		port->use_mgt_settable_log_sync_interval = true;

	/* port and master priority vector are initialized to this system vector */
	os_memcpy(&port->params.port_priority, &instance->params.system_priority, sizeof(struct ptp_priority_vector));
	os_memcpy(&port->params.master_priority, &instance->params.system_priority, sizeof(struct ptp_priority_vector));

	/* 10.2.4.12 ptpPortEnabled is set if time-synchronization and best master
	selection functions of the port are enabled */
	port->params.ptp_port_enabled = cfg->port_cfg[port_index].ptpPortEnabled;

	/*
	 * The specification does not initialize announce_interval before its first use
	 * so we are doing it here to prevent unwanted behavior.
	 */
	announce_interval = log_to_ns(port->params.initial_log_announce_interval);
	u64_to_u_scaled_ns(&port->params.announce_interval, announce_interval);

	/*
	The mechanism for measuring mean propagation delay and neighbor rate ratio on the link
	attached to this PTP Port.
	*/
	port->params.delay_mechanism = cfg->port_cfg[port_index].delayMechanism[instance->index];

	rc = gptp_port_set_profile_parameters(instance, port, port_index, cfg);

	return rc;
}

/** Apply gPTP profile parameters to a gptp instance
* \return		0 on success, negative value on failure
* \param profile	pointer to the gptp instance context
*/
__init static int gptp_instance_set_profile_parameters(struct gptp_instance *instance)
{
	int rc = 0;

	switch (instance->gptp->cfg.profile) {
	case CFG_GPTP_PROFILE_STANDARD:
		instance->params.gm_present = false;	/* will be determined by bmca */
		instance->is_grandmaster = false;		/* will be determined by bmca */
		break;
	case CFG_GPTP_PROFILE_AUTOMOTIVE:
		instance->params.gm_present = true;		/* in automotive profile the grandmaster is already known by all peers */
		instance->is_grandmaster = instance->gmCapable; /* acting as grand master if gmCapable is set */
		break;
	default:
		os_log(LOG_INFO, "profile %d not supported\n", instance->gptp->cfg.profile);
		rc = -1;
		break;
	}

	return rc;
}

__init static int gptp_instance_set_config_parameters(struct gptp_instance *instance, struct fgptp_config *cfg)
{
	struct fgptp_domain_config *domain_cfg = &cfg->domain_cfg[instance->index];
	struct gptp_ctx *gptp = instance->gptp;
	int port_index;

	instance->cfg.gm_id = cfg->gm_id;

	/* IEEE 802.1AS-2020 10.7.2.4 */
	instance->clock_master_log_sync_interval = CFG_GPTP_MAX_LOG_SYNC_INTERVAL;
	for (port_index = 0; port_index < instance->numberPorts; port_index++) {
		if (instance->clock_master_log_sync_interval > cfg->port_cfg[port_index].initialLogSyncInterval)
			instance->clock_master_log_sync_interval = cfg->port_cfg[port_index].initialLogSyncInterval;
	}
	instance->gmCapable = domain_cfg->gmCapable;
	instance->numberPorts = gptp->port_max;
	instance->versionNumber = PTP_VERSION;

	if (gptp_compute_clock_identity(gptp, instance->params.this_clock.identity) < 0) {
		os_log(LOG_ERR, "instance(%p) port(0) not available\n", instance);
		goto err; /* requested port not available, cannot start */
	}

	instance->params.system_priority.u.s.root_system_identity.u.s.clock_quality.clock_class = domain_cfg->clockClass;
	instance->params.system_priority.u.s.root_system_identity.u.s.clock_quality.clock_accuracy = domain_cfg->clockAccuracy;
	instance->params.system_priority.u.s.root_system_identity.u.s.clock_quality.offset_scaled_log_variance = htons(domain_cfg->offsetScaledLogVariance);
	instance->params.system_priority.u.s.root_system_identity.u.s.priority_1 = domain_cfg->priority1;
	instance->params.system_priority.u.s.root_system_identity.u.s.priority_2 = domain_cfg->priority2;

	gptp_instance_priority_vector_update(instance);

	/* set various instance parameters */
	instance->params.sys_current_utc_offset = 37;
	instance->params.sys_current_utc_offset_valid = 1;
	instance->params.sys_frequency_traceable = 0;
	instance->params.sys_leap59 = 0;
	instance->params.sys_leap61 = 0;
	instance->params.sys_time_source = TIME_SOURCE_INTERNAL_OSCILLATOR;
	instance->params.sys_time_traceable = 0;

	/*
	* per port state machines and variables
	*/
	instance->params.selected_role[0] = SLAVE_PORT; /* first slot in selected_role array has specific meaning, port0 start indeed at selected_role[1] (see below port_number attribute) */

	/* apply profile  */
	if (gptp_instance_set_profile_parameters(instance) < 0)
		goto err;

	return 0;

err:
	return -1;
}

/** Prints gptp packet and errors counters
* \return	none
* \param counters	pointer to the counters structure of the port
*/
static void gptp_dump_instance_port_counters(struct gptp_instance *instance, struct gptp_port *port)
{
	struct gptp_instance_stats *instance_stats = &instance->stats;
	struct gptp_instance_port_stats *stats = &port->stats;

	os_log(LOG_INFO_RAW, "Port(%d): domain(%u, %u):\n"
	"\tPortStatRxPkts  %u\n"
	"\tPortStatRxSyncCount  %u\n"
	"\tPortStatRxSyncReceiptTimeouts  %u\n"
	"\tPortStatRxFollowUpCount  %u\n"
	"\tPortStatRxFollowUpTimeouts  %u\n"
	"\tPortStatRxAnnounce  %u\n"
	"\tPortStatAnnounceReceiptTimeouts  %u\n"
	"\tPortStatAnnounceReceiptDropped  %u\n"
	"\tPortStatRxSignaling  %u\n"
	"\tPortStatTxPkts %u\n"
	"\tPortStatTxSyncCount %u\n"
	"\tPortStatTxFollowUpCount %u\n"
	"\tPortStatTxAnnounce %u\n"
	"\tPortStatTxSignaling %u\n"
	"\tPortStatAdjustOnSync %u\n"
	"\tPortStatMdSyncRcvSmReset %u\n"
	"\tPortStatNumSynchronizationLoss %u\n"
	"\tPortStatNumNotAsCapable %u\n",
	port->port_id,
	instance->index,
	instance->domain.domain_number,
	stats->num_rx_pkts,
	stats->num_rx_sync,
	stats->num_rx_sync_timeout,
	stats->num_rx_fup,
	stats->num_rx_fup_timeout,
	stats->num_rx_announce,
	stats->num_rx_announce_timeout,
	stats->num_rx_announce_dropped,
	stats->num_rx_sig,
	stats->num_tx_pkts,
	stats->num_tx_sync,
	stats->num_tx_fup,
	stats->num_tx_announce,
	stats->num_tx_sig,
	instance_stats->num_adjust_on_sync,
	stats->num_md_sync_rcv_sm_reset,
	instance_stats->num_synchro_loss,
	stats->num_not_as_capable
	);
}


static void gptp_dump_port_counters(struct gptp_port_common *port, const char *prefix)
{
	struct gptp_port_stats *stats = &port->stats;

	os_log(LOG_INFO_RAW, "%s(%d):\n"
		"\tPeer ClockIdentity 0x%"PRIx64"\n"
		"\tPortStatRxPdelayRequest %u\n"
		"\tPortStatRxPdelayResponse %u\n"
		"\tPortStatRxPdelayResponseFollowUp %u\n"
		"\trxPdelayRespLostExceeded %u\n"
		"\tPortStatTxPdelayRequest %u\n"
		"\tPortStatTxPdelayResponse %u\n"
		"\tPortStatTxPdelayResponseFollowUp %u\n"
		"\tPortStatMdPdelayReqSmReset %u\n",
		prefix,
		port->port_id,
		stats->peer_clock_id,
		stats->num_rx_pdelayreq,
		stats->num_rx_pdelayresp,
		stats->num_rx_pdelayrespfup,
		stats->num_rx_pdelayresp_lost_exceeded,
		stats->num_tx_pdelayreq,
		stats->num_tx_pdelayresp,
		stats->num_tx_pdelayrespfup,
		stats->num_md_pdelay_req_sm_reset
	);
}

static void gptp_dump_pdelay_stats(struct gptp_port_common *c, const char *prefix)
{
	struct stats *pdelay_stats;
	ptp_double delay_double;

	pdelay_stats = &c->pdelay_stats;

	stats_compute(pdelay_stats);
	u_scaled_ns_to_ptp_double(&delay_double, &c->params.mean_link_delay);

	os_log(LOG_INFO_RAW, "%s(%d): Propagation delay (ns): %4.2f			min %6d avg %6d max %6d variance %5"PRId64"\n", prefix, c->port_id, delay_double, pdelay_stats->min, pdelay_stats->mean, pdelay_stats->max, pdelay_stats->variance);
	os_log(LOG_INFO_RAW, "%s(%d): NeighborRateRatio     : %10.9f\n", prefix, c->port_id, c->params.neighbor_rate_ratio);

	stats_reset(pdelay_stats);
}

static void gptp_dump_cmlds_counters(struct gptp_ctx *gptp)
{
	int link_id;
	char *link_port_enabled_str;

	for (link_id = 0; link_id< gptp->cmlds.number_link_ports; link_id++) {
		link_port_enabled_str = (gptp->cmlds.link_ports[link_id].cmlds_link_port_enabled) ? "Enabled" : "Disabled";

		os_log(LOG_INFO_RAW, "CMLDS Port(%d): linkPortEnabled: %s\n", link_id, link_port_enabled_str);

		if (!gptp->cmlds.link_ports[link_id].cmlds_link_port_enabled)
			continue;

		gptp_dump_pdelay_stats(get_cmlds_port_common(gptp, link_id), "CMLDS Port");

		gptp_dump_port_counters(get_cmlds_port_common(gptp, link_id), "CMLDS Port");
	}
}

static void gptp_dump_net_counters(struct gptp_ctx *gptp)
{
	int net_port_index;

	for (net_port_index = 0; net_port_index < gptp->port_max; net_port_index++) {
		struct gptp_net_port_stats *stats = &gptp->net_ports[net_port_index].stats;

		if (!gptp->net_ports[net_port_index].initialized)
			continue;

		os_log(LOG_INFO_RAW, "Net Port(%d):\n"
			"\tPortStatRxPkts %u\n"
			"\tPortStatTxPkts %u\n"
			"\tPortStatRxErrEtype %u\n"
			"\tPortStatRxErrPortId %u\n"
			"\tPortStatTxErr %u\n"
			"\tPortStatTxErrAlloc %u\n"
			"\tPortStatHwTsRequest %u\n"
			"\tPortStatHwTsHandler %u\n",
			net_port_index,
			stats->num_rx_pkts,
			stats->num_tx_pkts,
			stats->num_rx_err_etype,
			stats->num_rx_err_portid,
			stats->num_tx_err,
			stats->num_tx_err_alloc,
			stats->num_hwts_request,
			stats->num_hwts_handler
		);
	}
}

const char *gptp_port_role2string(ptp_port_role_t port_role)
{
	switch (port_role) {
		case DISABLED_PORT:
			return "Disabled";
			break;
		case MASTER_PORT:
			return "Master  ";
			break;
		case PASSIVE_PORT:
			return "Passive ";
			break;
		case SLAVE_PORT:
			return "Slave   ";
			break;
		default:
			return "Unknown ";
			break;
	}
}


/** Prints gptp statistics to standard output
* \return	none
* \param gptp	pointer to the main gptp context
*/
void gptp_stats_dump(void *data)
{
	struct gptp_ctx *gptp = (struct gptp_ctx *)data;
	struct gptp_instance *instance;
	struct gptp_port *port;
	unsigned char instance_index, i;
	ptp_port_role_t port_role;

	gptp_dump_net_counters(gptp);

	gptp_dump_cmlds_counters(gptp);

	for (instance_index = 0; instance_index < gptp->domain_max; instance_index++) {
		instance = gptp->instances[instance_index];

		if (!instance->params.instance_enable)
			continue;

		for (i = 0; i < instance->numberPorts; i++) {
			port = &instance->ports[i];

			port_role = instance->params.selected_role[get_port_identity_number(port)];

			/* Display Role/Link/ASCapable/DelayMechanism for all kind of port (SLAVE/MASTER/DISABLE/PASSIVE) */
			os_log(LOG_INFO_RAW, "Port(%d): domain(%u, %u): Role: %s Link: %s asCapable: %s neighborGptpCapable: %s delayMechanism: %s\n",
				port->port_id, instance->index, instance->domain.domain_number,
				gptp_port_role2string(port_role),
				(gptp->net_ports[i].port_oper)? "Up " : "Down",
				(port->params.as_capable)? "Yes" : "No",
				(port->params.neighbor_gptp_capable) ? "Yes" : "No",
				is_common_p2p(port)? "COMMON_P2P" : "P2P");

			if (!gptp->net_ports[i].port_oper)
				continue;

			if (port->params.ptp_port_enabled && (port->instance->domain.domain_number == PTP_DOMAIN_0) && (!is_common_p2p(port)))
				gptp_dump_pdelay_stats(port->c, "Port");

			if (port_role == SLAVE_PORT)
				target_clkadj_dump_stats(&instance->target_clkadj_params);

			gptp_dump_instance_port_counters(instance, port);

			if ((instance->domain.domain_number == PTP_DOMAIN_0) && (!is_common_p2p(port)))
				gptp_dump_port_counters(port->c, "Port");
		}
	}
}


static void gptp_as_capable_update_fsm(struct gptp_port *port)
{
	port_announce_info_sm(port, PORT_ANNOUNCE_INFO_SM_EVENT_RUN);
	port_announce_rcv_sm(port);
}


/** Handles as_capable down event ,sets the per port variable as_capable to FALSE
* and calls all FSM listening to as_capable state change
* \return	none
* \param port	pointer to the port
*/
static void gptp_as_capable_down(struct gptp_port *port)
{
	if (port->params.as_capable) {
		os_log(LOG_INFO, "Port(%u): port is not AS_CAPABLE\n", port->port_id);

		port->params.as_capable = false;

		gptp_as_capable_update_fsm(port);
	}
}


/** Handles as_capable up event, sets the per port variable as_capable to TRUE
* and calls all FSM listening to as_capable state change
* \return	none
* \param port	pointer to the port
*/
static void gptp_as_capable_up(struct gptp_port *port)
{
	if ((!port->params.as_capable) && (port->params.ptp_port_enabled)) {
		os_log(LOG_INFO, "Port(%u): port is AS_CAPABLE\n", port->port_id);

		port->params.as_capable = true;

		gptp_as_capable_update_fsm(port);
	}
}


void gptp_update_as_capable(struct gptp_port *port)
{
	/* IEEE 802.1AS-2020 - 11.2.2 Determination of asCapable */
	if (get_as_capable_accross_domains(port) &&
		((port->params.neighbor_gptp_capable) || (port->instance->domain.domain_number == PTP_DOMAIN_0)))
		gptp_as_capable_up(port);
	else
		gptp_as_capable_down(port);

	gptp_ipc_gptp_port_params(port->instance, port, &port->instance->gptp->ipc_tx, IPC_DST_ALL);
}


void gptp_as_capable_across_domains_down(struct gptp_port_common *port)
{
	struct gptp_ctx *gptp = port->gptp;
	int i;

	if (port->params.as_capable_across_domains) {
		if (!port->as_capable_static) {
			os_log(LOG_INFO, "Port(%u): port is not AS_CAPABLE\n", port->port_id);

			port->params.as_capable_across_domains = false;

			for (i = 0; i < gptp->domain_max; i++) {
				if (!gptp->instances[i]->params.instance_enable)
					continue;

				gptp_update_as_capable(&gptp->instances[i]->ports[port->port_id]);
				gptp->instances[i]->ports[port->port_id].stats.num_not_as_capable++;
			}

			/* pdelay stats and as not capable counters are updated only
			when we are running a standard profile i.e. asCapable not statically defined */
			stats_reset(&port->pdelay_stats);
		}
	}
}


void gptp_as_capable_across_domains_up(struct gptp_port_common *port)
{
	struct gptp_ctx *gptp = port->gptp;
	int i;

	if (!port->params.as_capable_across_domains) {
		os_log(LOG_INFO, "Port(%u): link is AS_CAPABLE\n", port->port_id);

		port->params.as_capable_across_domains = true;

		for (i = 0; i < gptp->domain_max; i++) {
			if (!gptp->instances[i]->params.instance_enable)
				continue;

			gptp_update_as_capable(&gptp->instances[i]->ports[port->port_id]);
		}
	}
}


void gptp_neighbor_gptp_capable_down(struct gptp_port *port)
{
	struct ptp_instance_port_params *params = &port->params;

	if (params->neighbor_gptp_capable) {
		os_log(LOG_INFO, "Port(%u) domain(%u, %u): neighbor is not GPTP_CAPABLE\n", port->port_id, port->instance->index, port->instance->domain.domain_number);

		params->neighbor_gptp_capable = false;

		/* IEEE 802.1AS-2020 - section 11.2.2 f) The per-PTP port, per-domain global variable asCapable
		is updated upon neighborGptpCapable change */
		gptp_update_as_capable(port);
	}
}

void gptp_neighbor_gptp_capable_up(struct gptp_port *port)
{
	struct ptp_instance_port_params *params = &port->params;

	if (!params->neighbor_gptp_capable) {
		os_log(LOG_INFO, "Port(%u) domain(%u, %u): neighbor is GPTP_CAPABLE\n", port->port_id, port->instance->index, port->instance->domain.domain_number);

		params->neighbor_gptp_capable = true;

		/* IEEE 802.1AS-2020 - section 11.2.2 f) The per-PTP port, per-domain global variable asCapable
		is updated upon neighborGptpCapable change */
		gptp_update_as_capable(port);
	}
}


static void gptp_port_common_update_fsm(struct gptp_port_common *port)
{
	md_link_delay_interval_setting_sm(port);
	md_pdelay_req_sm(port, PDELAY_REQ_SM_EVENT_RUN);
	md_pdelay_resp_sm(port, PDELAY_RESP_SM_EVENT_RUN);
}


void gptp_port_update_fsm(struct gptp_port *port)
{
	if (port->c)
		gptp_port_common_update_fsm(port->c);

	port_sync_interval_setting_sm(port);

	port_announce_interval_setting_sm(port);
	port_announce_info_sm(port, PORT_ANNOUNCE_INFO_SM_EVENT_RUN);
	port_announce_rcv_sm(port);
	port_announce_transmit_sm(port, PORT_ANNOUNCE_TRANSMIT_SM_EVENT_RUN);

	if (!port->instance->gptp->force_2011) {
		port_gptp_capable_interval_setting_sm(port);
		port_gptp_capable_receive_sm(port, GPTP_CAPABLE_RECEIVE_SM_EVENT_RUN);
		port_gptp_capable_transmit_sm(port, GPTP_CAPABLE_TRANSMIT_SM_EVENT_RUN);
	}
}


/** Handles link down event, sets the per port variable port_oper to FALSE
* and calls all FSM listening to port_oper state change
* \return	none
* \param net_port	pointer to the network port
*/
static void gptp_link_down(struct gptp_net_port *net_port)
{
	struct gptp_ctx *gptp = container_of(net_port, struct gptp_ctx, net_ports[net_port->port_id]);
	struct gptp_port *port;
	int i;

	os_log(LOG_INFO, "Port(%u): link is DOWN\n", net_port->port_id);

	net_port->port_oper = false;

	for (i = 0; i < gptp->domain_max; i++) {
		port = net_port_to_gptp(net_port, i);
		if (port && port->instance->params.instance_enable)
			gptp_port_update_fsm(port);
	}

	for (i = 0; i < gptp->cmlds.number_link_ports; i++) {
		if (gptp->cmlds.link_ports[i].c.port_id == net_port->port_id) {
			gptp_port_common_update_fsm(&gptp->cmlds.link_ports[i].c);
			break;
		}
	}
}


/** Handles link up event, sets the per port variable port_oper to TRUE
* and calls all FSM listening to port_oper state change
* \return	none
* \param net_port	pointer to the network port
*/
static void gptp_link_up(struct gptp_net_port *net_port)
{
	struct gptp_ctx *gptp = container_of(net_port, struct gptp_ctx, net_ports[net_port->port_id]);
	struct gptp_port *port;
	int i;

	os_log(LOG_INFO, "Port(%u): link is UP\n", net_port->port_id);

	net_port->port_oper = true;

	for (i = 0; i < gptp->domain_max; i++) {
		port = net_port_to_gptp(net_port, i);
		if (port && port->instance->params.instance_enable)
			gptp_port_update_fsm(port);
	}

	for (i = 0; i < gptp->cmlds.number_link_ports; i++) {
		if (gptp->cmlds.link_ports[i].c.port_id == net_port->port_id) {
			gptp_port_common_update_fsm(&gptp->cmlds.link_ports[i].c);
			break;
		}
	}
}

static void gptp_ipc_get_mac_status(struct gptp_ctx *gptp, struct ipc_tx *ipc, unsigned int port_id)
{
	struct ipc_desc *desc;
	struct ipc_mac_service_get_status *get_status;
	int rc;

	desc = ipc_alloc(ipc, sizeof(struct ipc_mac_service_get_status));
	if (desc) {
		desc->type = IPC_MAC_SERVICE_GET_STATUS;
		desc->len = sizeof(struct ipc_mac_service_get_status);
		desc->flags = 0;

		get_status = &desc->u.mac_service_get_status;

		get_status->port_id = port_id;

		rc = ipc_tx(ipc, desc);
		if (rc < 0) {
			if (rc != -IPC_TX_ERR_NO_READER)
				os_log(LOG_ERR, "ipc_tx() failed\n");

			ipc_free(ipc, desc);
		}
	} else
		os_log(LOG_ERR, "ipc_alloc() failed\n");
}

static void gptp_ipc_rx_mac_service(struct ipc_rx const *rx, struct ipc_desc *desc)
{
	struct gptp_ctx *gptp = container_of(rx, struct gptp_ctx, ipc_rx_mac_service);
	struct gptp_net_port *net_port;
	struct ipc_mac_service_status *status;

	switch (desc->type) {
	case IPC_MAC_SERVICE_STATUS:
		status = &desc->u.mac_service_status;

		os_log(LOG_DEBUG, "IPC_MAC_SERVICE_STATUS: port(%u)\n", status->port_id);

		net_port = logical_to_net_port(gptp, status->port_id);
		if (net_port) {
			if (status->operational)
				gptp_link_up(net_port);
			else
				gptp_link_down(net_port);
		}

		break;

	default:
		break;
	}

	ipc_free(rx, desc);
}


static void gptp_begin_update_port_common_fsm(struct gptp_port_common *port, bool *ptp_port_enabled, s8 cmlds_flag)
{
	port->md.link_interval_sm.portEnabled3 = ptp_port_enabled;
	port->md.link_interval_sm.log_supported_pdelayreq_interval_max = CFG_GPTP_MAX_LOG_PDELAY_REQ_INTERVAL;
	port->md.link_interval_sm.log_supported_closest_longer_pdelayreq_Interval = CFG_GPTP_MIN_LOG_PDELAY_REQ_INTERVAL;
	md_link_delay_interval_setting_sm(port);

	port->md.pdelay_req_sm.portEnabled0 = ptp_port_enabled;
	port->md.pdelay_req_sm.s = cmlds_flag;
	md_pdelay_req_sm(port, PDELAY_REQ_SM_EVENT_RUN);

	port->md.pdelay_resp_sm.portEnabled1 = ptp_port_enabled;
	port->md.pdelay_resp_sm.s = cmlds_flag;
	md_pdelay_resp_sm(port, PDELAY_RESP_SM_EVENT_RUN);
}


/** Initialize all per port FSM
* \return	none
* \param port	pointer to the port
*/
__init static void gptp_begin_update_port_fsm(struct gptp_port *port)
{
	md_sync_rcv_sm(port, SYNC_RCV_SM_EVENT_RUN);
	md_sync_send_sm(port, SYNC_SND_SM_EVENT_RUN);

	port->sync_interval_sm.log_supported_sync_interval_max = CFG_GPTP_MAX_LOG_SYNC_INTERVAL;
	port->sync_interval_sm.log_supported_closest_longer_sync_interval = CFG_GPTP_MIN_LOG_SYNC_INTERVAL;
	port_sync_interval_setting_sm(port);

	port->announce_interval_sm.log_supported_announce_interval_max = CFG_GPTP_MAX_LOG_ANNOUNCE_INTERVAL;
	port->announce_interval_sm.log_supported_closest_longer_announce_interval = CFG_GPTP_MIN_LOG_ANNOUNCE_INTERVAL;
	port_announce_interval_setting_sm(port);

	if (!port->instance->gptp->force_2011) {
		port_gptp_capable_interval_setting_sm(port);
		port_gptp_capable_receive_sm(port, GPTP_CAPABLE_RECEIVE_SM_EVENT_RUN);
		port_gptp_capable_transmit_sm(port, GPTP_CAPABLE_TRANSMIT_SM_EVENT_RUN);
	}

	port_sync_sync_rcv_sm(port);
	port_sync_sync_send_sm(port, PORT_SYNC_SYNC_SEND_SM_EVENT_RUN);
	port_announce_rcv_sm(port);
	port_announce_info_sm(port, PORT_ANNOUNCE_INFO_SM_EVENT_RUN);
	port_announce_transmit_sm(port, PORT_ANNOUNCE_TRANSMIT_SM_EVENT_RUN);

	clock_slave_sync_sm(port);
}

/** Initialize all per time aware system FSM
* \return	none
* \param port	pointer to the port
*/
__init static void gptp_begin_update_instance_fsm(struct gptp_instance *instance)
{
	port_state_selection_sm(instance);

	clock_master_sync_send_sm(instance, CLOCK_MASTER_SYNC_SEND_SM_EVENT_RUN);
	clock_master_sync_receive_sm(instance);
	clock_master_sync_offset_sm(instance);

	site_sync_sync_sm(instance);
}


static void gptp_cmlds_enable_link_port(struct gptp_ctx *gptp, u16 link_id)
{
	if (link_id < gptp->cmlds.number_link_ports) {
		gptp->cmlds.link_ports[link_id].cmlds_link_port_enabled = true;

		gptp_port_common_update_fsm(&gptp->cmlds.link_ports[link_id].c);
	}
}

static int gptp_cmlds_init(struct gptp_ctx *gptp, struct fgptp_config *cfg)
{
	struct cmlds_ctx *cmlds = &gptp->cmlds;
	struct gptp_port_common *c;
	int i;

	if (gptp_compute_clock_identity(gptp, (u8 *)&cmlds->this_clock) < 0) {
		os_log(LOG_ERR, "gptp(%p) clock identity error\n", gptp);
		goto err_clock_identity;
	}

	cmlds->number_link_ports = gptp->port_max;

	/*
	TODO : loop over all link ports and initialize the associated gptp_port_common structure used by the CMLDS
	*/
	for (i = 0; i < cmlds->number_link_ports; i++) {
		c = &cmlds->link_ports[i].c;

		c->gptp = gptp;
		c->port_id = i;

		gptp_port_common_set_config_parameters(c, cfg, cmlds->this_clock.identity);

		gptp_port_common_set_profile_parameters(c, cfg);

		if (gptp_port_common_init_timers(c) < 0)
			goto err_port_timers;

		c->params.begin = true;

		gptp_begin_update_port_common_fsm(c, &cmlds->link_ports[i].cmlds_link_port_enabled, -1);

		c->params.begin = false;

		os_log(LOG_INIT, "CMLDS link port (%u) initialized\n", i);
	}

	return 0;

err_port_timers:
err_clock_identity:
	return -1;
}

static void gptp_cmlds_exit(struct gptp_ctx*gptp)
{
	struct cmlds_ctx *cmlds = &gptp->cmlds;
	int i;

	for (i = 0; i < cmlds->number_link_ports; i++) {
		gptp_port_common_exit_timers(&cmlds->link_ports[i].c);
	}
}

static int gptp_instance_init(struct gptp_ctx *gptp, int instance_index, struct fgptp_config *cfg)
{
	struct gptp_instance *instance = gptp->instances[instance_index];
	int domain = cfg->domain_cfg[instance_index].domain_number;
	struct gptp_port *port;
	ptp_delay_mechanism_t delay_mechanism;
	int i, port_index;

	instance->gptp = gptp;

	instance->index = (u8)instance_index;
	instance->domain.domain_number = (u8)domain;
	instance->domain.u.s.major_sdo_id = PTP_DOMAIN_MAJOR_SDOID;
	instance->domain.u.s.minor_sdo_id = PTP_DOMAIN_MINOR_SDOID;

	if (domain < 0)
		instance->params.instance_enable = false;
	else
		instance->params.instance_enable = true;

	if (gptp_instance_set_config_parameters(instance, cfg) < 0)
		goto err_system_config;

	instance->clock_target = cfg->domain_cfg[instance->index].clock_target;
	instance->clock_source = cfg->domain_cfg[instance->index].clock_source;

	target_clkadj_params_init(&instance->target_clkadj_params, instance->clock_target, &gptp->local_clock, instance->index, instance->domain.domain_number);

	for (port_index = 0; port_index < instance->numberPorts; port_index++) {
		port = &instance->ports[port_index];

		port->link_id = port_index;
		port->instance = instance;
		port->port_id = port_index;

		gptp_port_set_config_parameters(instance, port, port_index, cfg);

		delay_mechanism = port->params.delay_mechanism;

		/*
		Setting per Link-Port's cmldsLinkPortEnabled to TRUE if both the value of delayMechanism is Common_P2P and the value of
		ptpPortEnabled is TRUE, for at least one PTP Port that uses the CMLDS that is invoked on the Link Port.
		*/
		if ((delay_mechanism == COMMON_P2P) && (port->params.ptp_port_enabled) && (instance->params.instance_enable))
			gptp_cmlds_enable_link_port(gptp, port->link_id);

		if (instance->domain.domain_number == PTP_DOMAIN_0) {
			/* Instance's peer delay mechanism */
			port->c = &gptp->common_ports[port_index];
			port->c->gptp = gptp;
			port->c->port_id = port_index;

			gptp_port_common_set_config_parameters(port->c, cfg, instance->params.this_clock.identity);

			gptp_port_common_set_profile_parameters(port->c, cfg);

			if (is_common_p2p(port)) {
				/* Silence PDelay requests using the P2P delay mechanism when CMLDS is enabled on domain 0 */
				port->c->pdelay_transmit_enabled = false;
			}

			if (gptp_port_common_init_timers(port->c) < 0)
				goto err_port_timers;

			port->c->params.begin = true;

			gptp_begin_update_port_common_fsm(port->c, &port->params.ptp_port_enabled, 1);

			port->c->params.begin = false;
		} else {
			/* CMLDS peer delay mechanism */
			port->c = NULL;
		}

		os_log(LOG_INIT, "Configuring Port(%u) (%p) domain(%u, %d) delayMechanism(%s)\n", port_index, port, instance->index, (int8_t)instance->domain.domain_number, gptp_delay_mechanism2string(delay_mechanism));
	}

	instance->params.begin = true;

	for (port_index = 0; port_index < instance->numberPorts; port_index++) {
		port = &instance->ports[port_index];

		if (gptp_port_init_timers(port, port_index) < 0)
			goto err_port_timers;

		/* initial state for all per port state machines */
		gptp_begin_update_port_fsm(port);
	}

	/* per time-aware system timers */
	if (gptp_instance_init_timers(instance) < 0)
		goto err_instance_timers;

	/* initial state for per time-aware system state machines */
	gptp_begin_update_instance_fsm(instance);

	instance->params.begin = false;

	os_log(LOG_INIT, "instance(%p) domain(%u, %d) is %s (gm capable %d)\n",
		       instance, instance->index, (int8_t)instance->domain.domain_number,
		       !instance->params.instance_enable ? "disabled" : "enabled",
		       cfg->domain_cfg[instance_index].gmCapable);

	return 0;

err_instance_timers:
err_port_timers:
	for (i = 0; i < port_index; i++)
		gptp_port_exit_timers(&instance->ports[i]);

err_system_config:
	return -1;
}

static void gptp_instance_exit(struct gptp_instance *instance)
{
	int i;

	for (i = 0; i < instance->numberPorts; i++) {
		instance->ports[i].params.as_capable = false;

		gptp_port_exit_timers(&instance->ports[i]);
	}

	target_clkadj_params_exit(&instance->target_clkadj_params);

	gptp_instance_exit_timers(instance);
}

static int gptp_check_config(struct fgptp_config *cfg)
{
	int err = 0;
	int i, j;

	/* Multiple domains are not supported for Automotive profile and in 802.1AS-2011 standard mode */
	if ((cfg->profile == CFG_GPTP_PROFILE_AUTOMOTIVE) || (cfg->force_2011)){
		for (i = 1; i < cfg->domain_max; i++) {
			if (cfg->domain_cfg[i].domain_number != -1) {
				os_log(LOG_ERR, "Configuration not allowed (multiple gPTP domains with Automotive profile or force_2011 option\n");
				err++;

				goto exit;
			}
		}
	}

	/* Domain numbers:
	 *
	 * Per 802.1AS-2020 - 8.1 -
	 * A time-aware system shall support one or more domains, each
	 * with a distinct domain number in the range 0 through 127.
	 * A time-aware system shall support the domain whose domain
	 * number is 0, and that domain number shall not be changed to
	 * a nonzero value.
	 */
	if (cfg->domain_cfg[0].domain_number != PTP_DOMAIN_0) {
		os_log(LOG_ERR, "Error: gPTP instance 0 must be assigned to domain 0.\n");
		err++;
	}

	for (i = 0; i < cfg->domain_max; i++) {
		/* All numbers between 0 and PTP_DOMAIN_NUMBER_MAX, included, or -1 */
		if (cfg->domain_cfg[i].domain_number < -1 || cfg->domain_cfg[i].domain_number > PTP_DOMAIN_NUMBER_MAX) {
			os_log(LOG_ERR, "Error: gPTP domain numbers must be between 0 and %d included, or -1 to disable it\n", PTP_DOMAIN_NUMBER_MAX);
			err++;
		}

		/* All numbers must be unique (except for "-1"s) */
		for (j = i + 1; j < cfg->domain_max; j++)
			if ((cfg->domain_cfg[i].domain_number == cfg->domain_cfg[j].domain_number)
				       && (cfg->domain_cfg[i].domain_number != -1))
				break;

		if (j != cfg->domain_max) {
			os_log(LOG_ERR, "domain(%u) Error: when enabled, gPTP domain numbers must be unique\n", j);
			err++;
		}
	}

	/* Delay mechanism
	*
	* IEEE 802.1AS-2020 section 11.2.17.1
	* if the domain number is not 0, portDS.delayMechanism (see Table 14-8 in 14.8.5) must not be P2P
	*/
	for (i = 0; i < cfg->domain_max; i++) {
		for (j = 0; j < cfg->port_max; j++) {
			if ((cfg->port_cfg[j].delayMechanism[i] != P2P) && (cfg->port_cfg[j].delayMechanism[i] != COMMON_P2P)) {
				os_log(LOG_ERR, "domain(%u, %u) port(%u) Error: delayMechanism (%u) not supported\n", i, cfg->domain_cfg[i].domain_number, j, cfg->port_cfg[j].delayMechanism[i]);
				err++;
			}

			if ((cfg->domain_cfg[i].domain_number > PTP_DOMAIN_0) && (cfg->port_cfg[j].delayMechanism[i] == P2P)) {
				os_log(LOG_ERR, "domain(%u, %u) port(%u) Error: delayMechanism must not be P2P for domains > 0\n", i, cfg->domain_cfg[i].domain_number, j);
				err++;
			}

			if ((cfg->force_2011) && (cfg->domain_cfg[i].domain_number == PTP_DOMAIN_0) && (cfg->port_cfg[j].delayMechanism[i] != P2P)) {
				os_log(LOG_ERR, "domain(%u, %u) port(%u) Error: in 802.1AS-2011 standard mode, delayMechanism for gPTP domain 0 must be P2P\n", i, cfg->domain_cfg[i].domain_number, j);
				err++;
			}
		}
	}

exit:
	if (!err)
		os_log(LOG_INFO, "gptp config is valid\n");

	return err;
}

static void gptp_set_config_parameters(struct gptp_ctx *gptp, struct fgptp_config *cfg)
{
	gptp->management_enabled = cfg->management_enabled;

	gptp->clock_local = cfg->clock_local;
	gptp->clock_monotonic = OS_CLOCK_SYSTEM_MONOTONIC;

	gptp->sync_indication = cfg->sync_indication;
	gptp->gm_indication = cfg->gm_indication;
	gptp->pdelay_indication = cfg->pdelay_indication;

	gptp->cfg.profile = cfg->profile;

	gptp->cfg.rsync = cfg->rsync;
	gptp->cfg.rsync_interval = cfg->rsync_interval;

	gptp->cfg.statsInterval = cfg->statsInterval;

	/*
	* IEEE 802.1AS-2011 interoperability mode
	*/
	gptp->force_2011 = cfg->force_2011;
}

__init static struct gptp_ctx *gptp_alloc(unsigned int domains, unsigned int ports, unsigned int timer_n)
{
	struct gptp_ctx *gptp;
	u8 *instance;
	unsigned int gptp_ctx_size;
	unsigned int instance_size;
	unsigned int size;
	int i;

	gptp_ctx_size = sizeof(struct gptp_ctx) + ports * sizeof(struct gptp_net_port);
	instance_size = sizeof(struct gptp_instance) + ports * sizeof(struct gptp_port);
	size = gptp_ctx_size + domains * instance_size + timer_pool_size(timer_n);

	gptp = os_malloc(size);
	if (!gptp)
		goto err;

	os_memset(gptp, 0, size);

	instance = (u8 *)gptp + gptp_ctx_size;

	for (i = 0; i < domains; i++)
		gptp->instances[i] = (struct gptp_instance *)(instance + i * instance_size);

	gptp->timer_ctx = (struct timer_ctx *)(instance + domains * instance_size);

	gptp->domain_max = domains;
	gptp->port_max = ports;

	return gptp;

err:
	return NULL;
}

/** Initialize the gptp application (profile, state machines, timers)
* \return	pointer to the main gptp context if success, NULL on failure
* \param cfg	pointer to the config to be applied
* \param priv	platform dependent code private data
*/
__init void *gptp_init(struct fgptp_config *cfg, unsigned long priv)
{
	struct gptp_ctx *gptp;
	struct gptp_net_port *net_port;
	struct gptp_port *port;
	u32 local_time;
	int i, instance_index;
	unsigned int timer_n;
	unsigned int ipc_tx, ipc_tx_sync, ipc_rx;
	unsigned int ipc_tx_mac_service, ipc_rx_mac_service;

	log_level_set(gptp_COMPONENT_ID, cfg->log_level);

	if (gptp_check_config(cfg))
		goto err_config;

	timer_n = CFG_GPTP_MAX_TIMERS_PER_DOMAIN_AND_PORT * cfg->domain_max * cfg->port_max + (CFG_GPTP_MAX_TIMERS_PER_CMLDS_AND_PORT * cfg->port_max);
	gptp = gptp_alloc(cfg->domain_max, cfg->port_max, timer_n);
	if (!gptp)
		goto err_malloc;

	gptp_set_config_parameters(gptp, cfg);

	if (!cfg->is_bridge) {
		if (cfg->logical_port_list[0] == CFG_ENDPOINT_0_LOGICAL_PORT) {
			ipc_tx = IPC_GPTP_MEDIA_STACK;
			ipc_tx_sync = IPC_GPTP_MEDIA_STACK_SYNC;
			ipc_rx = IPC_MEDIA_STACK_GPTP;
		} else {
			ipc_tx = IPC_GPTP_1_MEDIA_STACK;
			ipc_tx_sync = IPC_GPTP_1_MEDIA_STACK_SYNC;
			ipc_rx = IPC_MEDIA_STACK_GPTP_1;
		}
	} else {
		ipc_tx = IPC_GPTP_BRIDGE_MEDIA_STACK;
		ipc_tx_sync = IPC_GPTP_BRIDGE_MEDIA_STACK_SYNC;
		ipc_rx = IPC_MEDIA_STACK_GPTP_BRIDGE;
	}

//	ptp_time_ops_unit_test();

	os_log(LOG_INIT, "gptp(%p) (profile %d - rsync %u - num ports = %u - force_2011 = %u)\n",
	       gptp, cfg->profile, cfg->rsync, gptp->port_max, cfg->force_2011);

	/* check we can get ptp time from hardware */
	if (os_clock_gettime32(gptp->clock_local, &local_time)) {
		os_log(LOG_ERR, "failed to get ptp current time\n");
		goto err_clock_get;
	}

	/*
	* Timer service initialization
	*/
	if (timer_pool_init(gptp->timer_ctx, timer_n, priv) < 0)
		goto err_timer_pool_init;

	if (ipc_tx_init(&gptp->ipc_tx, ipc_tx) < 0)
		goto err_ipc_tx;

	if (ipc_tx_init(&gptp->ipc_tx_sync, ipc_tx_sync) < 0)
		goto err_ipc_tx_sync;

	for (i = 0; i < gptp->port_max; i++) {
		struct net_address addr;
		unsigned int logical_port = cfg->logical_port_list[i];
		net_port = &gptp->net_ports[i];

		addr.ptype = PTYPE_PTP;
		addr.port = logical_port;
		addr.u.ptp.version = 2;
		addr.priority = PTPV2_DEFAULT_PRIORITY;

		net_port->port_id = i;
		net_port->logical_port = logical_port;
		net_port->rx_delay_compensation = cfg->port_cfg[i].rxDelayCompensation;
		net_port->tx_delay_compensation = cfg->port_cfg[i].txDelayCompensation;

		os_log(LOG_INFO, "Port(%u): Delay compensation (rx: %d, tx: %d)\n", i, net_port->rx_delay_compensation, net_port->tx_delay_compensation);

		net_port->initialized = false;

		if (net_rx_init(&net_port->net_rx, &addr, gptp_net_rx, priv) < 0)
			goto err_port_rx;

		if (net_tx_ts_init(&net_port->net_tx, &addr, gptp_hwts_handler, priv) < 0)
			goto err_port_tx;

		if (net_add_multi(&net_port->net_rx, logical_port, ptp_dst_mac) < 0)
			goto err_port_multi;

		net_port->initialized = true;
		continue;

	err_port_multi:
		net_tx_exit(&net_port->net_tx);

	err_port_tx:
		net_rx_exit(&net_port->net_rx);

	err_port_rx:
		continue;
	}

	if (gptp_stats_init_timers(gptp) < 0)
		goto err_stats_init;

	gptp_cmlds_init(gptp, cfg);

	for (instance_index = 0; instance_index < gptp->domain_max; instance_index++) {
		if (gptp_instance_init(gptp, instance_index, cfg) < 0)
			goto err_instance_init;
	}

	gptp_managed_objects_init(&gptp->module, gptp);

	/* For domain 0, in case of static as_capable (e.g.: automotive profile), as_capable is already
	 * (and always) set to TRUE, so all state machines dependending on this flag are started
	 */
	for (i = 0; i < gptp->port_max; i++) {
		port = &gptp->instances[PTP_DOMAIN_0]->ports[i];
		if (!is_common_p2p(port)) {
			if (gptp->net_ports[i].initialized && port->c->as_capable_static)
				gptp_as_capable_across_domains_up(port->c);
		}
	}

	if (gptp->management_enabled) {
		if (!cfg->is_bridge) {
			if (cfg->logical_port_list[0] == CFG_ENDPOINT_0_LOGICAL_PORT) {
				ipc_tx_mac_service = IPC_MEDIA_STACK_MAC_SERVICE;
				ipc_rx_mac_service = IPC_MAC_SERVICE_MEDIA_STACK;
			} else {
				ipc_tx_mac_service = IPC_MEDIA_STACK_MAC_SERVICE_1;
				ipc_rx_mac_service = IPC_MAC_SERVICE_1_MEDIA_STACK;
			}
		} else {
			ipc_tx_mac_service = IPC_MEDIA_STACK_MAC_SERVICE_BRIDGE;
			ipc_rx_mac_service = IPC_MAC_SERVICE_BRIDGE_MEDIA_STACK;
		}

		for (i = 0; i < gptp->port_max; i++)
			gptp_link_down(&gptp->net_ports[i]);

		if (ipc_rx_init(&gptp->ipc_rx_mac_service, ipc_rx_mac_service, gptp_ipc_rx_mac_service, priv) < 0)
			goto err_ipc_rx_mac_service;

		if (ipc_tx_init(&gptp->ipc_tx_mac_service, ipc_tx_mac_service) < 0)
			goto err_ipc_tx_mac_service;

		if (ipc_tx_connect(&gptp->ipc_tx_mac_service, &gptp->ipc_rx_mac_service) < 0)
			goto err_ipc_tx_connect;

		for (i = 0; i < gptp->port_max; i++)
			gptp_ipc_get_mac_status(gptp, &gptp->ipc_tx_mac_service, net_port_to_logical(&gptp->net_ports[i]));
	} else {
		for (i = 0; i < gptp->port_max; i++) {
			net_port = &gptp->net_ports[i];

			if (net_port->initialized)
				gptp_link_up(net_port);
			else
				gptp_link_down(net_port);
		}
	}

	/*
	* Grand Master ID propagation
	* if slave only in static configuration the GM id is already known
	* Note that, for automotive profile, only domain 0 is enabled
	*/
	if (cfg->profile == CFG_GPTP_PROFILE_AUTOMOTIVE)
		gptp_ipc_gm_status(gptp->instances[PTP_DOMAIN_0], &gptp->ipc_tx, IPC_DST_ALL);

	if (ipc_rx_init(&gptp->ipc_rx, ipc_rx, gptp_ipc_rx_media_stack, priv) < 0)
		goto err_ipc_rx;

	return gptp;

err_ipc_rx:
err_ipc_tx_connect:
	if (gptp->management_enabled)
		ipc_tx_exit(&gptp->ipc_tx_mac_service);

err_ipc_tx_mac_service:
	if (gptp->management_enabled)
		ipc_rx_exit(&gptp->ipc_rx_mac_service);

err_ipc_rx_mac_service:
err_instance_init:
	for (i = 0; i < instance_index ; i++)
		gptp_instance_exit(gptp->instances[i]);

err_stats_init:
	for (i = 0; i < gptp->port_max; i++) {
		net_port = &gptp->net_ports[i];

		if (!net_port->initialized)
			continue;

		net_del_multi(&net_port->net_rx, cfg->logical_port_list[i], ptp_dst_mac);
		net_tx_exit(&net_port->net_tx);
		net_rx_exit(&net_port->net_rx);
	}

	ipc_tx_exit(&gptp->ipc_tx_sync);

err_ipc_tx_sync:
	ipc_tx_exit(&gptp->ipc_tx);

err_ipc_tx:
	timer_pool_exit(gptp->timer_ctx);

err_timer_pool_init:
err_clock_get:
	os_free(gptp);

err_malloc:
err_config:
	return NULL;
}


/** Clean-up and exit gptp application
* \return	0 if success, negative error on failure
* \param gptp	pointer to the main gptp context
*/
__exit int gptp_exit(void *gptp_ctx)
{
	struct gptp_ctx *gptp = (struct gptp_ctx *)gptp_ctx;
	int i;

	ipc_rx_exit(&gptp->ipc_rx);

	if (gptp->management_enabled) {
		ipc_tx_exit(&gptp->ipc_tx_mac_service);

		ipc_rx_exit(&gptp->ipc_rx_mac_service);
	}

	for (i = 0; i < gptp->domain_max; i++)
		gptp_instance_exit(gptp->instances[i]);

	gptp_stats_exit_timers( gptp);

	gptp_cmlds_exit(gptp);

	for (i = 0; i < gptp->port_max; i++) {
		struct gptp_net_port *net_port = &gptp->net_ports[i];

		if (net_port->initialized) {
			net_del_multi(&net_port->net_rx, net_port_to_logical(net_port), ptp_dst_mac);
			net_tx_exit(&net_port->net_tx);
			net_rx_exit(&net_port->net_rx);
		}
	}

	ipc_tx_exit(&gptp->ipc_tx);

	ipc_tx_exit(&gptp->ipc_tx_sync);

	timer_pool_exit(gptp->timer_ctx);

	os_free(gptp);

	return 0;
}
