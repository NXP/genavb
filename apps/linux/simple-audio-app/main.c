/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sched.h>
#include <signal.h>

#include <genavb/genavb.h>
#include <genavb/srp.h>

#include "../common/ts_parser.h"
#include "../common/file_buffer.h"
#include "../common/common.h"
#include "../common/msrp.h"

#define CFG_CAPTURE_LATENCY_NS		1500000		// Additional fixed playback latency in ns

/* Application main modes */
#define MODE_AVDECC	0 /* the application relies on avdecc indication from avb stack*/
#define MODE_LISTENER	1 /* acting as media files server if avdecc is not used*/
#define MODE_TALKER	2 /* acting as media files server if avdecc is not used*/

/* GenAVB stack cofiguration */
#define FLAG_BLOCKING	(1 << 2) /* use blocking call to the genavb library */
#define FLAG_IOV	(1 << 3) /* use iov array for data and event */

#define PROCESS_PRIORITY	60 /* RT_FIFO priority to be used for the process */

/* default file name used for media */
#define DEFAULT_MEDIA_FILE_NAME "media.raw"


#define K		1024
#define DATA_BUF_SZ	(16*K)
#define EVENT_BUF_SZ	(K)


#define BATCH_SIZE	4096

#define EVENT_MAX	(BATCH_SIZE / PES_SIZE)

static int signal_terminate = 0;
static int signal_pause = 0;

/* application main context */
struct avb_app {
	unsigned int mode;
	unsigned int config;
	int media_fd;
	char *media_file_name;
	struct avb_stream_params stream_params;
	unsigned int stream_batch_size;
	unsigned int stream_flags;
	struct avb_control_handle *ctrl_h;
	int connected_stream_index;
};

struct avb_app app;

static unsigned int ts_parser_enabled = 0;

struct avb_stream_params default_stream_params = {
	.subtype = AVTP_SUBTYPE_61883_IIDC,
	.stream_class = SR_CLASS_A,
	.flags = 0,
	.format.u.s = {
		.v = 0,
		.subtype = AVTP_SUBTYPE_61883_IIDC,
		.subtype_u.iec61883 = {
			.sf = IEC_61883_SF_61883,
			.fmt = IEC_61883_CIP_FMT_6,
			.r = 0,
			.format_u.iec61883_6 = {
				.fdf_u.fdf = {
					.evt = IEC_61883_6_FDF_EVT_AM824,
					.sfc = IEC_61883_6_FDF_SFC_48000,
				},
				.dbs = 2,
				.b = 0,
				.nb = 1,
				.rsvd = 0,
				.label_iec_60958_cnt = 0,
				.label_mbla_cnt = 2,
				.label_midi_cnt = 0,
				.label_smptecnt = 0,
			},
		},
	},
	.port = 0,
	.stream_id = { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 },
	.dst_mac = { 0x91, 0xE0, 0xF0, 0x00, 0xeb, 0x15 },
};

static void usage (void)
{
	printf("\nUsage:\napp [options]\n");
	printf("\nOptions:\n"
		"\t-m <mode>             application mode: avdecc(default), listener, talker\n"
		"\t-b                    use blocking API calls\n"
		"\t-i                    use iov\n"
		"\t-f <file name>        media file name (default media.raw)\n"
		"\t-t                    parse the media file as mpegts and apply PCR based flow control\n"
		"\t-h                    print this help text\n");
}


static void set_avb_config(unsigned int *avb_flags)
{
	*avb_flags = 0;
}


static int apply_config(struct avb_stream_params *stream_params)
{
	memcpy(&app.stream_params, stream_params, sizeof(struct avb_stream_params));

	if (app.stream_params.direction == AVTP_DIRECTION_TALKER) {
		app.stream_params.clock_domain = AVB_MEDIA_CLOCK_DOMAIN_PTP;
		app.stream_params.talker.latency = max(CFG_CAPTURE_LATENCY_NS, sr_class_interval_p(app.stream_params.stream_class) / sr_class_interval_q(app.stream_params.stream_class));
	} else {
		app.stream_params.clock_domain = AVB_MEDIA_CLOCK_DOMAIN_STREAM;
	}

	app.stream_batch_size = BATCH_SIZE;

	if (!(app.config & FLAG_BLOCKING))
		app.stream_flags = AVTP_NONBLOCK;
	else
		app.stream_flags = 0;

	if (app.media_file_name == NULL)
		app.media_file_name = DEFAULT_MEDIA_FILE_NAME;

	if (app.stream_params.direction == AVTP_DIRECTION_LISTENER)
		app.media_fd = open(app.media_file_name, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	else
		app.media_fd = open(app.media_file_name, O_RDONLY);

	if (app.media_fd < 0) {
		printf("open(%s) failed: %s\n", app.media_file_name, strerror(errno));
		goto error_media_open;
	}

	/*
	* display the whole configuration
	*/

	print_stream_id(stream_params->stream_id);

	printf("mode: ");
	if (app.mode == MODE_LISTENER)
		printf("LISTENER\n");
	else if (app.mode == MODE_TALKER)
		printf("TALKER\n");
	else
		printf("AVDECC %s\n", (app.stream_params.direction == AVTP_DIRECTION_LISTENER) ? "LISTENER":"TALKER");

	printf("media file name: %s (fd %d)\n", app.media_file_name, app.media_fd);

	printf("flags: ");
	if (app.config & FLAG_BLOCKING)
		printf("BLOCKING ");
	if (app.config & FLAG_IOV)
		printf("IOV\n");

	printf("\n\n");

	return 0;

error_media_open:
	return -1;
}

static int handle_avdecc_event(struct avb_control_handle *ctrl_h, unsigned int *msg_type, union avb_media_stack_msg *msg, bool *is_audio_stream)
{
	unsigned int msg_len = sizeof(union avb_media_stack_msg);
	int rc;

	*is_audio_stream = false;

	rc = avb_control_receive(ctrl_h, msg_type, msg, &msg_len);
	if (rc != AVB_SUCCESS)
		goto receive_error;

	switch (*msg_type) {
	case AVB_MSG_MEDIA_STACK_CONNECT:
		if (!(avdecc_format_is_61883_6(&msg->media_stack_connect.stream_params.format) || avdecc_format_is_aaf_pcm(&msg->media_stack_connect.stream_params.format))) {
			printf("\nIgnoring stream formats other than 61883_6 or AAF\n");
			goto exit;
		}

		*is_audio_stream = true;

		printf("\nevent: AVB_MSG_MEDIA_STACK_CONNECT\n");
		app.connected_stream_index = msg->media_stack_connect.stream_index;
		break;

	case AVB_MSG_MEDIA_STACK_DISCONNECT:
		if (msg->media_stack_disconnect.stream_index != app.connected_stream_index) {
			printf("\nIgnoring stream with a different id than the current connected stream\n");
			goto exit;
		}

		*is_audio_stream = true;

		printf("\nevent: AVB_MSG_MEDIA_STACK_DISCONNECT\n");
		app.connected_stream_index = -1;
		break;

	default:
		break;
	}

exit:
receive_error:
	return rc;
}

static int listener_blocking(struct avb_stream_handle *stream_h, unsigned int batch_size, int file_dst)
{
	unsigned int event_len = EVENT_BUF_SZ;
	unsigned char data_buf[DATA_BUF_SZ];
	struct avb_event event[EVENT_BUF_SZ];
	int nbytes;
	int rc = 0;

	printf("Starting listener loop, blocking mode\n");

	while (1) {
		/*
		* read data from stack...
		*/
		nbytes = avb_stream_receive(stream_h, data_buf, batch_size, event, &event_len);
		if (nbytes <= 0) {
			if (nbytes < 0)
				printf("avb_stream_receive() failed: %s\n", avb_strerror(nbytes));
			else
				printf("avb_stream_receive() incomplete\n");

			goto exit;
		}

		if (event_len != 0) {
			if (event[0].event_mask & AVTP_MEDIA_CLOCK_RESTART)
				printf ("AVTP media clock restarted\n");

			if (event[0].event_mask & AVTP_PACKET_LOST)
				printf ("AVTP packet lost\n");
		}

		/*
		* ...and write to local file
		*/
		rc = write(file_dst, data_buf, nbytes);
		if (rc < nbytes) {
			if (rc < 0)
				printf("write() failed: %s\n", strerror(errno));
			else
				printf("write() incomplete\n");

			goto exit;
		}
	}

exit:
	return rc;

}


static int listener_nonblocking(struct avb_stream_handle *stream_h, int stream_fd, unsigned int batch_size, int file_dst)
{
	unsigned int event_len = EVENT_BUF_SZ;
	struct pollfd poll_fds[2];
	unsigned char data_buf[DATA_BUF_SZ];
	struct avb_event event[EVENT_BUF_SZ];
	int nbytes;
	int rc = 0;
	int ctrl_rx_fd = -1;
	int ready, i, n, nfds;
	unsigned int event_type;
	union avb_media_stack_msg msg;
	bool is_audio_stream;

	printf("Starting listener loop, non-blocking mode\n");

	/*
	* listen to read event from the stack
	*/

	nfds = 0;

	poll_fds[0].fd = stream_fd;
	poll_fds[0].events = POLLIN;
	poll_fds[0].revents = 0;
	nfds++;

	/* control fd required only for avdecc mode */
	if (app.ctrl_h) {
		ctrl_rx_fd = avb_control_rx_fd(app.ctrl_h);
		poll_fds[1].fd = ctrl_rx_fd;
		poll_fds[1].events = POLLIN;
		poll_fds[1].revents = 0;
		nfds++;
	} else {
		poll_fds[1].fd = -1;
		poll_fds[1].events = 0;
		poll_fds[1].revents = 0;
	}

	while (1) {
		if (signal_terminate) {
			printf("processing terminate signal\n");
			rc = -1;
			goto exit;
		}

		if ((ready = poll(poll_fds, nfds, -1)) == -1) {
			if (errno == EINTR) {
				continue;
			} else {
				printf("poll(%d) failed while processing listener errno %d: %s\n", stream_fd, errno, strerror(errno));
				rc = -1;
				goto exit;
			}
		}

		if (ready > 0) {
			for (n = 0, i = 0; i < nfds && n < ready; i++) {
				if (poll_fds[i].revents & POLLIN) {
					if (poll_fds[i].fd == ctrl_rx_fd) {
						n++;

						/*
						* read control event from avdecc
						*/
						if (handle_avdecc_event(app.ctrl_h, &event_type, &msg, &is_audio_stream) == AVB_SUCCESS) {
							if (event_type == AVB_MSG_MEDIA_STACK_DISCONNECT && is_audio_stream) {
								rc = 0;
								goto exit; /* disconnected, stop processing on this stream */
							}
						}
					} else if (poll_fds[i].fd == stream_fd) {
						n++;

						/*
						* read data from stack...
						*/
						nbytes = avb_stream_receive(stream_h, data_buf, batch_size, event, &event_len);
						if (nbytes <= 0) {
							if (nbytes < 0)
								printf("avb_stream_receive() failed: %s\n", avb_strerror(nbytes));
							else
								printf("avb_stream_receive() incomplete\n");

							rc = nbytes;
							goto exit;
						}

						if (event_len != 0) {
							if (event[0].event_mask & AVTP_MEDIA_CLOCK_RESTART)
								printf ("AVTP media clock restarted\n");

							if (event[0].event_mask & AVTP_PACKET_LOST)
								printf ("AVTP packet lost\n");
						}

						/*
						* ...and write to local file
						*/
						rc = write(file_dst, data_buf, nbytes);
						if (rc < nbytes) {
							if (rc < 0)
								printf("write() failed: %s\n", strerror(errno));
							else
								printf("write() incomplete\n");

							goto exit;
						}
					}
				}
			}
		}
	}
exit:
	return rc;
}


static int talker_blocking(struct avb_stream_handle *stream_h, unsigned int batch_size, int file_src)
{
	unsigned char data_buf[DATA_BUF_SZ] = {0};
	int nbytes;
	int rc = 0;

	printf("Starting talker loop, blocking mode\n");

	while (1) {
		/*
		* read data from local file...
		*/
		nbytes = read(file_src, data_buf, batch_size);

		/* no more data to read, we are done*/
		if (nbytes <= 0) {
			if (nbytes < 0)
				printf("read() failed: %s\n", strerror(errno));
			else
				printf("read() incomplete\n");

			goto exit;
		}

		/*
		* ...and write to avb stack
		*/
		rc = avb_stream_send(stream_h, data_buf, nbytes, NULL, 0);
		if (rc != nbytes) {
			if (rc < 0)
				printf("avb_stream_send() failed: %s\n", avb_strerror(rc));
			else
				printf("avb_stream_send() incomplete\n");

			goto exit;
		}
	}

exit:
	return rc;

}


static int talker_nonblocking(struct avb_stream_handle *stream_h, int stream_fd, unsigned int batch_size, int file_src)
{
	struct pollfd poll_fds[2];
	int nbytes;
	int rc = 0;
	int ctrl_rx_fd = -1;
	int ready, i, n, nfds;
	unsigned int event_type;
	union avb_media_stack_msg msg;
	struct avb_event event[EVENT_MAX];
	unsigned int event_n;
	unsigned long long byte_count;
	struct ts_parser p;
	struct file_buffer *b;
	bool is_audio_stream;

	printf("Starting talker loop, non-blocking mode\n");

	b = malloc(sizeof(struct file_buffer));
	if (!b) {
		printf("%s() cannot allocate file_buffer\n", __func__);
		rc = -1;
		goto err_malloc;
	}

loop:
	lseek(file_src, 0, SEEK_SET);

	if (ts_parser_enabled) {
		sleep(4);

		ts_parser_init(&p);

		file_buffer_init(b, 2);
	} else
		file_buffer_init(b, 1);

	byte_count = 0;

	/*
	* listen to write event from the stack
	*/
	nfds = 0;

	poll_fds[0].fd = stream_fd;
	poll_fds[0].events = POLLOUT;
	poll_fds[0].revents = 0;
	nfds++;

	/* control fd required only for avdecc mode */
	if (app.ctrl_h) {
		ctrl_rx_fd = avb_control_rx_fd(app.ctrl_h);
		poll_fds[1].fd = ctrl_rx_fd;
		poll_fds[1].events = POLLIN;
		poll_fds[0].revents = 0;
		nfds++;
	} else {
		poll_fds[1].fd = -1;
		poll_fds[1].events = 0;
		poll_fds[0].revents = 0;
	}

	while (1) {
		if (signal_terminate) {
			printf("processing terminate signal\n");
			rc = -1;
			goto exit;
		}

		if ((ready = poll(poll_fds, nfds, -1)) == -1) {
			if (errno == EINTR) {
				if (signal_pause) {
					printf("processing pause signal\n");
					poll_fds[0].fd = -1;
				} else {
					printf("processing play signal\n");
					poll_fds[0].fd = stream_fd;
				}

				continue;
			} else {
				printf("poll(%d) failed while processing listener errno %d: %s\n", stream_fd, errno, strerror(errno));
				rc = -1;
				goto exit;
			}
		}

		if (ready > 0) {
			for (n = 0, i = 0; i < nfds && n < ready; i++) {
				if (poll_fds[i].revents & POLLIN) {
					if (poll_fds[i].fd == ctrl_rx_fd) {
						n++;

						/*
						* read control event from avdecc
						*/
						if (handle_avdecc_event(app.ctrl_h, &event_type, &msg, &is_audio_stream) == AVB_SUCCESS) {
							if (event_type == AVB_MSG_MEDIA_STACK_DISCONNECT && is_audio_stream) {
								rc = 0;
								goto exit; /* disconnected, stop processing on this stream */
							}
						}
					}
				} else if (poll_fds[i].revents & POLLOUT) {
					if (poll_fds[i].fd == stream_fd) {
						n++;

					read:
						if (file_buffer_empty(b, 0)) {
							rc = file_buffer_write(b, file_src, 1000000);
							if (rc <= 0) {
								if (!rc) {
									printf("loop\n");

									talker_stream_flush(stream_h, NULL);

									goto loop;
								}

								printf("file_buffer_write() failed\n");

								goto exit;
							}
						} else {
							rc = file_buffer_write(b, file_src, 0);
							if (rc < 0) {
								printf("file_buffer_write() failed\n");
								goto exit;
							}
						}

						nbytes = batch_size;
						if (nbytes > file_buffer_available_wrap(b, 0))
							nbytes = file_buffer_available_wrap(b, 0);

						if (ts_parser_enabled) {
							event_n = EVENT_MAX;
							nbytes = ts_parser_timestamp_range(b, &p, event, &event_n, byte_count, nbytes, avb_stream_presentation_offset(stream_h));

							if (nbytes <= 0) {
								if (!nbytes)
									goto read;

								rc = -1;
								goto exit;
							}
						} else {
							event_n = 0;
						}

						/*
						* ...and write to avb stack
						*/
						rc = avb_stream_send(stream_h, file_buffer_buf(b, 0), nbytes, event, event_n);
						if (rc != nbytes) {
							if (rc < 0)
								printf("avb_stream_send() failed: %s\n", avb_strerror(rc));
							else
								printf("avb_stream_send() incomplete\n");

							goto exit;
						}

						file_buffer_read(b, 0, nbytes);

						byte_count += nbytes;
					}
				}
			}
		}
	}

exit:
	free(b);

err_malloc:
	return rc;
}


static int run_listener(struct avb_stream_handle *stream_h, int stream_fd, unsigned int batch_size)
{
	int rc;

	if (app.config & FLAG_BLOCKING)
		rc = listener_blocking(stream_h, batch_size, app.media_fd);
	else
		rc = listener_nonblocking(stream_h, stream_fd, batch_size, app.media_fd);

	return rc;
}


static int run_talker(struct avb_stream_handle *stream_h, int stream_fd, unsigned int batch_size)
{
	int rc;

	if (app.config & FLAG_BLOCKING)
		rc = talker_blocking(stream_h, batch_size, app.media_fd);
	else
		rc = talker_nonblocking(stream_h, stream_fd, batch_size, app.media_fd);

	return rc;
}


void signal_terminate_handler (int signal_num)
{
	signal_terminate = 1;
}


void signal_pause_handler (int signal_num)
{
	if (signal_num == SIGUSR1)
		signal_pause = 1;
	else if(signal_num == SIGUSR2)
		signal_pause = 0;
}

int main(int argc, char *argv[])
{
	struct avb_handle *avb_h;
	struct avb_stream_handle* stream_h;
	union avb_media_stack_msg msg;
	unsigned int avb_flags;
	int stream_fd, option;
	unsigned int event_type;
	int ctrl_rx_fd;
	struct pollfd ctrl_poll;
	int rc = 0;
	struct sched_param param = {
		.sched_priority = PROCESS_PRIORITY,
	};
	struct sigaction action;
	bool is_audio_stream;


	/*
	* Increase process priority to match the AVTP thread priority
	*/

	if (sched_setscheduler(0, SCHED_FIFO, &param) < 0) {
		printf("sched_setscheduler(), %s\n", strerror(errno));
		rc = -1;
		goto exit;
	}

	setlinebuf(stdout);

	printf("NXP's GenAVB reference audio application\n");


	/*
	* retrieve user's configuration parameters
	*/

	app.mode = MODE_AVDECC; app.config = 0; /* default is AVDECC, NON-BLOCKING, SINGLE BUFFER */
	app.connected_stream_index = -1;

	while ((option = getopt(argc, argv,"m:bif:ht")) != -1) {
		switch (option) {
		case 'm':
			if (!strcasecmp(optarg, "listener"))
				app.mode = MODE_LISTENER;
			else if (!strcasecmp(optarg, "talker"))
				app.mode = MODE_TALKER;
			else if(!strcasecmp(optarg, "avdecc"))
				app.mode = MODE_AVDECC;
			else {
				usage();
				goto exit;
			}
			break;

		case 'b':
			app.config |= FLAG_BLOCKING;
			printf("blocking mode not supported\n");
			rc = -1;
			goto exit;

		case 'i':
			app.config |= FLAG_IOV;
			printf("multiple buffers mode not supported\n");
			rc = -1;
			goto exit;

		case 'f':
			app.media_file_name = optarg;
			break;

		case 't':
			ts_parser_enabled = 1;
			break;

		case 'h':
		default:
			usage();
			rc = -1;
			goto exit;
		}
	}


	/*
	* set signals handler
	*/
	action.sa_handler = signal_terminate_handler;
	action.sa_flags = 0;

	if (sigemptyset(&action.sa_mask) < 0)
		printf("sigemptyset(): %s\n", strerror(errno));

	if (sigaction(SIGTERM, &action, NULL) < 0) /* Termination signal */
		printf("sigaction(): %s\n", strerror(errno));

	if (sigaction(SIGQUIT, &action, NULL) < 0) /* Quit from keyboard */
		printf("sigaction(): %s\n", strerror(errno));

	if (sigaction(SIGINT, &action, NULL) < 0) /* Interrupt from keyboard */
		printf("sigaction(): %s\n", strerror(errno));

	action.sa_handler = signal_pause_handler;
	action.sa_flags = 0;

	if (sigemptyset(&action.sa_mask) < 0)
		printf("sigemptyset(): %s\n", strerror(errno));

	if (sigaction(SIGUSR1, &action, NULL) < 0) /* User signal to pause talker streaming */
		printf("sigaction(): %s\n", strerror(errno));

	if (sigaction(SIGUSR2, &action, NULL) < 0) /* Resume talker streaming */
		printf("sigaction(): %s\n", strerror(errno));

	/*
	* setup the avb stack
	*/

	set_avb_config(&avb_flags);

	rc = avb_init(&avb_h, avb_flags);
	if (rc != AVB_SUCCESS) {
		printf("avb_init() failed: %s\n", avb_strerror(rc));
		rc = -1;
		goto error_avb_init;
	}

	if (app.mode != MODE_AVDECC) {
		rc = msrp_init(avb_h);
		if (rc < 0)
			goto err_msrp_init;
	}

wait_new_stream:
	printf("\nwait for new stream...\n");

	if (app.mode == MODE_AVDECC) {
		/*
		* listen to avdecc events to get stream parameters
		*/

		rc = avb_control_open(avb_h, &app.ctrl_h, AVB_CTRL_AVDECC_MEDIA_STACK);
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

			if (ctrl_poll.revents & POLLIN) {
				/*
				* read control event from avdecc
				*/
				if (handle_avdecc_event(app.ctrl_h, &event_type, &msg, &is_audio_stream) == AVB_SUCCESS) {
					if (event_type == AVB_MSG_MEDIA_STACK_CONNECT && is_audio_stream) {
						apply_config(&msg.media_stack_connect.stream_params);
						break; /* connected, start stream processing */
					}
				}
			}
		}
	}else {
		/*
		* no avdecc, static configuration used
		*/

		if (app.mode == MODE_TALKER)
			default_stream_params.direction = AVTP_DIRECTION_TALKER;
		else
			default_stream_params.direction = AVTP_DIRECTION_LISTENER;

		apply_config(&default_stream_params);

		if (app.mode == MODE_TALKER) {
			rc = msrp_talker_register(&default_stream_params);
			if (rc != AVB_SUCCESS) {
				printf("msrp_talker_register error, rc = %d\n", rc);
				goto err_msrp;
			}
		} else if (app.mode == MODE_LISTENER) {
			rc = msrp_listener_register(&default_stream_params);
			if (rc != AVB_SUCCESS) {
				printf("msrp_listener_register error, rc = %d\n", rc);
				goto err_msrp;
			}
		}

	}


	/*
	* setup the stream
	*/
	sleep(3);

	rc = avb_stream_create(avb_h, &stream_h, &app.stream_params, &app.stream_batch_size, app.stream_flags);
	if (rc != AVB_SUCCESS) {
		printf("avb_stream_create() failed: %s\n", avb_strerror(rc));
		rc = -1;
		goto error_stream_create;
	}
	printf("Configured AVB batch size (bytes): %d\n", app.stream_batch_size);

	/*
	* retrieve the file descriptor associated to the stream
	*/

	stream_fd = avb_stream_fd(stream_h);
	if (stream_fd < 0) {
		printf("avb_stream_fd() failed: %s\n", avb_strerror(stream_fd));
		rc = -1;
		goto error_stream_fd;
	}

	/*
	* run listener/talker main processing function
	*/

	if (app.stream_params.direction & AVTP_DIRECTION_TALKER)
		rc = run_talker(stream_h, stream_fd, app.stream_batch_size);
	else
		rc = run_listener(stream_h, stream_fd, app.stream_batch_size);

	/*
	* main processing loop exited. could be due to error or avdecc disconnect
	*/
	if (rc < 0) {
		printf("Loop function exited with error code %d\n", rc);
	} else {
		printf("Loop function exited upon avdecc disconnect\n");

		avb_stream_destroy(stream_h);

		if (app.ctrl_h) {
			avb_control_close(app.ctrl_h);
			app.ctrl_h = NULL;
		}

		goto wait_new_stream;
	}

	/*
	* destroy the stream, disconnect for avb stack and close media files
	*/

error_stream_fd:
	avb_stream_destroy(stream_h);

error_stream_create:
	if (app.mode == MODE_TALKER)
		msrp_talker_deregister(&default_stream_params);
	else if (app.mode == MODE_LISTENER)
		msrp_listener_deregister(&default_stream_params);

err_msrp:
	close(app.media_fd);

error_ctrl_poll:
	if (app.ctrl_h)
		avb_control_close(app.ctrl_h);

error_control_open:
	if (app.mode != MODE_AVDECC)
		msrp_exit();
err_msrp_init:
	avb_exit(avb_h);

error_avb_init:
exit:
	return rc;

}
