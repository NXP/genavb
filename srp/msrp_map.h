/*
 * Copyright 2020-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file	msrp_map.h
 * @brief	MSRP MAP interface.
 * @details
 */

#ifndef _MSRP_MAP_H_
#define _MSRP_MAP_H_

#include "srp.h"


static inline int is_talker_stream_declared(struct msrp_stream *stream, unsigned int port_id)
{
	return (stream->talker_declared_state & (1 << port_id));
}

static inline int is_talker_stream_declared_any(struct msrp_stream *stream)
{
	return (stream->talker_declared_state != 0);
}

static inline int is_talker_stream_registered(struct msrp_stream *stream, unsigned int port_id)
{
	return (stream->talker_registered_state & (1 << port_id));
}

static inline int is_talker_stream_registered_other(struct msrp_stream *stream, unsigned int port_id)
{
	return (stream->talker_registered_state & ~(1 << port_id));
}

static inline int is_talker_stream_registered_any(struct msrp_stream *stream)
{
	return (stream->talker_registered_state != 0);
}

static inline int is_talker_stream_registered_any_forwarding(struct msrp_map *map, struct msrp_stream *stream)
{
	return ((stream->talker_registered_state & map->forwarding_state) != 0);
}

static inline int is_talker_stream_user_declared(struct msrp_stream *stream, unsigned int port_id)
{
	return (stream->talker_user_declared_state & (1 << port_id));
}

static inline int is_talker_stream_user_declared_any(struct msrp_stream *stream)
{
	return (stream->talker_user_declared_state != 0);
}

static inline int is_listener_stream_declared(struct msrp_stream *stream, unsigned int port_id)
{
	return (stream->listener_declared_state & (1 << port_id));
}

static inline int is_listener_stream_declared_any(struct msrp_stream *stream)
{
	return (stream->listener_declared_state != 0);
}

static inline int is_listener_stream_registered(struct msrp_stream *stream, unsigned int port_id)
{
	return (stream->listener_registered_state & (1 << port_id));
}

static inline int is_listener_stream_registered_other(struct msrp_stream *stream, unsigned int port_id)
{
	return (stream->listener_registered_state & ~(1 << port_id));
}

static inline int is_listener_stream_registered_any(struct msrp_stream *stream)
{
	return (stream->listener_registered_state != 0);
}

static inline int is_listener_stream_registered_any_forwarding(struct msrp_map *map, struct msrp_stream *stream)
{
	return ((stream->listener_registered_state & map->forwarding_state) != 0);
}

static inline int is_listener_stream_user_declared(struct msrp_stream *stream, unsigned int port_id)
{
	return (stream->listener_user_declared_state & (1 << port_id));
}

static inline int is_listener_stream_user_declared_any(struct msrp_stream *stream)
{
	return (stream->listener_user_declared_state != 0);
}

static inline unsigned int is_msrp_port_forwarding(struct msrp_map *map, unsigned int port_id)
{
	return (map->forwarding_state & (1 << port_id));
}

struct msrp_map *msrp_get_map_context(struct msrp_ctx *msrp);
void msrp_map_init(struct msrp_ctx *msrp);
void msrp_map_exit(struct msrp_ctx *msrp);
void listener_registered_clear(struct msrp_stream *stream, unsigned int port_id);
void listener_registered_set(struct msrp_stream *stream, unsigned int port_id);
void listener_declared_clear(struct msrp_stream *stream, unsigned int port_id);
void listener_declared_set(struct msrp_stream *stream, unsigned int port_id);
void listener_user_declared_clear(struct msrp_stream *stream, unsigned int port_id);
void listener_user_declared_set(struct msrp_stream *stream, unsigned int port_id);
void talker_registered_clear(struct msrp_stream *stream, unsigned int port_id);
void talker_registered_set(struct msrp_stream *stream, unsigned int port_id);
void talker_declared_clear(struct msrp_stream *stream, unsigned int port_id);
void talker_declared_set(struct msrp_stream *stream, unsigned int port_id);
void talker_user_declared_clear(struct msrp_stream *stream, unsigned int port_id);
void talker_user_declared_set(struct msrp_stream *stream, unsigned int port_id);
void msrp_map_update_stream(struct msrp_map *map, struct msrp_stream *stream, bool new);
void msrp_map_update(struct msrp_map *map);

#endif /* _MSRP_MAP_H_ */
