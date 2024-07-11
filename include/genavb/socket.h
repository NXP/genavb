/*
 * Copyright 2018, 2020, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 \file
 \brief GenAVB public API
 \details Socket API definition for the GenAVB library

 \copyright Copyright 2018, 2020, 2023-2024 NXP
*/

#ifndef _GENAVB_PUBLIC_SOCKET_API_H_
#define _GENAVB_PUBLIC_SOCKET_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "types.h"
#include "net_types.h"

struct genavb_socket_rx;
struct genavb_socket_tx;

/**
 * \ingroup socket
 * Socket rx parameters
 */
struct genavb_socket_rx_params {
	struct net_address addr; /**< Socket address */
};

/**
 * \ingroup socket
 * Socket tx parameters
 */
struct genavb_socket_tx_params {
	struct net_address addr; /**< Socket address */
};

/** Open rx socket
 * \ingroup socket
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param sock		Socket handle
 * \param flags		Socket receive flags
 * \param params	Socket receive parameters
 */
int genavb_socket_rx_open(struct genavb_socket_rx **sock, genavb_sock_f_t flags,
			  struct genavb_socket_rx_params *params);

/** Open tx socket
 * \ingroup socket
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param sock		Socket handle
 * \param flags		Socket transmit flags
 * \param params	Socket transmit parameters
 */
int genavb_socket_tx_open(struct genavb_socket_tx **sock, genavb_sock_f_t flags,
			  struct genavb_socket_tx_params *params);

/** Socket transmit
 * \ingroup socket
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param sock		Socket handle
 * \param buf		buffer containing the data to send.
 * \param len		length of the data in bytes.
 */
int genavb_socket_tx(struct genavb_socket_tx *sock, void *buf, unsigned int len);

/** Socket transmit with timestamp required
 * \ingroup socket
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param sock		Socket handle
 * \param buf		buffer containing the data to send.
 * \param len		length of the data in bytes.
 * \param ts_priv	private data used to retrieve timestamp.
 */
int genavb_socket_tx_ts(struct genavb_socket_tx *sock, void *buf, unsigned int len, unsigned int ts_priv);

/** Retrieve timestamp after socket transmit with timestamp required
 * \ingroup socket
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param sock		Socket handle
 * \param ts		timestamp retrieved.
 * \param ts_priv	private data for the timestamp.
 */
int genavb_socket_tx_get_ts(struct genavb_socket_tx *sock, uint64_t *ts, unsigned int *ts_priv);

/** Socket receive
 * \ingroup socket
 * \return		::GENAVB_SUCCESS or negative error code.
 * \param sock		Socket handle
 * \param buf		buffer where data is to be copied.
 * \param len		length of the data in bytes.
 * \param ts		pointer to where to save receive timestamps.
 */
int genavb_socket_rx(struct genavb_socket_rx *sock, void *buf, unsigned int len, uint64_t *ts);

/** Close rx socket
 * \ingroup socket
 * \param sock		Socket handle
 */
void genavb_socket_rx_close(struct genavb_socket_rx *sock);

/** Close tx socket
 * \ingroup socket
 * \param sock		Socket handle
 */
void genavb_socket_tx_close(struct genavb_socket_tx *sock);

/* OS specific headers */
#include "os/socket.h"

#endif /* _GENAVB_PUBLIC_SOCKET_API_H_ */
