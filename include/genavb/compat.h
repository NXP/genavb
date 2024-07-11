/*
 * Copyright 2018, 2021, 2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file compat.h
 \brief GenAVB public API
 \details API definition for the GenAVB library

 \copyright  Copyright 2018, 2023 NXP
*/

/*
 * Error types
 */
#define	AVB_SUCCESS			GENAVB_SUCCESS
#define	AVB_ERR_NO_MEMORY		GENAVB_ERR_NO_MEMORY
#define	AVB_ERR_ALREADY_INITIALIZED	GENAVB_ERR_ALREADY_INITIALIZED
#define	AVB_ERR_INVALID_PARAMS		GENAVB_ERR_INVALID_PARAMS
#define	AVB_ERR_INVALID			GENAVB_ERR_INVALID
#define	AVB_ERR_STREAM_API_OPEN		GENAVB_ERR_STREAM_API_OPEN
#define	AVB_ERR_STREAM_BIND		GENAVB_ERR_STREAM_BIND
#define	AVB_ERR_STREAM_TX		GENAVB_ERR_STREAM_TX
#define	AVB_ERR_STREAM_RX		GENAVB_ERR_STREAM_RX
#define	AVB_ERR_STREAM_INVALID		GENAVB_ERR_STREAM_INVALID
#define	AVB_ERR_STREAM_PARAMS		GENAVB_ERR_STREAM_PARAMS
#define	AVB_ERR_STREAM_TX_NOT_ENOUGH_DATA	GENAVB_ERR_STREAM_TX_NOT_ENOUGH_DATA
#define	AVB_ERR_CTRL_TRUNCATED		GENAVB_ERR_CTRL_TRUNCATED
#define	AVB_ERR_CTRL_INIT		GENAVB_ERR_CTRL_INIT
#define	AVB_ERR_CTRL_ALLOC		GENAVB_ERR_CTRL_ALLOC
#define	AVB_ERR_CTRL_TX			GENAVB_ERR_CTRL_TX
#define	AVB_ERR_CTRL_RX			GENAVB_ERR_CTRL_RX
#define	AVB_ERR_CTRL_LEN		GENAVB_ERR_CTRL_LEN
#define	AVB_ERR_CTRL_TIMEOUT		GENAVB_ERR_CTRL_TIMEOUT
#define	AVB_ERR_CTRL_INVALID		GENAVB_ERR_CTRL_INVALID
#define	AVB_ERR_CTRL_FAILED		GENAVB_ERR_CTRL_FAILED
#define	AVB_ERR_CTRL_UNKNOWN		GENAVB_ERR_CTRL_UNKNOWN
#define	AVB_ERR_STACK_NOT_READY		GENAVB_ERR_STACK_NOT_READY

/*
 * Control message types
 */
#define	AVB_MSG_MEDIA_STACK_CONNECT		GENAVB_MSG_MEDIA_STACK_CONNECT
#define	AVB_MSG_MEDIA_STACK_DISCONNECT		GENAVB_MSG_MEDIA_STACK_DISCONNECT
#define	AVB_MSG_AECP				GENAVB_MSG_AECP
#define	AVB_MSG_ACMP_COMMAND			GENAVB_MSG_ACMP_COMMAND
#define	AVB_MSG_ACMP_RESPONSE			GENAVB_MSG_ACMP_RESPONSE
#define	AVB_MSG_ADP				GENAVB_MSG_ADP
#define	AVB_MSG_LISTENER_REGISTER		GENAVB_MSG_LISTENER_REGISTER
#define	AVB_MSG_LISTENER_DEREGISTER		GENAVB_MSG_LISTENER_DEREGISTER
#define	AVB_MSG_LISTENER_RESPONSE		GENAVB_MSG_LISTENER_RESPONSE
#define	AVB_MSG_LISTENER_STATUS			GENAVB_MSG_LISTENER_STATUS
#define	AVB_MSG_LISTENER_DECLARATION_STATUS	GENAVB_MSG_LISTENER_DECLARATION_STATUS
#define	AVB_MSG_TALKER_REGISTER			GENAVB_MSG_TALKER_REGISTER
#define	AVB_MSG_TALKER_DEREGISTER		GENAVB_MSG_TALKER_DEREGISTER
#define	AVB_MSG_TALKER_RESPONSE			GENAVB_MSG_TALKER_RESPONSE
#define	AVB_MSG_TALKER_STATUS			GENAVB_MSG_TALKER_STATUS
#define	AVB_MSG_TALKER_DECLARATION_STATUS	GENAVB_MSG_TALKER_DECLARATION_STATUS
#define	AVB_MSG_VLAN_REGISTER			GENAVB_MSG_VLAN_REGISTER
#define	AVB_MSG_VLAN_DEREGISTER			GENAVB_MSG_VLAN_DEREGISTER
#define	AVB_MSG_VLAN_RESPONSE			GENAVB_MSG_VLAN_RESPONSE
#define	AVB_MSG_ERROR_RESPONSE			GENAVB_MSG_ERROR_RESPONSE
#define	AVB_MSG_CLOCK_DOMAIN_SET_SOURCE		GENAVB_MSG_CLOCK_DOMAIN_SET_SOURCE
#define	AVB_MSG_CLOCK_DOMAIN_RESPONSE		GENAVB_MSG_CLOCK_DOMAIN_RESPONSE
#define	AVB_MSG_CLOCK_DOMAIN_GET_STATUS		GENAVB_MSG_CLOCK_DOMAIN_GET_STATUS
#define	AVB_MSG_CLOCK_DOMAIN_STATUS		GENAVB_MSG_CLOCK_DOMAIN_STATUS
#define	AVB_MSG_TYPE_MAX			GENAVB_MSG_TYPE_MAX

/*
 * Control channel types
 */
#define	AVB_CTRL_AVDECC_MEDIA_STACK	GENAVB_CTRL_AVDECC_MEDIA_STACK
#define	AVB_CTRL_AVDECC_CONTROLLER	GENAVB_CTRL_AVDECC_CONTROLLER
#define	AVB_CTRL_AVDECC_CONTROLLED	GENAVB_CTRL_AVDECC_CONTROLLED
#define	AVB_CTRL_MSRP			GENAVB_CTRL_MSRP
#define	AVB_CTRL_MVRP			GENAVB_CTRL_MVRP
#define	AVB_CTRL_CLOCK_DOMAIN		GENAVB_CTRL_CLOCK_DOMAIN
#define	AVB_CTRL_ID_MAX			GENAVB_CTRL_ID_MAX

/*
 * Function headers
 */
#define avb_init			genavb_init
#define avb_exit			genavb_exit
#define avb_control_open		genavb_control_open
#define avb_control_send		genavb_control_send
#define avb_control_send_sync		genavb_control_send_sync
#define avb_control_receive		genavb_control_receive
#define avb_control_close		genavb_control_close
#define avb_control_rx_fd		genavb_control_rx_fd
#define avb_control_tx_fd		genavb_control_tx_fd
#define avb_control_set_callback	genavb_control_set_callback
#define avb_strerror			genavb_strerror
#define avb_stream_create		genavb_stream_create
#define avb_stream_destroy		genavb_stream_destroy
#define avb_stream_receive		genavb_stream_receive
#define avb_stream_send			genavb_stream_send
#define avb_stream_presentation_offset	genavb_stream_presentation_offset
#define avb_stream_fd			genavb_stream_fd
#define avb_stream_h264_send		genavb_stream_h264_send
#define avb_stream_receive_iov		genavb_stream_receive_iov
#define avb_stream_send_iov		genavb_stream_send_iov
#define avb_stream_set_callback		genavb_stream_set_callback

/*
 * Common definitions
 */
#define avb_handle			genavb_handle
#define avb_control_handle		genavb_control_handle
#define avb_stream_handle		genavb_stream_handle
#define avb_msg_type_t			genavb_msg_type_t
#define avb_control_id_t		genavb_control_id_t
#define avb_msg_error_response		genavb_msg_error_response
#define avb_iovec			genavb_iovec
#define avb_event			genavb_event

/*
 * SRP
 */
#define avb_msrp_stream_params		genavb_msrp_stream_params
#define avb_msg_listener_register	genavb_msg_listener_register
#define avb_msg_listener_deregister	genavb_msg_listener_deregister
#define avb_listener_stream_status_t	genavb_listener_stream_status_t
#define avb_msg_listener_status		genavb_msg_listener_status
#define avb_msg_listener_response	genavb_msg_listener_response
#define avb_msg_talker_register		genavb_msg_talker_register
#define avb_msg_talker_deregister	genavb_msg_talker_deregister
#define avb_talker_stream_status_t	genavb_talker_stream_status_t
#define avb_msg_talker_status		genavb_msg_talker_status
#define avb_msg_talker_response		genavb_msg_talker_response
#define avb_msg_msrp			genavb_msg_msrp
#define avb_msg_vlan_register		genavb_msg_vlan_register
#define avb_msg_vlan_deregister		genavb_msg_vlan_deregister
#define avb_msg_vlan_response		genavb_msg_vlan_response

/*
 * Streaming
 */
#define avb_stream_flags_t		genavb_stream_flags_t
#define avb_stream_params		genavb_stream_params
#define avb_stream_disconnect		genavb_stream_disconnect
#define avb_media_stack_msg		genavb_media_stack_msg
#define AVB_STREAM_FLAGS_MCR		GENAVB_STREAM_FLAGS_MCR
#define AVB_STREAM_FLAGS_CUSTOM_TSPEC	GENAVB_STREAM_FLAGS_CUSTOM_TSPEC

/*
 * Clock domain
 */
#define avb_clock_domain_t		genavb_clock_domain_t
#define avb_clock_source_type_t		genavb_clock_source_type_t
#define avb_clock_source_local_id_t	genavb_clock_source_local_id_t
#define avb_msg_clock_domain_response	genavb_msg_clock_domain_response
#define avb_msg_clock_domain_status	genavb_msg_clock_domain_status
#define avb_msg_clock_domain_get_status	genavb_msg_clock_domain_get_status
#define avb_msg_clock_domain_set_source	genavb_msg_clock_domain_set_source
#define avb_msg_clock_domain		genavb_msg_clock_domain
#define	AVB_CLOCK_DOMAIN_DEFAULT		GENAVB_CLOCK_DOMAIN_DEFAULT
#define	AVB_MEDIA_CLOCK_DOMAIN_PTP		GENAVB_MEDIA_CLOCK_DOMAIN_PTP
#define	AVB_MEDIA_CLOCK_DOMAIN_STREAM		GENAVB_MEDIA_CLOCK_DOMAIN_STREAM
#define	AVB_MEDIA_CLOCK_DOMAIN_MASTER_CLK	GENAVB_MEDIA_CLOCK_DOMAIN_MASTER_CLK
#define	AVB_CLOCK_DOMAIN_0 			GENAVB_CLOCK_DOMAIN_0
#define	AVB_CLOCK_DOMAIN_1			GENAVB_CLOCK_DOMAIN_1
#define	AVB_CLOCK_DOMAIN_2			GENAVB_CLOCK_DOMAIN_2
#define	AVB_CLOCK_DOMAIN_3			GENAVB_CLOCK_DOMAIN_3
#define	AVB_CLOCK_DOMAIN_MAX			GENAVB_CLOCK_DOMAIN_MAX
#define	AVB_CLOCK_SOURCE_TYPE_INTERNAL		GENAVB_CLOCK_SOURCE_TYPE_INTERNAL
#define	AVB_CLOCK_SOURCE_TYPE_INPUT_STREAM	GENAVB_CLOCK_SOURCE_TYPE_INPUT_STREAM
#define	AVB_CLOCK_SOURCE_AUDIO_CLK		GENAVB_CLOCK_SOURCE_AUDIO_CLK
#define	AVB_CLOCK_SOURCE_PTP_CLK		GENAVB_CLOCK_SOURCE_PTP_CLK

/*
 * AVDECC
 */
#define avb_aecp_msg			genavb_aecp_msg
#define avb_adp_msg			genavb_adp_msg
#define avb_acmp_command		genavb_acmp_command
#define avb_acmp_response		genavb_acmp_response
#define avb_controller_msg		genavb_controller_msg
#define avb_controlled_msg		genavb_controlled_msg
