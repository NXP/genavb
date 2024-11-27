/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2019, 2021-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @brief ADP common code
 @details Handles ADP stack
*/

#include "os/stdlib.h"

#include "common/log.h"
#include "common/timer.h"
#include "common/net.h"

#include "genavb/aem.h"

#include "adp.h"
#include "aem.h"

#include "avdecc.h"
#include "avdecc_ieee.h"

static const u8 adp_dst_mac[6] = MC_ADDR_AVDECC_ADP_ACMP;

static const char *adp_msgtype2string(u8 msg_type)
{
	switch (msg_type) {
	case2str(ADP_ENTITY_AVAILABLE);
	case2str(ADP_ENTITY_DEPARTING);
	case2str(ADP_ENTITY_DISCOVER);
	case2str(ADP_ENTITY_NOTFOUND);
	default:
		return (char *) "Unknown adp message type";
	}
}

void adp_update(struct adp_ctx *adp)
{
	struct entity *entity = container_of(adp, struct entity, adp);

	if (entity->milan_mode)
		adp_milan_advertise_start(adp);
	else
		adp_ieee_advertise_start(adp);
}

/** Sends an ADP DISCOVER PDU to the network.
 * \return	0 on success, -1 if error
 * \param disc		Pointer to the discovery context
 * \param entity_id		Entity ID to discover. Set to to NULL or point to a zero value to discover all entities.
 */
int adp_discovery_send_packet(struct adp_discovery_ctx *disc, u8 *entity_id)
{
	struct avdecc_port *port = discovery_to_avdecc_port(disc);
	struct avdecc_ctx *avdecc = avdecc_port_to_context(port);
	struct adp_pdu *adp_hdr;
	void *pdu;
	struct net_tx_desc *desc;

	desc = net_tx_alloc(&port->net_tx, ADP_NET_DATA_SIZE);
	if (!desc) {
		os_log(LOG_ERR, "avdecc(%p) couldn't allocate message type (%x)\n", avdecc, ADP_ENTITY_DISCOVER);
		return -1;
	}

	pdu = NET_DATA_START(desc);

	desc->len += net_add_eth_header(pdu, adp_dst_mac, ETHERTYPE_AVTP);
	desc->len += avdecc_add_common_header((char *)pdu + desc->len, AVTP_SUBTYPE_ADP, ADP_ENTITY_DISCOVER, ADP_PDU_LEN, 0);

	adp_hdr = (struct adp_pdu *)((char *)pdu + desc->len);

	os_memset(adp_hdr, 0, sizeof(struct adp_pdu));
	if (entity_id)
		copy_64(&adp_hdr->entity_id, entity_id);

	desc->len += sizeof(struct adp_pdu);

	avdecc_net_tx(port, desc);

	os_log(LOG_INFO, "avdecc(%p) message type (%x)\n", avdecc, ADP_ENTITY_DISCOVER);

	return 0;
}

/** Sends an ADP PDU to the network.
 * \return		0 on success, -1 if error
 * \param adp		pointer to the adp context
 * \param message_type	advertise or discovery message type
 * \param port_id 	port on which to send the packet
 */
int adp_advertise_send_packet(struct adp_ctx *adp, u8 message_type, unsigned int port_id)
{
	struct adp_pdu *adp_hdr;
	void *pdu;
	struct net_tx_desc *desc;
	struct avb_interface_dynamic_desc *avb_itf_dynamic;
	struct entity *entity = container_of(adp, struct entity, adp);
	struct avdecc_port *port;

	avb_itf_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_AVB_INTERFACE, port_id, NULL);

	if (!avb_itf_dynamic) {
		os_log(LOG_ERR, "adp(%p) Can not get dynamic avb interface descriptor for index (%u)\n", &entity->adp, port_id);
		return -1;
	}

	if (message_type != ADP_ENTITY_AVAILABLE && message_type != ADP_ENTITY_DEPARTING)
		return -1;

	port = &entity->avdecc->port[port_id];

	desc = net_tx_alloc(&port->net_tx, ADP_NET_DATA_SIZE);
	if (!desc) {
		os_log(LOG_ERR, "adp(%p) Cannot alloc tx descriptor\n", &entity->adp);
		return -1;
	}

	pdu = NET_DATA_START(desc);

	desc->len += net_add_eth_header(pdu, adp_dst_mac, ETHERTYPE_AVTP);
	desc->len += avdecc_add_common_header((char *)pdu + desc->len, AVTP_SUBTYPE_ADP, message_type, ADP_PDU_LEN,
						(message_type == ADP_ENTITY_AVAILABLE) ? (entity->valid_time / 2) : 0);

	adp_hdr = (struct adp_pdu *)((char *)pdu + desc->len);

	adp_hdr->entity_id = entity->desc->entity_id;
	adp_hdr->entity_model_id = entity->desc->entity_model_id;
	adp_hdr->entity_capabilities = entity->desc->entity_capabilities;
	adp_hdr->talker_stream_sources = entity->desc->talker_stream_sources;
	adp_hdr->talker_capabilities = entity->desc->talker_capabilities;
	adp_hdr->listener_stream_sinks = entity->desc->listener_stream_sinks;
	adp_hdr->listener_capabilities = entity->desc->listener_capabilities;
	adp_hdr->controller_capabilities = entity->desc->controller_capabilities;

	adp_hdr->available_index = entity->desc->available_index;

	adp_hdr->gptp_grandmaster_id = avb_itf_dynamic->gptp_grandmaster_id;
	adp_hdr->gptp_domain_number = 0; //FIXME
	adp_hdr->identity_control_index = 0; //FIXME
	adp_hdr->interface_index = port_id;
	adp_hdr->association_id = entity->desc->association_id;
	adp_hdr->rsvd0 = 0;
	adp_hdr->rsvd1 = 0;

	desc->len += sizeof(struct adp_pdu);

	avdecc_net_tx(port, desc);

	os_log(LOG_INFO, "entity(%p) port(%u) entity id: %016"PRIx64" message type %s\n",
		entity, port_id, ntohll(entity->desc->entity_id), adp_msgtype2string(message_type));

	return 0;
}

static void adp_discover_rcv(struct avdecc_ctx *avdecc, struct adp_pdu *pdu, unsigned int port_id)
{
	int i;

	if (!pdu->entity_id) {
		for (i = 0; i < avdecc->num_entities; i++) {
			if (entity_ready(avdecc->entities[i]) && avdecc_entity_port_valid(avdecc->entities[i], port_id)) {
				if (avdecc->milan_mode)
					adp_milan_advertise_sm(avdecc->entities[i], port_id, ADP_MILAN_ADV_RCV_ADP_DISCOVER);
				else
					adp_ieee_advertise_interface_sm(avdecc->entities[i], port_id, ADP_INTERFACE_ADV_EVENT_RCV_DISCOVER);
			}
		}
	} else {
		struct entity *entity = avdecc_get_entity(avdecc, pdu->entity_id);
		if (entity && avdecc_entity_port_valid(entity, port_id)) {
			if (avdecc->milan_mode)
				adp_milan_advertise_sm(entity, port_id, ADP_MILAN_ADV_RCV_ADP_DISCOVER);
			else
				adp_ieee_advertise_interface_sm(entity, port_id, ADP_INTERFACE_ADV_EVENT_RCV_DISCOVER);
		}
	}

	os_log(LOG_DEBUG, "avdecc(%p) port(%u) done\n", avdecc, port_id);
}

/** Main ADP receive function.
 * Handles both advertise and discovery (6.2.5.3 and 6.2.6.4).
 * \return	0 on success, negative otherwise
 * \param port		pointer to the AVDECC port
 * \param pdu		pointer to the ADP PDU
 * \param msg_type	ADP message type (6.2.1.5)
 * \param valid_time	valid_time from AVTP control header (6.2.1.6)
 * \param mac_src	pointer to the source MAC address
 */
int adp_net_rx(struct avdecc_port *port, struct adp_pdu *pdu, u8 msg_type, u8 valid_time, u8 *mac_src)
{
	struct entity *entity;
	struct avdecc_ctx *avdecc = avdecc_port_to_context(port);

	os_log(LOG_DEBUG, "port(%u) message type %s\n", port->port_id, adp_msgtype2string(msg_type));

	switch (msg_type) {
	case ADP_ENTITY_AVAILABLE:
		adp_discovery_update(&port->discovery, pdu, valid_time, mac_src);

		if (avdecc->milan_mode) {
			entity = avdecc_get_local_listener(avdecc, port->port_id);
			if (entity)
				adp_milan_listener_rcv(entity, msg_type, pdu, valid_time);
		}

		break;

	case ADP_ENTITY_DEPARTING:
		adp_discovery_remove(&port->discovery, pdu);

		if (avdecc->milan_mode) {
			entity = avdecc_get_local_listener(avdecc, port->port_id);
			if (entity)
				adp_milan_listener_rcv(entity, msg_type, pdu, valid_time);
		}

		break;

	case ADP_ENTITY_DISCOVER:
		adp_discover_rcv(avdecc, pdu, port->port_id);

		break;

	default:
		break;
	}

	return 0;
}

/* ADP common controller discovery */
int adp_run_discovery(struct adp_discovery_ctx *disc, struct adp_pdu *pdu, u8 valid_time)
{
	return 0;
}

static struct entity_discovery *adp_discovery_find(struct adp_discovery_ctx *disc, u64 entity_id)
{
	int i;

	for (i = 0; i < disc->max_entities_discovery; i++)
		if (disc->entities[i].in_use && (entity_id == disc->entities[i].info.entity_id))
			return &disc->entities[i];

	return NULL;
}

/** Find discovered entity by ID on a specific port
 * \return entity_discovery if the entity has been discovered on network or NULL otherwise.
 * \param avdecc	Pointer to the avdecc context
 * \param port_id	Avdecc port / interface index on which to look for the discovered entity
 * \param entity_id	Entity ID of the entity to search for (in network order)
 */
struct entity_discovery *adp_find_entity_discovery(struct avdecc_ctx *avdecc, unsigned int port_id, u64 entity_id)
{
	struct adp_discovery_ctx *disc;

	if (port_id >= avdecc->port_max || !avdecc->port[port_id].initialized)
		return NULL;

	disc = &avdecc->port[port_id].discovery;

	return adp_discovery_find(disc, entity_id);
}

/** Find discovered entity by ID on a any port
 * \return entity_discovery if the entity has been discovered on network or NULL otherwise.
 * \param avdecc		Pointer to the avdecc context
 * \param entity_id		Entity ID of the entity to search for (in network order)
 * \param num_interfaces	Number of interfaces on which to search for the entity.
 */
struct entity_discovery *adp_find_entity_discovery_any(struct avdecc_ctx *avdecc, u64 entity_id, unsigned int num_interfaces)
{
	struct entity_discovery *entity_disc = NULL;
	unsigned int port_num;

	for (port_num = 0; port_num < num_interfaces; port_num++) {
		entity_disc = adp_find_entity_discovery(avdecc, port_num, entity_id);
		/* For an entity discovered on multiple interfaces, always return the entity on the first port.
		 * FIXME return the most recent discovered one
		 */
		if (entity_disc)
			break;
	}

	return entity_disc;
}

static unsigned int adp_get_total_discovered_entities(struct entity *entity)
{
	int i;
	unsigned int total_discovered = 0, num_interfaces;
	struct adp_discovery_ctx *disc;

	num_interfaces = aem_get_descriptor_max(entity->aem_descs, AEM_DESC_TYPE_AVB_INTERFACE);

	/* loop over all discovery contexts on all the entity interfaces */
	for (i = 0; i < num_interfaces; i++) {
		disc = &entity->avdecc->port[i].discovery;
		total_discovered += disc->num_discovered_entities;
	}

	return total_discovered;
}

/** Send an ADP IPC message
 * \return 0 on success or -1 otherwise.
 * \param entity	Pointer to the entity struct.
 * \param ipc		IPC to send the message through.
 * \param msg_type	Type of ADP message to send (See section 6.2.1.5 of IEEE 1722.1-2013).
 * \param info		Entity information to include in the message. Will be ignored if NULL.
 */
static int adp_ipc_tx(struct entity *entity, struct ipc_tx *ipc, unsigned int ipc_dst, u8 msg_type, struct entity_info *info)
{
	struct ipc_desc *tx_desc;
	int rc = 0;

	os_log(LOG_DEBUG, "entity(%p) Sending ADP IPC message_type (%d) entity_info(%p)\n", entity, msg_type, info);
	tx_desc = ipc_alloc(ipc, sizeof(struct genavb_adp_msg));
	if (tx_desc) {
		tx_desc->dst = ipc_dst;
		tx_desc->type = GENAVB_MSG_ADP;
		tx_desc->len = sizeof(struct genavb_adp_msg);

		tx_desc->u.adp_msg.msg_type =  msg_type;
		tx_desc->u.adp_msg.total = adp_get_total_discovered_entities(entity);

		if (info)
			os_memcpy(&tx_desc->u.adp_msg.info, info, sizeof(struct entity_info));
		else {
			os_memset(&tx_desc->u.adp_msg.info, 0, sizeof(struct entity_info));
			os_log(LOG_DEBUG, "entity(%p) Trying to send an IPC ADP message_type(%d) but no entity info available\n", entity, msg_type);
		}

		rc = ipc_tx(ipc, tx_desc);
		if (rc < 0) {
			if (rc == -IPC_TX_ERR_QUEUE_FULL)
				os_log(LOG_ERR, "entity(%p) ipc_tx() failed (%d), ADP message type (%d)\n", entity, rc, msg_type);
			ipc_free(ipc, tx_desc);
			rc = -1;
		}
	} else {
		os_log(LOG_ERR, "entity(%p) ipc_alloc() failed\n", entity);
		rc = -1;
	}

	return rc;
}

static struct entity_discovery * adp_discovery_get(struct adp_discovery_ctx *disc)
{
	int i;
	struct entity_discovery *entity_disc = NULL;

	for (i = 0; i < disc->max_entities_discovery; i++)
		if (!disc->entities[i].in_use)
			entity_disc = &disc->entities[i];

	if (entity_disc) {
		entity_disc->in_use = 1;
		entity_disc->disc->num_discovered_entities++;
	}
	else
		os_log(LOG_ERR, "disc(%p) no more discovery entries\n", disc);

	return entity_disc;
}

static void inline adp_discovery_put(struct entity_discovery *entity_disc)
{
	int i;
	struct avdecc_port *port = discovery_to_avdecc_port(entity_disc->disc);
	struct avdecc_ctx *avdecc = avdecc_port_to_context(port);
	struct entity *entity;

	entity = avdecc_get_local_controller(avdecc, port->port_id);
	if (entity) {
		if (adp_ipc_tx(entity, &avdecc->ipc_tx_controller, IPC_DST_ALL, ADP_ENTITY_DEPARTING, &entity_disc->info) < 0)
			os_log(LOG_ERR, "entity(%p) adp_ipc_tx() failed msg_type(%d) entity_info(%p)\n",
				entity, ADP_ENTITY_DEPARTING, &entity_disc->info);
	}

	if (!avdecc->milan_mode) {
		for (i = 0; i < avdecc->num_entities; i++)
			if (entity_ready(avdecc->entities[i]))
				acmp_ieee_listener_talker_left(&avdecc->entities[i]->acmp, entity_disc->info.entity_id);
	}

	entity_disc->in_use = 0;
	entity_disc->disc->num_discovered_entities--;
	os_memset(&entity_disc->info, 0 , sizeof(struct entity_info));
}

static void adp_discovery_update_entity(struct adp_discovery_ctx *disc, struct entity_info *info, struct adp_pdu *pdu, u8 *mac_addr)
{
	struct avdecc_port *port = discovery_to_avdecc_port(disc);
	struct avdecc_ctx *avdecc = avdecc_port_to_context(port);
	struct entity *entity = avdecc_get_local_controller(avdecc, port->port_id);
	bool send_ipc = false;

	if ((info->entity_id != pdu->entity_id) ||
		(info->entity_model_id != pdu->entity_model_id) ||
		(info->entity_capabilities != pdu->entity_capabilities) ||
		(info->talker_stream_sources != pdu->talker_stream_sources) ||
		(info->talker_capabilities != pdu->talker_capabilities) ||
		(info->listener_stream_sinks != pdu->listener_stream_sinks) ||
		(info->listener_capabilities != pdu->listener_capabilities) ||
		(info->controller_capabilities != pdu->controller_capabilities) ||
		(info->gptp_grandmaster_id != pdu->gptp_grandmaster_id) ||
		(info->gptp_domain_number != pdu->gptp_domain_number) ||
		(info->identity_control_index != pdu->identity_control_index) ||
		(info->interface_index != pdu->interface_index) ||
		(info->association_id != pdu->association_id) ||
		os_memcmp(info->mac_addr, mac_addr, 6) ) {

		/* The discovered entity has changed, notify the controller if existing. */
		if (entity)
			send_ipc = true;
	}

	info->entity_id = pdu->entity_id;
	info->entity_model_id = pdu->entity_model_id;
	info->entity_capabilities = pdu->entity_capabilities;
	info->talker_stream_sources = pdu->talker_stream_sources;
	info->talker_capabilities = pdu->talker_capabilities;
	info->listener_stream_sinks = pdu->listener_stream_sinks;
	info->listener_capabilities = pdu->listener_capabilities;
	info->controller_capabilities = pdu->controller_capabilities;
	info->gptp_grandmaster_id = pdu->gptp_grandmaster_id;
	info->gptp_domain_number = pdu->gptp_domain_number;
	info->identity_control_index = pdu->identity_control_index;
	info->interface_index = pdu->interface_index;
	info->available_index = pdu->available_index;
	info->association_id = pdu->association_id;
	os_memcpy(info->mac_addr, mac_addr, 6);
	os_memcpy(info->local_mac_addr, port->local_physical_mac, 6);

	if (send_ipc) {
		if (adp_ipc_tx(entity, &avdecc->ipc_tx_controller, IPC_DST_ALL, ADP_ENTITY_AVAILABLE, info) < 0)
			os_log(LOG_ERR, "entity(%p) adp_ipc_tx() failed msg_type(%d) entity_info(%p)\n", entity, ADP_ENTITY_AVAILABLE, info);
	}
}

void adp_discovery_update(struct adp_discovery_ctx *disc, struct adp_pdu *pdu, u8 valid_time, u8 *mac_src)
{
	struct entity_discovery *entity_disc;
	struct avdecc_port *port = discovery_to_avdecc_port(disc);
	struct avdecc_ctx *avdecc = avdecc_port_to_context(port);
	bool gptp_gmid_changed = false;

	os_log(LOG_INFO, "port(%u) entity : %016"PRIx64", capabilities : %x, association ID: %"PRIx64" gPTP GM ID : %016"PRIx64", valid time : %d s\n",
		port->port_id, ntohll(pdu->entity_id), pdu->entity_capabilities, ntohll(pdu->association_id), ntohll(pdu->gptp_grandmaster_id), valid_time * 2);

	entity_disc = adp_discovery_find(disc, pdu->entity_id);

	/* New entity */
	if (!entity_disc) {
		entity_disc = adp_discovery_get(disc);

		if (!entity_disc)
			return;

		os_memset(&entity_disc->info, 0, sizeof(struct entity_info));
	}
	else {
		/* Check if entity power-cycled */
 		if (ntohl(pdu->available_index) <= ntohl(entity_disc->info.available_index)) {
			os_log(LOG_INFO, "port(%u) entity : %016"PRIx64" power-cycled\n",
						port->port_id, ntohll(pdu->entity_id));

			/* Put entity as if it departed... */
			timer_stop(&entity_disc->timeout);
			adp_discovery_put(entity_disc);

			/* .. and get a new one */
			entity_disc = adp_discovery_get(disc);
			if (!entity_disc)
				return;
		}
	}

	/* Check if grand master ID changed for this entity */
	if (!cmp_64(&pdu->gptp_grandmaster_id, &entity_disc->info.gptp_grandmaster_id)) {
		gptp_gmid_changed = true;
		os_log(LOG_INFO, "port(%u) gPTP GM ID change for entity : %016"PRIx64", former : %016"PRIx64", new : %016"PRIx64"\n",
			port->port_id, ntohll(pdu->entity_id), ntohll(entity_disc->info.gptp_grandmaster_id), ntohll(pdu->gptp_grandmaster_id));
	}

	adp_discovery_update_entity(disc, &entity_disc->info, pdu, mac_src);

	timer_stop(&entity_disc->timeout);
	/* Per IEEE1722.1-2013 6.2.1.6: received valid_time field in the PDU is in units of 2s */
	timer_start(&entity_disc->timeout, max(1, valid_time * 2) * MS_PER_S);

	if (!avdecc->milan_mode)
		avdecc_ieee_discovery_update(avdecc, disc, entity_disc, gptp_gmid_changed);

	return;
}

/** Removes a discovered entity.
 * Stops the associated timeout timer if needed.
 * \return	0 on success, negative otherwise
 * \param disc		pointer to the discovery context
 * \param pdu		pointer to the ADP PDU
 */
void adp_discovery_remove(struct adp_discovery_ctx *disc, struct adp_pdu *pdu)
{
	struct entity_discovery *entity_disc;
	struct avdecc_port *port = discovery_to_avdecc_port(disc);

	entity_disc = adp_discovery_find(disc, pdu->entity_id);

	if (entity_disc) {
		os_log(LOG_INFO, "port(%u) entity remove: %016"PRIx64"\n", port->port_id, ntohll(entity_disc->info.entity_id));

		timer_stop(&entity_disc->timeout);
		adp_discovery_put(entity_disc);
	}
}

/** Main ADP IPC receive function.
 * \return 0 in all cases (error cases handled inside the function).
 * \param entity	Pointer to the entity struct.
 * \param adp_msg	Pointer to the received IPC ADP message.
 * \param len		Length of the received IPC message payload.
 * \param ipc		IPC the message was received through.
 */
int adp_ipc_rx(struct entity *entity, struct ipc_adp_msg *adp_msg, u32 len, struct ipc_tx *ipc, unsigned int ipc_dst)
{
	struct entity_discovery *entity_disc = NULL;
	struct avdecc_ctx *avdecc = entity->avdecc;
	struct adp_discovery_ctx *disc;
	int i, port_num, sent = 0;
	int rc = 0;
	unsigned int num_interfaces;

	if (len < sizeof(struct ipc_adp_msg)) {
		os_log(LOG_ERR, "entity(%p) Invalid IPC ADP message size (%d instead of expected %zd)\n",
			entity, len, sizeof(struct ipc_adp_msg));
		return -1;
	}

	os_log(LOG_DEBUG, "entity(%p) ADP message_type (%d)\n", entity, adp_msg->msg_type);

	switch (adp_msg->msg_type) {
	case ADP_ENTITY_DISCOVER:
		num_interfaces = aem_get_descriptor_max(entity->aem_descs, AEM_DESC_TYPE_AVB_INTERFACE);

		/* loop over all discovery contexts on all the entity interfaces */
		for (port_num = 0; port_num < num_interfaces; port_num++) {
			disc = &avdecc->port[port_num].discovery;

			for (i = 0; i < disc->max_entities_discovery; i++) {
				if (disc->entities[i].in_use) {
					rc = adp_ipc_tx(entity, ipc, ipc_dst, ADP_ENTITY_AVAILABLE, &disc->entities[i].info);
					if (rc < 0) {
						os_log(LOG_ERR, "entity(%p) adp_ipc_tx() failed msg_type(%d) entity_info(%p)\n",
							entity, ADP_ENTITY_AVAILABLE, &disc->entities[i].info);
						break;
					}
					else
						sent++;
				}
			}
		}

		if (!sent) {
			rc = adp_ipc_tx(entity, ipc, ipc_dst, ADP_ENTITY_NOTFOUND, NULL);
			if (rc < 0)
				os_log(LOG_ERR, "entity(%p) adp_ipc_tx() failed msg_type(%d) entity_info(%p)\n",
					entity, ADP_ENTITY_NOTFOUND, NULL);
		}

		if (sent != adp_get_total_discovered_entities(entity)) { /* should never happen */
			os_log(LOG_ERR, "entity(%p) inconsistent number of total discovered entities (%d discovered but %d reported)\n",
				entity, sent, adp_get_total_discovered_entities(entity));
		}

		break;

	case ADP_ENTITY_AVAILABLE:
		num_interfaces = aem_get_descriptor_max(entity->aem_descs, AEM_DESC_TYPE_AVB_INTERFACE);

		/* loop over all discovery contexts on all the entity interfaces */
		for (port_num = 0; port_num < num_interfaces; port_num++) {
			disc = &avdecc->port[port_num].discovery;

			entity_disc = adp_discovery_find(disc, adp_msg->info.entity_id);
			if (entity_disc) {
				rc = adp_ipc_tx(entity, ipc, ipc_dst, ADP_ENTITY_AVAILABLE, &entity_disc->info);
				if (rc < 0)
					os_log(LOG_ERR, "entity(%p) adp_ipc_tx() failed msg_type(%d) entity_info(%p)\n",
						entity, ADP_ENTITY_AVAILABLE, &entity_disc->info);
			}
		}

		if (!entity_disc) {
			rc = adp_ipc_tx(entity, ipc, ipc_dst, ADP_ENTITY_NOTFOUND, NULL);
			if (rc < 0)
				os_log(LOG_ERR, "entity(%p) adp_ipc_tx() failed msg_type(%d) entity_info(%p)\n",
					entity, ADP_ENTITY_NOTFOUND, NULL);
		}

		break;

	default:
		os_log(LOG_ERR, "entity(%p) Received ADP message from application with unknown msg_type(%d)\n",
			entity, adp_msg->msg_type);

		rc = adp_ipc_tx(entity, ipc, ipc_dst, ADP_ENTITY_NOTFOUND, NULL);
		if (rc < 0)
			os_log(LOG_ERR, "entity(%p) adp_ipc_tx() failed msg_type(%d) entity_info(%p)\n",
				entity, ADP_ENTITY_NOTFOUND, NULL);
		break;
	}

	return 0;
}

/** Discovery timeout handles.
 * The discovered entity entry is removed as no advertise has been received
 * since the time defined by the entity valid_time (6.2.1.6).
 * \return	0 on success, negative otherwise
 * \param data	pointer to the timeout context data
 */
void adp_discovery_timeout(void *data)
{
	struct entity_discovery *entity_disc = (struct entity_discovery *)data;
	struct avdecc_port *port = discovery_to_avdecc_port(entity_disc->disc);

	os_log(LOG_INFO, "port(%u) entity timeout: %016"PRIx64"\n", port->port_id, ntohll(entity_disc->info.entity_id));

	adp_discovery_put(entity_disc);
}

__init unsigned int adp_discovery_data_size(unsigned int max_entities_discovery)
{
	return max_entities_discovery * sizeof(struct entity_discovery);
}

__init int adp_discovery_init(struct adp_discovery_ctx *disc, void *data, struct avdecc_config *cfg)
{
	int i, j;
	struct avdecc_port *port = discovery_to_avdecc_port(disc);
	struct avdecc_ctx *avdecc = avdecc_port_to_context(port);

	disc->entities = (struct entity_discovery *)data;
	disc->max_entities_discovery = cfg->max_entities_discovery;
	disc->num_discovered_entities = 0;

	if (!port->initialized)
		goto exit;

	for (i = 0; i < disc->max_entities_discovery; i++) {
		disc->entities[i].timeout.func = adp_discovery_timeout;
		disc->entities[i].timeout.data = (void *) &disc->entities[i];

		if (timer_create(avdecc->timer_ctx, &disc->entities[i].timeout, 0, 2 * MS_PER_S) < 0) {
			os_log(LOG_CRIT, "disc(%p) timer_create failed\n", disc);
			goto err;
		}
		disc->entities[i].disc = disc;
	}

	if (adp_discovery_send_packet(disc, NULL) < 0)
		goto err;

	os_log(LOG_INIT, "port(%u) disc(%p) done\n", port->port_id, disc);

exit:
	return 0;

err:
	for (j = 0; j < i; j++)
		timer_destroy(&disc->entities[j].timeout);

	return -1;
}

__exit void adp_discovery_exit(struct adp_discovery_ctx *disc)
{
	int i;
	struct avdecc_port *port = discovery_to_avdecc_port(disc);

	if (!port->initialized)
		return;

	for (i = 0; i < disc->max_entities_discovery; i++)
		timer_destroy(&disc->entities[i].timeout);

	os_log(LOG_INIT, "disc(%p) done\n", disc);

}

__init int adp_init(struct adp_ctx *adp)
{
	struct entity *entity = container_of(adp, struct entity, adp);

	if (!entity->milan_mode) {
		if (adp_ieee_advertise_init(&adp->ieee.advertise) < 0)
			goto err_adv;

	} else {
		if (adp_milan_advertise_init(adp) < 0)
			goto err_adv;

		if (adp_milan_listener_sink_discovery_init(adp) < 0)
			goto err_disc;
	}

	return 0;

err_disc:
	adp_milan_advertise_exit(adp);

err_adv:
	return -1;
}

__exit void adp_exit(struct adp_ctx *adp)
{
	struct entity *entity = container_of(adp, struct entity, adp);

	if (!entity->milan_mode) {
		adp_ieee_advertise_exit(&adp->ieee.advertise);

	} else {
		adp_milan_advertise_exit(adp);
		adp_milan_listener_sink_discovery_exit(adp);
	}
}
