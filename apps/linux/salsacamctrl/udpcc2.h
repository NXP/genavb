/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2017, 2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

typedef enum {
	UDPCC2_MSG_TYPE_FORBIDDEN = 0,
	UDPCC2_MSG_TYPE_SetMessage = 1,
	UDPCC2_MSG_TYPE_GetMessage = 2,
	UDPCC2_MSG_TYPE_RespMessage = 3,
	UDPCC2_MSG_TYPE_NotifyMessage = 4
} udpcc2_message_type_t;

typedef enum {
	UDPCC2_CODE_UNSET = 0,
	UDPCC2_CODE_ACK = 1,
	UDPCC2_CODE_E_FAILED = 2,
	UDPCC2_CODE_E_BUSY = 3,
	UDPCC2_CODE_E_UNSUPPORTED = 4,
	UDPCC2_CODE_E_WRONG_MSG = 5,
	UDPCC2_CODE_E_WRONG_DATA = 6,
	UDPCC2_CODE_E_NO_MEMORY = 7
} udpcc2_code_t;

typedef enum {
	UDPCC2_CHANGE_FORBIDDEN = 0,
	UDPCC2_CHANGE_IMMEDIATE = 1,
	UDPCC2_CHANGE_PERSISTENT_IMMEDIATE = 2,
	UDPCC2_CHANGE_PERSISTENT = 3
} udpcc2_change_type_t;

typedef enum {
	UDPCC2_OPT_UNUSED 		= 0x0,
	UDPCC2_OPT_REBOOT 		= 0x1,
	UDPCC2_OPT_MAC_ADDR 		= 0x2,
	UDPCC2_OPT_IP_ADDR 		= 0x3,
	UDPCC2_OPT_NETMASK 		= 0x4,
	UDPCC2_OPT_GATEWAY 		= 0x5,
	UDPCC2_OPT_PORT 		= 0x6,
	UDPCC2_OPT_UDP_STREAM_PORT 	= 0x7,
	UDPCC2_OPT_UDP_STREAM_IP_ADDR 	= 0x8,
	UDPCC2_OPT_AVB_STREAM_MAC_ADDR 	= 0x9,
	UDPCC2_OPT_AVB_STREAM_ID 	= 0xa,
	UDPCC2_OPT_STREAM_TYPE		= 0x10,
	UDPCC2_OPT_DATA_RATE		= 0x12,
	UDPCC2_OPT_CAMERA_STOP		= 0x14,
	UDPCC2_OPT_CAMERA_START		= 0x15,
	UDPCC2_OPT_AUTOSTART		= 0x19,
	UDPCC2_OPT_AVB_VLAN		= 0x1a,
	UDPCC2_OPT_PHY			= 0x1b,
} udpcc2_configuration_option_t;

typedef enum {
	UDPCC2_STREAM_ERROR	= -1,
	UDPCC2_STREAM_UDP	= 0,
	UDPCC2_STREAM_AVB	= 1,
} udpcc2_stream_type_t;

typedef enum {
	UDPCC2_PHY_ERROR	= -1,
	UDPCC2_PHY_100BASET	= 0,
	UDPCC2_PHY_BRR 		= 1,
} udpcc2_phy_type_t;

/** UDPCC2 message description
 */
struct udpcc2_message {
	unsigned char type;			/**< Message type */
	unsigned char code; 			/**< Response code */
	unsigned short configuration_option; 	/**< Configuration option to be read/written or command to be executed */
	unsigned char change_type;		/**< Change type */
	unsigned char request_number;		/**< Sequence number to match commands and reponses */
	unsigned short payload_length;		/**< Length of the following payload, in bytes */
	unsigned char payload[];		/**< Message payload */
};

#define UDPCC2_DEFAULT_PORT 	25002
#define UDPCC2_DEFAULT_IPADDR	"192.168.1.2"

/** Send an UDPCC2 message to a Salsa camera
 * Send an UDPCC2 message, and optionnally receive a response.
 * \return		1 on success or negative value on error.
 * \param	ipaddr	Pointer to a character string containing the IPv4 address of the camera in dotted-decimal format, "ddd.ddd.ddd.ddd".
 * \param	port	UDP port of the control interface of the camera.
 * \param	cmd	UDPCC2 message to send.
 * \param	resp	If non-NULL, will contain the response message on return.
 * \param	resplen	Length of the response buffer, including the checksum (which is computed by udpcc2_send_msg).
 */
int udpcc2_send_msg(const char *ipaddr, unsigned int port, struct udpcc2_message *cmd, struct udpcc2_message *resp, unsigned int resplen);

int udpcc2_camera_reboot(const char *ipaddr, unsigned int port);
int udpcc2_camera_start(const char *ipaddr, unsigned int port);
int udpcc2_camera_stop(const char *ipaddr, unsigned int port);


int udpcc2_camera_get_state(const char *ipaddr, unsigned int port);

/** Set camera data rate
 * Configure the target bandwidth for the stream. Will become effective after the next camera reboot.
 * \return		1 on success or negative value on error.
 * \param	ipaddr	Pointer to a character string containing the IPv4 address of the camera in dotted-decimal format, "ddd.ddd.ddd.ddd".
 * \param	port	UDP port of the control interface of the camera.
 * \param	rate	Data rate to configure, in kbps and host byte order.
 */
int udpcc2_camera_set_rate(const char *ipaddr, unsigned int port, unsigned int rate);

/** Get camera data rate
 * Retrieve the current target bandwidth for the stream.
 * \return		Current data rate in kbps or negative value on error.
 * \param	ipaddr	Pointer to a character string containing the IPv4 address of the camera in dotted-decimal format, "ddd.ddd.ddd.ddd".
 * \param	port	UDP port of the control interface of the camera.
 */
int udpcc2_camera_get_rate(const char *ipaddr, unsigned int port);

/** Set camera AVB stream ID
 * Configure the stream ID for the AVB stream. Will become effective after the next camera reboot.
 * \return			1 on success or negative value on error.
 * \param	ipaddr		Pointer to a character string containing the IPv4 address of the camera in dotted-decimal format, "ddd.ddd.ddd.ddd".
 * \param	port		UDP port of the control interface of the camera.
 * \param	stream_id	stream_id to configure (host byte order).
 */
int udpcc2_camera_set_avb_stream_id(const char *ipaddr, unsigned int port, unsigned long long stream_id);

/** Get camera AVB stream ID
 * Retrieve the current stream ID for the AVB stream.
 * \return		Current AVB stream ID or 0 on error.
 * \param	ipaddr	Pointer to a character string containing the IPv4 address of the camera in dotted-decimal format, "ddd.ddd.ddd.ddd".
 * \param	port	UDP port of the control interface of the camera.
 */
unsigned long long udpcc2_camera_get_avb_stream_id(const char *ipaddr, unsigned int port);

/** Set camera control IP address
 * Configure the IPv4 address used to control the camera. Will become effective after the next camera reboot.
 * \return			1 on success or negative value on error.
 * \param	ipaddr		Pointer to a character string containing the IPv4 address of the camera in dotted-decimal format, "ddd.ddd.ddd.ddd".
 * \param	port		UDP port of the control interface of the camera.
 * \param	control_ipaddr	IPv4 address to configure (network byte order).
 */
int udpcc2_camera_set_control_ipaddr(const char *ipaddr, unsigned int port, unsigned int control_ipaddr);


/** Set camera MAC address
 * Configure the MAC address of the camera. Will become effective after the next camera reboot.
 * \return			1 on success or negative value on error.
 * \param	ipaddr		Pointer to a character string containing the IPv4 address of the camera in dotted-decimal format, "ddd.ddd.ddd.ddd".
 * \param	port		UDP port of the control interface of the camera.
 * \param	mac_addr	MAC address to configure (network byte order).
 */
int udpcc2_camera_set_mac_addr(const char *ipaddr, unsigned int port, unsigned long long mac_addr);

/** Get camera MAC address
 * Retrieve the current MAC address of the camera.
 * \return		Current MAC address or 0 on error.
 * \param	ipaddr	Pointer to a character string containing the IPv4 address of the camera in dotted-decimal format, "ddd.ddd.ddd.ddd".
 * \param	port	UDP port of the control interface of the camera.
 */
unsigned long long udpcc2_camera_get_mac_addr(const char *ipaddr, unsigned int port);


/** Set camera stream type (UDP or AVB)
 * Configure the stream type of the camera. Will become effective after the next camera reboot.
 * \return			1 on success or negative value on error.
 * \param	ipaddr		Pointer to a character string containing the IPv4 address of the camera in dotted-decimal format, "ddd.ddd.ddd.ddd".
 * \param	port		UDP port of the control interface of the camera.
 * \param	avb		Set to 1 to enable AVB streaming, 0 for UDP.
 */
int udpcc2_camera_set_stream_type(const char *ipaddr, unsigned int port, udpcc2_stream_type_t avb);

/** Get camera stream type
 * Retrieve the current stream type of the camera.
 * \return		1 if the camera is setup for AVB streaming, 0 for UDP, or negative value on error.
 * \param	ipaddr	Pointer to a character string containing the IPv4 address of the camera in dotted-decimal format, "ddd.ddd.ddd.ddd".
 * \param	port	UDP port of the control interface of the camera.
 */
udpcc2_stream_type_t udpcc2_camera_get_stream_type(const char *ipaddr, unsigned int port);


/** Set camera auto-start
 * Configure stream auto-start. Will become effective after the next camera reboot.
 * \return			1 on success or negative value on error.
 * \param	ipaddr		Pointer to a character string containing the IPv4 address of the camera in dotted-decimal format, "ddd.ddd.ddd.ddd".
 * \param	port		UDP port of the control interface of the camera.
 * \param	autostart	Set to 1 to enable stream auto-start.
 */
int udpcc2_camera_set_autostart(const char *ipaddr, unsigned int port, unsigned int autostart);

/** Get camera auto-start
 * Retrieve the current auto-start setting of the camera.
 * \return		1 if the camera is setup for auto-start, 0 if not, or negative value on error.
 * \param	ipaddr	Pointer to a character string containing the IPv4 address of the camera in dotted-decimal format, "ddd.ddd.ddd.ddd".
 * \param	port	UDP port of the control interface of the camera.
 */
 int udpcc2_camera_get_autostart(const char *ipaddr, unsigned int port);


 /** Configure camera AVB VLAN tagging
 * Configure VLAN id to use with AVB streams and enable/disable VLAN tagging. Will become effective after the next camera reboot.
 * \return			1 on success or negative value on error.
 * \param	ipaddr		Pointer to a character string containing the IPv4 address of the camera in dotted-decimal format, "ddd.ddd.ddd.ddd".
 * \param	port		UDP port of the control interface of the camera.
 * \param	enable		Set to 1 to add a VLAN tag on AVB streams.
 * \param	vlan_id		VLAN id to use on AVB streams (when VLAN tagging is enabled).
 */
int udpcc2_camera_set_avb_vlan(const char *ipaddr, unsigned int port, unsigned int enable, unsigned short vlan_id);

/** Get camera AVB VLAN configuration
 * Retrieve the current AVB VLAN setting of the camera.
 * \return		1 if VLAN tagging is enabled, 0 if disabled, or negative value on error.
 * \param	ipaddr	Pointer to a character string containing the IPv4 address of the camera in dotted-decimal format, "ddd.ddd.ddd.ddd".
 * \param	port	UDP port of the control interface of the camera.
 * \param	vlan_id	On successful return, will contain the vlan ID currently configured on the camera.
 */
 int udpcc2_camera_get_avb_vlan(const char *ipaddr, unsigned int port, unsigned short *vlan_id);


 /** Set camera preferred PHY
 * Configure default PHY to use. Will become effective after the next camera reboot.
 * \return			1 on success or negative value on error.
 * \param	ipaddr		Pointer to a character string containing the IPv4 address of the camera in dotted-decimal format, "ddd.ddd.ddd.ddd".
 * \param	port		UDP port of the control interface of the camera.
 * \param	phy		Set to 0 for 100BaseT Ethernet, 1 for BroadRReach.
 */
int udpcc2_camera_set_phy(const char *ipaddr, unsigned int port, udpcc2_phy_type_t phy);

/** Get camera preferred PHY
 * Retrieve the default PHY of the camera.
 * \return		1 if the camera is setup for BroadRReach, 0 for 100BaseT, or negative value on error.
 * \param	ipaddr	Pointer to a character string containing the IPv4 address of the camera in dotted-decimal format, "ddd.ddd.ddd.ddd".
 * \param	port	UDP port of the control interface of the camera.
 */
  udpcc2_phy_type_t udpcc2_camera_get_phy(const char *ipaddr, unsigned int port);
