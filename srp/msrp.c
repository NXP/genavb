/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
  @file		msrp.c
  @brief	MSRP module implementation.
  @details
*/


#include "os/stdlib.h"
#include "os/string.h"
#include "os/fdb.h"

#include "common/log.h"
#include "common/timer.h"
#include "common/net.h"
#include "common/ipc.h"

#include "msrp.h"
#include "srp.h"
#include "msrp_map.h"

static void msrp_talker_advertise_join_indication(struct mrp_application *app, struct mrp_attribute *attr, bool new);
static void msrp_talker_advertise_leave_indication(struct mrp_application *app, struct mrp_attribute *attr);

static void msrp_talker_failed_join_indication(struct mrp_application *app, struct mrp_attribute *attr, bool new);
static void msrp_talker_failed_leave_indication(struct mrp_application *app, struct mrp_attribute *attr);

static void msrp_listener_join_indication(struct mrp_application *app, struct mrp_attribute *attr, bool new);
static void msrp_listener_leave_indication(struct mrp_application *app, struct mrp_attribute *attr);

static void msrp_domain_join_indication(struct mrp_application *app, struct mrp_attribute *attr, bool new);
static void msrp_domain_leave_indication(struct mrp_application *app, struct mrp_attribute *attr);

static void msrp_register_stream_indication(struct msrp_port *port, struct msrp_stream *stream);
static void msrp_deregister_stream_indication(struct msrp_port *port, struct msrp_stream *stream);

static void msrp_register_attach_indication(struct msrp_port *port, struct msrp_stream *stream);
static void msrp_deregister_attach_indication(struct msrp_port *port, struct msrp_stream *stream);


static const u8 msrp_dst_mac[6] = MC_ADDR_MSRP;

static unsigned char msrp_attr2valuelength[MSRP_ATTR_TYPE_MAX] = {
	[MSRP_ATTR_TYPE_TALKER_ADVERTISE] = sizeof(struct msrp_talker_advertise_attribute_value),
	[MSRP_ATTR_TYPE_TALKER_FAILED] = sizeof(struct msrp_talker_failed_attribute_value),
	[MSRP_ATTR_TYPE_LISTENER] = sizeof(struct msrp_listener_attribute_value),
	[MSRP_ATTR_TYPE_DOMAIN] = sizeof(struct msrp_domain_attribute_value)
};


static unsigned char msrp_attr2length[MSRP_ATTR_TYPE_MAX] = {
	[MSRP_ATTR_TYPE_TALKER_ADVERTISE] = MSRP_ATTR_LEN_TALKER_ADVERTISE,
	[MSRP_ATTR_TYPE_TALKER_FAILED] = MSRP_ATTR_LEN_TALKER_FAILED,
	[MSRP_ATTR_TYPE_LISTENER] = MSRP_ATTR_LEN_LISTENER,
	[MSRP_ATTR_TYPE_DOMAIN] = MSRP_ATTR_LEN_DOMAIN
};


static const char *msrp_attribute_type2string(msrp_attribute_type_t type)
{
	switch (type) {
		case2str(MSRP_ATTR_TYPE_TALKER_ADVERTISE);
		case2str(MSRP_ATTR_TYPE_TALKER_FAILED);
		case2str(MSRP_ATTR_TYPE_LISTENER);
		case2str(MSRP_ATTR_TYPE_DOMAIN);
		default:
			return (char *) "Unknown MSRP Attribute type";
	}
}

static const char *msrp_talker_failure_code2string(msrp_reservation_failure_code_t code)
{
	switch (code) {
	case2str(INSUFFICIENT_BANDWIDTH);
	case2str(INSUFFICIENT_BRIDGE_RESOURCES);
	case2str(INSUFFICIENT_BANDWIDTH_FOR_TRAFFIC_CLASS);
	case2str(STREAM_ID_ALREADY_IN_USE);
	case2str(STREAM_DESTINATION_ADDRESS_ALREADY_IN_USE);
	case2str(STREAM_PREEMPTED_BY_HIGHER_RANK);
	case2str(REPORTED_LATENCY_HAS_CHANGED);
	case2str(EGRESS_PORT_IS_NOT_AVB_CAPABLE);
	case2str(USE_DIFFERENT_DESTINATION_ADDRESS);
	case2str(OUT_OF_MSRP_RESOURCES);
	case2str(OUT_OF_MMRP_RESOURCES);
	case2str(CANNOT_STORE_DESTINATION_ADDRESS);
	case2str(REQUESTED_PRIORITY_IS_NOT_AN_SR_CLASS_PRIORITY);
	case2str(MAX_FRAME_SIZE_TOO_LARGE_FOR_MEDIA);
	case2str(FAN_IN_PORT_LIMIT_REACHED);
	case2str(CHANGE_IN_FIRST_VALUE_FOR_REGISTED_STREAM_ID);
	case2str(VLAN_BLOCKED_ON_EGRESS_PORT);
	case2str(VLAN_TAGGING_DISABLED_ON_EGRESS_PORT);
	case2str(SR_CLASS_PRIORITY_MISMATCH);
	default:
		return (char *) "Unknown Talker Failure code";
	}
}

static struct msrp_port *logical_to_msrp_port(struct msrp_ctx *msrp, unsigned int logical_port)
{
	int i;

	for (i = 0; i < msrp->port_max; i++)
		if (logical_port == msrp->port[i].logical_port) {
			if (msrp->port[i].srp_port->initialized)
				return &msrp->port[i];
			else
				return NULL;
		}

	return NULL;
}

unsigned int msrp_to_logical_port(struct msrp_ctx *msrp, unsigned int port_id)
{
	return msrp->port[port_id].logical_port;
}

static const char *msrp_attribute_type_to_string(unsigned int type)
{
	return msrp_attribute_type2string(type);
}

static unsigned int msrp_is_operational(struct msrp_ctx *msrp, unsigned int port_id)
{
	return msrp->operational_state & (1 << port_id);
}

static void msrp_update_operational_state(struct msrp_ctx *msrp, unsigned int port_id, bool state)
{
	if (!state)
		msrp->operational_state &= ~(1 << port_id);
	else
		msrp->operational_state |= (1 << port_id);

	os_log(LOG_DEBUG, "msrp(%p) operational_state(0x%04x)\n", msrp, msrp->operational_state);
}

static void msrp_update_forwarding_state(struct msrp_ctx *msrp, unsigned int port_id, bool state)
{
	int i;

	for (i = 0; i < MSRP_MAX_MAP_CONTEXT; i++) {
		if (!state)
			msrp->map[i].forwarding_state &= ~(1 << port_id);
		else
			msrp->map[i].forwarding_state |= (1 << port_id);

		os_log(LOG_DEBUG, "msrp_map(%p) forwarding_state(0x%04x)\n", &msrp->map[i], msrp->map[i].forwarding_state);

		msrp_map_update(&msrp->map[i]);
	}
}

static unsigned int msrp_is_enabled(struct msrp_ctx *msrp, unsigned int port_id)
{
	return msrp->enabled_state & (1 << port_id);
}

static void msrp_update_enabled_state(struct msrp_ctx *msrp, unsigned int port_id, bool state)
{
	int i;

	if (!state)
		msrp->enabled_state &= ~(1 << port_id);
	else
		msrp->enabled_state |= (1 << port_id);

	os_log(LOG_DEBUG, "msrp(%p) enabled_state(0x%04x)\n", msrp, msrp->enabled_state);

	for (i = 0; i < MSRP_MAX_MAP_CONTEXT; i++)
		msrp_map_update(&msrp->map[i]);
}

/** Allocate an MSRP packet for transmission
* \return	0 on success, negative value on failure
* \param app	MRP application context
* \param size	packet size to allocate
*/
static struct net_tx_desc *msrp_net_tx_alloc(struct mrp_application *app, unsigned int size)
{
	struct msrp_port *port = container_of(app, struct msrp_port, mrp_app);

	return net_tx_alloc(&port->srp_port->net_tx, size);
}

/** Transmit MSRP packet to the network
* \return	0 on success, negative value on failure
* \param app	MRP application context
* \param desc	descriptor for the packet to transmit
*/
static void msrp_net_tx(struct mrp_application *app, struct net_tx_desc *desc)
{
	struct msrp_port *port = container_of(app, struct msrp_port, mrp_app);
	struct msrp_ctx *msrp = container_of(port, struct msrp_ctx, port[port->port_id]);

	if (!msrp_is_operational(msrp, port->port_id)) {
		os_log(LOG_DEBUG, "port(%u) transmit disabled\n", port->port_id);
		goto err_tx_disabled;
	}

	if (net_tx(&port->srp_port->net_tx, desc) < 0) {
		os_log(LOG_ERR, "port(%u) Cannot transmit packet\n", port->port_id);
		port->num_tx_err++;
		goto err_tx;
	}

	port->num_tx_pkts++;

	return;

err_tx:
err_tx_disabled:
	net_tx_free(desc);
}

static unsigned int msrp_attribute_value_length(unsigned int attribute_type)
{
	return msrp_attr2valuelength[attribute_type];
}

static unsigned int msrp_attribute_length(unsigned int attribute_type)
{
	return msrp_attr2length[attribute_type];
}

static void msrp_vector_add_event(struct mrp_attribute *attr, struct mrp_vector *vector, mrp_protocol_attribute_event_t event)
{
	struct msrp_listener_attribute_value *listener_attr_value;
	struct msrp_domain_attribute_value *domain_attr_value;
	struct msrp_talker_failed_attribute_value *talker_failed_attr_value;
	struct msrp_talker_advertise_attribute_value *talker_advertise_attr_value;

	switch (attr->type) {
	case MSRP_ATTR_TYPE_DOMAIN:
		domain_attr_value = (struct msrp_domain_attribute_value *)attr->val;

		os_log(LOG_INFO, "port(%u) domain(%d, %d, %d) %s %s\n", attr->app->port_id, domain_attr_value->sr_class_id, domain_attr_value->sr_class_priority,
		       ntohs(domain_attr_value->sr_class_vid), msrp_attribute_type2string(attr->type), mrp_attribute_event2string(event));

		mrp_vector_add_event(vector, event);

		break;

	case MSRP_ATTR_TYPE_TALKER_ADVERTISE:
		talker_advertise_attr_value = (struct msrp_talker_advertise_attribute_value *)attr->val;

		os_log(LOG_INFO, "port(%u) stream_id(%016" PRIx64 ") %s %s\n", attr->app->port_id, ntohll(talker_advertise_attr_value->stream_id),
			msrp_attribute_type2string(attr->type), mrp_attribute_event2string(event));

		mrp_vector_add_event(vector, event);

		break;

	case MSRP_ATTR_TYPE_TALKER_FAILED:
		talker_failed_attr_value = (struct msrp_talker_failed_attribute_value *)attr->val;

		os_log(LOG_INFO, "port(%u) stream_id(%016" PRIx64 ") %s %s %s\n", attr->app->port_id, ntohll(talker_failed_attr_value->stream_id),
		msrp_attribute_type2string(attr->type), mrp_attribute_event2string(event),
		msrp_talker_failure_code2string(talker_failed_attr_value->failure_info.failure_code));

		mrp_vector_add_event(vector, event);

		break;

	case MSRP_ATTR_TYPE_LISTENER:
		listener_attr_value = (struct msrp_listener_attribute_value *)attr->val;

		os_log(LOG_INFO, "port(%u) stream_id(%016" PRIx64 ") %s %s %s\n", attr->app->port_id, ntohll(listener_attr_value->stream_id),
			msrp_attribute_type2string(attr->type), mrp_attribute_event2string(event),
			mrp_listener_declaration2string(listener_attr_value->declaration_type));

		mrp_vector_add_event_four(vector, event, listener_attr_value->declaration_type);

		break;

	default:
		break;
	}
}


void msrp_stream_free(struct msrp_stream *stream)
{
	struct msrp_map *map = stream->map;
	struct msrp_ctx *msrp = container_of(map, struct msrp_ctx, map[map->map_id]);

	if (msrp->is_bridge)
		fdb_delete(stream->fv.data_frame.destination_address, ntohs(stream->fv.data_frame.vlan_identifier), true);

	list_del(&stream->list);

	map->num_streams--;

	os_log(LOG_INFO, "stream_id(%016"PRIx64") destroyed, - num streams %d\n", htonll(stream->fv.stream_id),
		map->num_streams);

	os_free(stream);
}

/** Parse and dispatch MSRP join indication
 * \return	none
 * \param attr	pointer to MRP attribute
 * \param new	specify if the join is seen for the first time
 */
static void msrp_mad_join_indication(struct mrp_application *app, struct mrp_attribute *attr, bool new)
{
	switch (attr->type) {
	case MSRP_ATTR_TYPE_LISTENER:
		msrp_listener_join_indication(app, attr, new);
		break;

	case MSRP_ATTR_TYPE_TALKER_ADVERTISE:
		msrp_talker_advertise_join_indication(app, attr, new);
		break;

	case MSRP_ATTR_TYPE_TALKER_FAILED:
		msrp_talker_failed_join_indication(app, attr, new);
		break;

	case MSRP_ATTR_TYPE_DOMAIN:
		msrp_domain_join_indication(app, attr, new);
		break;

	default:
		os_log(LOG_ERR, "invalid attribute type %d\n", attr->type);
		break;
	}
}


/** Parse and dispatch MSRP leave indication
 * \return	none
 * \param attr	pointer to MRP attribute
 */
static void msrp_mad_leave_indication(struct mrp_application *app, struct mrp_attribute *attr)
{
	switch (attr->type) {
	case MSRP_ATTR_TYPE_LISTENER:
		msrp_listener_leave_indication(app, attr);
		break;

	case MSRP_ATTR_TYPE_TALKER_ADVERTISE:
		msrp_talker_advertise_leave_indication(app, attr);
		break;

	case MSRP_ATTR_TYPE_TALKER_FAILED:
		msrp_talker_failed_leave_indication(app, attr);
		break;

	case MSRP_ATTR_TYPE_DOMAIN:
		msrp_domain_leave_indication(app, attr);
		break;

	default:
		os_log(LOG_ERR, "invalid attribute type %d\n", attr->type);
		break;
	}
}

/** MAP MSRP attribute comparison
 * \param attr_type	attribute type of the stream to compare with new attribute
 * \param val	stream attibute value to be compared if attribute type are the same
 * \param new_attr_type	attribute type of the new attribute to compare with the stream attribute
 * \param new_val	new attibute value to be compared if attribute type are the same
 */
int msrp_talker_attribute_cmp(msrp_attribute_type_t attr_type, u8 *val, msrp_attribute_type_t new_attr_type, u8 *new_val)
{
	unsigned int length;

	if (attr_type != new_attr_type)
		return -1;

	if (attr_type == MSRP_ATTR_TYPE_TALKER_ADVERTISE)
		length = sizeof(struct msrp_talker_advertise_attribute_value);
	else
		length = sizeof(struct msrp_talker_failed_attribute_value);

	return (os_memcmp(val, new_val, length));
}

/** Go through all registered MSRP stream instance and find a matching one
 * \return	pointer to the MSRP stream instance, or NULL is not found
 * \param msrp	MSRP main context
 * \param stream_id	64-bit stream identifier value
 */
static struct msrp_stream *msrp_find_stream(struct msrp_map *map, u64 stream_id)
{
	struct list_head *entry;
	struct msrp_stream *stream;

	for (entry = list_first(&map->streams); entry != &map->streams; entry = list_next(entry)) {

		stream = container_of(entry, struct msrp_stream, list);

		if (stream->fv.stream_id == stream_id)
			return stream;
	}

	os_log(LOG_DEBUG, "stream_id(%016"PRIx64") not found\n", ntohll(stream_id));

	return NULL;
}


/** Get MSRP attribute direction
 * \return	stream direction (MSRP_DIRECTION_TALKER or MSRP_DIRECTION_LISTENER)
 * \param attribute_type	MSRP attribute type value
 */
unsigned int msrp_stream_direction(msrp_attribute_type_t attribute_type)
{
	unsigned int direction;

	switch (attribute_type) {
	case MSRP_ATTR_TYPE_LISTENER:
		direction = MSRP_DIRECTION_LISTENER;
		break;

	case MSRP_ATTR_TYPE_TALKER_ADVERTISE:
	case MSRP_ATTR_TYPE_TALKER_FAILED:
	default:
		direction = MSRP_DIRECTION_TALKER;
		break;
	}

	return direction;
}


/** Create and register a new MSRP stream
 * \return	pointer to MSRP stream instance, or NULL on failure
 * \param msrp	pointer to the msrp main context
 * \param fv	pointer to first value field of the the MSRP PDU
 * \param direction	stream direction (MSRP_DIRECTION_TALKER or MSRP_DIRECTION_LISTENER)
 */
static struct msrp_stream *msrp_create_stream(struct msrp_map *map, u64 stream_id)
{
	struct msrp_ctx *msrp = container_of(map, struct msrp_ctx, map[map->map_id]);
	struct msrp_stream *stream;
	unsigned int size;
	int i;

	/* FIXME: not sure stream ID 0 should be processed */
	if (stream_id == 0x0) {
		os_log(LOG_ERR, "NULL stream ID received\n");
		return NULL;
	}

	/* make sure the stream does not exist already */
	stream = msrp_find_stream(map, stream_id);
	if (stream)
		goto exist;

	if (!(map->num_streams < CFG_MSRP_MAX_STREAMS)) {
		os_log(LOG_ERR, "stream_id(%016"PRIx64") creation failed (max streams)\n", ntohll(stream_id));
		goto err;
	}

	size = sizeof(struct msrp_stream) + msrp->port_max * sizeof(struct msrp_stream_port);

	stream = os_malloc(size);
	if (stream == NULL) {
		os_log(LOG_ERR, "stream_id(%016"PRIx64") creation failed (no memory)\n", ntohll(stream_id));
		goto err;
	}

	os_memset(stream, 0, size);

	stream->sr_class = SR_CLASS_NONE;

	/*common for listeners and talkers attributes. Will be overwritten by talker update/merge function but that's not a problem */
	stream->fv.stream_id = stream_id;

	for (i = 0; i < msrp->port_max; i++) {

		stream->port[i].talker_declaration_type = MSRP_TALKER_DECLARATION_TYPE_NONE;
		stream->port[i].listener_declaration_type = MSRP_LISTENER_DECLARATION_TYPE_NONE;

		list_head_init(&stream->port[i].registered_talker_attributes);
		list_head_init(&stream->port[i].registered_listener_attributes);
		stream->port[i].declared_talker_attribute = NULL;
		stream->port[i].declared_listener_attribute = NULL;
	}

	stream->map = map;

	list_add(&map->streams, &stream->list);

	map->num_streams++;

	os_log(LOG_INFO, "stream_id(%016"PRIx64") created, num streams %d\n",
			ntohll(stream_id),
			map->num_streams);

exist:
	/* first value content is updated, conditionally, outside this function */
	return stream;

err:
	return NULL;
}

/** Find a matching MSRP domain by class id, vid and priority
 * \return	pointer to the MSRP domain, or NULL if no matching domain found
 * \param port	pointer to the MSRP port context
 * \param fv	pointer to the first value field of the MSRP domain PDU
 */
static struct msrp_domain *msrp_find_domain(struct msrp_port *port, struct msrp_pdu_fv_domain *fv)
{
	struct list_head *entry;
	struct msrp_domain *domain;

	for (entry = list_first(&port->domains); entry != &port->domains; entry = list_next(entry)) {

		domain = container_of(entry, struct msrp_domain, list);

		//FIXME: maybe simple compare raw 32bits of the domain
		if ((domain->fv.sr_class_id == fv->sr_class_id) && (domain->fv.sr_class_priority == fv->sr_class_priority) && (domain->fv.sr_class_vid == fv->sr_class_vid))
			return domain;
	}

	return NULL;
}

/** Finds SR class for a given priority and vid
 * \return	Sr Class
 * \param port	pointer to the MSRP port context
 * \param priority	priority value
 * \param vlan_id	vlan id value
 */
static sr_class_t msrp_get_sr_class(struct msrp_port *port, unsigned char priority, unsigned short vlan_id)
{
	struct msrp_domain *domain;
	int i;

	for (i = 0; i < CFG_MSRP_MAX_CLASSES; i++) {

		domain = port->domain[i];
		if (!domain)
			continue;

		if ((domain->fv.sr_class_priority == priority) && (domain->fv.sr_class_vid == vlan_id))
			goto found;
	}

	return SR_CLASS_NONE;

found:
	return i;
}


/** Find a matching MSRP domain by class and vid
 * \return	pointer to the MSRP domain, or NULL if not matching domain found
 * \param port	pointer to the MSRP port context
 * \param class	MSRP domain class id value
 * \param vlan_id	MSRP domain vlan id value
 */
static struct msrp_domain *msrp_get_domain_by_class_vid(struct msrp_port *port, unsigned char class, unsigned short vlan_id)
{
	struct list_head *entry;
	struct msrp_domain *domain;

	for (entry = list_first(&port->domains); entry != &port->domains; entry = list_next(entry)) {

		domain = container_of(entry, struct msrp_domain, list);

		if ((domain->fv.sr_class_id == class) && (domain->fv.sr_class_vid == vlan_id))
			return domain;
	}

	return NULL;
}


/** Create and register a new MSRP domain
 * \return	pointer to the MSRP domain, or NULL on failure
 * \param port	pointer to the MSRP port context to domain is to be attached to
 * \param fv	pointer to the MSRP domain first value field
 */
static struct msrp_domain *msrp_create_domain(struct msrp_port *port, struct msrp_pdu_fv_domain *fv)
{
	struct msrp_domain *domain;

	/* make sure this domain attribute does not exist already */
	domain = msrp_find_domain(port, fv);
	if (domain) {
		//os_log(LOG_DEBUG, "domain(%d, %d, %d) already exists\n", domain->fv.sr_class_id, domain->fv.sr_class_priority, ntohs(domain->fv.sr_class_vid));
		goto exist;
	}

	if (!(port->num_domains < CFG_MSRP_MAX_DOMAINS)) {
		os_log(LOG_ERR, "port(%u) domain(%u, %u, %u) creation failed (max domains)\n", port->port_id, fv->sr_class_id, fv->sr_class_priority, ntohs(fv->sr_class_vid));
		goto err;
	}

	domain = os_malloc(sizeof(struct msrp_domain));
	if (domain == NULL) {
		os_log(LOG_ERR, "port(%u) domain(%u, %u, %u) creation failed (no memory)\n", port->port_id, fv->sr_class_id, fv->sr_class_priority, ntohs(fv->sr_class_vid));
		goto err;
	}

	os_memset(domain, 0, sizeof(struct msrp_domain));

	domain->port = port;

	/* fill in domain (for now stored in network order) */
	os_memcpy(&domain->fv, fv, sizeof(struct msrp_pdu_fv_domain));

	list_add(&port->domains, &domain->list);

	port->num_domains++;

	os_log(LOG_INFO, "port(%u) domain(%u, %u, %u) created, num domains %d\n", port->port_id, fv->sr_class_id, fv->sr_class_priority, ntohs(fv->sr_class_vid), port->num_domains);

exist:
	return domain;

err:
	return NULL;
}


/** Free MSRP domain
 * \return	void
 * \param port	pointer to the MSRP port context
 * \param domain	pointer to MSRP domain to be free
 */
static void msrp_free_domain(struct msrp_port *port, struct msrp_domain *domain)
{
	list_del(&domain->list);

	port->num_domains--;

	os_log(LOG_INFO, "port(%u) domain(%u, %u, %u) destroyed - num domains %d\n", port->port_id, domain->fv.sr_class_id, domain->fv.sr_class_priority, ntohs(domain->fv.sr_class_vid), port->num_domains);

	os_free(domain);
}



/** Create and send MSRP domain declaration
 * \return	0 on success, negative value on failure
 * \param port	pointer to the MSRP port context
 * \param class	domain class id value
 * \param prio	domain priority value
 * \param vid	domain vlan id value
 */
static struct msrp_domain *msrp_declare_domain(struct msrp_port *port, unsigned char class_id, unsigned char prio, unsigned short vid)
{
	struct msrp_domain *domain;
	struct msrp_pdu_fv_domain fv_domain;

	fv_domain.sr_class_id = class_id;
	fv_domain.sr_class_priority = prio;
	fv_domain.sr_class_vid = vid;

	if ((domain = msrp_create_domain(port, &fv_domain)) == NULL)
		return NULL;

	mrp_mad_join_request(&port->mrp_app, MSRP_ATTR_TYPE_DOMAIN, (u8 *)&fv_domain, 1);

	return domain;
}

static void msrp_stream_add_talker_attribute(struct msrp_stream *stream, struct mrp_attribute *attr, unsigned int port_id)
{
	struct list_head *entry;

	for (entry = list_first(&stream->port[port_id].registered_talker_attributes); entry != &stream->port[port_id].registered_talker_attributes; entry = list_next(entry))
		if (entry == &attr->list_app)
			return;

	list_add(&stream->port[port_id].registered_talker_attributes, &attr->list_app);
}

static void msrp_stream_remove_talker_attribute(struct msrp_stream *stream, struct mrp_attribute *attr, unsigned int port_id)
{
	struct list_head *entry;

	for (entry = list_first(&stream->port[port_id].registered_talker_attributes); entry != &stream->port[port_id].registered_talker_attributes; entry = list_next(entry))
		if (entry == &attr->list_app) {
			list_del(&attr->list_app);
			return;
		}
}

static void msrp_stream_remove_all_talker_attributes(struct msrp_stream *stream, unsigned int port_id)
{
	struct list_head *entry, *entry_next;

	for (entry = list_first(&stream->port[port_id].registered_talker_attributes); entry_next = list_next(entry), entry != &stream->port[port_id].registered_talker_attributes; entry = entry_next)
		list_del(entry);
}

static void msrp_stream_add_listener_attribute(struct msrp_stream *stream, struct mrp_attribute *attr, unsigned int port_id)
{
	struct list_head *entry;

	for (entry = list_first(&stream->port[port_id].registered_listener_attributes); entry != &stream->port[port_id].registered_listener_attributes; entry = list_next(entry))
		if (entry == &attr->list_app)
			return;

	list_add(&stream->port[port_id].registered_listener_attributes, &attr->list_app);
}

static void msrp_stream_remove_listener_attribute(struct msrp_stream *stream, struct mrp_attribute *attr, unsigned int port_id)
{
	struct list_head *entry;

	for (entry = list_first(&stream->port[port_id].registered_listener_attributes); entry != &stream->port[port_id].registered_listener_attributes; entry = list_next(entry))
		if (entry == &attr->list_app) {
			list_del(&attr->list_app);
			return;
		}
}

static void msrp_stream_remove_all_listener_attributes(struct msrp_stream *stream, unsigned int port_id)
{
	struct list_head *entry, *entry_next;

	for (entry = list_first(&stream->port[port_id].registered_listener_attributes); entry_next = list_next(entry), entry != &stream->port[port_id].registered_listener_attributes; entry = entry_next)
		list_del(entry);

}

/* 802.1Q-2018, 35.2.6 */
static void msrp_stream_leave_immediate(struct msrp_stream *stream, unsigned int port_id, msrp_attribute_type_t attribute_type, unsigned int declaration_type)
{
	struct list_head *entry, *entry_next;
	struct mrp_attribute *attr;

	if (attribute_type == MSRP_ATTR_TYPE_LISTENER) {
		for (entry = list_first(&stream->port[port_id].registered_listener_attributes); entry_next = list_next(entry), entry != &stream->port[port_id].registered_listener_attributes; entry = entry_next) {
			struct msrp_listener_attribute_value *attr_val;

			attr = container_of(entry, struct mrp_attribute, list_app);
			attr_val = (struct msrp_listener_attribute_value *)attr->val;

			if (attr_val->declaration_type != declaration_type) {
				list_del(&attr->list_app);
				mrp_process_attribute_leave_immediate(attr);
			}
		}

	} else {
		for (entry = list_first(&stream->port[port_id].registered_talker_attributes); entry_next = list_next(entry), entry != &stream->port[port_id].registered_talker_attributes; entry = entry_next) {
			attr = container_of(entry, struct mrp_attribute, list_app);

			if (attr->type != attribute_type) {
				list_del(&attr->list_app);
				mrp_process_attribute_leave_immediate(attr);
			}
		}
	}
}


/** Get MRP attribute from MSRP stream
 * \return	pointer to the MRP attribute, or NULL on failure
 * \param stream	pointer to MSRP stream instance
 * \param attribute_type	MSRP attribute type  (talker advertise, talker failed, listener, domain)
 * \param value	MSRP attribute declaration type value
 */
static struct mrp_attribute *msrp_stream_get_registered_attribute(struct msrp_port *port, struct msrp_stream *stream, msrp_attribute_type_t attribute_type, unsigned int declaration_type)
{
	struct list_head *entry;
	struct mrp_attribute *attr;

	if (attribute_type == MSRP_ATTR_TYPE_LISTENER) {
		for (entry = list_first(&stream->port[port->port_id].registered_listener_attributes); entry != &stream->port[port->port_id].registered_listener_attributes; entry = list_next(entry)) {
			struct msrp_listener_attribute_value *attr_val;

			attr = container_of(entry, struct mrp_attribute, list_app);
			attr_val = (struct msrp_listener_attribute_value *)attr->val;

			if (attr_val->declaration_type == declaration_type)
				goto found;
		}

	} else {
		for (entry = list_first(&stream->port[port->port_id].registered_talker_attributes); entry != &stream->port[port->port_id].registered_talker_attributes; entry = list_next(entry)) {
			attr = container_of(entry, struct mrp_attribute, list_app);

			if (attr->type == attribute_type)
				goto found;
		}
	}

	os_log(LOG_DEBUG, "attribute%s %s is not registered\n", msrp_attribute_type2string(attribute_type), (attribute_type == MSRP_ATTR_TYPE_LISTENER)?mrp_listener_declaration2string(declaration_type):"");

	return NULL;

found:
	os_log(LOG_DEBUG, "attribute(%p) %s %s found\n", attr, msrp_attribute_type2string(attribute_type), (attribute_type == MSRP_ATTR_TYPE_LISTENER)?mrp_listener_declaration2string(declaration_type):"");

	return attr;
}

/** Updates stream parameters (based on talker registered attributes)
 * \return	none
 * \param stream	pointer to MSRP stream instance
 * \param attr		pointer to new talker MRP attribute
 */
static void msrp_stream_update(struct msrp_port *port, struct msrp_stream *stream, struct mrp_attribute *attr)
{
	unsigned int len = msrp_attr2valuelength[attr->type];

	if (is_talker_stream_user_declared(stream, port->port_id)) {
		if (os_memcmp(&stream->fv, attr->val, len))
			os_log(LOG_ERR, "stream_id(%016"PRIx64") declared talker attribute doesn't match registered\n", htonll(stream->fv.stream_id));
	} else if (is_talker_stream_registered(stream, port->port_id)) {
		if (os_memcmp(&stream->fv, attr->val, len))
			os_log(LOG_ERR, "stream_id(%016"PRIx64") registered talker attribute changed value\n", htonll(stream->fv.stream_id));
	} else {
		os_memcpy(&stream->fv, attr->val, len);
		stream->sr_class = msrp_get_sr_class(port, stream->fv.priority, stream->fv.data_frame.vlan_identifier);
	}
}

/** Merge MSRP listener attribute
 * \return	0 none
 * \param port	pointer to MSRP port context
 * \param stream	pointer to MSRP stream instance
 */
static void msrp_listener_registration_merge(struct msrp_port *port, struct msrp_stream *stream)
{
	msrp_listener_declaration_type_t listener_declaration_type;
	struct mrp_attribute *attr;

	if ((attr = msrp_stream_get_registered_attribute(port, stream, MSRP_ATTR_TYPE_LISTENER, MSRP_LISTENER_DECLARATION_TYPE_READY_FAILED)))
		listener_declaration_type = MSRP_LISTENER_DECLARATION_TYPE_READY_FAILED;

	else if ((attr = msrp_stream_get_registered_attribute(port, stream, MSRP_ATTR_TYPE_LISTENER, MSRP_LISTENER_DECLARATION_TYPE_ASKING_FAILED))) {
		if ((attr = msrp_stream_get_registered_attribute(port, stream, MSRP_ATTR_TYPE_LISTENER, MSRP_LISTENER_DECLARATION_TYPE_READY)))
			listener_declaration_type = MSRP_LISTENER_DECLARATION_TYPE_READY_FAILED;
		else
			listener_declaration_type = MSRP_LISTENER_DECLARATION_TYPE_ASKING_FAILED;
	}

	else if ((attr = msrp_stream_get_registered_attribute(port, stream, MSRP_ATTR_TYPE_LISTENER, MSRP_LISTENER_DECLARATION_TYPE_READY)))
		listener_declaration_type = MSRP_LISTENER_DECLARATION_TYPE_READY;

	else {
		listener_registered_clear(stream, port->port_id); /* all listeners attributes have left */
		stream->port[port->port_id].listener_declaration_type = MSRP_LISTENER_DECLARATION_TYPE_NONE;
		msrp_deregister_attach_indication(port, stream);

		return;
	}

	if (listener_declaration_type != stream->port[port->port_id].listener_declaration_type) {
		stream->port[port->port_id].listener_declaration_type = listener_declaration_type;
		msrp_register_attach_indication(port, stream);
	}
}

static void msrp_talker_registration_merge(struct msrp_port *port, struct msrp_stream *stream, struct mrp_attribute *new_attr)
{
	msrp_talker_declaration_type_t talker_declaration_type;
	struct msrp_pdu_fv_talker_failed *new_attr_fv;
	struct mrp_attribute *attr;


	if ((attr = msrp_stream_get_registered_attribute(port, stream, MSRP_ATTR_TYPE_TALKER_FAILED, 0))) {
		/* Make sure to always update failure info in stream first value from newly registered talker failed attributes */
		if (new_attr && (new_attr->type == MSRP_ATTR_TYPE_TALKER_FAILED)) {
			new_attr_fv = (struct msrp_pdu_fv_talker_failed *)new_attr->val;
			os_memcpy(&stream->fv.failure_info, &new_attr_fv->failure_info, sizeof(struct msrp_failure_information));
		}

		talker_declaration_type = MSRP_TALKER_DECLARATION_TYPE_FAILED;
	} else if ((attr = msrp_stream_get_registered_attribute(port, stream, MSRP_ATTR_TYPE_TALKER_ADVERTISE, 0))) {
		// FIXME handle more than one advertise registered
		talker_declaration_type = MSRP_TALKER_DECLARATION_TYPE_ADVERTISE;
	} else {
		talker_registered_clear(stream, port->port_id); /* all talker attributes have left */
		stream->port[port->port_id].talker_declaration_type = MSRP_TALKER_DECLARATION_TYPE_NONE;
		msrp_deregister_stream_indication(port, stream);

		return;
	}

	if (talker_declaration_type != stream->port[port->port_id].talker_declaration_type) {
		stream->port[port->port_id].talker_declaration_type = talker_declaration_type;
		msrp_register_stream_indication(port, stream);
	}
}

/** Register a vlan member
* \return	none
* \param port	pointer to the MSRP port context used to retrieved the MVRP context
* \param stream	pointer to the MSRP stream for which a vlan is registered
*/
void msrp_register_vlan(struct msrp_port *port, struct msrp_stream *stream)
{
	struct msrp_ctx *msrp = container_of(port, struct msrp_ctx, port[port->port_id]);

	if (!(stream->vlan_state & MRP_FLAG_DECLARED)) {
		if (mvrp_register_vlan_member(msrp->srp->mvrp, msrp_to_logical_port(msrp, port->port_id), stream->fv.data_frame.vlan_identifier) == GENAVB_SUCCESS)
			stream->vlan_state |= MRP_FLAG_DECLARED;
	}
}

/** Deregister a vlan instance
* \return	none
* \param port	pointer to the MSRP port context used to retrieved the MVRP context
* \param stream pointer to the MSRP stream for which a vlan is deregistered
*/
void msrp_deregister_vlan(struct msrp_port *port, struct msrp_stream *stream)
{
	struct msrp_ctx *msrp = container_of(port, struct msrp_ctx, port[port->port_id]);

	if (stream->vlan_state & MRP_FLAG_DECLARED) {
		if (mvrp_deregister_vlan_member(msrp->srp->mvrp, msrp_to_logical_port(msrp, port->port_id), stream->fv.data_frame.vlan_identifier) == GENAVB_SUCCESS)
			stream->vlan_state &= ~MRP_FLAG_DECLARED;
	}
}

/** Increment stream id value (802.1Q - 35.2.2.8)
 * \return	none
 * \param attr_type	MSRP attribute type (talker advertize, talker failed, listener, domain)
 * \param fv	pointer to MSRP attribute first value to increment
 */
static void msrp_increment_stream(msrp_attribute_type_t attr_type, struct msrp_pdu_fv_talker_failed *fv)
{
	u64 stream_id = ntohll(fv->stream_id);
	u16 uid;

	uid = (u16)(stream_id & 0xffff) + 1;
	stream_id = (stream_id & 0xffffffffffff0000ULL) | uid;

	fv->stream_id = htonll(stream_id);

	if ((attr_type == MSRP_ATTR_TYPE_TALKER_ADVERTISE) || (attr_type == MSRP_ATTR_TYPE_TALKER_FAILED))
		fv->data_frame.destination_address[5]++;
}


/** Increment domain class and priority values (802.1Q - 35.2.2.9)
 * \return	none
 * \param fv	pointer to domain first value to increment
 */
static void msrp_increment_domain(struct msrp_pdu_fv_domain *fv)
{
	fv->sr_class_id++;
	fv->sr_class_priority++;
}

static int msrp_attribute_check(struct mrp_pdu_header *mrp_header)
{
	u8 fv_length;

	if (!mrp_header->attribute_type || (mrp_header->attribute_type >= MSRP_ATTR_TYPE_MAX)) {
		os_log(LOG_ERR, "bad attribute_type(%u)\n", mrp_header->attribute_type);
		goto err;
	}

	fv_length = msrp_attr2length[mrp_header->attribute_type];

	/* check for valid attribute length */
	if (mrp_header->attribute_length != fv_length) {
		os_log(LOG_ERR, "bad attribute_length (%u), expected (%u)\n", mrp_header->attribute_length, fv_length);
		goto err;
	}

	return 0;

err:
	return -1;
}

static int msrp_event_length(struct mrp_pdu_header *mrp_header, unsigned int number_of_values)
{
	unsigned int event_length;

	event_length = (number_of_values + 2) / 3;

	switch (mrp_header->attribute_type) {
	case MSRP_ATTR_TYPE_TALKER_ADVERTISE:
	case MSRP_ATTR_TYPE_TALKER_FAILED:
	case MSRP_ATTR_TYPE_DOMAIN:
	default:
		break;

	case MSRP_ATTR_TYPE_LISTENER:
		event_length += (number_of_values + 3) / 4;
		break;
	}

	return event_length;
}


static int msrp_vector_handler(struct mrp_application *app, struct mrp_pdu_header *mrp_header, void *vector_data, unsigned int number_of_values)
{
	unsigned int number_of_threepacked;
	u8 *three_packed_events, *four_packed_events;
	unsigned int event[3];
	unsigned int declaration_type[4] = {0};
	u8 *attr_val;
	struct msrp_listener_attribute_value listener_attr_value;
	int i;

	number_of_threepacked = (number_of_values + 2) / 3;
	three_packed_events = (u8 *)vector_data + mrp_header->attribute_length;

	switch (mrp_header->attribute_type) {
	case MSRP_ATTR_TYPE_TALKER_ADVERTISE:
	case MSRP_ATTR_TYPE_TALKER_FAILED:
	case MSRP_ATTR_TYPE_LISTENER:
	{
		struct msrp_pdu_fv_listener *fv = (struct msrp_pdu_fv_listener *)vector_data;

		/* only listener attribute uses four packed values */
		four_packed_events = three_packed_events + number_of_threepacked;

		/* walk through all first values within this pdu */
		for (i = 0; i < number_of_values; i++, msrp_increment_stream(mrp_header->attribute_type, (struct msrp_pdu_fv_talker_failed *)fv)) {
			if (!(i % 3)) {
				mrp_get_three_packed_event(*three_packed_events, event);
				three_packed_events++;
			}

			if (mrp_header->attribute_type == MSRP_ATTR_TYPE_LISTENER) {
				if (!(i % 4)) {
					mrp_get_four_packed_event(*four_packed_events, declaration_type);
					four_packed_events++;
				}

				/* 802.1Q - 35.2.2.7.2
				Ignore: The StreamID referenced by FirstValue+n is not defined in the MSRPDU. The AttributeEvent value encoded in the ThreePackedEvent
				shall be [..] ignored on receive */
				if (declaration_type[i % 4] == MSRP_FOUR_PACKED_IGNORE)
					continue;

				listener_attr_value.stream_id = fv->stream_id;
				listener_attr_value.declaration_type = declaration_type[i % 4];
				attr_val = (u8 *)&listener_attr_value;

				os_log(LOG_INFO, "port(%u) stream_id(%016" PRIx64 ") %s %s %s\n", app->port_id, ntohll(fv->stream_id),
					msrp_attribute_type2string(mrp_header->attribute_type), mrp_attribute_event2string(event[i % 3]),
					mrp_listener_declaration2string(declaration_type[i % 4]));
			} else if (mrp_header->attribute_type == MSRP_ATTR_TYPE_TALKER_ADVERTISE) {
				attr_val = (u8 *)fv;

				os_log(LOG_INFO, "port(%u) stream_id(%016" PRIx64 ") %s %s\n", app->port_id, ntohll(fv->stream_id),
					msrp_attribute_type2string(mrp_header->attribute_type), mrp_attribute_event2string(event[i % 3]));
			} else {
				struct msrp_pdu_fv_talker_failed *talker_failed_fv = (struct msrp_pdu_fv_talker_failed *)fv;
				attr_val = (u8 *)fv;

				os_log(LOG_INFO, "port(%u) stream_id(%016" PRIx64 ") %s %s %s\n", app->port_id, ntohll(fv->stream_id),
					msrp_attribute_type2string(mrp_header->attribute_type), mrp_attribute_event2string(event[i % 3]),
					msrp_talker_failure_code2string(talker_failed_fv->failure_info.failure_code));
			}

			mrp_process_attribute(app, mrp_header->attribute_type, attr_val, event[i % 3]);
		}
	}
		break;

	case MSRP_ATTR_TYPE_DOMAIN:
	{
		struct msrp_pdu_fv_domain *fv = (struct msrp_pdu_fv_domain *)vector_data;

		for (i = 0; i < number_of_values; i++, msrp_increment_domain(fv)) {
			if (!(i % 3)) {
				mrp_get_three_packed_event(*three_packed_events, event);
				three_packed_events++;
			}

			os_log(LOG_INFO, "port(%u) domain(%u, %u, %u) %s\n", app->port_id, fv->sr_class_id, fv->sr_class_priority, ntohs(fv->sr_class_vid), mrp_attribute_event2string(event[i % 3]));

			mrp_process_attribute(app, MSRP_ATTR_TYPE_DOMAIN, (u8 *)fv, event[i % 3]);
		}
	}
		break;

	default:
		/* should never happen */
		os_log(LOG_ERR, "unknown attribute_type(%d)\n", mrp_header->attribute_type);
		break;
	}

	return 0;
}

/** Process received MSRP packet
 * \return	0 on success, negative value on failure
 * \param msrp	pointer to MSRP component context
 * \param port_id Identifier of the port
 * \param desc	pointer to the received packet descriptor
 */
int msrp_process_packet(struct msrp_ctx *msrp, unsigned int port_id, struct net_rx_desc *desc)
{
	struct msrp_port *port;

	if (port_id >= msrp->port_max)
		return -1;

	if (!msrp_is_operational(msrp, port_id)) {
		os_log(LOG_DEBUG, "port(%u) msrp(%p) receive disabled\n", port_id, msrp);
		return -1;
	}

	port = &msrp->port[port_id];

	return mrp_process_packet(&port->mrp_app, desc);
}

void msrp_port_status(struct msrp_ctx *msrp, struct ipc_mac_service_status *status)
{
	struct msrp_port *port = logical_to_msrp_port(msrp, status->port_id);

	if (port) {
		os_log(LOG_INFO, "msrp(%p) port(%u) operational (%u)\n", msrp, port->port_id, status->operational);

		msrp_update_operational_state(msrp, port->port_id, status->operational);

		msrp_update_forwarding_state(msrp, port->port_id, status->operational);
	}
}

/** Talker primitive called by the upper layer to declare a given stream i.e. send a talker advertise (802.1Qat - 35.2.3.1.1)
 * \return	0 on success, negative value on failure
 * \param port	pointer to the MSRP port context
 * \param cmd	pointer to IPC descriptor containing talker advertise information
 */
static struct msrp_stream *msrp_register_stream_request(struct msrp_map *map, struct msrp_port *port, struct ipc_msrp_talker_register *cmd)
{
	struct msrp_ctx *msrp = container_of(map, struct msrp_ctx, map[map->map_id]);
	struct msrp_stream *stream;
	struct msrp_domain *domain;
	struct msrp_pdu_fv_talker_failed fv;
	u64 stream_id;

	copy_64(&stream_id, cmd->stream_id);

	os_log(LOG_INFO, "port(%u) stream_id(%016" PRIx64 ") class(%d) vlan_id(%u)\n", port->port_id, htonll(stream_id), cmd->params.stream_class, cmd->params.vlan_id);

	if (!sr_class_enabled(cmd->params.stream_class) || (cmd->params.stream_class == SR_CLASS_NONE)) {
		os_log(LOG_ERR, "port(%u) stream_id(%016" PRIx64 ") invalid class(%d)\n", port->port_id, htonll(stream_id), cmd->params.stream_class);
		goto err;
	}

	/* fill talker advertise fv with parameters from upper layer */
	fv.stream_id = stream_id;
	os_memcpy(&fv.data_frame.destination_address, cmd->params.destination_address, 6);

	/* Use the default domain vid if requested */
	if ((cmd->params.vlan_id == VLAN_VID_DEFAULT) || (htons(cmd->params.vlan_id) == port->domain[cmd->params.stream_class]->fv.sr_class_vid)) {
		/* If using default vlan for the sr class, use default domain parameters */
		fv.data_frame.vlan_identifier = port->domain[cmd->params.stream_class]->fv.sr_class_vid;
		fv.priority = port->domain[cmd->params.stream_class]->fv.sr_class_priority;
	} else {
		/* FIXME, not using default sr class vlan, check if a matching domain exists, otherwise create a new one? */
		fv.data_frame.vlan_identifier = htons(cmd->params.vlan_id);

		domain = msrp_get_domain_by_class_vid(port, sr_class_id(cmd->params.stream_class), fv.data_frame.vlan_identifier);
		if (!domain) {
			domain = msrp_declare_domain(port, sr_class_id(cmd->params.stream_class), sr_class_pcp(cmd->params.stream_class), fv.data_frame.vlan_identifier);
			if (!domain)
				goto err;
		}

		fv.priority = domain->fv.sr_class_priority;
	}

	fv.tspec.max_frame_size = htons(cmd->params.max_frame_size);
	fv.tspec.max_interval_frames = htons(cmd->params.max_interval_frames);
	/* FIXME get traffic class from sr class */
	fv.accumulated_latency = htonl(cmd->params.accumulated_latency + port->latency[0].port_tc_max_latency);
	fv.rank = cmd->params.rank;

	/* unused fields */
	os_memset(&fv.failure_info, 0, sizeof(struct msrp_failure_information));
	fv.reserved = 0;

	stream = msrp_create_stream(map, stream_id);
	if (!stream)  {
		os_log(LOG_ERR, "port(%u) stream_id(%016" PRIx64 ") creation failed\n", port->port_id, ntohll(stream_id));
		goto err;
	}

	if (is_listener_stream_user_declared(stream, port->port_id)) {
		os_log(LOG_ERR, "port(%u) stream_id(%016" PRIx64 ") listener already declared\n", port->port_id, ntohll(stream_id));
		goto err;
	}

	if (is_talker_stream_registered_any(stream) || is_talker_stream_user_declared_any(stream)) {

		if (os_memcmp(&stream->fv, &fv, msrp_attr2length[MSRP_ATTR_TYPE_TALKER_ADVERTISE])) {
			os_log(LOG_ERR, "port(%u) stream_id(%016" PRIx64 ") talker already registered with different value\n", port->port_id, ntohll(stream_id));
			goto err;
		}
	} else {
		stream->sr_class = cmd->params.stream_class;
		os_memcpy(&stream->fv, &fv, msrp_attr2length[MSRP_ATTR_TYPE_TALKER_FAILED]);
	}

	talker_user_declared_set(stream, port->port_id);
	msrp_map_update_stream(map, stream, true);

	/* declare vlan */
	/* Not required by 802.1Q, but required by AVnu_SRP-17 */
	if (!msrp->is_bridge)
		msrp_register_vlan(port, stream);

	/* Check if listener attribute was still registered and ready and notify AVDECC */
	//FIXME : move this in a middle layer which will handle SRP state tracking / upper layer notification
	if (is_listener_stream_registered(stream, port->port_id)
	&& (stream->port[port->port_id].listener_declaration_type != MSRP_LISTENER_DECLARATION_TYPE_ASKING_FAILED))
		msrp_register_attach_indication(port, stream);

	return stream;

err:
	return NULL;
}

/* Caller should check that stream is user declared is_talker_stream_user_declared(), before calling this function. */
static void msrp_deregister_talker_stream(struct msrp_map *map, struct msrp_port *port, struct msrp_stream *stream)
{
	struct msrp_ctx *msrp = container_of(map, struct msrp_ctx, map[map->map_id]);

	talker_user_declared_clear(stream, port->port_id);

	/* remove vlan */
	if (!msrp->is_bridge)
		msrp_deregister_vlan(port, stream);

	msrp_map_update_stream(map, stream, false);

}

/** Talker primitive called by the upper layer to stop advertising a given stream.(802.1Qat - 35.2.3.1.1)
 * \return	0 on success, negative value on failure
 * \param port	pointer to MSRP port context
 * \param stream_id pointer to the 64-bit stream identifier
 */
static int msrp_deregister_stream_request(struct msrp_map *map, struct msrp_port *port, u64 stream_id)
{
	struct msrp_stream *stream;

	os_log(LOG_INFO, "port(%u) stream_id(%016" PRIx64 ")\n", port->port_id, htonll(stream_id));

	stream = msrp_find_stream(map, stream_id);
	if (!stream) {
		os_log(LOG_ERR, "port(%u) stream_id(%016" PRIx64 ") not found\n", port->port_id, htonll(stream_id));
		goto err;
	}

	if (!is_talker_stream_user_declared(stream, port->port_id)) {
		os_log(LOG_ERR, "port(%u) stream_id(%016" PRIx64 ") not declared\n", port->port_id, htonll(stream_id));
		goto err;
	}

	msrp_deregister_talker_stream(map, port, stream);

	return 0;

err:
	return -1;
}

/** Listener primitive called by the upper layer to subscribe to a given stream i.e. send a listener ready (802.1Qat - 35.2.3.1.5).
 * \return	0 on success, negative value on failure
 * \param port	pointer to MSRP port context
 * \param stream_id pointer to the 64-bit stream identifier
 */
static struct msrp_stream *msrp_register_attach_request(struct msrp_map *map, struct msrp_port *port, u64 stream_id)
{
	struct msrp_stream *stream;

	os_log(LOG_INFO, "port(%u) stream_id(%016" PRIx64 ")\n", port->port_id, htonll(stream_id));

	stream = msrp_create_stream(map, stream_id);
	if (!stream) {
		os_log(LOG_ERR, "port(%u) stream_id(%016" PRIx64 ") creation failed\n", port->port_id, htonll(stream_id));
		goto err;
	}

	if (is_talker_stream_user_declared(stream, port->port_id)) {
		os_log(LOG_ERR, "port(%u) stream_id(%016" PRIx64 ") talker already declared\n", port->port_id, htonll(stream_id));
		goto err;
	}

	listener_user_declared_set(stream, port->port_id);
	msrp_map_update_stream(map, stream, true);

	return stream;

err:
	return NULL;
}


/* Caller should check that stream is user declared is_listener_stream_user_declared(), before calling this function. */
static void msrp_deregister_listener_stream(struct msrp_map *map, struct msrp_port *port, struct msrp_stream *stream)
{
	listener_user_declared_clear(stream, port->port_id);

	msrp_map_update_stream(map, stream, false);
}

/** Listener primitive called by the upper layer to withdraw from a given stream (802.1Qat - 35.2.3.1.7)
 * \return	0 on success, negative value on failure
 * \param port	pointer to MSRP port context
 * \param stream_id pointer to the 64-bit stream identifier
 */
static int msrp_deregister_attach_request(struct msrp_map *map, struct msrp_port *port, u64 stream_id)
{
	struct msrp_stream *stream;

	os_log(LOG_INFO, "port(%u) stream_id(%016"PRIx64")\n", port->port_id, htonll(stream_id));

	stream = msrp_find_stream(map, stream_id);
	if (!stream) {
		os_log(LOG_ERR, "port(%u) stream_id(%016" PRIx64 ") not found\n", port->port_id, htonll(stream_id));
		goto err;
	}

	if (!is_listener_stream_user_declared(stream, port->port_id)) {
		os_log(LOG_ERR, "port(%u) stream_id(%016" PRIx64 ") not declared\n", port->port_id, htonll(stream_id));
		goto err;
	}

	msrp_deregister_listener_stream(map, port, stream);

	return 0;

err:
	return -1;
}

/** Deregister all user declared talker and listener streams
 * \return	none
 * \param map	MSRP MAP context pointer
 */
static void msrp_stream_deregister_all(struct msrp_map *map)
{
	struct msrp_ctx *msrp = container_of(map, struct msrp_ctx, map[map->map_id]);
	struct list_head *entry, *next;
	struct msrp_stream *stream;
	int i;

	os_log(LOG_INFO, "msrp(%p): deregister all declared talker and listener streams\n", msrp);

	for (entry = list_first(&map->streams); next = list_next(entry), entry != &map->streams; entry = next) {

		stream = container_of(entry, struct msrp_stream, list);
		for (i = 0; i < msrp->port_max; i++) {
			struct msrp_port *port = &msrp->port[i];

			/* Declared streams are either talker or listener. */
			if (is_talker_stream_user_declared(stream, port->port_id))
				msrp_deregister_talker_stream(map, port, stream);
			else if (is_listener_stream_user_declared(stream, port->port_id))
				msrp_deregister_listener_stream(map, port, stream);
		}
	}
}

static void msrp_ipc_listener_status(struct msrp_ctx *msrp, unsigned int port_id, struct msrp_stream *stream)
{
	struct ipc_desc *desc;
	struct ipc_msrp_listener_status *status;

	/* For endpoints, only send listener status if the application layer declared a listener attribute */
	if (!msrp->is_bridge && !is_listener_stream_user_declared(stream, port_id))
		return;

	/* Send listener status to media stack */
	desc = ipc_alloc(&msrp->ipc_tx, sizeof(struct ipc_msrp_listener_status));
	if (desc) {
		desc->type = GENAVB_MSG_LISTENER_STATUS;
		desc->len = sizeof(struct ipc_msrp_listener_status);
		desc->dst = IPC_DST_ALL;

		status = &desc->u.msrp_listener_status;

		status->port = msrp_to_logical_port(msrp, port_id);
		copy_64(&status->stream_id, &stream->fv.stream_id);

		if (is_talker_stream_registered(stream, port_id)) {
			if (stream->port[port_id].talker_declaration_type == MSRP_TALKER_DECLARATION_TYPE_ADVERTISE)
				status->status = ACTIVE;
			else
				status->status = FAILED;
		} else
			status->status = NO_TALKER;

		if ((status->status == ACTIVE) || (status->status == FAILED)) {
			status->params.stream_class = sr_pcp_class(stream->fv.priority);
			os_memcpy(status->params.destination_address, stream->fv.data_frame.destination_address, 6);
			status->params.vlan_id = ntohs(stream->fv.data_frame.vlan_identifier);
			status->params.max_frame_size = ntohs(stream->fv.tspec.max_frame_size);
			status->params.max_interval_frames = ntohs(stream->fv.tspec.max_interval_frames);
			status->params.accumulated_latency = ntohl(stream->fv.accumulated_latency);
			status->params.rank = stream->fv.rank;

			if (status->status == FAILED) {
				os_memcpy(status->failure.bridge_id, stream->fv.failure_info.bridge_id, 8);
				status->failure.failure_code = stream->fv.failure_info.failure_code;
			} else {
				os_memset(&status->failure, 0,  sizeof(struct msrp_failure_information));
			}
		}

		if (ipc_tx(&msrp->ipc_tx, desc) < 0) {
			os_log(LOG_DEBUG, "msrp(%p) ipc_tx() failed\n", msrp);
			ipc_free(&msrp->ipc_tx, desc);
		}
	} else
		os_log(LOG_ERR, "msrp(%p) ipc_alloc() failed\n", msrp);
}


static void msrp_ipc_talker_status(struct msrp_ctx *msrp, unsigned int port_id, struct msrp_stream *stream)
{
	struct ipc_desc *desc;
	struct ipc_msrp_talker_status *status;

	/* For endpoints, only send talker status if the application layer declared a talker attribute */
	if (!msrp->is_bridge && !is_talker_stream_user_declared(stream, port_id))
		return;

	desc = ipc_alloc(&msrp->ipc_tx, sizeof(struct ipc_msrp_talker_status));
	if (desc) {
		desc->type = GENAVB_MSG_TALKER_STATUS;
		desc->len = sizeof(struct ipc_msrp_talker_status);
		desc->dst = IPC_DST_ALL;

		status = &desc->u.msrp_talker_status;

		status->port = msrp_to_logical_port(msrp, port_id);
		copy_64(&status->stream_id, &stream->fv.stream_id);

		if (is_listener_stream_registered(stream, port_id)) {
			if (stream->port[port_id].listener_declaration_type == MSRP_LISTENER_DECLARATION_TYPE_READY)
				status->status = ACTIVE_LISTENER;
			else if (stream->port[port_id].listener_declaration_type == MSRP_LISTENER_DECLARATION_TYPE_READY_FAILED)
				status->status = ACTIVE_AND_FAILED_LISTENERS;
			else
				status->status = FAILED_LISTENER;
		} else
			status->status = NO_LISTENER;

		if (ipc_tx(&msrp->ipc_tx, desc) < 0) {
			os_log(LOG_DEBUG, "msrp(%p) ipc_tx() failed\n", msrp);
			ipc_free(&msrp->ipc_tx, desc);
		}
	} else
		os_log(LOG_ERR, "msrp(%p) ipc_alloc() failed\n", msrp);
}


/** Talker primitive informing the upper layer that a new available stream being joined by a listener (802.1Qat - 35.2.3.1.6)
 * \return	0 on success, negative value on failure
 * \param port	pointer to the MSRP port context
 * \param stream	pointer to the MSRP stream instance
 */
static void msrp_register_attach_indication(struct msrp_port *port, struct msrp_stream *stream)
{
	struct msrp_ctx *msrp = container_of(port, struct msrp_ctx, port[port->port_id]);

	os_log(LOG_INFO, "port(%u) stream_id(%016" PRIx64 ") declaration type %s\n", port->port_id, htonll(stream->fv.stream_id), mrp_listener_declaration2string(stream->port[port->port_id].listener_declaration_type));

	msrp_ipc_talker_status(msrp, port->port_id, stream);
}



/** Talker primitive informing the upper layer that a stream has been left by a listener (802.1Qat - 35.2.3.1.8)
 * \return	none
 * \param port	pointer to the MSRP port context
 * \param stream	pointer to the MSRP stream instance
 */
static void msrp_deregister_attach_indication(struct msrp_port *port, struct msrp_stream *stream)
{
	struct msrp_ctx *msrp = container_of(port, struct msrp_ctx, port[port->port_id]);

	os_log(LOG_INFO, "port(%u) stream_id(%016" PRIx64 ")\n", port->port_id, htonll(stream->fv.stream_id));

	msrp_ipc_talker_status(msrp, port->port_id, stream);
}



/** Listener primitive informing the upper layer that a new available stream is being advertised by a talker (802.1Qat - 35.2.3.1.2)
 * \return	none
 * \param port	pointer to the MSRP port context
 * \param stream	pointer to the MSRP stream instance
 */
static void msrp_register_stream_indication(struct msrp_port *port, struct msrp_stream *stream)
{
	struct msrp_ctx *msrp = container_of(port, struct msrp_ctx, port[port->port_id]);

	os_log(LOG_INFO, "port(%u) stream_id(%016" PRIx64 ")\n", port->port_id, htonll(stream->fv.stream_id));

#if defined (CFG_MRP_AUTO_LISTENER_REPLY)
	/* we have received a talker advertise, let just reply that we are ready to receive the stream */
	struct msrp_map *map = msrp_get_map_context(msrp);
	msrp_register_attach_request(map, port, &stream->fv.stream_id);
#endif

	msrp_ipc_listener_status(msrp, port->port_id, stream);
}



/** Listener primitive informing the upper layer that a stream is not available anymore (802.1Qat - 35.2.3.1.4).
 * \return	none
 * \param port	pointer to the MSRP port context
 * \param stream	pointer to the MSRP stream instance
 */
static void msrp_deregister_stream_indication(struct msrp_port *port, struct msrp_stream *stream)
{
	struct msrp_ctx *msrp = container_of(port, struct msrp_ctx, port[port->port_id]);

	os_log(LOG_INFO, "port(%u) stream_id(%016" PRIx64 ")\n", port->port_id, htonll(stream->fv.stream_id));

	msrp_ipc_listener_status(msrp, port->port_id, stream);
}



/** Handle talker advertise messages received by the listener entity
 * \return	none
 * \param attr	pointer to MRP attribute
 * \param new	specify if the talker is seen for the first time
 */
static void msrp_talker_advertise_join_indication(struct mrp_application *app, struct mrp_attribute *attr, bool new)
{
	struct msrp_talker_advertise_attribute_value *attr_val = (struct msrp_talker_advertise_attribute_value *)attr->val;
	struct msrp_port *port = container_of(app, struct msrp_port, mrp_app);
	struct msrp_ctx *msrp = container_of(port, struct msrp_ctx, port[port->port_id]);
	struct msrp_map *map = msrp_get_map_context(msrp);
	struct msrp_stream *stream;

	os_log(LOG_INFO, "port(%u) stream_id(%016" PRIx64 ") new(%d)\n", port->port_id, htonll(attr_val->stream_id), new);

	stream = msrp_create_stream(map, attr_val->stream_id);
	if (stream) {
		if (!new)
			msrp_stream_leave_immediate(stream, port->port_id, attr->type, 0);

		msrp_stream_update(port, stream, attr);

		msrp_stream_add_talker_attribute(stream, attr, port->port_id);

		talker_registered_set(stream, port->port_id);

		msrp_talker_registration_merge(port, stream, attr);

		msrp_map_update_stream(map, stream, new);
	}
}



/** Handle talker leave messages received by the listener entity
 * \return	none
 * \param attr	pointer to mrp attribute
 */
static void msrp_talker_advertise_leave_indication(struct mrp_application *app, struct mrp_attribute *attr)
{
	struct msrp_talker_advertise_attribute_value *attr_val = (struct msrp_talker_advertise_attribute_value *)attr->val;
	struct msrp_port *port = container_of(app, struct msrp_port, mrp_app);
	struct msrp_ctx *msrp = container_of(port, struct msrp_ctx, port[port->port_id]);
	struct msrp_map *map = msrp_get_map_context(msrp);
	struct msrp_stream *stream;

	os_log(LOG_INFO, "port(%u) stream_id(%016" PRIx64 ") vlan_id(%u)\n", port->port_id, htonll(attr_val->stream_id), ntohs(attr_val->data_frame.vlan_identifier));

	stream = msrp_find_stream(map, attr_val->stream_id);
	if (stream) {
		msrp_stream_remove_talker_attribute(stream, attr, port->port_id);

		msrp_talker_registration_merge(port, stream, NULL);

		msrp_map_update_stream(map, stream, false);
	}
}

/** Handle talker failed messages received by the listener entity
 * \return	none
 * \param attr	pointer to MRP attribute
 * \param new	specify if the talker is seen for the first time
 */
static void msrp_talker_failed_join_indication(struct mrp_application *app, struct mrp_attribute *attr, bool new)
{
	struct msrp_talker_failed_attribute_value *attr_val = (struct msrp_talker_failed_attribute_value *)attr->val;
	struct msrp_port *port = container_of(app, struct msrp_port, mrp_app);
	struct msrp_ctx *msrp = container_of(port, struct msrp_ctx, port[port->port_id]);
	struct msrp_map *map = msrp_get_map_context(msrp);
	struct msrp_stream *stream;

	os_log(LOG_INFO, "port(%u) stream_id(%016" PRIx64 ") new(%d)\n", port->port_id, htonll(attr_val->stream_id), new);

	stream = msrp_create_stream(map, attr_val->stream_id);
	if (stream) {
		if (!new)
			msrp_stream_leave_immediate(stream, port->port_id, attr->type, 0);

		msrp_stream_update(port, stream, attr);

		msrp_stream_add_talker_attribute(stream, attr, port->port_id);

		talker_registered_set(stream, port->port_id);

		msrp_talker_registration_merge(port, stream, attr);

		msrp_map_update_stream(map, stream, new);
	}
}



/** Handle talker failed leave messages received by the listener entity
 * \return	none
 * \param attr	pointer to mrp attribute
 */
static void msrp_talker_failed_leave_indication(struct mrp_application *app, struct mrp_attribute *attr)
{
	struct msrp_talker_failed_attribute_value *attr_val = (struct msrp_talker_failed_attribute_value *)attr->val;
	struct msrp_port *port = container_of(app, struct msrp_port, mrp_app);
	struct msrp_ctx *msrp = container_of(port, struct msrp_ctx, port[port->port_id]);
	struct msrp_map *map = msrp_get_map_context(msrp);
	struct msrp_stream *stream;

	os_log(LOG_INFO, "port(%u) stream_id(%016" PRIx64 ") vlan_id(%u)\n", port->port_id, htonll(attr_val->stream_id), ntohs(attr_val->data_frame.vlan_identifier));

	stream = msrp_find_stream(map, attr_val->stream_id);
	if (stream) {
		msrp_stream_remove_talker_attribute(stream, attr, port->port_id);

		msrp_talker_registration_merge(port, stream, NULL);

		msrp_map_update_stream(map, stream, false);
	}
}


/** Handle listener join messages received by the Talker (802.1Qat - 35.1.2.1)
 * On receipt of a MAD_Join.indication for a Listener Declaration, the Talker first merges (35.2.4.4.3) the
 * Listener Declarations that it has registered for the same Stream.
 * If the merged ListenerDeclaration is associated with a Stream that the Talker can supply, and the DeclarationType is either Ready
 * or Ready Failed (i.e., one or more Listeners can receive the Stream), the Talker can start the transmission for
 * this Stream immediately. If the merged Listener Declaration is an Asking Failed, the Talker shall stop the
 * transmission for the Stream, if it is transmitting.
 * \return	none
 * \param attr	pointer to MRP attribute
 * \param new	specify if the listener join is seen for the first time
 */
static void msrp_listener_join_indication(struct mrp_application *app, struct mrp_attribute *attr, bool new)
{
	struct msrp_listener_attribute_value *attr_val = (struct msrp_listener_attribute_value *)attr->val;
	struct msrp_port *port = container_of(app, struct msrp_port, mrp_app);
	struct msrp_ctx *msrp = container_of(port, struct msrp_ctx, port[port->port_id]);
	struct msrp_map *map = msrp_get_map_context(msrp);
	struct msrp_stream *stream;

	os_log(LOG_INFO, "port(%u) stream_id(%016" PRIx64 ") %s new(%d)\n", port->port_id, htonll(attr_val->stream_id), mrp_listener_declaration2string(attr_val->declaration_type), new);

	stream = msrp_create_stream(map, attr_val->stream_id);
	if (stream) {
		if (!new)
			msrp_stream_leave_immediate(stream, port->port_id, attr->type, attr_val->declaration_type);

		msrp_stream_add_listener_attribute(stream, attr, port->port_id);

		listener_registered_set(stream, port->port_id);

		msrp_listener_registration_merge(port, stream);

		msrp_map_update_stream(map, stream, new);
	}
}


/** Handle listener leave messages received by the Talker (802.1Qat - 35.1.2.1)
 * On receipt of a MAD_Leave.indication for a Listener Declaration, if the StreamID of the Declaration
 * matches a Stream that the Talker is transmitting, then the Talker shall stop the transmission for this Stream, if
 * it is transmitting
 * \return	none
 * \param attr	pointer to MRP attribute
 */
static void msrp_listener_leave_indication(struct mrp_application *app, struct mrp_attribute *attr)
{
	struct msrp_listener_attribute_value *attr_val = (struct msrp_listener_attribute_value *)attr->val;
	struct msrp_port *port = container_of(app, struct msrp_port, mrp_app);
	struct msrp_ctx *msrp = container_of(port, struct msrp_ctx, port[port->port_id]);
	struct msrp_map *map = msrp_get_map_context(msrp);
	struct msrp_stream *stream;

	os_log(LOG_INFO, "port(%u) stream_id(%016" PRIx64 ") %s\n", port->port_id, htonll(attr_val->stream_id), mrp_listener_declaration2string(attr_val->declaration_type));

	stream = msrp_find_stream(map, attr_val->stream_id);
	if (stream) {
		msrp_stream_remove_listener_attribute(stream, attr, port->port_id);

		msrp_listener_registration_merge(port, stream);

		msrp_map_update_stream(map, stream, false);
	}
}

static void msrp_domain_update_boundary_port(struct msrp_port *port)
{
	struct list_head *entry;
	struct msrp_domain *domain;
	unsigned int mismatching_registration, existing_registration, mismatching_vlan, boundary_port;
	unsigned int sr_class;
	unsigned short sr_class_vid = 0;
	unsigned int update = 0;
	struct msrp_pdu_fv_domain *fv;
	struct msrp_ctx *msrp = container_of(port, struct msrp_ctx, port[port->port_id]);
	struct msrp_map *map = msrp_get_map_context(msrp);

	/* 802.11Q-2011 34.2.1.4 h) */
	for (sr_class = 0; sr_class < CFG_MSRP_MAX_CLASSES; sr_class++) {
		existing_registration = 0;
		mismatching_registration = 0;
		mismatching_vlan = 0;

		if (!sr_class_enabled(sr_class) || (sr_class == SR_CLASS_NONE))
			continue;

		if (port->domain[sr_class])
			fv = &port->domain[sr_class]->fv;
		else
			fv = NULL;

		for (entry = list_first(&port->domains); entry != &port->domains; entry = list_next(entry)) {
			domain = container_of(entry, struct msrp_domain, list);

			if ((domain->state & MRP_FLAG_REGISTERED) && (domain->fv.sr_class_id == sr_class_id(sr_class))) {
				existing_registration = 1;

				if (fv) {
					if (domain->fv.sr_class_priority != sr_class_pcp(sr_class)) {
						mismatching_registration = 1;
					} else if (domain->fv.sr_class_vid != fv->sr_class_vid) {
						sr_class_vid = domain->fv.sr_class_vid;
						mismatching_vlan = 1;
					}
				}
			}
		}

		boundary_port = 0;

		if (port->domain[sr_class]) {
			if (!existing_registration)
				boundary_port = 1;
			else if (mismatching_registration)
				boundary_port = 1;

			if (mismatching_vlan) {
				/* FIXME, should we update the existing talker streams ? */
				mrp_mad_leave_request(&port->mrp_app, MSRP_ATTR_TYPE_DOMAIN, (u8 *)&port->domain[sr_class]->fv);

				port->domain[sr_class] = msrp_declare_domain(port, sr_class_id(sr_class), sr_class_pcp(sr_class), sr_class_vid);
			}
		} else {
			if (existing_registration)
				boundary_port = 1;
		}

		if (port->srp_domain_boundary_port[sr_class] != boundary_port) {
			os_log(LOG_INFO, "port(%u) class(%u), srp boundary %u\n", port->port_id, sr_class, boundary_port);
			port->srp_domain_boundary_port[sr_class] = boundary_port;

			update = 1;
		}
	}
	if (update)
		msrp_map_update(map);
}

static void msrp_domain_join_indication(struct mrp_application *app, struct mrp_attribute *attr, bool new)
{
	struct msrp_domain_attribute_value *attr_val = (struct msrp_domain_attribute_value *)attr->val;
	struct msrp_port *port = container_of(app, struct msrp_port, mrp_app);
	struct msrp_domain *domain;

	os_log(LOG_INFO, "port(%u) domain(%u, %u, %u) new(%d)\n", port->port_id, attr_val->sr_class_id, attr_val->sr_class_priority, ntohs(attr_val->sr_class_vid), new);

	domain = msrp_create_domain(port, (struct msrp_pdu_fv_domain *)attr_val);
	if (domain) {
		domain->state |= MRP_FLAG_REGISTERED;
		msrp_domain_update_boundary_port(port);
	}
}

static void msrp_domain_leave_indication(struct mrp_application *app, struct mrp_attribute *attr)
{
	struct msrp_domain_attribute_value *attr_val = (struct msrp_domain_attribute_value *)attr->val;
	struct msrp_port *port = container_of(app, struct msrp_port, mrp_app);
	struct msrp_domain *domain;

	os_log(LOG_INFO, "port(%u) domain(%u, %u, %u)\n", port->port_id, attr_val->sr_class_id, attr_val->sr_class_priority, ntohs(attr_val->sr_class_vid));

	domain = msrp_find_domain(port, (struct msrp_pdu_fv_domain *)attr_val);
	if (domain) {
		domain->state &= ~MRP_FLAG_REGISTERED;
		msrp_domain_update_boundary_port(port);
	}
}

static void msrp_ipc_listener_response(struct msrp_ctx *msrp, struct ipc_tx *ipc, unsigned int ipc_dst, unsigned int port, u64 stream_id, u32 status)
{
	struct ipc_desc *desc;
	struct ipc_msrp_listener_response *response;

	os_log(LOG_INFO, "stream_id(%016"PRIx64")\n", htonll(stream_id));

	/* Send listener response to media stack */
	desc = ipc_alloc(ipc, sizeof(struct ipc_msrp_listener_response));
	if (desc) {
		desc->dst = ipc_dst;
		desc->type = GENAVB_MSG_LISTENER_RESPONSE;
		desc->len = sizeof(struct ipc_msrp_listener_response);

		response = &desc->u.msrp_listener_response;

		response->port = msrp_to_logical_port(msrp, port);
		copy_64(response->stream_id, &stream_id);
		response->status = status;

		if (ipc_tx(ipc, desc) < 0) {
			os_log(LOG_ERR, "msrp(%p) ipc_tx() failed\n", msrp);
			ipc_free(ipc, desc);
		}
	} else
		os_log(LOG_ERR, "msrp(%p) ipc_alloc() failed\n", msrp);
}

static void msrp_ipc_talker_response(struct msrp_ctx *msrp, struct ipc_tx *ipc, unsigned int ipc_dst, unsigned int port, u64 stream_id, u32 status)
{
	struct ipc_desc *desc;
	struct ipc_msrp_talker_response *response;

	os_log(LOG_INFO, "stream_id(%016"PRIx64")\n", htonll(stream_id));

	/* Send talker response to media stack */
	desc = ipc_alloc(ipc, sizeof(struct ipc_msrp_talker_response));
	if (desc) {
		desc->dst = ipc_dst;
		desc->type = GENAVB_MSG_TALKER_RESPONSE;
		desc->len = sizeof(struct ipc_msrp_talker_response);

		response = &desc->u.msrp_talker_response;

		response->port = msrp_to_logical_port(msrp, port);
		copy_64(response->stream_id, &stream_id);
		response->status = status;

		if (ipc_tx(ipc, desc) < 0) {
			os_log(LOG_ERR, "msrp(%p) ipc_tx() failed\n", msrp);
			ipc_free(ipc, desc);
		}
	} else
		os_log(LOG_ERR, "msrp(%p) ipc_alloc() failed\n", msrp);
}

static void msrp_ipc_error_response(struct msrp_ctx *msrp, struct ipc_tx *ipc, unsigned int ipc_dst, u32 type, u32 len, u32 status)
{
	struct ipc_desc *desc;
	struct ipc_error_response *error;

	/* Send error response to media stack */
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

		if (ipc_tx(ipc, desc) < 0) {
			os_log(LOG_ERR, "msrp(%p) ipc_tx() failed\n", msrp);
			ipc_free(ipc, desc);
		}
	} else
		os_log(LOG_ERR, "msrp(%p) ipc_alloc() failed\n", msrp);
}

static void __msrp_ipc_rx(struct msrp_ctx *msrp, struct ipc_desc *desc, struct ipc_tx *ipc)
{
	struct msrp_port *port;
	struct msrp_stream *stream;
	struct msrp_map *map = msrp_get_map_context(msrp);
	u64 stream_id;
	u32 status;

	switch (desc->type) {
	case GENAVB_MSG_LISTENER_REGISTER:
		if (desc->len != sizeof(struct ipc_msrp_listener_register)) {
			os_log(LOG_ERR, "msrp(%p) wrong length received %u expected %zu\n", msrp, desc->len, sizeof(struct ipc_msrp_listener_register));
			status = GENAVB_ERR_CTRL_LEN;
			goto err;
			break;
		}

		port = logical_to_msrp_port(msrp, desc->u.msrp_listener_register.port);
		if (!port) {
			os_log(LOG_ERR, "msrp(%p) invalid port(%u)\n", msrp, desc->u.msrp_listener_register.port);
			status = GENAVB_ERR_CTRL_INVALID;
			goto err;
		}

		stream_id = get_64(desc->u.msrp_listener_register.stream_id);

		stream = msrp_register_attach_request(map, port, stream_id);

		if (ipc) {
			if (stream)
				status = GENAVB_SUCCESS;
			else
				status = GENAVB_ERR_CTRL_FAILED;

			msrp_ipc_listener_response(msrp, ipc, desc->src, port->port_id, stream_id, status);

			if (stream)
				msrp_ipc_listener_status(msrp, port->port_id, stream);
		}

		break;

	case GENAVB_MSG_LISTENER_DEREGISTER:
		if (desc->len != sizeof(struct ipc_msrp_listener_deregister)){
			os_log(LOG_ERR, "msrp(%p) wrong length received %u expected %zu\n", msrp, desc->len, sizeof(struct ipc_msrp_listener_deregister));
			status = GENAVB_ERR_CTRL_LEN;
			goto err;
			break;
		}

		port = logical_to_msrp_port(msrp, desc->u.msrp_listener_deregister.port);
		if (!port) {
			os_log(LOG_ERR, "msrp(%p) invalid port(%u)\n", msrp, desc->u.msrp_listener_deregister.port);
			status = GENAVB_ERR_CTRL_INVALID;
			goto err;
		}

		stream_id = get_64(desc->u.msrp_listener_deregister.stream_id);

		if (msrp_deregister_attach_request(map, port, stream_id) < 0)
			status = GENAVB_ERR_CTRL_FAILED;
		else
			status = GENAVB_SUCCESS;

		if (ipc)
			msrp_ipc_listener_response(msrp, ipc, desc->src, port->port_id, stream_id, status);

		break;

	case GENAVB_MSG_TALKER_REGISTER:
		if (desc->len != sizeof(struct ipc_msrp_talker_register)){
			os_log(LOG_ERR, "msrp(%p) wrong length received %u expected %zu\n", msrp, desc->len, sizeof(struct ipc_msrp_talker_register));
			status = GENAVB_ERR_CTRL_LEN;
			goto err;
			break;
		}

		port = logical_to_msrp_port(msrp, desc->u.msrp_talker_register.port);
		if (!port) {
			os_log(LOG_ERR, "msrp(%p) invalid port(%u)\n", msrp, desc->u.msrp_talker_register.port);
			status = GENAVB_ERR_CTRL_INVALID;
			goto err;
		}

		stream = msrp_register_stream_request(map, port, &desc->u.msrp_talker_register);

		if (ipc) {
			if (stream)
				status = GENAVB_SUCCESS;
			else
				status = GENAVB_ERR_CTRL_FAILED;

			msrp_ipc_talker_response(msrp, ipc, desc->src, port->port_id, get_64(desc->u.msrp_talker_register.stream_id), status);

			if (stream)
				msrp_ipc_talker_status(msrp, port->port_id, stream);
		}

		break;

	case GENAVB_MSG_TALKER_DEREGISTER:
		if (desc->len != sizeof(struct ipc_msrp_talker_deregister)){
			os_log(LOG_ERR, "msrp(%p) wrong length received %u expected %zu\n", msrp, desc->len, sizeof(struct ipc_msrp_talker_deregister));
			status = GENAVB_ERR_CTRL_LEN;
			goto err;
			break;
		}

		port = logical_to_msrp_port(msrp, desc->u.msrp_talker_deregister.port);
		if (!port) {
			os_log(LOG_ERR, "msrp(%p) invalid port(%u)\n", msrp, desc->u.msrp_talker_deregister.port);
			status = GENAVB_ERR_CTRL_INVALID;
			goto err;
		}

		stream_id = get_64(desc->u.msrp_talker_deregister.stream_id);

		if (msrp_deregister_stream_request(map, port, stream_id) < 0)
			status = GENAVB_ERR_CTRL_FAILED;
		else
			status = GENAVB_SUCCESS;

		if (ipc)
			msrp_ipc_talker_response(msrp, ipc, desc->src, port->port_id, stream_id, status);

		break;

	case GENAVB_MSG_MANAGED_SET:
		os_log(LOG_DEBUG, "GENAVB_MSG_MANAGED_SET\n");
		srp_ipc_managed_set(msrp->srp, ipc, desc->src, desc->u.data, desc->u.data + desc->len);
		break;

	case GENAVB_MSG_MANAGED_GET:
		os_log(LOG_DEBUG, "GENAVB_MSG_MANAGED_GET\n");
		srp_ipc_managed_get(msrp->srp, ipc, desc->src, desc->u.data, desc->u.data + desc->len);
		break;

	case GENAVB_MSG_DEREGISTER_ALL:
		if (desc->len != 0) {
			os_log(LOG_ERR, "msrp(%p) wrong length received %u expected %u\n", msrp, desc->len, 0);
			status = GENAVB_ERR_CTRL_LEN;
			goto err;
			break;
		}

		msrp_stream_deregister_all(map);
		break;

	default:
		os_log(LOG_ERR, "msrp(%p) unknown IPC type %d\n", msrp, desc->type);
		status = GENAVB_ERR_CTRL_INVALID;
		goto err;
		break;
	}

	return;

err:
	if (ipc)
		msrp_ipc_error_response(msrp, ipc, desc->src, desc->type, desc->len, status);
}

static void msrp_ipc_rx(struct ipc_rx const *rx, struct ipc_desc *desc)
{
	struct msrp_ctx *msrp = container_of(rx, struct msrp_ctx, ipc_rx);
	struct ipc_tx *ipc_tx;

	if (desc->flags & IPC_FLAGS_AVB_MSG_SYNC)
		ipc_tx = &msrp->ipc_tx_sync;
	else
		ipc_tx = &msrp->ipc_tx;

	__msrp_ipc_rx(msrp, desc, ipc_tx);

	ipc_free(rx, desc);
}

/** Exits all MSRP domains
 * \return	none
 * \param port	pointer to a MSRP port context
 */
__init static void msrp_exit_domains(struct msrp_port *port)
{
	struct list_head *entry, *entry_next;
	struct msrp_domain *domain;
	unsigned int sr_class;

	for (entry = list_first(&port->domains); entry_next = list_next(entry), entry != &port->domains; entry = entry_next) {
		domain = container_of(entry, struct msrp_domain, list);

		mrp_mad_leave_request(&port->mrp_app, MSRP_ATTR_TYPE_DOMAIN, (u8 *)&domain->fv);

		msrp_free_domain(port, domain);
	}

	for (sr_class = 0; sr_class < CFG_MSRP_MAX_CLASSES; sr_class++)
		port->domain[sr_class] = NULL;
}

__init int msrp_port_enable(struct msrp_port *port)
{
	struct msrp_ctx *msrp = container_of(port, struct msrp_ctx, port[port->port_id]);
	unsigned int sr_class;

	if (msrp_is_enabled(msrp, port->port_id) || !port->srp_port->initialized)
		goto out;

	mrp_enable(&port->mrp_app);

	for (sr_class = 0; sr_class < CFG_MSRP_MAX_CLASSES; sr_class++) {

		/* create default domains supported by this endpoint */
		if (sr_class_enabled(sr_class) && (sr_class != SR_CLASS_NONE)) {
			port->domain[sr_class] = msrp_declare_domain(port, sr_class_id(sr_class), sr_class_pcp(sr_class), port->sr_pvid);
			if (!port->domain[sr_class])
				goto err_domain;
		}
	}

	msrp_domain_update_boundary_port(port);

	if (net_add_multi(&port->srp_port->net_rx, msrp_to_logical_port(msrp, port->port_id), msrp_dst_mac) < 0)
		goto err_multi;

	msrp_update_enabled_state(msrp, port->port_id, true);

	os_log(LOG_INFO, "port(%u) enabled\n", port->port_id);

out:
	return 0;

err_multi:
	msrp_exit_domains(port);

err_domain:
	mrp_disable(&port->mrp_app);

	return -1;
}

__exit void msrp_port_disable(struct msrp_port *port)
{
	struct msrp_ctx *msrp = container_of(port, struct msrp_ctx, port[port->port_id]);
	struct msrp_map *map = msrp_get_map_context(msrp);
	struct list_head *entry, *next;
	struct msrp_stream *stream;

	if (!msrp_is_enabled(msrp, port->port_id) || !port->srp_port->initialized)
		goto out;

	if (net_del_multi(&port->srp_port->net_rx, msrp_to_logical_port(msrp, port->port_id), msrp_dst_mac) < 0)
		os_log(LOG_ERR, "port(%u) cannot remove msrp multicast\n", port->port_id);

	msrp_exit_domains(port);

	for (entry = list_first(&map->streams); next = list_next(entry), entry != &map->streams; entry = next) {
		stream = container_of(entry, struct msrp_stream, list);

		msrp_stream_remove_all_listener_attributes(stream, port->port_id);
		msrp_stream_remove_all_talker_attributes(stream, port->port_id);

		msrp_listener_registration_merge(port, stream);

		msrp_talker_registration_merge(port, stream, NULL);
	}

	msrp_update_enabled_state(msrp, port->port_id, false);

	mrp_disable(&port->mrp_app);

	os_log(LOG_INFO, "port(%u) disabled\n", port->port_id);

out:
	return;
}

/** MSRP component initialization.
 * initialize the MRP layer for MSRP, the state machines, streams and domains lists
 * \return	0 on success, negative value on failure
 * \param port	pointer to MSRP port context
 */
__init void msrp_enable(struct msrp_ctx *msrp)
{
	int i;

	if (msrp->msrp_enabled_status)
		goto out;

	for (i = 0; i < msrp->port_max; i++)
		if (msrp->port[i].msrp_port_enabled_status)
			msrp_port_enable(&msrp->port[i]);

	msrp->msrp_enabled_status = true;

	os_log(LOG_INFO, "msrp(%p) enabled\n", msrp);

out:
	return;
}

__exit void msrp_disable(struct msrp_ctx *msrp)
{
	int i;

	if (!msrp->msrp_enabled_status)
		goto out;

	for (i = 0; i < msrp->port_max; i++)
		if (msrp->port[i].msrp_port_enabled_status)
			msrp_port_disable(&msrp->port[i]);

	msrp->msrp_enabled_status = false;

	os_log(LOG_INFO, "msrp(%p) disabled\n", msrp);

out:
	return;
}

__init static int msrp_port_init(struct msrp_port *port, unsigned int port_id, struct msrp_config *cfg)
{
	struct msrp_ctx *msrp = container_of(port, struct msrp_ctx, port[port_id]);
	struct srp_ctx *srp = msrp->srp;
	unsigned int sr_class;
	int i;

	port->port_id = port_id;

	port->logical_port = cfg->logical_port_list[port_id];

	port->srp_port = &srp->port[port_id];

	port->flags = cfg->flags;

	for (i = 0; i < CFG_TRAFFIC_CLASS_MAX; i++) {
		port->latency[i].traffic_class = i;
		port->latency[i].port_id = port_id;
		port->latency[i].port_tc_max_latency = CFG_PORT_TC_MAX_LATENCY;
	}

	/* initialize MSRP state machines */
	port->mrp_app.srp = msrp->srp;
	port->mrp_app.port_id = port_id;
	port->mrp_app.attribute_type_to_string = msrp_attribute_type_to_string;
	port->mrp_app.mad_join_indication = msrp_mad_join_indication;
	port->mrp_app.mad_leave_indication = msrp_mad_leave_indication;
	port->mrp_app.attribute_length = msrp_attribute_length;
	port->mrp_app.attribute_value_length = msrp_attribute_value_length;
	port->mrp_app.vector_add_event = msrp_vector_add_event;
	port->mrp_app.net_tx = msrp_net_tx;
	port->mrp_app.net_tx_alloc = msrp_net_tx_alloc;

	port->mrp_app.dst_mac = msrp_dst_mac;
	port->mrp_app.ethertype = ETHERTYPE_MSRP;
	port->mrp_app.min_attr_type = MSRP_ATTR_TYPE_TALKER_ADVERTISE;
	port->mrp_app.max_attr_type = MSRP_ATTR_TYPE_DOMAIN;
	port->mrp_app.proto_version = MSRP_PROTO_VERSION;
	port->mrp_app.has_attribute_list_length = 1;

	port->mrp_app.attribute_check = msrp_attribute_check;
	port->mrp_app.event_length = msrp_event_length;
	port->mrp_app.vector_handler = msrp_vector_handler;

	if (mrp_init(&port->mrp_app, APP_MSRP, CFG_MSRP_PARTICIPANT_TYPE) < 0)
		goto err_mrp;

	mrp_disable(&port->mrp_app);

	list_head_init(&port->domains);

	port->sr_pvid = CFG_MVRP_VID;

	for (sr_class = 0; sr_class < CFG_MSRP_MAX_CLASSES; sr_class++)
		port->oper_idle_slope[sr_class] = 0;

	/* all ports enabled by default */
	port->msrp_port_enabled_status = true;

	os_log(LOG_INIT, "port(%u) done\n", port_id);

	return 0;

err_mrp:
	return -1;
}

__exit static void msrp_port_exit(struct msrp_port *port)
{
	msrp_port_disable(port);

	mrp_exit(&port->mrp_app);
}

__init int msrp_init(struct msrp_ctx *msrp, struct msrp_config *cfg, unsigned long priv)
{
	unsigned int ipc_tx, ipc_tx_sync, ipc_rx;
	int i, j;

	msrp->port_max = cfg->port_max;
	msrp->is_bridge = cfg->is_bridge;

	if (!msrp->is_bridge) {
		if (cfg->logical_port_list[0] == CFG_ENDPOINT_0_LOGICAL_PORT) {
			ipc_tx = IPC_MSRP_MEDIA_STACK;
			ipc_tx_sync = IPC_MSRP_MEDIA_STACK_SYNC;
			ipc_rx = IPC_MEDIA_STACK_MSRP;
		} else {
			ipc_tx = IPC_MSRP_1_MEDIA_STACK;
			ipc_tx_sync = IPC_MSRP_1_MEDIA_STACK_SYNC;
			ipc_rx = IPC_MEDIA_STACK_MSRP_1;
		}
	} else {
		ipc_tx = IPC_MSRP_BRIDGE_MEDIA_STACK;
		ipc_tx_sync = IPC_MSRP_BRIDGE_MEDIA_STACK_SYNC;
		ipc_rx = IPC_MEDIA_STACK_MSRP_BRIDGE;
	}

	if (ipc_rx_init(&msrp->ipc_rx, ipc_rx, msrp_ipc_rx, priv) < 0)
		goto err_ipc_rx;

	if (ipc_tx_init(&msrp->ipc_tx, ipc_tx) < 0)
		goto err_ipc_tx;

	if (ipc_tx_init(&msrp->ipc_tx_sync, ipc_tx_sync) < 0)
		goto err_ipc_tx_sync;

	msrp_map_init(msrp);

	for (i = 0; i < msrp->port_max; i++)
		if (msrp_port_init(&msrp->port[i], i, cfg) < 0)
			goto err_msrp_init;

	if (cfg->enabled)
		msrp_enable(msrp);

	os_log(LOG_INIT, "msrp(%p) done\n", msrp);

	return 0;

err_msrp_init:
	for (j = 0; j < i; j++)
		msrp_port_exit(&msrp->port[j]);

err_ipc_tx_sync:
	ipc_tx_exit(&msrp->ipc_tx);

err_ipc_tx:
	ipc_rx_exit(&msrp->ipc_rx);

err_ipc_rx:
	return -1;
}


/** MSRP component clean-up
 * Goes through stream and domain lists and destroy pending instance
 * \return	0 on success, negative value on failure
 * \param msrp	pointer to MSRP context
 */
__exit int msrp_exit(struct msrp_ctx *msrp)
{
	int i;

	msrp_disable(msrp);

	for (i = 0; i < msrp->port_max; i++)
		msrp_port_exit(&msrp->port[i]);

	msrp_map_exit(msrp);

	ipc_tx_exit(&msrp->ipc_tx_sync);

	ipc_tx_exit(&msrp->ipc_tx);

	ipc_rx_exit(&msrp->ipc_rx);

	os_log(LOG_INIT, "msrp(%p) done\n", msrp);

	return 0;
}
