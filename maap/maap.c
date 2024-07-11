/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2018, 2021-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief MAAP common code
 @details Handles all MAAP
*/

#include "os/stdlib.h"

#include "genavb/qos.h"

#include "common/log.h"
#include "common/random.h"

#include "maap.h"

static const u8 maap_multicast_addr[6] = MAAP_MULTICAST_ADDR;

static void maap_sm(struct maap_range *range, struct maap_port *port, maap_event_t event, struct maap_range_info *range_info);
static void maap_ipc_indication(struct maap_ctx *maap, struct ipc_tx *ipc, unsigned int ipc_dst, unsigned int port_id, u32 range_id, const u8 *base_addr, u16 count, u16 status);
static void maap_net_transmit(struct maap_port *port, struct maap_range *range, u8 msg_type, struct maap_range_info *range_info);

static const char *maap_event2string(maap_event_t event)
{
	switch (event) {
	case2str(MAAP_EVENT_BEGIN);
	case2str(MAAP_EVENT_RELEASE);
	case2str(MAAP_EVENT_RESTART);
	case2str(MAAP_EVENT_RESERVE_ADDRESS);
	case2str(MAAP_EVENT_RPROBE);
	case2str(MAAP_EVENT_RDEFEND);
	case2str(MAAP_EVENT_RANNOUNCE);
	case2str(MAAP_EVENT_PROBE_COUNT);
	case2str(MAAP_EVENT_ANNOUNCE_TIMER);
	case2str(MAAP_EVENT_PROBE_TIMER);
	case2str(MAAP_EVENT_PORT_OPERATIONAL);
	case2str(MAAP_EVENT_ERR);
	default:
		return (char *) "Unknown maap event";
	}
}

/* _COMMON_MAAP_CODE_ */
/**
 * Check if range of addresses is between 91:E0:F0:00:00:00 and 91:E0:F0:00:FD:FF (dynamic pool)
 * \return true is the range is in the dynamic pool, false otherwise
 * \param start_address
 * \param count, number of addresses
 */
static bool maap_is_in_dynamic_pool(const u8 *start_address, unsigned int count)
{
	const u8 addr_min[6] = MAAP_DYNAMIC_POOL_MIN;
	const u8 addr_max[6] = MAAP_DYNAMIC_POOL_MAX;

	u64 start_addr;
	u64 end_addr;
	bool check_start, check_end;

	start_addr = MAC_VALUE(start_address);
	end_addr = start_addr + count - 1;

	check_start = (start_addr >= MAC_VALUE(addr_min)) && (start_addr <= MAC_VALUE(addr_max));
	check_end = (end_addr <= MAC_VALUE(addr_max));

	return (check_start && check_end);
}

/**
 * Generate a random mac address between 91:E0:F0:00:00:00 and 91:E0:F0:00:FD:FF
 * \param range, the pointer to the range that need to generate its first MAC address
 * \param count, number of addresses in the range, max 65024
 */
/* FIXME : Seed should depend of the local mac address */
static void maap_generate_address(struct maap_range *range, unsigned int count)
{
	u16 rand_bytes;
	unsigned int max;

	if (count > MAAP_DYNAMIC_POOL_SIZE)
		count = MAAP_DYNAMIC_POOL_SIZE;

	max = MAAP_DYNAMIC_POOL_SIZE - count;
	rand_bytes = random_range(0x0000, max);

	range->start_mac_addr[0] = 0x91;
	range->start_mac_addr[1] = 0xE0;
	range->start_mac_addr[2] = 0xF0;
	range->start_mac_addr[3] = 0x00;
	range->start_mac_addr[4] = (rand_bytes >> 8) & 0xFF;
	range->start_mac_addr[5] = rand_bytes & 0xFF;
}

/**
 * Detect a conflict between two address ranges
 * \return TRUE if a conflict has been detected, FALSE otherwise
 * \param port, the port
 * \param addr_local, the first address of the local range
 * \param count_local, the number of addresses in the local range
 * \param addr_pdu, the first address of the range received from the AVTPDU
 * \param count_pdu, the number of addresses in the range received from the AVTPDU
 * \param range_info, a maap_range_info structure in which are stored the informations related to the conflict
 */
static bool are_ranges_in_conflict(struct maap_port *port, const u8 *addr_local, unsigned int count_local, const u8 *addr_pdu, unsigned int count_pdu, struct maap_conflict_info *conflict_info)
{
	u64 tmp_addr_loc;
	u64 tmp_addr_loc_end;
	u64 tmp_addr_pdu;
	u64 tmp_addr_pdu_end;
	const u8 *addr_conflict_ptr;
	u64 addr_conflict;
	u64 addr_conflict_end;

	/* Convert addresses to u64 */
	tmp_addr_loc = MAC_VALUE(addr_local);
	tmp_addr_pdu = MAC_VALUE(addr_pdu);

	tmp_addr_loc_end = tmp_addr_loc + (count_local - 1);
	tmp_addr_pdu_end = tmp_addr_pdu + (count_pdu - 1);

	if (tmp_addr_pdu < tmp_addr_loc && tmp_addr_pdu_end >= tmp_addr_loc) {
		/* start address from pdu is before local start address but end address from pdu is after local start address = conflict */
		addr_conflict_ptr = addr_local;
		addr_conflict = tmp_addr_loc;
		addr_conflict_end = (tmp_addr_pdu_end < tmp_addr_loc_end) ? tmp_addr_pdu_end : tmp_addr_loc_end;

	} else if (tmp_addr_pdu >= tmp_addr_loc && tmp_addr_pdu <= tmp_addr_loc_end) {
		/* start address from pdu is between start and end local addresses = conflict */
		addr_conflict_ptr = addr_pdu;
		addr_conflict = tmp_addr_pdu;
		addr_conflict_end = (tmp_addr_pdu_end < tmp_addr_loc_end) ? tmp_addr_pdu_end : tmp_addr_loc_end;

	} else {
		/* start address from pdu is after local end address or end address from pdu is before local start address */
		return false;
	}

	conflict_info->count = (unsigned int)(addr_conflict_end - addr_conflict);
	conflict_info->count += 1;

	os_log(LOG_INFO, "maap conflict detected on count(%u) addresses, starting at %016" PRIx64 " on port(%u)\n", conflict_info->count, addr_conflict, port->port_id);

	os_memcpy(conflict_info->start_addr, addr_conflict_ptr, sizeof(u8) * 6);

	return true;
}

/**
 * When allocating a new range on a port, check if it will generate a conflict with the other ranges of the same port
 * \return true if a conflict is found, false otherwise
 * \param port pointer to the MAAP port
 * \param new_range pointer to the newly allocated range
 */
static bool check_internal_conflict(struct maap_port *port, struct maap_range *new_range)
{
	struct list_head *entry, *entry_next;
	struct maap_range *range;
	struct maap_range_info range_info;

	/* Check conflict on each range of this port */
	for (entry = list_first(&port->range); entry_next = list_next(entry), entry != &port->range; entry = entry_next) {

		range = container_of(entry, struct maap_range, list);

		if (are_ranges_in_conflict(port, range->start_mac_addr, range->mac_count, new_range->start_mac_addr, new_range->mac_count, &range_info.conflict))
			return true;
	}

	return false;
}

/**
 * Get the range accroding to its id
 * \return the range with this id, NULL if the id is not used
 * \param maap, pointer to maap context
 * \param port, pointer to port
 * \param range_id, id of the range
 */
static struct maap_range *get_range_by_id(struct maap_ctx *maap, struct maap_port *port, u32 range_id)
{
	struct list_head *entry, *entry_next;
	struct maap_range *range;

	for (entry = list_first(&port->range); entry_next = list_next(entry), entry != &port->range; entry = entry_next) {

		range = container_of(entry, struct maap_range, list);

		if (range->range_id == range_id)
			return range;
	}

	return NULL;
}

/**
 * Check if range_id is already used in this maap context
 * \return false if there is no range with this id, otherwise true
 * \param maap, pointer to maap context
 * \param port, pointer to port
 * \param range_id, id to check
 */
static bool is_range_id_used(struct maap_ctx *maap, struct maap_port *port, u32 range_id)
{
	struct maap_range *range;

	range = get_range_by_id(maap, port, range_id);

	if (!range)
		return false;

	return true;
}

/**
 * Compare mac addresses
 * \return TRUE if the MAC address of the receiving station is numerically lower than the MAC address from the received MAAP AVTPDU
 * \param local mac address, the physical mac address of the receiving station
 * \param received mac address, the physical mac address from the AVTPDU
 */
static bool maap_compare_MAC(const u8 *local_mac_address, const u8 *received_mac_address)
{
	u64 addr_local_u64;
	u64 addr_received_u64;

	/* Convert addresses to u64 */
	addr_local_u64 = REVERSE_MAC_VALUE(local_mac_address);
	addr_received_u64 = REVERSE_MAC_VALUE(received_mac_address);

	os_log(LOG_DEBUG, "Comparing phy mac local %016" PRIx64 " and received %016" PRIx64 "\n", addr_local_u64, addr_received_u64);

	return (addr_local_u64 < addr_received_u64);
}

/**
 * Release action from 1722-2016 Annex B. Free the range on release event
 * \param maap, Context
 * \param port
 * \param range
 */
static void maap_release(struct maap_ctx *maap, struct maap_port *port, struct maap_range *range)
{
	u8 addr[6] = {0};
	unsigned int count = 0;
	u32 range_id;

	range_id = range->range_id;

	if (maap_remove_range_on_port(maap, port, range_id, addr, &count) < 0) {
		maap_ipc_indication(maap, &maap->ipc_tx, IPC_DST_ALL, port->port_id, range_id, addr, count, MAAP_STATUS_ERROR);

	} else {
		maap_ipc_indication(maap, &maap->ipc_tx, IPC_DST_ALL, port->port_id, range_id, addr, count, MAAP_STATUS_FREE);
	}
}

/**
 * sProbe action from 1722-2016 Annex B
 * \param port
 * \param range
 */
static void maap_sprobe(struct maap_port *port, struct maap_range *range)
{
	long int rand_interval;
	unsigned int interval;

	maap_net_transmit(port, range, MAAP_PROBE, NULL);

	rand_interval = random_range(MAAP_PROBE_INTERVAL_MIN, MAAP_PROBE_INTERVAL_MAX);
	interval = (unsigned int)os_abs((int)rand_interval);

	timer_stop(&range->sm.probe_timer);
	os_log(LOG_DEBUG, "Starting probe timer of %d ms\n", interval);
	timer_start(&range->sm.probe_timer, interval);
}

/**
 * sAnnounce action from 1722-2016 Annex B
 * \param port
 * \param range
 */
static void maap_sannounce(struct maap_port *port, struct maap_range *range)
{
	long int rand_interval;
	unsigned int interval;

	maap_net_transmit(range->port, range, MAAP_ANNOUNCE, NULL);

	rand_interval = random_range(MAAP_ANNOUNCE_INTERVAL_MIN, MAAP_ANNOUNCE_INTERVAL_MAX);
	interval = (unsigned int)os_abs((int)rand_interval);

	timer_stop(&range->sm.announce_timer);
	os_log(LOG_DEBUG, "Starting announce timer of %d ms\n", interval);
	timer_start(&range->sm.announce_timer, interval);
}

/**
 * Update maap state machine according to input event
 * \return none
 * \param range, MAAP range which contains the state machine
 * \param port
 * \param event, the triggered event, refer to maap_event_t enum to see all available events
 * \param range_info, informations related to a conflict (between ranges) detected in a received PROBE, ANNOUNCE or DEFEND
 */
static void maap_sm(struct maap_range *range, struct maap_port *port, maap_event_t event, struct maap_range_info *range_info)
{
	struct maap_ctx *maap = container_of(port, struct maap_ctx, port[port->port_id]);
	maap_state_t state;

start:
	state = range->sm.state;

	os_log(LOG_DEBUG, "Event %s (%d) on port(%u) with the range(%02x:%02x:%02x:%02x:%02x:%02x, %u)\n",
				maap_event2string(event), event, port->port_id, range->start_mac_addr[0], range->start_mac_addr[1],
				range->start_mac_addr[2], range->start_mac_addr[3], range->start_mac_addr[4], range->start_mac_addr[5], range->mac_count);

	switch (event) {
	case MAAP_EVENT_BEGIN:
		switch (state) {
		case MAAP_STATE_INITIAL:

			/* For the Begin event we generated the range address while initializing the range (maap_range_alloc) */
			event = MAAP_EVENT_RESERVE_ADDRESS;
			goto start;

			break;

		default:
			break;
		}
		break;

	case MAAP_EVENT_RELEASE:
		switch (state) {
		case MAAP_STATE_PROBE:
			timer_stop(&range->sm.probe_timer);

			maap_release(maap, port, range);

			/* range->sm.state = MAAP_STATE_INITIAL; Range freed */
			return;

		case MAAP_STATE_DEFEND:
			timer_stop(&range->sm.announce_timer);

			maap_release(maap, port, range);

			/* range->sm.state = MAAP_STATE_INITIAL; Range freed */
			return;

		default:
			break;
		}
		break;

	case MAAP_EVENT_RESTART:
		switch (state) {
		case MAAP_STATE_INITIAL:
			/* Send conflict indication only if we previously (last sent message) notified upper layer with success indication. */
			if (range->sm.send_conflict_indication) {
				maap_ipc_indication(maap, &maap->ipc_tx, IPC_DST_ALL, port->port_id, range->range_id,
							range->start_mac_addr, range->mac_count, MAAP_STATUS_CONFLICT);

				range->sm.send_conflict_indication = false;
			}

			maap_generate_address(range, range->mac_count);
			/* This action triggers a ReserveAddress event 1722-2016 Table B.7 */
			event = MAAP_EVENT_RESERVE_ADDRESS;
			goto start;

			break;

		default:
			break;
		}
		break;

	case MAAP_EVENT_RESERVE_ADDRESS:
		switch (state) {
		case MAAP_STATE_INITIAL:
			range->sm.maap_probe_count = MAAP_PROBE_RETRANSMITS_VAL;

			/* Send the probe and start the probe timer */
			maap_sprobe(port, range);

			range->sm.state = MAAP_STATE_PROBE;
			break;

		default:
			break;
		}
		break;

	case MAAP_EVENT_RPROBE:
		switch (state) {
		case MAAP_STATE_PROBE:
			if (!maap_compare_MAC(range_info->local_physical_mac, range_info->src_physical_mac)) {
				timer_stop(&range->sm.probe_timer);

				/* Received a conflicting probe message */
				range->sm.state = MAAP_STATE_INITIAL;
				/* This action trigger a Restart event */
				event = MAAP_EVENT_RESTART;
				goto start;
			}
			break;

		case MAAP_STATE_DEFEND:
			maap_net_transmit(port, range, MAAP_DEFEND, range_info);

			break;

		default:
			break;
		}
		break;

	case MAAP_EVENT_RDEFEND:
	case MAAP_EVENT_RANNOUNCE:
		switch (state) {
		case MAAP_STATE_PROBE:
			timer_stop(&range->sm.probe_timer);

			/* Received a conflicting announce message */
			range->sm.state = MAAP_STATE_INITIAL;
			/* This action trigger a Restart event */
			event = MAAP_EVENT_RESTART;
			goto start;

			break;

		case MAAP_STATE_DEFEND:
			if (!maap_compare_MAC(range_info->local_physical_mac, range_info->src_physical_mac)) {
				timer_stop(&range->sm.announce_timer);

				/* Received a conflicting announce message */
				range->sm.state = MAAP_STATE_INITIAL;
				/* This action trigger a Restart event */
				event = MAAP_EVENT_RESTART;
				goto start;
			}
			break;

		default:
			break;
		}
		break;

	case MAAP_EVENT_PROBE_COUNT:
		switch (state) {
		case MAAP_STATE_PROBE:
			timer_stop(&range->sm.probe_timer);
			/* Device has sent all its probe messages without receiving conflicting messages, the addresses of the range are successfully attributed */
			maap_ipc_indication(maap, &maap->ipc_tx, IPC_DST_ALL, port->port_id, range->range_id, range->start_mac_addr, range->mac_count, MAAP_STATUS_SUCCESS);

			range->sm.send_conflict_indication = true;

			os_log(LOG_INFO, "Probe phase ended on port(%u), the range(%02x:%02x:%02x:%02x:%02x:%02x, %u) has been allocated successfully\n",
						port->port_id, range->start_mac_addr[0], range->start_mac_addr[1], range->start_mac_addr[2],
						range->start_mac_addr[3], range->start_mac_addr[4], range->start_mac_addr[5], range->mac_count);

			/* Start sending announces and start the announce timer */
			maap_sannounce(port, range);

			range->sm.state = MAAP_STATE_DEFEND;
			break;

		default:
			break;
		}
		break;

	case MAAP_EVENT_ANNOUNCE_TIMER:
		switch (state) {
		case MAAP_STATE_DEFEND:
			maap_sannounce(port, range);
			break;

		default:
			break;
		}
		break;

	case MAAP_EVENT_PROBE_TIMER:
		switch (state) {
		case MAAP_STATE_PROBE:
			maap_sprobe(port, range);

			range->sm.maap_probe_count -= 1;

			if (range->sm.maap_probe_count <= 0) {
				event = MAAP_EVENT_PROBE_COUNT;
				goto start;
			}

			break;

		default:
			break;
		}
		break;

	case MAAP_EVENT_PORT_OPERATIONAL:
		switch (state) {
		case MAAP_STATE_INITIAL:
			/* For the PortOperational event, we don't re-generate the range address (or re-initialize the whole range),
			* we can re-use the range address that was generated before the port went down,
			* assuming that an higher component don't free the range when the port goes down
			*/
			event = MAAP_EVENT_RESERVE_ADDRESS;
			goto start;

			break;

		case MAAP_STATE_PROBE:
			timer_stop(&range->sm.probe_timer);

			range->sm.state = MAAP_STATE_INITIAL;
			/* Do not restart and keep using the same address as when the port went down. */
			event = MAAP_EVENT_RESERVE_ADDRESS;
			goto start;

			break;

		case MAAP_STATE_DEFEND:
			timer_stop(&range->sm.announce_timer);

			range->sm.state = MAAP_STATE_INITIAL;
			/* Do not restart and keep using the same address as when the port went down. */
			event = MAAP_EVENT_RESERVE_ADDRESS;
			goto start;

			break;

		default:
			break;
		}
		break;

	default:
		os_log(LOG_ERR, "maap(%p) invalid event: %d\n", maap, event);
		break;
	}
}

/**
 * Get maap port from its logical port id
 * \return the maap port with the corresponding logical port id
 * \param maap, maap context
 * \param port_id, logical port id
 */
struct maap_port *logical_to_maap_port(struct maap_ctx *maap, unsigned int logical_port)
{
	int i;

	for (i = 0; i < maap->port_max; i++) {
		if (logical_port == maap->port[i].logical_port) {
			return &maap->port[i];
		}
	}

	return NULL;
}

/**
 * Get logical port
 * \return logical port id
 * \param maap_ctx, maap context
 * \param port_id
 */
unsigned int maap_port_to_logical(struct maap_ctx *maap, unsigned int port_id)
{
	return maap->port[port_id].logical_port;
}
/* _COMMON_MAAP_CODE_ */

/* _COMMUNICATION_HANDLERS_ */
/* NET */
/* NET_RX */
/*
 * Handle a received PROBE message
 * \param port
 * \param pdu, the message
 * \param desc, the net rx descriptor
 */
static void maap_handle_probe(struct maap_port *port, struct maap_pdu *pdu, struct net_rx_desc *desc)
{
	struct eth_hdr *eth = (struct eth_hdr *)((u8 *)desc + desc->l2_offset);
	struct maap_range *range;
	struct list_head *entry, *entry_next;
	struct maap_range_info range_info;
	u16 requested_count;

	requested_count = ntohs(pdu->requested_count);

	os_log(LOG_DEBUG, "Received PROBE on port(%u) for the range(%02x:%02x:%02x:%02x:%02x:%02x, %u)\n",
				port->port_id, pdu->requested_start_address[0], pdu->requested_start_address[1],
				pdu->requested_start_address[2], pdu->requested_start_address[3], pdu->requested_start_address[4],
				pdu->requested_start_address[5], requested_count);

	/* Check conflict on each range of this port */
	for (entry = list_first(&port->range); entry_next = list_next(entry), entry != &port->range; entry = entry_next) {

		range = container_of(entry, struct maap_range, list);

		/* Only receive packets that conflict with the range's addresses */
		if (!are_ranges_in_conflict(port, range->start_mac_addr, range->mac_count, pdu->requested_start_address, requested_count, &range_info.conflict))
			continue;

		os_memcpy(range_info.local_physical_mac, port->local_physical_mac, 6);

		os_memcpy(range_info.src_physical_mac, eth->src, sizeof(u8) * 6);

		os_memcpy(range_info.requested_start_addr, pdu->requested_start_address, sizeof(u8) * 6);

		range_info.requested_count = requested_count;

		maap_sm(range, port, MAAP_EVENT_RPROBE, &range_info);
	}
}

/*
 * Handle a received ANNOUNCE message
 * \param port
 * \param pdu, the message
 * \param desc, the net rx descriptor
 */
static void maap_handle_announce(struct maap_port *port, struct maap_pdu *pdu, struct net_rx_desc *desc)
{
	struct eth_hdr *eth = (struct eth_hdr *)((u8 *)desc + desc->l2_offset);
	struct maap_range *range;
	struct list_head *entry, *entry_next;
	struct maap_range_info range_info;
	u16 requested_count;

	requested_count = ntohs(pdu->requested_count);

	os_log(LOG_DEBUG, "Received ANNOUNCE on port(%u) for the range(%02x:%02x:%02x:%02x:%02x:%02x, %u)\n",
				port->port_id, pdu->requested_start_address[0], pdu->requested_start_address[1],
				pdu->requested_start_address[2], pdu->requested_start_address[3], pdu->requested_start_address[4],
				pdu->requested_start_address[5], requested_count);

	for (entry = list_first(&port->range); entry_next = list_next(entry), entry != &port->range; entry = entry_next) {

		range = container_of(entry, struct maap_range, list);

		/* Only receive packets that conflict with the range's address range */
		if (!are_ranges_in_conflict(port, range->start_mac_addr, range->mac_count, pdu->requested_start_address, requested_count, &range_info.conflict))
			continue;

		os_memcpy(range_info.local_physical_mac, port->local_physical_mac, 6);

		os_memcpy(range_info.src_physical_mac, eth->src, sizeof(u8) * 6);

		os_memcpy(range_info.requested_start_addr, pdu->requested_start_address, sizeof(u8) * 6);

		range_info.requested_count = requested_count;

		maap_sm(range, port, MAAP_EVENT_RANNOUNCE, &range_info);
	}
}

/*
 * Handle a received DEFEND message.
 * Note that the requested_start_addr/count are the range requested from a previously sent and conflicting PROBE that induced a DEFEND message as a response.
 * The conflict_start_addr/count are the conflict detected by the sender regarding our previous PROBE.
 * \param port
 * \param pdu, the message
 * \param desc, the net rx descriptor
 */
static void maap_handle_defend(struct maap_port *port, struct maap_pdu *pdu, struct net_rx_desc *desc)
{
	struct eth_hdr *eth = (struct eth_hdr *)((u8 *)desc + desc->l2_offset);
	struct maap_range *range;
	struct list_head *entry, *entry_next;
	struct maap_range_info range_info;
	u16 requested_count;

	requested_count = ntohs(pdu->requested_count);

	for (entry = list_first(&port->range); entry_next = list_next(entry), entry != &port->range; entry = entry_next) {

		range = container_of(entry, struct maap_range, list);

		/* Retrieve the range that sent the PROBE that triggered this DEFEND respond */
		if ((os_memcmp(range->start_mac_addr, pdu->requested_start_address, sizeof(u8) * 6) == 0) && (range->mac_count == requested_count)) {

			os_log(LOG_INFO, "Received DEFEND on port(%u) because the range(%02x:%02x:%02x:%02x:%02x:%02x, %u) is"
					 " in conflict with range(%02x:%02x:%02x:%02x:%02x:%02x, %u)\n",
						port->port_id, range->start_mac_addr[0], range->start_mac_addr[1], range->start_mac_addr[2],
						range->start_mac_addr[3], range->start_mac_addr[4], range->start_mac_addr[5], range->mac_count,
						pdu->conflict_start_address[0], pdu->conflict_start_address[1], pdu->conflict_start_address[2],
						pdu->conflict_start_address[3], pdu->conflict_start_address[4], pdu->conflict_start_address[5], ntohs(pdu->conflict_count));

			os_memcpy(range_info.local_physical_mac, port->local_physical_mac, 6);

			os_memcpy(range_info.src_physical_mac, eth->src, sizeof(u8) * 6);

			os_memcpy(range_info.requested_start_addr, pdu->requested_start_address, sizeof(u8) * 6);

			range_info.requested_count = requested_count;

			os_memcpy(range_info.conflict.start_addr, pdu->conflict_start_address, sizeof(u8) * 6);

			range_info.conflict.count = ntohs(pdu->conflict_count);

			maap_sm(range, port, MAAP_EVENT_RDEFEND, &range_info);
		}
	}
}

/**
 * Handler on receive NET packet
 * \return none
 * \param rx, the rx (receive) NET channel
 * \param desc, descritor of the frame received through the NET channel
 */
static void maap_net_receive(struct net_rx *rx, struct net_rx_desc *desc)
{
	struct maap_port *port = container_of(rx, struct maap_port, net_rx);
	void *data = (char *)desc + desc->l3_offset;
	struct maap_pdu *pdu = (struct maap_pdu *)data;

	/* Checks */
	if (desc->ethertype != ETHERTYPE_AVTP) {
		os_log(LOG_ERR, "MAAP received a net packet with the wrong ethertype\n");
		goto err;
	}

	if (pdu->subtype != AVTP_SUBTYPE_MAAP) {
		os_log(LOG_ERR, "MAAP received a net packet with the wrong subtype, expected: %02X, received: %02X\n", AVTP_SUBTYPE_MAAP, pdu->subtype);
		goto err;
	}

	/* Processing */
	switch (pdu->message_type) {
	case MAAP_PROBE:
		maap_handle_probe(port, pdu, desc);
		break;

	case MAAP_ANNOUNCE:
		maap_handle_announce(port, pdu, desc);
		break;

	case MAAP_DEFEND:
		maap_handle_defend(port, pdu, desc);
		break;

	default:
		os_log(LOG_ERR, "MAAP net received an unknown message type %02X", pdu->message_type);
		break;
	}

err:
	net_rx_free(desc);
}
/* NET_RX */

/* NET_TX */
/**
 * Transmit message as a NET packet
 * \return none
 * \param port
 * \param range
 * \param msg_type PROBE, ANNOUNCE or DEFEND
 * \param range_info, informations on the conflict detected in a received PROBE message that induced sending a DEFEND message as a response
 */
static void maap_net_transmit(struct maap_port *port, struct maap_range *range, u8 msg_type, struct maap_range_info *range_info)
{
	struct maap_pdu *pdu;
	struct net_tx_desc *desc;
	void *buf;

	desc = net_tx_alloc(&port->net_tx, sizeof(struct eth_hdr) + sizeof(struct maap_pdu));
	if (!desc) {
		os_log(LOG_ERR,"Port(%u): Cannot alloc net_tx\n", port->port_id);
		goto err_alloc;
	}

	buf = NET_DATA_START(desc);
	if (msg_type == MAAP_DEFEND && range_info) {
		desc->len += net_add_eth_header(buf, range_info->src_physical_mac, ETHERTYPE_AVTP);
	} else {
		desc->len += net_add_eth_header(buf, maap_multicast_addr, ETHERTYPE_AVTP);
	}

	pdu = (struct maap_pdu *)((char *)buf + desc->len);

	pdu->subtype = AVTP_SUBTYPE_MAAP;
	pdu->sv = 0; /* see IEEE 1722-2016 section 4.4.5.2 (sv (stream_id valid) field) */
	pdu->version = AVTP_VERSION_0; /* see IEEE 1722-2016 section 4.4.3.4 (version field) */
	pdu->message_type = msg_type;
	pdu->maap_version = MAAP_VERSION;
	MAAP_CONTROL_DATA_LENGTH_SET(pdu, 16); /* see IEEE 1722-2016 section B.2.1 */
	pdu->stream_id = 0; /* not used, see IEEE 1722-2016 section B.2.4 */

	if (msg_type == MAAP_DEFEND && range_info) {
		os_memcpy(pdu->requested_start_address, range_info->requested_start_addr, sizeof(u8) * 6);
		pdu->requested_count = htons(range_info->requested_count);
		os_memcpy(pdu->conflict_start_address, range_info->conflict.start_addr, sizeof(u8) * 6);
		pdu->conflict_count = htons(range_info->conflict.count);

		os_log(LOG_DEBUG, "Transmitting %d on port(%u) because our range(%02x:%02x:%02x:%02x:%02x:%02x, %u) is "
				  "in conflict with range(%02x:%02x:%02x:%02x:%02x:%02x, %u)\n",
					msg_type, port->port_id, range->start_mac_addr[0], range->start_mac_addr[1], range->start_mac_addr[2],
					range->start_mac_addr[3], range->start_mac_addr[4], range->start_mac_addr[5], range->mac_count,
					range_info->requested_start_addr[0], range_info->requested_start_addr[1], range_info->requested_start_addr[2],
					range_info->requested_start_addr[3], range_info->requested_start_addr[4], range_info->requested_start_addr[5],
					range_info->conflict.count);

	} else {
		os_memcpy(pdu->requested_start_address, range->start_mac_addr, sizeof(u8) * 6);
		pdu->requested_count = htons(range->mac_count);
		os_memset(pdu->conflict_start_address, 0, sizeof(u8) * 6);
		pdu->conflict_count = htons(0);

		os_log(LOG_DEBUG, "Transmitting %d on port(%u) for the range(%02x:%02x:%02x:%02x:%02x:%02x, %u)\n",
					msg_type, port->port_id, range->start_mac_addr[0], range->start_mac_addr[1], range->start_mac_addr[2],
					range->start_mac_addr[3], range->start_mac_addr[4], range->start_mac_addr[5], range->mac_count);

	}

	desc->len += sizeof(struct maap_pdu);

	if (net_tx(&port->net_tx, desc) < 0) {
		os_log(LOG_ERR,"Port(%u): Cannot transmit packet\n", port->port_id);
		goto err_tx;
	}

	return;

err_tx:
	net_tx_free(desc);

err_alloc:
	return;
}
/* NET_TX */
/* NET */

/* IPC */
/* IPC_TX */
static void maap_ipc_create_response(struct maap_ctx *maap, struct ipc_tx *ipc, unsigned int ipc_dst, unsigned int logical_port, u32 range_id, const u8 *base_addr, u16 count, u16 status)
{
	struct ipc_desc *desc;
	struct ipc_maap_create_response *msg;

	desc = ipc_alloc(ipc, sizeof(struct ipc_maap_create_response));
	if (desc) {
		desc->dst = ipc_dst;
		desc->type = GENAVB_MSG_MAAP_CREATE_RANGE_RESPONSE;
		desc->len = sizeof(struct ipc_maap_create_response);
		desc->flags = 0;

		msg = &desc->u.maap_create_response;

		msg->status = status;
		msg->port_id = logical_port;
		msg->range_id = range_id;
		msg->count = count;
		os_memcpy(msg->base_address, base_addr, sizeof(u8) * 6);

		if (ipc_tx(ipc, desc) < 0) {
			os_log(LOG_ERR, "maap(%p) ipc_tx() failed\n", maap);

			ipc_free(ipc, desc);
		}

		os_log(LOG_DEBUG, "maap(%p) Status %u response for range(%u, %02x:%02x:%02x:%02x:%02x:%02x, %u) sent to %u\n",
			maap, status, range_id, base_addr[0], base_addr[1], base_addr[2], base_addr[3], base_addr[4], base_addr[5], count, ipc_dst);

	} else {
		os_log(LOG_ERR, "maap(%p) ipc_alloc() failed\n", maap);
	}
}

static void maap_ipc_delete_response(struct maap_ctx *maap, struct ipc_tx *ipc, unsigned int ipc_dst, unsigned int logical_port, u32 range_id, u16 status)
{
	struct ipc_desc *desc;
	struct ipc_maap_delete_response *msg;

	desc = ipc_alloc(ipc, sizeof(struct ipc_maap_delete_response));
	if (desc) {
		desc->dst = ipc_dst;
		desc->type = GENAVB_MSG_MAAP_DELETE_RANGE_RESPONSE;
		desc->len = sizeof(struct ipc_maap_delete_response);
		desc->flags = 0;

		msg = &desc->u.maap_delete_response;

		msg->status = status;
		msg->port_id = logical_port;
		msg->range_id = range_id;

		if (ipc_tx(ipc, desc) < 0) {
			os_log(LOG_ERR, "maap(%p) ipc_tx() failed\n", maap);

			ipc_free(ipc, desc);
		}

		os_log(LOG_DEBUG, "maap(%p) Status %u response for range(%u) sent to %u\n", maap, status, range_id, ipc_dst);

	} else {
		os_log(LOG_ERR, "maap(%p) ipc_alloc() failed\n", maap);
	}
}

static void maap_ipc_indication(struct maap_ctx *maap, struct ipc_tx *ipc, unsigned int ipc_dst, unsigned int logical_port, u32 range_id, const u8 *base_addr, u16 count, u16 status)
{
	struct ipc_desc *desc;
	struct ipc_maap_status *msg;

	desc = ipc_alloc(ipc, sizeof(struct ipc_maap_status));
	if (desc) {
		desc->dst = ipc_dst;
		desc->type = GENAVB_MSG_MAAP_STATUS;
		desc->len = sizeof(struct ipc_maap_status);
		desc->flags = 0;

		msg = &desc->u.maap_status;

		msg->status = status;
		msg->port_id = logical_port;
		msg->range_id = range_id;
		msg->count = count;
		os_memcpy(msg->base_address, base_addr, sizeof(u8) * 6);

		if (ipc_tx(ipc, desc) < 0) {
			os_log(LOG_ERR, "maap(%p) ipc_tx() failed\n", maap);

			ipc_free(ipc, desc);
		}

		os_log(LOG_DEBUG, "maap(%p) Status %u indication for range(%u, %02x:%02x:%02x:%02x:%02x:%02x, %u) sent to %u\n",
			maap, status, range_id, base_addr[0], base_addr[1], base_addr[2], base_addr[3], base_addr[4], base_addr[5], count, ipc_dst);

	} else {
		os_log(LOG_ERR, "maap(%p) ipc_alloc() failed\n", maap);
	}
}
/* IPC_TX */

/* IPC_RX */
/**
 * Handler on receive IPC packet. Execute command received through IPC and respond with a GENAVB_MSG_MAAP_STATUS message
 * \return none
 * \param rx, the rx (receive) IPC channel
 * \param desc, descriptor of the IPC packet
 */
static void maap_ipc_receive(struct ipc_rx const *rx, struct ipc_desc *desc)
{
	struct maap_ctx *maap = container_of(rx, struct maap_ctx, ipc_rx);
	struct maap_port *port;
	struct ipc_tx *ipc_tx;
	struct maap_range *range;
	u8 out_base_addr[6] = {0};
	unsigned int out_count = 0;

	if (desc->flags & IPC_FLAGS_AVB_MSG_SYNC)
		ipc_tx = &maap->ipc_tx_sync;
	else
		ipc_tx = &maap->ipc_tx;

	switch (desc->type) {
	case GENAVB_MSG_MAAP_CREATE_RANGE:
		os_log(LOG_DEBUG, "Received GENAVB_MSG_MAAP_CREATE_RANGE\n");

		port = logical_to_maap_port(maap, desc->u.maap_create.port_id);
		if (!port) {
			os_log(LOG_ERR, "maap(%p) invalid logical port(%u)\n", maap, desc->u.maap_create.port_id);
			goto err_create_range;
		}

		if (desc->u.maap_create.flag == 1) {
			range = maap_new_range_on_port(maap, port, desc->u.maap_create.range_id, desc->u.maap_create.base_address, desc->u.maap_create.count);

		} else {
			range = maap_new_range_on_port(maap, port, desc->u.maap_create.range_id, NULL, desc->u.maap_create.count);
		}

		if (!range)
			goto err_create_range;

		maap_ipc_create_response(maap, ipc_tx, desc->src, port->logical_port, range->range_id, range->start_mac_addr, range->mac_count, MAAP_RESPONSE_SUCCESS);

		break;

err_create_range:
		maap_ipc_create_response(maap, ipc_tx, desc->src, desc->u.maap_create.port_id, desc->u.maap_create.range_id,
						desc->u.maap_create.base_address, desc->u.maap_create.count, MAAP_RESPONSE_ERROR);
		break;

	case GENAVB_MSG_MAAP_DELETE_RANGE:
		os_log(LOG_DEBUG, "Received GENAVB_MSG_MAAP_DELETE_RANGE\n");

		port = logical_to_maap_port(maap, desc->u.maap_delete.port_id);
		if (!port) {
			os_log(LOG_ERR, "maap(%p) invalid logical port(%u)\n", maap, desc->u.maap_delete.port_id);
			goto err_delete_range;
		}

		if (maap_remove_range_on_port(maap, port, desc->u.maap_delete.range_id, out_base_addr, &out_count) < 0)
			goto err_delete_range;

		maap_ipc_delete_response(maap, ipc_tx, desc->src, port->logical_port, desc->u.maap_delete.range_id, MAAP_RESPONSE_SUCCESS);
		maap_ipc_indication(maap, ipc_tx, IPC_DST_ALL, port->logical_port, desc->u.maap_delete.range_id, out_base_addr, out_count, MAAP_STATUS_FREE);

		break;

err_delete_range:
		maap_ipc_delete_response(maap, ipc_tx, desc->src, desc->u.maap_delete.port_id, desc->u.maap_delete.range_id, MAAP_RESPONSE_ERROR);
		break;

	default:
		os_log(LOG_ERR, "maap(%p) MAAP unknown ipc type %d\n", maap, desc->type);
		break;
	}

	ipc_free(rx, desc);
}

/* IPC_RX */
static void maap_ipc_rx_mac_service(struct ipc_rx const *rx, struct ipc_desc *desc)
{
	struct maap_port *port = container_of(rx, struct maap_port, ipc_rx_mac_service);
	struct ipc_mac_service_status *status;

	switch (desc->type) {
	case IPC_MAC_SERVICE_STATUS:
		status = &desc->u.mac_service_status;

		os_log(LOG_DEBUG, "IPC_MAC_SERVICE_STATUS: logical_port(%u) operational(%u): received on maap port (%u)\n",
			status->port_id, status->operational, port->port_id);

		if (status->operational) {

			struct list_head *entry, *entry_next;
			struct maap_range *range;

			/* send PortOpertional! event to all ranges */
			for (entry = list_first(&port->range); entry_next = list_next(entry), entry != &port->range; entry = entry_next) {

				range = container_of(entry, struct maap_range, list);

				maap_sm(range, port, MAAP_EVENT_PORT_OPERATIONAL, NULL);
			}
		}
		break;

	default:
		break;
	}

	ipc_free(rx, desc);
}

/* IPC */
/* _COMMUNICATION_HANDLERS_ */

/* TIMER HANDLERS */
/*
 * Probe timer handler that will send periodically PROBE messages
 * \param data, the range with the state machine associated with this timer
 */
static void maap_probe_timer_handler(void *data)
{
	struct maap_range *range = (struct maap_range *)data;
	maap_event_t event;

	event = MAAP_EVENT_PROBE_TIMER;
	maap_sm(range, range->port, event, NULL);
}

/*
 * Announce timer handler that will send periodically ANNOUNCE messages
 * \param data, the range with the state machine associated with this timer
 */
static void maap_announce_timer_handler(void *data)
{
	struct maap_range *range = (struct maap_range *)data;
	maap_event_t event;

	event = MAAP_EVENT_ANNOUNCE_TIMER;
	maap_sm(range, range->port, event, NULL);
}
/* TIMER HANDLERS */

/* _MEMORY_ALLOC_ */
/**
 * Memory allocation for MAAP context
 * \return the pointer to the maap context created
 * \param the max number of ports from the maap_config
 */
__init static struct maap_ctx *maap_alloc(unsigned int n_ports, unsigned int n_timers)
{
	struct maap_ctx *maap;
	unsigned int maap_ctx_size, size;
	u8 *instance;

	maap_ctx_size = sizeof(struct maap_ctx) + n_ports * sizeof(struct maap_port);
	size = maap_ctx_size + timer_pool_size(n_timers);

	maap = os_malloc(size);
	if (!maap)
		goto err_alloc;

	os_memset(maap, 0, size);

	instance = (u8 *)maap + maap_ctx_size;

	maap->timer_ctx = (struct timer_ctx *)(instance);

	maap->port_max = n_ports;

	os_log(LOG_INIT, "maap(%p) allocated with %d ports\n", maap, maap->port_max);

	return maap;

err_alloc:
	return NULL;
}
/* _MEMORY_ALLOC_ */

/* _INIT_ */
/**
 * Initialize MAAP state machine's timers
 * \return	0 on success, negative value on failure
 * \param range, range containing the state machine concerned
 * \param port, the port where the range was allocated
 * \param timer_ctx, pointer to the MAAP context timer
 */
static int maap_timers_init(struct maap_range *range, struct maap_port *port, struct timer_ctx *timer_ctx)
{
	/* Probe timer */
	range->sm.probe_timer.func = maap_probe_timer_handler;
	range->sm.probe_timer.data = range;
	if (timer_create(timer_ctx, &range->sm.probe_timer, 0, MAAP_PROBE_TIMER_GRANULARITY) < 0)
		goto err_timer_probe;

	/* Announce timer */
	range->sm.announce_timer.func = maap_announce_timer_handler;
	range->sm.announce_timer.data = range;
	if (timer_create(timer_ctx, &range->sm.announce_timer, 0, MAAP_ANNOUNCE_TIMER_GRANULARITY) < 0)
		goto err_timer_announce;

	os_log(LOG_DEBUG, "On port(%u), timers done\n", port->port_id);

	return 0;

err_timer_announce:
	timer_destroy(&range->sm.probe_timer);

err_timer_probe:
	os_log(LOG_ERR, "On port(%u), timers failed\n", port->port_id);

	return -1;
}

/**
 * Create a new range of MAC addresses and its associated state machine
 * \return a pointer to the range created on success, NULL value on failure
 * \param maap, pointer to the MAAP context
 * \param port, the port where the range is allocated
 * \param range_id, chosen range id
 * \param addr, a chosen initial MAC address if needed, NULL otherwise
 * \param n_addr, number of addresses requested for the range
 */
static struct maap_range *maap_range_alloc(struct maap_ctx *maap, struct maap_port *port, u32 range_id, const u8 *addr, unsigned int n_addr)
{
	struct maap_range *range;

	if (n_addr > MAAP_DYNAMIC_POOL_SIZE || n_addr <= 0)
		goto err_alloc;

	if (addr && !maap_is_in_dynamic_pool(addr, n_addr))
		goto err_alloc;

	if (port->allocated_ranges_count >= MAAP_PORT_MAX_RANGES_ALLOCATION)
		goto err_alloc;

	if (is_range_id_used(maap, port, range_id))
		goto err_alloc;

	range = os_malloc(sizeof(struct maap_range));
	if (!range)
		goto err_alloc;

	list_head_init(&range->list);

	/* Keep port information where the range is allocated */
	range->port = port;

	/* Init MAC address range. */
	if (!addr) {
		/* Generate the range address on init */
		maap_generate_address(range, n_addr);
	} else {
		os_memcpy(range->start_mac_addr, addr, sizeof(u8) * 6);
	}

	range->mac_count = n_addr;

	range->range_id = range_id;

	/* Check for internal conflict inside the same port */
	if (check_internal_conflict(port, range))
		goto err_init;

	/* Init range's state machine */
	range->sm.state = MAAP_STATE_INITIAL;
	range->sm.action = MAAP_ACTION_NONE;
	range->sm.maap_probe_count = MAAP_PROBE_RETRANSMITS_VAL;
	range->sm.send_conflict_indication = false;

	/* Init state machine's timers */
	if (maap_timers_init(range, port, maap->timer_ctx) < 0)
		goto err_init;

	port->allocated_ranges_count++;

	os_log(LOG_DEBUG, "On port(%u), init range(%02x:%02x:%02x:%02x:%02x:%02x, %u) done\n",
			port->port_id, range->start_mac_addr[0], range->start_mac_addr[1], range->start_mac_addr[2],
			range->start_mac_addr[3], range->start_mac_addr[4], range->start_mac_addr[5], range->mac_count);

	return range;

err_init:
	os_free(range);

err_alloc:
	return NULL;
}

__init static int maap_port_ipc_init(struct maap_port *port, unsigned long priv)
{
	struct maap_ctx *maap = container_of(port, struct maap_ctx, port[port->port_id]);

	if (maap->management_enabled) {
		if (ipc_rx_init(&port->ipc_rx_mac_service, (port->port_id == CFG_ENDPOINT_0_LOGICAL_PORT ) ? IPC_MAC_SERVICE_MEDIA_STACK : IPC_MAC_SERVICE_1_MEDIA_STACK,
					maap_ipc_rx_mac_service, priv) < 0)
			goto err_ipc_rx_mac_service;

		if (ipc_tx_init(&port->ipc_tx_mac_service, (port->port_id == CFG_ENDPOINT_0_LOGICAL_PORT ) ? IPC_MEDIA_STACK_MAC_SERVICE : IPC_MEDIA_STACK_MAC_SERVICE_1) < 0)
			goto err_ipc_tx_mac_service;

		if (ipc_tx_connect(&port->ipc_tx_mac_service, &port->ipc_rx_mac_service) < 0)
			goto err_ipc_tx_mac_connect;
	}

	return 0;

err_ipc_tx_mac_connect:
	if (maap->management_enabled)
		ipc_tx_exit(&port->ipc_tx_mac_service);
err_ipc_tx_mac_service:
	if (maap->management_enabled)
		ipc_rx_exit(&port->ipc_rx_mac_service);
err_ipc_rx_mac_service:
	return -1;
}

/**
 * Initialize all MAAP ports (NET channel and linked list of ranges) of a MAAP context
 * \return	0 on success, on fail port are set as not initialized
 * \param maap pointer to the MAAP context
 * \param cfg, maap's configuration
 * \param priv int handler
 */
__init static int maap_ports_init(struct maap_ctx *maap, struct maap_config *cfg, unsigned long priv)
{
	unsigned int i;

	for (i = 0; i < maap->port_max; i++) {
		struct net_address addr;
		unsigned int logical_port = cfg->logical_port_list[i];

		maap->port[i].port_id = i;
		maap->port[i].logical_port = logical_port;
		maap->port[i].initialized = false;
		maap->port[i].allocated_ranges_count = 0;

		addr.ptype = PTYPE_AVTP;
		addr.port = logical_port;
		addr.priority = AVDECC_DEFAULT_PRIORITY;
		addr.u.avtp.subtype = AVTP_SUBTYPE_MAAP;

		if (net_get_local_addr(i, maap->port[i].local_physical_mac) < 0) {
			os_log(LOG_ERR, "MAAP could not get local physical mac address on port(%u)\n", i);

			continue;
		}

		/* Init port's NET channel */
		if (net_rx_init(&maap->port[i].net_rx, &addr, maap_net_receive, priv) < 0)
			goto err_net_rx_init;

		if (net_tx_init(&maap->port[i].net_tx, &addr) < 0)
			goto err_net_tx_init;

		if (net_add_multi(&maap->port[i].net_rx, logical_port, maap_multicast_addr) < 0)
			goto err_port_multi;

		if (maap_port_ipc_init(&maap->port[i], priv) < 0)
			goto err_ipc_init;

		/* Init port's MAC address range list */
		list_head_init(&maap->port[i].range);

		maap->port[i].initialized = true;

		os_log(LOG_INIT, "maap(%p) - port(%u)(%p) / max %d done\n", maap, maap->port[i].port_id, &maap->port[i], maap->port_max);

		continue;

	err_ipc_init:
		net_del_multi(&maap->port[i].net_rx, logical_port, maap_multicast_addr);
	err_port_multi:
		net_tx_exit(&maap->port[i].net_tx);

	err_net_tx_init:
		net_rx_exit(&maap->port[i].net_rx);

	err_net_rx_init:
		continue;
	}

	return 0;
}

/**
 * Initialize MAAP context ipc channel
 * \return	0 on success, -1 otherwise
 * \param maap pointer to the MAAP context
 * \param priv int handler
 */
__init static int maap_ipc_init(struct maap_ctx *maap, unsigned long priv)
{
	if (ipc_rx_init(&maap->ipc_rx, IPC_MEDIA_STACK_MAAP, maap_ipc_receive, priv) < 0)
		goto err_ipc_rx;

	if (ipc_tx_init(&maap->ipc_tx, IPC_MAAP_MEDIA_STACK) < 0)
		goto err_ipc_tx;

	if (ipc_tx_init(&maap->ipc_tx_sync, IPC_MAAP_MEDIA_STACK_SYNC) < 0)
		goto err_ipc_tx_sync;

	os_log(LOG_INIT, "done\n");

	return 0;

err_ipc_tx_sync:
	ipc_tx_exit(&maap->ipc_tx);

err_ipc_tx:
	ipc_rx_exit(&maap->ipc_rx);

err_ipc_rx:
	return -1;
}
/* _INIT_ */

/* _EXIT_ */
/**
 * MAAP range's clean-up and destroys its timers
 * \return 0 on success, negative value on failure
 * \param maap_range	pointer to the MAAP range
 */
__exit static int maap_range_free(struct maap_range *range)
{
	timer_destroy(&range->sm.probe_timer);
	timer_destroy(&range->sm.announce_timer);

	range->port->allocated_ranges_count--;

	os_memset(range, 0, sizeof(*range));

	os_free(range);

	os_log(LOG_INIT, "done\n");

	return 0;
}

__exit static void maap_port_ipc_exit(struct maap_port *port)
{
	struct maap_ctx *maap = container_of(port, struct maap_ctx, port[port->port_id]);

	if (maap->management_enabled) {
		ipc_tx_exit(&port->ipc_tx_mac_service);
		ipc_rx_exit(&port->ipc_rx_mac_service);
	}
}

/**
 * Destroy MAAP port's NET channel and ranges
 * \return none
 * \param maap, pointer to the MAAP context
 */
__exit static void maap_ports_exit(struct maap_ctx *maap)
{
	unsigned int i;

	for (i = 0; i < maap->port_max; i++) {
		if (!maap->port[i].initialized)
			continue;

		maap_port_ipc_exit(&maap->port[i]);

		/* Exit NET channel on port i */
		if (net_del_multi(&maap->port[i].net_rx, maap_port_to_logical(maap, maap->port[i].port_id), maap_multicast_addr) < 0)
			os_log(LOG_ERR, "port(%u) cannot remove maap multicast\n", maap->port[i].port_id);

		net_tx_exit(&maap->port[i].net_tx);
		net_rx_exit(&maap->port[i].net_rx);

		/* Exit ranges on port i */
		maap_remove_all_range_on_port(maap, &maap->port[i]);

		maap->port[i].initialized = false;
	}

	os_log(LOG_INIT, "done\n");
}

/**
 * Destroy MAAP context's ipc channel
 * \return none
 * \param maap, pointer to the MAAP context
 */
__exit static void maap_ipc_exit(struct maap_ctx *maap)
{
	ipc_tx_exit(&maap->ipc_tx);
	ipc_rx_exit(&maap->ipc_rx);
	ipc_tx_exit(&maap->ipc_tx_sync);

	os_log(LOG_INIT, "done\n");
}
/* _EXIT_ */

/* _INTERFACE_ */
/**
 * Entry point for external use (AVDEEC).
 * Allows to start the process of attributing a MAC address range to a port and start the corresponding state machine
 * \return	the range created on success, NULL otherwise
 * \param maap	pointer to the MAAP context
 * \param port, pointer to the port
 * \param range_id, chosen id for the range
 * \param addr, a chosen initial MAC address if needed otherwise NULL
 * \param n_addr, number of MAC addresses requested for the range
 */
struct maap_range *maap_new_range_on_port(struct maap_ctx *maap, struct maap_port *port, u32 range_id, const u8 *addr, unsigned int n_addr)
{
	struct maap_range *range;

	range = maap_range_alloc(maap, port, range_id, addr, n_addr);
	if (!range)
		goto err_range_alloc;

	/* Add range to the linked list of MAC address range of the port */
	list_add(&port->range, &range->list);

	/* Start the range's state machine */
	maap_sm(range, port, MAAP_EVENT_BEGIN, NULL);

	os_log(LOG_INFO, "maap(%p) success range(%u, %02x:%02x:%02x:%02x:%02x:%02x, %u) on port(%u) created\n", maap, range->range_id,
		range->start_mac_addr[0], range->start_mac_addr[1], range->start_mac_addr[2],
		range->start_mac_addr[3], range->start_mac_addr[4], range->start_mac_addr[5], range->mac_count, port->port_id);

	return range;

err_range_alloc:
	os_log(LOG_ERR, "maap(%p) failed to create range of count(%u) on port(%u)\n", maap, n_addr, port->port_id);

	return NULL;
}

/**
 * Entry point for external use (AVDEEC).
 * Allows to remove all ranges attributed on a port
 * \return	0 on success, -1 otherwise
 * \param maap	pointer to the MAAP context
 * \param port, pointer to the port
 */
int maap_remove_all_range_on_port(struct maap_ctx *maap, struct maap_port *port)
{
	struct list_head *entry, *entry_next;
	struct maap_range *range;

	/* Exit ranges */
	for (entry = list_first(&port->range); entry_next = list_next(entry), entry != &port->range; entry = entry_next) {

		range = container_of(entry, struct maap_range, list);

		list_del(entry);
		maap_range_free(range);
	}

	return 0;
}

/**
 * Entry point for external use (AVDEEC).
 * Allows to remove a ranges attributed on a port by its range_id
 * \return	0 on success, -1 otherwise
 * \param maap	pointer to the MAAP context
 * \param port, pointer to the port
 * \param range_id, id of the range to delete
 * \param addr, OUTPUT, pointer to an u8 array to save the first address of the deleted range to send an indication
 * \param count, OUTPUT, pointer to an unsigned int to save the number of addresses in the deleted range to send an indication
 */
int maap_remove_range_on_port(struct maap_ctx *maap, struct maap_port *port, u32 range_id, u8 *addr, unsigned int *count)
{
	struct list_head *entry, *entry_next;
	struct maap_range *range;

	/* Exit ranges */
	for (entry = list_first(&port->range); entry_next = list_next(entry), entry != &port->range; entry = entry_next) {

		range = container_of(entry, struct maap_range, list);

		if (range->range_id == range_id) {
			os_memcpy(addr, range->start_mac_addr, sizeof(u8) * 6);
			*count = range->mac_count;

			list_del(entry);
			maap_range_free(range);

			os_log(LOG_INFO, "maap(%p) success range(%02x:%02x:%02x:%02x:%02x:%02x, %u) on port(%u) freed\n", maap,
				addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], *count, port->port_id);

			return 0;
		}
	}

	os_log(LOG_ERR, "maap(%p) failed to free range(%u) on port(%u)\n", maap, range_id, port->port_id);

	return -1;
}
/* _INTERFACE_ */

/**
 * Initialize the MAAP context
 * \return	maap (the context) on success, NULL value on failure
 * \param cfg, MAAP config
 * \param priv, handler
 */
__init void *maap_init(struct maap_config *cfg, unsigned long priv)
{
	struct maap_ctx *maap;
	unsigned int n_timers;

	n_timers = MAAP_NUMBER_OF_TIMERS;

	log_level_set(maap_COMPONENT_ID, cfg->log_level);

	maap = maap_alloc(cfg->port_max, n_timers);
	if (!maap)
		goto err_malloc;

	maap->management_enabled = cfg->management_enabled;

	/* Init timer pool, timer_ctx */
	if (timer_pool_init(maap->timer_ctx, n_timers, priv) < 0)
		goto err_ctx_timers;

	if (maap_ports_init(maap, cfg, priv) < 0)
		goto err_ports;

	if (maap_ipc_init(maap, priv) < 0)
		goto err_ipc;

	os_log(LOG_INIT, "maap(%p) done\n", maap);

	return maap;

err_ipc:
	maap_ports_exit(maap);

err_ports:
	timer_pool_exit(maap->timer_ctx);

err_ctx_timers:
	os_free(maap);

err_malloc:
	return NULL;
}

/**
 * MAAP context level clean-up
 * \return	0 on success, negative value on failure
 * \param maap_ctx	pointer to the MAAP context
 */
__exit int maap_exit(void *maap_ctx)
{
	struct maap_ctx *maap = maap_ctx;

	maap_ipc_exit(maap);

	maap_ports_exit(maap);

	timer_pool_exit(maap->timer_ctx);

	os_free(maap);

	os_log(LOG_INIT, "done\n");

	return 0;
}
