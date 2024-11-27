/*
 * Copyright 2020-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file	msrp_map.c
 * @brief	MSRP MAP implementation.
 * @details
 */

#include "msrp_map.h"
#include "os/fdb.h"
#include "common/fqtss.h"

void listener_registered_clear(struct msrp_stream *stream, unsigned int port_id)
{
	stream->listener_registered_state &= ~(1 << port_id);
}

void listener_registered_set(struct msrp_stream *stream, unsigned int port_id)
{
	stream->listener_registered_state |= (1 << port_id);
}

void listener_declared_clear(struct msrp_stream *stream, unsigned int port_id)
{
	stream->listener_declared_state &= ~(1 << port_id);
}

void listener_declared_set(struct msrp_stream *stream, unsigned int port_id)
{
	stream->listener_declared_state |= (1 << port_id);
}

void listener_user_declared_clear(struct msrp_stream *stream, unsigned int port_id)
{
	stream->listener_user_declared_state &= ~(1 << port_id);
}

void listener_user_declared_set(struct msrp_stream *stream, unsigned int port_id)
{
	stream->listener_user_declared_state |= (1 << port_id);
}

void talker_registered_clear(struct msrp_stream *stream, unsigned int port_id)
{
	stream->talker_registered_state &= ~(1 << port_id);
}

void talker_registered_set(struct msrp_stream *stream, unsigned int port_id)
{
	stream->talker_registered_state |= (1 << port_id);
}

void talker_declared_clear(struct msrp_stream *stream, unsigned int port_id)
{
	stream->talker_declared_state &= ~(1 << port_id);
}

void talker_declared_set(struct msrp_stream *stream, unsigned int port_id)
{
	stream->talker_declared_state |= (1 << port_id);
}

void talker_user_declared_clear(struct msrp_stream *stream, unsigned int port_id)
{
	stream->talker_user_declared_state &= ~(1 << port_id);
}

void talker_user_declared_set(struct msrp_stream *stream, unsigned int port_id)
{
	stream->talker_user_declared_state |= (1 << port_id);
}

static void fdb_set_state(struct msrp_stream *stream, unsigned int port_id, unsigned int state)
{
	stream->fdb_state = (stream->fdb_state & ~(1 << port_id)) | (state << port_id);
}

static int fdb_state_cmp(struct msrp_stream *stream, unsigned int port_id, unsigned int state)
{
	return ((stream->fdb_state & (1 << port_id)) == (state << port_id));
}

static int msrp_fdb_update(struct msrp_stream *stream, struct msrp_port *port, u8 *mac, u16 vid, bool forward)
{
	struct msrp_ctx *msrp = container_of(port, struct msrp_ctx, port[port->port_id]);
	genavb_fdb_port_control_t control;
	unsigned int rc = 0;

	control = forward ? GENAVB_FDB_PORT_CONTROL_FORWARDING : GENAVB_FDB_PORT_CONTROL_FILTERING;

	if (fdb_state_cmp(stream, port->port_id, forward)) {
		os_log(LOG_DEBUG, "port(%u) stream(%016"PRIx64") fdb already set: requested_state(%u) fdb state(0x%04x)\n", port->port_id, ntohll(stream->fv.stream_id), forward, stream->fdb_state);
	} else {
		if (fdb_update(msrp_to_logical_port(msrp, port->port_id), mac, vid, true, control) < 0) {
			rc = -1;
		} else {
			fdb_set_state(stream, port->port_id, forward);
		}
	}

	return rc;
}

static void fqtss_set_state(struct msrp_stream *stream, unsigned int port_id, unsigned int state)
{
	stream->fqtss_state = (stream->fqtss_state & ~(1 << port_id)) | (state << port_id);
}

static int fqtss_state_cmp(struct msrp_stream *stream, unsigned int port_id, unsigned int state)
{
	return ((stream->fqtss_state & (1 << port_id)) == (state << port_id));
}

/** Update port fqtss idle slope
 * \return	0 on success, -1 on error
 * \param msrp	pointer to MSRP port context
 * \param stream	stream class
 * \param fv		stream first value
 * \param add	true add stream/increase idle slope, false remove stream/decrease idle slope
 */
static int msrp_update_fqtss(struct msrp_port *port, struct msrp_stream *stream, struct msrp_pdu_fv_talker_advertise *fv, bool add)
{
	struct msrp_ctx *msrp = container_of(port, struct msrp_ctx, port[port->port_id]);
	unsigned int logical_port_id = msrp_to_logical_port(msrp, port->port_id);
	u16 max_frame_size, max_interval_frames;
	u64 idle_slope;
	int rc = 0;

	if (fqtss_state_cmp(stream, port->port_id, add))
		goto out;

	max_frame_size = ntohs(fv->tspec.max_frame_size);
	max_interval_frames = ntohs(fv->tspec.max_interval_frames);

	idle_slope = srp_tspec_to_idle_slope(logical_port_id, stream->sr_class, max_frame_size, max_interval_frames);

	if (add)
		rc = common_fqtss_stream_add(logical_port_id, &fv->stream_id, ntohs(fv->data_frame.vlan_identifier), fv->priority, idle_slope, msrp->is_bridge);
	else
		rc = common_fqtss_stream_remove(logical_port_id, &fv->stream_id, ntohs(fv->data_frame.vlan_identifier), fv->priority, idle_slope, msrp->is_bridge);

	if (rc < 0) {
		os_log(LOG_ERR, "port(%u) stream_id(%016" PRIx64 ") fqtss %s (%u, %u, %" PRIu64 ") failed\n", port->port_id, htonll(fv->stream_id),
		       add ? "add" : "remove", max_frame_size, max_interval_frames, idle_slope);
	} else {
		fqtss_set_state(stream, port->port_id, add);

		os_log(LOG_INFO, "port(%u) stream_id(%016" PRIx64 ") fqtss %s (%u, %u, %" PRIu64 ")\n", port->port_id, htonll(fv->stream_id),
		       add ? "add" : "remove", max_frame_size, max_interval_frames, idle_slope);
	}

out:
	return rc;
}

/** Determine talker declaration type
 * Examines all talker registrations and determines declaration type and failure code
 * \return	MSRP talker declaration type to declare
 * \param stream	pointer to MSRP stream instance
 */

static msrp_talker_declaration_type_t map_talker_declaration_type(struct msrp_stream *stream, struct msrp_ctx *msrp, u8 *failure_code)
{
	msrp_talker_declaration_type_t declaration_type = MSRP_TALKER_DECLARATION_TYPE_NONE;
	unsigned int count = 0;
	unsigned int i;

	for (i = 0; i < msrp->port_max; i++) {
		if (is_talker_stream_registered(stream, i)) {
			declaration_type = stream->port[i].talker_declaration_type;
			count++;
		}
	}

	if (count >= 2) {
		/* More than one registration, always an error */
		declaration_type = MSRP_TALKER_DECLARATION_TYPE_FAILED;
		*failure_code = STREAM_ID_ALREADY_IN_USE;
	} else if (stream->sr_class == SR_CLASS_NONE) {
		/* Not supported SR class */
		declaration_type = MSRP_TALKER_DECLARATION_TYPE_FAILED;
		*failure_code = REQUESTED_PRIORITY_IS_NOT_AN_SR_CLASS_PRIORITY;
	} else if (count == 0) {
		/* Possibly declared by user */
		declaration_type = MSRP_TALKER_DECLARATION_TYPE_ADVERTISE;
	}

	return declaration_type;
}

/** Merge MSRP listener attribute
 * \return	merged listener declaration type according to IEEE 802.1Q 35.2.4.4.3
 * \param stream	pointer to MSRP stream instance
 * \param port	pointer to the MSRP port
 */
static msrp_listener_declaration_type_t map_listener_registration_merge(struct msrp_stream *stream, struct msrp_port *port)
{
	struct msrp_ctx *msrp = container_of(port, struct msrp_ctx, port[port->port_id]);
	msrp_listener_declaration_type_t listener_declaration_type = MSRP_LISTENER_DECLARATION_TYPE_NONE;
	int i;

	if (is_listener_stream_user_declared_any(stream))
		listener_declaration_type = MSRP_LISTENER_DECLARATION_TYPE_READY;

	for (i = 0; i < msrp->port_max; i++) {
		/* ignore registrations on the port we are declaring */
		/* no mentioned in the standard, but we give "priority" to our own declarations */
		if (port->port_id == i)
			continue;

		if (is_listener_stream_registered(stream, i)) {

			if (listener_declaration_type == MSRP_LISTENER_DECLARATION_TYPE_NONE) {
				listener_declaration_type = stream->port[i].listener_declaration_type;
			} else if (listener_declaration_type == MSRP_LISTENER_DECLARATION_TYPE_READY_FAILED) {
				break;
			} else if (listener_declaration_type == MSRP_LISTENER_DECLARATION_TYPE_ASKING_FAILED) {
				if ((stream->port[i].listener_declaration_type == MSRP_LISTENER_DECLARATION_TYPE_READY) ||
				    (stream->port[i].listener_declaration_type == MSRP_LISTENER_DECLARATION_TYPE_READY_FAILED)) {
					listener_declaration_type = MSRP_LISTENER_DECLARATION_TYPE_READY_FAILED;
					break;
				}
			} else {
				if ((stream->port[i].listener_declaration_type == MSRP_LISTENER_DECLARATION_TYPE_READY_FAILED) ||
				    (stream->port[i].listener_declaration_type == MSRP_LISTENER_DECLARATION_TYPE_ASKING_FAILED)) {
					listener_declaration_type = MSRP_LISTENER_DECLARATION_TYPE_READY_FAILED;
					break;
				}
			}
		}
	}

	os_log(LOG_DEBUG, "port(%u) stream_id(%016" PRIx64 ") declaration_type(%s)\n", port->port_id, ntohll(stream->fv.stream_id), mrp_listener_declaration2string(listener_declaration_type));

	return listener_declaration_type;
}


static void msrp_reservation_destroy(struct msrp_stream *stream, struct msrp_port *port, unsigned short direction)
{
	os_memset(&stream->port[port->port_id].reservations[direction], 0, sizeof(struct msrp_reservation));
}

static void msrp_reservation_create(struct msrp_stream *stream, struct msrp_port *port, unsigned short direction, unsigned short declaration_type)
{
	stream->port[port->port_id].reservations[direction].port_id = port->port_id;
	stream->port[port->port_id].reservations[direction].declaration_type = declaration_type;
	stream->port[port->port_id].reservations[direction].direction = direction;

	if (direction == MSRP_DIRECTION_TALKER) {
		stream->port[port->port_id].reservations[direction].accumulated_latency = stream->fv.accumulated_latency;
		os_memcpy(&stream->port[port->port_id].reservations[direction].failed_bridge_id, stream->fv.failure_info.bridge_id, 8);
		stream->port[port->port_id].reservations[direction].failure_code = stream->fv.failure_info.failure_code;
	}

	stream->port[port->port_id].reservations[direction].valid = 1;

	os_log(LOG_DEBUG, "stream(%016"PRIx64") declaration %u direction %u\n", ntohll(stream->fv.stream_id),declaration_type, direction);
}

static void msrp_ipc_talker_declaration_status(struct msrp_ctx *msrp, unsigned int port_id, struct msrp_stream *stream)
{
	struct ipc_desc *desc;
	struct ipc_msrp_talker_declaration_status *status;

	desc = ipc_alloc(&msrp->ipc_tx, sizeof(struct ipc_msrp_talker_declaration_status));
	if (desc) {
		desc->type = GENAVB_MSG_TALKER_DECLARATION_STATUS;
		desc->len = sizeof(struct ipc_msrp_talker_declaration_status);
		desc->dst = IPC_DST_ALL;

		status = &desc->u.msrp_talker_declaration_status;

		status->port = msrp_to_logical_port(msrp, port_id);
		copy_64(&status->stream_id, &stream->fv.stream_id);

		if (is_talker_stream_declared(stream, port_id)) {
			struct mrp_attribute *declared_talker_attribute = stream->port[port_id].declared_talker_attribute;

			if (declared_talker_attribute) {
				if (declared_talker_attribute->type == MSRP_ATTR_TYPE_TALKER_ADVERTISE) {
					status->declaration_type = TALKER_ADVERTISE;
					os_memset(&status->failure, 0, sizeof(struct msrp_failure_information));
				} else if (declared_talker_attribute->type == MSRP_ATTR_TYPE_TALKER_FAILED) {
					struct msrp_talker_failed_attribute_value *attr_value = (struct msrp_talker_failed_attribute_value *) declared_talker_attribute->val;

					status->declaration_type = TALKER_FAILED;
					os_memcpy(status->failure.bridge_id, attr_value->failure_info.bridge_id, 8);
					status->failure.failure_code = attr_value->failure_info.failure_code;
				}
			}
		} else {
			status->declaration_type = NO_TALKER_DECLARATION;
			os_memset(&status->failure, 0, sizeof(struct msrp_failure_information));
		}

		if (ipc_tx(&msrp->ipc_tx, desc) < 0) {
			os_log(LOG_DEBUG, "msrp(%p) ipc_tx() failed\n", msrp);
			ipc_free(&msrp->ipc_tx, desc);
		}
	} else {
		os_log(LOG_ERR, "msrp(%p) ipc_alloc() failed\n", msrp);
	}
}

/** MAP MSRP talker remove attribute
 * \param stream	pointer to MSRP stream instance
 * \param port	pointer to MSRP port
 */
static void talker_remove_declaration(struct msrp_stream *stream, struct msrp_port *port)
{
	struct msrp_ctx *msrp = container_of(port, struct msrp_ctx, port[port->port_id]);
	struct mrp_attribute *prev_attr = stream->port[port->port_id].declared_talker_attribute;
	struct msrp_pdu_fv_talker_advertise *fv = (struct msrp_pdu_fv_talker_advertise *)prev_attr->val;

	if (msrp->is_bridge)
		msrp_fdb_update(stream, port, fv->data_frame.destination_address, ntohs(fv->data_frame.vlan_identifier), false);

	msrp_update_fqtss(port, stream, (struct msrp_pdu_fv_talker_advertise *)prev_attr->val, false);
	mrp_mad_leave_request(&port->mrp_app, prev_attr->type, prev_attr->val);
	talker_declared_clear(stream, port->port_id);

	msrp_ipc_talker_declaration_status(msrp, port->port_id, stream);

	msrp_reservation_destroy(stream, port, MSRP_DIRECTION_TALKER);
}

/** MAP MSRP talker ask new attribute
 * \param stream	pointer to MSRP stream instance
 * \param port	pointer to MSRP port
 * \param new_declaration_type	requested new declaration
 * \param new_failure_code	If non-zero, the new failure code (set by the stack) for the failed declaration. Otherwise, the stack is not the origin of the failure (if any).
 * \param new	specify if the attribute registration is signaled for the first time
 */
static void talker_update_declaration(struct msrp_stream *stream, struct msrp_port *port, msrp_talker_declaration_type_t new_declaration_type, u8 new_failure_code, bool new)
{
	struct msrp_ctx *msrp = container_of(port, struct msrp_ctx, port[port->port_id]);
	struct mrp_attribute *prev_attr = stream->port[port->port_id].declared_talker_attribute;
	struct msrp_talker_failed_attribute_value attr_value;
	msrp_attribute_type_t attribute_type;
	struct msrp_pdu_fv_talker_advertise *fv;
	bool send_ipc_status = false;
	bool immediate_change = false;

	os_memcpy(&attr_value, &stream->fv, sizeof(struct msrp_talker_failed_attribute_value));

	/* 802.1BA-2011, section 6.4 */
	if ((new_declaration_type == MSRP_TALKER_DECLARATION_TYPE_ADVERTISE) && port->srp_domain_boundary_port[stream->sr_class]) {
		new_declaration_type = MSRP_TALKER_DECLARATION_TYPE_FAILED;
		new_failure_code = EGRESS_PORT_IS_NOT_AVB_CAPABLE;
	}

start:
	if ((new_declaration_type == MSRP_TALKER_DECLARATION_TYPE_ADVERTISE) &&
	    is_listener_stream_registered(stream, port->port_id) &&
	    (stream->port[port->port_id].listener_declaration_type != MSRP_LISTENER_DECLARATION_TYPE_ASKING_FAILED)) {
		if (msrp_update_fqtss(port, stream, (struct msrp_pdu_fv_talker_advertise *)&attr_value, true) < 0) {
			new_declaration_type = MSRP_TALKER_DECLARATION_TYPE_FAILED;
			new_failure_code = INSUFFICIENT_BANDWIDTH_FOR_TRAFFIC_CLASS;
			goto start;
		}

		if (msrp->is_bridge) {
			if (msrp_fdb_update(stream, port, attr_value.data_frame.destination_address, ntohs(attr_value.data_frame.vlan_identifier), true) < 0) {
				new_declaration_type = MSRP_TALKER_DECLARATION_TYPE_FAILED;
				goto start;
			}
		}
	} else {
		if (is_talker_stream_declared(stream, port->port_id)) {
			if (msrp->is_bridge) {
				fv = (struct msrp_pdu_fv_talker_advertise *)prev_attr->val;

				msrp_fdb_update(stream, port, fv->data_frame.destination_address, ntohs(fv->data_frame.vlan_identifier), false);
			}

			msrp_update_fqtss(port, stream, (struct msrp_pdu_fv_talker_advertise *)prev_attr->val, false);
		}
	}

	if (new_declaration_type == MSRP_TALKER_DECLARATION_TYPE_ADVERTISE)
		attribute_type = MSRP_ATTR_TYPE_TALKER_ADVERTISE;
	else
		attribute_type = MSRP_ATTR_TYPE_TALKER_FAILED;

	/* Override the failure code and update the bridge id only if the stack is the origin of the failure (new_failure_code != 0), otherwise keep the value from the first value. */
	if ((new_declaration_type == MSRP_TALKER_DECLARATION_TYPE_FAILED) && (new_failure_code != 0)) {
		/* construct talker attribute value */
		attr_value.failure_info.failure_code = new_failure_code;
		//attr_value.failure_info.bridge_id =...
	}

	/* If already declared with another value, remove it */
	if (is_talker_stream_declared(stream, port->port_id)
		&& msrp_talker_attribute_cmp(prev_attr->type, prev_attr->val, attribute_type, (u8 *)&attr_value)) {
		/* If enabled as configuration option, send the leave event before changing the declaration.
		 * Otherwise, just remove/free the previous attribute internally and send a join event for the new attribute.
		 */
		if (port->flags & MSRP_SEND_LEAVE_ON_DECLARATION_UPDATE) {
			mrp_mad_leave_request(&port->mrp_app, prev_attr->type, prev_attr->val);
		} else {
			immediate_change = true;
			mrp_process_attribute_free_immediate(prev_attr);
		}

		talker_declared_clear(stream, port->port_id);

		send_ipc_status = true;

		msrp_reservation_destroy(stream, port, MSRP_DIRECTION_TALKER);
	}

	/* If not already declared */
	if (!is_talker_stream_declared(stream, port->port_id)) {
		if (immediate_change)
			stream->port[port->port_id].declared_talker_attribute = mrp_mad_join_immediate(&port->mrp_app, attribute_type, (u8 *)&attr_value);
		else
			stream->port[port->port_id].declared_talker_attribute = mrp_mad_join_request(&port->mrp_app, attribute_type, (u8 *)&attr_value, new);

		if (stream->port[port->port_id].declared_talker_attribute) {
			talker_declared_set(stream, port->port_id);

			send_ipc_status = true;

			msrp_reservation_create(stream, port, MSRP_DIRECTION_TALKER, new_declaration_type);
		}
	}

	if (send_ipc_status)
		msrp_ipc_talker_declaration_status(msrp, port->port_id, stream);
}

static void msrp_ipc_listener_declaration_status(struct msrp_ctx *msrp, unsigned int port_id, struct msrp_stream *stream)
{
	struct ipc_desc *desc;
	struct ipc_msrp_listener_declaration_status *status;

	desc = ipc_alloc(&msrp->ipc_tx, sizeof(struct ipc_msrp_listener_declaration_status));
	if (desc) {
		desc->type = GENAVB_MSG_LISTENER_DECLARATION_STATUS;
		desc->len = sizeof(struct ipc_msrp_listener_declaration_status);
		desc->dst = IPC_DST_ALL;

		status = &desc->u.msrp_listener_declaration_status;

		status->port = msrp_to_logical_port(msrp, port_id);
		copy_64(&status->stream_id, &stream->fv.stream_id);

		if (is_listener_stream_declared(stream, port_id)) {
			struct mrp_attribute *declared_listener_attribute = stream->port[port_id].declared_listener_attribute;

			if (declared_listener_attribute) {
				struct msrp_listener_attribute_value *attr_value = (struct msrp_listener_attribute_value *) declared_listener_attribute->val;
				if (attr_value->declaration_type == MSRP_LISTENER_DECLARATION_TYPE_ASKING_FAILED)
					status->declaration_type = LISTENER_FAILED;
				else if (attr_value->declaration_type == MSRP_LISTENER_DECLARATION_TYPE_READY)
					status->declaration_type = LISTENER_READY;
				else if (attr_value->declaration_type == MSRP_LISTENER_DECLARATION_TYPE_READY_FAILED)
					status->declaration_type = LISTENER_READY_FAILED;
			}
		} else {
			status->declaration_type = NO_LISTENER_DECLARATION;
		}

		if (ipc_tx(&msrp->ipc_tx, desc) < 0) {
			os_log(LOG_DEBUG, "msrp(%p) ipc_tx() failed\n", msrp);
			ipc_free(&msrp->ipc_tx, desc);
		}
	} else {
		os_log(LOG_ERR, "msrp(%p) ipc_alloc() failed\n", msrp);
	}
}

/** MAP MSRP listener remove attribute
 * \param stream	pointer to MSRP stream instance
 * \param port	pointer to MSRP port
 */
static void listener_remove_declaration(struct msrp_stream *stream, struct msrp_port *port)
{
	struct msrp_ctx *msrp = container_of(port, struct msrp_ctx, port[port->port_id]);
	struct mrp_attribute *prev_attr = stream->port[port->port_id].declared_listener_attribute;

	if (!msrp->is_bridge)
		msrp_deregister_vlan(port, stream);

	mrp_mad_leave_request(&port->mrp_app, prev_attr->type, prev_attr->val);
	listener_declared_clear(stream, port->port_id);

	msrp_ipc_listener_declaration_status(msrp, port->port_id, stream);

	msrp_reservation_destroy(stream, port, MSRP_DIRECTION_LISTENER);
}

/** MAP MSRP listener ask new attribute
 * \param stream	pointer to MSRP stream instance
 * \param port	pointer to MSRP port
 * \param new_declaration_type	requested new declaration
 * \param new	specify if the attribute registration is signaled for the first time
 */
static void listener_update_declaration(struct msrp_stream *stream, struct msrp_port *port, msrp_listener_declaration_type_t new_declaration_type, bool new)
{
	struct msrp_ctx *msrp = container_of(port, struct msrp_ctx, port[port->port_id]);
	struct mrp_attribute *prev_attr = stream->port[port->port_id].declared_listener_attribute;
	struct msrp_listener_attribute_value attr_value, *prev_value;
	bool send_ipc_status = false;
	bool immediate_change = false;

	/* If talker declaration type is FAILED then listener declaration type should be ASKING_FAILED (according to the spec IEEE 802.1Q 35.2.4.4.a) */
	if (!is_talker_stream_registered(stream, port->port_id) ||
		stream->port[port->port_id].talker_declaration_type != MSRP_TALKER_DECLARATION_TYPE_ADVERTISE) {
		new_declaration_type = MSRP_LISTENER_DECLARATION_TYPE_ASKING_FAILED;
	}

	if (!msrp->is_bridge) {
		/* 802.1Q 35.1.2.2: If the Listener receives a Talker Advertise declaration and the Listener is ready to receive the Stream, the Listener
		shall declare the following in the order specified: (1) An MVRP VLAN membership request (2) An MSRP Listener Ready declaration for the Stream */

		if (new_declaration_type == MSRP_LISTENER_DECLARATION_TYPE_ASKING_FAILED)
			msrp_deregister_vlan(port, stream);
		else
			msrp_register_vlan(port, stream);
	}

	/* If stream already declared with another value, remove it first */
	if (is_listener_stream_declared(stream, port->port_id)) {
		prev_value = (struct msrp_listener_attribute_value *)prev_attr->val;
		if (new_declaration_type != prev_value->declaration_type) {
			/* If enabled as configuration option, send the leave event before changing the declaration.
			 * Otherwise, just remove/free the previous attribute internally and send a join event for the new attribute.
			 */
			if (port->flags & MSRP_SEND_LEAVE_ON_DECLARATION_UPDATE) {
				mrp_mad_leave_request(&port->mrp_app, prev_attr->type, prev_attr->val);
			} else {
				immediate_change = true;
				mrp_process_attribute_free_immediate(prev_attr);
			}

			listener_declared_clear(stream, port->port_id);

			send_ipc_status = true;

			/* Destroy reservation entry */
			msrp_reservation_destroy(stream, port, MSRP_DIRECTION_LISTENER);
		}
	}

	/* If listener stream is not declared in this port, then declare it */
	if (!is_listener_stream_declared(stream, port->port_id)) {
		attr_value.stream_id = stream->fv.stream_id;
		attr_value.declaration_type = new_declaration_type;

		if (immediate_change)
			stream->port[port->port_id].declared_listener_attribute = mrp_mad_join_immediate(&port->mrp_app, MSRP_ATTR_TYPE_LISTENER, (u8 *)&attr_value);
		else
			stream->port[port->port_id].declared_listener_attribute = mrp_mad_join_request(&port->mrp_app, MSRP_ATTR_TYPE_LISTENER, (u8 *)&attr_value, new);

		if (stream->port[port->port_id].declared_listener_attribute) {
			listener_declared_set(stream, port->port_id);

			send_ipc_status = true;

			/* Create a reservation entry */
			msrp_reservation_create(stream, port, MSRP_DIRECTION_LISTENER, new_declaration_type);
		}
	}

	if (send_ipc_status)
		msrp_ipc_listener_declaration_status(msrp, port->port_id, stream);
}

/* Update the MSRP talker stream status for the given MAP context according to the spec IEEE 802.1Q 35.2.4.3
 * \param stream	Stream attribute to be updated
 * \param map	MSRP MAP context pointer
 * \param new	specify if the attribute registration is signaled for the first time
 */
static void msrp_map_update_talker_stream(struct msrp_stream *stream, struct msrp_map *map, bool new)
{
	struct msrp_ctx *msrp = container_of(map, struct msrp_ctx, map[map->map_id]);
	u8 failure_code = 0;
	msrp_talker_declaration_type_t declaration_type;
	int i;

	if (!is_talker_stream_registered_any_forwarding(map, stream) && !is_talker_stream_user_declared_any(stream)) {
		/* If there is no talker attribute registered in any port that is in the forwarding set, then leave talker declarations for all ports */

		for (i = 0; i < msrp->port_max; i++) {
			/* for each port */

			if (is_talker_stream_declared(stream, i))
				talker_remove_declaration(stream, &msrp->port[i]);
		}
	} else {
		/* There is at least one talker port registered in the forwarding set or declared by user */

		declaration_type = map_talker_declaration_type(stream, msrp, &failure_code);

		for (i = 0; i < msrp->port_max; i++) {
			/* for each port */

			if (!is_talker_stream_registered(stream, i) && is_msrp_port_forwarding(map, i)) {
				talker_update_declaration(stream, &msrp->port[i], declaration_type, failure_code, new);
			} else {
				/* Talker is registered or not forwarding, remove declaration */
				if (is_talker_stream_declared(stream, i))
					talker_remove_declaration(stream, &msrp->port[i]);
			}
		}
	}

	os_log(LOG_INFO, "stream_id(%016" PRIx64 ") map(%p) talker_state(R:0x%04x, D:0x%04x)\n", ntohll(stream->fv.stream_id), map, (stream->talker_registered_state & map->forwarding_state), stream->talker_declared_state);
}

/* Update the MSRP listener stream status for the given MAP context according to the standard 802.1Q, section 35.2.4.4
 * \param stream	Stream attribute to be updated
 * \param map	MSRP MAP context pointer
 * \param new	specify if the attribute registration is signaled for the first time
 */
static void msrp_map_update_listener_stream(struct msrp_stream *stream, struct msrp_map *map, bool new)
{
	struct msrp_ctx *msrp = container_of(map, struct msrp_ctx, map[map->map_id]);
	msrp_listener_declaration_type_t declaration_type;
	int i;

	if (!is_listener_stream_registered_any_forwarding(map, stream) && !is_listener_stream_user_declared_any(stream)) {
		/* If there is no listener attribute registered in any port that is in the forwarding set, leave listener declarations for all ports */

		for (i = 0; i < msrp->port_max; i++) {
			/* For each port */
			if (is_listener_stream_declared(stream, i))
				listener_remove_declaration(stream, &msrp->port[i]);
		}

	} else {
		/* There is at least one listener port registered */
		/* Propagate registered listener attributes on ports with registered talkers
		 * 		normally a single talker is registered
		 * 		don't declare listener if it's already registered in that port
		 * 		leave listener declaration first if it will change
		 */
		for (i = 0; i < msrp->port_max; i++) {

			if (((is_talker_stream_registered(stream, i) && is_listener_stream_registered_other(stream, i))
				|| is_listener_stream_user_declared(stream, i))
				&& is_msrp_port_forwarding(map, i)) {
				/* If talker stream is registered in this port and this port is in the forwarding set */
				/* IEEE 802.1Q 35.2.4.4.b */
				declaration_type = map_listener_registration_merge(stream, &msrp->port[i]);
				listener_update_declaration(stream, &msrp->port[i], declaration_type, new);
			} else {
				/* Talker stream is not registered in this port, remove listener declaration, or this port is not in the forwarding set */
				if (is_listener_stream_declared(stream, i))
					listener_remove_declaration(stream, &msrp->port[i]);
			}
		}
	}

	os_log(LOG_INFO, "stream_id(%016" PRIx64 ") map(%p) listener_state(R:0x%04x, D:0x%04x)\n", ntohll(stream->fv.stream_id), map, (stream->listener_registered_state & map->forwarding_state), stream->listener_declared_state);
}

/** Get the MSRP MAP context
* \return	MSRP MAP context (msrp_map) on success, NULL on failure
* \param msrp	MSRP global context structure
*/
struct msrp_map *msrp_get_map_context(struct msrp_ctx *msrp)
{
	/* 802.1Q, section 35.2.4.5 MAP Context for MSRP */
	return &msrp->map[0];
}

/** Exits all MSRP streams
 * \return	none
 * \param msrp_stream	stream to free
 * \param msrp	MSRP context to update
 */
__init void msrp_map_stream_exit(struct msrp_stream *stream, struct msrp_ctx *msrp)
{
	int i;

	for (i = 0; i < msrp->port_max; i++) {
		if (is_talker_stream_declared(stream, i))
			talker_remove_declaration(stream, &msrp->port[i]);

		if (is_listener_stream_declared(stream, i))
			listener_remove_declaration(stream, &msrp->port[i]);
	}

	msrp_stream_free(stream);
}

/** Init the MSRP MAP context
* \param msrp	MSRP global context structure
*/
void msrp_map_init(struct msrp_ctx *msrp)
{
	int i;

	for (i = 0; i < MSRP_MAX_MAP_CONTEXT; i++) {
		list_head_init(&msrp->map[i].streams);
		msrp->map[i].num_streams = 0;
		msrp->map[i].forwarding_state = 0;
		msrp->map[i].map_id = i;
	}

	os_log(LOG_INIT, "done\n");
}

/** Erase the MSRP MAP context
* \param msrp	MSRP global context structure
*/
void msrp_map_exit(struct msrp_ctx *msrp)
{
	struct list_head *entry, *next;
	struct msrp_stream *stream;
	int i;

	for (i = 0; i < MSRP_MAX_MAP_CONTEXT; i++) {

		for (entry = list_first(&msrp->map[i].streams); next = list_next(entry), entry != &msrp->map[i].streams; entry = next) {
			stream = container_of(entry, struct msrp_stream, list);

			msrp_map_stream_exit(stream, msrp);
		}
	}

	os_log(LOG_INIT, "done\n");
}

/* Update the MSRP stream status for the given MAP context
 * \param stream	Stream attribute to be updated
 * \param map	MSRP MAP context pointer
 * \param new	specify if the attribute registration is signaled for the first time
 */
void msrp_map_update_stream(struct msrp_map *map, struct msrp_stream *stream, bool new)
{
	msrp_map_update_talker_stream(stream, map, new);
	msrp_map_update_listener_stream(stream, map, new);

	/* if talker stream is not registered or declared anywhere, it should be deleted */
	if (!is_listener_stream_registered_any(stream) &&
	    !is_listener_stream_declared_any(stream) &&
	    !is_talker_stream_registered_any(stream) &&
	    !is_talker_stream_declared_any(stream) &&
	    !is_listener_stream_user_declared_any(stream) &&
	    !is_talker_stream_user_declared_any(stream))
			msrp_stream_free(stream);
}

/* Update all MSRP stream instance attributes status for the given MAP context
 * \param map	MSRP MAP context pointer
 */
void msrp_map_update(struct msrp_map *map)
{
	struct list_head *entry, *next;
	struct msrp_stream *stream;

	/*For each stream instance*/
	for (entry = list_first(&map->streams); next = list_next(entry), entry != &map->streams; entry = next) {
		stream = container_of(entry, struct msrp_stream, list);

		msrp_map_update_stream(map, stream, false);
	}
}
