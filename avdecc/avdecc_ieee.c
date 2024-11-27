/*
 * Copyright 2021-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVDECC IEEE 1722.1 specific code
 @details Handles all AVDECC IEEE 1722.1 specific functions
*/

#include "avdecc.h"
#include "avdecc_ieee.h"

/** Try to establish fast connect with the discovered entity on the specified interface.
 * \return 	none
 * \param 	entity 	       	pointer to local entity struct
 * \param 	entity_desc 	pointer to discovered entity_discovery struct
 * \param 	port_id		port index on which to try the connection.
 */
void avdecc_ieee_try_fast_connect(struct entity *entity, struct entity_discovery *entity_disc, unsigned int port_id)
{
	struct avb_interface_dynamic_desc *avb_itf_dynamic;

	avb_itf_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_AVB_INTERFACE, port_id, NULL);
	if (!avb_itf_dynamic) {
		os_log(LOG_ERR, "entity(%p) invalid interface index (%u)\n", entity, port_id);
		return;
	}

	if ((entity->flags & AVDECC_FAST_CONNECT_MODE)
	&& cmp_64(&entity_disc->info.gptp_grandmaster_id, &avb_itf_dynamic->gptp_grandmaster_id))
		acmp_ieee_listener_fast_connect(&entity->acmp, entity_disc->info.entity_id, port_id);
}

/** Handles changes/updates for the discovered entity
 * \return 	none
 * \param 	avdecc 	        	pointer to avdecc context
 * \param	disc			pointer to discovery context
 * \param 	entity_desc 		pointer to discovered entity_discovery struct
 * \param 	gptp_gmid_changed 	true if the GM id changed for the discovered entity, false otherwise
 */
void avdecc_ieee_discovery_update(struct avdecc_ctx *avdecc, struct adp_discovery_ctx *disc, struct entity_discovery *entity_disc, bool gptp_gmid_changed)
{
	struct avdecc_port *port = container_of(disc, struct avdecc_port, discovery);
	struct entity *entity;
	int i;

	/* DEMO back-to-back */
	/* Find first local entity with Listener capability
	 * (assume at most one listener entity per endpoint).
	 */
	entity = avdecc_get_local_listener_any(avdecc, port->port_id);
	if (!entity) {
		os_log(LOG_DEBUG, "avdecc(%p) port(%u) Couldn't find any local listener entity supporting this port to do fast-connect\n",
			avdecc, port->port_id);
		return;
	}

	/* Prepare fast connect for the first discovered entity which has talker capabilities (and is not ourself) */
	if ((entity_disc->info.talker_capabilities & htons(ADP_TALKER_IMPLEMENTED))
	&& (entity_disc->info.talker_capabilities & htons(ADP_TALKER_VIDEO_SOURCE | ADP_TALKER_AUDIO_SOURCE))
	&& !cmp_64(&entity_disc->info.entity_id, &entity->desc->entity_id)
	&& (entity->flags & AVDECC_FAST_CONNECT_BTB))
		acmp_ieee_listener_fast_connect_btb(&entity->acmp, entity_disc->info.entity_id, port->port_id);
	/* DEMO back-to-back */

	if (gptp_gmid_changed) {
		for (i = 0; i < avdecc->num_entities; i++)
			if (entity_ready(avdecc->entities[i]) && avdecc_entity_port_valid(avdecc->entities[i], port->port_id))
				avdecc_ieee_try_fast_connect(avdecc->entities[i], entity_disc, port->port_id);
	}

}
