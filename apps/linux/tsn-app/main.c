/*
 * Copyright 2020-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

#include <genavb/genavb.h>
#include <genavb/helpers.h>

#include "../common/log.h"
#include "../common/timer.h"
#include "../common/thread.h"

#include "tsn_tasks_config.h"
#include "cyclic_task.h"
#include "serial_controller.h"
#include "network_only.h"
#include "opcua/opcua_server.h"

#define STATS_THREAD_PERIOD_SEC 1
#define NUM_CONTROL_FDS		2
#define PTS_NAME_STR_LEN	30
#define TSN_APP_LOG "/var/log/tsn_app"

static int signal_terminate = 0;

enum app_mode {
	NETWORK_ONLY,
	SERIAL
};

struct stats_ctx {
	int fd;
	thr_thread_slot_t *thread_slot;
	void (*handler)(void *data);
	void *data;
};

static int stats_handler(void *data, unsigned int events)
{
	int rc;
	uint64_t n_exp;
	struct stats_ctx *ctx = data;

	rc = read(ctx->fd, &n_exp, sizeof(n_exp));
	if (rc < 0) {
		ERR("read error: %s", strerror(errno));
		goto err;
	}

	if (n_exp > 0) {
		aar_log_update_time(GENAVB_CLOCK_GPTP_0_0);

		if (ctx->handler)
			ctx->handler(ctx->data);

		thread_print_stats();
	}

	return 0;

err:
	return -1;
}

static int stats_thread_init(struct stats_ctx *ctx, void (*handler)(void *), void *data)
{
	ctx->fd = create_timerfd_periodic(CLOCK_MONOTONIC);
	if (ctx->fd < 0) {
		ERR("create_timer_fd_periodic() failed");
		goto err;
	}

	INF("%s: Add the stats thread slot\n", __func__);

	ctx->handler = handler;
	ctx->data = data;

	if (thread_slot_add(THR_CAP_STATS, ctx->fd, EPOLLIN, ctx, stats_handler, NULL, 0, &ctx->thread_slot) < 0) {
		ERR("thread_slot_add() failed");
		goto err_close;
	}

	if (start_timerfd_periodic(ctx->fd, STATS_THREAD_PERIOD_SEC, 0) < 0) {
		ERR("start_timerfd_periodic() failed");
		goto err_close;
	}

	return 0;

err_close:
	close(ctx->fd);

err:
	return -1;
}

static void stats_thread_exit(struct stats_ctx *ctx)
{
	stop_timerfd(ctx->fd);
	close(ctx->fd);
}

static int pseudo_tty_init(char *slave_file)
{
	int fd, file_fd, rc;
	char *pts_name;

	fd = posix_openpt(O_RDWR | O_NOCTTY);
	if (fd < 0) {
		ERR("posix_openpt(): %s\n", strerror(errno));
		goto err;
	}

	rc = grantpt(fd);
	if (rc < 0) {
		ERR("grantpt(): %s\n", strerror(errno));
		goto err_close;
	}

	rc = unlockpt(fd);
	if (rc < 0) {
		ERR("unlockpt(): %s\n", strerror(errno));
		goto err_close;
	}

	pts_name = ptsname(fd);
	if (!pts_name) {
		ERR("ptsname() error\n");
		goto err_close;
	}

	if (slave_file) {
		ssize_t written;

		file_fd = open(slave_file, O_CREAT | O_WRONLY, S_IROTH);
		if (file_fd < 0) {
			ERR("open(): %s\n", strerror(errno));
			goto err_close;
		}

		written = write(file_fd, pts_name, strlen(pts_name));
		if (written < 0) {
			ERR("write(): %s\n", strerror(errno));
			goto err_close_file;
		}
		if (written < strlen(pts_name)) {
			ERR("write() incomplete\n");
			goto err_close_file;
		}

		close(file_fd);
	}

	INF("pts device: %s\n", pts_name);

	return fd;

err_close_file:
	close(file_fd);

err_close:
	close(fd);

err:
	return -1;
}

void signal_terminate_handler(int signal_num)
{
	signal_terminate = 1;
}

void sigusr1_handler(int sig)
{
    cyclic_stats_reset_handler();
}


static void usage(void)
{
	printf("\nUsage:\ntsn-app [options]\n");
	printf("\nOptions:\n"
	       "\t-m <mode>          supported mode: \"network_only\" or \"serial\" (default: \"network_only\")\n"
	       "\t-r <role>          supported role: \"controller\", \"io_device_0\" or \"io_device_1\" (default: \"controller\")\n"
	       "\t-s <sleep handler> supported sleep handlers: \"epoll\" or \"nanosleep\" (default: \"nanosleep\")\n"
	       "\t-p <period>        task period in nanoseconds (default: 2000000 ns)\n"
	       "\t-n <peers number>  number of IO devices (default: 1, only used if role is set to \"controller\"\n"
	       "\t-f <file name>     pts file name (default: don't write pts file name, only used if mode is set to \"serial\")\n"
		"\t-x                use AF_XDP sockets instead of standard raw sockets\n");
};

int main(int argc, char *argv[])
{
	unsigned int mode = NETWORK_ONLY;
	unsigned int role = CONTROLLER_0;
	unsigned int timer_type = TSN_TIMER_NANOSLEEP;
	unsigned long period_ns = 0;
	unsigned long num_peers = 0;
	unsigned int flags = 0;
	struct genavb_handle *genavb_handle;
	struct sched_param param = {
		.sched_priority = 1,
	};
	struct sigaction action;
	int option;
	int rc = 0;
	int pt_fd = -1;
	char *slave_file = NULL;
	struct stats_ctx stats_ctx;
	void (*stats_handler)(void *);
	void (*exit_fn)(void *) = NULL;
	void *ctx;

	//setlinebuf(stdout);

	while ((option = getopt(argc, argv, "hf:m:p:r:n:s:x")) != -1) {
		switch (option) {

		case 'f':
			slave_file = optarg;
			break;

		case 'm':
			if (!strcasecmp(optarg, "serial")) {
				mode = SERIAL;
			} else if (!strcasecmp(optarg, "network_only")) {
				mode = NETWORK_ONLY;
			} else {
				printf("invalid -m %s option\n", optarg);
				usage();
				goto err;
			}
			break;

		case 'p':
			if ((h_strtoul(&period_ns, optarg, NULL, 0) < 0) || !period_ns) {
				printf("invalid -p %s option\n", optarg);
				usage();
				goto err;
			}
			break;

		case 'n':
			if ((h_strtoul(&num_peers, optarg, NULL, 0) < 0) || !num_peers) {
				printf("invalid -n %s option\n", optarg);
				usage();
				goto err;
			}
			break;

		case 'r':
			if (!strcasecmp(optarg, "controller")) {
				role = CONTROLLER_0;
			} else if (!strcasecmp(optarg, "io_device_0")) {
				role = IO_DEVICE_0;
			} else if (!strcasecmp(optarg, "io_device_1")) {
				role = IO_DEVICE_1;
			} else {
				printf("invalid -r %s option\n", optarg);
				usage();
				goto err;
			}
			break;

		case 's':
			if (!strcasecmp(optarg, "epoll")) {
				timer_type = TSN_TIMER_EPOLL;
			} else if (!strcasecmp(optarg, "nanosleep")) {
				timer_type = TSN_TIMER_NANOSLEEP;
			} else {
				printf("invalid -s %s option\n", optarg);
				usage();
				goto err;
			}
			break;

		case 'x':
			flags = GENAVB_FLAGS_NET_XDP;
			break;
		case 'h':
			usage();
			goto err;
		default:
			rc = -1;
			usage();
			goto err;
		}
	}

	rc = aar_log_init(TSN_APP_LOG);
	if (rc < 0)
		goto err;

	INF("NXP's GenAVB/TSN stack reference TSN application\n");

	rc = genavb_init(&genavb_handle, flags);
	if (rc != GENAVB_SUCCESS) {
		ERR("genavb_init() failed: %s", avb_strerror(rc));
		rc = -1;
		goto err;
	}

	/*
	 * set signals handler
	 */
	action.sa_handler = signal_terminate_handler;
	action.sa_flags = 0;

	if (sigemptyset(&action.sa_mask) < 0)
		ERR("sigemptyset(): %s\n", strerror(errno));

	if (sigaction(SIGTERM, &action, NULL) < 0) /* Termination signal */
		ERR("sigaction(): %s\n", strerror(errno));

	if (sigaction(SIGQUIT, &action, NULL) < 0) /* Quit from keyboard */
		ERR("sigaction(): %s\n", strerror(errno));

	if (sigaction(SIGINT, &action, NULL) < 0) /* Interrupt from keyboard */
		ERR("sigaction(): %s\n", strerror(errno));

	action.sa_handler = sigusr1_handler;
	if (sigaction(SIGUSR1, &action, NULL) < 0) /* SIGUSR1 */
		ERR("sigaction(): %s\n", strerror(errno));; 

	rc = sched_setscheduler(0, SCHED_FIFO, &param);
	if (rc < 0) {
		ERR("sched_setscheduler() failed: %s\n", strerror(errno));
		goto err_avb_exit;
	}

	rc = thread_init();
	if (rc < 0)
		goto err_avb_exit;

	rc = opcua_server_init();
	if (rc < 0) {
		ERR("opcua_server_init() failed: %s\n", strerror(errno));
		goto err_thread_exit;
	}

	if (mode == SERIAL) {
		exit_fn = serial_controller_exit;
		stats_handler = serial_controller_stats_handler;

		if (role != CONTROLLER_0) {
			ERR("Only controller role is supported\n");
			rc = -1;
			goto err_opcua_exit;
		}

		pt_fd = pseudo_tty_init(slave_file);
		if (pt_fd < 0) {
			ERR("pseudo_tty_init() failed\n");
			rc = -1;
			goto err_opcua_exit;
		}

		ctx = serial_controller_init(period_ns, num_peers, pt_fd, timer_type);
		if (!ctx) {
			ERR("serial_controller_init() failed\n");
			goto err_close_pt;
		}
	} else if (mode == NETWORK_ONLY) {
		exit_fn = network_only_exit;
		stats_handler = network_only_stats_handler;

		ctx = network_only_init(role, period_ns, num_peers, timer_type);
		if (!ctx) {
			ERR("network_only_init() failed\n");
			goto err_opcua_exit;
		}
	} else {
		ERR("Invalid mode %d\n", mode);
		rc = -1;
		goto err_opcua_exit;
	}

	if (stats_thread_init(&stats_ctx, stats_handler, ctx) < 0) {
		ERR("stats_thread_init() failed: %s\n", strerror(errno));
		rc = -1;
		goto err_exit_fn;
	}

	while (1) {
		pause();
		if ((errno == EINTR) && signal_terminate) {
			INF("processing terminate signal\n");
			rc = -1;
			goto err_stats_exit;
		}
	}

err_stats_exit:
	stats_thread_exit(&stats_ctx);

err_exit_fn:
	if (exit_fn)
		exit_fn(ctx);

err_close_pt:
	if (pt_fd > 0)
		close(pt_fd);

err_opcua_exit:
	opcua_server_exit();

err_thread_exit:
	thread_exit();

err_avb_exit:
	genavb_exit(genavb_handle);

err:
	return rc;
}
