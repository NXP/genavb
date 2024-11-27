/*
 * Copyright 2015-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2021, 2023-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Network services
 @details Network services implementation
*/

#ifndef _OS_NET_H_
#define _OS_NET_H_

#include "os/sys_types.h"

#include "osal/net.h"
#include "genavb/stats.h"
#include "genavb/qos.h"

/** Initializes a network receive context
 *
 * If addr is not NULL, the receive queue will be bound to a particular address.
 * All callers must pass a valid address or no packets will be received.
 *
 * \return 0 on success, -1 on error
 * \param rx	pointer to network receive context
 * \param addr	pointer to network address structure (may be NULL)
 * \param func	callback to receive network descriptors
 * \param priv	platform dependent code private data
 */
int net_rx_init(struct net_rx *rx, struct net_address *addr, void (*func)(struct net_rx *, struct net_rx_desc *), unsigned long priv);

/** Initializes a network receive context (with support for batching)
 *
 * If addr is not NULL, the receive queue will be bound to a particular address.
 * All callers must pass a valid address or no packets will be received.
 * To enable buffering set both packets and time to a value above 0.
 *
 * \return 0 on success, -1 on error
 * \param rx	pointer to network receive context
 * \param addr	pointer to network address structure (may be NULL)
 * \param func	callback to receive network descriptors (with support for batching)
 * \param packets receive buffer level required to call the callback (in packet units)
 * \param time	receive buffer level required to call the callback (in nanosecond units, measured from the arrival time of the first packet in the queue)
 * \param priv	platform dependent code private data
 */
int net_rx_init_multi(struct net_rx *rx, struct net_address *addr, void (*func)(struct net_rx *, struct net_rx_desc **, unsigned int), unsigned int packets, unsigned int time, unsigned long priv);

/** Cleans up a network receive context
 *
 * \return	none
 * \param rx	pointer to network receive context
 */
void net_rx_exit(struct net_rx *rx);

/** Network receive handler
 *
 * The function dequeues one packet and returns the descriptor.
 *
 * \return	pointer to rx network descriptor
 * \param rx	pointer to network receive context
 */
struct net_rx_desc * __net_rx(struct net_rx *rx);

/** Network receive handler
 *
 * The function dequeues one packet at a time from the receive queue and calls the registered handler for each packet
 *
 * \return	none
 * \param rx	pointer to network receive context
 */
void net_rx(struct net_rx *rx);

/** Network receive handler (with batching)
 *
 * The function dequeues a batch of packets from the receive queue and calls the registered handler for the entire
 * batch
 *
 * \return	none
 * \param rx	pointer to network receive context
 */
void net_rx_multi(struct net_rx *rx);

/** Network transmit handler (with batching)
 *
 * The function takes ownership of descriptors and will either transmit them successfully or free/drop
 * them.
 *
 * \return Number of successfully transmited descriptors, -1 on error
 * \param tx	pointer to network transmit context
 * \param desc	array of network transmit descriptor pointers
 * \param n	array length
 */
int net_tx_multi(struct net_tx *tx, struct net_tx_desc **desc, unsigned int n);

/** Network transmit handler
 *
 * In case of error, the caller must free the descriptor
 *
 * \return Number of successfully transmited descriptors (one), -1 on error
 * \param tx	pointer to network transmit context
 * \param desc	pointer to network transmit descriptor
 */
int net_tx(struct net_tx *tx, struct net_tx_desc *desc);

/** Initializes a network transmit context
 *
 * If addr is not NULL, the transmit queue will be connected to a particular address.
 * SR streams should always pass a valid address structure or traffic shapping will not be performed.
 *
 * \return 0 on success, -1 on error
 * \param tx	pointer to network transmit context
 * \param addr	pointer to network address structure (may be NULL)
 */
int net_tx_init(struct net_tx *tx, struct net_address *addr);

/** Enable RAW transmiation without updating SRC MAC address
 * \param tx	pointer to network transmit context
 */
void net_tx_enable_raw(struct net_tx *tx);

/** Cleans up a network transmit context
 *
 * \param tx	pointer to network transmit context
 */
void net_tx_exit(struct net_tx *tx);

/** Retrieves port mac address
 *
 * \return 0 on success, -1 on error
 * \param port_id	port id
 * \param addr		pointer to mac address buffer
 */
int net_get_local_addr(unsigned int port_id, unsigned char *addr);

/** Initializes a network transmit context (with support for transmit timestamps)
 *
 * \return 0 on success, -1 on error
 * \param tx		pointer to network transmit context
 * \param addr		pointer to network address structure
 * \param func		callback to receive transmit timestamps
 * \param priv		platform dependent code private data
 */
int net_tx_ts_init(struct net_tx *tx, struct net_address *addr, void (*func)(struct net_tx *, uint64_t, unsigned int), unsigned long priv);

/** Cleans up a network transmit context (with support for transmit timestamps)
 *
 * \return 0 on success, -1 on error
 * \param tx		pointer to network transmit context
 */
int net_tx_ts_exit(struct net_tx *tx);

/** Get Network transmit timestamp from ts queue
 *
 * The function dequeues one timestamp at a time from the transmit timestamp receive queue
 *
 * \return	1 on success, -1 on error
 * \param tx	pointer to network transmit context
 * \param ts	pointer to timestamp
 * \param tx	pointer to private data
 */
int net_tx_ts_get(struct net_tx *tx, uint64_t *ts, unsigned int *private);

/** Network transmit timestamp receive handler
 *
 * The function dequeues one timestamp at a time from the transmit timestamp receive queue
 * and calls the registered handler for each timestamp.
 *
 * \return	none
 * \param tx	pointer to network transmit context
 */
void net_tx_ts_process(struct net_tx *tx);

/** Multicast address add
 *
 * This function programs the network device associated to the net_rx context
 * to listen to the specified multicast address. If the address is already registered
 * a reference count is incremented.
 *
 * \return	0 on success, -1 on error
 * \param rx	pointer to network receive  context
 * \param port_id	port number
 * \param hw_addr	ethernet multicast address
 */
int net_add_multi(struct net_rx *rx, unsigned int port_id, const unsigned char *hw_addr);

/** Multicast address delete
 *
 * This function programs the network device associated to the net_rx context
 * to not listen anymore to the specified multicast address. The multicast
 * address is really removed from the MAC when the reference count drops to zero.
 *
 * \return	0 on success, -1 on error
 * \param rx	pointer to network receive  context
 * \param port_id	port number
 * \param hw_addr	ethernet multicast address
 */
int net_del_multi(struct net_rx *rx, unsigned int port_id, const unsigned char *hw_addr);

struct net_tx_desc *net_tx_alloc(struct net_tx *tx, unsigned int size);

int net_tx_alloc_multi(struct net_tx *tx, struct net_tx_desc **desc, unsigned int n, unsigned int size);

struct net_tx_desc *net_tx_clone(struct net_tx *tx, struct net_tx_desc *src);

void net_tx_free(struct net_tx_desc *buf);

void net_rx_free(struct net_rx_desc *buf);

void net_free_multi(void **buf, unsigned int n);

/** Enable transmit event notification from the network context.
 *
 * \return	0 on success, -1 on error
 * \param tx	pointer to network transmit context
 * \param priv	private data
 */
int net_tx_event_enable(struct net_tx *tx, unsigned long priv);

/** Disable event notification from the network context.
 *
 * \return	0 on success, -1 on error
 * \param tx	pointer to network transmit context
 */
int net_tx_event_disable(struct net_tx *tx);

/** Returns the available free space in the network transmit queue.
 *
 * \return 		0 if there is no free space in the transmit queue, else returns the space available in the transmit queue
 * \param tx	pointer to network transmit context
 */
unsigned int net_tx_available(struct net_tx *tx);


/** Returns the status of a logical port.
 *
 * \return 		0 on success, -1 on error
 * \param tx		pointer to network transmit context
 * \param port_id	port number
 * \param up		true if port is up, false otherwise
 * \param point_to_point	true if port is full duplex (valid only if up is true)
 * \param rate		port rate in bits per second (valid only if up is true)
 */
int net_port_status(struct net_tx *tx, unsigned int port_id, bool *up, bool *point_to_point, uint64_t *rate);

/** Returns the traffic class for a given priority and logical port.
 *
 * \return 		traffic class
 * \param port_id	logical port number
 * \param priority	priority
 */
unsigned int net_port_priority_to_traffic_class(unsigned int port_id, uint8_t priority);

/** Set the traffic class on hardware of the port.
 *
 * \return 		GENAVB_SUCCESS or negative error code.
 * \param port_id	logical port number
 * \param config	configuration of traffic class
 */
int net_port_traffic_class_set(unsigned int port_id, struct genavb_traffic_class_config *config);

/** Returns the number of available stats
 *
 * \return 		stats array size or negative value on error
 * \param port_id	logical port number
 */
int net_port_stats_get_number(unsigned int port_id);

/** Write back the statistics names
 *
 * \return 		0 on success, -1 on error
 * \param port_id	logical port number
 * \param buf		pointer to buffer used to return pointers to statistics
 * 			names strings (array of const char *)
 * \param buf_len	buffer length
 */
int net_port_stats_get_strings(unsigned int port_id, const char **buf, unsigned int buf_len);

/** Write back the statistics
 *
 * \return 		0 on success, -1 on error
 * \param port_id	logical port number
 * \param buf		pointer to buffer which will be used to write statistics
 * 			values (array of uint64_t values)
 * \param buf_len	buffer length
 */
int net_port_stats_get(unsigned int port_id, uint64_t *buf, unsigned int buf_len);

/** Enable software mac learning on bridge
 *
 * \return 		0 on success, -1 on error
 * \param enable	enable software mac learn
 */
int bridge_software_maclearn(bool enable);

/** Get the status (up/duplex/rate) of a logical port.
 *
 * \return		GENAVB_SUCCESS or negative error code.
 * \param port_id	logical port number
 * \param up		true if port is up, false otherwise
 * \param duplex	true if port is full duplex, false if port is half duplex
 * \param rate		port rate in bits per second
 */
int net_port_status_get(unsigned int port_id, bool *up, bool *duplex, uint64_t *rate);

/** Returns the MTU size of a logical port.
 *
 * \return 		the MTU size
 * \param tx		pointer to network transmit context
 * \param port_id	logical port number
 */
unsigned int net_port_mtu_get(struct net_tx *tx, unsigned int port_id);

/** Sets the maximum frame size
 *
 * \return		GENAVB_SUCCESS or negative error code.
 * \param port_id	logical port number
 * \param size		maximum frame size to set
 */
int net_port_set_max_frame_size(unsigned int port_id, uint16_t size);

#endif /* _OS_NET_H_ */
