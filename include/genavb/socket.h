/*
 * Copyright 2018, 2020, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief GenAVB/TSN public API
 \details Socket API definition for the GenAVB/TSN library

 \copyright Copyright 2018, 2020, 2023-2024 NXP
*/

#ifndef _GENAVB_PUBLIC_SOCKET_API_H_
#define _GENAVB_PUBLIC_SOCKET_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "clock.h"
#include "types.h"
#include "net_types.h"

struct genavb_socket_rx;
struct genavb_socket_tx;

/**
 * \defgroup socket		Socket
 * \ingroup library
 *
 * \defgroup socket_tx	Socket Transmit
 * \ingroup socket
 *
 * \defgroup socket_rx	Socket Receive
 * \ingroup socket
 *
 */

/**
 * \ingroup socket
 * Socket rx/tx parameters
 */
typedef enum {
	GENAVB_SOCKF_NONBLOCK = 0x01, /**< Non-blocking mode (only applies to receive socket) */
	GENAVB_SOCKF_ZEROCOPY = 0x02, /**< Zero-copy mode (not implemented) */
	GENAVB_SOCKF_RAW = 0x04	      /**< Raw socket (only applies to transmit socket) */
} genavb_sock_f_t;

/**
 * \ingroup socket_tx
 * genavb_socket_tx_send flags
 */
typedef enum {
	GENAVB_SOCKET_TX_TS = (1 << 0),		/**< Request transmit timestamp */
	GENAVB_SOCKET_TX_TIME = (1 << 1)	/**< Specify transmit time */
} genavb_socket_tx_send_flags_t;


/**
 * \ingroup socket_rx
 * Socket rx parameters
 */
struct genavb_socket_rx_params {
	struct net_address addr; /**< Socket address */
};

/**
 * \ingroup socket_tx
 * Socket tx parameters
 */
struct genavb_socket_tx_params {
	struct net_address addr; /**< Socket address */
};

/**
 * \ingroup socket_tx
 * Socket tx flags
 */
struct genavb_socket_tx_send_params {
	genavb_socket_tx_send_flags_t flags;
	unsigned long priv;
	uint64_t ts;				/**< Transmit time, in nanoseconds */
};

/** Open rx socket
 * \ingroup socket_rx
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param sock		Socket handle
 * \param flags		Socket receive flags
 * \param params	Socket receive parameters
 */
int genavb_socket_rx_open(struct genavb_socket_rx **sock, genavb_sock_f_t flags,
			  struct genavb_socket_rx_params *params);

/** Open tx socket
 * \ingroup socket_tx
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param sock		Socket handle
 * \param flags		Socket transmit flags
 * \param params	Socket transmit parameters
 */
int genavb_socket_tx_open(struct genavb_socket_tx **sock, genavb_sock_f_t flags,
			  struct genavb_socket_tx_params *params);

/** Socket transmit
 * \ingroup socket_tx
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param sock		Socket handle
 * \param buf		buffer containing the data to send.
 * \param len		length of the data in bytes.
 */
int genavb_socket_tx(struct genavb_socket_tx *sock, void *buf, unsigned int len);

/** Socket transmit
 * \ingroup socket_tx
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param sock		Socket handle
 * \param buf		buffer containing the data to send.
 * \param len		length of the data in bytes.
 * \param params	transmit parameters.
 */
int genavb_socket_tx_send(struct genavb_socket_tx *sock, void *buf, unsigned int len, struct genavb_socket_tx_send_params *params);

/** Socket transmit with timestamp required
 * \ingroup socket_tx
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param sock		Socket handle
 * \param buf		buffer containing the data to send.
 * \param len		length of the data in bytes.
 * \param ts_priv	private data used to retrieve timestamp.
 */
int genavb_socket_tx_ts(struct genavb_socket_tx *sock, void *buf, unsigned int len, unsigned int ts_priv);

/** Retrieve timestamp after socket transmit with timestamp required
 * \ingroup socket_tx
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param sock		Socket handle
 * \param ts		timestamp retrieved.
 * \param ts_priv	private data for the timestamp.
 */
int genavb_socket_tx_get_ts(struct genavb_socket_tx *sock, uint64_t *ts, unsigned int *ts_priv);

/** Socket receive
 * \ingroup socket_rx
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param sock		Socket handle
 * \param buf		buffer where data is to be copied.
 * \param len		length of the data in bytes.
 * \param ts		pointer to where to save receive timestamps.
 */
int genavb_socket_rx(struct genavb_socket_rx *sock, void *buf, unsigned int len, uint64_t *ts);

/** Close rx socket
 * \ingroup socket_rx
 * \param sock		Socket handle
 */
void genavb_socket_rx_close(struct genavb_socket_rx *sock);

/** Close tx socket
 * \ingroup socket_tx
 * \param sock		Socket handle
 */
void genavb_socket_tx_close(struct genavb_socket_tx *sock);

/* OS specific headers */
#include "os/socket.h"

#endif /* _GENAVB_PUBLIC_SOCKET_API_H_ */
