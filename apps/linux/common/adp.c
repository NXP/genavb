/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2020-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/poll.h>

#include <genavb/genavb.h>
#include "adp.h"


/** Initiate a dump of the ADP database of entities (i.e. entities currently seen on the network).
 * Once the function has been called, entities may be retrieved by receiving ADP messages from the AVB stack (one message per entity).
 *
 * \return AVB return code (AVB_SUCCESS or negative error code).
 * \param ctrl_h	AVB control handle to use to send the request to the GenAVB stack (must be for a AVB_CTRL_AVDECC_CONTROLLER channel).
 */
int adp_start_dump_entities(struct avb_control_handle *ctrl_h)
{
	struct avb_adp_msg msg;
	int rc;

	msg.msg_type = ADP_ENTITY_DISCOVER;
	rc = avb_control_send(ctrl_h, AVB_MSG_ADP, &msg, sizeof(struct avb_adp_msg));

	return rc;
}


int adp_new_entity(struct entity_info *entities, unsigned int n_entities, struct avb_adp_msg *msg)
{
	unsigned int i;

	for (i = 0; i < n_entities; i++) {
		if ((memcmp(&entities[i].entity_id, &msg->info.entity_id, 8) == 0) &&
			(memcmp(&entities[i].local_mac_addr, &msg->info.local_mac_addr, 6) == 0)) /* we can receive the same entity on two different interfaces */
			return 0;
	}

	return 1;
}

/** Dump the full ADP database of entities.
 *
 * \return	Total number of entities returned on success, or negative AVB error code.
 * \param ctrl_h	AVB control handle to use to send the request to the GenAVB stack (must be for a AVB_CTRL_AVDECC_CONTROLLER channel).
 * \param entities	on return, *entities will point to a newly-allocated array of entity_info structures describing the various entities.
 */
int adp_dump_entities(struct avb_control_handle *ctrl_h, struct entity_info **entities)
{
	union avb_controller_msg msg;
	avb_msg_type_t msg_type;
	int rc = -AVB_ERR_CTRL_RX;
	int i, n_entities;
	int ctrl_rx_fd;
	struct pollfd ctrl_poll;

	if (!ctrl_h || !entities)
		return -AVB_ERR_INVALID_PARAMS;

	rc = adp_start_dump_entities(ctrl_h);
	if (rc != AVB_SUCCESS) {
		printf("%s: ERROR: Got error message %d(%s) while trying to start dump of ADP entities.\n", __func__, rc, avb_strerror(rc));
		rc = -AVB_ERR_CTRL_RX;
		goto exit;
	}

	*entities = NULL;
	n_entities = 0;
	i = -1;

	ctrl_rx_fd = avb_control_rx_fd(ctrl_h);
	ctrl_poll.fd = ctrl_rx_fd;
	ctrl_poll.events = POLLIN;
	ctrl_poll.revents = 0;

	while (i < n_entities) {
		if (poll(&ctrl_poll, 1, -1) == -1) {
			printf("%s: ERROR: poll(%d) failed on waiting for connect\n", __func__, ctrl_poll.fd);
			rc = -AVB_ERR_CTRL_RX;
			goto exit;
		}

		if (ctrl_poll.revents & POLLIN) {
			unsigned int msg_len = sizeof(union avb_controller_msg);

			rc = avb_control_receive(ctrl_h, &msg_type, &msg, &msg_len);
			if (rc != AVB_SUCCESS) {
				printf("%s: ERROR: Got error message %d(%s) while trying to receive ADP response.\n", __func__, rc, avb_strerror(rc));
				rc = -AVB_ERR_CTRL_RX;
				goto exit;
			}

			switch (msg_type) {
			case AVB_MSG_ADP:
				/* The code below is a bit more complex than would seem necessary, because we need to handle a possible
				 * race condition: The API used to fetch entity discovery information closely follows the ADP protocol,
				 * which means a new entity notification (AVAILABLE or DEPARTING) could be sent by the stack after the
				 * application asked for an ADP database dump but before the stack received the dump request.
				 *
				 * Note 1: because of the way the ADP code is architected in the stack, an ADP dump will be made of a
				 * sequence of contiguous ADP messages, without any external notifications interrupting it.
				 *
				 * Note 2: we do not need to handle notifications arriving after the dump: we can listen to
				 * notifications later on if we need to, but the dump itself will accurately represent the ADP database
				 * at a specific time.
				 *
				 * As a consequence, we only need to handle ADP notifications that may be received before the dump
				 * actually starts (in addition to the messages for the dump itself).
				 */
				switch (msg.adp.msg_type) {
				case ADP_ENTITY_AVAILABLE:
					/* If we never allocated the entities array (or if we freed it in a previous
					 * ENTITY_DEPARTING message), n_entities will be 0, msg.adp.total >= 1,
					 * and *entities NULL. In that case, realloc will behave like malloc.
					 *
					 * If we already allocated the array, the current message has to be an external
					 * notification (all messages from the dump will have the same total of entities,
					 * and it cannot be more than the last notification received, if any),
					 * so it is safe to reallocate and start over from the beginning of the array.
					 */
					if (n_entities != msg.adp.total) {
						n_entities = msg.adp.total;
						*entities = realloc(*entities, n_entities * sizeof(struct entity_info));
						i = 0;
					}

					if (!*entities) {
						rc = -AVB_ERR_NO_MEMORY;
						goto exit;
					}

					/* We may have received an ENTITY_AVAILABLE notification before the dump started.
					 * In that case, the total number of detected entities retrieved in the notification
					 * will already be accurate (as well as the details of the new entity), but the dump
					 * will show that entity information a 2nd time, so we just skip duplicates.
					 */
					if (adp_new_entity(*entities, i, &msg.adp)) {
						memcpy(&(*entities)[i], &msg.adp.info, sizeof(struct entity_info));
						i++;
					}

					break;

				/* Dump messages can only be ENTITY_AVAILABLE or ENTITY_NOTFOUND, so this as to be an external
				 * notification. We do not bother trying to sync the existing array in that case and we just
				 * start over, since the dump messages are still to be read and will give us the info we need.
				 */
				case ADP_ENTITY_DEPARTING:
					if (*entities) {
						free(*entities);
						*entities = NULL;
						n_entities = 0;
						i = -1;
					}
					break;

				case ADP_ENTITY_NOTFOUND:
					rc = 0;
					goto exit;

				default:
					printf("%s: ERROR: Received a message of type %d while handling dump of ADP entities.\n", __func__, msg_type);
					rc = -AVB_ERR_CTRL_RX;
					goto exit;
				}
				break;

			default:
				printf("%s: WARNING: Ignoring AVDECC message type %d received while handling dump of ADP entities.\n", __func__, msg_type);
				break;
			}
		}
	}

	if (i == n_entities)
		rc = i;
exit:
	if ((rc <= 0) && *entities)
		free(*entities);
	return rc;
}
