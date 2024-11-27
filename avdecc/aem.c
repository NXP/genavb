/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2019, 2021-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @brief AEM common code
 @details Implements AEM related functions
*/

#include "os/stdlib.h"

#include "common/log.h"
#include "common/net.h"

#include "genavb/aem.h"

#include "aem.h"


/*  According to IEEE 1722.1-20013, section 6.2.1.8, the entity ID is a unique EUI-64 assigned
 *  by the vendor manufacturing the device. We can only provide valid EUI-64 for OUIs we own,
 *  so we use the Freescale OUI-24, which also happens to be the OUI used to generate i.MX
 *  MAC addresses (meaning that at least for i.MX devices, the first 6 bytes of the entity ID
 *  should be equal to the MAC address of the first Ethernet interface).
 *
 *  TODO: Add an API to allow a customer to provide its own entity IDs, without relying on
 *  the ones we (GenAVB) generate.
 */
static u8 FSL_OUI[3] = { 0x00, 0x04, 0x9f };
static inline void compute_entity_id_from_mac(u8 *eui, u8 *mac_addr, u8 mod)
{
	eui[0] = FSL_OUI[0];
	eui[1] = FSL_OUI[1];
	eui[2] = FSL_OUI[2];
	eui[3] = mac_addr[3];
	eui[4] = mac_addr[4];
	eui[5] = mac_addr[5];
	eui[6] = 0x00;
	eui[7] = mod;
}

__init static void aem_single_dynamic_desc_init(struct aem_desc_hdr *aem_dynamic_descs, u16 type, void *data, u16 size, u16 total)
{
	struct aem_desc_hdr *desc = &aem_dynamic_descs[type];

	desc->ptr = data;
	desc->size = size;
	desc->total = total;
}

/**
 * Allocates the AVDECC dynamic states memory and init the dynamic states descriptors
 * \return		pointer to the dynamic descriptors memory
 * \param aem_descs	pointer to the entity static descriptors
 */
__init void *aem_dynamic_descs_init(struct aem_desc_hdr *aem_descs, struct avdecc_entity_config *cfg)
{
	unsigned int size, path_sequence_size;
	unsigned int num_interfaces, num_stream_inputs, num_stream_outputs, num_clock_domain;
	void *aem_dynamic_descs;
	unsigned int offset = 0;

	if (!aem_descs)
		goto err;

	size = AEM_NUM_DESC_TYPES * sizeof(struct aem_desc_hdr);

	size += sizeof(struct entity_dynamic_desc);

	num_interfaces = aem_get_descriptor_max(aem_descs, AEM_DESC_TYPE_AVB_INTERFACE);
	path_sequence_size = cfg->max_ptlv_entries * sizeof(struct ptp_clock_identity);
	size += num_interfaces * (sizeof(struct avb_interface_dynamic_desc) + path_sequence_size);

	num_stream_outputs = aem_get_talker_streams(aem_descs);
	size += num_stream_outputs * sizeof(struct stream_output_dynamic_desc);

	num_stream_inputs = aem_get_listener_streams(aem_descs);
	size += num_stream_inputs * sizeof(struct stream_input_dynamic_desc);

	num_clock_domain = aem_get_descriptor_max(aem_descs, AEM_DESC_TYPE_CLOCK_DOMAIN);
	size += num_clock_domain * sizeof(struct clock_domain_dynamic_desc);

	aem_dynamic_descs = os_malloc(size);
	if (!aem_dynamic_descs)
		goto err;

	os_memset(aem_dynamic_descs, 0, size);

	offset = AEM_NUM_DESC_TYPES * sizeof(struct aem_desc_hdr);

	aem_single_dynamic_desc_init(aem_dynamic_descs, AEM_DESC_TYPE_ENTITY, (char *)aem_dynamic_descs + offset,
			sizeof(struct entity_dynamic_desc), 1);

	offset += sizeof(struct entity_dynamic_desc);

	aem_single_dynamic_desc_init(aem_dynamic_descs, AEM_DESC_TYPE_AVB_INTERFACE, (char *)aem_dynamic_descs + offset,
			sizeof(struct avb_interface_dynamic_desc), num_interfaces);

	offset += num_interfaces * (sizeof(struct avb_interface_dynamic_desc) + path_sequence_size);

	aem_single_dynamic_desc_init(aem_dynamic_descs, AEM_DESC_TYPE_STREAM_OUTPUT, (char *)aem_dynamic_descs + offset,
			sizeof(struct stream_output_dynamic_desc), num_stream_outputs);

	offset += num_stream_outputs * sizeof(struct stream_output_dynamic_desc);

	aem_single_dynamic_desc_init(aem_dynamic_descs, AEM_DESC_TYPE_STREAM_INPUT, (char *)aem_dynamic_descs + offset,
			sizeof(struct stream_input_dynamic_desc), num_stream_inputs);

	offset += num_stream_inputs * sizeof(struct stream_input_dynamic_desc);

	aem_single_dynamic_desc_init(aem_dynamic_descs, AEM_DESC_TYPE_CLOCK_DOMAIN, (char *)aem_dynamic_descs + offset,
			sizeof(struct clock_domain_dynamic_desc), num_clock_domain);

	offset += num_clock_domain * sizeof(struct clock_domain_dynamic_desc);

	return aem_dynamic_descs;

err:
	return NULL;
}

__init void aem_init(struct aem_desc_hdr *aem_desc, struct avdecc_entity_config *cfg, int entity_num)
{
	unsigned char local_mac[6] = {0};
	int num_interfaces, i;
	struct entity_descriptor *entity = aem_get_descriptor(aem_desc, AEM_DESC_TYPE_ENTITY, 0, NULL);
	u64 entity_id, association_id;
	struct avdecc_config *avdecc_cfg = container_of(cfg, struct avdecc_config, entity_cfg[entity_num]);

	/* Fetch fields from (in order of descending priority):
	 * . the config file (if available) first,
	 * . or the AEM definition file,
	 * . or dynamically from the MAC address (for the entity id only),
	 * . or dynamically from the entity id (for the entity model id only).
	 */

	if (entity->entity_capabilities &  htonl(ADP_ENTITY_ASSOCIATION_ID_SUPPORTED)) {
		if (cfg->association_id) {
			association_id = htonll(cfg->association_id);
			copy_64(&entity->association_id, &association_id);
		}
	} else
		entity->association_id = 0;

	if (entity->association_id == 0)
		entity->entity_capabilities &= ~htonl(ADP_ENTITY_ASSOCIATION_ID_VALID);
	else
		entity->entity_capabilities |= htonl(ADP_ENTITY_ASSOCIATION_ID_VALID);

	if (cfg->entity_id) {
		entity_id = htonll(cfg->entity_id);
		copy_64(&entity->entity_id, &entity_id);
	} else if (entity->entity_id == 0) {
		/* Entity IDs are per device so just use port0 address for all entities generated on all ports */
		if (net_get_local_addr(avdecc_cfg->logical_port_list[0], local_mac) >= 0)
			compute_entity_id_from_mac((u8 *)&entity->entity_id, local_mac, entity_num);
	}

	if (entity->entity_model_id == 0) {
		os_memcpy(&entity->entity_model_id, &entity->entity_id, 8);
	}


	/* dynamic settings */
	num_interfaces = aem_get_descriptor_max(aem_desc, AEM_DESC_TYPE_AVB_INTERFACE);
	for (i = 0; i < num_interfaces; i++) {
		struct avb_interface_descriptor *interface = aem_get_descriptor(aem_desc, AEM_DESC_TYPE_AVB_INTERFACE, i, NULL);

		if (net_get_local_addr(avdecc_cfg->logical_port_list[i], local_mac) >= 0)
			os_memcpy(interface->mac_address, local_mac, 6);
	}

}
