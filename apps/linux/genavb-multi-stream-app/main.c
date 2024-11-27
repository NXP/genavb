/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define _GNU_SOURCE
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <limits.h>

#include <genavb/genavb.h>

#include "../common/common.h"
#include "../common/alsa.h"


#define BATCH_SIZE			1024
#define CFG_CAPTURE_LATENCY_NS		1500000		// Additional fixed playback latency in ns

#define PROCESS_PRIORITY	51 /* RT priority to be used for the process */

#define MAX_THREAD			16
#define MAX_STREAMS_PER_THREAD		1

#define STREAM_CONNECTED	(1 << 0)

#define STREAM_ADD	1
#define STREAM_REMOVE	2

#define FILENAME_SIZE	SCHAR_MAX

#define DEFAULT_ALSA_DEVICE	"plughw:0,0"

struct media_app_stream {
	int index;
	void *thread;
	unsigned int state;
	struct avb_stream_params params;
	struct avb_stream_handle *stream_h;
	int stream_fd;
	unsigned int batch_size;
	char media_file_name[FILENAME_SIZE];
	int media_fd;
	unsigned int media_flags;
	void *alsa_h;
};

struct mailbox {
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	int type;
	int fd;
	int index;
	short events;
};

struct thread {
	int index;
	pthread_t id;
	struct mailbox msg;
	struct media_app_stream stream[MAX_STREAMS_PER_THREAD];
	int num_streams;
	struct pollfd poll_fd[MAX_STREAMS_PER_THREAD];
};


/* application main context */
struct media_app {
	struct avb_handle *avb_h;
	struct avb_control_handle *ctrl_h;
	struct thread thread[MAX_THREAD];
	int stream_flags;
	unsigned int flags;
	char *alsa_device;
};

#define APP_FLAG_ALSA		(1 << 0)
#define APP_FLAG_CLOCK_SLAVE	(1 << 1)


struct media_app app;
pthread_barrier_t init_barrier;


static void set_avb_config(unsigned int *avb_flags)
{
	*avb_flags = 0;
}


static void dump_stream_infos(struct media_app_stream *stream)
{
	print_stream_id(stream->params.stream_id);

	printf("media file name: %s\n", stream->media_file_name);
	printf("mode: %s\n", (stream->params.direction == AVTP_DIRECTION_LISTENER)? "LISTENER":"TALKER");
}

static struct media_app_stream *find_free_stream(struct thread *thread)
{
	struct media_app_stream *stream = NULL;
	int i;

	for (i = 0; i < MAX_STREAMS_PER_THREAD; i++) {
		if (!(thread->stream[i].state & STREAM_CONNECTED)) {
			stream = &thread->stream[i];
			printf("%s: stream(%p) found\n", __func__, stream);
			break;
		}
	}

	return stream;
}


static struct thread *find_free_thread(void)
{
	struct thread *thread = NULL;
	int i;

	for (i = 0; i < MAX_THREAD; i++) {
		if (app.thread[i].num_streams < MAX_STREAMS_PER_THREAD) {
			thread = &app.thread[i];
			printf("%s: thread(%p) found\n", __func__, thread);
			break;
		}
	}

	return thread;
}

static struct media_app_stream *find_stream_by_id(void *id)
{
	struct media_app_stream *stream = NULL;
	int i, j;

	print_stream_id(id);

	for (i = 0; i < MAX_THREAD; i++) {
		for (j = 0; j < MAX_STREAMS_PER_THREAD; j++) {
			if (app.thread[i].stream[j].state & STREAM_CONNECTED) {

				if (!memcmp(app.thread[i].stream[j].params.stream_id, id, 8)) {
					stream = &app.thread[i].stream[j];

					printf("%s: stream(%p) found\n", __func__, stream);

					break;
				}
			}
		}
	}

	return stream;
}

static void msg_send(struct thread *thread, int type, int index, int fd, short events)
{
	pthread_mutex_lock(&thread->msg.mutex);

	thread->msg.index = index;
	thread->msg.type = type;
	thread->msg.fd = fd;
	thread->msg.events = events;

	printf("msg_send(%p, %d)\n", thread, type);

	if (!pthread_kill(thread->id, SIGUSR1)) {

		while (thread->msg.type)
			pthread_cond_wait(&thread->msg.cond, &thread->msg.mutex);
	}

	pthread_mutex_unlock(&thread->msg.mutex);
}


static void msg_receive(struct thread *thread)
{
	int index;

	pthread_mutex_lock(&thread->msg.mutex);

	if (thread->msg.type) {

		printf("msg_receive(%p, %d)\n", thread, thread->msg.type);

		index = thread->msg.index;

		switch (thread->msg.type) {
		case STREAM_ADD:
			thread->poll_fd[index].fd = thread->msg.fd;
			thread->poll_fd[index].events = thread->msg.events;
			thread->num_streams++;
			break;

		case STREAM_REMOVE:
			thread->poll_fd[index].fd = -1;
			thread->num_streams--;
			break;

		default:
			break;
		}

		thread->msg.type = 0;
		pthread_cond_signal(&thread->msg.cond);
	}

	pthread_mutex_unlock(&thread->msg.mutex);
}

static int thread_remove_stream(struct media_app_stream *stream)
{
	struct thread *thread = stream->thread;
	int rc = 0;

	msg_send(thread, STREAM_REMOVE, stream->index, 0, 0);

	stream->state &= ~STREAM_CONNECTED;

	if (stream->media_flags & MEDIA_FLAGS_ALSA)
		alsa_tx_exit(stream->alsa_h);

	close(stream->media_fd);

	avb_stream_destroy(stream->stream_h);

	printf("%s: thread(%p) removed stream(%p)\n", __func__, thread, stream);

	return rc;
}


static int thread_add_stream(struct thread *thread, struct avb_stream_params *stream_params, unsigned int avdecc_stream_index)
{
	struct media_app_stream *stream;
	struct avb_stream_handle *stream_h;
	int stream_fd;
	int rc = 0;

	stream = find_free_stream(thread);
	if (!stream) {
		printf("%s: could not find free stream\n", __func__);
		rc = -1;
		goto err_stream_get;
	}

	stream->media_flags = 0;

	/*
	* create media file
	*/

	if (stream_params->direction == AVTP_DIRECTION_LISTENER) {

		/*
		 * FIXME, provide choice via config file
		 */
		if ((app.flags & APP_FLAG_ALSA) && !avdecc_stream_index)
			stream->media_flags |= MEDIA_FLAGS_ALSA;
		else
			stream->media_flags |= MEDIA_FLAGS_FILE;

		if (stream->media_flags & MEDIA_FLAGS_FILE) {
			rc = snprintf(stream->media_file_name, FILENAME_SIZE, "/home/media/listener_media%d.raw", avdecc_stream_index);
			if ((rc < 0) || (rc >= FILENAME_SIZE)) {
				printf("Error %d while generating listener media file name \n", rc);
				rc = -1;
				goto err_filename;
			}

			stream->media_fd = open(stream->media_file_name, O_WRONLY | O_CREAT | O_TRUNC | O_NONBLOCK, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
		}
		else if (stream->media_flags & MEDIA_FLAGS_ALSA) {
			stream->alsa_h = alsa_tx_init(stream_params->stream_id, &stream_params->format, BATCH_SIZE, app.alsa_device);
			if (!stream->alsa_h) {
				printf("alsa_tx_init error\n");
				rc = -1;
				goto err_alsa;
			}

		}
	} else {
		rc = snprintf(stream->media_file_name, FILENAME_SIZE, "/home/media/talker_media%d.raw", avdecc_stream_index);
		if ((rc < 0) || (rc >= FILENAME_SIZE)) {
			printf("Error %d while generating talker media file name \n", rc);
			rc = -1;
			goto err_filename;
		}

		stream->media_fd = open(stream->media_file_name, O_RDONLY | O_NONBLOCK);

		if (avdecc_format_is_61883_6(&stream_params->format)
		&& (AVDECC_FMT_61883_6_FDF_EVT(&stream_params->format) == IEC_61883_6_FDF_EVT_AM824))
		{
			stream->media_flags |= MEDIA_FLAGS_SET_AM824_LABEL_RAW;
			printf("IEC-61883-6 AM824 data format\n");
		}
	}

	if (stream->media_fd < 0) {
		printf("%s: open(%s) failed: %s\n", __func__, stream->media_file_name, strerror(errno));
		goto err_open;
	}

	/*
	* setup the stream
	*/

	stream->batch_size = BATCH_SIZE;
	rc = avb_stream_create(app.avb_h, &stream_h, stream_params, &stream->batch_size, app.stream_flags);
	if (rc != AVB_SUCCESS) {
		printf("%s: avb_stream_create() failed: %s\n", __func__, avb_strerror(rc));
		rc = -1;
		goto err_stream_create;
	}
	printf("Configured AVB batch size (bytes): %d\n", stream->batch_size);


	/*
	* retrieve the file descriptor associated to the stream
	*/

	stream_fd = avb_stream_fd(stream_h);
	if (stream_fd < 0) {
		printf("%s: avb_stream_fd() failed: %s\n", __func__, avb_strerror(stream_fd));
		rc = -1;
		goto err_stream_fd;
	}

	/*
	* link stream to the group and add it to polling list
	*/

	memcpy(&stream->params, stream_params, sizeof(struct avb_stream_params));
	stream->stream_h = stream_h;
	stream->stream_fd = stream_fd;
	stream->state |= STREAM_CONNECTED;
	stream->thread = thread;

	dump_stream_infos(stream);

	/*
	* add stream to thread polling list
	*/

	if (stream_params->direction == AVTP_DIRECTION_LISTENER)
		msg_send(thread, STREAM_ADD, stream->index, stream->stream_fd, POLLIN);
	else
		msg_send(thread, STREAM_ADD, stream->index, stream->stream_fd, POLLOUT);

	printf("%s: thread(%p) added stream(%p) fd(%d)\n", __func__, thread, stream, stream->stream_fd);

	return rc;

err_stream_fd:
	avb_stream_destroy(stream->stream_h);

err_stream_create:
	close(stream->media_fd);

err_open:
err_alsa:
err_filename:
err_stream_get:
	return rc;
}

static void stream_thread_cleanup(void *arg)
{
	struct thread *thread = (struct thread *)arg;
	int i;

	for (i = 0; i < MAX_STREAMS_PER_THREAD; i++)
		thread_remove_stream(&thread->stream[i]);
}


void signal_usr_handler(int signal_num)
{
}

static void *stream_thread(void *arg)
{
	struct thread *thread = (struct thread *)arg;
	struct media_app_stream *stream;
	struct sigaction action;
	sigset_t mask, omask;
	int ready, i, n;
	int err;

	printf("%s: thread(%p) started\n", __func__, thread);

	action.sa_handler = signal_usr_handler;
	action.sa_flags = 0;

	if (sigemptyset(&action.sa_mask) < 0)
		printf("sigemptyset(): %s\n", strerror(errno));

	if (sigaction(SIGUSR1, &action, NULL) < 0) /* User signal to pause talker streaming */
		printf("sigaction(): %s\n", strerror(errno));

	/*
	* intialize fd/stream table
	*/
	for (i = 0; i < MAX_STREAMS_PER_THREAD; i++) {
		thread->poll_fd[i].fd = -1;
		thread->stream[i].index = i;
	}

	pthread_cleanup_push(stream_thread_cleanup, thread);

	/* Block the SIGUSR1 signal to avoid missing it before the ppoll */

	err = sigemptyset(&omask);
	if (err < 0) {
		printf("thread(%p): sigemptyset failed: %s\n", thread, strerror(errno));
		goto exit;
	}

	err = sigemptyset(&mask);
	if (err < 0) {
		printf("thread(%p): sigemptyset failed: %s\n", thread, strerror(errno));
		goto exit;
	}

	err = sigaddset(&mask, SIGUSR1);
	if (err < 0) {
		printf("thread(%p): sigaddset failed: %s\n", thread, strerror(errno));
		goto exit;
	}

	err = pthread_sigmask(SIG_BLOCK, &mask, &omask);
	if (err != 0) {
		printf("thread(%p): pthread_sigmask failed: %s\n", thread, strerror(err));
		goto exit;
	}

	/*
	* Wait for barrier to make sure we wake up the main
	* thread when all threads complete init
	*/
	pthread_barrier_wait(&init_barrier);

	/*
	* listen to read event from the stack
	*/

	while (1) {
		ready = ppoll(thread->poll_fd, MAX_STREAMS_PER_THREAD, NULL, &omask);
		if (ready < 0) {
			if (errno == EINTR) {
				msg_receive(thread);
				continue;
			}

			printf("thread(%p): poll() failed: %s\n", thread, strerror(errno));

			goto exit;
		}

		for (n = 0, i = 0; (i < MAX_STREAMS_PER_THREAD) && (n < ready); i++) {
			if ((thread->poll_fd[i].fd > 0) && (thread->poll_fd[i].revents & (POLLIN | POLLOUT))) {
				stream = &thread->stream[i];

				if (stream->params.direction == AVTP_DIRECTION_TALKER)
					talker_file_handler(stream->stream_h, stream->media_fd, stream->batch_size, stream->media_flags);
				else {
					if (stream->media_flags & MEDIA_FLAGS_FILE)
						listener_file_handler(stream->stream_h, stream->media_fd, stream->batch_size, NULL);
					else if (stream->media_flags & MEDIA_FLAGS_ALSA)
						alsa_tx(stream->alsa_h, stream->stream_h, &stream->params);
				}
				n++;
			}
		}

		msg_receive(thread);
	}

exit:
	pthread_cleanup_pop(1);

	return (void*)0;
}


static int handle_avdecc_event(struct avb_control_handle *ctrl_h)
{
	struct avb_stream_params *params;
	struct media_app_stream *stream;
	struct thread *thread;
	union avb_media_stack_msg msg;
	unsigned int msg_type, msg_len;
	int rc;

	msg_len =  sizeof(union avb_media_stack_msg);
	rc = avb_control_receive(ctrl_h, &msg_type, &msg, &msg_len);
	if (rc != AVB_SUCCESS)
		goto error_control_receive;

	switch (msg_type) {
	case AVB_MSG_MEDIA_STACK_CONNECT:
		printf("AVB_MSG_MEDIA_STACK_CONNECT\n");

		params = &msg.media_stack_connect.stream_params;

		stream = find_stream_by_id(&params->stream_id);
		if (stream) {
			printf("%s: stream already created\n", __func__);
			break;
		}

		if (params->direction == AVTP_DIRECTION_TALKER) {
			if (app.flags & APP_FLAG_CLOCK_SLAVE)
				params->clock_domain = AVB_MEDIA_CLOCK_DOMAIN_STREAM;
			else
				params->clock_domain = AVB_MEDIA_CLOCK_DOMAIN_PTP;

			params->talker.latency = max(CFG_CAPTURE_LATENCY_NS, sr_class_interval_p(params->stream_class) / sr_class_interval_q(params->stream_class));
		} else {
			params->clock_domain = AVB_MEDIA_CLOCK_DOMAIN_STREAM;
		}



		thread = find_free_thread();
		if (thread) {
			if (thread_add_stream(thread, params, msg.media_stack_connect.stream_index) < 0)
				printf("%s: stream creation failed\n", __func__);
		} else
			printf("%s: stream creation failed: could not find free thread\n", __func__);

		break;

	case AVB_MSG_MEDIA_STACK_DISCONNECT:
		printf("AVB_MSG_MEDIA_STACK_DISCONNECT\n");

		stream = find_stream_by_id(&msg.media_stack_disconnect.stream_id);

		if (stream)
			thread_remove_stream(stream);

		break;

	default:
		break;
	}

error_control_receive:
	return rc;
}


static void thread_destroy(struct thread *thread)
{
	pthread_cancel(thread->id);
	pthread_join(thread->id, NULL);
	pthread_cond_destroy(&thread->msg.cond);
	pthread_mutex_destroy(&thread->msg.mutex);
}


static int thread_create(struct thread *thread)
{
	int rc;

	pthread_mutex_init(&thread->msg.mutex, NULL);
	pthread_cond_init(&thread->msg.cond, NULL);

	rc = pthread_create(&thread->id, NULL, &stream_thread, thread);
	if (rc != 0) {
		pthread_cond_destroy(&thread->msg.cond);
		pthread_mutex_destroy(&thread->msg.mutex);
	}

	return rc;
}


int main(int argc, char *argv[])
{
	unsigned int avb_flags;
	int ctrl_rx_fd;
	struct pollfd ctrl_poll;
	int option;
	struct sched_param param = {
		.sched_priority = PROCESS_PRIORITY,
	};
	int i,j;
	int rc = 0;

	setlinebuf(stdout);

	memset(&app, 0, sizeof(app));

	printf("NXP's GenAVB reference multiple audio stream application\n");

	if (sched_setscheduler(0, SCHED_FIFO, &param) < 0) {
		printf("sched_setscheduler() failed: %s\n", strerror(errno));
		rc = -1;
		goto error_sched;
	}

	app.alsa_device = DEFAULT_ALSA_DEVICE;

	while ((option = getopt(argc, argv, "asd:")) != -1) {
		switch (option) {
		case 'a':
			app.flags |= APP_FLAG_ALSA;
			break;

		case 'd':
			app.alsa_device = optarg;
			break;

		case 's':
			app.flags |= APP_FLAG_CLOCK_SLAVE;
			break;

		default:
			break;
		}
	}

	/*
	* setup the avb stack
	*/

	set_avb_config(&avb_flags);

	rc = avb_init(&app.avb_h, avb_flags);
	if (rc != AVB_SUCCESS) {
		printf("avb_init() failed: %s\n", avb_strerror(rc));
		rc = -1;
		goto error_avb_init;
	}

	/*
	* common setting applied to any stream
	*/
	app.stream_flags = AVTP_NONBLOCK;

	/*
	* Init the barrier with a count of all created thread plus the main thread
	*/
	pthread_barrier_init(&init_barrier, NULL, MAX_THREAD + 1);

	/*
	* create all streams threads
	*/
	for (i = 0; i < MAX_THREAD; i++) {
		rc = thread_create(&app.thread[i]);
		if (rc != 0) {
			printf("pthread_create for thread number (%d) failed: %s\n", i, strerror(rc));
			/* Remove all previously created threads*/
			for (j = 0; j < i; j++)
				thread_destroy(&app.thread[i]);
			goto error_thread_create;
		}

		app.thread[i].index= i;
	}


	/*
	* Wait for all threads to complete init
	*/
	pthread_barrier_wait(&init_barrier);

	/*
	* listen to avdecc events to get stream parameters
	*/

	rc = avb_control_open(app.avb_h, &app.ctrl_h, AVB_CTRL_AVDECC_MEDIA_STACK);
	if (rc != AVB_SUCCESS) {
		printf("avb_control_open() failed: %s\n", avb_strerror(rc));
		goto error_control_open;
	}

	ctrl_rx_fd = avb_control_rx_fd(app.ctrl_h);
	ctrl_poll.fd = ctrl_rx_fd;
	ctrl_poll.events = POLLIN;
	ctrl_poll.revents = 0;

	while (1) {
		if (poll(&ctrl_poll, 1, -1) == -1) {
			printf("poll(%d) failed on waiting for connect\n", ctrl_poll.fd);
			rc = -1;
			goto error_ctrl_poll;
		}

		if (ctrl_poll.revents & POLLIN)
			handle_avdecc_event(app.ctrl_h);
	}

error_ctrl_poll:
	avb_control_close(app.ctrl_h);

error_control_open:
	/*
	* destroy all stream threads
	*/
	for (i = 0; i < MAX_THREAD; i++)
		thread_destroy(&app.thread[i]);

	pthread_barrier_destroy(&init_barrier);

error_thread_create:
	avb_exit(app.avb_h);

error_avb_init:
error_sched:
	return rc;
}
