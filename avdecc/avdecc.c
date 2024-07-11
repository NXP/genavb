/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2019, 2021-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVDECC common code
 @details Handles all AVDECC entities
*/

#include "os/stdlib.h"
#include "os/string.h"

#include "common/log.h"
#include "common/avtp.h"
#include "common/ether.h"
#include "common/srp.h"

#include "genavb/aem.h"
#include "genavb/qos.h"

#include "avdecc.h"
#include "avdecc_ieee.h"

struct __attribute__ ((packed)) genavb_avdecc_managed_objects_default_ds_request {
	avb_u16 container_id;
	avb_u16 container_len;
	avb_u16 clk_id_leaf_id;
	avb_u16 clk_id_leaf_len;
	avb_u16 priority1_leaf_id;
	avb_u16 priority1_leaf_len;
	avb_u16 clk_class_leaf_id;
	avb_u16 clk_class_leaf_len;
	avb_u16 offset_scaled_log_variance_leaf_id;
	avb_u16 offset_scaled_log_variance_leaf_len;
	avb_u16 clk_accuracy_leaf_id;
	avb_u16 clk_accuracy_leaf_len;
	avb_u16 priority2_leaf_id;
	avb_u16 priority2_leaf_len;
};

struct __attribute__ ((packed)) genavb_avdecc_managed_objects_response_success {
	avb_u16 container_id;    //should be 0
	avb_u16 container_len;   // should be 2 + leaves' data length
	avb_u16 global_status;   // should be 0
};

struct __attribute__ ((packed)) genavb_avdecc_managed_objects_default_ds_response_success {
	avb_u16 clk_id_leaf_id;  // should be 0
	avb_u16 clk_id_leaf_len; // should be 2 + 8 = 10
	avb_u16 clk_id_leaf_status; // should be 0
	avb_u64 clk_id_leaf_value; // correct clockidentity
	avb_u16 priority1_leaf_id; // should be 5
	avb_u16 priority1_leaf_len; // should be 2 + 1
	avb_u16 priority1_leaf_status; // should be 0
	avb_u8 priority1_leaf_value; // correct value
	avb_u16 clk_class_leaf_id; // should be 2
	avb_u16 clk_class_leaf_len; // should be 2 + 1
	avb_u16 clk_class_leaf_status; // should be 0
	avb_u8 clk_class_leaf_value; // correct value
	avb_u16 offset_scaled_log_variance_leaf_id; // should be 4
 	avb_u16 offset_scaled_log_variance_leaf_len; // should be 2 + 2
	avb_u16 offset_scaled_log_variance_leaf_status; // should be 0
	avb_u16 offset_scaled_log_variance_leaf_value; //correct value
	avb_u16 clk_accuracy_leaf_id; // should be 3
	avb_u16 clk_accuracy_leaf_len; // should be 2 + 1
	avb_u16 clk_accuracy_leaf_status; //should be 0
	avb_u8 clk_accuracy_leaf_value; // correct value
	avb_u16 priority2_leaf_id; // should be 6
	avb_u16 priority2_leaf_len; // should be 2 + 1
	avb_u16 priority2_leaf_status; // should be 0
	avb_u8 priority2_leaf_value; // correct value
};

static const char *talker_stream_status2string(genavb_talker_stream_status_t status)
{
	switch (status) {
	case2str(NO_LISTENER);
	case2str(FAILED_LISTENER);
	case2str(ACTIVE_AND_FAILED_LISTENERS);
	case2str(ACTIVE_LISTENER);
	default:
		return (char *) "Unknown talker stream status";
	}
}

static const char *talker_stream_declaration_type2string(genavb_talker_stream_declaration_type_t talker_declaration_type)
{
	switch (talker_declaration_type) {
	case2str(NO_TALKER_DECLARATION);
	case2str(TALKER_ADVERTISE);
	case2str(TALKER_FAILED);
	default:
		return (char *) "Unknown talker declaration type";
	}
}

static const char *listener_stream_status2string(genavb_listener_stream_status_t status)
{
	switch (status) {
	case2str(NO_TALKER);
	case2str(ACTIVE);
	case2str(FAILED);
	default:
		return (char *) "Unknown listener stream status";
	}
}

static const char *listener_stream_declaration_type2string(genavb_listener_stream_declaration_type_t listener_declaration_type)
{
	switch (listener_declaration_type) {
	case2str(NO_LISTENER_DECLARATION);
	case2str(LISTENER_READY);
	case2str(LISTENER_READY_FAILED);
	case2str(LISTENER_FAILED);
	default:
		return (char *) "Unknown listener declaration type";
	}
}

/** gPTP IPC, sends a GENAVB_MSG_MANAGED_GET message to get the default_parameter_data_set container
 * \return none
 * \param port
 */
static void avdecc_ipc_managed_get_default_ds(struct avdecc_port *port)
{
	struct ipc_desc *desc;
	struct genavb_avdecc_managed_objects_default_ds_request *request;
	int rc;

	desc = ipc_alloc(&port->ipc_tx_gptp, sizeof(struct genavb_msg_managed_get));
	if (desc) {
		desc->dst = IPC_DST_ALL;
		desc->type = GENAVB_MSG_MANAGED_GET;
		desc->len = sizeof(struct genavb_msg_managed_get);
		desc->flags = 0;

		os_memset(&desc->u.managed_get, 0, sizeof(struct genavb_msg_managed_get));

		request = (struct genavb_avdecc_managed_objects_default_ds_request *)&desc->u.managed_get;

		request->container_id = 0;
		request->container_len = 24;
		request->clk_id_leaf_id = 0;
		request->priority1_leaf_id = 5;
		request->clk_class_leaf_id = 2;
		request->offset_scaled_log_variance_leaf_id = 4;
		request->clk_accuracy_leaf_id = 3;
		request->priority2_leaf_id = 6;

		rc = ipc_tx(&port->ipc_tx_gptp, desc);
		if (rc < 0) {
			os_log(LOG_ERR, "ipc_tx() failed (%d)\n", rc);
			ipc_free(&port->ipc_tx_gptp, desc);
		}
	} else
		os_log(LOG_ERR, "ipc_alloc() failed\n");
}

/** gPTP IPC response parser, parse a GENAVB_MSG_MANAGED_GET message that contains the default_parameter_data_set data
 * \return none
 * \param entity
 * \param port_id
 * \param managed_object_resp, the response of a managed_get request on default_parameter_data_set container
 */
static void avdecc_ipc_managed_parse_default_ds_response(struct entity *entity, unsigned int port_id, struct genavb_avdecc_managed_objects_response_success *managed_object_resp)
{
	struct avb_interface_descriptor *avb_itf;
	struct genavb_avdecc_managed_objects_default_ds_response_success *dflt_param_data_set = (struct genavb_avdecc_managed_objects_default_ds_response_success *)(managed_object_resp + 1);
	unsigned int container_len_expected, leaf_len_expected;

	if (managed_object_resp->global_status != 0) {
		os_log(LOG_ERR, "Managed GET response status %u failed\n", managed_object_resp->global_status);
		return;
	}

	container_len_expected = sizeof(struct genavb_avdecc_managed_objects_default_ds_response_success);
	container_len_expected += (sizeof(struct genavb_avdecc_managed_objects_response_success) - offsetof(struct genavb_avdecc_managed_objects_response_success, global_status));
	if (managed_object_resp->container_len != container_len_expected) {
		os_log(LOG_ERR, "entity(%p): Managed object get default_parameter_data_set failed: status (%u), len : expected(%u) got(%u)\n",
				entity, managed_object_resp->global_status, container_len_expected, managed_object_resp->container_len);
		goto exit;
	}

	avb_itf = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_AVB_INTERFACE, port_id, NULL);

	if (!avb_itf) {
		os_log(LOG_ERR, "entity(%p): Unsupported avb interface (%u)\n", entity, port_id);
		goto exit;
	}

	leaf_len_expected = sizeof(dflt_param_data_set->clk_id_leaf_status) + sizeof(dflt_param_data_set->clk_id_leaf_value);
	if (dflt_param_data_set->clk_id_leaf_status == 0 && dflt_param_data_set->clk_id_leaf_len == leaf_len_expected) {
		avb_itf->clock_identity = get_64(&dflt_param_data_set->clk_id_leaf_value);

	} else {
		os_log(LOG_ERR, "entity(%p): Managed object get clock identity failed: status (%u), len : expected(%u) got(%u)\n",
				entity, dflt_param_data_set->clk_id_leaf_status, leaf_len_expected, dflt_param_data_set->clk_id_leaf_len);
		goto exit;
	}

	leaf_len_expected = sizeof(dflt_param_data_set->priority1_leaf_status) + sizeof(dflt_param_data_set->priority1_leaf_value);
	if (dflt_param_data_set->priority1_leaf_status == 0 && dflt_param_data_set->priority1_leaf_len == leaf_len_expected) {
		avb_itf->priority1 = dflt_param_data_set->priority1_leaf_value;

	} else {
		os_log(LOG_ERR, "entity(%p): Managed object get clock identity failed: status (%u), len : expected(%u) got(%u)\n",
				entity, dflt_param_data_set->priority1_leaf_status, leaf_len_expected, dflt_param_data_set->priority1_leaf_len);
		goto exit;
	}

	leaf_len_expected = sizeof(dflt_param_data_set->clk_class_leaf_status) + sizeof(dflt_param_data_set->clk_class_leaf_value);
	if (dflt_param_data_set->clk_class_leaf_status == 0 && dflt_param_data_set->clk_class_leaf_len == leaf_len_expected) {
		avb_itf->clock_class = dflt_param_data_set->clk_class_leaf_value;

	} else {
		os_log(LOG_ERR, "entity(%p): Managed object get clock identity failed: status (%u), len : expected(%u) got(%u)\n",
				entity, dflt_param_data_set->clk_class_leaf_status, leaf_len_expected, dflt_param_data_set->clk_class_leaf_len);
		goto exit;
	}

	leaf_len_expected = sizeof(dflt_param_data_set->offset_scaled_log_variance_leaf_status) + sizeof(dflt_param_data_set->offset_scaled_log_variance_leaf_value);
	if (dflt_param_data_set->offset_scaled_log_variance_leaf_status == 0 && dflt_param_data_set->offset_scaled_log_variance_leaf_len == leaf_len_expected) {
		avb_itf->offset_scaled_log_variance = dflt_param_data_set->offset_scaled_log_variance_leaf_value;

	} else {
		os_log(LOG_ERR, "entity(%p): Managed object get clock identity failed: status (%u), len : expected(%u) got(%u)\n",
				entity, dflt_param_data_set->offset_scaled_log_variance_leaf_status, leaf_len_expected, dflt_param_data_set->offset_scaled_log_variance_leaf_len);
		goto exit;
	}

	leaf_len_expected = sizeof(dflt_param_data_set->clk_accuracy_leaf_status) + sizeof(dflt_param_data_set->clk_accuracy_leaf_value);
	if (dflt_param_data_set->clk_accuracy_leaf_status == 0 && dflt_param_data_set->clk_accuracy_leaf_len == leaf_len_expected) {
		avb_itf->clock_accuracy = dflt_param_data_set->clk_accuracy_leaf_value;

	} else {
		os_log(LOG_ERR, "entity(%p): Managed object get clock identity failed: status (%u), len : expected(%u) got(%u)\n",
				entity, dflt_param_data_set->clk_accuracy_leaf_status, leaf_len_expected, dflt_param_data_set->clk_accuracy_leaf_len);
		goto exit;
	}

	leaf_len_expected = sizeof(dflt_param_data_set->priority2_leaf_status) + sizeof(dflt_param_data_set->priority2_leaf_value);
	if (dflt_param_data_set->priority2_leaf_status == 0 && dflt_param_data_set->priority2_leaf_len == leaf_len_expected) {
		avb_itf->priority2 = dflt_param_data_set->priority2_leaf_value;

	} else {
		os_log(LOG_ERR, "entity(%p): Managed object get clock identity failed: status (%u), len : expected(%u) got(%u)\n",
				entity, dflt_param_data_set->priority2_leaf_status, leaf_len_expected, dflt_param_data_set->priority2_leaf_len);
		goto exit;
	}

exit:
	return;
}

/** gPTP IPC response, receive handler for GENAVB_MSG_MANAGED_GET messages
 * \return none
 * \param entity
 * \param port_id
 * \param managed_get_response, the response of a managed_get request
 */
static void avdecc_ipc_managed_get_response(struct entity *entity, unsigned int port_id, struct genavb_msg_managed_get_response *managed_get_response)
{
	struct genavb_avdecc_managed_objects_response_success *managed_object_resp = (struct genavb_avdecc_managed_objects_response_success *)managed_get_response;

	switch (managed_object_resp->container_id) {
	case 0: // default_parameter_data_set
		avdecc_ipc_managed_parse_default_ds_response(entity, port_id, managed_object_resp);
		break;

	default:
		os_log(LOG_ERR, "Unsupported managed container id %u\n", managed_object_resp->container_id);
		break;
	}
}

void avdecc_inflight_restart(struct inflight_ctx *entry)
{
	timer_stop(&entry->timeout);
	timer_start(&entry->timeout, entry->timeout_ms);
}

/** Inflight timeout callback.
 * \return	none
 * \param	data	timer private data
 */
void avdecc_inflight_timeout(void *data)
{
	struct inflight_ctx *entry = data;
	int rc;

	rc = entry->cb(entry);

	os_log(LOG_DEBUG, "Inflight (%p) timeout, cb returned %d\n", entry, rc);

	if (rc == AVDECC_INFLIGHT_TIMER_STOP) {
		list_del(&entry->list);
		list_add(&entry->entity->free_inflight, &entry->list);
		timer_stop(&entry->timeout);
	}
	else if (rc == AVDECC_INFLIGHT_TIMER_RESTART)
		avdecc_inflight_restart(entry);
	else
		os_log(LOG_CRIT, "callback returned invalid return code...\n");
}

/** Gets a free inflight context.
 * \return	pointer to a free inflight context, NULL if not available
 * \param	entity		pointer to the entity context to get an inflight entry from.
 */
struct inflight_ctx *avdecc_inflight_get(struct entity *entity)
{
	struct inflight_ctx *entry = NULL;
	struct list_head *list_entry;

	if (!list_empty(&entity->free_inflight)) {
		list_entry = list_first(&entity->free_inflight);
		entry = container_of(list_entry, struct inflight_ctx, list);
		list_del(list_entry);
	}
	else
		os_log(LOG_ERR, "entity(%p) No more inflight entries available\n", entity);

	return entry;
}

/** Enqueues the provided inflight context and starts its associated timeout.
 * It is used to save a command PDU and to manage the retransmission.
 * \return	0
 * \param	inflight_head	pointer to the list head
 * \param	entry		pointer to the inflight context
 * \param	timeout		timeout in ms
 */
int avdecc_inflight_start(struct list_head *inflight_head, struct inflight_ctx *entry, unsigned int timeout)
{

	entry->list_head = inflight_head;
	list_add(inflight_head, &entry->list);
	entry->timeout_ms = timeout;
	timer_start(&entry->timeout, timeout);

	os_log(LOG_DEBUG, "Started inflight (%p) with sequence id %d, timeout %d ms\n", entry, entry->data.sequence_id, timeout);

	return 0;
}


/**
 * Puts an inflight entry back to the free list.
 * The entry must not be in use: must be called after calling ::avdecc_inflight_get but before calling ::avdecc_inflight_start.
 * \param 	entity	Entity context the entry should be returned to.
 * \param	entry	Inflight entry to return to the free inflight list.
 */
void avdecc_inflight_abort(struct entity *entity, struct inflight_ctx *entry)
{
	list_add(&entity->free_inflight, &entry->list);
}



struct inflight_ctx *avdecc_inflight_find(struct list_head *inflight_head, u16 sequence_id)
{
	struct list_head *list_entry;
	struct inflight_ctx *entry;

	list_entry = list_first(inflight_head);

	while (list_entry != inflight_head) {
		entry = container_of(list_entry, struct inflight_ctx, list);

		if (sequence_id == entry->data.sequence_id)
			return entry;

		list_entry = list_next(list_entry);
	}

	return NULL;
}

/**
 * Checks the entity lock to see if the controller is authorized or not.
 * \return false if the controller is authorized (either entity is not locked or
 * it's the controller owning the lock), or true if entity is locked by another controller
 * \param entity	pointer to the entity structure to check
 * \param controller_id	controller entity ID trying to alter the entity state.
 */
bool avdecc_entity_is_locked(struct entity *entity, u64 controller_id)
{
       struct entity_dynamic_desc *entity_dynamic = aem_get_descriptor(entity->aem_dynamic_descs,
                                                                                     AEM_DESC_TYPE_ENTITY, 0, NULL);
       if ((entity_dynamic->lock_status == LOCKED)
               && (entity_dynamic->locking_controller_id != controller_id)) {

               return true;
       }

       return false;
}

/**
 * Checks the entity acquire status to see if the controller is authorized or not.
 * \return false if the controller is authorized (either entity is not acquired or
 * it's the controller owning the lock), or true if entity is locked by another controller
 * \param entity	pointer to the entity structure to check
 * \param controller_id	controller entity ID trying to alter the entity state.
 */
bool avdecc_entity_is_acquired(struct entity *entity, u64 controller_id)
{
       struct entity_dynamic_desc *entity_dynamic = aem_get_descriptor(entity->aem_dynamic_descs,
                                                                                     AEM_DESC_TYPE_ENTITY, 0, NULL);
       if ((entity_dynamic->acquire_status == ACQUIRED)
               && (entity_dynamic->acquiring_controller_id != controller_id)) {

               return true;
       }

       return false;
}

/**
 * Find an inflight entry matching the provided sequence ID and controller entity ID.
 * \return pointer to found inflight entry or NULL.
 * \param inflight_head	Inflight list to search.
 * \param sequence_id	sequence ID to match.
 * \param controller_id	controller entity ID to match.
 */
struct inflight_ctx *aem_inflight_find_controller(struct list_head *inflight_head, u16 sequence_id, u64 controller_id)
{
	struct list_head *list_entry;
	struct inflight_ctx *entry;

	list_entry = list_first(inflight_head);

	while (list_entry != inflight_head) {
		entry = container_of(list_entry, struct inflight_ctx, list);

		if ((sequence_id == entry->data.sequence_id) && (controller_id == entry->data.pdu.aem.controller_entity_id))
			return entry;

		list_entry = list_next(list_entry);
	}

	return NULL;
}


void avdecc_inflight_remove(struct entity *entity, struct inflight_ctx *entry)
{
	if (entry) {
		timer_stop(&entry->timeout);
		list_del(&entry->list);
		list_add(&entity->free_inflight, &entry->list);
		os_log(LOG_DEBUG, "Removed inflight (%p)\n", entry);
	}
}

/**
 * Removes a PDU from the inflight list, based on the PDU sequence ID.
 * \return	* -1 if no inflight PDU matched the sequence ID.
 * 			* 0 if a PDU was found.
 * \param	entity			Pointer to entity context.
 * \param 	inflight_head	Inflight list to search/update.
 * \param	sequence_id		Sequence ID to be searched and removed from the list.
 * \param	orig_seq_id		On successful return, will contain the sequence ID of the original PDU that triggered the command.
 * \param 	priv0			On successful return, will contain the inflight first entry private data.
 * \param 	priv1			On successful return, will contain the inflight second entry private data.
  */
int avdecc_inflight_cancel(struct entity *entity, struct list_head *inflight_head, u16 sequence_id, u16 *orig_seq_id, void **priv0, void **priv1)
{
	struct inflight_ctx *entry;

	entry = avdecc_inflight_find(inflight_head, sequence_id);
	if (entry) {
		if (orig_seq_id)
			*orig_seq_id = entry->data.orig_seq_id;
		if (priv0)
			*priv0 = (void *)entry->data.priv[0];
		if (priv1)
			*priv1 = (void *)entry->data.priv[1];

		avdecc_inflight_remove(entity, entry);
		os_log(LOG_DEBUG, "Cancelled inflight (%p)\n", entry);

		return 0;
	}

	return -1;
}

__init static unsigned int avdecc_inflight_data_size(struct avdecc_entity_config *cfg)
{
	return cfg->max_inflights * sizeof(struct inflight_ctx);
}

__init static int avdecc_inflight_init(struct entity *entity, void *data, struct avdecc_entity_config *cfg)
{
	int i;

	entity->inflight_storage = (struct inflight_ctx *)data;
	entity->max_inflights = cfg->max_inflights;

	list_head_init(&entity->free_inflight);

	for (i = 0; i < entity->max_inflights; i++) {
		list_add(&entity->free_inflight, &entity->inflight_storage[i].list);
		entity->inflight_storage[i].timeout.data = &entity->inflight_storage[i];
		entity->inflight_storage[i].timeout.func = avdecc_inflight_timeout;
		if (timer_create(entity->avdecc->timer_ctx, &entity->inflight_storage[i].timeout, 0, AVDECC_CFG_INFLIGHT_TIMER_RESOLUTION) < 0) {
			os_log(LOG_ERR, "entity(%p) timer_create failed\n", entity);
			goto err;
		}
		entity->inflight_storage[i].entity = entity;
	}

	os_log(LOG_INIT, "avdecc(%p) entity(%p) %d inflight commands max\n", entity->avdecc, entity, entity->max_inflights);

	return 0;

err:
	while(i--)
		timer_destroy(&entity->inflight_storage[i].timeout);

	return -1;
}

__exit static int avdecc_inflight_exit(struct entity *entity)
{
	int i;

	for (i = 0; i < entity->max_inflights; i++)
		timer_destroy(&entity->inflight_storage[i].timeout);

	return 0;
}

static void avdecc_entity_lock_timeout(void *data)
{
	struct entity *entity = (struct entity *)data;
	struct entity_dynamic_desc *entity_dynamic = aem_get_descriptor(entity->aem_dynamic_descs,
										AEM_DESC_TYPE_ENTITY, 0, NULL);

	entity_dynamic->lock_status = UNLOCKED;

	/* Send unsolicited notification (notify that the entity is now unlocked)
	* Per AVNU.IO.CONTROL 7.5.2.
	*/
	if (entity->milan_mode)
		aecp_aem_send_async_unsolicited_notification(&entity->aecp, AECP_AEM_CMD_LOCK_ENTITY, AEM_DESC_TYPE_ENTITY, 0);
}

__init static bool avdecc_entity_check(struct entity *entity)
{
	struct avdecc_ctx *avdecc = entity->avdecc;
	struct stream_descriptor *stream_desc;
	struct control_descriptor *control_desc;
	unsigned int num_interfaces, num_stream_input, num_stream_output, num_control;
	u16 avb_itf_index, num_of_formats, control_value_type;
	int i;

	num_interfaces = aem_get_descriptor_max(entity->aem_descs, AEM_DESC_TYPE_AVB_INTERFACE);
	if (num_interfaces > avdecc->port_max) {
		os_log(LOG_ERR, "entity(%p) unsupported num interfaces %u, max ports %u\n",
			entity, num_interfaces, avdecc->port_max);
		goto err;
	}

	for (i = 0; i < num_interfaces; i++) {
		if (!avdecc->port[i].initialized) {
			os_log(LOG_ERR, "entity(%p) unsupported avb interface index %u\n", entity, i);
			goto err;
		}
	}

	num_stream_input = aem_get_descriptor_max(entity->aem_descs, AEM_DESC_TYPE_STREAM_INPUT);

	for (i = 0; i < num_stream_input; i++) {
		stream_desc = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_STREAM_INPUT, i, NULL);
		if (!stream_desc) {
			os_log(LOG_ERR, "entity(%p) invalid stream input index %u\n", entity, i);
			goto err;
		}

		avb_itf_index = ntohs(stream_desc->avb_interface_index);
		if (avb_itf_index >= avdecc->port_max || !avdecc->port[avb_itf_index].initialized) {
			os_log(LOG_ERR, "entity(%p) unsupported avb interface index %u for stream input %u\n",
				entity, avb_itf_index, i);
			goto err;
		}

		num_of_formats = ntohs(stream_desc->number_of_formats);
		if (num_of_formats > AEM_NUM_FORMATS_MAX) {
			os_log(LOG_ERR, "entity(%p) number of formats %u exceeding max %u for stream input %u\n",
				entity, num_of_formats, AEM_NUM_FORMATS_MAX, i);
			goto err;
		}
	}

	num_stream_output = aem_get_descriptor_max(entity->aem_descs, AEM_DESC_TYPE_STREAM_OUTPUT);

	for (i = 0; i < num_stream_output; i++) {
		stream_desc = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_STREAM_OUTPUT, i, NULL);
		if (!stream_desc) {
			os_log(LOG_ERR, "entity(%p) invalid stream output index %u\n", entity, i);
			goto err;
		}

		avb_itf_index = ntohs(stream_desc->avb_interface_index);
		if (avb_itf_index >= avdecc->port_max || !avdecc->port[avb_itf_index].initialized) {
			os_log(LOG_ERR, "entity(%p) unsupported avb interface index %u for stream output %u\n",
				entity, avb_itf_index, i);
			goto err;
		}

		num_of_formats = ntohs(stream_desc->number_of_formats);
		if (num_of_formats > AEM_NUM_FORMATS_MAX) {
			os_log(LOG_ERR, "entity(%p) number of formats %u exceeding max %u for stream output %u\n",
				entity, num_of_formats, AEM_NUM_FORMATS_MAX, i);
			goto err;
		}
	}

	num_control = aem_get_descriptor_max(entity->aem_descs, AEM_DESC_TYPE_CONTROL);

	for (i = 0; i < num_control; i++) {
		control_desc = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_CONTROL, i, NULL);
		if (!control_desc) {
			os_log(LOG_ERR, "entity(%p) invalid control index %u\n", entity, i);
			goto err;
		}

		control_value_type = AEM_CONTROL_GET_VALUE_TYPE(ntohs(control_desc->control_value_type));

		switch (control_value_type) {
		case AEM_CONTROL_LINEAR_UINT8:
			if (ntohs(control_desc->number_of_values) > AEM_NUM_VALUE_DETAILS_LINEAR_U8_MAX) {
				os_log(LOG_ERR, "entity(%p) control descriptor(%d) control_value_type(%u): number_of_values(%u) exceeding max (%u)\n",
							entity, i, control_value_type, ntohs(control_desc->number_of_values), AEM_NUM_VALUE_DETAILS_LINEAR_U8_MAX);
				goto err;
			}
			break;

		case AEM_CONTROL_UTF8:
			if (ntohs(control_desc->number_of_values) > AEM_NUM_VALUE_DETAILS_LINEAR_UTF8_MAX) {
				os_log(LOG_ERR, "entity(%p) control descriptor(%d) control_value_type(%u): number_of_values(%u) exceeding max (%u)\n",
							entity, i, control_value_type, ntohs(control_desc->number_of_values), AEM_NUM_VALUE_DETAILS_LINEAR_UTF8_MAX);
				goto err;
			}
			break;

		default:
			os_log(LOG_ERR, "entity(%p) control descriptor(%d) unsupported control_value_type(%u)\n", entity, i, control_value_type);
			goto err;
		}
	}

	return true;

err:
	return false;
}

__init static int avdecc_dynamic_states_init(struct entity *entity, struct avdecc_entity_config *cfg)
{
	struct entity_dynamic_desc *entity_dynamic;
	struct avb_interface_dynamic_desc *avb_itf_dynamic;
	unsigned int num_interfaces;
	int i;
	struct avdecc_ctx *avdecc = entity->avdecc;

	entity_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_ENTITY, 0, NULL);
	if (!entity_dynamic)
		goto err_entity;

	entity_dynamic->lock_status = UNLOCKED;
	entity_dynamic->acquire_status = RELEASED;

	entity_dynamic->lock_timer.func = avdecc_entity_lock_timeout;
	entity_dynamic->lock_timer.data = entity;

	if (timer_create(avdecc->timer_ctx, &entity_dynamic->lock_timer, 0, AVDECC_CFG_ENTITY_LOCK_TIMER_GRANULARITY_MS) < 0) {
		os_log(LOG_ERR, "entity(%p) timer_create failed\n", entity);
		goto err_timer;
	}

	num_interfaces = aem_get_descriptor_max(entity->aem_descs, AEM_DESC_TYPE_AVB_INTERFACE);
	for (i = 0; i < num_interfaces; i++) {
		avb_itf_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_AVB_INTERFACE, i, NULL);
		if (!avb_itf_dynamic)
			goto err_interfaces;

		avb_itf_dynamic->link_down = 0;
		avb_itf_dynamic->link_up = 0;
		avb_itf_dynamic->operational_state = false;
		avb_itf_dynamic->as_capable = false;

		avb_itf_dynamic->gptp_gm_changed = 0;
		avb_itf_dynamic->gptp_grandmaster_id = 0;

		avb_itf_dynamic->propagation_delay = 0;

		avb_itf_dynamic->max_ptlv_entries = cfg->max_ptlv_entries;
		avb_itf_dynamic->num_ptlv_entries = 0;
	}

	return 0;

err_interfaces:
	timer_destroy(&entity_dynamic->lock_timer);

err_timer:
err_entity:
	return -1;
}

__init static int avdecc_entity_mc_init(struct entity *entity)
{
	unsigned char mc_addr[6] = MC_ADDR_AVDECC_ADP_ACMP;
	struct avdecc_ctx *avdecc = entity->avdecc;
	int num_interfaces, i, j;

	/* dynamic settings */
	num_interfaces = aem_get_descriptor_max(entity->aem_descs, AEM_DESC_TYPE_AVB_INTERFACE);

	for (i = 0; i < num_interfaces; i++) {

		if (net_add_multi(&avdecc->port[i].net_rx, avdecc_port_to_logical(entity->avdecc, i), mc_addr) < 0) {
			os_log(LOG_ERR, "avdecc(%p) port(%u) entity(%p) cannot add multicast address\n", avdecc, i, entity);
			goto err;
		}
	}
	return 0;

err:
	for (j = 0; j < i; j++)
		net_del_multi(&avdecc->port[j].net_rx, avdecc_port_to_logical(entity->avdecc, i), mc_addr);

	return -1;
}

__exit static void avdecc_entity_mc_exit(struct entity *entity)
{
	unsigned char mc_addr[6] = MC_ADDR_AVDECC_ADP_ACMP;
	struct avdecc_ctx *avdecc = entity->avdecc;
	int num_interfaces, i;

	/* dynamic settings */
	num_interfaces = aem_get_descriptor_max(entity->aem_descs, AEM_DESC_TYPE_AVB_INTERFACE);

	for (i = 0; i < num_interfaces; i++)
		net_del_multi(&avdecc->port[i].net_rx, avdecc_port_to_logical(entity->avdecc, i), mc_addr);
}

__exit static void avdecc_dynamic_states_exit(struct entity *entity)
{
	struct entity_dynamic_desc *entity_dynamic;

	entity_dynamic = aem_get_descriptor(entity->aem_dynamic_descs,
						AEM_DESC_TYPE_ENTITY, 0, NULL);

	timer_destroy(&entity_dynamic->lock_timer);

}

__init static struct entity *avdecc_entity_alloc(struct avdecc_entity_config *cfg)
{
	struct entity *entity;
	unsigned int size;

	size = sizeof(struct entity);
	size += avdecc_inflight_data_size(cfg);
	size += aecp_data_size(cfg);
	size += acmp_data_size(cfg);

	entity = os_malloc(size);
	if (!entity)
		goto err;

	os_memset(entity, 0, size);

	return entity;

err:
	return NULL;
}

/* Common AVDECC code entry points */
__init static struct entity *avdecc_entity_init(struct avdecc_ctx *avdecc, int entity_num, struct avdecc_entity_config *cfg)
{
	struct entity *entity;
	u8 *avdecc_data, *acmp_data, *aecp_data;

	/* Pass the milan mode to the entity cfg. */
	cfg->milan_mode = avdecc->milan_mode;

	entity = avdecc_entity_alloc(cfg);
	if (!entity)
		goto err_malloc;

	entity->flags = cfg->flags;
	entity->channel_waitmask = cfg->channel_waitmask;
	entity->valid_time = cfg->valid_time;

	entity->milan_mode = avdecc->milan_mode;
	entity->avdecc = avdecc;

	if (cfg->aem)
		entity->aem_descs = cfg->aem;
	else
		entity->aem_descs = aem_entity_static_init();

	aem_init(entity->aem_descs, cfg, entity_num);

	entity->aem_dynamic_descs = aem_dynamic_descs_init(entity->aem_descs, cfg);
	if (!entity->aem_dynamic_descs) {
		os_log(LOG_CRIT, "Cannot init dynamic descriptors for entity (%d)\n", entity_num);
		goto err_dynamic_desc_init;
	}

	entity->desc = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_ENTITY, 0, NULL);
	if (!entity->desc) {
		os_log(LOG_CRIT, "Cannot find entity (%d)\n", entity_num);
		goto err_entity_desc;
	}

	entity->index = entity_num;
	entity->channel_openmask = 0;

	if (!avdecc_entity_check(entity))
		goto err_entity_check;

	if (avdecc_dynamic_states_init(entity, cfg) < 0)
		goto err_dynamic_states_init;

	avdecc_data = (u8 *)(entity + 1);
	if (avdecc_inflight_init(entity, avdecc_data, cfg) < 0)
		goto err_inflight;

	if (adp_init(&entity->adp) < 0)
		goto err_adp;

	aecp_data = avdecc_data + avdecc_inflight_data_size(cfg);
	if (aecp_init(&entity->aecp, aecp_data, cfg) < 0)
		goto err_aecp;

	acmp_data = (acmp_data_size(cfg)) ? (aecp_data + aecp_data_size(cfg)) : NULL;
	if (acmp_init(&entity->acmp, acmp_data, cfg) < 0)
		goto err_acmp;

	if (avdecc_entity_mc_init(entity) < 0)
		goto err_multi;

	os_log(LOG_INIT, "entity(%d) (%p) desc(%p), id : %016"PRIx64", name : %s\n",
		entity_num, entity, entity->desc, ntohll(entity->desc->entity_id), entity->desc->entity_name);

	return entity;

err_multi:
	acmp_exit(&entity->acmp);
err_acmp:
	aecp_exit(&entity->aecp);
err_aecp:
	adp_exit(&entity->adp);
err_adp:
	avdecc_inflight_exit(entity);
err_inflight:
	avdecc_dynamic_states_exit(entity);
err_dynamic_states_init:
err_entity_check:
err_entity_desc:
err_dynamic_desc_init:
	os_free(entity);
err_malloc:
	return NULL;
}

__exit static void avdecc_entity_exit(struct entity *entity)
{
	avdecc_dynamic_states_exit(entity);
	avdecc_entity_mc_exit(entity);
	acmp_exit(&entity->acmp);
	aecp_exit(&entity->aecp);
	adp_exit(&entity->adp);
	avdecc_inflight_exit(entity);
	os_free(entity);
}

/** Main AVDECC network receive function.
 * Receives all AVDECC frames and routes them to the different sub-stack
 * components.
 * \return 	none
 * \param 	rx 	pointer to network receive context
 * \param 	desc 	pointer to network receive descriptor
 */
void avdecc_net_rx(struct net_rx *rx, struct net_rx_desc *desc)
{
	struct eth_hdr *eth = (struct eth_hdr *)((char *)desc + desc->l2_offset);
	struct avtp_ctrl_hdr *avtp = (struct avtp_ctrl_hdr *)((char *)desc + desc->l3_offset);
	struct avdecc_port *port = container_of(rx, struct avdecc_port, net_rx);

	switch (avtp->subtype) {
	case AVTP_SUBTYPE_ADP:
		adp_net_rx(port, (struct adp_pdu *)(avtp + 1), avtp->control_data, AVTP_GET_STATUS(avtp), eth->src);
		break;
	case AVTP_SUBTYPE_AECP:
		aecp_net_rx(port, (struct aecp_pdu *)(avtp + 1), avtp->control_data, AVTP_GET_STATUS(avtp), AVTP_GET_CTRL_DATA_LEN(avtp) + 8, eth->src); /* Add 8 to account for the entity id field */
		break;
	case AVTP_SUBTYPE_ACMP:
		acmp_net_rx(port, (struct acmp_pdu *)(avtp + 1), avtp->control_data, AVTP_GET_STATUS(avtp));
		break;
	default :
		os_log(LOG_ERR, "unknown subtype %x\n", avtp->subtype);
		break;
	}

	net_rx_free(desc);
}


static void avdecc_local_entity_updated(struct avdecc_ctx *avdecc, struct entity *entity)
{
	struct adp_discovery_ctx *disc;
	unsigned int num_interfaces;
	int i, port;

	if (entity_ready(entity)) {
		adp_update(&entity->adp);

		if (!avdecc->milan_mode) {

			num_interfaces = aem_get_descriptor_max(entity->aem_descs, AEM_DESC_TYPE_AVB_INTERFACE);

			for (port = 0; port < num_interfaces; port++) {
				disc = &avdecc->port[port].discovery;

				for (i = 0; i < disc->max_entities_discovery; i++) {
					if (disc->entities[i].in_use)
						avdecc_ieee_try_fast_connect(entity, &disc->entities[i], port);
				}
			}
		} else if (aem_get_talker_streams(entity->aem_descs)) {
			acmp_milan_talkers_maap_start(entity);
		}
	}
}

/** AVDECC IPC receive callback.
 * IPC received from MAAP
 * \return 	none
 * \param 	rx 	pointer to IPC receive context
 * \param 	desc 	pointer to IPC descriptor
 */
void avdecc_ipc_rx_maap(struct ipc_rx const *rx, struct ipc_desc *desc)
{
	struct entity *entity;
	struct avdecc_ctx *avdecc = container_of(rx, struct avdecc_ctx, ipc_rx_maap);
	avb_u16 status, port_id;
	avb_u8 *base_address;
	avb_u16 count;
	avb_u32 range_id;

	os_log(LOG_DEBUG, "Received MAAP IPC type: %d\n", desc->type);

	switch (desc->type) {
	case GENAVB_MSG_MAAP_STATUS:
		if (desc->len != sizeof(struct genavb_maap_status)) {
			os_log(LOG_ERR, "MAAP status message failed\n");
			break;
		}

		status = desc->u.maap_status.status;
		count = desc->u.maap_status.count;
		base_address = desc->u.maap_status.base_address;
		port_id = desc->u.maap_status.port_id;
		range_id = desc->u.maap_status.range_id;

		os_log(LOG_INFO, "avdecc(%p) port(%u) range(%u) status(%u) base_address (%02x:%02x:%02x:%02x:%02x:%02x) count (%u)\n",
			avdecc, port_id, range_id, status, base_address[0], base_address[1], base_address[2], base_address[3], base_address[4],
			base_address[5], count);

		if (avdecc->milan_mode) {
			entity = avdecc_get_local_talker(avdecc, port_id);
			if (!entity) {
				os_log(LOG_DEBUG, "avdecc(%p) port(%u) Couldn't find any local talker entity supporting this port.\n",
					avdecc, port_id);
				goto exit;
			}

			if (status == MAAP_STATUS_SUCCESS)
				acmp_milan_talker_maap_valid(entity, port_id, range_id, base_address, count);
			else if (status == MAAP_STATUS_CONFLICT)
				acmp_milan_talker_maap_conflict(entity, port_id, range_id, base_address, count);
		}

		break;

	case GENAVB_MSG_MAAP_CREATE_RANGE_RESPONSE:
		status = desc->u.maap_create_response.status;
		range_id = desc->u.maap_create_response.range_id;
		port_id = desc->u.maap_create_response.port_id;

		if (status != MAAP_RESPONSE_SUCCESS) {
			os_log(LOG_ERR, "avdecc(%p) port(%u) range(%u) error on range creation\n", avdecc, port_id, range_id);
			goto exit;
		}

		os_log(LOG_DEBUG, "avdecc(%p) port(%u) range(%u) range creation success\n", avdecc, port_id, range_id);

		break;

	case GENAVB_MSG_MAAP_DELETE_RANGE_RESPONSE:
		status = desc->u.maap_delete_response.status;
		range_id = desc->u.maap_delete_response.range_id;
		port_id = desc->u.maap_delete_response.port_id;

		if (status != MAAP_RESPONSE_SUCCESS) {
			os_log(LOG_ERR, "avdecc(%p) port(%u) range(%u) error on range deletion\n", avdecc, port_id, range_id);
			goto exit;
		}

		os_log(LOG_DEBUG, "avdecc(%p) port(%u) range(%u) range deletion success\n", avdecc, port_id, range_id);

		break;

	default:
		break;
	}

exit:
	ipc_free(rx, desc);
}

/** AVDECC IPC receive callback.
 * IPC received from gPTP application.
 * \return 	none
 * \param 	rx 	pointer to IPC receive context
 * \param 	desc 	pointer to IPC descriptor
 */
void avdecc_ipc_rx_gptp(struct ipc_rx const *rx, struct ipc_desc *desc)
{
	struct avdecc_port *port = container_of(rx, struct avdecc_port, ipc_rx_gptp);
	struct avdecc_ctx *avdecc = avdecc_port_to_context(port);
	struct avb_interface_dynamic_desc *avb_itf_dynamic;
	struct avb_interface_descriptor *avb_itf;
	int i;

	os_log(LOG_DEBUG, "Received IPC type: %d\n", desc->type);

	switch (desc->type) {
	case GENAVB_MSG_GM_STATUS:
		if ((desc->len < (sizeof(struct genavb_msg_gm_status))) || (desc->len != (sizeof(struct genavb_msg_gm_status) + desc->u.gm_status.num_ptlv * sizeof(struct ptp_clock_identity)))) {
			os_log(LOG_ERR, "avdecc(%p) grandmaster status message failed", avdecc);
			break;
		}

		/* We can receive indication from multiple domains, filter on the supported domain.
		 * FIXME: make the supported domain configurable
		 */
		if (desc->u.gm_status.domain != CFG_DEFAULT_GPTP_DOMAIN)
			break;

		for (i = 0; i < avdecc->num_entities; i++) {
			struct entity *entity = avdecc->entities[i];
			bool path_sequence_changed = false;

			avb_itf = aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_AVB_INTERFACE, port->port_id, NULL);
			avb_itf_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_AVB_INTERFACE, port->port_id, NULL);

			if (!avb_itf || !avb_itf_dynamic) /* Interface index not supported in this entity */
				continue;

			/* update the domain number */
			avb_itf->domain_number = desc->u.gm_status.domain;

			if (avb_itf_dynamic->num_ptlv_entries != desc->u.gm_status.num_ptlv)
				path_sequence_changed = true;

			/* update the path sequence to the grandmaster as per Milan Dicovery Connection Control 6.6.1 */
			if (desc->u.gm_status.num_ptlv <= avb_itf_dynamic->max_ptlv_entries) {
				avb_itf_dynamic->num_ptlv_entries = desc->u.gm_status.num_ptlv;
			} else {
				/* Truncate the path trace to the max supported size. */
				avb_itf_dynamic->num_ptlv_entries = avb_itf_dynamic->max_ptlv_entries;
			}

			if (path_sequence_changed || os_memcmp(&avb_itf_dynamic->path_sequence, &desc->u.gm_status.path_sequence, avb_itf_dynamic->num_ptlv_entries * sizeof(struct ptp_clock_identity))) {
				os_memcpy(&avb_itf_dynamic->path_sequence, &desc->u.gm_status.path_sequence, avb_itf_dynamic->num_ptlv_entries * sizeof(struct ptp_clock_identity));

				path_sequence_changed = true;
			}

			if (!cmp_64(&avb_itf_dynamic->gptp_grandmaster_id, &desc->u.gm_status.gm_id)) {
				os_log(LOG_INFO, "GTP GM id change for local entity(%p) old 0x%"PRIx64" new 0x%"PRIx64"\n",
					entity, ntohll(avb_itf_dynamic->gptp_grandmaster_id), ntohll(desc->u.gm_status.gm_id));

				copy_64(&avb_itf_dynamic->gptp_grandmaster_id, &desc->u.gm_status.gm_id);

				avb_itf_dynamic->gptp_gm_changed++;

				avdecc_local_entity_updated(avdecc, entity);

				if (entity_ready(entity)) {
					if (avdecc->milan_mode) {
						adp_milan_advertise_sm(entity, port->port_id, ADP_MILAN_ADV_GM_CHANGE);

						aecp_milan_register_get_counters_async_notification(entity, port->port_id);
					} else
						adp_ieee_advertise_interface_sm(entity, port->port_id, ADP_INTERFACE_ADV_EVENT_GM_CHANGE);
				}
			}

			if (path_sequence_changed && entity_ready(entity)) {
				/* Register send of an asynchronous GET_AS_PATH unsolicited notification per MILAN Specification v1.2 5.4.5.2
				 * to notify changes in the path sequence from gPTP
				 */
				aecp_trigger_async_get_as_path_notification(entity, avb_itf_dynamic->interface_index);
			}
		}

		break;

	case GENAVB_MSG_GPTP_PORT_PARAMS:
		if (desc->len != sizeof(struct genavb_msg_gptp_port_params)) {
			os_log(LOG_ERR, "avdecc(%p) gptp port params message failed", avdecc);
			break;
		}

		/* Updates only on the concerned port and filter on the supported domain
		 * FIXME: make the supported domain configurable
		 */
		if (desc->u.gptp_port_params.port_id != port->logical_port || desc->u.gptp_port_params.domain != CFG_DEFAULT_GPTP_DOMAIN)
			break;

		for (i = 0; i < avdecc->num_entities; i++) {
			struct entity *entity = avdecc->entities[i];

			avb_itf_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_AVB_INTERFACE, port->port_id, NULL);

			if (!avb_itf_dynamic) /* Interface index not supported in this entity */
				continue;

			avb_itf_dynamic->as_capable = desc->u.gptp_port_params.as_capable;
			avb_itf_dynamic->propagation_delay = desc->u.gptp_port_params.pdelay;
		}

		break;

	case GENAVB_MSG_MANAGED_GET_RESPONSE:
		if (desc->len > GENAVB_MAX_MANAGED_SIZE) {
			os_log(LOG_ERR, "avdecc(%p) gptp managed get response message failed", avdecc);
			break;
		}

		for (i = 0; i < avdecc->num_entities; i++) {
			struct entity *entity = avdecc->entities[i];

			avb_itf_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_AVB_INTERFACE, port->port_id, NULL);

			if (!avb_itf_dynamic) /* Interface index not supported in this entity */
				continue;

			avdecc_ipc_managed_get_response(entity, port->port_id, &desc->u.managed_get_response);
		}

		break;

	default:
		break;
	}

	ipc_free(rx, desc);
}

static void avdecc_ipc_gm_get_status(struct avdecc_port *port)
{
	struct ipc_desc *desc;
	int rc;

	desc = ipc_alloc(&port->ipc_tx_gptp, sizeof(struct genavb_msg_gm_status));
	if (desc) {
		desc->type = GENAVB_MSG_GM_GET_STATUS;
		desc->len = sizeof(struct genavb_msg_gm_status);
		desc->flags = 0;

		desc->u.gm_get_status.domain = CFG_DEFAULT_GPTP_DOMAIN;

		rc = ipc_tx(&port->ipc_tx_gptp, desc);
		if (rc < 0) {
			os_log(LOG_ERR, "ipc_tx() failed (%d)\n", rc);
			ipc_free(&port->ipc_tx_gptp, desc);
		}
	} else
		os_log(LOG_ERR, "ipc_alloc() failed\n");
}

static void avdecc_ipc_srp_deregister_all(struct avdecc_port *port)
{
	struct avdecc_ctx *avdecc = avdecc_port_to_context(port);
	struct ipc_desc *desc;
	int rc;

	os_log(LOG_INFO, "avdecc(%p) port(%u) Deregister all SRP stream declarations on port.\n", avdecc, port->port_id);

	desc = ipc_alloc(&port->ipc_tx_srp, 0);
	if (desc) {
		desc->type = GENAVB_MSG_DEREGISTER_ALL;
		desc->len = 0;
		desc->flags = 0;

		rc = ipc_tx(&port->ipc_tx_srp, desc);
		if (rc < 0) {
			os_log(LOG_ERR, "ipc_tx() failed (%d)\n", rc);
			ipc_free(&port->ipc_tx_srp, desc);
		}
	} else
		os_log(LOG_ERR, "ipc_alloc() failed\n");
}

/** Find a local entity (ready or not) with Controller capability.
 *  \return	pointer to first found local controller entity (if any), NULL otherwise.
 */
struct entity *avdecc_get_local_controller_any(struct avdecc_ctx *avdecc)
{
	unsigned int i = 0;

	while (i < avdecc->num_entities) {
		if (avdecc->entities[i]->desc->controller_capabilities & htonl(ADP_CONTROLLER_IMPLEMENTED))
			return avdecc->entities[i];
		i++;
	}

	return NULL;
}

/** Find a local entity that is ready, with Controller capability and has the port valid.
 * \param avdecc	pointer to avdecc context
 * \param port_id	avdecc port id/interface index
 *  \return		pointer to first found local controller entity (if any), NULL otherwise.
 */
struct entity *avdecc_get_local_controller(struct avdecc_ctx *avdecc, unsigned int port_id)
{
	unsigned int i = 0;

	while (i < avdecc->num_entities) {
		if ((avdecc->entities[i]->desc->controller_capabilities & htonl(ADP_CONTROLLER_IMPLEMENTED))
			&& entity_ready(avdecc->entities[i]) && avdecc_entity_port_valid(avdecc->entities[i], port_id))
			return avdecc->entities[i];
		i++;
	}

	return NULL;
}

/** Find a local entity (ready or not) with Talker or Listener capability.
 *  \return	pointer to first matching local entity (if any), NULL if none was foudn.
 */
struct entity *avdecc_get_local_controlled_any(struct avdecc_ctx *avdecc)
{
	unsigned int i = 0;

	while (i < avdecc->num_entities) {
		if ((avdecc->entities[i]->desc->listener_capabilities & htons(ADP_LISTENER_IMPLEMENTED)) ||
			(avdecc->entities[i]->desc->talker_capabilities & htons(ADP_TALKER_IMPLEMENTED)))
			return avdecc->entities[i];
		i++;
	}

	return NULL;
}

/** Find a local entity that is ready, with Listener capability and has the port valid.
 * \param avdecc	pointer to avdecc context
 * \param port_id	avdecc port id/interface index
 *  \return		pointer to first found local listener entity (if any), NULL otherwise.
 */
struct entity *avdecc_get_local_listener(struct avdecc_ctx *avdecc, unsigned int port_id)
{
	unsigned int i = 0;

	while (i < avdecc->num_entities) {
		if ((avdecc->entities[i]->desc->listener_capabilities & htons(ADP_LISTENER_IMPLEMENTED))
			&& entity_ready(avdecc->entities[i]) && avdecc_entity_port_valid(avdecc->entities[i], port_id))
			return avdecc->entities[i];
		i++;
	}

	return NULL;
}

/** Find a local entity (ready or not) and with Listener capability and has the port valid.
 * \param avdecc	pointer to avdecc context
 * \param port_id	avdecc port id/interface index
 *  \return		pointer to first found local listener entity (if any), NULL otherwise.
 */
struct entity *avdecc_get_local_listener_any(struct avdecc_ctx *avdecc, unsigned int port_id)
{
	unsigned int i = 0;

	while (i < avdecc->num_entities) {
		if ((avdecc->entities[i]->desc->listener_capabilities & htons(ADP_LISTENER_IMPLEMENTED))
			&& avdecc_entity_port_valid(avdecc->entities[i], port_id))
			return avdecc->entities[i];
		i++;
	}

	return NULL;
}

/** Find a local entity that is ready, with Talker capability and has the port valid.
 * \param avdecc	pointer to avdecc context
 * \param port_id	avdecc port id/interface index
 *  \return		pointer to first found local talker entity (if any), NULL otherwise.
 */
struct entity *avdecc_get_local_talker(struct avdecc_ctx *avdecc, unsigned int port_id)
{
	unsigned int i = 0;

	while (i < avdecc->num_entities) {
		if ((avdecc->entities[i]->desc->talker_capabilities & htons(ADP_TALKER_IMPLEMENTED))
			&& entity_ready(avdecc->entities[i]) && avdecc_entity_port_valid(avdecc->entities[i], port_id))
			return avdecc->entities[i];
		i++;
	}

	return NULL;
}

/** Send an IPC HEARTBEAT message on the provided tx IPC.
 * \ return	0 (or positive) on success, -1 otherwise.
 */
static int avdecc_ipc_send_heartbeat(struct ipc_tx *tx, unsigned int ipc_dst)
{
	struct ipc_desc *tx_desc;
	int rc = 0;

	os_log(LOG_DEBUG, "ipc_tx(%p) Sending IPC heartbeat\n", tx);
	tx_desc = ipc_alloc(tx, sizeof(struct ipc_heartbeat));
	if (tx_desc) {
		tx_desc->dst = ipc_dst;
		tx_desc->type = IPC_HEARTBEAT;
		tx_desc->len = sizeof(struct ipc_heartbeat);
		tx_desc->u.hearbeat.status =  0;


		rc = ipc_tx(tx, tx_desc);
		if (rc < 0) {
			if (rc == -IPC_TX_ERR_QUEUE_FULL)
				os_log(LOG_ERR, "ict_tx(%p) ipc_tx() failed (%d)\n", tx, rc);
			ipc_free(tx, tx_desc);
			rc = -1;
		}
	} else {
		os_log(LOG_ERR, "ipc_tx(%p) ipc_alloc() failed\n", tx);
		rc = -1;
	}

	return rc;
}

/** AVDECC IPC receive callback.
 * IPC received from the controller API.
 * \return 	none
 * \param 	rx 	pointer to IPC receive context
 * \param 	desc 	pointer to IPC descriptor
 */
void avdecc_ipc_rx_controller(struct ipc_rx const *rx, struct ipc_desc *desc)
{
	struct avdecc_ctx *avdecc = container_of(rx, struct avdecc_ctx, ipc_rx_controller);
	struct entity *entity;
	struct ipc_tx *ipc = &avdecc->ipc_tx_controller;
	int rc = -1;

	os_log(LOG_DEBUG, "Received IPC type: %d\n", desc->type);

	/* Assume at most one controller entity per endpoint.
	 * FIXME to be replaced by explicit entity selection by app
	 * if/when multiple controller entities per endpoint is implemented.
	 */
	entity = avdecc_get_local_controller_any(avdecc);
	if (!entity) {
		os_log(LOG_ERR, "avdecc(%p) Couldn't find any local controller entities, ignoring message from application.\n", avdecc);
		goto exit;
	}

	if (desc->flags & IPC_FLAGS_AVB_MSG_SYNC)
		ipc = &avdecc->ipc_tx_controller_sync;

	switch (desc->type) {
	case GENAVB_MSG_ACMP_COMMAND:
		if (entity_ready(entity))
			rc = acmp_ipc_rx(entity, &desc->u.acmp_command, desc->len, ipc, desc->src);
		break;
	case GENAVB_MSG_AECP:
		if (entity_ready(entity))
			rc = aecp_ipc_rx_controller(entity, &desc->u.aecp_msg, desc->len, ipc, desc->src);
		break;
	case GENAVB_MSG_ADP:
		if (entity_ready(entity))
			rc = adp_ipc_rx(entity, &desc->u.adp_msg, desc->len, ipc, desc->src);
		break;

	case IPC_HEARTBEAT:
		os_log(LOG_DEBUG, "Sending HEARTBEAT on ipc_tx(%p)\n", ipc);
		avdecc_ipc_send_heartbeat(ipc, desc->src);

		if (!(entity->channel_openmask & AVDECC_WAITMASK_CONTROLLER)) {
			entity->channel_openmask |= AVDECC_WAITMASK_CONTROLLER;

			avdecc_local_entity_updated(avdecc, entity);
		}
		break;

	default:
		os_log(LOG_ERR, "Received unknown message type %d on a CONTROLLER channel\n", desc->type);
		break;
	}

exit:
	if (rc < 0) {
		// FIXME: If we were unable to send a command that expected a response, send an error response back to the application.
	}

	ipc_free(rx, desc);
}


void avdecc_ipc_rx_controlled(struct ipc_rx const *rx, struct ipc_desc *desc)
{
	struct avdecc_ctx *avdecc = container_of(rx, struct avdecc_ctx, ipc_rx_controlled);
	struct entity *entity;

	os_log(LOG_DEBUG, "avdecc(%p) Received IPC type: %d\n", avdecc, desc->type);

	/* Assume at most one non-controller entity per endpoint.
	 * FIXME to be replaced by explicit entity selection by app
	 * if/when multiple (non-controller) entities per endpoint is implemented.
	 */
	entity = avdecc_get_local_controlled_any(avdecc);
	if (!entity) {
		os_log(LOG_ERR, "avdecc(%p) Couldn't find any local non-controller entities, ignoring message from application.\n", avdecc);
		goto exit;
	}

	switch (desc->type) {
	case GENAVB_MSG_AECP:
		if (entity_ready(entity))
			aecp_ipc_rx_controlled(entity, &desc->u.aecp_msg, desc->len);
		// TODO else ???
		break;

	case IPC_HEARTBEAT:
		avdecc_ipc_send_heartbeat(&avdecc->ipc_tx_controlled, IPC_DST_ALL);

		if (!(entity->channel_openmask & AVDECC_WAITMASK_CONTROLLED)) {
			entity->channel_openmask |= AVDECC_WAITMASK_CONTROLLED;

			avdecc_local_entity_updated(avdecc, entity);
		}
		break;

	default:
		os_log(LOG_ERR, "Received unknown message type %d on a CONTROLLED channel\n", desc->type);
		break;
	}

exit:
	ipc_free(rx, desc);
}


void avdecc_ipc_rx_media_stack(struct ipc_rx const *rx, struct ipc_desc *desc)
{
	struct avdecc_ctx *avdecc = container_of(rx, struct avdecc_ctx, ipc_rx_media_stack);
	struct entity *entity;

	os_log(LOG_DEBUG, "avdecc(%p) Received IPC type: %d\n", avdecc, desc->type);

	switch (desc->type) {
	case IPC_HEARTBEAT:
		/* Assume at most one non-controller entity per endpoint.
		 * FIXME to be replaced by explicit entity selection by app
		 * if/when multiple (non-controller) entities per endpoint is implemented.
		 */
		entity = avdecc_get_local_controlled_any(avdecc);
		if (!entity) {
			os_log(LOG_ERR, "avdecc(%p) Couldn't find any local non-controller entities, ignoring message from application.\n", avdecc);
			goto exit;
		}

		avdecc_ipc_send_heartbeat(&avdecc->ipc_tx_media_stack, IPC_DST_ALL);

		if (!(entity->channel_openmask & AVDECC_WAITMASK_MEDIA_STACK)) {
			entity->channel_openmask |= AVDECC_WAITMASK_MEDIA_STACK;

			avdecc_local_entity_updated(avdecc, entity);
		}
		break;

	case GENAVB_MSG_MEDIA_STACK_BIND:
		if (desc->len != sizeof(struct ipc_media_stack_bind)) {
			os_log(LOG_ERR, "avdecc(%p) wrong length received %u expected %zu\n", avdecc, desc->len, sizeof(struct ipc_media_stack_bind));
			goto exit;
		}

		if (!avdecc->milan_mode) {
			os_log(LOG_ERR, "avdecc(%p) GENAVB_MSG_MEDIA_STACK_BIND supported only in Milan mode\n", avdecc);
			goto exit;
		}

		os_log(LOG_INFO, "avdecc(%p): received GENAVB_MSG_MEDIA_STACK_BIND: Controller (%016"PRIx64") bound listener stream (%016"PRIx64", %u, %s) to talker stream (%016"PRIx64", %u) \n",
				avdecc, desc->u.media_stack_bind.controller_entity_id,
				desc->u.media_stack_bind.entity_id, desc->u.media_stack_bind.listener_stream_index, (desc->u.media_stack_bind.started == ACMP_LISTENER_STREAM_STARTED) ? "STARTED" : "STOPPED",
				desc->u.media_stack_bind.talker_entity_id, desc->u.media_stack_bind.talker_stream_index);

		if ((entity = avdecc_get_entity(avdecc, htonll(desc->u.media_stack_bind.entity_id))) != NULL)
			acmp_milan_listener_sink_rcv_binding_params(entity, &desc->u.media_stack_bind);

		break;

	default:
		os_log(LOG_ERR, "Received unknown message type %d on a MEDIA_STACK channel\n", desc->type);
		break;
	}

exit:
	ipc_free(rx, desc);
}

void avdecc_ipc_rx_srp(struct ipc_rx const *rx, struct ipc_desc *desc)
{
	struct avdecc_port *port = container_of(rx, struct avdecc_port, ipc_rx_srp);
	struct avdecc_ctx *avdecc = avdecc_port_to_context(port);
	struct ipc_msrp_listener_status *listener_status;
	struct ipc_msrp_talker_status *talker_status;
	struct ipc_msrp_listener_declaration_status *listener_declaration_status;
	struct ipc_msrp_talker_declaration_status *talker_declaration_status;
	u64 stream_id;
	struct entity *entity;

	os_log(LOG_DEBUG, "avdecc(%p) port(%u) received IPC type: %d\n", avdecc, port->port_id, desc->type);

	switch (desc->type) {
	case GENAVB_MSG_LISTENER_STATUS:

		listener_status = &desc->u.msrp_listener_status;
		stream_id = get_64(listener_status->stream_id);

		os_log(LOG_INFO, "avdecc(%p) port(%u) Received listener status %s for stream_id(%016"PRIx64")\n",
				  avdecc, port->port_id, listener_stream_status2string(listener_status->status), ntohll(stream_id));

		if (avdecc->milan_mode) {
			u16 listener_unique_id;

			entity = avdecc_get_local_listener(avdecc, port->port_id);
			if (!entity) {
				os_log(LOG_DEBUG, "avdecc(%p) port(%u) Couldn't find any local listener entity supporting this port.\n",
					avdecc, port->port_id);
				goto exit;
			}

			if (!acmp_milan_get_listener_unique_id(entity, stream_id, &listener_unique_id))
				acmp_milan_listener_srp_state_sm(entity, listener_unique_id, listener_status);
		}

		break;

	case GENAVB_MSG_TALKER_STATUS:
		talker_status = &desc->u.msrp_talker_status;
		stream_id = get_64(talker_status->stream_id);

		os_log(LOG_INFO, "avdecc(%p) port(%u) Received talker status %s for stream_id(%016"PRIx64")\n",
				  avdecc, port->port_id, talker_stream_status2string(talker_status->status), ntohll(stream_id));

		if (avdecc->milan_mode) {
			u16 talker_unique_id;

			entity = avdecc_get_local_talker(avdecc, port->port_id);
			if (!entity) {
				os_log(LOG_DEBUG, "avdecc(%p) port(%u) Couldn't find any local talker entity supporting this port.\n",
					avdecc, port->port_id);
				goto exit;
			}

			if (!acmp_milan_get_talker_unique_id(entity, stream_id, &talker_unique_id))
				acmp_milan_talker_update_status(entity, talker_unique_id, talker_status);
		}
		break;

	case GENAVB_MSG_TALKER_DECLARATION_STATUS:
		talker_declaration_status = &desc->u.msrp_talker_declaration_status;
		stream_id = get_64(talker_declaration_status->stream_id);

		os_log(LOG_INFO, "avdecc(%p) port(%u) talker declaration %s for stream_id(%016"PRIx64")\n",
				  avdecc, port->port_id, talker_stream_declaration_type2string(talker_declaration_status->declaration_type), ntohll(stream_id));

		if (avdecc->milan_mode) {
			u16 talker_unique_id;

			entity = avdecc_get_local_talker(avdecc, port->port_id);
			if (!entity) {
				os_log(LOG_DEBUG, "avdecc(%p) port(%u) Couldn't find any local talker entity supporting this port.\n",
					avdecc, port->port_id);
				goto exit;
			}

			if (!acmp_milan_get_talker_unique_id(entity, stream_id, &talker_unique_id))
				acmp_milan_talker_update_declaration(entity, talker_unique_id, talker_declaration_status);
		}
		break;

	case GENAVB_MSG_LISTENER_DECLARATION_STATUS:
		listener_declaration_status = &desc->u.msrp_listener_declaration_status;
		stream_id = get_64(listener_declaration_status->stream_id);

		os_log(LOG_INFO, "avdecc(%p) port(%u) listener declaration %s for stream_id(%016"PRIx64")\n",
				  avdecc, port->port_id, listener_stream_declaration_type2string(listener_declaration_status->declaration_type), ntohll(stream_id));

		break;

	case GENAVB_MSG_TALKER_RESPONSE:
	case GENAVB_MSG_LISTENER_RESPONSE:
		break;

	default:
		os_log(LOG_ERR, "avdecc(%p) port(%u) received unknown message type %d on MSRP channel\n", avdecc, port->port_id, desc->type);
		break;
	}

exit:
	ipc_free(rx, desc);
}

static void avdecc_ipc_get_mac_status(struct avdecc_ctx *avdecc, struct ipc_tx *ipc, unsigned int port_id)
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
			os_log(LOG_ERR, "ipc_tx() failed (%d)\n", rc);
			ipc_free(ipc, desc);
		}
	} else
		os_log(LOG_ERR, "ipc_alloc() failed\n");
}

static void avdecc_ipc_rx_mac_service(struct ipc_rx const *rx, struct ipc_desc *desc)
{
	struct avdecc_port *port = container_of(rx, struct avdecc_port, ipc_rx_mac_service);
	struct avdecc_ctx *avdecc = avdecc_port_to_context(port);
	struct ipc_mac_service_status *status;
	struct avb_interface_dynamic_desc *avb_itf_dynamic;
	int i;

	switch (desc->type) {
	case IPC_MAC_SERVICE_STATUS:
		status = &desc->u.mac_service_status;

		os_log(LOG_DEBUG, "IPC_MAC_SERVICE_STATUS: logical_port(%u) operational(%u): received on avdecc port (%u)\n",
			status->port_id, status->operational, port->port_id);

		for (i = 0; i < avdecc->num_entities; i++) {
			struct entity *entity = avdecc->entities[i];

			avb_itf_dynamic = aem_get_descriptor(entity->aem_dynamic_descs, AEM_DESC_TYPE_AVB_INTERFACE, port->port_id, NULL);
			if (!avb_itf_dynamic) /* Interface index not supported in this entity */
				continue;

			if (!avb_itf_dynamic->operational_state && status->operational) {
				avb_itf_dynamic->link_up++;

				if (entity_ready(entity)) {
					if (avdecc->milan_mode) {
						adp_milan_advertise_sm(entity, port->port_id, ADP_MILAN_ADV_LINK_UP);

						aecp_milan_register_get_counters_async_notification(entity, port->port_id);
					} else
						adp_ieee_advertise_interface_sm(entity, port->port_id, ADP_INTERFACE_ADV_EVENT_LINK_UP);
				}

			} else if (avb_itf_dynamic->operational_state && !status->operational) {
				avb_itf_dynamic->link_down++;

				if (entity_ready(entity)) {
					if (avdecc->milan_mode) {
						adp_milan_advertise_sm(entity, port->port_id, ADP_MILAN_ADV_LINK_DOWN);

						aecp_milan_register_get_counters_async_notification(entity, port->port_id);
					} else
						adp_ieee_advertise_interface_sm(entity, port->port_id, ADP_INTERFACE_ADV_EVENT_LINK_DOWN);
				}
			}

			/* Save the current state */
			avb_itf_dynamic->operational_state = (status->operational) ? true : false;
		}

		break;

	default:
		break;
	}

	ipc_free(rx, desc);
}

/** Sends an AVDECC packet back to the stack.
 * Takes ownership of the TX descriptor in all cases.
 * \param	avdecc		pointer to the AVDECC context
 * \param	rx_desc		RX descriptor containing the packet to send
 * \param	local_mac	Pointer to local MAC address to set as source
 */
static void avdecc_net_tx_loopback(struct avdecc_port *port, struct net_rx_desc *rx_desc, u8 *local_mac)
{
	struct eth_hdr *eth = (struct eth_hdr *)NET_DATA_START(rx_desc);

	/* Make sure any response will come back locally as well. */
	os_memcpy(eth->src, local_mac, 6);

	rx_desc->l3_offset = rx_desc->l2_offset + sizeof(struct eth_hdr);

	avdecc_net_rx(&port->net_rx, rx_desc);
}

/** Sends an AVDECC packet on the network.
 * Takes ownership of the TX descriptor in all cases.
 * \return	0 on success, -1 in case of failure
 * \param	port		pointer to the AVDECC port on which the packet will be sent
 * \param	desc		TX descriptor containing the packet to send
 */
int avdecc_net_tx(struct avdecc_port *port, struct net_tx_desc *desc)
{
	struct net_tx_desc *tx_desc;
	struct eth_hdr *eth = (struct eth_hdr *)NET_DATA_START(desc);
	int rc = 0;

	os_log(LOG_DEBUG, "port (%u), l2_offset %x, len %d, flags %d\n",
		port->port_id, desc->l2_offset, desc->len, desc->flags);

	if (MAC_IS_MCAST(eth->dst)) {
		tx_desc = net_tx_clone(&port->net_tx, desc);
		if (!tx_desc)
			os_log(LOG_ERR, "port(%u) cannot alloc tx descriptor\n", port->port_id);
		else {
			if (net_tx(&port->net_tx, tx_desc) < 0) {
				os_log(LOG_ERR, "port(%u) cannot transmit packet\n", port->port_id);
				net_tx_free(tx_desc);
			}
		}

		avdecc_net_tx_loopback(port, (struct net_rx_desc *)desc, port->local_physical_mac);

	} else {
		if (os_memcmp(eth->dst, port->local_physical_mac, 6) == 0) {
			avdecc_net_tx_loopback(port, (struct net_rx_desc *)desc, port->local_physical_mac);
		} else {
			if (net_tx(&port->net_tx, desc) < 0) {
				os_log(LOG_ERR, "port(%u) cannot transmit packet\n", port->port_id);
				net_tx_free(desc);
				rc = -1;
			}
		}
	}

	if (!rc)
		os_log(LOG_DEBUG, "Success \n");

	return rc;
}

size_t avdecc_add_common_header(void *buf, u8 subtype, u8 msg_type, u16 length, u8 status)
{
	struct avtp_ctrl_hdr *avtp_ctrl = (struct avtp_ctrl_hdr *)buf;

	avtp_ctrl->subtype = subtype;
	avtp_ctrl->version = 0;
	avtp_ctrl->sv = 0;
	avtp_ctrl->control_data = msg_type;
	AVTP_SET_CTRL_DATA_STATUS(avtp_ctrl, status, length);

	return sizeof(struct avtp_ctrl_hdr);
}

/**
 * Get the avdecc port for a specific logical port number
 * \return			0 on success and the interface index set in the interface_index pointer, -1 otherwise.
 * \return			The avdecc port with the corresponding logical port id on success, NULL otherwise
 * \param avdecc		pointer to avdecc context
 * \param logical_port		logical port number
 */
struct avdecc_port *logical_to_avdecc_port(struct avdecc_ctx *avdecc, unsigned int logical_port)
{
	int i;

	for (i = 0; i < avdecc->port_max; i++) {
		if (logical_port == avdecc->port[i].logical_port) {
			if (avdecc->port[i].initialized)
				return &avdecc->port[i];
			else
				return NULL;
		}
	}

	return NULL;
}

/**
 * Get the logical port for a specific avdecc port (avb interface index)
 * \return			logical port id
 * \param avdecc		pointer to avdecc context
 * \param port_id		avdecc port id/interface index
 */
unsigned int avdecc_port_to_logical(struct avdecc_ctx *avdecc, unsigned int port_id)
{
	return avdecc->port[port_id].logical_port;
}

/** Find the local entity matching the provided entity ID and return it if it is ready.
 *  \return	pointer to the entity (if found and ready), NULL otherwise.
 */
struct entity *avdecc_get_entity(struct avdecc_ctx *avdecc, u64 entity_id)
{
	int i;

	for (i = 0; i < avdecc->num_entities; i++)
		if ((avdecc->entities[i]->desc->entity_id == entity_id) && entity_ready(avdecc->entities[i]))
			return avdecc->entities[i];

	return NULL;
}

/** Checks if the provided interface index is valid for the AVDECC entity.
 * \return		True if the interface index is enabled, False otherwise
 * \param entity	Pointer to entity
 * \param port_id	avdecc port id/interface index
 */
bool avdecc_entity_port_valid(struct entity *entity, unsigned int port_id)
{
	if (aem_get_descriptor(entity->aem_descs, AEM_DESC_TYPE_AVB_INTERFACE, port_id, NULL))
		return true;
	else
		return false;
}

__init static int avdecc_port_ipc_init(struct avdecc_port *port, unsigned long priv)
{
	struct avdecc_ctx *avdecc = avdecc_port_to_context(port);

	if (avdecc->use_gptp_bridge_stack) {
		/* No gptp endpoint stack is available: use the bridge stack for GM status events. */
		if (ipc_rx_init(&port->ipc_rx_gptp, IPC_GPTP_BRIDGE_MEDIA_STACK,
				avdecc_ipc_rx_gptp, priv) < 0)
			goto err_ipc_rx_gptp;

		if (ipc_tx_init(&port->ipc_tx_gptp, IPC_MEDIA_STACK_GPTP_BRIDGE) < 0)
			goto err_ipc_tx_gptp;
	} else {
		if (ipc_rx_init(&port->ipc_rx_gptp, (port->port_id == CFG_ENDPOINT_0_LOGICAL_PORT ) ? IPC_GPTP_MEDIA_STACK : IPC_GPTP_1_MEDIA_STACK,
				avdecc_ipc_rx_gptp, priv) < 0)
			goto err_ipc_rx_gptp;

		if (ipc_tx_init(&port->ipc_tx_gptp, (port->port_id == CFG_ENDPOINT_0_LOGICAL_PORT ) ? IPC_MEDIA_STACK_GPTP : IPC_MEDIA_STACK_GPTP_1) < 0)
			goto err_ipc_tx_gptp;
	}

	if (ipc_tx_connect(&port->ipc_tx_gptp, &port->ipc_rx_gptp) < 0)
		goto err_ipc_tx_gptp_connect;

	if (avdecc->srp_enabled) {
		if (ipc_tx_init(&port->ipc_tx_srp, (port->port_id == CFG_ENDPOINT_0_LOGICAL_PORT ) ? IPC_MEDIA_STACK_MSRP : IPC_MEDIA_STACK_MSRP_1) < 0)
			goto err_ipc_tx_srp;

		if (ipc_rx_init(&port->ipc_rx_srp, (port->port_id == CFG_ENDPOINT_0_LOGICAL_PORT ) ? IPC_MSRP_MEDIA_STACK : IPC_MSRP_1_MEDIA_STACK, avdecc_ipc_rx_srp, priv) < 0)
			goto err_ipc_rx_srp;

		if (ipc_tx_connect(&port->ipc_tx_srp, &port->ipc_rx_srp) < 0)
			goto err_ipc_tx_srp_connect;
	}

	if (avdecc->management_enabled) {
		if (ipc_rx_init(&port->ipc_rx_mac_service, (port->port_id == CFG_ENDPOINT_0_LOGICAL_PORT ) ? IPC_MAC_SERVICE_MEDIA_STACK : IPC_MAC_SERVICE_1_MEDIA_STACK,
					avdecc_ipc_rx_mac_service, priv) < 0)
			goto err_ipc_rx_mac_service;

		if (ipc_tx_init(&port->ipc_tx_mac_service, (port->port_id == CFG_ENDPOINT_0_LOGICAL_PORT ) ? IPC_MEDIA_STACK_MAC_SERVICE : IPC_MEDIA_STACK_MAC_SERVICE_1) < 0)
			goto err_ipc_tx_mac_service;

		if (ipc_tx_connect(&port->ipc_tx_mac_service, &port->ipc_rx_mac_service) < 0)
			goto err_ipc_tx_mac_connect;

		avdecc_ipc_get_mac_status(avdecc, &port->ipc_tx_mac_service, port->port_id);
	}

	return 0;

err_ipc_tx_mac_connect:
	if (avdecc->management_enabled)
		ipc_tx_exit(&port->ipc_tx_mac_service);
err_ipc_tx_mac_service:
	if (avdecc->management_enabled)
		ipc_rx_exit(&port->ipc_rx_mac_service);
err_ipc_rx_mac_service:
err_ipc_tx_srp_connect:
	if (avdecc->srp_enabled)
		ipc_rx_exit(&port->ipc_rx_srp);

err_ipc_rx_srp:
	if (avdecc->srp_enabled)
		ipc_tx_exit(&port->ipc_tx_srp);

err_ipc_tx_srp:
err_ipc_tx_gptp_connect:
	ipc_tx_exit(&port->ipc_tx_gptp);

err_ipc_tx_gptp:
	ipc_rx_exit(&port->ipc_rx_gptp);

err_ipc_rx_gptp:
	return -1;
}


__init static int avdecc_ports_init(struct avdecc_ctx *avdecc, struct avdecc_config *cfg, unsigned long priv)
{
	unsigned int i;
	struct net_address addr;
	unsigned int logical_port;

	for (i = 0; i < avdecc->port_max; i++) {
		logical_port = cfg->logical_port_list[i];

		avdecc->port[i].port_id = i;
		avdecc->port[i].logical_port = logical_port;
		avdecc->port[i].initialized = false;

		if (net_get_local_addr(logical_port, avdecc->port[i].local_physical_mac) < 0) {
			os_log(LOG_ERR, "avdecc(%p) could not get local physical mac address on port(%u)\n", avdecc, i);
			goto err_local_addr;
		}

		/**
		 * Network Rx/Tx
		 */
		addr.ptype = PTYPE_AVTP;
		addr.port = logical_port;
		addr.priority = AVDECC_DEFAULT_PRIORITY;
		addr.u.avtp.subtype = AVTP_SUBTYPE_AVDECC;

		if (net_rx_init(&avdecc->port[i].net_rx, &addr, avdecc_net_rx, priv) < 0)
			goto err_net_rx_init;

		if (net_tx_init(&avdecc->port[i].net_tx, &addr) < 0)
			goto err_net_tx_init;

		if (avdecc_port_ipc_init(&avdecc->port[i], priv) < 0)
			goto err_ipc_init;

		avdecc->port[i].initialized = true;

		os_log(LOG_INIT, "avdecc(%p) - port(%u)(%p) / max %d done\n",
			avdecc, avdecc->port[i].port_id, &avdecc->port[i], avdecc->port_max);

		continue;

	err_ipc_init:
		net_tx_exit(&avdecc->port[i].net_tx);
	err_net_tx_init:
		net_rx_exit(&avdecc->port[i].net_rx);

	err_net_rx_init:
	err_local_addr:
		continue;
	}

	return 0;
}

__init static struct avdecc_ctx *avdecc_alloc(unsigned int timer_n, unsigned int n_ports, unsigned int max_entities_discovery)
{
	struct avdecc_ctx *avdecc;
	unsigned int size;

	size = sizeof(struct avdecc_ctx) + n_ports * sizeof(struct avdecc_port);
	size += timer_pool_size(timer_n);
	size += adp_discovery_data_size(max_entities_discovery) * n_ports;

	avdecc = os_malloc(size);
	if (!avdecc)
		goto err;

	os_memset(avdecc, 0, size);

	avdecc->timer_ctx = (struct timer_ctx *)((u8 *)(avdecc + 1) + n_ports * sizeof(struct avdecc_port));
	avdecc->adp_discovery_data = (void *)((u8 *)avdecc->timer_ctx + timer_pool_size(timer_n));
	avdecc->port_max = n_ports;

	return avdecc;

err:
	return NULL;
}

__exit static void avdecc_port_ipc_exit(struct avdecc_port *port)
{
	struct avdecc_ctx *avdecc = avdecc_port_to_context(port);

	if (avdecc->management_enabled) {
		ipc_tx_exit(&port->ipc_tx_mac_service);
		ipc_rx_exit(&port->ipc_rx_mac_service);
	}

	if (avdecc->srp_enabled) {
		ipc_rx_exit(&port->ipc_rx_srp);
		ipc_tx_exit(&port->ipc_tx_srp);
	}

	ipc_tx_exit(&port->ipc_tx_gptp);

	ipc_rx_exit(&port->ipc_rx_gptp);
}

__exit static void avdecc_ports_exit(struct avdecc_ctx *avdecc)
{
	unsigned int i;

	for (i = 0; i < avdecc->port_max; i++) {
		if (!avdecc->port[i].initialized)
			continue;

		net_tx_exit(&avdecc->port[i].net_tx);
		net_rx_exit(&avdecc->port[i].net_rx);

		avdecc_port_ipc_exit(&avdecc->port[i]);

		avdecc->port[i].initialized = false;
	}

	os_log(LOG_INIT, "done\n");
}

__init void *avdecc_init(struct avdecc_config *cfg, unsigned long priv)
{
	struct avdecc_ctx *avdecc;
	unsigned int timer_n;
	u8 *adp_discovery_data;
	int i = 0, j, k = 0;

	if (cfg->num_entities == 0)
		cfg->num_entities = 1;

	timer_n = cfg->port_max * cfg->num_entities * CFG_AVDECC_MAX_TIMERS_PER_ENTITY;

	avdecc = avdecc_alloc(timer_n, cfg->port_max, cfg->max_entities_discovery);
	if (!avdecc)
		goto err_malloc;

	log_level_set(avdecc_COMPONENT_ID, cfg->log_level);

	avdecc->srp_enabled = cfg->srp_enabled;
	avdecc->management_enabled = cfg->management_enabled;
	avdecc->milan_mode = cfg->milan_mode;
	avdecc->use_gptp_bridge_stack = cfg->use_gptp_bridge_stack;

	if (avdecc_ports_init(avdecc, cfg, priv) < 0)
		goto err_ports;

	/**
	 * IPC
	 */
	if (ipc_rx_init(&avdecc->ipc_rx_controller, IPC_CONTROLLER_AVDECC, avdecc_ipc_rx_controller, priv) < 0)
		goto err_ipc_rx_controller;

	if (ipc_rx_init(&avdecc->ipc_rx_controlled, IPC_CONTROLLED_AVDECC, avdecc_ipc_rx_controlled, priv) < 0)
			goto err_ipc_rx_controlled;

	if (ipc_rx_init(&avdecc->ipc_rx_media_stack, IPC_MEDIA_STACK_AVDECC, avdecc_ipc_rx_media_stack, priv) < 0)
			goto err_ipc_rx_media_stack;

	if (ipc_tx_init(&avdecc->ipc_tx_media_stack, IPC_AVDECC_MEDIA_STACK) < 0)
		goto err_ipc_tx_avtp;

	if (ipc_tx_init(&avdecc->ipc_tx_maap, IPC_MEDIA_STACK_MAAP) < 0)
		goto err_ipc_tx_maap;

	if (ipc_rx_init(&avdecc->ipc_rx_maap, IPC_MAAP_MEDIA_STACK, avdecc_ipc_rx_maap, priv) < 0)
		goto err_ipc_rx_maap;

	if (ipc_tx_connect(&avdecc->ipc_tx_maap, &avdecc->ipc_rx_maap) < 0)
		goto err_ipc_tx_maap_connect;

	if (ipc_tx_init(&avdecc->ipc_tx_controlled, IPC_AVDECC_CONTROLLED) < 0)
		goto err_ipc_tx_controlled;

	if (ipc_tx_init(&avdecc->ipc_tx_controller, IPC_AVDECC_CONTROLLER) < 0)
		goto err_ipc_tx_controller;

	if (ipc_tx_init(&avdecc->ipc_tx_controller_sync, IPC_AVDECC_CONTROLLER_SYNC) < 0)
		goto err_ipc_tx_controller_sync;


	/**
	 * Timers
	 */
	if (timer_pool_init(avdecc->timer_ctx, timer_n, priv) < 0)
		goto err_timer_pool_init;

	avdecc->num_entities = cfg->num_entities;

	for (i = 0; i < avdecc->num_entities; i++) {
		avdecc->entities[i] = avdecc_entity_init(avdecc, i, &cfg->entity_cfg[i]);
		if (!avdecc->entities[i])
			goto err_entity_init;
	}

	for (k = 0; k < avdecc->port_max; k++) {
		adp_discovery_data = (u8 *)avdecc->adp_discovery_data + k * adp_discovery_data_size(cfg->max_entities_discovery);

		if (adp_discovery_init(&avdecc->port[k].discovery, adp_discovery_data, cfg) < 0)
			goto err_discovery_init;
	}

	for (j = 0; j < avdecc->port_max; j++) {
		/* No gptp endpoint stack is available:
		 * Requests for the DefaultDS are not meaningful to gptp bridge stack.
		 */
		if (!avdecc->use_gptp_bridge_stack)
			avdecc_ipc_managed_get_default_ds(&avdecc->port[j]);

		avdecc_ipc_gm_get_status(&avdecc->port[j]);
	}

	if (avdecc->srp_enabled) {
		for (j = 0; j < avdecc->port_max; j++)
			avdecc_ipc_srp_deregister_all(&avdecc->port[j]);
	}

	os_log(LOG_INIT, "avdecc(%p) done, loaded %d entities.\n", avdecc, avdecc->num_entities);

	return avdecc;

err_discovery_init:
	for (j = 0; j < k; j++)
		adp_discovery_exit(&avdecc->port[j].discovery);

err_entity_init:
	for (j = 0; j < i; j++)
		avdecc_entity_exit(avdecc->entities[j]);

	timer_pool_exit(avdecc->timer_ctx);

err_timer_pool_init:
	ipc_tx_exit(&avdecc->ipc_tx_controller_sync);

err_ipc_tx_controller_sync:
	ipc_tx_exit(&avdecc->ipc_tx_controller);

err_ipc_tx_controller:
	ipc_tx_exit(&avdecc->ipc_tx_controlled);

err_ipc_tx_controlled:
err_ipc_tx_maap_connect:
	ipc_rx_exit(&avdecc->ipc_rx_maap);
err_ipc_rx_maap:
	ipc_tx_exit(&avdecc->ipc_tx_maap);

err_ipc_tx_maap:
	ipc_tx_exit(&avdecc->ipc_tx_media_stack);

err_ipc_tx_avtp:
	ipc_rx_exit(&avdecc->ipc_rx_media_stack);

err_ipc_rx_media_stack:
	ipc_rx_exit(&avdecc->ipc_rx_controlled);

err_ipc_rx_controlled:
	ipc_rx_exit(&avdecc->ipc_rx_controller);

err_ipc_rx_controller:
	avdecc_ports_exit(avdecc);

err_ports:
	os_free(avdecc);

err_malloc:
	return NULL;
}

__exit int avdecc_exit(void *avdecc_h)
{
	struct avdecc_ctx *avdecc = (struct avdecc_ctx *)avdecc_h;
	int i;

	/* De-init entities in reverse order while decrementing the num_entities
	 * counter to protect against access to the freed entity (on avdecc_net_tx_loopback())
	 */
	while (avdecc->num_entities-- > 0) {
		avdecc_entity_exit(avdecc->entities[avdecc->num_entities]);
	}

	/* avdecc_entity_exit() can generate PDUs (in loopback), so de-init the
	 * discovery context after that.
	 */
	for (i = 0; i < avdecc->port_max; i++) {
		if (!avdecc->port[i].initialized)
			continue;

		adp_discovery_exit(&avdecc->port[i].discovery);
	}

	timer_pool_exit(avdecc->timer_ctx);

	ipc_tx_exit(&avdecc->ipc_tx_controller_sync);

	ipc_tx_exit(&avdecc->ipc_tx_controller);

	ipc_tx_exit(&avdecc->ipc_tx_controlled);

	ipc_rx_exit(&avdecc->ipc_rx_maap);

	ipc_tx_exit(&avdecc->ipc_tx_maap);

	ipc_tx_exit(&avdecc->ipc_tx_media_stack);

	ipc_rx_exit(&avdecc->ipc_rx_media_stack);

	ipc_rx_exit(&avdecc->ipc_rx_controlled);

	ipc_rx_exit(&avdecc->ipc_rx_controller);

	avdecc_ports_exit(avdecc);

	os_log(LOG_INIT, "avdecc(%p)\n", avdecc);

	os_free(avdecc_h);

	return 0;
}
