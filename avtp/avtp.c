/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2019, 2021-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief AVTP common code
 @details Common AVTP code entry points
*/

#include "os/stdlib.h"
#include "os/clock.h"

#include "common/log.h"
#include "common/net.h"
#include "common/avtp.h"

#include "avtp.h"
#include "stream.h"
#include "media_clock.h"

/* FIXME add port information to clock domain, grids, ... structures and remove the global variable */
static struct avtp_ctx *_avtp;

static struct avtp_port *logical_to_avtp_port(struct avtp_ctx *avtp, unsigned int port)
{
	int i;

	for (i = 0; i < avtp->port_max; i++)
		if (port == avtp->port[i].logical_port)
			return &avtp->port[i];

	return NULL;
}

unsigned int avtp_to_logical_port(unsigned int port_id)
{
	return _avtp->port[port_id].logical_port;
}

unsigned int avtp_to_clock(unsigned int port_id)
{
	return _avtp->port[port_id].clock_gptp;
}

__init static struct avtp_ctx *avtp_alloc(unsigned int ports, unsigned int timer_n)
{
	struct avtp_ctx *avtp;
	unsigned int size;

	size = sizeof(struct avtp_ctx) + ports * sizeof(struct avtp_port);
	size += timer_pool_size(timer_n);

	avtp = os_malloc(size);
	if (!avtp)
		goto err;

	os_memset(avtp, 0, size);

	avtp->port_max = ports;

	avtp->timer_ctx = (struct timer_ctx *)((u8 *)avtp + sizeof(struct avtp_ctx) + ports * sizeof(struct avtp_port));

	return avtp;

err:
	return NULL;
}

/** Initializes avtp global context
 *
 * Called from avtp platform dependent code.
 *
 * \return 0 on success, -1 in case of error
 * \param avtp pointer to avtp global context
 * \param cfg pointer to avtp configuration
 * \param priv platform dependent code private data
 */
__init void *avtp_init(struct avtp_config *cfg, unsigned long priv)
{
	struct avtp_ctx *avtp;
	struct avtp_port *port;
	unsigned int timer_n = CFG_AVTP_MAX_TIMERS;
	int i;

	avtp = avtp_alloc(cfg->port_max, timer_n);
	if (!avtp)
		goto err_malloc;

	log_level_set(avtp_COMPONENT_ID, cfg->log_level);

	_avtp = avtp;

	if (ipc_tx_init(&avtp->ipc_tx_stats, IPC_AVTP_STATS) < 0)
		goto err_ipc_tx_stats;

	if (ipc_rx_init_no_notify(&avtp->ipc_rx_media_stack, IPC_MEDIA_STACK_AVTP) < 0)
		goto err_ipc_rx_media_stack;

	if (ipc_tx_init(&avtp->ipc_tx_media_stack, IPC_AVTP_MEDIA_STACK) < 0)
		goto err_ipc_tx_media_stack;

	if (ipc_rx_init_no_notify(&avtp->ipc_rx_clock_domain, IPC_MEDIA_STACK_CLOCK_DOMAIN) < 0)
		goto err_ipc_rx_clock_domain;

	if (ipc_tx_init(&avtp->ipc_tx_clock_domain, IPC_CLOCK_DOMAIN_MEDIA_STACK) < 0)
		goto err_ipc_tx_clock_domain;

	if (ipc_tx_init(&avtp->ipc_tx_clock_domain_sync, IPC_CLOCK_DOMAIN_MEDIA_STACK_SYNC) < 0)
		goto err_ipc_tx_clock_domain_sync;

	if (ipc_rx_init_no_notify(&avtp->ipc_rx_avdecc, IPC_AVDECC_AVTP) < 0)
		goto err_ipc_rx_avdecc;

	if (ipc_tx_init(&avtp->ipc_tx_avdecc, IPC_AVTP_AVDECC) < 0)
		goto err_ipc_tx_avdecc;

	if (timer_pool_init(avtp->timer_ctx, timer_n, priv) < 0)
		goto err_timer_pool_init;

	for (i = 0; i < avtp->port_max; i++) {
		port = &avtp->port[i];

		port->logical_port = cfg->logical_port_list[i];

		port->clock_gptp = cfg->clock_gptp_list[i];

		list_head_init(&port->talker);
		list_head_init(&port->listener);

		clock_source_init(&port->ptp_source, GRID_PRODUCER_PTP, i, priv);
	}

	list_head_init(&avtp->stream_destroyed);

	for (i = 0; i < AVTP_CFG_NUM_DOMAINS; i++)
		clock_domain_init(&avtp->domain[i], i, priv);

	avtp->priv = priv;

	avtp->stream_listener_count = 0;
	avtp->stream_talker_count = 0;

	os_log(LOG_INIT, "avtp(%p) done\n", avtp);

	return avtp;

err_timer_pool_init:
	ipc_tx_exit(&avtp->ipc_tx_avdecc);

err_ipc_tx_avdecc:
	ipc_rx_exit(&avtp->ipc_rx_avdecc);

err_ipc_rx_avdecc:
	ipc_tx_exit(&avtp->ipc_tx_clock_domain_sync);

err_ipc_tx_clock_domain_sync:
	ipc_tx_exit(&avtp->ipc_tx_clock_domain);

err_ipc_tx_clock_domain:
	ipc_rx_exit(&avtp->ipc_rx_clock_domain);

err_ipc_rx_clock_domain:
	ipc_tx_exit(&avtp->ipc_tx_media_stack);

err_ipc_tx_media_stack:
	ipc_rx_exit(&avtp->ipc_rx_media_stack);

err_ipc_rx_media_stack:
	ipc_tx_exit(&avtp->ipc_tx_stats);

err_ipc_tx_stats:
	os_free(avtp);

err_malloc:
	return NULL;
}

/** Cleans up avtp global context
 *
 * Called from avtp platform dependent code.
 *
 * \return 0 on success, -1 in case of error
 * \param avtp pointer to avtp global context
 */
int avtp_exit(void *avtp_ctx)
{
	struct avtp_ctx *avtp = (struct avtp_ctx *)avtp_ctx;
	struct avtp_port *port;
	struct list_head *entry, *next;
	struct stream_listener *stream;
	int i;

	os_log(LOG_INIT, "avtp(%p)\n", avtp);

	/* Destroy all current streams */
	for (i = 0; i < avtp->port_max; i++) {
		port = &avtp->port[i];

		for (entry = list_first(&port->listener); next = list_next(entry), entry != &port->listener; entry = next) {
			stream = container_of(entry, struct stream_listener, common.list);
			stream_destroy(stream, &avtp->ipc_tx_stats);
		}

		for (entry = list_first(&port->talker); next = list_next(entry), entry != &port->talker; entry = next) {
			struct stream_talker *stream = container_of(entry, struct stream_talker, common.list);
			stream_destroy(stream, &avtp->ipc_tx_stats);
		}

		clock_source_exit(&port->ptp_source);
	}

	stream_free_all(avtp);

	for (i = 0; i < AVTP_CFG_NUM_DOMAINS; i++)
		clock_domain_exit(&avtp->domain[i]);

	timer_pool_exit(avtp->timer_ctx);

	ipc_tx_exit(&avtp->ipc_tx_clock_domain_sync);

	ipc_tx_exit(&avtp->ipc_tx_clock_domain);

	ipc_rx_exit(&avtp->ipc_rx_clock_domain);

	ipc_tx_exit(&avtp->ipc_tx_media_stack);

	ipc_rx_exit(&avtp->ipc_rx_media_stack);

	ipc_tx_exit(&avtp->ipc_tx_stats);

	ipc_tx_exit(&avtp->ipc_tx_avdecc);

	ipc_rx_exit(&avtp->ipc_rx_avdecc);

	os_free(avtp);

	os_log(LOG_INIT, "done\n");

	return 0;
}

static void process_stats_print(struct ipc_avtp_process_stats *msg)
{
	struct process_stats *stats = &msg->stats;

	stats_compute(&stats->events);
	stats_compute(&stats->sched_intvl);
	stats_compute(&stats->processing_time);

	os_log(LOG_INFO, "events %2d/%2d/%2d/%2"PRIu64"    sched_intvl % 10d/% 10d/% 10d processing time % 10d/% 10d/% 10d (ns)\n",
			stats->events.min, stats->events.mean, stats->events.max, stats->events.variance,
			stats->sched_intvl.min, stats->sched_intvl.mean, stats->sched_intvl.max,
			stats->processing_time.min, stats->processing_time.mean, stats->processing_time.max);
}


void stats_ipc_rx(struct ipc_rx const *rx, struct ipc_desc *desc)
{
	switch (desc->type) {
	case IPC_AVTP_PROCESS_STATS:
		process_stats_print((struct ipc_avtp_process_stats *)&desc->u);
		break;

	case IPC_AVTP_STREAM_TALKER_STATS:
		stream_talker_stats_print((struct ipc_avtp_talker_stats *)&desc->u);
		break;

	case IPC_AVTP_STREAM_LISTENER_STATS:
		stream_listener_stats_print((struct ipc_avtp_listener_stats *)&desc->u);
		break;

	case IPC_AVTP_CLOCK_DOMAIN_STATS:
		clock_domain_stats_print((struct ipc_avtp_clock_domain_stats *)&desc->u);
		break;

	case IPC_AVTP_CLOCK_GRID_STATS:
		clock_grid_stats_print((struct ipc_avtp_clock_grid_stats *)&desc->u);
		break;

	case IPC_AVTP_CLOCK_GRID_CONSUMER_STATS:
		clock_grid_consumer_stats_print((struct ipc_avtp_clock_grid_consumer_stats *)&desc->u);
		break;

	default:
		break;
	}

	ipc_free(rx, desc);
}

static void avtp_get_counters(struct avtp_ctx *avtp)
{
	struct ipc_avtp_listener_counters *listener_counters;
	struct ipc_avtp_talker_counters *talker_counters;
	struct ipc_tx *tx = &avtp->ipc_tx_avdecc;
	struct list_head *entry, *next;
	struct ipc_avtp_counters *msg;
	struct avtp_port *port;
	struct ipc_desc *desc;
	unsigned int size;
	u8 *msg_counters;
	int i;

	size = sizeof(struct ipc_avtp_counters) +
		sizeof(struct ipc_avtp_listener_counters) * avtp->stream_listener_count +
		sizeof(struct ipc_avtp_talker_counters) * avtp->stream_talker_count;

	desc = ipc_alloc(tx, size);
	if (!desc)
		goto err_ipc_alloc;

	desc->dst = IPC_DST_ALL;
	desc->type = IPC_AVTP_COUNTERS;
	desc->len = size;
	desc->flags = 0;

	msg = (struct ipc_avtp_counters *)&desc->u;

	msg->stream_listener_count = avtp->stream_listener_count;
	msg->stream_talker_count = avtp->stream_talker_count;

	msg_counters = msg->data;

	for (i = 0; i < avtp->port_max; i++) {
		port = &avtp->port[i];

		for (entry = list_first(&port->listener); next = list_next(entry), entry != &port->listener; entry = next) {
			struct stream_listener *stream = container_of(entry, struct stream_listener, common.list);

			listener_counters = (struct ipc_avtp_listener_counters *)msg_counters;

			copy_64(&listener_counters->stream_id, &stream->id);

			listener_counters->counters.rx = stream->stats.rx;
			listener_counters->counters.pkt_lost = stream->stats.pkt_lost;
			listener_counters->counters.mr = stream->stats.mr;
			listener_counters->counters.tu = stream->stats.tu;
			listener_counters->counters.unsupported_format = stream->stats.format_err + stream->stats.subtype_err;
			listener_counters->counters.early_timestamp = stream->stats.early_timestamp;
			listener_counters->counters.late_timestamp = stream->stats.late_timestamp;

			/* Next listener stream */
			msg_counters += sizeof(struct ipc_avtp_listener_counters);
		}
	}

	for (i = 0; i < avtp->port_max; i++) {
		port = &avtp->port[i];

		for (entry = list_first(&port->talker); next = list_next(entry), entry != &port->talker; entry = next) {
			struct stream_talker *stream = container_of(entry, struct stream_talker, common.list);

			talker_counters = (struct ipc_avtp_talker_counters *)msg_counters;

			copy_64(&talker_counters->stream_id, &stream->id);

			talker_counters->counters.tx = stream->stats.tx;
			talker_counters->counters.mr = stream->stats.mr;

			/* Next talker stream */
			msg_counters += sizeof(struct ipc_avtp_talker_counters);
		}
	}

	if (ipc_tx(tx, desc) < 0)
		goto err_ipc_tx;

	return;

err_ipc_tx:
	ipc_free(tx, desc);

err_ipc_alloc:
	return;
}

/** AVTP IPC receive callback.
 * IPC received from AVDECC
 * \return 	none
 * \param 	rx 	pointer to IPC receive context
 * \param 	desc 	pointer to IPC descriptor
 */
void avtp_ipc_rx_avdecc(struct ipc_rx const *rx, struct ipc_desc *desc)
{
	struct avtp_ctx *avtp = container_of(rx, struct avtp_ctx, ipc_rx_avdecc);

	switch (desc->type) {
	case IPC_AVTP_GET_COUNTERS:
		avtp_get_counters(avtp);
		break;

	default:
		break;
	}

	ipc_free(rx, desc);

	return;
}

static void process_stats_dump(struct avtp_ctx *avtp, struct process_stats *stats)
{
	struct ipc_tx *tx = &avtp->ipc_tx_stats;
	struct ipc_desc *desc;
	struct ipc_avtp_process_stats *msg;

	desc = ipc_alloc(tx, sizeof(*msg));
	if (!desc)
		goto err_ipc_alloc;

	desc->type = IPC_AVTP_PROCESS_STATS;
	desc->len = sizeof(*msg);
	desc->flags = 0;

	msg = (struct ipc_avtp_process_stats *)&desc->u;

	os_memcpy(&msg->stats, stats, sizeof(*stats));

	if (ipc_tx(tx, desc) < 0)
		goto err_ipc_tx;

	stats_reset(&stats->events);
	stats_reset(&stats->sched_intvl);
	stats_reset(&stats->processing_time);

	return;

err_ipc_tx:
	ipc_free(tx, desc);

err_ipc_alloc:
	return;
}


void avtp_media_event(void *data)
{
	struct media_rx *media = (struct media_rx *)data;
	struct stream_talker *stream = container_of(media, struct stream_talker, media);

	stream->net_tx(stream);
}

void avtp_net_tx_event(void *data)
{
	struct net_tx *tx = (struct net_tx *)data;
	struct stream_talker *stream = container_of(tx, struct stream_talker, tx);

	stream->net_tx(stream);
}

void avtp_stats_dump(void *avtp_ctx, struct process_stats *stats)
{
	struct avtp_ctx *avtp = (struct avtp_ctx *)avtp_ctx;
	struct avtp_port *port;
	int i;

	process_stats_dump(avtp, stats);

	for (i = 0; i < AVTP_CFG_NUM_DOMAINS; i++)
		clock_domain_stats_dump(&avtp->domain[i], &avtp->ipc_tx_stats);

	for (i = 0; i < avtp->port_max; i++) {
		port = &avtp->port[i];
		stream_stats_dump(port, &avtp->ipc_tx_stats);
	}
}

void avtp_alternative_header_init(struct avtp_alternative_hdr *avtp_alt, u8 subtype, u8 h)
{
	avtp_alt->subtype = subtype;
	avtp_alt->h = h;
	avtp_alt->version = AVTP_VERSION_0;
}

unsigned int avtp_data_header_init(struct avtp_data_hdr *avtp_data, u8 subtype, void *stream_id)
{
	avtp_data->subtype = subtype;
	avtp_data->sv = 1;
	avtp_data->version = AVTP_VERSION_0;
	avtp_data->mr = 0;
#ifdef CFG_AVTP_1722A
	avtp_data->f_s_d = 0;
#else
	avtp_data->r = 0;
	avtp_data->gv = 0;
#endif
	avtp_data->tv = 0;
	avtp_data->sequence_num = 0;
#ifdef CFG_AVTP_1722A
	avtp_data->format_specific_data_1 = 0;
#else
	avtp_data->reserved = 0;
#endif
	avtp_data->tu = 0;
	copy_64(&avtp_data->stream_id, stream_id);
#ifdef CFG_AVTP_1722A
	avtp_data->format_specific_data_2 = 0;
#else
	avtp_data->gateway_info = 0;
#endif

	return sizeof(struct avtp_data_hdr);
}

/** AVTP alternative format receive descriptor flush
 *
 * Flushes received avtp descriptors to the upper protocol layer
 *
 * \return none
 * \param stream	pointer to stream context
 * \param desc		array of avtp receive descriptors
 * \param desc_n	descriptor array length
 */
static inline void avtp_alternative_desc_flush(struct stream_listener *stream, struct avtp_rx_desc **desc, unsigned int *desc_n)
{
	if (*desc_n) {
		stream->net_rx(stream, desc, *desc_n);

		*desc_n = 0;
	}
}


/** AVTP stream format receive descriptor flush
 *
 * Flushes received avtp descriptors to the upper protocol layer, or frees them in case of error.
 *
 * \return none
 * \param stream	pointer to stream context
 * \param desc		array of avtp receive descriptors
 * \param desc_n	descriptor array length
 * \param ts		array of avtp timestamps
 * \param ts_n		timestamp array length
 */
static inline void avtp_stream_desc_flush(struct stream_listener *stream, struct avtp_rx_desc **desc, unsigned int *desc_n,
				   struct timestamp *ts, unsigned *ts_n)
{
	/* Flush timestamps if clock recovery is enabled and array is not empty */
	if (stream->source) {
		if (*ts_n) {
			stream->stats.clock_tx += *ts_n;
			clock_producer_stream_rx(&stream->source->grid, ts, ts_n, 0);
			*ts_n = 0;
		}
	}

	if (*desc_n) {
		stream->net_rx(stream, desc, *desc_n);

		*desc_n = 0;
	}
}

void avtp_alternative_net_rx(struct net_rx *net_rx, struct net_rx_desc **desc, unsigned int n)
{
	struct stream_listener *stream = container_of(net_rx, struct stream_listener, rx);
	struct avtp_rx_desc *avtp_desc, **avtp_desc_first = NULL;
	struct avtp_alternative_hdr *hdr;
	unsigned int desc_n;
	int i;

	if (os_clock_gettime32(stream->clock_gptp, &stream->gptp_current) < 0)
		stream->stats.gptp_err++;

	stats_update(&stream->stats.batch, n);

	stream->stats.rx += n;

	for (i = 0, desc_n = 0; i < n; i++) {

		avtp_desc = (struct avtp_rx_desc *)desc[i];

		hdr = (struct avtp_alternative_hdr *)((char *)desc[i] + desc[i]->l3_offset);

		avtp_desc->flags = 0;

		if (unlikely(hdr->subtype != stream->subtype)) {
			stream->stats.subtype_err++;

			avtp_alternative_desc_flush(stream, avtp_desc_first, &desc_n);

			net_rx_free(&avtp_desc->desc);

			/* skip this descriptor */
			continue;
		}

		if (!desc_n)
			avtp_desc_first = (struct avtp_rx_desc **)&desc[i];

		desc_n++;
	}

	avtp_alternative_desc_flush(stream, avtp_desc_first, &desc_n);
}

/** AVTP stream network receive callback
 *
 * Parses and validates AVTP stream headers, extracting timestamp information, for a batch of descriptors.
 * Valid packets are sent to the upper protocol layer (in one or more batches).
 * The function takes ownership of the received descriptors and is responsible for freeing them.
 *
 * \return none
 * \param net_rx pointer to network receive context
 * \param desc array of network receive descriptors
 * \param n array length
 */
void avtp_stream_net_rx(struct net_rx *net_rx, struct net_rx_desc **desc, unsigned int n)
{
	struct stream_listener *stream = container_of(net_rx, struct stream_listener, rx);
	struct avtp_rx_desc *avtp_desc, **avtp_desc_first = NULL;
	struct avtp_data_hdr *hdr;
	int i = 0;
	unsigned int desc_n, ts_n;
	struct timestamp ts[NET_RX_BATCH];

	if (os_clock_gettime32(stream->clock_gptp, &stream->gptp_current) < 0)
		stream->stats.gptp_err++;

	stats_update(&stream->stats.batch, n);

	stream->stats.rx += n;

	for (i = 0, desc_n = 0, ts_n = 0; i < n; i++) {

		os_log(LOG_DEBUG, "port %d, ethertype %x, len %d, timestamp %u\n", desc[i]->port, desc[i]->ethertype, desc[i]->len, desc[i]->ts);

		avtp_desc = (struct avtp_rx_desc *)desc[i];

		hdr = (struct avtp_data_hdr *)((char *)desc[i] + desc[i]->l3_offset);

		avtp_desc->flags = 0;

		if (unlikely(hdr->subtype != stream->subtype)) {
			stream->stats.subtype_err++;

			avtp_stream_desc_flush(stream, avtp_desc_first, &desc_n, ts, &ts_n);

			net_rx_free(&avtp_desc->desc);

			/* skip this descriptor */
			continue;
		}

		if (likely(stream->pkt_received)) {
			if (unlikely(hdr->sequence_num != ((stream->sequence_num + 1) & 0xff))) {
				avtp_stream_desc_flush(stream, avtp_desc_first, &desc_n, ts, &ts_n);

				avtp_desc->flags |= AVTP_PACKET_LOST;
				stream->stats.pkt_lost++;
			}
		}

		if (unlikely(hdr->mr != stream->mr)) {
			avtp_stream_desc_flush(stream, avtp_desc_first, &desc_n, ts, &ts_n);

			avtp_desc->flags |= AVTP_MEDIA_CLOCK_RESTART;
			stream->mr = hdr->mr;
			stream->stats.mr++;
		}

		stream->sequence_num = hdr->sequence_num;

		stream->pkt_received++;

#ifdef CFG_AVTP_1722A
		avtp_desc->format_specific_data_2 = hdr->format_specific_data_2;
#endif

		if (unlikely(hdr->tu)) {
			avtp_desc->flags |= AVTP_TIMESTAMP_UNCERTAIN;
			stream->stats.tu++;
		}

		if (likely(hdr->tv)) {
			avtp_desc->avtp_timestamp = ntohl(hdr->avtp_timestamp);

			if (stream->source) {
				ts[ts_n].ts_nsec = avtp_desc->avtp_timestamp;
				ts[ts_n].flags = avtp_desc->flags;
				ts_n++;
			}
		} else {
			avtp_desc->flags |= AVTP_TIMESTAMP_INVALID;
		}

		avtp_desc->protocol_specific_header = hdr->protocol_specific_header;

		avtp_desc->l4_len = ntohs(hdr->stream_data_length);

		avtp_desc->l4_offset = desc[i]->l3_offset + sizeof(struct avtp_data_hdr);

		if (!desc_n)
			avtp_desc_first = (struct avtp_rx_desc **)&desc[i];

		desc_n++;
	}

	avtp_stream_desc_flush(stream, avtp_desc_first, &desc_n, ts, &ts_n);
}

static void avtp_ipc_send_listener_connect_response(struct avtp_ctx *avtp, unsigned int ipc_dst, u8 *stream_id, struct stream_listener *stream)
{
	struct ipc_desc *desc;
	struct ipc_avtp_listener_connect_response *response;

	desc = ipc_alloc(&avtp->ipc_tx_media_stack, sizeof(struct ipc_avtp_listener_connect_response));
	if (desc) {
		desc->dst = ipc_dst;
		desc->type = IPC_AVTP_LISTENER_CONNECT_RESPONSE;
		desc->len = sizeof(struct ipc_avtp_listener_connect_response);

		response = &desc->u.avtp_listener_connect_response;

		os_memcpy(&response->stream_id, stream_id, 8);

		if (stream)
			response->status = GENAVB_SUCCESS;
		else
			response->status = GENAVB_ERR_STREAM_PARAMS;

		if (ipc_tx(&avtp->ipc_tx_media_stack, desc) < 0) {
			os_log(LOG_ERR, "avtp(%p) ipc_tx() failed\n", avtp);
			ipc_free(&avtp->ipc_tx_media_stack, desc);
		}
	}
}

static void avtp_ipc_send_talker_connect_response(struct avtp_ctx *avtp, unsigned int ipc_dst, u8 *stream_id, struct stream_talker *stream)
{
	struct ipc_desc *desc;
	struct ipc_avtp_talker_connect_response *response;

	desc = ipc_alloc(&avtp->ipc_tx_media_stack, sizeof(struct ipc_avtp_talker_connect_response));
	if (desc) {
		desc->dst = ipc_dst;
		desc->type = IPC_AVTP_TALKER_CONNECT_RESPONSE;
		desc->len = sizeof(struct ipc_avtp_talker_connect_response);

		response = &desc->u.avtp_talker_connect_response;

		os_memcpy(&response->stream_id, stream_id, 8);

		if (stream) {
			response->status = GENAVB_SUCCESS;
			response->latency = stream->latency;
			response->batch = stream->tx_batch;
			response->max_payload_size = stream->payload_size;
		} else
			response->status = GENAVB_ERR_STREAM_PARAMS;

		if (ipc_tx(&avtp->ipc_tx_media_stack, desc) < 0) {
			os_log(LOG_ERR, "avtp(%p) ipc_tx() failed\n", avtp);
			ipc_free(&avtp->ipc_tx_media_stack, desc);
		}
	}
}

static void avtp_ipc_send_disconnect_response(struct avtp_ctx *avtp, unsigned int ipc_dst, u8 *stream_id, u16 status)
{
	struct ipc_desc *desc;
	struct ipc_avtp_disconnect_response *response;

	desc = ipc_alloc(&avtp->ipc_tx_media_stack, sizeof(struct ipc_avtp_disconnect_response));
	if (desc) {
		desc->dst = ipc_dst;
		desc->type = IPC_AVTP_DISCONNECT_RESPONSE;
		desc->len = sizeof(struct ipc_avtp_disconnect_response);

		response = &desc->u.avtp_disconnect_response;

		os_memcpy(&response->stream_id, stream_id, 8);
		response->status = status;

		if (ipc_tx(&avtp->ipc_tx_media_stack, desc) < 0) {
			os_log(LOG_ERR, "avtp(%p) ipc_tx() failed\n", avtp);
			ipc_free(&avtp->ipc_tx_media_stack, desc);
		}
	}
}

static void avtp_ipc_error_response(struct avtp_ctx *avtp, unsigned int ipc_dst, u32 type, u32 len, u32 status)
{
	struct ipc_desc *desc;
	struct ipc_error_response *error;

	/* Send error response to media stack */
	desc = ipc_alloc(&avtp->ipc_tx_media_stack, sizeof(struct ipc_error_response));
	if (desc) {
		desc->dst = ipc_dst;
		desc->type = GENAVB_MSG_ERROR_RESPONSE;
		desc->len = sizeof(struct ipc_error_response);
		desc->flags = 0;

		error = &desc->u.error;

		error->type = type;
		error->len = len;
		error->status = status;

		if (ipc_tx(&avtp->ipc_tx_media_stack, desc) < 0) {
			os_log(LOG_ERR, "avtp(%p) ipc_tx() failed\n", avtp);
			ipc_free(&avtp->ipc_tx_media_stack, desc);
		}
	} else
		os_log(LOG_ERR, "avtp(%p) ipc_alloc() failed\n", avtp);
}

/** AVTP ipc receive
 *
 * The function takes ownership of the received descriptor and is responsible for freeing it.
 *
 * \return none
 * \param rx pointer to ipc receive context
 * \param desc pointer to ipc descriptor
 */
static void avtp_ipc_rx_media_stack(struct ipc_rx const *rx, struct ipc_desc *desc)
{
	struct avtp_ctx *avtp = container_of(rx, struct avtp_ctx, ipc_rx_media_stack);
	u16 status;
	struct avtp_port *port;

	os_log(LOG_INFO, "\n");

	switch (desc->type) {
	case IPC_AVTP_CONNECT:
		if (desc->len != sizeof(struct ipc_avtp_connect)) {
			os_log(LOG_ERR, "ipc_rx(%p) stream creation failed: corrupt IPC desc(%p)\n", ipc_rx, desc);
			status = GENAVB_ERR_CTRL_LEN;
			goto err;
			break;
		}

		port = logical_to_avtp_port(avtp, desc->u.avtp_connect.port);
		if (!port) {
			os_log(LOG_ERR, "ipc_rx(%p) stream creation failed: unknown port %d (ipc_desc(%p))\n", ipc_rx, desc->u.avtp_connect.port, desc);
			status = GENAVB_ERR_INVALID_PARAMS;
			goto err;
			break;
		}

		switch (desc->u.avtp_connect.direction) {
		case AVTP_DIRECTION_LISTENER: {
			struct stream_listener *stream;

			stream = stream_listener_create(avtp, port, &desc->u.avtp_connect);
			if (!stream)
				os_log(LOG_ERR, "ipc_rx(%p) listener stream creation failed (ipc_desc(%p))\n", ipc_rx, desc);

			avtp_ipc_send_listener_connect_response(avtp, desc->src, desc->u.avtp_connect.stream_id, stream);

			break;
		}

		case AVTP_DIRECTION_TALKER: {
			struct stream_talker *stream;

			stream = stream_talker_create(avtp, port, &desc->u.avtp_connect);
			if (!stream)
				os_log(LOG_ERR, "ipc_rx(%p) talker stream creation failed (ipc_desc(%p))\n", ipc_rx, desc);

			avtp_ipc_send_talker_connect_response(avtp, desc->src, desc->u.avtp_connect.stream_id, stream);

			break;
		}

		default:
			os_log(LOG_ERR, "ipc_rx(%p) stream creation failed: unknown direction %d (ipc_desc(%p))\n", ipc_rx, desc->u.avtp_connect.direction, desc);
			status = GENAVB_ERR_CTRL_INVALID;
			goto err;
			break;
		}

		break;

	case IPC_AVTP_DISCONNECT:
	{
		if (desc->len != sizeof(struct ipc_avtp_disconnect)) {
			status = GENAVB_ERR_CTRL_LEN;
			goto err;
			break;
		}

		port = logical_to_avtp_port(avtp, desc->u.avtp_disconnect.port);
		if (!port) {
			os_log(LOG_ERR, "ipc_rx(%p) stream destruction failed: unknown port %d (ipc_desc(%p)\n", ipc_rx, desc->u.avtp_disconnect.port, desc);
			status = GENAVB_ERR_INVALID_PARAMS;
                        goto err;
			break;
		}

		switch (desc->u.avtp_disconnect.direction) {
		case AVTP_DIRECTION_LISTENER:
		{
			struct stream_listener *stream;

			stream = stream_listener_find(port, &desc->u.avtp_disconnect.stream_id);
			if (stream)
				stream_destroy(stream, &avtp->ipc_tx_stats);

			avtp_ipc_send_disconnect_response(avtp, desc->src, desc->u.avtp_disconnect.stream_id, GENAVB_SUCCESS);

			break;
		}

		case AVTP_DIRECTION_TALKER:
		{
			struct stream_talker *stream;

			stream = stream_talker_find(port, &desc->u.avtp_disconnect.stream_id);
			if (stream)
				stream_destroy(stream, &avtp->ipc_tx_stats);

			avtp_ipc_send_disconnect_response(avtp, desc->src, desc->u.avtp_disconnect.stream_id, GENAVB_SUCCESS);

			break;
		}

		default:
			os_log(LOG_ERR, "ipc_rx(%p) stream destruction failed: unknown direction %d (ipc_desc(%p))\n", ipc_rx, desc->u.avtp_disconnect.direction, desc);
			status = GENAVB_ERR_CTRL_INVALID;
			goto err;

			break;
		}

		break;
	}

	case IPC_HEARTBEAT:
		/* We do not send a response in that case: not getting an error when sending the HEARTBEAT is enough for the
		 * GenAVB library to know the GenAVB stack is up and running, and non-streaming apps will not have an ipc_rx
		 * set up to receive the response anyway.
		 */
		break;

	default:
		os_log(LOG_ERR, "ipc_rx(%p) unknown IPC %d (ipc_desc(%p))\n", ipc_rx, desc->type, desc);
		status = GENAVB_ERR_CTRL_INVALID;
		goto err;

		break;
	}

	ipc_free(rx, desc);

	return;

err:
	avtp_ipc_error_response(avtp, desc->src, desc->type, desc->len, status);

	ipc_free(rx, desc);
}

void avtp_ipc_rx(void *avtp_ctx)
{
	struct avtp_ctx *avtp = (struct avtp_ctx *)avtp_ctx;
	struct ipc_desc *desc;

	desc = __ipc_rx(&avtp->ipc_rx_clock_domain);
	if (desc)
		clock_domain_ipc_rx_media_stack(&avtp->ipc_rx_clock_domain, desc);

	desc = __ipc_rx(&avtp->ipc_rx_media_stack);
	if (desc)
		avtp_ipc_rx_media_stack(&avtp->ipc_rx_media_stack, desc);

	desc = __ipc_rx(&avtp->ipc_rx_avdecc);
	if (desc)
		avtp_ipc_rx_avdecc(&avtp->ipc_rx_avdecc, desc);
}
