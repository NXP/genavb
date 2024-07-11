/*
* Copyright 2018-2024 NXP
*
* SPDX-License-Identifier: BSD-3-Clause
*/

/**
 \file control.c
 \brief GenAVB public API for linux
 \details API definition for the GenAVB library
 \copyright Copyright 2018-2024 NXP
*/

#include "os/string.h"
#include "os/stdlib.h"
#include "common/ipc.h"

#include "control.h"

const ipc_id_t ipc_id[GENAVB_CTRL_ID_MAX][3] =
{
	[GENAVB_CTRL_AVDECC_MEDIA_STACK] = {
		[CTRL_TX] = IPC_MEDIA_STACK_AVDECC,
		[CTRL_RX] = IPC_AVDECC_MEDIA_STACK,
		[CTRL_RX_SYNC] = IPC_ID_NONE,
	},

	[GENAVB_CTRL_AVDECC_CONTROLLER] = {
		[CTRL_TX] = IPC_CONTROLLER_AVDECC,
		[CTRL_RX] = IPC_AVDECC_CONTROLLER,
		[CTRL_RX_SYNC] = IPC_AVDECC_CONTROLLER_SYNC
	},

	[GENAVB_CTRL_AVDECC_CONTROLLED] = {
		[CTRL_TX] = IPC_CONTROLLED_AVDECC,
		[CTRL_RX] = IPC_AVDECC_CONTROLLED,
		[CTRL_RX_SYNC] = IPC_ID_NONE,
	},

	[GENAVB_CTRL_MSRP] = {
		[CTRL_TX] = IPC_MEDIA_STACK_MSRP,
		[CTRL_RX] = IPC_MSRP_MEDIA_STACK,
		[CTRL_RX_SYNC] = IPC_MSRP_MEDIA_STACK_SYNC,
	},

	[GENAVB_CTRL_MSRP_1] = {
		[CTRL_TX] = IPC_MEDIA_STACK_MSRP_1,
		[CTRL_RX] = IPC_MSRP_1_MEDIA_STACK,
		[CTRL_RX_SYNC] = IPC_MSRP_1_MEDIA_STACK_SYNC,
	},

	[GENAVB_CTRL_MSRP_BRIDGE] = {
		[CTRL_TX] = IPC_MEDIA_STACK_MSRP_BRIDGE,
		[CTRL_RX] = IPC_MSRP_BRIDGE_MEDIA_STACK,
		[CTRL_RX_SYNC] = IPC_MSRP_BRIDGE_MEDIA_STACK_SYNC,
	},

	[GENAVB_CTRL_MVRP] = {
		[CTRL_TX] = IPC_MEDIA_STACK_MVRP,
		[CTRL_RX] = IPC_MVRP_MEDIA_STACK,
		[CTRL_RX_SYNC] = IPC_MVRP_MEDIA_STACK_SYNC,
	},

	[GENAVB_CTRL_MVRP_1] = {
		[CTRL_TX] = IPC_MEDIA_STACK_MVRP_1,
		[CTRL_RX] = IPC_MVRP_1_MEDIA_STACK,
		[CTRL_RX_SYNC] = IPC_MVRP_1_MEDIA_STACK_SYNC,
	},

	[GENAVB_CTRL_MVRP_BRIDGE] = {
		[CTRL_TX] = IPC_MEDIA_STACK_MVRP_BRIDGE,
		[CTRL_RX] = IPC_MVRP_BRIDGE_MEDIA_STACK,
		[CTRL_RX_SYNC] = IPC_MVRP_BRIDGE_MEDIA_STACK_SYNC,
	},

	[GENAVB_CTRL_CLOCK_DOMAIN] = {
		[CTRL_TX] = IPC_MEDIA_STACK_CLOCK_DOMAIN,
		[CTRL_RX] = IPC_CLOCK_DOMAIN_MEDIA_STACK,
		[CTRL_RX_SYNC] = IPC_CLOCK_DOMAIN_MEDIA_STACK_SYNC,
	},

	[GENAVB_CTRL_GPTP] = {
		[CTRL_TX] = IPC_MEDIA_STACK_GPTP,
		[CTRL_RX] = IPC_GPTP_MEDIA_STACK,
		[CTRL_RX_SYNC] = IPC_GPTP_MEDIA_STACK_SYNC,
	},

	[GENAVB_CTRL_GPTP_1] = {
		[CTRL_TX] = IPC_MEDIA_STACK_GPTP_1,
		[CTRL_RX] = IPC_GPTP_1_MEDIA_STACK,
		[CTRL_RX_SYNC] = IPC_GPTP_1_MEDIA_STACK_SYNC,
	},

	[GENAVB_CTRL_GPTP_BRIDGE] = {
		[CTRL_TX] = IPC_MEDIA_STACK_GPTP_BRIDGE,
		[CTRL_RX] = IPC_GPTP_BRIDGE_MEDIA_STACK,
		[CTRL_RX_SYNC] = IPC_GPTP_BRIDGE_MEDIA_STACK_SYNC,
	},

	[GENAVB_CTRL_MAAP] = {
		[CTRL_TX] = IPC_MEDIA_STACK_MAAP,
		[CTRL_RX] = IPC_MAAP_MEDIA_STACK,
		[CTRL_RX_SYNC] = IPC_MAAP_MEDIA_STACK_SYNC,
	}
};

#define MAAP_MAX_CTRL_ID 256		/* Control application can allocate range with ID between 0 and 255 */

#define HEARTBEAT_TIMEOUT 3000
int send_heartbeat(struct ipc_tx *tx, struct ipc_rx *rx, unsigned int flags)
{
	int rc;
	struct ipc_heartbeat hb;
	unsigned int msg_type, msg_len;

	hb.status = 0;

	/*
	* Send Heartbeat message
	*/
	rc = avb_ipc_send(tx, IPC_HEARTBEAT, &hb, sizeof(hb), flags);
	if (rc != GENAVB_SUCCESS) {
		//printf("Couldn't send HEARTBEAT message rc(%d)\n", rc);
		goto exit;
	}

	if (rx) {
		msg_len = sizeof(struct ipc_heartbeat);
		rc = avb_ipc_receive_sync(rx, &msg_type, &hb, &msg_len, HEARTBEAT_TIMEOUT);
		if ((rc != GENAVB_SUCCESS) || (msg_type != IPC_HEARTBEAT)) {
			//printf("Couldn't receive HEARTBEAT reply rc(%d) type(%d)\n", rc, msg_type);
			goto exit;
		}
	}

exit:
	return rc;
}

int avb_ipc_send(const struct ipc_tx *tx, genavb_msg_type_t msg_type, void const *msg, unsigned int msg_len, unsigned int flags)
{
	struct ipc_desc *desc;
	int rc;

	if (msg_len > sizeof(desc->u))
		return -GENAVB_ERR_CTRL_LEN;

	desc = ipc_alloc(tx, DEFAULT_IPC_DATA_SIZE);
	if (desc) {
		desc->type = msg_type;
		desc->len = msg_len;
		desc->flags = flags;

		os_memcpy(&desc->u, msg, msg_len);

		if (ipc_tx(tx, desc) < 0) {
			rc = -GENAVB_ERR_CTRL_TX;
			goto err_ipc_tx;
		}
	} else {
		rc = -GENAVB_ERR_CTRL_ALLOC;
		goto err_ipc_alloc;
	}

	return GENAVB_SUCCESS;

err_ipc_tx:
	ipc_free(tx, desc);

err_ipc_alloc:
	return rc;
}

int avb_ipc_receive(struct ipc_rx const *rx, unsigned int *msg_type, void *msg, unsigned int *msg_len)
{
	struct ipc_desc *desc;
	int rc = GENAVB_SUCCESS;

	if (!msg) {
		rc = -GENAVB_ERR_INVALID_PARAMS;
		goto err_rx_ioctl;
	}

	desc = __ipc_rx(rx);
	if (!desc) {
		rc = (-GENAVB_ERR_CTRL_RX);
		goto err_rx_ioctl;
	}

	*msg_type = desc->type;

	if (*msg_len < desc->len) {
		rc = (-GENAVB_ERR_CTRL_TRUNCATED);
	} else
		*msg_len = desc->len;

	os_memcpy(msg, (void*)&desc->u, *msg_len);

	if (*msg_type == GENAVB_MSG_ERROR_RESPONSE) {
		rc = -desc->u.error.status;
	}

	ipc_free(rx, desc);

err_rx_ioctl:
	return rc;
}

int genavb_control_close(struct genavb_control_handle *handle)
{
	if (ipc_id[handle->id][CTRL_RX_SYNC] != IPC_ID_NONE)
		ipc_rx_exit(&handle->rx_sync);

	ipc_rx_exit(&handle->rx);

	ipc_tx_exit(&handle->tx);

	os_free(handle);

	return GENAVB_SUCCESS;
}

int genavb_control_receive(struct genavb_control_handle const *handle, genavb_msg_type_t *msg_type, void *msg, unsigned int *msg_len)
{
	unsigned int __msg_type;
	int rc;

	rc = avb_ipc_receive(&handle->rx, &__msg_type, msg, msg_len);
	if (rc == GENAVB_SUCCESS)
		*msg_type = __msg_type;

	return rc;
}

static int avb_control_valid(struct genavb_control_handle const *handle, genavb_msg_type_t msg_type, void const *msg, unsigned int msg_len)
{
	int rc = 0;
	avb_u8 inner_type;


	switch (handle->id) {
	case GENAVB_CTRL_AVDECC_MEDIA_STACK:
		switch (msg_type) {
		case GENAVB_MSG_MEDIA_STACK_BIND:
			if (msg_len == sizeof(struct genavb_msg_media_stack_bind))
				rc = 1;
			break;

		default:
			break;
		}
		break;
	case GENAVB_CTRL_AVDECC_CONTROLLED:
		switch (msg_type) {
		case GENAVB_MSG_AECP:
			if (msg_len >= (offsetof(struct genavb_aecp_msg, buf) + sizeof(struct aecp_aem_pdu))) {
				struct aecp_aem_pdu *pdu = (struct aecp_aem_pdu *)((struct genavb_aecp_msg *)msg)->buf;
				avb_u16 response_type = AECP_AEM_GET_CMD_TYPE(pdu);
				inner_type = ((struct genavb_aecp_msg *)msg)->msg_type;
				if (inner_type == AECP_AEM_RESPONSE) {
					switch (response_type) {
					case AECP_AEM_CMD_SET_CONTROL:
					case AECP_AEM_CMD_START_STREAMING:
					case AECP_AEM_CMD_STOP_STREAMING:
					case AECP_AEM_CMD_REMOVE_AUDIO_MAPPINGS:
					case AECP_AEM_CMD_ADD_AUDIO_MAPPINGS:
					case AECP_AEM_CMD_GET_AUDIO_MAP:
						rc = 1;
						break;
					default:
						break;
					}
				}
			}
			break;
		default:
			break;
		}
		break;
	case GENAVB_CTRL_AVDECC_CONTROLLER:
		switch (msg_type) {
		case GENAVB_MSG_AECP:
			if (msg_len >= (offsetof(struct genavb_aecp_msg, buf) + sizeof(struct aecp_aem_pdu))) {
				inner_type = ((struct genavb_aecp_msg *)msg)->msg_type;
				if (inner_type == AECP_AEM_COMMAND)
					rc = 1;
			}
			break;
		case GENAVB_MSG_ACMP_COMMAND:
			if (msg_len >= sizeof(struct genavb_acmp_command)) {
				inner_type = ((struct genavb_acmp_command *)msg)->message_type;
				switch (inner_type) {
				case ACMP_CONNECT_RX_COMMAND:
				case ACMP_DISCONNECT_RX_COMMAND:
				case ACMP_GET_TX_STATE_COMMAND:
				case ACMP_GET_RX_STATE_COMMAND:
				case ACMP_GET_TX_CONNECTION_COMMAND:
					rc = 1;
					break;
				default:
					break;
				}
			}
			break;
		case GENAVB_MSG_ADP:
			if (msg_len >= sizeof(struct genavb_adp_msg)) {
				inner_type = ((struct genavb_adp_msg *)msg)->msg_type;
				switch (inner_type) {
				case ADP_ENTITY_DISCOVER:
				case ADP_ENTITY_AVAILABLE:
					rc = 1;
					break;
				default:
					break;
				}
			}
			break;

		default:
			break;
		}

		break;

	case GENAVB_CTRL_MSRP:
	case GENAVB_CTRL_MSRP_BRIDGE:
		switch (msg_type) {
		case GENAVB_MSG_TALKER_REGISTER:
			if (msg_len == sizeof(struct genavb_msg_talker_register))
				rc = 1;
			break;

		case GENAVB_MSG_TALKER_DEREGISTER:
			if (msg_len == sizeof(struct genavb_msg_talker_deregister))
				rc = 1;

			break;

		case GENAVB_MSG_LISTENER_REGISTER:
			if (msg_len == sizeof(struct genavb_msg_listener_register))
				rc = 1;

			break;

		case GENAVB_MSG_LISTENER_DEREGISTER:
			if (msg_len == sizeof(struct genavb_msg_listener_deregister))
				rc = 1;

			break;

		case GENAVB_MSG_MANAGED_SET:
		case GENAVB_MSG_MANAGED_GET:
			rc = 1;
			break;

		default:
			break;
		}
		break;

	case GENAVB_CTRL_MVRP:
	case GENAVB_CTRL_MVRP_BRIDGE:
		switch (msg_type) {
		case GENAVB_MSG_VLAN_REGISTER:
			if (msg_len == sizeof(struct genavb_msg_vlan_register))
				rc = 1;

			break;

		case GENAVB_MSG_VLAN_DEREGISTER:
			if (msg_len == sizeof(struct genavb_msg_vlan_deregister))
				rc = 1;

			break;

		default:
			break;
		}

		break;

	case GENAVB_CTRL_CLOCK_DOMAIN:
		switch (msg_type) {
		case GENAVB_MSG_CLOCK_DOMAIN_SET_SOURCE:
			if (msg_len == sizeof(struct genavb_msg_clock_domain_set_source))
				rc = 1;

			break;

		case GENAVB_MSG_CLOCK_DOMAIN_GET_STATUS:
			if (msg_len == sizeof(struct genavb_msg_clock_domain_get_status))
				rc = 1;

			break;

		default:
			break;
		}

		break;

	case GENAVB_CTRL_GPTP:
	case GENAVB_CTRL_GPTP_BRIDGE:
		switch (msg_type) {
		case GENAVB_MSG_MANAGED_SET:
		case GENAVB_MSG_MANAGED_GET:
			rc = 1;
			break;
		default:
			break;
		}

		break;

	case GENAVB_CTRL_MAAP:
		switch (msg_type) {
		case GENAVB_MSG_MAAP_CREATE_RANGE:
			if (msg_len == sizeof(struct genavb_msg_maap_create)) {
				avb_u32 range_id = ((struct genavb_msg_maap_create *)msg)->range_id;

				if (range_id < MAAP_MAX_CTRL_ID)
					rc = 1;
			}

			break;

		case GENAVB_MSG_MAAP_DELETE_RANGE:
			if (msg_len == sizeof(struct genavb_msg_maap_delete)) {
				avb_u32 range_id = ((struct genavb_msg_maap_delete *)msg)->range_id;

				if (range_id < MAAP_MAX_CTRL_ID)
					rc = 1;
			}

			break;

		default:
			break;
		}

	default:
		break;
	}

	return rc;
}

int _avb_control_send(struct genavb_control_handle const *handle, genavb_msg_type_t msg_type, void const *msg, unsigned int msg_len, unsigned int flags)
{
	if (!avb_control_valid(handle, msg_type, msg, msg_len))
		return -GENAVB_ERR_CTRL_INVALID;

	return avb_ipc_send(&handle->tx, msg_type, msg, msg_len, flags);
}

int genavb_control_send(struct genavb_control_handle const *handle, genavb_msg_type_t msg_type, void const *msg, unsigned int msg_len)
{
	return _avb_control_send(handle, msg_type, msg, msg_len, 0);
}

int genavb_control_send_sync(struct genavb_control_handle const *handle, genavb_msg_type_t *msg_type, void const *msg, unsigned int msg_len, void *rsp, unsigned int *rsp_len, int timeout)
{
	unsigned int __msg_type;
	int rc;

	if (ipc_id[handle->id][CTRL_RX_SYNC] == IPC_ID_NONE) {
		rc = -GENAVB_ERR_CTRL_INVALID;
		goto exit;
	}

	rc = _avb_control_send(handle, *msg_type, msg, msg_len, IPC_FLAGS_AVB_MSG_SYNC);
	if  (rc != GENAVB_SUCCESS)
		goto exit;

	rc = avb_ipc_receive_sync(&handle->rx_sync, &__msg_type, rsp, rsp_len, timeout);
	if (rc == GENAVB_SUCCESS)
		*msg_type = __msg_type;

exit:
	return rc;
}
