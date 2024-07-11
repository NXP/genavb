/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
  @file		mvrp.c
  @brief	MVRP module implementation.
  @details
*/


#include "os/stdlib.h"
#include "os/string.h"
#include "os/vlan.h"

#include "common/log.h"
#include "common/net.h"
#include "common/srp.h"

#include "srp/mvrp.h"
#include "srp/srp.h"
#include "srp/mvrp_map.h"

static const u8 mvrp_dst_mac[6] = MC_ADDR_MVRP;

static struct mvrp_port *logical_to_mvrp_port(struct mvrp_ctx *mvrp, unsigned int logical_port)
{
	int i;

	for (i = 0; i < mvrp->port_max; i++)
		if (logical_port == mvrp->port[i].logical_port) {
			if (mvrp->port[i].srp_port->initialized)
				return &mvrp->port[i];
			else
				return NULL;
		}

	return NULL;
}

static unsigned int mvrp_to_logical_port(struct mvrp_ctx *mvrp, unsigned int port_id)
{
	return mvrp->port[port_id].logical_port;
}

static const char *mvrp_attribute_type_to_string(unsigned int type)
{
	switch (type) {
		case2str(MVRP_ATTR_TYPE_VID);
	default:
		return "Unknown MVRP Attribute type";
	}
}

static unsigned int mvrp_is_operational(struct mvrp_ctx *mvrp, unsigned int port_id)
{
	return mvrp->operational_state & (1 << port_id);
}

static void mvrp_update_operational_state(struct mvrp_ctx *mvrp, unsigned int port_id, bool state)
{
	if (!state)
		mvrp->operational_state &= ~(1 << port_id);
	else
		mvrp->operational_state |= (1 << port_id);

	os_log(LOG_DEBUG, "mvrp(%p) operational_state(0x%04x)\n", mvrp, mvrp->operational_state);
}

static void mvrp_update_forwarding_state(struct mvrp_ctx *mvrp, unsigned int port_id, bool state)
{
	int i;

	for (i = 0; i < MVRP_MAX_MAP_CONTEXT; i++) {
		if (!state)
			mvrp->map[i].forwarding_state &= ~(1 << port_id);
		else
			mvrp->map[i].forwarding_state |= (1 << port_id);

		os_log(LOG_DEBUG, "mvrp_map(%p) forwarding_state(0x%04x)\n", &mvrp->map[i], mvrp->map[i].forwarding_state);

		mvrp_map_update(&mvrp->map[i]);
	}
}

/** Allocate an MSRP packet for transmission
* \return	0 on success, negative value on failure
* \param app	MRP application context
* \param size	packet size to allocate
*/
static struct net_tx_desc *mvrp_net_tx_alloc(struct mrp_application *app, unsigned int size)
{
	struct mvrp_port *port = container_of(app, struct mvrp_port, mrp_app);

	return net_tx_alloc(&port->srp_port->net_tx, size);
}

/** Transmit MVRP packet to the network
* \return	0 on success, negative value on failure
* \param app	MRP application context
* \param desc	descriptor of the packet to transmit
*/
static void mvrp_net_tx(struct mrp_application *app, struct net_tx_desc *desc)
{
	struct mvrp_port *port = container_of(app, struct mvrp_port, mrp_app);
	struct mvrp_ctx *mvrp = container_of(port, struct mvrp_ctx, port[port->port_id]);

	if (!mvrp_is_operational(mvrp, port->port_id)) {
		os_log(LOG_DEBUG, "port(%u) mvrp(%p) transmit disabled\n", app->port_id, mvrp);
		goto err_tx_disabled;
	}

	if (net_tx(&port->srp_port->net_tx, desc) < 0) {
		os_log(LOG_ERR,"port(%p) Cannot transmit packet\n", port);
		port->num_tx_err++;
		goto err_tx;
	}

	port->num_tx_pkts++;

	return;

err_tx:
err_tx_disabled:
	net_tx_free(desc);
}

static unsigned int mvrp_attribute_value_length(unsigned int attribute_type)
{
	return sizeof(struct mvrp_attribute_value);
}

static unsigned int mvrp_attribute_length(unsigned int attribute_type)
{
	return MVRP_ATTR_LEN;
}

static void mvrp_vector_add_event(struct mrp_attribute *attr, struct mrp_vector *vector, mrp_protocol_attribute_event_t event)
{
	struct mvrp_attribute_value *attr_val = (struct mvrp_attribute_value *)attr->val;

	mrp_vector_add_event(vector, event);

	os_log(LOG_INFO, "port(%u) vlan_id(%u) MVRP_ATTR_TYPE_VID %s\n", attr->app->port_id, ntohs(attr_val->vid), mrp_attribute_event2string(event));
}

/** Find a vlan instance
 * \return	pointer to vlan instance on NULL if no matching vlan is found
 * \param map	pointer to the MVRP MAP context
 * \param vlan_id	vlan tag identifier
 */
static struct mvrp_vlan *mvrp_find_vlan(struct mvrp_map *map, unsigned short vlan_id)
{
	struct list_head *entry;
	struct mvrp_vlan *vlan;

	/* walk through the map client chained list to find an entry matching the specifed vlan ID */
	for (entry = list_first(&map->vlans); entry != &map->vlans; entry = list_next(entry)) {

		vlan = container_of(entry, struct mvrp_vlan, list);

		if (vlan->vlan_id == vlan_id)
			return vlan;
	}

	return NULL;
}

/** Create and register a vlan instance in the MVRP context
 * \return	pointer to the created vlan instance or NULL on failure
 * \param map	pointer to the MVRP MAP context
 * \param vlan_id	vlan tag identifier
 */
static struct mvrp_vlan *mvrp_create_vlan(struct mvrp_map *map, unsigned short vlan_id)
{
	struct mvrp_ctx *mvrp = container_of(map, struct mvrp_ctx, map[map->map_id]);
	struct mvrp_vlan *vlan;
	unsigned int size;

	vlan = mvrp_find_vlan(map, vlan_id);
	if (vlan)
		goto exist;

	if (!(map->num_vlans < CFG_MVRP_MAX_VLANS)) {
		os_log(LOG_ERR, "vlan_id(%u) creation failed (max vlans)\n", ntohs(vlan_id));
		goto err;
	}

	if ((ntohs(vlan_id) < CFG_MVRP_VID_MIN) || (ntohs(vlan_id) > CFG_MVRP_VID_MAX)) {
		os_log(LOG_ERR, "vlan_id(%u) creation failed (out of range)\n", ntohs(vlan_id));
		goto err;
	}

	size = sizeof(struct mvrp_vlan) + mvrp->port_max * sizeof(unsigned int);

	vlan = os_malloc(size);
	if (vlan == NULL) {
		os_log(LOG_ERR, "vlan_id(%u) creation failed (no memory)\n", ntohs(vlan_id));
		goto err;
	}

	os_memset(vlan, 0, size);

	vlan->vlan_id = vlan_id;

	/* chain this new entry */
	list_add(&map->vlans, &vlan->list);

	map->num_vlans++;

	os_log(LOG_INFO, "vlan_id(%u) created, num_vlans(%u)\n", ntohs(vlan_id), map->num_vlans);

exist:
	return vlan;

err:
	return NULL;
}

/** Frees a vlan instance registered in the MVRP main context.
 * \return	none
 * \param map	pointer to the MVRP MAP context
 * \param vlan	pointer to the vlan instance to destroy
 */
void mvrp_free_vlan(struct mvrp_map *map, struct mvrp_vlan *vlan)
{
	struct mvrp_ctx *mvrp = container_of(map, struct mvrp_ctx, map[map->map_id]);

	list_del(&vlan->list);

	map->num_vlans--;

	if (mvrp->is_bridge)
		vlan_delete(ntohs(vlan->vlan_id), true);

	os_log(LOG_INFO, "vlan_id(%u) destroyed, num_vlans(%u)\n", ntohs(vlan->vlan_id), map->num_vlans);

	os_free(vlan);
}

static void mvrp_increment(struct mvrp_pdu_fv *fv)
{
	fv->vid = htons(ntohs(fv->vid) + 1);
}

static int mvrp_attribute_check(struct mrp_pdu_header *mrp_header)
{
	if (mrp_header->attribute_type != MVRP_ATTR_TYPE_VID) {
		os_log(LOG_ERR, "bad attribute_type(%u)\n", mrp_header->attribute_type);
		goto err;
	}

	/* check for valid attribute length */
	if (mrp_header->attribute_length != MVRP_ATTR_LEN) {
		os_log(LOG_ERR, "bad attribute_length (%u), expected (%u)\n", mrp_header->attribute_length, MVRP_ATTR_LEN);
		goto err;
	}

	return 0;

err:
	return -1;
}

static int mvrp_event_length(struct mrp_pdu_header *mrp_header, unsigned int number_of_values)
{
	return (number_of_values + 2) / 3;
}

static int mvrp_vector_handler(struct mrp_application *app, struct mrp_pdu_header *mrp_header, void *vector_data, unsigned int number_of_values)
{
	struct mvrp_pdu_fv *fv = (struct mvrp_pdu_fv *)vector_data;
	u8 *three_packed_events;
	unsigned int event[3];
	int i;

	three_packed_events = (u8 *)fv + MVRP_ATTR_LEN;

	for (i = 0; i < number_of_values; i++, mvrp_increment(fv)) {
		if (!(i % 3)) {
			mrp_get_three_packed_event(*three_packed_events, event);
			three_packed_events++;
		}

		os_log(LOG_INFO, "port(%u) vlan_id(%u) %s\n", app->port_id, ntohs(fv->vid), mrp_attribute_event2string(event[i % 3]));

		mrp_process_attribute(app, MVRP_ATTR_TYPE_VID, (u8 *)fv, event[i % 3]);
	}

	return 0;
}


/** Decode and process the MVRP frame received by the upper SRP layer. Packet points to MVRP data.
 * \return	0 on success, negative value on failure
 * \param mvrp	pointer to the MVRP context
 * \param port_id	logical port identifier
 * \param desc	pointer to the received packet descriptor
 */
int mvrp_process_packet(struct mvrp_ctx *mvrp, unsigned int port_id, struct net_rx_desc *desc)
{
	struct mvrp_port *port;

	if (port_id >= mvrp->port_max)
		return -1;

	if (!mvrp_is_operational(mvrp, port_id)) {
		os_log(LOG_DEBUG, "port(%u) mvrp(%p) receive disabled\n", port_id, mvrp);
		return -1;
	}

	port = &mvrp->port[port_id];

	return mrp_process_packet(&port->mrp_app, desc);
}

void mvrp_port_status(struct mvrp_ctx *mvrp, struct ipc_mac_service_status *status)
{
	struct mvrp_port *port = logical_to_mvrp_port(mvrp, status->port_id);

	if (port) {
		os_log(LOG_INFO, "mvrp(%p) port(%u) operational (%u)\n", mvrp, port->port_id, status->operational);

		mvrp_update_operational_state(mvrp, port->port_id, status->operational);

		mvrp_update_forwarding_state(mvrp, port->port_id, status->operational);
	}
}

/** Called by the upper layer to join a given vlan.(802.1Q -11.2.3.2.1)
 * \return	0 on success, negative value on failure
 * \param mvrp		pointer to MVRP context
 * \param port_id	logical port identifier
 * \param vlan_id	vlan tag identifier
 */
int mvrp_register_vlan_member(struct mvrp_ctx *mvrp, unsigned int port_id, unsigned short vlan_id)
{
	int rc = GENAVB_SUCCESS;
	struct mvrp_map *map;
	struct mvrp_port *port;
	struct mvrp_vlan *vlan;
	bool new;

	map = mvrp_get_map_context(mvrp, vlan_id);
	if (!map) {
		os_log(LOG_ERR, "no map context for vlan_id(%u)\n", ntohs(vlan_id));
		rc = GENAVB_ERR_CTRL_FAILED;
		goto err;
	}

	port = logical_to_mvrp_port(mvrp, port_id);
	if (!port) {
		os_log(LOG_ERR, "mvrp(%p) invalid logical_port(%u)\n", mvrp, port_id);
		rc = GENAVB_ERR_CTRL_INVALID;
		goto err;
	}

	if ((vlan = mvrp_find_vlan(map, vlan_id)) == NULL) {

		if ((vlan = mvrp_create_vlan(map, vlan_id)) == NULL) {
			rc = GENAVB_ERR_CTRL_FAILED;
			goto err;
		}

		/* first time this vlan is registered */
		new = true;

	} else {
		/* not the first time this vlan is registered */
		new = false;
	}

	if (!vlan->ref_count[port->port_id]) {
		vlan_user_declaration_set(vlan, port->port_id);
		mvrp_map_update_vlan(map, vlan, new);
	}

	vlan->ref_count[port->port_id]++;

	os_log(LOG_INFO, "port(%u) vlan_id(%u) ref count %u\n", port->port_id, ntohs(vlan_id), vlan->ref_count[port->port_id]);

err:
	return rc;
}

/** Called by the upper layer to leave a given vlan.(802.1Q -11.2.3.2.1)
 * \return	0 on success, negative value on failure
 * \param mvrp		pointer to MVRP context
 * \param port_id	logical port identifier
 * \param vlan_id	vlan tag identifier of the vlan entry to deregister
 */
int mvrp_deregister_vlan_member(struct mvrp_ctx *mvrp, unsigned int port_id, unsigned short vlan_id)
{
	int rc = GENAVB_SUCCESS;
	struct mvrp_vlan *vlan;
	struct mvrp_map *map;
	struct mvrp_port *port;

	map = mvrp_get_map_context(mvrp, vlan_id);
	if (!map) {
		os_log(LOG_ERR, "no map context for vlan_id(%u)\n", ntohs(vlan_id));
		rc = GENAVB_ERR_CTRL_FAILED;
		goto err;
	}

	port = logical_to_mvrp_port(mvrp, port_id);
	if (!port) {
		os_log(LOG_ERR, "mvrp(%p) invalid logical_port(%u)\n", mvrp, port_id);
		rc = GENAVB_ERR_CTRL_INVALID;
		goto err;
	}

	vlan = mvrp_find_vlan(map, vlan_id);
	if (vlan) {

		if (vlan->ref_count[port->port_id])
			vlan->ref_count[port->port_id]--;

		os_log(LOG_INFO, "port(%u) vlan_id(%u) ref count %u\n", port->port_id, ntohs(vlan_id), vlan->ref_count[port->port_id]);

		/*this vlan is no more used, send leave request */
		if (!vlan->ref_count[port->port_id]) {
			vlan_user_declaration_clear(vlan, port->port_id);
			mvrp_map_update_vlan(map, vlan, false);
		}
	}

err:
	return rc;
}


/** Signal a new VLAN registration to upper layer.(802.1Q -11.2.3.2.2)
 * \return	none
 * \param attr	pointer to MRP attribute
 * \param new	specify if the vlan registration is signaled for the first time
 */
static void mvrp_join_indication(struct mrp_application *app, struct mrp_attribute *attr, bool new)
{
	struct mvrp_attribute_value *attr_val= (struct mvrp_attribute_value *)attr->val;
	struct mvrp_port *port = container_of(app, struct mvrp_port, mrp_app);
	struct mvrp_ctx *mvrp = container_of(port, struct mvrp_ctx, port[port->port_id]);
	struct genavb_vlan_port_map vlan_port_map = {0};
	struct mvrp_map *map;
	struct mvrp_vlan *vlan;

	os_log(LOG_INFO, "port(%u) vlan_id(%u) new(%d)\n", port->port_id, ntohs(attr_val->vid), new);

	map = mvrp_get_map_context(mvrp, attr_val->vid);
	if (!map) {
		os_log(LOG_ERR, "no map context for vlan_id(%u)\n", ntohs(attr_val->vid));
		return;
	}

	vlan = mvrp_find_vlan(map, attr_val->vid);
	if (!vlan) {
		vlan = mvrp_create_vlan(map, attr_val->vid);
	}
	if (!vlan)
		return;

	vlan_port_map.port_id = mvrp_to_logical_port(mvrp, port->port_id);
	vlan_port_map.control = GENAVB_VLAN_ADMIN_CONTROL_REGISTERED;

	vlan_registered_set(vlan, port->port_id);
	mvrp_map_update_vlan(map, vlan, new);

	if (mvrp->is_bridge)
		vlan_update(ntohs(vlan->vlan_id), true, &vlan_port_map);
}


/** Signal a VLAN deregistration to upper layer.(802.1Q -11.2.3.2.2)
 * \return	none
 * \param attr	pointer to MRP attribute
 */
static void mvrp_leave_indication(struct mrp_application *app, struct mrp_attribute *attr)
{
	struct mvrp_attribute_value *attr_val= (struct mvrp_attribute_value *)attr->val;
	struct mvrp_port *port = container_of(app, struct mvrp_port, mrp_app);
	struct mvrp_ctx *mvrp = container_of(port, struct mvrp_ctx, port[port->port_id]);
	struct genavb_vlan_port_map vlan_port_map = {0};
	struct mvrp_map *map;
	struct mvrp_vlan *vlan;

	os_log(LOG_INFO, "port(%u) vlan_id(%u)\n", port->port_id, ntohs(attr_val->vid));

	map = mvrp_get_map_context(mvrp, attr_val->vid);
	if (!map) {
		os_log(LOG_ERR, "no map context for vlan_id(%u)\n", ntohs(attr_val->vid));
		return;
	}

	vlan = mvrp_find_vlan(map, attr_val->vid);
	if (!vlan)
		return;

	vlan_port_map.port_id = mvrp_to_logical_port(mvrp, port->port_id);
	vlan_port_map.control = GENAVB_VLAN_ADMIN_CONTROL_NOT_REGISTERED;

	vlan_registered_clear(vlan, port->port_id);
	mvrp_map_update_vlan(map, vlan, false);

	if (mvrp->is_bridge)
		vlan_update(ntohs(vlan->vlan_id), true, &vlan_port_map);
}

static void mvrp_ipc_vlan_response(struct mvrp_ctx *mvrp, struct ipc_tx *ipc, unsigned int ipc_dst, u16 port, u16 vlan_id, u32 status)
{
	struct ipc_desc *desc;
	struct ipc_mvrp_vlan_response *response;

	os_log(LOG_INFO, "vlan_id(%u)\n", ntohs(vlan_id));

	/* Send vlan response to media stack */
	desc = ipc_alloc(ipc, sizeof(struct ipc_mvrp_vlan_response));
	if (desc) {
		desc->dst = ipc_dst;
		desc->type = GENAVB_MSG_VLAN_RESPONSE;
		desc->len = sizeof(struct ipc_mvrp_vlan_response);

		response = &desc->u.mvrp_vlan_response;

		response->port = port;
		response->vlan_id = vlan_id;

		response->status = status;

		if (ipc_tx(ipc, desc) < 0) {
			os_log(LOG_ERR, "mvrp(%p) ipc_tx() failed\n", mvrp);
			ipc_free(ipc, desc);
		}
	} else
		os_log(LOG_ERR, "mvrp(%p) ipc_alloc() failed\n", mvrp);
}


static void mvrp_ipc_error_response(struct mvrp_ctx *mvrp, struct ipc_tx *ipc, unsigned int ipc_dst, u32 type, u32 len, u32 status)
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
			os_log(LOG_ERR, "mvrp(%p) ipc_tx() failed\n", mvrp);
			ipc_free(ipc, desc);
		}
	} else
		os_log(LOG_ERR, "mvrp(%p) ipc_alloc() failed\n", mvrp);
}

static void mvrp_ipc_rx(struct ipc_rx const *rx, struct ipc_desc *desc)
{
	struct mvrp_ctx *mvrp = container_of(rx, struct mvrp_ctx, ipc_rx);
	struct ipc_tx *ipc;
	u32 status;

	if (desc->flags & IPC_FLAGS_AVB_MSG_SYNC)
		ipc = &mvrp->ipc_tx_sync;
	else
		ipc = &mvrp->ipc_tx;

	switch (desc->type) {
	case GENAVB_MSG_VLAN_REGISTER:
		if (desc->len != sizeof(struct ipc_mvrp_vlan_register)) {
			os_log(LOG_ERR, "mvrp(%p) wrong length received %d expected %zd\n", mvrp, desc->len, sizeof(struct ipc_mvrp_vlan_register));
			status = GENAVB_ERR_CTRL_LEN;
			goto err;
			break;
		}

		status = mvrp_register_vlan_member(mvrp, desc->u.mvrp_vlan_register.port, htons(desc->u.mvrp_vlan_register.vlan_id));

		mvrp_ipc_vlan_response(mvrp, ipc, desc->src, desc->u.mvrp_vlan_register.port, desc->u.mvrp_vlan_register.vlan_id, status);

		break;

	case GENAVB_MSG_VLAN_DEREGISTER:
		if (desc->len != sizeof(struct ipc_mvrp_vlan_register)){
			os_log(LOG_ERR, "mvrp(%p) wrong length received %d expected %zd\n", mvrp, desc->len, sizeof(struct ipc_mvrp_vlan_deregister));
			status = GENAVB_ERR_CTRL_LEN;
			goto err;
			break;
		}

		status = mvrp_deregister_vlan_member(mvrp, desc->u.mvrp_vlan_deregister.port, htons(desc->u.mvrp_vlan_deregister.vlan_id));

		mvrp_ipc_vlan_response(mvrp, ipc, desc->src, desc->u.mvrp_vlan_deregister.port, desc->u.mvrp_vlan_deregister.vlan_id, status);

		break;

	default:
		os_log(LOG_ERR, "mvrp(%p) unknown IPC type %d\n", mvrp, desc->type);
		status = GENAVB_ERR_CTRL_INVALID;
		goto err;
		break;
	}

	ipc_free(rx, desc);

	return;

err:
	mvrp_ipc_error_response(mvrp, ipc, desc->src, desc->type, desc->len, status);

	ipc_free(rx, desc);
}

__init static int mvrp_port_init(struct mvrp_port *port, unsigned int port_id, struct mvrp_config *cfg)
{
	struct mvrp_ctx *mvrp = container_of(port, struct mvrp_ctx, port[port_id]);
	struct srp_ctx *srp = mvrp->srp;

	port->port_id = port_id;

	port->logical_port = cfg->logical_port_list[port_id];

	port->srp_port = &srp->port[port_id];

	/* initialize MVRP state machines*/
	port->mrp_app.srp = mvrp->srp;
	port->mrp_app.port_id = port_id;
	port->mrp_app.attribute_type_to_string = mvrp_attribute_type_to_string;
	port->mrp_app.mad_join_indication = mvrp_join_indication;
	port->mrp_app.mad_leave_indication = mvrp_leave_indication;
	port->mrp_app.attribute_length = mvrp_attribute_length;
	port->mrp_app.attribute_value_length = mvrp_attribute_value_length;
	port->mrp_app.vector_add_event = mvrp_vector_add_event;
	port->mrp_app.net_tx = mvrp_net_tx;
	port->mrp_app.net_tx_alloc = mvrp_net_tx_alloc;

	port->mrp_app.dst_mac = mvrp_dst_mac;
	port->mrp_app.ethertype = ETHERTYPE_MVRP;
	port->mrp_app.min_attr_type = MVRP_ATTR_TYPE_VID;
	port->mrp_app.max_attr_type = MVRP_ATTR_TYPE_VID;
	port->mrp_app.proto_version = MVRP_PROTO_VERSION;
	port->mrp_app.has_attribute_list_length = 0;

	port->mrp_app.attribute_check = mvrp_attribute_check;
	port->mrp_app.event_length = mvrp_event_length;
	port->mrp_app.vector_handler = mvrp_vector_handler;

	if (!port->srp_port->initialized)
		goto out;

	if (mrp_init(&port->mrp_app, APP_MVRP, CFG_MVRP_PARTICIPANT_TYPE) < 0)
		goto err_mrp;

	if (net_add_multi(&port->srp_port->net_rx, mvrp_to_logical_port(mvrp, port->port_id), mvrp_dst_mac) < 0)
		goto err_multi;

	os_log(LOG_INIT, "port(%u) done\n", port_id);

out:
	return 0;

err_multi:
	mrp_exit(&port->mrp_app);

err_mrp:
	return -1;
}

__exit static void mvrp_port_exit(struct mvrp_port *port)
{
	struct mvrp_ctx *mvrp = container_of(port, struct mvrp_ctx, port[port->port_id]);

	if (!port->srp_port->initialized)
		goto out;

	if (net_del_multi(&port->srp_port->net_rx, mvrp_to_logical_port(mvrp, port->port_id), mvrp_dst_mac) < 0)
		os_log(LOG_ERR, "port(%u) cannot remove mvrp multicast\n", port->port_id);

	mrp_exit(&port->mrp_app);

out:
	return;
}

/** MVRP component initialization (timers, state machines,...)
 * \return	0 on success, negative value on failure
 * \param mvrp	pointer to MVRP context
 */
__init int mvrp_init(struct mvrp_ctx *mvrp, struct mvrp_config *cfg, unsigned long priv)
{
	unsigned int ipc_tx, ipc_tx_sync, ipc_rx;
	int i, j;

	if (!cfg->is_bridge) {
		if (cfg->logical_port_list[0] == CFG_ENDPOINT_0_LOGICAL_PORT) {
			ipc_tx = IPC_MVRP_MEDIA_STACK;
			ipc_tx_sync = IPC_MVRP_MEDIA_STACK_SYNC;
			ipc_rx = IPC_MEDIA_STACK_MVRP;
		} else {
			ipc_tx = IPC_MVRP_1_MEDIA_STACK;
			ipc_tx_sync = IPC_MVRP_1_MEDIA_STACK_SYNC;
			ipc_rx = IPC_MEDIA_STACK_MVRP_1;
		}
	} else {
		ipc_tx = IPC_MVRP_BRIDGE_MEDIA_STACK;
		ipc_tx_sync = IPC_MVRP_BRIDGE_MEDIA_STACK_SYNC;
		ipc_rx = IPC_MEDIA_STACK_MVRP_BRIDGE;
	}

	mvrp->port_max = cfg->port_max;
	mvrp->is_bridge = cfg->is_bridge;

	if (ipc_rx_init(&mvrp->ipc_rx, ipc_rx, mvrp_ipc_rx, priv) < 0)
		goto err_ipc_rx;

	if (ipc_tx_init(&mvrp->ipc_tx, ipc_tx) < 0)
		goto err_ipc_tx;

	if (ipc_tx_init(&mvrp->ipc_tx_sync, ipc_tx_sync) < 0)
		goto err_ipc_tx_sync;

	mvrp_map_init(mvrp);

	for (i = 0; i < mvrp->port_max; i++)
		if (mvrp_port_init(&mvrp->port[i], i, cfg) < 0)
			goto err_mvrp_init;

	os_log(LOG_INIT, "mvrp(%p) done\n", mvrp);

	return 0;

err_mvrp_init:
	for (j = 0; j < i; j++)
		mvrp_port_exit(&mvrp->port[j]);

err_ipc_tx_sync:
	ipc_tx_exit(&mvrp->ipc_tx);

err_ipc_tx:
	ipc_rx_exit(&mvrp->ipc_rx);

err_ipc_rx:
	return -1;
}


/** MVRP component clean-up, destroy all pending vlan attributes
 * \return	0 on success, negative value on failure
 * \param mvrp	pointer to MVRP context
 */
__exit int mvrp_exit(struct mvrp_ctx *mvrp)
{
	int i;

	for (i = 0; i < mvrp->port_max; i++)
		mvrp_port_exit(&mvrp->port[i]);

	mvrp_map_exit(mvrp);

	ipc_tx_exit(&mvrp->ipc_tx_sync);

	ipc_tx_exit(&mvrp->ipc_tx);

	ipc_rx_exit(&mvrp->ipc_rx);

	os_log(LOG_INIT, "mvrp(%p) done\n", mvrp);

	return 0;
}
