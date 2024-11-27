/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#include "gst_pipeline_definitions.h"
#include "gstreamer_single.h"


void stream_init_stats(struct gstreamer_stream *stream)
{
	stats_init(&stream->stream_stats.write_delay, 31, stream, NULL);
	stats_init(&stream->stream_stats.delay, 31, stream, NULL);
	stats_init(&stream->stream_stats.period, 31, stream, NULL);
	stats_init(&stream->stream_stats.rate, 31, stream, NULL);
	stats_init(&stream->stream_stats.mpeg_ts.pcr_delay, 31, stream, NULL);
	stats_init(&stream->stream_stats.mpeg_ts.pcr_period, 31, stream, NULL);

	stream->stream_stats.pkt_lost = 0;
	stream->stream_stats.ts_err = 0;
	stream->stream_stats.mr = 0;
	stream->started = 0;
	stream->pipe_source.byte_count = 0;
	stream->pipe_source.dropping = 0;
	stream->stream_stats.byte_count_prev = 0;
	gettime_s_monotonic(&stream->pipe_source.tlast);
	stream->ts_parser.pcr_count = 0;
	stream->pipe_source.count = 0;
	stream->pipe_source.late_count = 0;
	stream->pipe_source.ontime_count = 0;
}

void stream_update_stats(struct gstreamer_stream *stream, unsigned long long byte_count, unsigned long long now, unsigned long long buffer_pts)
{
	int period = 0, rate = 0;

	stats_update(&stream->stream_stats.delay, (int)(buffer_pts - now));

	if (byte_count) {
		period = buffer_pts - stream->stream_stats.buffer_pts_prev;

		if (period) {
			rate = ((byte_count - stream->stream_stats.byte_count_prev) * NSECS_PER_SEC) / period;

			stats_update(&stream->stream_stats.period, period);

			stats_update(&stream->stream_stats.rate, rate);

			stream->stream_stats.buffer_pts_prev = buffer_pts;
			stream->stream_stats.byte_count_prev = byte_count;
		}
	} else {
		stream->stream_stats.buffer_pts_prev = buffer_pts;
		stream->stream_stats.byte_count_prev = byte_count;
	}

	if ((stream->stream_stats.delay.current_count < 100) && (stream->pipe_source.byte_count < 1000000)) {
		printf("now: %"GST_TIME_FORMAT"(%u) count: %llu ts: %"GST_TIME_FORMAT"(%u) delay: %"GST_STIME_FORMAT" period: %d rate: %d\n",
			GST_TIME_ARGS(now), (unsigned int)now,
			byte_count,
			GST_TIME_ARGS(buffer_pts), (unsigned int)buffer_pts,
			GST_STIME_ARGS((long long)buffer_pts - (long long)now),
			period,
			rate);
	}
}

void stream_dump_stats(struct gstreamer_stream *stream)
{
	stats_compute(&stream->stream_stats.write_delay);
	stats_compute(&stream->stream_stats.delay);
	stats_compute(&stream->stream_stats.period);
	stats_compute(&stream->stream_stats.rate);

	printf("stream(%u): write delay(ns): %7d/%7d/%7d delay(ns): %9d/%9d/%9d period(ns): %9d/%9d/%9d rate(bytes/s): %7d/%7d/%7d pkt_lost: %7llu ts_err: %7llu mr: %7llu\n", stream->source_index,
		stream->stream_stats.write_delay.min, stream->stream_stats.write_delay.mean, stream->stream_stats.write_delay.max,
		stream->stream_stats.delay.min, stream->stream_stats.delay.mean, stream->stream_stats.delay.max,
		stream->stream_stats.period.min, stream->stream_stats.period.mean, stream->stream_stats.period.max,
		stream->stream_stats.rate.min, stream->stream_stats.rate.mean, stream->stream_stats.rate.max,
		stream->stream_stats.pkt_lost, stream->stream_stats.ts_err, stream->stream_stats.mr
	);

	stats_reset(&stream->stream_stats.write_delay);
	stats_reset(&stream->stream_stats.delay);
	stats_reset(&stream->stream_stats.period);
	stats_reset(&stream->stream_stats.rate);

	if (avdecc_format_is_61883_4(&stream->params.format)) {
		stats_compute(&stream->stream_stats.mpeg_ts.pcr_delay);
		stats_compute(&stream->stream_stats.mpeg_ts.pcr_period);

		printf("stream(%u): pcr delay(ns): (%lld) %7d/%7d/%7d period(ns): %7d/%7d/%7d\n", stream->source_index,
			stream->ts_parser.buffer_pts0 - stream->ts_parser.pcr0,
			stream->stream_stats.mpeg_ts.pcr_delay.min, stream->stream_stats.mpeg_ts.pcr_delay.mean, stream->stream_stats.mpeg_ts.pcr_delay.max,
			stream->stream_stats.mpeg_ts.pcr_period.min, stream->stream_stats.mpeg_ts.pcr_period.mean, stream->stream_stats.mpeg_ts.pcr_period.max
		);

		stats_reset(&stream->stream_stats.mpeg_ts.pcr_delay);
		stats_reset(&stream->stream_stats.mpeg_ts.pcr_period);
	}
}

void dump_stream_infos(struct gstreamer_stream *stream)
{
	print_stream_id(stream->params.stream_id);

	printf("mode: %s\n", (stream->params.direction == AVTP_DIRECTION_LISTENER)? "LISTENER":"TALKER");
}

void stream_61883_4_update_stats(struct gstreamer_stream *stream, unsigned char *buf, unsigned long long buffer_pts)
{
	unsigned long long pcr;
	int period;

	if (ts_parser_is_pcr(buf, &pcr)) {

		if (!stream->ts_parser.pcr_count) {
			stream->ts_parser.pcr0 = pcr;
			stream->ts_parser.buffer_pts0 = buffer_pts;
		}

		stats_update(&stream->stream_stats.mpeg_ts.pcr_delay, (pcr - stream->ts_parser.pcr0) - (buffer_pts - stream->ts_parser.buffer_pts0));

		if (stream->ts_parser.pcr_count > 1) {
			period = pcr - stream->stream_stats.mpeg_ts.pcr_prev;
			stats_update(&stream->stream_stats.mpeg_ts.pcr_period, period);
		} else
			period = 0;

		if (stream->ts_parser.pcr_count < 10) {
			printf("pcr(ns): %llu(%lld) ts(ns): %llu(%lld) pcr delay(ns): %lld period(ns): %d\n",
				stream->ts_parser.pcr0, pcr - stream->ts_parser.pcr0,
				stream->ts_parser.buffer_pts0, buffer_pts - stream->ts_parser.buffer_pts0,
				(pcr - stream->ts_parser.pcr0) - (buffer_pts - stream->ts_parser.buffer_pts0), period);
		}

		stream->stream_stats.mpeg_ts.pcr_prev = pcr;
		stream->ts_parser.pcr_count++;
	}
}


#define CVF_MJPEG_SPLASH_CAPTURE 0

#if CVF_MJPEG_SPLASH_CAPTURE
/* Splash screen file format: records of 32-bit frame_size followed by frame_size bytes
 */
#define CVF_MJPEG_SPLASH_CAPTURE_THRESHOLD CVF_MJPEG_SPLASH_MAX_FRAME_SIZE
#define CVF_MJPEG_CAPTURE_FILENAME "/tmp/cvf_mjpeg_splash_screen.mjpg"

static void cvf_mjpeg_splash_capture(struct gstreamer_stream *stream)
{
	static unsigned int capture_splash_previous_frame_size = 0;
	static int capture_splash_fd = -1;
	unsigned int frame_size = stream->pipe_source.buffer_byte_count;

	// Only save the splash screen from the first camera
	if (stream->source_index == 0) {

		// Start a new splash screen
		if ((capture_splash_previous_frame_size > CVF_MJPEG_SPLASH_CAPTURE_THRESHOLD) && (frame_size < CVF_MJPEG_SPLASH_CAPTURE_THRESHOLD)) {
			printf("Starting splash screen\n");
			capture_splash_fd = open(CVF_MJPEG_CAPTURE_FILENAME, O_WRONLY | O_CREAT | O_TRUNC);
			if (capture_splash_fd < 0)
				printf("stream(%u): Could not open capture file\n", stream->source_index);
		}

		// Continuing the current splash screen
		if ((frame_size < CVF_MJPEG_SPLASH_CAPTURE_THRESHOLD) && (capture_splash_fd >= 0)) {
			write(capture_splash_fd, &frame_size, 4);
			write(capture_splash_fd, stream->pipe_source.info.data, frame_size);
		}

		// Close the current splash screen
		if ((capture_splash_previous_frame_size < CVF_MJPEG_SPLASH_CAPTURE_THRESHOLD) && (frame_size > CVF_MJPEG_SPLASH_CAPTURE_THRESHOLD)) {
			close(capture_splash_fd);
			printf("Closing splash screen\n");
		}

		capture_splash_previous_frame_size = frame_size;
		//printf("stream(%d) frame size %d\n", stream->source_index, stream->gst_stream.buffer_byte_count);
	}

}
#endif

static int cvf_mjpeg_splash_read_frame(int fd, unsigned char *buf, unsigned int *frame_size)
{
	int rc;

	/*Read first 4 bytes containing the frame size*/
	rc = read(fd, frame_size, 4);
	if ( rc != 4 ) {
		printf("Can not read frame size (%d, expected 4)\n", rc);
		return -1;
	}

	if (*frame_size > CVF_MJPEG_SPLASH_MAX_FRAME_SIZE) {
		printf("Frame size read (%u) exceeding max %u \n", *frame_size, CVF_MJPEG_SPLASH_MAX_FRAME_SIZE);
		return -1;
	}

	rc = read(fd, buf, *frame_size);
	if (rc != *frame_size) {
		printf("Short read while warming up Gstreamer pipeline (%d, expected %d)\n", rc, *frame_size);
		return -1;
	}

	return 0;
}

int gst_cvf_mjpeg_warm_up_pipeline(struct gstreamer_pipeline *gst, unsigned int nframes)
{
	GstBuffer *buffer;
	GstMapInfo info;
	GstState state = GST_STATE_NULL;
	unsigned char *data_buf;
	int splash_fd;
	int rc = 0;
	unsigned char *buf;
	unsigned int frame_size, i, count;
	unsigned long long gsttime = 0;

	printf("Warming up pipeline\n");

	buf = malloc(CVF_MJPEG_SPLASH_MAX_FRAME_SIZE);
	if (!buf) {
		printf("Couldn't not allocate buffer to warm up Gstreamer pipeline\n");
		return -1;
	}

	splash_fd = open(CVF_MJPEG_SPLASH_FILENAME, O_RDONLY);
	if (splash_fd < 0) {
		printf("Couldn't not warm up Gstreamer pipeline: %s(%d)\n", strerror(errno), errno);
		rc = 0;
		goto err_open;
	}


	// Wait for pipeline to become ready.
	rc = gst_element_get_state(gst->pipeline, &state, NULL, 0);
	while ((state != GST_STATE_PLAYING) && (state != GST_STATE_PAUSED)) {
		usleep(10000); // 10ms
		rc = gst_element_get_state(gst->pipeline, &state, NULL, 0);

	}
	printf("Pipeline ready (state %d)\n", state);

	gsttime = gst_clock_get_time(gst->clock) - gst_element_get_base_time(GST_ELEMENT(gst->pipeline));

	if (cvf_mjpeg_splash_read_frame(splash_fd, buf, &frame_size) < 0) {
		printf("Error reading first frame \n");
		rc = -1;
		goto exit;
	}

	count = 0;
	while (count < nframes) {
		printf ("Reading frame %d ts %llu base_time %" G_GUINT64_FORMAT "  now %" G_GUINT64_FORMAT "\n",
				count, gsttime, gst_element_get_base_time(GST_ELEMENT(gst->pipeline)), gst_clock_get_time(gst->clock));

		if (cvf_mjpeg_splash_read_frame(splash_fd, buf, &frame_size) < 0) {
			printf("Error reading frame %d \n", count);
			rc = -1;
			goto exit;
		}

		for (i = 0; i < gst->definition->num_sources; i++) {
			/* Create a new empty buffer */
			buffer = gst_buffer_new_allocate(NULL, frame_size, NULL);
			if (!buffer) {
				printf("Couldn't allocate Gstreamer buffer\n");
				rc = -1;
				goto exit;
			}

			buffer = gst_buffer_make_writable(buffer);

			if (gst_buffer_map(buffer, &info, GST_MAP_WRITE))
				data_buf = info.data;
			else
				data_buf = NULL;

			if (!data_buf) {
				printf("Couldn't get data buffer\n");

				gst_buffer_unref(buffer);

				rc = -1;
				goto exit;
			}

			/* copy data from file
			 */
			memcpy(data_buf, buf, frame_size);
			gst_buffer_unmap(buffer, &info);
			gst_buffer_set_size(buffer, frame_size);


			GST_BUFFER_PTS(buffer) = gsttime + MJPEG_PIPELINE_LATENCY; // Arbitrary delay

			rc = gst_app_src_push_buffer(gst->source[i].source, buffer);
			if (rc != GST_FLOW_OK) {
				if (rc == GST_FLOW_FLUSHING)
					printf("Pipeline not in PAUSED or PLAYING state\n");
				else
					printf("End-of-Stream occurred\n");

				rc = -1;
				goto exit;
			} else
				rc = 1;
		}

		gsttime += NSECS_PER_SEC / CVF_MJPEG_SPLASH_FPS;
		usleep(USECS_PER_SEC / CVF_MJPEG_SPLASH_FPS);
		count++;
	}



exit:
	close(splash_fd);
err_open:
	free(buf);

	return rc;
}

#define CVF_TS_VALID_WINDOW 35000000 // 35ms
int listener_gst_handler_cvf(struct gstreamer_stream *stream, unsigned int events)
{
	struct gstreamer_pipeline *gst = stream->pipe_source.gst_pipeline;
	unsigned int event_len;
	struct avb_event event[EVENT_BUF_SZ];
	int nbytes;
	int rc = 1;
	unsigned long long gsttime;
	unsigned long long base_ts;
	avb_s32 ts_diff;
	unsigned int pushed = 0;
	uint64_t start = 0, end = 0;
	time_t tnow;

	tnow = time(NULL);
	if ((tnow - stream->pipe_source.tlast) > 10) {
		stream_dump_stats(stream);
		stream->pipe_source.tlast = tnow;
	}

	gettime_ns(&start);

	while (pushed < stream->batch_size) {
		if (!(stream->pipe_source.count % 10000))
			printf("stream(%u): bytes: %llu\n", stream->source_index, stream->pipe_source.byte_count);

		stream->pipe_source.count++;

		/* Create a new empty buffer if needed */
		if (!stream->pipe_source.buffer) {
			stream->pipe_source.buffer = gst_buffer_new();
			if (!stream->pipe_source.buffer) {
				printf("stream(%u): Couldn't allocate Gstreamer buffer\n", stream->source_index);
				//gst_memory_unref(memory);
				rc = -1;
				goto exit;
			}
		}

		if (!stream->pipe_source.memory) {
			stream->pipe_source.memory = gst_allocator_alloc(NULL, GST_MEMORY_OBJ_SIZE, NULL);
			if (!stream->pipe_source.memory) {
				printf("stream(%u): Couldn't allocate Gstreamer memory object\n", stream->source_index);
				gst_buffer_unref(stream->pipe_source.buffer);
				rc = -1;
				goto exit;
			}

			if (gst_memory_map(stream->pipe_source.memory, &stream->pipe_source.info, GST_MAP_WRITE))
				stream->pipe_source.data_buf = stream->pipe_source.info.data;
			else
				stream->pipe_source.data_buf = NULL;

			if (!stream->pipe_source.data_buf) {
				printf("stream(%u): Couldn't get data buffer\n", stream->source_index);
				gst_buffer_unref(stream->pipe_source.buffer);
				gst_memory_unref(stream->pipe_source.memory);
				rc = -1;
				goto exit;
			}

			gst_buffer_append_memory(stream->pipe_source.buffer, stream->pipe_source.memory);
		}

		stream->pipe_source.buffer = gst_buffer_make_writable(stream->pipe_source.buffer);


		/* read data from stack...
		 * TODO: make a single call to genavb lib with an iovec of gst buffers
		 */
		event_len = EVENT_BUF_SZ;
		nbytes = avb_stream_receive(stream->stream_h, stream->pipe_source.data_buf, GST_MEMORY_OBJ_SIZE - stream->pipe_source.memory_byte_count, event, &event_len);
		//printf("Received %d bytes from genavb ... \n", nbytes);

		if (nbytes < 0) {
			printf("stream(%u): avb_stream_receive() failed: %s\n", stream->source_index, avb_strerror(nbytes));

			stream->pipe_source.buffer_byte_count += stream->pipe_source.memory_byte_count;
			gst_memory_unmap(stream->pipe_source.memory, &stream->pipe_source.info);
			gst_buffer_unref(stream->pipe_source.buffer);
			stream->pipe_source.buffer_byte_count = 0;
			stream->pipe_source.memory_byte_count = 0;
			stream->pipe_source.buffer = NULL;
			stream->pipe_source.memory = NULL;

			rc = nbytes;
			goto exit;
		}
		if (nbytes == 0) {
			goto exit;
		}

		stream->data_received = 1;

		if (!stream->pipe_source.dropping) {
			pushed += nbytes;
			stream->pipe_source.data_buf += nbytes;
			stream->pipe_source.memory_byte_count += nbytes;
		}

		gsttime = gst_clock_get_time(gst->clock);
		if (gsttime ==  gst->time) {
			printf("ERROR: Clock jumped into the past, Gstreamer pipeline likely stalled...\n");
			//FIXME
		}
		gst->time = gsttime;

		/* FIXME ? Doesn't filter packets that are too late or too early, which usually occur when gptp has been disrupted*/
		if (event_len != 0) {
			// printf("stream(%u) event_len(%d) mask0 0x%x maskn 0x%x size %d nbytes %d\n",stream->source_index,
			//		event_len, event[0].event_mask, event[event_len - 1].event_mask, stream->gst_stream.buffer_byte_count, nbytes);

			if (event[0].event_mask & AVTP_MEDIA_CLOCK_RESTART) {
				stream->pipe_source.dropping = 1;
				stream->stream_stats.mr++;
			}

			if (event[0].event_mask & AVTP_PACKET_LOST) {
				stream->pipe_source.dropping = 1;
				stream->stream_stats.pkt_lost++;
			}

			if (event[event_len - 1].event_mask & AVTP_END_OF_FRAME) {
				stream->pipe_source.buffer_byte_count += stream->pipe_source.memory_byte_count;
#if CVF_MJPEG_SPLASH_CAPTURE
				cvf_mjpeg_splash_capture(stream);
#endif

				gst_memory_unmap(stream->pipe_source.memory, &stream->pipe_source.info);
				gst_memory_resize(stream->pipe_source.memory, 0, stream->pipe_source.memory_byte_count);
				gst_buffer_set_size(stream->pipe_source.buffer, stream->pipe_source.buffer_byte_count);

				if (!(event[event_len - 1].event_mask & AVTP_TIMESTAMP_INVALID)) {
					ts_diff = (avb_s32)event[event_len - 1].ts - (avb_s32)(gsttime & 0xffffffff) + SALSA_LATENCY;
					if ((stream->params.format.u.s.subtype_u.cvf.subtype == CVF_FORMAT_SUBTYPE_H264) || ((ts_diff > (-CVF_TS_VALID_WINDOW / 2)) && (ts_diff < (CVF_TS_VALID_WINDOW / 2)))) {
						base_ts = gsttime & 0xffffffff00000000;
						GST_BUFFER_PTS(stream->pipe_source.buffer) = base_ts | event[event_len - 1].ts;
						/* Handle 32bit wrap */
						if (avtp_after(event[event_len - 1].ts, gsttime & 0xffffffff)) {
							/* Timestamp in the future */
							if ((event[event_len - 1].ts < (gsttime & 0xffffffff)))
								GST_BUFFER_PTS(stream->pipe_source.buffer) += 0x100000000ULL;
						} else {
							/* Timestamp in the past */
							if ((event[event_len - 1].ts > (gsttime & 0xffffffff)))
								GST_BUFFER_PTS(stream->pipe_source.buffer) -= 0x100000000ULL;
						}
					} else {
						GST_BUFFER_PTS(stream->pipe_source.buffer) = gsttime - stream->pipe_source.gst_pipeline->listener.local_pts_offset;
						stream->stream_stats.ts_err++;
					}
				}

				if (event[event_len - 1].event_mask & (AVTP_TIMESTAMP_INVALID | AVTP_TIMESTAMP_UNCERTAIN)) {
					GST_BUFFER_PTS(stream->pipe_source.buffer) = gsttime - stream->pipe_source.gst_pipeline->listener.local_pts_offset;
					stream->stream_stats.ts_err++;
				}

				GST_BUFFER_PTS(stream->pipe_source.buffer) += stream->pipe_source.gst_pipeline->listener.local_pts_offset;

				stream_update_stats(stream, stream->pipe_source.byte_count, gsttime, GST_BUFFER_PTS(stream->pipe_source.buffer));

				if (!GST_CLOCK_TIME_IS_VALID(gst->basetime))
					gst->basetime = gst_element_get_base_time(GST_ELEMENT(gst->pipeline));

				if (GST_BUFFER_PTS(stream->pipe_source.buffer) < gst->basetime) {
					printf("stream(%u): frame in the past, dropping (frame %" G_GUINT64_FORMAT ", basetime %" G_GUINT64_FORMAT " now %llu)\n",
							stream->source_index, GST_BUFFER_PTS(stream->pipe_source.buffer), gst->basetime, gsttime);
					stream->pipe_source.dropping = 1;
					rc = 1;
				} else {
					GST_BUFFER_PTS(stream->pipe_source.buffer) -= gst->basetime;
				}

				stream->pipe_source.byte_count += stream->pipe_source.buffer_byte_count;

				// printf("stream(%u): Handling frame with PTS %llu, basetime %llu now %llu size %d dropping %d)\n",
				//	stream->source_index, GST_BUFFER_PTS(stream->gst_stream.buffer), gst->basetime, gsttime, stream->gst_stream.buffer_byte_count, stream->gst_stream.dropping);

				if (stream->params.format.u.s.subtype_u.cvf.subtype != CVF_FORMAT_SUBTYPE_H264 && ((gsttime - stream->previous_frame_time) < 25000000))
					stream->pipe_source.dropping = 1;
				if (!stream->pipe_source.dropping) {
					stream->previous_frame_time = gst->time;
					GST_BUFFER_DURATION(stream->pipe_source.buffer) = GST_CLOCK_TIME_NONE;
					rc = gst_app_src_push_buffer(stream->pipe_source.source, stream->pipe_source.buffer);
					//printf("stream(%d): Pushed %d bytes to Gstreamer ts %llu ts_diff %d\n", stream->source_index, stream->pipe_source.buffer_byte_count, GST_BUFFER_PTS(stream->pipe_source.buffer), ts_diff);

					if (rc != GST_FLOW_OK) {
						if (rc == GST_FLOW_FLUSHING)
							printf("stream(%u): Pipeline not in PAUSED or PLAYING state\n", stream->source_index);
						else
							printf("stream(%u): End-of-Stream occurred\n", stream->source_index);

						rc = -1;
					} else {
						rc = 1;
					}
				} else {
					gst_buffer_unref(stream->pipe_source.buffer);
					stream->pipe_source.dropping = 0;
				}

				stream->pipe_source.buffer_byte_count = 0;
				stream->pipe_source.memory_byte_count = 0;
				stream->pipe_source.buffer = NULL;
				stream->pipe_source.memory = NULL;

				goto exit; // We reached an end-of-frame, so there is not much data left and we might as well go back to sleep until another full batch.

			}
		}

		if (stream->pipe_source.memory_byte_count  >= (GST_MEMORY_OBJ_SIZE - MAX_PKT_SIZE_CVF)) {
			gst_memory_unmap(stream->pipe_source.memory, &stream->pipe_source.info);
			gst_memory_resize(stream->pipe_source.memory, 0, stream->pipe_source.memory_byte_count);
			stream->pipe_source.buffer_byte_count += stream->pipe_source.memory_byte_count;
			stream->pipe_source.memory_byte_count = 0;
			stream->pipe_source.memory = NULL;
			printf("stream(%u) Current buffer byte count: %d\n", stream->source_index, stream->pipe_source.buffer_byte_count);
		}
	}

exit:
	gettime_ns(&end);

	stats_update(&stream->stream_stats.write_delay, end - start);

	return rc;
}

int listener_gst_handler(struct gstreamer_stream *stream, unsigned int events)
{
	struct gstreamer_pipeline *gst = stream->pipe_source.gst_pipeline;
	unsigned int event_len;
	GstBuffer *buffer;
	GstMapInfo info;
	unsigned char *data_buf;
	struct avb_event event[EVENT_BUF_SZ];
	int nbytes;
	int rc = 0;
	unsigned long long base_ts;
	unsigned long long gsttime;
	unsigned int pushed = 0;
	uint64_t start = 0, end = 0, tnow;
	int delta;
	uint64_t ts_offset;

	gettime_ns_monotonic(&start);
	tnow = start / NSECS_PER_SEC;
	if ((tnow - stream->pipe_source.tlast) > 10) {
		stream_dump_stats(stream);
		stream->pipe_source.tlast = tnow;
	}

	while (pushed != stream->batch_size) {
		if (!(stream->pipe_source.count % 10000))
			printf("bytes: %llu\n", stream->pipe_source.byte_count);

		stream->pipe_source.count++;

		/* Create a new empty buffer */
		buffer = gst_buffer_new_allocate(NULL, stream->frame_size, NULL);
		if (!buffer) {
			printf("Couldn't allocate Gstreamer buffer\n");
			rc = -1;
			goto exit;
		}

		buffer = gst_buffer_make_writable(buffer);

		if (gst_buffer_map(buffer, &info, GST_MAP_WRITE))
			data_buf = info.data;
		else
			data_buf = NULL;

		if (!data_buf) {
			printf("Couldn't get data buffer\n");

			gst_buffer_unref(buffer);

			rc = -1;
			goto exit;
		}

		/* read data from stack...
		 * TODO: make a single call to genavb lib with an iovec of gst buffers
		 */
		event_len = EVENT_BUF_SZ;
		nbytes = avb_stream_receive(stream->stream_h, data_buf, stream->frame_size, event, &event_len);
		gst_buffer_unmap(buffer, &info);
		if (nbytes <= 0) {
			if (nbytes < 0)
				printf("avb_stream_receive() failed: %s\n", avb_strerror(nbytes));
			else
				printf("avb_stream_receive() incomplete\n");

			gst_buffer_unref(buffer);

			rc = nbytes;
			goto exit;
		}

		stream->data_received = 1;

		if (nbytes != stream->frame_size)
			printf("Short read: GenAVB returned less data (%d) than requested (%d).\n", nbytes, stream->frame_size);

		gst_buffer_set_size(buffer, nbytes);

		if (event[0].event_mask & AVTP_MEDIA_CLOCK_RESTART)
			printf ("AVTP media clock restarted\n");

		if (event[0].event_mask & AVTP_PACKET_LOST)
			printf ("AVTP packet lost\n");

		if (!(event[0].event_mask & (AVTP_TIMESTAMP_INVALID | AVTP_TIMESTAMP_UNCERTAIN))) {
			gsttime = gst_clock_get_time(gst->clock);
			if (gsttime == gst->time) {
				printf("Error: clock likely jumped into the past, restarting pipeline\n");
				gst_buffer_unref(buffer);

				gst_stop_pipeline(gst);
				gst_start_pipeline(gst, GST_PRIORITY, GST_DIRECTION_LISTENER);
				rc = -1;
				goto exit;
			} else
				gst->time = gsttime;
			base_ts = gsttime & 0xffffffff00000000;

			if (!GST_CLOCK_TIME_IS_VALID(gst->basetime))
				gst->basetime = gst_element_get_base_time(GST_ELEMENT(gst->pipeline));

			/*Calculate the ts shift in ns based on the timestamp index for ieee61888_6 format*/
			if (avdecc_format_is_61883_6(&stream->params.format)){
				ts_offset = gst_util_uint64_scale (event[0].index, GST_SECOND, avdecc_fmt_sample_rate(&stream->params.format) * avdecc_fmt_sample_size(&stream->params.format));
				event[0].ts -= ts_offset;
			}

			delta = (int)event[0].ts - (int)(gsttime & 0xffffffff);

			/* Filter packets that are too late or too early, which usually occur when gptp has been disrupted */
			if ((stream->pipe_source.dropping && ((delta < -AVTP_TS_MIN_DELTA) || (delta > AVTP_TS_MAX_DELTA))) ||
			    (!stream->pipe_source.dropping && ((delta < -AVTP_TS_MIN_DELTA_STOP) || (delta > AVTP_TS_MAX_DELTA_STOP)))
			) {
				stream->pipe_source.late_count++;
				stream->pipe_source.ontime_count = 0;

				if (!stream->pipe_source.dropping) {
					printf("stop playing, late buffer %"GST_TIME_FORMAT" %"GST_TIME_FORMAT" %"GST_STIME_FORMAT"\n",
							GST_TIME_ARGS(event[0].ts), GST_TIME_ARGS((unsigned int)(gsttime & 0xffffffff)), GST_STIME_ARGS(delta));

					stream->pipe_source.dropping = 1;
				}

				gst_buffer_unref(buffer);

				goto exit;
			}

			stream->pipe_source.ontime_count++;

			if (stream->pipe_source.dropping) {
				if (stream->pipe_source.ontime_count > 1000) {
					printf("start playing %llu\n", stream->pipe_source.late_count);

					stream->pipe_source.late_count = 0;
					stream->pipe_source.dropping = 0;
				} else {
					gst_buffer_unref(buffer);

					goto exit;
				}
			}

			// TODO Handle event.index
			GST_BUFFER_PTS(buffer) = base_ts | event[0].ts;

			/* Handle 32bit wrap */
			if (avtp_after(event[0].ts, gsttime & 0xffffffff)) {
				/* Timestamp in the future */
				if ((event[0].ts < (gsttime & 0xffffffff)))
					GST_BUFFER_PTS(buffer) += 0x100000000ULL;
			} else {
				/* Timestamp in the past */
				if ((event[0].ts > (gsttime & 0xffffffff)))
					GST_BUFFER_PTS(buffer) -= 0x100000000ULL;
			}

			stream_update_stats(stream, stream->pipe_source.byte_count + event[0].index, gsttime, GST_BUFFER_PTS(buffer));

			if (avdecc_format_is_61883_4(&stream->params.format))
				stream_61883_4_update_stats(stream, data_buf, GST_BUFFER_PTS(buffer));

			GST_BUFFER_PTS(buffer) += stream->pipe_source.gst_pipeline->listener.local_pts_offset;

			GST_BUFFER_PTS(buffer) -= gst->basetime;
		}

		// TODO: with Gstreamer 1.x, pass a list of buffers in a single call
		rc = gst_app_src_push_buffer(stream->pipe_source.source, buffer);
		if (rc != GST_FLOW_OK) {
			if (rc == GST_FLOW_FLUSHING)
				printf("Pipeline not in PAUSED or PLAYING state\n");
			else
				printf("End-of-Stream occurred\n");

			rc = -1;
			goto exit;
		} else
			rc = 1;

		pushed += nbytes;
		stream->pipe_source.byte_count += nbytes;
	}
exit:
	gettime_ns_monotonic(&end);

	stats_update(&stream->stream_stats.write_delay, end - start);

	if (rc == 0)
		rc = pushed;

	return rc;
}

void apply_stream_params(struct gstreamer_stream *stream, struct avb_stream_params *stream_params)
{
	memcpy(&stream->params, stream_params, sizeof(struct avb_stream_params));


	if (stream_params->direction == AVTP_DIRECTION_TALKER)
		stream->params.talker.latency = max(CFG_TALKER_LATENCY_NS, sr_class_interval_p(stream->params.stream_class) / sr_class_interval_q(stream->params.stream_class));

	stream->params.clock_domain = AVB_CLOCK_DOMAIN_0;

	printf("%s : Setting %s to AVB Clock Domain: AVB_CLOCK_DOMAIN_0 (Received parameters %d) \n", __func__, (stream_params->direction == AVTP_DIRECTION_TALKER) ? "TALKER" : "LISTENER", stream_params->clock_domain);

	stream->flags = AVTP_NONBLOCK;
	stream->previous_frame_time = 0;

	if (!avdecc_format_is_61883_6(&stream_params->format))
		stream->params.flags &= ~AVB_STREAM_FLAGS_MCR;

	switch (stream_params->format.u.s.subtype) {
	case AVTP_SUBTYPE_61883_IIDC:
		stream->batch_size = BATCH_SIZE;
		if (stream_params->format.u.s.subtype_u.iec61883.sf == 0) {
			printf("Unsupported 61883_IIDC format\n");
			break;
		} else
			switch (stream_params->format.u.s.subtype_u.iec61883.fmt) {
			case IEC_61883_CIP_FMT_6:
				stream->frame_size = stream->batch_size;
				break;

			case IEC_61883_CIP_FMT_4:
				stream->frame_size = avdecc_fmt_sample_size(&stream_params->format);
				break;

			case IEC_61883_CIP_FMT_8:
			default:
				printf("Unsupported IEC-61883 format: %d\n", stream_params->format.u.s.subtype);
				break;
			}

		stream->pipe_source.dropping = 0;

		if (stream_params->direction == AVTP_DIRECTION_LISTENER)
			stream->listener_gst_handler = listener_gst_handler;

		break;

#ifdef CFG_AVTP_1722A
	case AVTP_SUBTYPE_CVF:
		stream->batch_size = BATCH_SIZE_CVF;
		stream->pipe_source.dropping = 1; // Always start by dropping frames, to ensure the data passed to Gstreamer starts on a frame boundary (by waiting for an EOF event)

		if (stream_params->direction == AVTP_DIRECTION_LISTENER)
			stream->listener_gst_handler = listener_gst_handler_cvf;

		break;
#endif

	default:
		printf("Unsupported AVTP subtype: %d\n", stream_params->format.u.s.subtype);
		break;
	}
	/* FIX ME: PTS offset can't be too small because CVF case for camera. Need to be fixed if audio/video mode implemented
	if (stream->gst_stream.gst->gst.pts_offset < (stream->gst_stream.gst->gst.pipeline_latency + stream->gst_stream.local_pts_offset)) {
		stream->gst_stream.gst->gst.pts_offset = stream->gst_stream.gst->gst.pipeline_latency + stream->gst_stream.local_pts_offset;
		printf("Warning: PTS offset too small, resetting to %lld ns.\n", stream->gst_stream.gst->gst.pipeline_latency + stream->gst_stream.local_pts_offset);
	}
	stream->gst_stream.gst->gst.pts_offset -= stream->gst_stream.local_pts_offset;
	*/
	stream->pipe_source.buffer = NULL;
	stream->pipe_source.memory = NULL;
	stream->pipe_source.buffer_byte_count = 0;
	stream->pipe_source.memory_byte_count = 0;
}
