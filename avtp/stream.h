/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2019, 2021-2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 @file
 @brief Stream handling functions
 @details
*/

#ifndef _STREAM_H_
#define _STREAM_H_

#include "os/sys_types.h"
#include "common/list.h"

#include "avtp.h"

#include "genavb/media.h"
#include "os/media.h"
#include "media_clock.h"

#define HEADER_TEMPLATE_SIZE 64

#define STREAM_FLAG_VLAN		(1 << 0)	/* Stream is vlan tagged */
#define STREAM_FLAG_SR			(1 << 1)	/* Stream has a stream reservation */
#define STREAM_FLAG_MEDIA_WAKEUP	(1 << 2)	/* Stream processing started by media interface (instead of clock generation) */
#define STREAM_FLAG_CLOCK_GENERATION	(1 << 3)	/* Stream requires clock generation */
#define STREAM_FLAG_NO_MEDIA		(1 << 4)	/* Stream doesn't exchange data with media interface */
#define STREAM_FLAG_CUSTOM_TSPEC	(1 << 5)	/* Stream params inherited from the media interface */
#define STREAM_FLAG_DESTROYED		(1 << 6)	/* Stream is destroyed and waits to be freed */

/** Common stream context
 *
 * Common fields to Listener and Talker streams
 */
struct stream_common {
	struct avtp_ctx *avtp;
	struct list_head list;
	u64 destroy_time;
	unsigned int flags;
};

/** Listener stream context.
 *
 * Each listener stream contains a network receive context. There is a one to one mapping between streams
 * and receive queues.
 */
struct stream_listener {
	struct stream_common common;			/* Must be placed at the start of the structure */

	struct clock_domain *domain;
	struct clock_source *source;

	u64 id;						/**< AVTP stream_id, stored as Big Endian */
	u8 dst_mac[6];
	u16 port;

	os_clock_id_t clock_gptp;

	avtp_direction_t direction;

	sr_class_t class;

	void (*net_rx)(struct stream_listener *, struct avtp_rx_desc **, unsigned int);
	int (*init)(struct stream_listener *);
	void (*exit)(struct stream_listener *);

	struct net_rx rx;

	struct media_tx media;

	struct avdecc_format format;

	unsigned int subtype;
	u8 sequence_num;
	unsigned int mr;
	unsigned int pkt_received;
	u32 gptp_current;	/* gptp snapshot taken at the start of the stream batch processing */

	u32 max_transit_time;
	u32 max_timing_uncertainty;

	union {
		struct {
			unsigned int current_frame_offset;
		} cvf;

		struct {
			unsigned int syt_interval_ln2;
			u8 syt_count;
		} iec61883_6;

		struct {
			u32 hdr[2];
			u32 hdr_mask[2];
		} aaf;

		struct crf_subtype_data {
			/* Header data to match on receive (starting at pull field) */
			u32 hdr[2];
			u8 type;
			unsigned int ts_last;
			unsigned int ts_last_set;
			unsigned int received_ts_last;
			unsigned int state;
			unsigned int period_nominal;
			unsigned int period_err;
			unsigned int period;
			unsigned int timestamp;
			struct timer timer;
			unsigned int free_wheeling_to_locked_delay;
		} crf;
	} subtype_data;

	struct listener_stats {
		unsigned int rx;
		unsigned int media_tx;
		unsigned int clock_tx;
		unsigned int pkt_lost;
		unsigned int mr;
		unsigned int tu;
		unsigned int subtype_err;
		unsigned int format_err;
		unsigned int media_tx_err;
		unsigned int media_tx_dropped;
		unsigned int gptp_err;
		unsigned int early_timestamp;
		unsigned int late_timestamp;

		struct stats avb_delay;
		struct stats avtp_delay;
		struct stats batch;
	} stats;
};

/** Talker stream context
 *
 * Each talker stream contains a network transmit context. There is a one to one mapping between streams
 * and transmit queues.
 */
struct stream_talker {
	struct stream_common common;			/* Must be placed at the start of the structure */

	struct clock_domain *domain;
	unsigned int locked_count;

	u64 id;

	avtp_direction_t direction;

	sr_class_t class;
	unsigned int subtype;
	u16 port;

	os_clock_id_t clock_gptp;

	void (*net_tx)(struct stream_talker *);
	void (*init)(struct stream_talker *, unsigned int *);

	struct net_tx tx;

	struct media_rx media;

	struct avdecc_format format;
	unsigned int payload_size;
	unsigned int frames_per_interval;
	unsigned int frames_per_packet;
	unsigned int media_count;
	unsigned int sample_rate;
	unsigned int samples_per_timestamp;

	struct clock_grid_consumer consumer;	// Uses a grid with a MULT producer, itself using a source with a HW producer
	bool consumer_enabled;

	unsigned int ts_n;
	unsigned int tx_batch;
	unsigned int late;
	unsigned int frame_with_ts;
	unsigned int time_per_packet;
	unsigned int ts_last;
	u32 ts_launch;
	u32 gptp_current; 	/* gptp snapshot taken at the start of the stream batch processing */

	unsigned int ts_media_prev;

	unsigned int header_len;

	unsigned int tx_event_enabled;
	unsigned long priv;

	union {
		struct {
			unsigned int syt_interval_ln2;
			struct iec_61883_hdr *iec_hdr;
		} iec61883;

		struct {
			unsigned int frames_per_timestamp;
			unsigned int sparse;
			unsigned int tx_count;
		} aaf;

		struct {
			u32 ts_msb;
			u32 ts_period;
			struct os_timer t;
		} crf;

		struct {
			struct cvf_h264_hdr *h264_hdr;
			unsigned int prev_incomplete_nal;
			unsigned int is_nalu_ts_valid;
			u32 h264_timestamp;
			u8 nalu_header;
		} cvf_h264;
	} subtype_data;

	unsigned int latency;
	unsigned int max_transit_time;

	struct avtp_data_hdr *avtp_hdr;
	u8 header_template[HEADER_TEMPLATE_SIZE];

	struct talker_stats {
		unsigned int tx;
		unsigned int tx_err;
		unsigned int media_rx;
		unsigned int media_err;
		unsigned int media_underrun;
		unsigned int mr;
		unsigned int clock_rx;
		unsigned int clock_err;
		unsigned int partial;
		unsigned int clock_invalid;
		unsigned int gptp_err;

		struct stats sched_intvl;
	} stats;
};

struct ipc_avtp_listener_stats {
	avb_u64 stream_id;
	struct listener_stats stats;
	unsigned int clock_rec_enabled;

	struct ipc_avtp_clock_rec_stats clock_stats;
};

struct ipc_avtp_talker_stats {
	avb_u64 stream_id;
	struct talker_stats stats;
};

#define stream_destroy(stream, ipc_tx) \
{	\
	if ((stream)->direction == AVTP_DIRECTION_LISTENER)	\
		stream_listener_destroy((struct stream_listener *)stream, ipc_tx);	\
	else	\
		stream_talker_destroy((struct stream_talker *)stream, ipc_tx);	\
}

struct stream_listener *stream_listener_find(struct avtp_port *port, void *stream_id);
struct stream_talker *stream_talker_find(struct avtp_port *port, void *stream_id);

void avtp_latency_stats(struct stream_listener *stream, struct avtp_rx_desc *desc);

struct stream_listener *stream_listener_create(struct avtp_ctx *avtp, struct avtp_port *port, struct ipc_avtp_connect *params);
void stream_listener_destroy(struct stream_listener *stream, struct ipc_tx *tx);

struct stream_talker *stream_talker_create(struct avtp_ctx *avtp, struct avtp_port *port, struct ipc_avtp_connect *params);
void stream_talker_destroy(struct stream_talker *stream, struct ipc_tx *tx);
int stream_media_rx(struct stream_talker *stream, struct media_rx_desc **media_desc_array, u32 *ts, unsigned int *flags, unsigned int * alignment_ts);

int stream_tx_flow_control(struct stream_talker *stream, unsigned int *tx_batch);

void stream_stats_dump(struct avtp_port *port, struct ipc_tx *tx);
void stream_talker_stats_print(struct ipc_avtp_talker_stats *msg);
void stream_listener_stats_print(struct ipc_avtp_listener_stats *msg);

void stream_free_all(struct avtp_ctx *avtp);

int stream_clock_consumer_enable(struct stream_talker *stream);
void stream_clock_consumer_disable(struct stream_talker *stream);

unsigned int avtp_stream_presentation_offset(struct stream_talker *stream);

static inline void stream_media_clock_reset(struct stream_talker *stream)
{
	avtp_data_header_toggle_mcr(stream->avtp_hdr);

	stream->stats.mr++;
}

static inline void stream_net_tx_handler(struct stream_talker *stream)
{
	u32 current_time;

	if (os_clock_gettime32(stream->clock_gptp, &current_time) < 0) {
		stream->stats.gptp_err++;
	} else {
		stats_update(&stream->stats.sched_intvl, current_time - stream->gptp_current);
		stream->gptp_current = current_time;
	}

	stream->net_tx(stream);
}

static inline int stream_net_tx(struct stream_talker *stream, struct media_rx_desc **desc, unsigned int n)
{
	int rc;

	/* Send packets */
	rc = net_tx_multi(&stream->tx, (struct net_tx_desc **)desc, n);
	if (rc < (int)n) {
		if (rc > 0) {
			stream->stats.tx += rc;
			stream->stats.tx_err += n - rc;
		} else
			stream->stats.tx_err += n;

		return -1;
	}

	stream->stats.tx += rc;

	return 0;
}

static inline int stream_media_tx(struct stream_listener *stream, struct media_desc **desc, unsigned int n)
{
	int rc;

	rc = media_tx(&stream->media, desc, n);
	if (rc < (int)n) {
		if (rc < 0) {
			stream->stats.media_tx_err++;
			stream->stats.media_tx_dropped += n;
		}
		else
			stream->stats.media_tx_dropped += n - rc;
	}

	if (rc > 0)
		stream->stats.media_tx += rc;

	return rc;
}

static inline unsigned int stream_domain_phase_change(struct stream_talker *stream)
{
	if (stream->locked_count != stream->domain->locked_count) {
		stream->locked_count = stream->domain->locked_count;
		return 1;
	}

	return 0;
}

static inline void stream_listener_check_timestamps(struct stream_listener *stream, avb_u32 rx_ts, avb_u32 avtp_ts, unsigned int tsamples)
{
	if (is_avtp_ts_late(rx_ts, avtp_ts, tsamples))
		stream->stats.late_timestamp++;
	else if (is_avtp_ts_early(rx_ts, avtp_ts, stream->max_transit_time, stream->max_timing_uncertainty, tsamples))
		stream->stats.early_timestamp++;
}

#endif /* _STREAM_H_ */
