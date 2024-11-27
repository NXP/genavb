/*
 * Copyright 2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2019, 2021-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Clock Domain interface handling
 @details
*/
#include "os/clock.h"
#include "os/string.h"

#include "common/log.h"

#include "avtp.h"
#include "stream.h"
#include "clock_domain.h"

const char *clock_domain_status2string(genavb_clock_domain_status_t status)
{
	switch (status) {
		case2str(GENAVB_CLOCK_DOMAIN_STATUS_UNLOCKED);
		case2str(GENAVB_CLOCK_DOMAIN_STATUS_LOCKED);
		case2str(GENAVB_CLOCK_DOMAIN_STATUS_FREE_WHEELING);
		case2str(GENAVB_CLOCK_DOMAIN_STATUS_HW_ERROR);
	default:
		return (char *)"Unknown clock domain status";
	}
}

static struct clock_source *clock_domain_find_source(struct clock_domain *domain, struct avtp_ctx *avtp,
				struct genavb_msg_clock_domain_set_source *set_source)
{
	struct clock_source *source = NULL;

	if (set_source->source_type == GENAVB_CLOCK_SOURCE_TYPE_INPUT_STREAM)
		source = &domain->stream_source;
	else if (set_source->source_type == GENAVB_CLOCK_SOURCE_TYPE_INTERNAL) {
		switch (set_source->local_id) {
		case GENAVB_CLOCK_SOURCE_AUDIO_CLK:
			source = &domain->hw_source;
			break;

		case GENAVB_CLOCK_SOURCE_PTP_CLK:
			source = &avtp->port[0].ptp_source;
			break;

		default:
			break;
		}
	}

	return source;
}

int __clock_domain_update_source(struct clock_domain *domain, struct clock_source *new_source, void *data)
{
	struct avtp_ctx *avtp = container_of(domain, struct avtp_ctx, domain[domain->id]);
	struct avtp_port *port;
	int i;

	/* Setting a null source is not supported. */
	if (!new_source) {
		os_log(LOG_ERR, "clock_domain(%p) can not set null source\n", domain);
		goto err;
	}

	/* Look up all talker streams and disable clock generation  */
	for (i = 0; i < avtp->port_max; i++) {
		struct list_head *entry;

		port = &avtp->port[i];

		for (entry = list_first(&port->talker); entry != &port->talker; entry = list_next(entry)) {
			struct stream_talker *stream = container_of(entry, struct stream_talker, common.list);

			/* For talkers in the specified domain, disable clock consumer (if not previously disabled). */
			if (stream->domain == domain)
				stream_clock_consumer_disable(stream);
		}
	}

	/* Setup domain with new source */
	if (clock_domain_set_source(domain, new_source, data) < 0)
		goto err;

	/* Start with new source */
	for (i = 0; i < avtp->port_max; i++) {
		struct list_head *entry;

		port = &avtp->port[i];

		for (entry = list_first(&port->talker); entry != &port->talker; entry = list_next(entry)) {
			struct stream_talker *stream = container_of(entry, struct stream_talker, common.list);

			/* Enable clock consumer for all talkers in the domain. The loop (with consumer disable) above guarantees
			 * that all talkers are disabled at this stage.
			 */
			if (stream->domain == domain) {
				if (stream_clock_consumer_enable(stream) < 0)
					goto err;

				stream_media_clock_reset(stream);
			}
		}
	}

	return 0;

err:
	return -1;
}

static int clock_domain_update_source(struct clock_domain *domain, struct clock_source *new_source)
{
	struct avtp_ctx *avtp = container_of(domain, struct avtp_ctx, domain[domain->id]);
	struct avtp_port *port;
	struct stream_listener *stream_source = NULL;
	int i;

	os_log(LOG_INFO, "new source(%p) for domain(%p): %d\n", new_source, domain, domain->id);

	/* If new source is a stream, look it up  */
	if (domain->source_type == GENAVB_CLOCK_SOURCE_TYPE_INPUT_STREAM) {
		for (i = 0; i < avtp->port_max; i++) {
			port = &avtp->port[i];

			stream_source = stream_listener_find(port, domain->stream_id);
			if (stream_source)
				break;
		}
	}

	/*
	 * stream_source can be NULL and attached later
	 * but domain needs to be updated with new source
	 */

	return __clock_domain_update_source(domain, new_source, stream_source);
}

static void clock_domain_status_indication(struct clock_domain *domain, struct ipc_tx *tx, unsigned int ipc_dst)
{
	struct ipc_desc *desc;

	if (!(domain->source_flags & CLOCK_DOMAIN_SOURCE_FLAG_USER))
		return;

	os_log(LOG_INFO, "domain(%p): %u, state: %40s\n", domain, domain->id, clock_domain_status2string(domain->status));

	desc = ipc_alloc(tx, sizeof(struct genavb_msg_clock_domain_status));
	if (desc) {
		desc->dst = ipc_dst;
		desc->flags = 0;
		desc->len = sizeof(struct genavb_msg_clock_domain_status);
		desc->type = GENAVB_MSG_CLOCK_DOMAIN_STATUS;

		desc->u.clock_domain_status.domain = GENAVB_CLOCK_DOMAIN_0 + domain->id;
		desc->u.clock_domain_status.source_type = domain->source_type;

		if (domain->source_type == GENAVB_CLOCK_SOURCE_TYPE_INTERNAL)
			desc->u.clock_domain_status.local_id = domain->local_id;
		else
			os_memcpy(desc->u.clock_domain_status.stream_id, domain->stream_id, 8);

		desc->u.clock_domain_status.status = domain->status;

		if (ipc_tx(tx, desc) < 0) {
			os_log(LOG_DEBUG, "domain(%p) ipc_tx(%p) failed\n", domain, tx);
			ipc_free(tx, desc);
		}
	} else
		os_log(LOG_ERR, "domain(%p) ipc_tx(%p) IPC descriptor allocation failed\n", domain, tx);
}

static void clock_domain_response(struct clock_domain *domain, unsigned int status, struct ipc_tx *tx, unsigned int ipc_dst)
{
	struct ipc_desc *desc;

	os_log(LOG_INFO, "domain(%p): %d\n", domain, domain->id);

	desc = ipc_alloc(tx, sizeof(struct genavb_msg_clock_domain_response));
	if (desc) {
		desc->dst = ipc_dst;
		desc->flags = 0;
		desc->len = sizeof(struct genavb_msg_clock_domain_response);
		desc->type = GENAVB_MSG_CLOCK_DOMAIN_RESPONSE;

		desc->u.clock_domain_response.domain = GENAVB_CLOCK_DOMAIN_0 + domain->id;
		desc->u.clock_domain_response.status = status;

		if (ipc_tx(tx, desc) < 0) {
			os_log(LOG_DEBUG, "domain(%p) ipc_tx(%p) failed\n", domain, tx);
			ipc_free(tx, desc);
		}
	} else
		os_log(LOG_ERR, "domain(%p) ipc_tx(%p) IPC descriptor allocation failed\n", domain, tx);
}

static void clock_domain_error(struct ipc_desc *rx_desc, unsigned int status, struct ipc_tx *tx)
{
	struct ipc_desc *desc;

	desc = ipc_alloc(tx, sizeof(struct genavb_msg_error_response));
	if (desc) {
		desc->dst = rx_desc->src;
		desc->flags = 0;
		desc->len = sizeof(struct genavb_msg_error_response);
		desc->type = GENAVB_MSG_ERROR_RESPONSE;

		desc->u.error.type = rx_desc->type;
		desc->u.error.len = rx_desc->len;
		desc->u.error.status = status;

		if (ipc_tx(tx, desc) < 0) {
			os_log(LOG_DEBUG, "ipc_tx(%p) failed\n", tx);
			ipc_free(tx, desc);
		}
	} else
		os_log(LOG_ERR, "ipc_tx(%p) IPC descriptor allocation failed\n", tx);
}

static int clock_domain_set_source_hdlr(struct clock_domain *domain, struct avtp_ctx *avtp, struct genavb_msg_clock_domain_set_source *set_source)
{
	struct clock_source *new_source;

	new_source = clock_domain_find_source(domain, avtp, set_source);
	if (!new_source)
		goto err;

	if ((new_source == domain->source) &&
			(domain->source_type != GENAVB_CLOCK_SOURCE_TYPE_INPUT_STREAM || !os_memcmp(domain->stream_id, set_source->stream_id, 8)))
		return 0;

	/* Save domain source info */
	domain->source_type = set_source->source_type;
	if (domain->source_type == GENAVB_CLOCK_SOURCE_TYPE_INTERNAL)
		domain->local_id = set_source->local_id;
	else if (domain->source_type == GENAVB_CLOCK_SOURCE_TYPE_INPUT_STREAM)
		copy_64(domain->stream_id, set_source->stream_id);

	if (clock_domain_update_source(domain, new_source) < 0)
		goto err;

	return 0;

err:
	return -1;
}

int clock_domain_set_source_legacy(struct clock_domain *domain, struct avtp_ctx *avtp, struct ipc_avtp_connect *ipc)
{
	struct genavb_msg_clock_domain_set_source set_source;

	/* If domain source was set using new API, skip */
	if (domain->source_flags & CLOCK_DOMAIN_SOURCE_FLAG_USER)
		goto err;

	/* If domain source is a stream and it's active, skip */
	if (domain->source
	&& (domain->source_type == GENAVB_CLOCK_SOURCE_TYPE_INPUT_STREAM)
	&& domain->source->grid.producer.u.stream.rec)
		goto out;

	switch (ipc->clock_domain) {
	case GENAVB_MEDIA_CLOCK_DOMAIN_STREAM:
		if (ipc->direction == AVTP_DIRECTION_TALKER)
			goto err;

		if (ipc->flags & IPC_AVTP_FLAGS_MCR) {
			/* Keep compatibiity if no HW support */
			if (!domain->hw_sync)
				goto out;

			set_source.source_type = GENAVB_CLOCK_SOURCE_TYPE_INPUT_STREAM;
			copy_64(set_source.stream_id, &ipc->stream_id);
		} else
			goto out;

		break;

	case GENAVB_MEDIA_CLOCK_DOMAIN_PTP:
		if (ipc->direction == AVTP_DIRECTION_LISTENER)
			goto err;

		set_source.source_type = GENAVB_CLOCK_SOURCE_TYPE_INTERNAL;
		set_source.local_id = GENAVB_CLOCK_SOURCE_PTP_CLK;
		break;

	default:
		goto out;
		break;
	}

	return clock_domain_set_source_hdlr(domain, avtp, &set_source);

out:
	return 0;

err:
	return -1;
}

/** Clock Domain ipc receive callback
 *
 * The function takes ownership of the received descriptor and is responsible for freeing it.
 *
 * \return none
 * \param rx pointer to ipc receive context
 * \param desc pointer to ipc descriptor
 */
void clock_domain_ipc_rx_media_stack(struct ipc_rx const *rx, struct ipc_desc *desc)
{
	struct avtp_ctx *avtp = container_of(rx, struct avtp_ctx, ipc_rx_clock_domain);
	struct genavb_msg_clock_domain_set_source *set_source;
	unsigned int domain_id;
	struct ipc_tx *tx;
	struct clock_domain *domain;

	os_log(LOG_INFO, "\n");

	if (desc->flags & IPC_FLAGS_AVB_MSG_SYNC)
		tx = &avtp->ipc_tx_clock_domain_sync;
	else
		tx = &avtp->ipc_tx_clock_domain;

	switch (desc->type) {
	case GENAVB_MSG_CLOCK_DOMAIN_SET_SOURCE:
		if (desc->len != sizeof(struct genavb_msg_clock_domain_set_source)) {
			clock_domain_error(desc, GENAVB_ERR_CTRL_LEN, tx);
			break;
		}

		set_source = &desc->u.clock_domain_set_source;
		domain_id = set_source->domain;

		if ((domain_id < GENAVB_CLOCK_DOMAIN_0) || (domain_id >= GENAVB_CLOCK_DOMAIN_MAX)) {
			clock_domain_error(desc, GENAVB_ERR_CTRL_INVALID, tx);
			break;
		}

		domain = clock_domain_get(avtp, domain_id);
		if (!domain) {
			clock_domain_error(desc, GENAVB_ERR_CTRL_FAILED, tx);
			break;
		}

		if (clock_domain_set_source_hdlr(domain, avtp, set_source) < 0) {
			clock_domain_response(domain, GENAVB_ERR_CTRL_FAILED, tx, desc->src);
			break;
		}

		domain->source_flags |= CLOCK_DOMAIN_SOURCE_FLAG_USER;

		clock_domain_response(domain, GENAVB_SUCCESS, tx, desc->src);

		break;

	case GENAVB_MSG_CLOCK_DOMAIN_GET_STATUS:
		domain_id = desc->u.clock_domain_get_status.domain;

		if ((domain_id < GENAVB_CLOCK_DOMAIN_0) || (domain_id >= GENAVB_CLOCK_DOMAIN_MAX)) {
			clock_domain_error(desc, GENAVB_ERR_CTRL_INVALID, tx);
			break;
		}

		domain = clock_domain_get(avtp, domain_id);
		if (!domain) {
			clock_domain_error(desc, GENAVB_ERR_CTRL_INVALID, tx);
			break;
		}

		if (!(domain->source_flags & CLOCK_DOMAIN_SOURCE_FLAG_USER)) {
			clock_domain_error(desc, GENAVB_ERR_CTRL_INVALID, tx);
			break;
		}

		clock_domain_status_indication(domain, tx, desc->src);

		break;

	default:
		clock_domain_error(desc, GENAVB_ERR_CTRL_UNKNOWN, tx);

		break;
	}

	ipc_free(rx, desc);
}

static const genavb_clock_domain_status_t state_to_status[] = {
	[0] = GENAVB_CLOCK_DOMAIN_STATUS_UNLOCKED,
	[CLOCK_DOMAIN_STATE_LOCKED] = GENAVB_CLOCK_DOMAIN_STATUS_LOCKED,
	[CLOCK_DOMAIN_STATE_FREE_WHEELING] = GENAVB_CLOCK_DOMAIN_STATUS_UNLOCKED,
	[CLOCK_DOMAIN_STATE_FREE_WHEELING | CLOCK_DOMAIN_STATE_LOCKED] = GENAVB_CLOCK_DOMAIN_STATUS_FREE_WHEELING
};

static void clock_domain_set_status(struct clock_domain *domain, unsigned int status)
{
	struct avtp_ctx *avtp = container_of(domain, struct avtp_ctx, domain[domain->id]);

	if (domain->status != status) {
		domain->status = status;

		if (domain->state & CLOCK_DOMAIN_STATE_LOCKED)
			domain->locked_count++;

		clock_domain_status_indication(domain, &avtp->ipc_tx_clock_domain, IPC_DST_ALL);
	}
}

void clock_domain_clear_state(struct clock_domain *domain, clock_domain_state_t state)
{
	domain->state &= ~state;

	if (state == CLOCK_DOMAIN_STATE_LOCKED)
		clock_domain_set_status(domain, state_to_status[domain->state]);
}

void clock_domain_set_state(struct clock_domain *domain, clock_domain_state_t state)
{
        domain->state |= state;

	if (state == CLOCK_DOMAIN_STATE_LOCKED)
		clock_domain_set_status(domain, state_to_status[domain->state]);
}

void clock_domain_stats_print(struct ipc_avtp_clock_domain_stats *msg)
{
	os_log(LOG_INFO, "domain(%p): %u, state: %40s, locked count: %10u\n", msg->domain, msg->domain_id, clock_domain_status2string(msg->domain_status), msg->domain_locked_count);
}

static void _clock_domain_stats_dump(struct clock_domain *domain, struct ipc_tx *tx)
{
	struct ipc_desc *desc;
	struct ipc_avtp_clock_domain_stats *msg;

	desc = ipc_alloc(tx, sizeof(*msg));
	if (!desc)
		goto err_ipc_alloc;

	desc->type = IPC_AVTP_CLOCK_DOMAIN_STATS;
	desc->len = sizeof(*msg);
	desc->flags = 0;

	msg = (struct ipc_avtp_clock_domain_stats *)&desc->u;

	msg->domain = domain;
	msg->domain_id = domain->id;
	msg->domain_status = domain->status;
	msg->domain_locked_count = domain->locked_count;

	if (ipc_tx(tx, desc) < 0)
		goto err_ipc_tx;

	return;

err_ipc_tx:
	ipc_free(tx, desc);

err_ipc_alloc:
	return;
}

void clock_domain_stats_dump(struct clock_domain *domain, struct ipc_tx *tx)
{
	struct clock_grid *grid;
	struct list_head *entry;

	if (!list_empty(&domain->grids)) {
		_clock_domain_stats_dump(domain, tx);

		for (entry = list_first(&domain->grids); entry != &domain->grids; entry = list_next(entry)) {
			grid = container_of(entry, struct clock_grid, list);

			clock_grid_stats_dump(grid, tx);
		}
	}
}

/** User clock generation wake-up scheduler.
 * This scheduler is woken-up by the lower-level HW clock, updates HW user clocks wake-up counters
 * and if expired calls the network transmit callback associated to the stream.
 * \return	none
 * \param clock	pointer to media_clock_gen_hw
 */
void clock_domain_sched(struct clock_domain *domain, unsigned int prio)
{
	struct list_head *entry, *next;
	struct stream_talker *stream;
	unsigned int reset;

	clock_grid_ts_update(&domain->source->grid, domain->ts_update_n, &reset);

	for (entry = list_first(&domain->sched_streams[prio]); next = list_next(entry), entry != &domain->sched_streams[prio]; entry = next) {
		stream = container_of(entry, struct stream_talker, consumer.list);

		stream_net_tx_handler(stream);
	}
}

void clock_domain_exit_consumer_wakeup(struct clock_grid_consumer *consumer)
{
	/* Check if the consumer is in the sched_streams list. */
	if (consumer->list.next) {		//FIXME WAKEUP to be removed once wake-up is replaced by media/net event
		struct clock_domain *domain = consumer->grid->domain;

		/* Remove consumer from the list. */
		list_del(&consumer->list);

		/* If last user clock, stop HW*/
		if (list_empty(&domain->sched_streams[consumer->prio]))
			os_timer_stop(&domain->source->timer[consumer->prio]);
	}
}


/**
 *  This will initialize the wake-up for the consumer.
 */
int clock_domain_init_consumer_wakeup(struct clock_domain *domain, struct clock_grid_consumer *consumer, unsigned int wake_freq_p, unsigned int wake_freq_q)
{
	struct clock_source *source = domain->source;
	struct clock_scheduling_params *sched_params = &source->sched_params[consumer->prio];

	if (!wake_freq_p) {
		os_log(LOG_ERR, "clock_grid_consumer(%p) null wake period\n", consumer);
		goto err;
	}

	if (clock_source_ready(source) < 0) {
		os_log(LOG_ERR, "clock_source(%p) is not ready\n", consumer);
		goto err;
	}

	if (((u64)NSECS_PER_SEC * wake_freq_q) < ((u64)source->tick_period * MCG_WAKE_UP_MIN * wake_freq_p)) {
		os_log(LOG_ERR, "clock_grid_consumer(%p) wake period (%u ns) is too small (%u ns)\n",
		       consumer, (unsigned int)(((u64)NSECS_PER_SEC * wake_freq_q) / wake_freq_p), MCG_WAKE_UP_MIN * source->tick_period);

		goto err;
	}

	/* First user clock, start HW */
	if (list_empty(&domain->sched_streams[consumer->prio])) {

		if (os_timer_start(&source->timer[consumer->prio], 0, wake_freq_p, wake_freq_q, 0) < 0)
			goto err;

		domain->ts_update_n = ((u64)source->grid.nominal_freq_p * wake_freq_q + source->grid.nominal_freq_q * wake_freq_p - 1) / (source->grid.nominal_freq_q * wake_freq_p);

		sched_params->wake_freq_p = wake_freq_p;
		sched_params->wake_freq_q = wake_freq_q;

		os_log(LOG_INFO, "HW clock_source(%p) wake-up period : %u/%u %u ns\n",
				source, sched_params->wake_freq_q, sched_params->wake_freq_p,
				(unsigned int)(((u64)NSECS_PER_SEC * sched_params->wake_freq_q) / sched_params->wake_freq_p));

	} else {
		if ((sched_params->wake_freq_p * wake_freq_q) != (sched_params->wake_freq_q * wake_freq_p)) {
			os_log(LOG_ERR, "clock consumer(%p) frequency (%u Hz) mismatch, current (%u Hz)\n",
					consumer, wake_freq_p / wake_freq_q, sched_params->wake_freq_p / sched_params->wake_freq_q);

			goto err;
		}
	}

	list_add_tail(&domain->sched_streams[consumer->prio], &consumer->list);

	return 0;

err:
	return -1;
}

void clock_domain_exit_consumer(struct clock_grid_consumer *consumer)
{
	clock_grid_consumer_exit(consumer);
}


/** Clock domain consumer initialization function.
 * \return	 0 if success, -1 otherwise
 * \param domain	pointer to clock domain
 * \param consumer	pointer to clock consumer
 * \param offset	consumer maximum timestamp offset
 * \param nominal_freq_p consumer timestamp nominal frequency (in the form p/q Hz)
 * \param nominal_freq_q consumer timestamp nominal frequency
 * \param alignment	consumer timestamp alignment
*/
int clock_domain_init_consumer(struct clock_domain *domain, struct clock_grid_consumer *consumer, unsigned int offset, u32 nominal_freq_p, u32 nominal_freq_q, unsigned int alignment, unsigned int prio)
{
	struct clock_grid *grid;
	int rc = 0;

	consumer->prio = prio;

	if (!domain->source) {
		os_log(LOG_ERR, "clock_domain(%p) has no source defined\n", domain);
		return -1;
	}

	switch (domain->source->grid.producer.type) {
	case GRID_PRODUCER_HW:
	case GRID_PRODUCER_PTP:
	case GRID_PRODUCER_STREAM:

		grid = clock_domain_find_grid(domain, nominal_freq_p, nominal_freq_q);
		if (!grid) {
			grid = clock_grid_alloc();
			if (!grid) {
				os_log(LOG_ERR, "clock_domain(%p) Could not allocate new clock_grid\n", domain);
				break;
			}

			rc = clock_grid_init_mult(grid, domain, &domain->source->grid, nominal_freq_p, nominal_freq_q);
			if (rc < 0) {
				clock_grid_free(grid);
				break;
			}
		}

		clock_grid_consumer_attach(consumer, grid, offset, alignment);

		break;

	default:
		os_log(LOG_ERR, "clock_source(%p) type(%d) invalid\n", domain->source, domain->source->grid.producer.type);
		rc = -1;
		break;
	}

	return rc;
}


struct clock_grid *clock_domain_find_grid(struct clock_domain *domain, u32 nominal_freq_p, u32 nominal_freq_q)
{
	struct list_head *entry, *next;
	struct clock_grid *grid;

	for (entry = list_first(&domain->grids); next = list_next(entry), entry != &domain->grids; entry = next) {
		grid = container_of(entry, struct clock_grid, list);

		if (((u64)grid->nominal_freq_p * nominal_freq_q) == ((u64)grid->nominal_freq_q * nominal_freq_p))
			return grid;
	}

	return NULL;
}

void clock_domain_add_grid(struct clock_domain *domain, struct clock_grid *grid)
{
	grid->domain = domain;
	list_add_tail(&domain->grids, &grid->list);
}

void clock_domain_remove_grid(struct clock_grid *grid)
{
	list_del(&grid->list);
	grid->domain = NULL;
}

unsigned int clock_domain_is_source_stream(struct clock_domain *domain, void *stream_id)
{
	if ((!domain->source) || (domain->source_type != GENAVB_CLOCK_SOURCE_TYPE_INPUT_STREAM))
		return 0;

	return cmp_64(domain->stream_id, stream_id);
}

unsigned int clock_domain_is_locked(struct clock_domain *domain)
{
	if (domain->status == GENAVB_CLOCK_DOMAIN_STATUS_UNLOCKED)
		return 0;

	return 1;
}

int clock_domain_set_source(struct clock_domain *domain, struct clock_source *source, void *data)
{
	/* Close previous source */
	if (domain->source) {
		clock_source_close(domain->source);
		clock_domain_remove_grid(&domain->source->grid);
		domain->source = NULL;
	}

	/* Add new source */
	domain->source = source;
	clock_domain_add_grid(domain, &source->grid);
	if (clock_source_open(domain->source, data) < 0) {
		clock_domain_remove_grid(&domain->source->grid);
		domain->source = NULL;

		os_log(LOG_ERR, "domain(%p): %d => clock_source_open(%p) error , %s\n",
		       domain, domain->id, source, clock_grid_producer_type2string(source->grid.producer.type));
		return -1;
	}

	os_log(LOG_INFO, "domain(%p): %d => clock source grid(%p), %s\n",
	       domain, domain->id, source, clock_grid_producer_type2string(source->grid.producer.type));

	return 0;
}

int clock_domain_has_stream(struct avtp_ctx *avtp, struct clock_domain *domain)
{
	struct stream_talker *talker;
	struct stream_listener *listener;
	struct list_head *entry;
	struct avtp_port *port;
	int i;

	for (i = 0; i < avtp->port_max; i++) {
		port = &avtp->port[i];

		for (entry = list_first(&port->talker); entry != &port->talker; entry = list_next(entry)) {
			talker = container_of(entry, struct stream_talker, common.list);

			if (talker->domain == domain)
				return 1;
		}

		for (entry = list_first(&port->listener); entry != &port->listener; entry = list_next(entry)) {
			listener = container_of(entry, struct stream_listener, common.list);

			if (listener->domain == domain)
				return 1;
		}
	}
	return 0;
}

struct clock_domain *clock_domain_get(struct avtp_ctx *avtp, unsigned int id)
{
	struct clock_domain *domain = NULL;

	if (id >= GENAVB_CLOCK_DOMAIN_0) {
		/* New clock domain API */
		if (id >= GENAVB_CLOCK_DOMAIN_MAX)
			return NULL;

		domain = &avtp->domain[id - GENAVB_CLOCK_DOMAIN_0];

	} else {
		/* Legacy support */
		switch (id) {
		case GENAVB_MEDIA_CLOCK_DOMAIN_STREAM:
		case GENAVB_MEDIA_CLOCK_DOMAIN_MASTER_CLK:
			domain = &avtp->domain[0];
			break;

		case GENAVB_MEDIA_CLOCK_DOMAIN_PTP:
			domain = &avtp->domain[1];
			break;

		default:
			domain = NULL;
			goto exit;
			break;
		}
	}

	os_log(LOG_INFO, "ipc id %d => domain(%p): %d\n", id, domain, domain->id);

exit:
	return domain;
}

__init void clock_domain_init(struct clock_domain *domain, unsigned int id, unsigned long priv)
{
	int i;

	domain->id = id;
	list_head_init(&domain->grids);

	for (i = 0; i < CFG_SR_CLASS_MAX; i++)
		list_head_init(&domain->sched_streams[i]);

	clock_source_init(&domain->stream_source, GRID_PRODUCER_STREAM, id, priv);
	clock_source_init(&domain->hw_source, GRID_PRODUCER_HW, id, priv);

	domain->hw_sync = media_clock_rec_init(id);

	os_log(LOG_INFO, "domain(%p): %d\n", domain, id);
}

__exit void clock_domain_exit(struct clock_domain *domain)
{
	clock_source_exit(&domain->hw_source);
	clock_source_exit(&domain->stream_source);
	if (domain->hw_sync)
		media_clock_rec_exit(domain->hw_sync);

	os_log(LOG_INFO, "domain(%p): %d\n", domain, domain->id);
}
