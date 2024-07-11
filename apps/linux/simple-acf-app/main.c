/*
 * Copyright 2014-2016 Freescale Semiconductor, Inc.
 * Copyright 2016, 2018-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <sys/timerfd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sched.h>
#include <signal.h>

#include <genavb/genavb.h>
#include <genavb/helpers.h>
#include <genavb/srp.h>
#include <genavb/acf.h>

#include "../common/common.h"
#include "../common/log.h"
#include "../common/time.h"
#include "../common/msrp.h"

/* Application main modes */
#define MODE_LISTENER	0 /* acting as media files server if avdecc is not used*/
#define MODE_TALKER	1 /* acting as media files server if avdecc is not used*/

/* GenAVB stack cofiguration */
#define FLAG_IOV	(1 << 3) /* use iov array for data and event */

#define PROCESS_PRIORITY	60 /* RT_FIFO priority to be used for the process */

/* default file name used for media */
#define DEFAULT_MEDIA_FILE_NAME "media.raw"

#define DEFAULT_LOG_FILE_NAME "/var/log/avb_media_app"

#define MAX_DATA_BUF_SZ	(4*K)
#define MAX_EVENT_BUF_SZ	(512)

#define MAX_FRAME_SIZE 	256
#define MAX_INTERVAL_FRAMES 	1
#define ACF_PAYLOAD_SZ 	128
#define ACF_TX_INTERVAL_US 	1000000


#define BATCH_SIZE	1

avb_u8 default_stream_id[8] =  { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0 };
avb_u8 default_dst_mac[6] = { 0x91, 0xE0, 0xF0, 0x00, 0xeb, 0x15 };

static int signal_terminate = 0;

struct can_stats {
	unsigned long long num_rx_count;
	unsigned long long num_rx_lost;
	unsigned long long num_rx_error;
	unsigned int rx_size;
	unsigned char rx_bus_id;
	unsigned long long num_tx_count;
};

struct avtp_stats {
	unsigned long long num_rx_count;
	unsigned long long num_rx_lost;
	unsigned long long num_rx_ts_invalid;
	unsigned long long num_tx_count;
};

struct app_stats {
	struct avtp_stats avtp;
	struct can_stats can;
	struct stats latency_stats;
	struct stats sync_stats;
};

/* application main context */
struct acf_app {
	unsigned int mode;
	unsigned int config;
	unsigned int avtp_subtype;
	unsigned int sr_class;
	unsigned int vlan_id;
	int media_fd;
	char *media_file_name;
	char *log_file_name;
	struct genavb_stream_params stream_params;
	unsigned int stream_batch_size;
	unsigned int stream_flags;
	unsigned int max_frame_size;
	unsigned int max_interval_frames;
	unsigned long tx_interval;
	unsigned int tx_burst;
	unsigned int acf_payload_size;
	unsigned long stats_interval_ms;
	struct app_stats stats;
	int timer_stats_fd;
	int timer_process_fd;
};

struct acf_app app;

static void listener_stats_print(void);

static void usage (void)
{
	printf("\nUsage:\napp [options]\n");
	printf("\nOptions:\n"
		"Talker or Listener:\n"
		"\t-m <mode>                 application mode: (default) listener, talker\n"
		"\t-f <file_name>            media file name (default media.raw)\n"
		"\t-t <avtp_type>            avtp sub type: ntscf (default),  tscf\n"
		"\t-s <sr_class>             stream reservation class: none , a (default), b\n"
		"\t-L <file_name             log file name, (default /var/log/avb_media_app), b\n"
		"\t-h                        print this help text\n"
		"Talker specific:\n"
		"\t-v <vlan_id>              vlan id: 0 (default)\n"
		"\t-p <payload_size>         ACF payload size in bytes: 128 (default)\n"
		"\t-i <transmit_interval>    transmit interval in us: 1000000 (default)\n"
		"\t-b <burst_interval>       number of packet to transmit per interval: 1 (default)\n"
		"\t-S <stats interval>       statistics interval in msesc: 1000 (default)\n");
}

static void set_avb_config(unsigned int *avb_flags)
{
	*avb_flags = 0;
}

static void set_stream_params(struct genavb_stream_params *stream_params)
{
	if (app.mode == MODE_TALKER) {
		stream_params->direction = AVTP_DIRECTION_TALKER;
		stream_params->talker.vlan_id = htons(app.vlan_id);
		stream_params->talker.priority = 0;
		stream_params->clock_domain = GENAVB_MEDIA_CLOCK_DOMAIN_PTP;
		stream_params->talker.latency = max(CFG_TALKER_LATENCY_NS, sr_class_interval_p(stream_params->stream_class) / sr_class_interval_q(stream_params->stream_class));
		stream_params->talker.max_frame_size = (avb_u16)app.max_frame_size;
		stream_params->talker.max_interval_frames = (avb_u16)app.max_interval_frames;
	} else {
		stream_params->direction = AVTP_DIRECTION_LISTENER;
		stream_params->clock_domain = GENAVB_MEDIA_CLOCK_DOMAIN_STREAM;
	}

	stream_params->stream_class = (sr_class_t)app.sr_class;

	stream_params->subtype = app.avtp_subtype;
	if (stream_params->subtype == AVTP_SUBTYPE_NTSCF) {
		memset(&stream_params->format.u.raw, 0, sizeof(struct avdecc_format));
	} else {
		stream_params->format.u.s.v = 0,
		stream_params->format.u.s.subtype = AVTP_SUBTYPE_TSCF,
		stream_params->format.u.s.subtype_u.tscf.m = 0;
		stream_params->format.u.s.subtype_u.tscf.t3v = 0;
		stream_params->format.u.s.subtype_u.tscf.type_3 = 0;
		stream_params->format.u.s.subtype_u.tscf.t2v = 0;
		stream_params->format.u.s.subtype_u.tscf.type_2 = 0;
		stream_params->format.u.s.subtype_u.tscf.t1v = 0;
		stream_params->format.u.s.subtype_u.tscf.type_1 = 0;
		stream_params->format.u.s.subtype_u.tscf.t0v = 1;
		stream_params->format.u.s.subtype_u.tscf.type_0 = 0x78;
	}

	stream_params->flags = GENAVB_STREAM_FLAGS_CUSTOM_TSPEC;

	stream_params->port = 0;
	memcpy(stream_params->stream_id, default_stream_id, 8);
	print_stream_id(stream_params->stream_id);
	memcpy(stream_params->dst_mac, default_dst_mac, 6);
}

static int apply_config(void)
{
	avb_u64 stream_id;

	app.stream_batch_size = BATCH_SIZE;

	app.stream_flags = AVTP_NONBLOCK |AVTP_DGRAM;

	if (app.media_file_name == NULL)
		app.media_file_name = DEFAULT_MEDIA_FILE_NAME;

	if (app.mode == MODE_LISTENER)
		app.media_fd = open(app.media_file_name, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	else
		app.media_fd = open(app.media_file_name, O_RDONLY);

	if (app.media_fd < 0) {
		printf("open(%s) failed: %s\n", app.media_file_name, strerror(errno));
		goto error_media_open;
	}

	app.max_frame_size = (sizeof(struct avtp_data_hdr) + sizeof(struct acf_msg) + sizeof(struct acf_can_hdr) + app.acf_payload_size);

	/*
	* display the whole configuration
	*/

	printf("mode: ");
	if (app.mode == MODE_LISTENER)
		printf("LISTENER\n");
	else
		printf("TALKER\n");

	printf("media file name: %s (fd %d)\n", app.media_file_name, app.media_fd);
	printf("log_file_name: %s\n", app.log_file_name);

	printf("flags: ");
	if (app.config & FLAG_IOV)
		printf("IOV\n");
	else
		printf("none\n");

	printf("subtype: ");
	if (app.avtp_subtype == AVTP_SUBTYPE_TSCF)
		printf("TSCF\n");
	else
		printf("NTSCF\n");

	printf("SR class: ");
	if (app.sr_class == SR_CLASS_A)
		printf("CLASS A\n");
	else if (app.sr_class == SR_CLASS_B)
		printf("CLASS B\n");
	else
		printf("NONE\n");

	printf("vlan ID: %u\n", app.vlan_id);

	memcpy(&stream_id, default_stream_id, 8);
	printf("Stream ID: %016"PRIx64"\n", stream_id);

	if (app.mode == MODE_TALKER) {
		printf("acf payload size: %u\n", app.acf_payload_size);
		printf("tx interval: %lu\n", app.tx_interval);
		printf("tx burst: %u\n", app.tx_burst);
	}

	printf("\n\n");

	return 0;

error_media_open:
	return -1;
}

static void acf_stats_print(struct stats *s)
{
	INF("%s min %d mean %d max %d rms^2 %llu stddev^2 %llu", (char*)s->priv, s->min, s->mean, s->max, s->ms, s->variance);
}

static void listener_stats_print(void)
{
	stats_compute(&app.stats.latency_stats);
	acf_stats_print(&app.stats.latency_stats);
	stats_reset(&app.stats.latency_stats);

	if (app.avtp_subtype == AVTP_SUBTYPE_TSCF) {
		stats_compute(&app.stats.sync_stats);
		acf_stats_print(&app.stats.sync_stats);
		stats_reset(&app.stats.sync_stats);
	}

	INF("CAN: size %u, bus_id 0x%x, rx %llu, lost %llu", app.stats.can.rx_size, app.stats.can.rx_bus_id, app.stats.can.num_rx_count, app.stats.can.num_rx_lost);

	INF("AVTP: rx count %llu, rx lost %llu, ts invalid %llu\n", app.stats.avtp.num_rx_count, app.stats.avtp.num_rx_lost, app.stats.avtp.num_rx_ts_invalid);
}

#define LISTENER_STAT_FD_IDX 		0
#define LISTENER_STREAM_FD_IDX 	1
static int listener_nonblocking(struct genavb_stream_handle *stream_h, int stream_fd, unsigned int batch_size, int file_dst, int stats_fd)
{
	unsigned char acf_msg_buf[MAX_DATA_BUF_SZ];
	struct genavb_event event[MAX_EVENT_BUF_SZ] = {0};
	unsigned int event_len = MAX_EVENT_BUF_SZ;
	struct pollfd poll_fds[3];
	uint64_t now, message_origin_timestamp;
	int ready, i, n, nfds, nbytes;
	int rc = 0;

	struct acf_msg *acf_hdr;
	unsigned int acf_payload_length, acf_payload_offset;

	struct acf_can_hdr *can_hdr;
	unsigned char can_previous_seq = 0;
	unsigned long long can_lost_packets = 0;

	INF("Starting listener loop, non-blocking mode");

	/*
	* listen to read event from the stack
	*/

	nfds = 0;

	poll_fds[LISTENER_STAT_FD_IDX].fd = stats_fd;
	poll_fds[LISTENER_STAT_FD_IDX].events = POLLIN;
	nfds++;

	poll_fds[LISTENER_STREAM_FD_IDX].fd = stream_fd;
	poll_fds[LISTENER_STREAM_FD_IDX].events = POLLIN;
	nfds++;

	poll_fds[nfds].fd = -1;
	poll_fds[nfds].events = 0;

	stats_init(&app.stats.latency_stats, 31, "Latency (ns)", NULL);
	stats_init(&app.stats.sync_stats, 31, "Synchro (ns)", NULL);

	while (1) {
		if (signal_terminate) {
			INF("processing terminate signal");
			rc = -1;
			goto exit;
		}

		if ((ready = poll(poll_fds, nfds, -1)) == -1) {
			if (errno == EINTR)
				continue;
			else {
				ERR("poll(%d) failed while processing listener errno %d: %s", stream_fd, errno, strerror(errno));
				rc = -1;
				goto exit;
			}
		}

		if (ready > 0) {
			for (n = 0, i = 0; i < nfds && n < ready; i++) {
				if (poll_fds[LISTENER_STREAM_FD_IDX].revents && POLLIN) {
					n++;

					if (gettime_ns(&now) < 0) {
						rc = -1;
						goto exit;
					}

					/*
					 * read data from stack...
					 */
					event_len = MAX_EVENT_BUF_SZ;
					nbytes = genavb_stream_receive(stream_h, acf_msg_buf, MAX_DATA_BUF_SZ, event, &event_len);
					if (nbytes <= 0) {
						if (nbytes < 0)
							ERR("genavb_stream_receive() failed: %s", genavb_strerror(nbytes));
						else
							ERR("genavb_stream_receive() incomplete");

						rc = nbytes;
						goto exit;
					}

					if (event_len != 0) {
						if (event[0].event_mask & AVTP_MEDIA_CLOCK_RESTART)
							INF("AVTP media clock restarted");

						if (event[0].event_mask & AVTP_PACKET_LOST) {
							INF("AVTP packet lost");
							app.stats.avtp.num_rx_lost++;
						}

						if (event[0].event_mask & AVTP_END_OF_FRAME)
							INF("AVTP end of frame (size %d)", event[0].index);

						if (event[0].event_mask & (AVTP_TIMESTAMP_INVALID | AVTP_TIMESTAMP_UNCERTAIN)) {
							INF("AVTP timestamp not valid");
							app.stats.avtp.num_rx_ts_invalid++;
						} else {
							/* log synchronization accuracy between now and presentation time */
							stats_update(&app.stats.sync_stats, (unsigned int)now - event[0].ts);
						}
					}

					acf_hdr = (struct acf_msg *)acf_msg_buf;
					acf_payload_offset = sizeof(struct acf_msg);
					acf_payload_length = (ACF_MSG_LENGTH(acf_hdr) << 2) - sizeof(struct acf_msg);
					DBG("msg_type: 0x%x msg_length: %u nbytes: %u", acf_hdr->acf_msg_type, (ACF_MSG_LENGTH(acf_hdr) << 2), nbytes);

					switch (acf_hdr->acf_msg_type) {
					case ACF_MSG_TYPE_CAN :
						can_hdr = (struct acf_can_hdr*)(&acf_msg_buf[acf_payload_offset]);
						acf_payload_offset += sizeof(struct acf_can_hdr);
						acf_payload_length -= sizeof(struct acf_can_hdr);
						message_origin_timestamp = ntohll(can_hdr->message_timestamp);

						/* compute average end to end latency */
						stats_update(&app.stats.latency_stats, (unsigned long)(now - message_origin_timestamp));

						/* compute cumulative packet loss */
						if (app.stats.can.num_rx_count) {
							if (acf_msg_buf[acf_payload_offset] >= (unsigned char)(can_previous_seq + 1))
								can_lost_packets = acf_msg_buf[acf_payload_offset] -(unsigned char)(can_previous_seq + 1);
							else
								can_lost_packets = (unsigned char)(255 -can_previous_seq) + acf_msg_buf[acf_payload_offset];
						}
						can_previous_seq = acf_msg_buf[acf_payload_offset];

						app.stats.can.num_rx_count++;
						app.stats.can.num_rx_lost += can_lost_packets;
						app.stats.can.rx_bus_id = can_hdr->can_bus_id;
						app.stats.can.rx_size = nbytes;

						DBG("CAN: size %u bus_id 0x%x, rx %llu, lost %llu",  app.stats.can.rx_size, app.stats.can.rx_bus_id, app.stats.can.num_rx_count, app.stats.can.rx_lost);
						break;
					default:
						ERR("Unsupported message type received: 0x%x", acf_hdr->acf_msg_type);
						app.stats.can.num_rx_error++;
						goto exit;
					}

					 app.stats.avtp.num_rx_count++;

					/*
					 * ...and write to local file
					 */
					rc = write(file_dst, &acf_msg_buf[acf_payload_offset], acf_payload_length);
					if (rc < acf_payload_length) {
						if (rc < 0)
							ERR("write() failed: %s", strerror(errno));
						else
							ERR("write() incomplete");

						goto exit;
					}

					DBG("rx count %llu, rx lost: %llu; ts invalid: %llu\n", app.stats.avtp.num_rx_count, app.stats.avtp.num_rx_lost, app.avtp.stats.num_rx_ts_invalid);
				}

				if (poll_fds[LISTENER_STAT_FD_IDX].revents && POLLIN) {
					char tmp[8];
					n++;
					if ((nbytes = read(poll_fds[i].fd, tmp, 8)) < 8) {
						if (nbytes < 0) {
							printf("stats_fd read() failed: %s\n", strerror(errno));
							goto exit;
						}
					} else
						listener_stats_print();
				}
			}
		}
	}
exit:
	return rc;
}


static void talker_stats_print(void)
{
	INF("%s: tx count %llu", (app.avtp_subtype == AVTP_SUBTYPE_TSCF)?"TSCF":"NTSCF", app.stats.avtp.num_tx_count);
}

#define TALKER_PROCESS_FD_IDX 0
#define TALKER_STAT_FD_IDX 1
#define TALKER_STREAM_FD_IDX 2
static int talker_nonblocking(struct genavb_stream_handle *stream_h, int stream_fd, unsigned int batch_size, int file_src, int process_fd, int stats_fd)
{
	unsigned char acf_msg_buf[MAX_DATA_BUF_SZ];
	struct genavb_event event[MAX_EVENT_BUF_SZ];
	struct pollfd poll_fds[4];
	int nbytes, ready, i, n, nfds;
	uint64_t start_time;
	unsigned int event_n;
	struct acf_msg *acf_hdr;
	struct acf_can_hdr *can_hdr;
	unsigned int nburst = 0;
	int rc = 0;

	INF("Starting talker loop, non-blocking mode (fds: %d - %d - %d)", stream_fd, process_fd, stats_fd);

	lseek(file_src, 0, SEEK_SET);

	nfds = 0;

	/*
	* listen to timer event for talker transmit
	*/
	poll_fds[TALKER_PROCESS_FD_IDX].fd = process_fd;
	poll_fds[TALKER_PROCESS_FD_IDX].events = POLLIN;
	poll_fds[TALKER_PROCESS_FD_IDX].revents = 0;
	nfds++;

	/*
	* listen to timer event for stats output
	*/
	poll_fds[TALKER_STAT_FD_IDX].fd = stats_fd;
	poll_fds[TALKER_STAT_FD_IDX].events = POLLIN;
	poll_fds[TALKER_STAT_FD_IDX].revents = 0;
	nfds++;

	/*
	* listen to write event from the stack
	*/
	poll_fds[TALKER_STREAM_FD_IDX].fd = stream_fd;
	poll_fds[TALKER_STREAM_FD_IDX].events = POLLOUT;
	poll_fds[TALKER_STREAM_FD_IDX].revents = 0;
	nfds++;

	poll_fds[nfds].fd = -1;
	poll_fds[nfds].events = 0;
	poll_fds[nfds].revents = 0;

	app.stats.avtp.num_tx_count = 0;

	while (1) {
		if (signal_terminate) {
			INF("processing terminate signal");
			rc = -1;
			goto exit;
		}

		if ((ready = poll(poll_fds, nfds, -1)) == -1) {
			if (errno == EINTR)
				continue;
			else {
				INF("poll(%d) failed while processing talker errno %d: %s", stream_fd, errno, strerror(errno));
				rc = -1;
				goto exit;
			}
		}

		if (ready > 0) {
			for (n = 0, i = 0; i < nfds && n < ready; i++) {
				if (poll_fds[TALKER_STAT_FD_IDX].revents & POLLIN) {
					n++;
					char tmp[8];
					if ((nbytes = read(poll_fds[TALKER_STAT_FD_IDX].fd, tmp, 8)) < 8) {
						if (nbytes < 0) {
							printf("stats timer_fd read() failed: %s\n", strerror(errno));
							goto exit;
						}
					} else
						talker_stats_print();
				}

				if (poll_fds[TALKER_PROCESS_FD_IDX].revents & POLLIN) {
					n++;
					char tmp[8];
					if ((nbytes = read(poll_fds[TALKER_PROCESS_FD_IDX].fd, tmp, 8)) < 8) {
						if (nbytes < 0) {
							printf("process timer_fd read() failed: %s\n", strerror(errno));
							goto exit;
						}
					} else
						/*add stream_fd */
						poll_fds[TALKER_STREAM_FD_IDX].fd = stream_fd;
				}

				if (poll_fds[TALKER_STREAM_FD_IDX].revents & POLLOUT) {
						n++;
read_again:
						/* reading dummy data from file (emulates data from a CAN bus) */
						nbytes = read(app.media_fd, &acf_msg_buf[sizeof(struct acf_msg) + sizeof(struct acf_can_hdr)], app.acf_payload_size);
						if (nbytes < app.acf_payload_size ) {
							if (nbytes < 0) {
								rc = nbytes;
								goto exit;
							} else {
								lseek(app.media_fd, 0, SEEK_SET);
								goto read_again;
							}
						}

						acf_hdr = (struct acf_msg *)acf_msg_buf;
						acf_hdr->acf_msg_type = ACF_MSG_TYPE_CAN;
						ACF_MSG_LENGTH_SET(acf_hdr, (sizeof(struct acf_msg) + sizeof(struct acf_can_hdr) + app.acf_payload_size ) >> 2);  /* number of quadlets */

						can_hdr = (struct acf_can_hdr *)(&acf_msg_buf[sizeof(struct acf_msg)]);
						memset(can_hdr, 0, sizeof(struct acf_can_hdr));
						can_hdr->mtv = 1;

						if (gettime_ns(&start_time) < 0) {
							rc = -1;
							goto exit;
						}

						can_hdr->message_timestamp = htonll(start_time);
						can_hdr->can_bus_id = 0x17; /* arbitrary value */

						/* for debug purpose (sequence number) */
						acf_msg_buf[sizeof(struct acf_msg) + sizeof(struct acf_can_hdr)] = (unsigned char)++app.stats.avtp.num_tx_count;

						/* add presentation timestamp depending on subtype */
						if (app.avtp_subtype == AVTP_SUBTYPE_TSCF) {
							event[0].index = (ACF_MSG_LENGTH(acf_hdr) << 2) - 1;
							event[0].event_mask =  AVTP_SYNC;
							event[0].ts = start_time + genavb_stream_presentation_offset(stream_h);
							event_n = 1;
							rc = genavb_stream_send(stream_h, acf_msg_buf, (ACF_MSG_LENGTH(acf_hdr) << 2), event, event_n);
						} else {
							rc = genavb_stream_send(stream_h, acf_msg_buf, (ACF_MSG_LENGTH(acf_hdr) << 2), NULL, 0);
						}

						if (rc != (ACF_MSG_LENGTH(acf_hdr) << 2)) {
							if (rc < 0)
								ERR("genavb_stream_send() failed, rc = %s", genavb_strerror(rc));
							else
								ERR("genavb_stream_send() incomplete");

							goto exit;
						}

						if (app.avtp_subtype == AVTP_SUBTYPE_TSCF)
							DBG("TSCF packet sent: %d bytes, num_tx_count %llu,  ts %x\n",  (ACF_MSG_LENGTH(acf_hdr) << 2), app.stats.avtp.num_tx_count, event[0].ts);
						else
							DBG("NTSCF packet sent: %d bytes, num_tx_count %llu\n", (ACF_MSG_LENGTH(acf_hdr) << 2), app.stats.avtp.num_tx_count);

						nburst++;

						if (nburst >= app.tx_burst) {
							/* remove stream_fd until next transmit interval */
							if (app.tx_interval)
								poll_fds[TALKER_STREAM_FD_IDX].fd = -1;

							nburst = 0;
						} else
							goto read_again;
				}
			}
		}
	}

exit:
	return rc;
}


static int run_listener(struct genavb_stream_handle *stream_h, int stream_fd, unsigned int batch_size)
{
	int rc;

	rc = listener_nonblocking(stream_h, stream_fd, batch_size, app.media_fd, app.timer_stats_fd);

	return rc;
}


static int run_talker(struct genavb_stream_handle *stream_h, int stream_fd, unsigned int batch_size)
{
	int rc;

	rc = talker_nonblocking(stream_h, stream_fd, batch_size, app.media_fd, app.timer_process_fd, app.timer_stats_fd);

	return rc;
}


void signal_terminate_handler (int signal_num)
{
	signal_terminate = 1;
}


static void set_signal_handlers(void)
{
	struct sigaction action;

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
}

static int set_timers_fd(void)
{
	unsigned long timer_msecs, timer_usecs;
	struct itimerspec its;

	app.timer_process_fd = timerfd_create(CLOCK_MONOTONIC, 0);
	if (app.timer_process_fd < 0) {
		printf("%s timerfd_create() failed %s\n", __func__, strerror(errno));
		goto err_process_timer_create;
	}

	#define NSECS_PER_USEC 1000
	timer_usecs = app.tx_interval;
	its.it_value.tv_sec = timer_usecs / USECS_PER_SEC;
	its.it_value.tv_nsec =  (timer_usecs % USECS_PER_SEC) * NSECS_PER_USEC;
	its.it_interval.tv_sec = its.it_value.tv_sec;
	its.it_interval.tv_nsec = its.it_value.tv_nsec;
	if (timerfd_settime(app.timer_process_fd, 0, &its, NULL) < 0) {
		printf("%s timerfd_settime() failed %s\n", __func__, strerror(errno));
		goto err_process_timer_set;
	}

	app.timer_stats_fd = timerfd_create(CLOCK_MONOTONIC, 0);
	if (app.timer_stats_fd < 0) {
		printf("%s timerfd_create() failed %s\n", __func__, strerror(errno));
		goto err_stats_timer_create;
	}

	timer_msecs = app.stats_interval_ms;
	its.it_value.tv_sec = timer_msecs / MSECS_PER_SEC;
	its.it_value.tv_nsec =  (timer_msecs % MSECS_PER_SEC) * NSECS_PER_MSEC;
	its.it_interval.tv_sec = its.it_value.tv_sec;
	its.it_interval.tv_nsec = its.it_value.tv_nsec;
	if (timerfd_settime(app.timer_stats_fd, 0, &its, NULL) < 0) {
		printf("%s timerfd_settime() failed %s\n", __func__, strerror(errno));
		goto err_stats_timer_set;
	}

	return 0;

err_stats_timer_set:
	close(app.timer_stats_fd);

err_stats_timer_create:
err_process_timer_set:
	close(app.timer_process_fd);

err_process_timer_create:
	return -1;

}

int main(int argc, char *argv[])
{
	struct genavb_handle *avb_h;
	struct genavb_stream_handle* stream_h = NULL;
	struct sched_param param = {
		.sched_priority = PROCESS_PRIORITY,
	};
	unsigned long optval;
	unsigned int avb_flags;
	int stream_fd, option;
	int rc = 0;

	/*
	* Increase process priority to match the AVTP thread priority
	*/

	if (sched_setscheduler(0, SCHED_FIFO, &param) < 0) {
		printf("sched_setscheduler(), %s\n", strerror(errno));
		rc = -1;
		goto exit;
	}

	setlinebuf(stdout);

	printf("NXP's GenAVB reference ACF application\n");

	/*
	* retrieve user's configuration parameters
	*/

	/* default settings */
	app.mode = MODE_LISTENER; app.config = 0; /*LISTENER, NON-BLOCKING, SINGLE BUFFER */
	app.avtp_subtype = AVTP_SUBTYPE_NTSCF ;  /* NTSCF */
	app.vlan_id = 0; app.sr_class = SR_CLASS_A ; /* no stream reservation */
	app.tx_interval = ACF_TX_INTERVAL_US; app.tx_burst = 1;   /* 1 packet every ACF_TX_INTERVAL_US interval */
	app.acf_payload_size = ACF_PAYLOAD_SZ;
	app.max_frame_size = (sizeof(struct avtp_data_hdr) + sizeof(struct acf_msg) + sizeof(struct acf_can_hdr) + app.acf_payload_size);
	app.max_interval_frames = MAX_INTERVAL_FRAMES;
	app.stats_interval_ms = MSECS_PER_SEC;
	app.log_file_name = DEFAULT_LOG_FILE_NAME;

	while ((option = getopt(argc, argv,"m:f:L:t:s:v:p:i:b:S:I:h")) != -1) {
		switch (option) {
		case 'm':
			if (!strcasecmp(optarg, "listener"))
				app.mode = MODE_LISTENER;
			else if (!strcasecmp(optarg, "talker"))
				app.mode = MODE_TALKER;
			else {
				usage();
				goto exit;
			}
			break;

		case 'f':
			app.media_file_name = optarg;
			break;

		case 'L':
			app.log_file_name = optarg;
			break;

		case 't':
			if (!strcasecmp(optarg, "tscf"))
				app.avtp_subtype = AVTP_SUBTYPE_TSCF;
			else if (!strcasecmp(optarg, "ntscf"))
				app.avtp_subtype = AVTP_SUBTYPE_NTSCF;
			else {
				usage();
				goto exit;
			}
			break;

		case 's':
			if (!strcasecmp(optarg, "a"))
				app.sr_class = SR_CLASS_A;
			else if (!strcasecmp(optarg, "b"))
				app.sr_class = SR_CLASS_B;
			else if (!strcasecmp(optarg, "none"))
				app.sr_class = SR_CLASS_NONE;
			else {
				usage();
				goto exit;
			}
			break;

		case 'v':
			if(h_strtoul(&optval, optarg, NULL, 0) < 0) {
				printf("vlan_id not a valid unsigned integer\n");
				rc = -1;
				goto exit;
			}
			app.vlan_id = (unsigned int)optval;
			break;

		case 'p':
			if ((h_strtoul(&optval, optarg, NULL, 0) < 0) || (!optval)) {
				printf("acf_payload_size not a valid unsigned integer\n");
				rc = -1;
				goto exit;
			}
			app.acf_payload_size = (unsigned int)optval;
			break;

		case 'i':
			if (h_strtoul(&app.tx_interval, optarg, NULL, 0) < 0) {
				printf("tx_interval not a valid unsigned integer\n");
				rc = -1;
				goto exit;
			}
			break;

		case 'b':
			if (h_strtoul(&optval, optarg, NULL, 0) < 0) {
				printf("tx_burst not a valid unsigned integer\n");
				rc = -1;
				goto exit;
			}
			app.tx_burst = (unsigned int)optval;
			break;

		case 'S':
			 if (h_strtoul(&app.stats_interval_ms, optarg, NULL, 0) < 0) {
				printf("stats_interval_ms not a valid unsigned integer\n");
				rc = -1;
				goto exit;
			}
			break;

		case 'I':

			if (h_strtoul(&optval, optarg, NULL, 16) < 0) {
				printf("stream ID modifier not a valid unsigned integer\n");
				rc = -1;
				goto exit;
			}
			default_stream_id[0] = (avb_u8)optval;
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
	set_signal_handlers();

	/*
	* timers file descriptors for stats and talker processing interval
	*/
	if (set_timers_fd() < 0)
		goto err_timerfd;

	rc = aar_log_init(app.log_file_name);
	if (rc < 0)
		goto err_log_init;

	/*
	* setup the avb stack
	*/

	set_avb_config(&avb_flags);

	rc = genavb_init(&avb_h, avb_flags);
	if (rc != GENAVB_SUCCESS) {
		ERR("genavb_init() failed, rc = %s", genavb_strerror(rc));
		rc = -1;
		goto error_avb_init;
	}

	rc = msrp_init(avb_h);
	if (rc < 0) {
		ERR("msrp_init() failed");
		goto err_msrp_init;
	}

wait_new_stream:
	printf("\nwait for new stream...\n");

	if (apply_config() < 0) {
		rc = -1;
		goto error_apply_config;
	}

	set_stream_params(&app.stream_params);

	/*
	* setup the stream
	*/

	rc = genavb_stream_create(avb_h, &stream_h, &app.stream_params, &app.stream_batch_size, app.stream_flags);
	if (rc != GENAVB_SUCCESS) {
		ERR("genavb_stream_create() failed, rc = %s", genavb_strerror(rc));
		rc = -1;
		goto error_stream_create;
	}

	/*
	* setup the stream reservation
	*/
	if (app.sr_class != SR_CLASS_NONE) {
		if (app.stream_params.direction & AVTP_DIRECTION_TALKER) {
			rc = msrp_talker_register(&app.stream_params);
			if (rc != GENAVB_SUCCESS) {
				ERR("msrp_talker_register error, rc = %s", genavb_strerror(rc));
				goto err_msrp_register;
			}
		} else {
			rc = msrp_listener_register(&app.stream_params);
			if (rc != GENAVB_SUCCESS) {
				ERR("msrp_listener_register error, rc = %s", genavb_strerror(rc));
				goto err_msrp_register;
			}
		}
	}

	/* additional delay for SRP protocol establisment */
	sleep(4);

	/*
	* retrieve the file descriptor associated to the stream
	*/

	stream_fd = genavb_stream_fd(stream_h);
	if (stream_fd < 0) {
		ERR("genavb_stream_fd() failed, rc = %s\n", genavb_strerror(stream_fd));
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
	* main processing loop exited.
	*/
	if (rc < 0) {
		ERR("Loop function exited with error code %d\n", rc);
	} else {
		printf("Loop function exited\n");
		genavb_stream_destroy(stream_h);
		goto wait_new_stream;
	}

	/*
	* destroy the stream, disconnect for avb stack and close media files
	*/

error_stream_fd:
	if (app.sr_class != SR_CLASS_NONE) {
		if (app.stream_params.direction & AVTP_DIRECTION_TALKER)
			msrp_talker_deregister(&app.stream_params);
		else
			msrp_listener_deregister(&app.stream_params);
	}

err_msrp_register:
	genavb_stream_destroy(stream_h);

error_stream_create:
	close(app.media_fd);

error_apply_config:
	msrp_exit();

err_msrp_init:
	genavb_exit(avb_h);

error_avb_init:
	aar_log_exit();

err_log_init:
err_timerfd:
	if (app.timer_stats_fd > 0)
		close(app.timer_stats_fd);
	if (app.timer_process_fd > 0)
		close(app.timer_process_fd);

exit:
	return rc;

}
