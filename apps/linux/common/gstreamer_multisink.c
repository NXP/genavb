/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <inttypes.h>

#include <genavb/genavb.h>
#include "common.h"
#include "gstreamer_multisink.h"

#define minimum(a,b)  ((a)<(b)?(a):(b))


/* Offset was determined empirically to avoid late buffers */
#define TALKER_PRESENTATION_OFFSET		15000000

const char *gst_state[] = {
	STR(GST_STATE_STARTING),
	STR(GST_STATE_STARTED),
	STR(GST_STATE_STOPPED),
	STR(GST_STATE_LOOP)
};

const char *stream_state[] = {
	STR(STREAM_STATE_CONNECTING),
	STR(STREAM_STATE_CONNECTED),
	STR(STREAM_STATE_DISCONNECTING),
	STR(STREAM_STATE_DISCONNECTED),
	STR(STREAM_STATE_WAIT_DATA),
	STR(STREAM_STATE_WAIT_START),
	STR(STREAM_STATE_LOOP),
	STR(STREAM_STATE_STOPPED)
};

const char *stream_event[] = {
	STR(STREAM_EVENT_CONNECT),
	STR(STREAM_EVENT_DISCONNECT),
	STR(STREAM_EVENT_DATA),
	STR(STREAM_EVENT_TIMER),
	STR(STREAM_EVENT_LOOP_DONE),
	STR(STREAM_EVENT_PLAY),
	STR(STREAM_EVENT_STOP)
};

const char *gst_event[] = {
	STR(GST_EVENT_START),
	STR(GST_EVENT_STOP),
	STR(GST_EVENT_LOOP),
	STR(GST_EVENT_TIMER)
};

static void stream_stats_show(struct stats *s)
{
	struct talker_gst_multi_app *stream = s->priv;

	printf("stream(%d) min delay %d/ mean delay %d/ max delay %d\n", stream->index, stream->delay.min, stream->delay.mean, stream->delay.max);
}


static GstFlowReturn talker_gst_multi_new_sample_handler(GstAppSink *sink, gpointer data)
{
	struct talker_gst_multi_app *stream = data;
	pthread_mutex_lock(&stream->samples_lock);
	stream->samples++;
	pthread_mutex_unlock(&stream->samples_lock);
	return GST_FLOW_OK;
}


int talker_gst_multi_data_handler(struct talker_gst_multi_app *stream)
{
	struct talker_gst_media *gst = stream->gst;
	struct avb_event event;
	int nbytes = 0;
	int rc = 0;
	uint64_t now = 0;
	unsigned int remaining, written = 0;
	unsigned int event_n;
	unsigned long long offset;
	unsigned long long read_samples = 0;

	if (stream->state == STREAM_STATE_CONNECTED)
		remaining = stream->batch_size;
	else {
		 /* exit when there are no samples to read or they are in the future */
		remaining = -1;
	}

	while (remaining) {
		if (stream->current_size == 0) {

			if (!(stream->count % 10000))
				printf("stream(%d) bytes: %llu\n", stream->index, stream->byte_count);

			stream->count++;

			pthread_mutex_lock(&stream->samples_lock);
			read_samples = stream->samples;
			pthread_mutex_unlock(&stream->samples_lock);

			if (read_samples)
				stream->current_sample = gst_app_sink_pull_sample(GST_APP_SINK(stream->sink));
			else
				stream->current_sample = NULL;

			if (!stream->current_sample) {
//				printf("%s stream(%d) gst_app_sink_pull_sample() failed\n", __func__, _stream->index);
				break;
			}

			pthread_mutex_lock(&stream->samples_lock);
			stream->samples--;
			pthread_mutex_unlock(&stream->samples_lock);

			stream->current_buffer = gst_sample_get_buffer(stream->current_sample);
			if (!stream->current_buffer) {
				printf("%s stream(%d) gst_sample_get_buffer() failed\n", __func__, stream->index);
				goto exit_unref;
			}

			if (gst_buffer_map(stream->current_buffer, &stream->current_info, GST_MAP_READ))
				stream->current_data = stream->current_info.data;
			else {
				printf("%s stream(%d) gst_buffer_map() failed\n", __func__, stream->index);
				goto exit_unref;
			}

			if (!stream->current_data) {
				printf("%s stream(%d) couldn't get data buffer\n", __func__, stream->index);
				goto exit_unmap;
			}

			stream->current_size = stream->current_info.size;
		}

		nbytes = minimum(stream->current_size, remaining);

		event.index = 0;
		event.event_mask = 0;
		event_n = 0;

		if ((stream->current_size == stream->current_info.size) && GST_CLOCK_TIME_IS_VALID(GST_BUFFER_PTS(stream->current_buffer))) {

			gst->gst_pipeline->basetime = gst_element_get_base_time(GST_ELEMENT(gst->gst_pipeline->pipeline));
			if(!GST_CLOCK_TIME_IS_VALID(gst->gst_pipeline->basetime) || !gst->gst_pipeline->basetime) {
				printf("%s stream(%d) Bad basetime: %"GST_TIME_FORMAT" \n", __func__, stream->index, GST_TIME_ARGS(gst->gst_pipeline->basetime));
				break;
			}

			gettime_ns(&now);

			if (stream->state == STREAM_STATE_CONNECTED) {
				offset = TALKER_PRESENTATION_OFFSET + avb_stream_presentation_offset(stream->stream_h);

				if (!stream->audio || !stream->byte_count) {
					event.event_mask = AVTP_SYNC;
					event.ts = (gst->gst_pipeline->basetime + GST_BUFFER_PTS(stream->current_buffer) + offset) & 0xffffffff;
					event_n = 1;
					stream->last_ts = event.ts;
				}

				stats_update(&stream->delay, (int)(gst->gst_pipeline->basetime + GST_BUFFER_PTS(stream->current_buffer) + offset) - now);

				if (stream->count < 10)
					printf("stream(%d) now: %"GST_TIME_FORMAT"(%u) count: %lld ts: %"GST_TIME_FORMAT" (%u) delta: %"GST_STIME_FORMAT" basetime: %"GST_TIME_FORMAT" buffer ts: %"GST_TIME_FORMAT" size: %u\n",
						stream->index,
						GST_TIME_ARGS(now), (unsigned int)now,
						stream->byte_count,
						GST_TIME_ARGS(gst->gst_pipeline->basetime + GST_BUFFER_PTS(stream->current_buffer) + offset),
						(unsigned int)(gst->gst_pipeline->basetime + GST_BUFFER_PTS(stream->current_buffer) + offset),
						GST_STIME_ARGS((GstClockTimeDiff)((gst->gst_pipeline->basetime + GST_BUFFER_PTS(stream->current_buffer) + offset) - now)),
						GST_TIME_ARGS(gst->gst_pipeline->basetime), GST_TIME_ARGS(GST_BUFFER_PTS(stream->current_buffer)),
						stream->current_size);
			}
		}

		if (stream->state == STREAM_STATE_CONNECTED) {
			if (stream->params.format.u.s.subtype_u.cvf.subtype == CVF_FORMAT_SUBTYPE_H264) {
				/* In some cases, Gstreamer will output invalid timestamp for buffers
                       		 * So use the last valid one */
				if ((stream->current_size == stream->current_info.size) && !(GST_CLOCK_TIME_IS_VALID(GST_BUFFER_PTS(stream->current_buffer)))) {
						event.event_mask |= AVTP_SYNC;
						event.ts = stream->last_ts;
						event_n = 1;
				}

				if ((stream->params.format.u.s.subtype_u.cvf.subtype == CVF_FORMAT_SUBTYPE_H264) && (stream->current_size == nbytes)) {
						event.event_mask |= AVTP_FRAME_END;
						event_n = 1;
				}

// 				printf(" %s : h264 stream sending nbytes %d event_n %d event.event_mask %x event-ts %"GST_TIME_FORMAT" GST Buffer PTS %"GST_TIME_FORMAT" GST Basetime %"GST_TIME_FORMAT"<<<<< \n", __func__, nbytes, event_n, event.event_mask, GST_TIME_ARGS((event.ts)), GST_TIME_ARGS(GST_BUFFER_PTS(stream->current_buffer)), GST_TIME_ARGS((gst->gst_pipeline->basetime)));

				rc = avb_stream_h264_send(stream->stream_h, stream->current_data, nbytes, &event, event_n);
			} else {
				// TODO use iov version of API
				rc = avb_stream_send(stream->stream_h, stream->current_data, nbytes, &event, event_n);
			}

			if (rc != nbytes) {
				if (rc < 0) {
					if (rc == -GENAVB_ERR_STREAM_TX_NOT_ENOUGH_DATA) {
						/* This h264 specific, meaning that we sent less data than the API needs to decide on the packetization mode
						 * and we need to come back later (next poll) with more data (a batch size) or the full NALU*/
						rc = 0;
					} else {
						printf("%s failed: %s \n",
								(stream->params.format.u.s.subtype_u.cvf.subtype == CVF_FORMAT_SUBTYPE_H264) ? "avb_stream_h264_send" : "avb_stream_send",
								avb_strerror(rc));

						stream->current_size = 0;
						goto exit_unmap;
					}
				}

				printf("%s incomplete (sent %d instead of %d) \n",
						(stream->params.format.u.s.subtype_u.cvf.subtype == CVF_FORMAT_SUBTYPE_H264) ? "avb_stream_h264_send" : "avb_stream_send",
						rc, nbytes);

				nbytes = rc;

				/* Incomplete send, lets wait again before re-send */
				stream->byte_count += nbytes;
				written += nbytes;
				remaining -= nbytes;
				stream->current_size -= nbytes;
				stream->current_data += nbytes;
				break;
			}

			stream->byte_count += nbytes;
		}

		written += nbytes;
		remaining -= nbytes;
		stream->current_size -= nbytes;
		stream->current_data += nbytes;

		if (stream->current_size == 0) {
			gst_buffer_unmap(stream->current_buffer, &stream->current_info);
			gst_sample_unref(stream->current_sample);
			stream->current_data = NULL;
		}
	}

	rc = written;

	goto exit;

exit_unmap:
	gst_buffer_unmap(stream->current_buffer, &stream->current_info);

exit_unref:
	gst_sample_unref(stream->current_sample);
	rc = -1;

exit:
	return rc;
}

int talker_gst_multi_fsm(struct talker_gst_media *gst, enum gst_event event)
{
	GstAppSinkCallbacks callbacks = { NULL };
	enum gst_state old_state = gst->state;
	unsigned int print = 1;
	unsigned int active;
	unsigned int async_msg_received;
	int i;
	int rc = 0;

start:
	switch (gst->state) {
	case GST_STATE_STARTING:

		if (gst_start_pipeline(gst->gst_pipeline, GST_PRIORITY, GST_DIRECTION_TALKER) < 0) {
			printf("gst_start_pipeline() failed\n");
			rc = -1;
			break;
		}

		for (i = 0; i < gst->gst_pipeline->definition->num_sinks; i++) {
			struct talker_gst_multi_app *stream = gst->gst_pipeline->sink[i].data;

			stream->sink = stream->gst->gst_pipeline->sink[stream->sink_index].sink;
			gst_app_sink_set_max_buffers(stream->sink, 1000);
			pthread_mutex_lock(&stream->samples_lock);
			stream->samples = 0;
			pthread_mutex_unlock(&stream->samples_lock);
			callbacks.new_sample = talker_gst_multi_new_sample_handler;
			gst_app_sink_set_callbacks(stream->sink, &callbacks, stream, NULL);
		}

		gst->state = GST_STATE_STARTED;
		/* fall through */

	case GST_STATE_STARTED:
		switch (event) {
		case GST_EVENT_STOP:
			/* Check that all streams are in DISCONNECTED state, then switch to STOPPED */
			active = 0;
			for (i = 0; i < gst->gst_pipeline->definition->num_sinks; i++) {
				struct talker_gst_multi_app *stream = gst->gst_pipeline->sink[i].data;

				if ((stream->state != STREAM_STATE_DISCONNECTED) && (stream->state != STREAM_STATE_STOPPED))
					active = 1;
			}

			if (active)
				break;

			gst_stop_pipeline(gst->gst_pipeline);
			gst->state = GST_STATE_STOPPED;

			break;

		case GST_EVENT_LOOP:
			/* Check that no stream is in CONNECTED/WAIT_FOR_DATA state, then switch to LOOP */
			active = 0;
			for (i = 0; i < gst->gst_pipeline->definition->num_sinks; i++) {
				struct talker_gst_multi_app *stream = gst->gst_pipeline->sink[i].data;

				if ((stream->state != STREAM_STATE_DISCONNECTED) &&
				    (stream->state != STREAM_STATE_LOOP))
				active = 1;
			}

			if (active)
				break;

			gst_stop_pipeline(gst->gst_pipeline);
			gst->timer_count = 0;
			gst->state = GST_STATE_LOOP;

			break;

		case GST_EVENT_TIMER:
			print = 0;

			/* Read and drop buffers for DISCONNECTED streams */
			for (i = 0; i < gst->gst_pipeline->definition->num_sinks; i++) {
				struct talker_gst_multi_app *stream = gst->gst_pipeline->sink[i].data;
				pthread_mutex_lock(&stream->gst->gst_pipeline->msg_lock);
				async_msg_received = stream->gst->gst_pipeline->async_msg_received;
				pthread_mutex_unlock(&stream->gst->gst_pipeline->msg_lock);

				if (stream->state == STREAM_STATE_DISCONNECTED && async_msg_received)
					rc = talker_gst_multi_data_handler((struct talker_gst_multi_app *)gst->gst_pipeline->sink[i].data);
			}

			break;

		case GST_EVENT_START:
			break;

		default:
			rc = -1;
			break;
		}

		break;

	case GST_STATE_STOPPED:
		switch (event) {
		case GST_EVENT_START:
			gst->state = GST_STATE_STARTING;
			goto start;

			break;

		case GST_EVENT_TIMER:
			print = 0;
			break;

		default:
			rc = -1;
			break;
		}

		break;

	case GST_STATE_LOOP:
		switch (event) {
		case GST_EVENT_TIMER:
//			printf("%s timer %d %d\n", __func__, gst->timer_count, PIPELINE_LOOP_TIMEOUT);

			/* Wait before restarting the pipeline and setting stream state */
			if (gst->timer_count++ > PIPELINE_LOOP_TIMEOUT) {
				for (i = 0; i < gst->gst_pipeline->definition->num_sinks; i++) {
					struct talker_gst_multi_app *stream = gst->gst_pipeline->sink[i].data;

					if (stream->state == STREAM_STATE_LOOP)
						talker_gst_multi_stream_fsm(stream, STREAM_EVENT_LOOP_DONE);
				}

				break;
			} else
				print = 0;

			break;

		case GST_EVENT_STOP:
			/* Check that all streams are in DISCONNECTED state, then switch to STOPPED */
			active = 0;
			for (i = 0; i < gst->gst_pipeline->definition->num_sinks; i++) {
				struct talker_gst_multi_app *stream = gst->gst_pipeline->sink[i].data;

				if ((stream->state != STREAM_STATE_DISCONNECTED) && (stream->state != STREAM_STATE_STOPPED))
					active = 1;
			}

			if (active)
				break;

			gst->state = GST_STATE_STOPPED;

			break;

		case GST_EVENT_START:
			gst->state = GST_STATE_STARTING;
			goto start;

			break;

		default:
			rc = -1;
			break;
		}

		break;

	default:
		rc = -1;
		break;
	}

	if (print || rc < 0)
		printf("%s %p old state %s current state %s event %s rc %d\n", __func__, gst, gst_state[old_state], gst_state[gst->state], gst_event[event], rc);

	return rc;
}


int talker_gst_multi_stream_fsm(struct talker_gst_multi_app *stream, enum stream_event event)
{
	enum stream_state old_state = stream->state;
	unsigned int print = 1;
	unsigned int async_msg_received;
	int rc = 0;

start:
	switch (stream->state) {
	case STREAM_STATE_DISCONNECTING:
		stream->state = STREAM_STATE_DISCONNECTED;

		if (talker_gst_multi_fsm(stream->gst, GST_EVENT_STOP) < 0) {
			rc = -1;
			break;
		}
		/* fall through */

	case STREAM_STATE_DISCONNECTED:
		switch (event) {
		case STREAM_EVENT_CONNECT:
			stream->state = STREAM_STATE_CONNECTING;
			goto start;
			break;

		case STREAM_EVENT_TIMER:
			print = 0;
			break;

		case STREAM_EVENT_DISCONNECT:
			break;

		default:
			rc = -1;
			break;
		}

		break;

	case STREAM_STATE_CONNECTING:

		if (talker_gst_multi_fsm(stream->gst, GST_EVENT_START) < 0) {
			rc = -1;
			break;
		}

		stream->byte_count = 0;
		stream->count = 0;
		stats_init(&stream->delay, 9, stream, stream_stats_show);
		stream->state = STREAM_STATE_WAIT_START;

		break;

	case STREAM_STATE_CONNECTED:
		switch (event) {
		case STREAM_EVENT_TIMER:
			print = 0;
			break;

		case STREAM_EVENT_DATA:
			print = 0;

			rc = talker_gst_multi_data_handler(stream);

			if (!rc) {
				/* No data read */
				stream->state = STREAM_STATE_WAIT_DATA;
				stream->timer_count = 0;
				stream->stream_poll_set(stream->stream_poll_data, 0);
			} else if (rc < 0) {
				stream->state = STREAM_STATE_DISCONNECTING;
				stream->stream_poll_set(stream->stream_poll_data, 0);
				printf("[ERROR] %s: stream(%d) encountered a fatal error while writing data to stack ... Disconnect \n", __func__, stream->index);
				goto start;
			}

			break;

		case STREAM_EVENT_DISCONNECT:
			stream->state = STREAM_STATE_DISCONNECTING;
			goto start;
			break;

		case STREAM_EVENT_CONNECT:
			break;

		case STREAM_EVENT_STOP:
			stream->state = STREAM_STATE_STOPPED;
			if (talker_gst_multi_fsm(stream->gst, GST_EVENT_STOP) < 0) {
				rc = -1;
				break;
			}

			stream->stream_poll_set(stream->stream_poll_data, 0);
			talker_stream_flush(stream->stream_h, &stream->params);
			break;

		case STREAM_EVENT_PLAY:
			print = 0;
			break;

		default:
			rc = -1;
			break;
		}

		break;

	case STREAM_STATE_WAIT_DATA:
		switch (event) {
		case STREAM_EVENT_TIMER:
			print = 0;

			pthread_mutex_lock(&stream->samples_lock);
			rc = stream->samples;
			pthread_mutex_unlock(&stream->samples_lock);

			if (rc > 0) {
				stream->timer_count = 0;
				stream->state = STREAM_STATE_CONNECTED;
				stream->stream_poll_set(stream->stream_poll_data, 1);
			} else {
				stream->timer_count++;

				/* Pipeline was already launched, we are likely looping at the end
				 * Flush the stream ASAP,so that a "looping" listener don't get
				 * a late sample from previous file as a first new sample */
				if ( stream->timer_count == STREAM_FLUSH_TIMEOUT)
					talker_stream_flush(stream->stream_h, &stream->params);

				if (stream->timer_count > STREAM_WAIT_DATA_TIMEOUT) {
					print = 1;

					stream->state = STREAM_STATE_LOOP;
					printf("Bytes sent: %llu\n", stream->byte_count);
					talker_gst_multi_fsm(stream->gst, GST_EVENT_LOOP);
				}
			}

			break;

		case STREAM_EVENT_DISCONNECT:
			stream->state = STREAM_STATE_DISCONNECTING;
			goto start;
			break;

		case STREAM_EVENT_STOP:
			stream->state = STREAM_STATE_STOPPED;

			if (talker_gst_multi_fsm(stream->gst, GST_EVENT_STOP) < 0) {
				rc = -1;
				break;
			}

			stream->stream_poll_set(stream->stream_poll_data, 0);
			talker_stream_flush(stream->stream_h, &stream->params);

			break;

		case STREAM_EVENT_PLAY:
			print = 0;
			break;

		default:
			rc = -1;
			break;
		}

		break;

	case STREAM_STATE_WAIT_START:
		switch (event) {
		case STREAM_EVENT_DATA:
			print = 0;
			stream->stream_poll_set(stream->stream_poll_data, 0);

			break;
		case STREAM_EVENT_TIMER:
			print = 0;

			pthread_mutex_lock(&stream->gst->gst_pipeline->msg_lock);
			async_msg_received = stream->gst->gst_pipeline->async_msg_received;
			pthread_mutex_unlock(&stream->gst->gst_pipeline->msg_lock);

			if (async_msg_received) {
				stream->timer_count = 0;
				stream->state = STREAM_STATE_WAIT_DATA;
				goto start;
			} else {
				stream->timer_count++;

				/* Waiting for the pipeline to finish its state transition to PLAYING*/
				if (stream->timer_count > STREAM_PIPELINE_START_TIMEOUT) {
					printf("[ERROR] Pipeline taking too long to go to PLAYING... stop it \n");
					print = 1;
					stream->state = STREAM_STATE_STOPPED;
					if (talker_gst_multi_fsm(stream->gst, GST_EVENT_STOP) < 0) {
						rc = -1;
						break;
					}
				}
			}

			break;

		case STREAM_EVENT_DISCONNECT:
			stream->state = STREAM_STATE_DISCONNECTING;
			goto start;
			break;

		case STREAM_EVENT_STOP:
			stream->state = STREAM_STATE_STOPPED;

			if (talker_gst_multi_fsm(stream->gst, GST_EVENT_STOP) < 0) {
				rc = -1;
				break;
			}

			break;

		case STREAM_EVENT_PLAY:
			print = 0;
			break;

		default:
			rc = -1;
			break;
		}

		break;

	case STREAM_STATE_LOOP:
		switch (event) {
		case STREAM_EVENT_TIMER:
			print = 0;
			/* Wait until all streams connected to the same pipeline finish looping */
			break;

		case STREAM_EVENT_DISCONNECT:
			stream->state = STREAM_STATE_DISCONNECTING;
			goto start;

			break;

		case STREAM_EVENT_LOOP_DONE:
			stream->state = STREAM_STATE_CONNECTING;
			goto start;
			break;

		case STREAM_EVENT_STOP:
			stream->state = STREAM_STATE_STOPPED;
			if (talker_gst_multi_fsm(stream->gst, GST_EVENT_STOP) < 0) {
				rc = -1;
				break;
			}

			stream->stream_poll_set(stream->stream_poll_data, 0);
			talker_stream_flush(stream->stream_h, &stream->params);
			break;

		case STREAM_EVENT_PLAY:
			print = 0;
			break;

		default:
			rc = -1;
			break;
		}

		break;

	case STREAM_STATE_STOPPED:
		switch (event) {
		case STREAM_EVENT_TIMER:
			print = 0;
			break;

		case STREAM_EVENT_DISCONNECT:
			stream->state = STREAM_STATE_DISCONNECTING;
			goto start;
			break;

		case STREAM_EVENT_STOP:
			print = 0;
			break;

		case STREAM_EVENT_PLAY:
			stream->state = STREAM_STATE_CONNECTING;
			goto start;
			break;

		default:
			rc = -1;
			break;
		}

		break;

	default:
		rc = -1;
		break;
	}

	if (print || rc < 0)
		printf("%s stream(%d) old %s current %s event %s %d\n",
			__func__, stream->index,
			stream_state[old_state], stream_state[stream->state], stream_event[event], rc);

	return rc;
}
