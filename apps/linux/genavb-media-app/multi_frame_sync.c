/*
 * Copyright 2017-2018, 2020, 2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>

#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>

#include "multi_frame_sync.h"
#include "../common/thread.h"
#include "../common/time.h"

struct gstreamer_sync {
	GstAppSink *appsink;
	GstClockTime basetime;
	int event_fd;
	int index;
	thr_thread_slot_t *thread_slot;
};

struct multi_frame_sync_handle {
	unsigned int nstreams;
	struct gstreamer_sync sync[MFS_MAX_STREAMS];
	GstBuffer *last_buffer[MFS_MAX_STREAMS];
	GstClockTime last_ts[MFS_MAX_STREAMS];
	GstClockTime last_cluster_time;
	unsigned long long first_wakeup;
	unsigned int ready;
	int (*push_buffers)(void *priv, GstBuffer **buffers);
	void *priv;

};

static struct multi_frame_sync_handle mfs;



#define MFS_FRAME_PERIOD	35000000	// 30 fps + wake-up latency margin
#define MFS_VALID_TIME		400000000	// How long to keep displaying a stale frame before blanking it
#define CVF_TS_VALID_WINDOW 500000000 // 500ms

static void multi_frame_sync(unsigned long long current_wall_time)
{
	unsigned long long current_ts, ts_diff_current, ts_diff;
	int ts_index_sorted[MFS_MAX_STREAMS];
	unsigned int i, j, dummy, first_active, first_valid;
	const unsigned int nstreams = mfs.nstreams;

	/* bubble sort */
	for (i = 0; i < nstreams; i++)
		ts_index_sorted[i] = i;

	for (i = 0; i < nstreams - 1; i++) {
		for (j = i + 1; j < nstreams; j++) {
			if (mfs.last_ts[ts_index_sorted[i]] > mfs.last_ts[ts_index_sorted[j]]) {
				dummy = ts_index_sorted[i];
				ts_index_sorted[i] = ts_index_sorted[j];
				ts_index_sorted[j] = dummy;
			}
		}
	}

	current_ts = mfs.last_ts[ts_index_sorted[nstreams - 1]];
	/* filter out inactive streams (last_frame too old) */
	i = 0;
	while ((i < nstreams - 1) && ((mfs.last_ts[ts_index_sorted[i]] + MFS_VALID_TIME) < current_ts)) {
		i++;
	}
	first_valid = i;
	while ((i < nstreams - 1) && ((mfs.last_ts[ts_index_sorted[i]] + MFS_FRAME_PERIOD) < current_ts)) {
		i++;
	}
	first_active = i;

	ts_diff_current = current_ts - mfs.last_ts[ts_index_sorted[first_active]];
	ts_diff = ts_diff_current;

	while (i < nstreams - 1) {
		ts_diff =  mfs.last_ts[ts_index_sorted[i]] + MFS_FRAME_PERIOD - mfs.last_ts[ts_index_sorted[i + 1]];
		/* If mfs.last_ts[ts_index_sorted[i]] is not updated, the above could become negative, so ts_diff(unsigned) will wrap
		 * and the test below will still work.
		 */
		if (ts_diff < ts_diff_current)
			break;
		i++;
	}
	if ((ts_diff >= ts_diff_current) && ((current_ts - mfs.last_cluster_time) > MFS_FRAME_PERIOD / 2)) {
		GstBuffer *buffers[MFS_MAX_STREAMS] = {NULL};
		long long latency = current_ts - current_wall_time;
		if ((latency < -CVF_TS_VALID_WINDOW) || (latency > CVF_TS_VALID_WINDOW))
			current_ts = current_wall_time;
		mfs.last_cluster_time = current_ts;

		// Pass the frames
		for (i = 0; i < nstreams; i++) {
			unsigned int idx = ts_index_sorted[i];

			if ((i >= first_valid) && (mfs.last_buffer[idx] != NULL)) {
				GST_BUFFER_PTS(mfs.last_buffer[idx]) = current_ts;
				buffers[idx] = mfs.last_buffer[idx];
			} else
				buffers[idx] = NULL;
		}

		mfs.push_buffers(mfs.priv, buffers);
	}
}

static int multi_frame_sync_handler(void *data, unsigned int events)
{
	GstSample *sample;
	GstBuffer *buffer;
	GstClockTime ts;
	int read_samples;
	unsigned long long n_samples;
	uint64_t current_wall_time;
	const unsigned int nstreams = mfs.nstreams;
	unsigned int index;

	if (gettime_ns(&current_wall_time) != 0)
		goto err;

	for (index = 0; index < nstreams; index++) {
		n_samples = 0;

		read_samples = read(mfs.sync[index].event_fd, &n_samples, 8);
		if (read_samples != 8) {
			if (read_samples < 0)
				printf("%s: read error, index=%d, error:%s\n", __func__, index, strerror(errno));
			else
				printf("%s: read incomplete, index=%d, read_samples=%d expected 8\n", __func__, index, read_samples);
		}

		if (n_samples > 1) {
			printf("stream(%d) Dropping %llu frames\n", index, n_samples - 1); //FIXME Add some stats instead
		}

		if (n_samples > 0) {
			// VPUDEC seems very sensitive to buffer starvation, so make sure to free up buffers as quickly as possible to avoid a pipeline error
			sample = gst_base_sink_get_last_sample(GST_BASE_SINK(mfs.sync[index].appsink));
			buffer = gst_sample_get_buffer(sample);
			if (buffer == mfs.last_buffer[index]) {
				gst_sample_unref(sample);
				continue;
			}
			gst_buffer_ref(buffer);
			gst_sample_unref(sample);

			if (mfs.first_wakeup == 0)
				mfs.first_wakeup = current_wall_time;

			if (!GST_CLOCK_TIME_IS_VALID(mfs.sync[index].basetime)) {
				mfs.sync[index].basetime = gst_element_get_base_time(GST_ELEMENT(mfs.sync[index].appsink));
			}
			ts = GST_BUFFER_PTS(buffer);
			ts += mfs.sync[index].basetime;


			if (mfs.last_ts[index] != 0) {
				gst_buffer_unref(mfs.last_buffer[index]);
				mfs.ready = 1;
			}

			mfs.last_buffer[index] = buffer;
			mfs.last_ts[index] = ts;
		}
	}

	if ((current_wall_time - mfs.first_wakeup) > MFS_FRAME_PERIOD)
		mfs.ready = 1;

	if (mfs.ready)
		multi_frame_sync(current_wall_time);

	return 0;

err:
	return -1;
}

static GstFlowReturn multi_frame_sync_gst_new_sample_handler(GstAppSink *sink, gpointer data)
{
	struct gstreamer_sync *sync = data;
	unsigned long long val = 1;
	int rc = 0;

	rc = write(sync->event_fd, &val, 8);
	if (rc != 8) {
		if (rc < 0)
			printf("write() failed: %s\n", strerror(errno));
		else
			printf("write() incomplete, returned %d\n", rc);

		return GST_FLOW_ERROR;
	}

	return GST_FLOW_OK;
}


void mfs_init(unsigned int nstreams, int (*push_buffers)(void *priv, GstBuffer **buffers), void *priv)
{
	int i;

	memset(mfs.last_buffer, 0, sizeof(GstBuffer *) * MFS_MAX_STREAMS);
	memset(mfs.last_ts, 0, sizeof(GstClockTime) * MFS_MAX_STREAMS);
	mfs.last_cluster_time = GST_CLOCK_TIME_NONE;
	mfs.first_wakeup = 0;
	mfs.ready = 0;

	for (i = 0; i < MFS_MAX_STREAMS; i++) {
		mfs.sync[i].basetime = GST_CLOCK_TIME_NONE;
	}
	mfs.push_buffers = push_buffers;
	mfs.priv = priv;
	mfs.nstreams = nstreams;
}

int mfs_add_sync(unsigned int i, GstAppSink *appsink)
{
	GstAppSinkCallbacks callbacks = { NULL };

	mfs.sync[i].index = i;

	mfs.sync[i].appsink = appsink;

	mfs.sync[i].event_fd = eventfd(0, EFD_NONBLOCK);
	if (mfs.sync[i].event_fd < 0) {
		printf("eventfd failed for mfs.sync %d\n", i);
		goto error_event;
	}

	if (thread_slot_add(THR_CAP_GSTREAMER_SYNC, mfs.sync[i].event_fd, EPOLLIN, &mfs.sync[i], multi_frame_sync_handler, NULL, 0, &mfs.sync[i].thread_slot) < 0) {
		printf("thread_slot_add failed for mfs.sync %d\n", i);
		goto error_thread_sync;
	}

	callbacks.new_sample = multi_frame_sync_gst_new_sample_handler;
	gst_app_sink_set_callbacks(mfs.sync[i].appsink, &callbacks, &mfs.sync[i], NULL);

	return 0;

error_thread_sync:
	close(mfs.sync[i].event_fd);

error_event:
	return -1;

}


void mfs_remove_sync(unsigned int i)
{
	thread_slot_free(mfs.sync[i].thread_slot);
	close(mfs.sync[i].event_fd);
}
