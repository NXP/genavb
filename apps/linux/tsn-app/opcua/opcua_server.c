/*
 * Copyright 2021-2022 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define _GNU_SOURCE
#include <sched.h>
#include <pthread.h>

#include <open62541/plugin/log_syslog.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "../../common/log.h"
#include "../../common/timer.h"

#include "../tsn_task.h"
#include "opcua_server.h"
#include "model/tsn_app_model.h"
#include "model/tsn_app_ua_nodeid_header.h"
#include "../tsn_tasks_config.h"

#define OPCUA_LOG_LEVEL	      UA_LOGLEVEL_INFO
#define OPCUA_THREAD_PRIORITY 1
#define OPCUA_THREAD_CPU_CORE 2

#define OP_STRING &UA_TYPES[UA_TYPES_STRING]
#define OP_INT32  &UA_TYPES[UA_TYPES_INT32]
#define OP_UINT32 &UA_TYPES[UA_TYPES_UINT32]
#define OP_UINT64 &UA_TYPES[UA_TYPES_UINT64]

static volatile UA_Boolean running = true;

static struct opcua_server *op_server;

struct opcua_server {
	pthread_t opcua_thread;
	UA_Server *server;
};

struct stats_nid {
	UA_UInt32 min;
	UA_UInt32 mean;
	UA_UInt32 max;
	UA_UInt32 abs_min;
	UA_UInt32 abs_max;
	UA_UInt32 ms;
	UA_UInt32 variance;
};

struct hist_nid {
	UA_UInt32 n_slots;
	UA_UInt32 slot_size;
	UA_UInt32 slots;
};

struct tsn_task_stats_nid {
	struct stats_nid sched_err;
	struct hist_nid sched_err_hist;
	struct stats_nid proc_time;
	struct hist_nid proc_time_hist;
	struct stats_nid total_time;
	struct hist_nid total_time_hist;
	UA_UInt32 sched;
	UA_UInt32 sched_early;
	UA_UInt32 sched_late;
	UA_UInt32 sched_missed;
	UA_UInt32 sched_timeout;
	UA_UInt32 clock_discont;
	UA_UInt32 clock_err;
	UA_UInt32 sched_err_max;
};

struct tsn_config_nid {
	UA_UInt32 role;
	UA_UInt32 num_peers;
};

struct net_socket_stats_nid {
	UA_UInt32 direction;
	UA_UInt32 frames;
	UA_UInt32 frames_err;
	UA_UInt32 id;
};

struct cyclic_socket_stats_nid {
	struct stats_nid traffic_latency;
	struct hist_nid traffic_latency_hist;
	UA_UInt32 peer_id;
	UA_UInt32 valid_frames;
	UA_UInt32 err_id;
	UA_UInt32 err_ts;
	UA_UInt32 err_underflow;
	UA_UInt32 link;
};

static struct tsn_task_stats_nid tsn_task_stats_nid = {
	.sched_err = {
		.min = UA_1ID_TSNAPP_TASKSTATS_SCHEDERRSTATS_MIN,
		.mean = UA_1ID_TSNAPP_TASKSTATS_SCHEDERRSTATS_MEAN,
		.max = UA_1ID_TSNAPP_TASKSTATS_SCHEDERRSTATS_MAX,
		.abs_min = UA_1ID_TSNAPP_TASKSTATS_SCHEDERRSTATS_ABSMIN,
		.abs_max = UA_1ID_TSNAPP_TASKSTATS_SCHEDERRSTATS_ABSMAX,
		.ms = UA_1ID_TSNAPP_TASKSTATS_SCHEDERRSTATS_MS,
		.variance = UA_1ID_TSNAPP_TASKSTATS_SCHEDERRSTATS_VARIANCE,
	},
	.sched_err_hist = {
		.n_slots = UA_1ID_TSNAPP_TASKSTATS_SCHEDERRHISTO_NSLOTS,
		.slot_size = UA_1ID_TSNAPP_TASKSTATS_SCHEDERRHISTO_SLOTSIZE,
		.slots = UA_1ID_TSNAPP_TASKSTATS_SCHEDERRHISTO_SLOTS,
	},
	.proc_time = {
		.min = UA_1ID_TSNAPP_TASKSTATS_PROCTIMESTATS_MIN,
		.mean = UA_1ID_TSNAPP_TASKSTATS_PROCTIMESTATS_MEAN,
		.max = UA_1ID_TSNAPP_TASKSTATS_PROCTIMESTATS_MAX,
		.abs_min = UA_1ID_TSNAPP_TASKSTATS_PROCTIMESTATS_ABSMIN,
		.abs_max = UA_1ID_TSNAPP_TASKSTATS_PROCTIMESTATS_ABSMAX,
		.ms = UA_1ID_TSNAPP_TASKSTATS_PROCTIMESTATS_MS,
		.variance = UA_1ID_TSNAPP_TASKSTATS_PROCTIMESTATS_VARIANCE,
	},
	.proc_time_hist = {
		.n_slots = UA_1ID_TSNAPP_TASKSTATS_PROCTIMEHISTO_NSLOTS,
		.slot_size = UA_1ID_TSNAPP_TASKSTATS_PROCTIMEHISTO_SLOTSIZE,
		.slots = UA_1ID_TSNAPP_TASKSTATS_PROCTIMEHISTO_SLOTS,
	},
	.total_time = {
		.min = UA_1ID_TSNAPP_TASKSTATS_TOTALTIMESTATS_MIN,
		.mean = UA_1ID_TSNAPP_TASKSTATS_TOTALTIMESTATS_MEAN,
		.max = UA_1ID_TSNAPP_TASKSTATS_TOTALTIMESTATS_MAX,
		.abs_min = UA_1ID_TSNAPP_TASKSTATS_TOTALTIMESTATS_ABSMIN,
		.abs_max = UA_1ID_TSNAPP_TASKSTATS_TOTALTIMESTATS_ABSMAX,
		.ms = UA_1ID_TSNAPP_TASKSTATS_TOTALTIMESTATS_MS,
		.variance = UA_1ID_TSNAPP_TASKSTATS_TOTALTIMESTATS_VARIANCE,
	},
	.total_time_hist = {
		.n_slots = UA_1ID_TSNAPP_TASKSTATS_TOTALTIMEHISTO_NSLOTS,
		.slot_size = UA_1ID_TSNAPP_TASKSTATS_TOTALTIMEHISTO_SLOTSIZE,
		.slots = UA_1ID_TSNAPP_TASKSTATS_TOTALTIMEHISTO_SLOTS,
	},
	.sched = UA_1ID_TSNAPP_TASKSTATS_SCHED,
	.sched_early = UA_1ID_TSNAPP_TASKSTATS_SCHEDEARLY,
	.sched_late = UA_1ID_TSNAPP_TASKSTATS_SCHEDLATE,
	.sched_missed = UA_1ID_TSNAPP_TASKSTATS_SCHEDMISSED,
	.sched_timeout = UA_1ID_TSNAPP_TASKSTATS_SCHEDTIMEOUT,
	.clock_discont = UA_1ID_TSNAPP_TASKSTATS_CLOCKDISCOUNT,
	.clock_err = UA_1ID_TSNAPP_TASKSTATS_CLOCKERR,
};

static struct tsn_config_nid tsn_config_nid = {
	.role = UA_1ID_TSNAPP_CONFIGURATION_ROLE,
	.num_peers = UA_1ID_TSNAPP_CONFIGURATION_NUMPEERS,
};

static struct net_socket_stats_nid net_socket_stats_nid[MAX_RX_SOCKET + MAX_TX_SOCKET] = {
	[0] = {
		.direction = UA_1ID_TSNAPP_SOCKETSTATS_NETRXSOCKET0_DIRECTION,
		.frames = UA_1ID_TSNAPP_SOCKETSTATS_NETRXSOCKET0_FRAMES,
		.frames_err = UA_1ID_TSNAPP_SOCKETSTATS_NETRXSOCKET0_FRAMESERR,
		.id = UA_1ID_TSNAPP_SOCKETSTATS_NETRXSOCKET0_ID,
	},
	[1] = {
		.direction = UA_1ID_TSNAPP_SOCKETSTATS_NETRXSOCKET1_DIRECTION,
		.frames = UA_1ID_TSNAPP_SOCKETSTATS_NETRXSOCKET1_FRAMES,
		.frames_err = UA_1ID_TSNAPP_SOCKETSTATS_NETRXSOCKET1_FRAMESERR,
		.id = UA_1ID_TSNAPP_SOCKETSTATS_NETRXSOCKET1_ID,
	},
	[MAX_RX_SOCKET] = {
		.direction = UA_1ID_TSNAPP_SOCKETSTATS_NETTXSOCKET0_DIRECTION,
		.frames = UA_1ID_TSNAPP_SOCKETSTATS_NETTXSOCKET0_FRAMES,
		.frames_err = UA_1ID_TSNAPP_SOCKETSTATS_NETTXSOCKET0_FRAMESERR,
		.id = UA_1ID_TSNAPP_SOCKETSTATS_NETTXSOCKET0_ID,
	},
};

struct cyclic_socket_stats_nid cyclic_socket_stats_nid[MAX_PEERS] = {
	[0] = {
		.traffic_latency = {
			.min = UA_1ID_TSNAPP_SOCKETSTATS_CYCLICRXSOCKET0_TRAFFICLATENCYSTATS_MIN,
			.mean = UA_1ID_TSNAPP_SOCKETSTATS_CYCLICRXSOCKET0_TRAFFICLATENCYSTATS_MEAN,
			.max = UA_1ID_TSNAPP_SOCKETSTATS_CYCLICRXSOCKET0_TRAFFICLATENCYSTATS_MAX,
			.abs_min = UA_1ID_TSNAPP_SOCKETSTATS_CYCLICRXSOCKET0_TRAFFICLATENCYSTATS_ABSMIN,
			.abs_max = UA_1ID_TSNAPP_SOCKETSTATS_CYCLICRXSOCKET0_TRAFFICLATENCYSTATS_ABSMAX,
			.ms = UA_1ID_TSNAPP_SOCKETSTATS_CYCLICRXSOCKET0_TRAFFICLATENCYSTATS_MS,
			.variance = UA_1ID_TSNAPP_SOCKETSTATS_CYCLICRXSOCKET0_TRAFFICLATENCYSTATS_VARIANCE,
		},
		.traffic_latency_hist = {
			.n_slots = UA_1ID_TSNAPP_SOCKETSTATS_CYCLICRXSOCKET0_TRAFFICLATENCYHISTO_NSLOTS,
			.slot_size = UA_1ID_TSNAPP_SOCKETSTATS_CYCLICRXSOCKET0_TRAFFICLATENCYHISTO_SLOTSIZE,
			.slots = UA_1ID_TSNAPP_SOCKETSTATS_CYCLICRXSOCKET0_TRAFFICLATENCYHISTO_SLOTS,
		},
		.peer_id = UA_1ID_TSNAPP_SOCKETSTATS_CYCLICRXSOCKET0_PEERID,
		.valid_frames = UA_1ID_TSNAPP_SOCKETSTATS_CYCLICRXSOCKET0_VALIDFRAMES,
		.err_id = UA_1ID_TSNAPP_SOCKETSTATS_CYCLICRXSOCKET0_ERRID,
		.err_ts = UA_1ID_TSNAPP_SOCKETSTATS_CYCLICRXSOCKET0_ERRTS,
		.err_underflow = UA_1ID_TSNAPP_SOCKETSTATS_CYCLICRXSOCKET0_ERRUNDERFLOW,
		.link = UA_1ID_TSNAPP_SOCKETSTATS_CYCLICRXSOCKET0_LINK,
	},
	[1] = {
		.traffic_latency = {
			.min = UA_1ID_TSNAPP_SOCKETSTATS_CYCLICRXSOCKET1_TRAFFICLATENCYSTATS_MIN,
			.mean = UA_1ID_TSNAPP_SOCKETSTATS_CYCLICRXSOCKET1_TRAFFICLATENCYSTATS_MEAN,
			.max = UA_1ID_TSNAPP_SOCKETSTATS_CYCLICRXSOCKET1_TRAFFICLATENCYSTATS_MAX,
			.abs_min = UA_1ID_TSNAPP_SOCKETSTATS_CYCLICRXSOCKET1_TRAFFICLATENCYSTATS_ABSMIN,
			.abs_max = UA_1ID_TSNAPP_SOCKETSTATS_CYCLICRXSOCKET1_TRAFFICLATENCYSTATS_ABSMAX,
			.ms = UA_1ID_TSNAPP_SOCKETSTATS_CYCLICRXSOCKET1_TRAFFICLATENCYSTATS_MS,
			.variance = UA_1ID_TSNAPP_SOCKETSTATS_CYCLICRXSOCKET1_TRAFFICLATENCYSTATS_VARIANCE,
		},
		.traffic_latency_hist = {
			.n_slots = UA_1ID_TSNAPP_SOCKETSTATS_CYCLICRXSOCKET1_TRAFFICLATENCYHISTO_NSLOTS,
			.slot_size = UA_1ID_TSNAPP_SOCKETSTATS_CYCLICRXSOCKET1_TRAFFICLATENCYHISTO_SLOTSIZE,
			.slots = UA_1ID_TSNAPP_SOCKETSTATS_CYCLICRXSOCKET1_TRAFFICLATENCYHISTO_SLOTS,
		},
		.peer_id = UA_1ID_TSNAPP_SOCKETSTATS_CYCLICRXSOCKET1_PEERID,
		.valid_frames = UA_1ID_TSNAPP_SOCKETSTATS_CYCLICRXSOCKET1_VALIDFRAMES,
		.err_id = UA_1ID_TSNAPP_SOCKETSTATS_CYCLICRXSOCKET1_ERRID,
		.err_ts = UA_1ID_TSNAPP_SOCKETSTATS_CYCLICRXSOCKET1_ERRTS,
		.err_underflow = UA_1ID_TSNAPP_SOCKETSTATS_CYCLICRXSOCKET1_ERRUNDERFLOW,
		.link = UA_1ID_TSNAPP_SOCKETSTATS_CYCLICRXSOCKET1_LINK,
	},
};

static void opcua_set_value(void *UA_RESTRICT p, const UA_DataType *type, const int nodeId)
{
	UA_Variant v;

	UA_Variant_setScalar(&v, p, type);
	UA_Server_writeValue(op_server->server, UA_NODEID_NUMERIC(2, nodeId), v);
}

static void opcua_update_dir(int direction, UA_Int32 nid_dir)
{
	char *dir;
	UA_String opcua_dir;

	if (direction == TX)
		dir = "tx";
	else
		dir = "rx";

	opcua_dir = UA_STRING(dir);

	opcua_set_value(&opcua_dir, OP_STRING, nid_dir);
}

void opcua_init_params(struct cyclic_task *task)
{
	int i;
	char *id;
	UA_String role;
	struct tsn_config_nid *nid = &tsn_config_nid;
	struct net_socket_stats_nid *nid_net = net_socket_stats_nid;

	/* Configuration */
	if (task->id == CONTROLLER_0)
		id = "Controller 0";
	else if (task->id == IO_DEVICE_0)
		id = "IO device 0";
	else if (task->id == IO_DEVICE_1)
		id = "IO device 1";
	else
		id = "Unknown";

	role = UA_STRING(id);

	opcua_set_value(&role, OP_STRING, nid->role);
	opcua_set_value(&task->num_peers, OP_UINT64, nid->num_peers);

	for (i = 0; i < task->num_peers; i++)
		opcua_update_dir(task->rx_socket[i].net_sock->dir, nid_net[i].direction);

	opcua_update_dir(task->tx_socket.net_sock->dir, nid_net[2].direction);
}

static void opcua_update_stats(struct stats *stats, struct stats_nid *nid)
{
	opcua_set_value(&stats->min, OP_INT32, nid->min);
	opcua_set_value(&stats->mean, OP_INT32, nid->mean);
	opcua_set_value(&stats->max, OP_INT32, nid->max);
	opcua_set_value(&stats->abs_min, OP_INT32, nid->abs_min);
	opcua_set_value(&stats->abs_max, OP_INT32, nid->abs_max);
	opcua_set_value(&stats->ms, OP_UINT64, nid->ms);
	opcua_set_value(&stats->variance, OP_UINT64, nid->variance);
}

static void opcua_update_histogram(struct hist *hist, struct hist_nid *nid)
{
	UA_Variant value;

	opcua_set_value(&hist->n_slots, OP_UINT32, nid->n_slots);
	opcua_set_value(&hist->slot_size, OP_UINT32, nid->slot_size);

	UA_Variant_setArray(&value, hist->slots, hist->n_slots, OP_UINT32);
	UA_Server_writeValue(op_server->server, UA_NODEID_NUMERIC(2, nid->slots), value);
}

void opcua_update_task_stats(struct tsn_task_stats *task)
{
	struct tsn_task_stats_nid *nid = &tsn_task_stats_nid;

	opcua_set_value(&task->sched, OP_UINT32, nid->sched);
	opcua_set_value(&task->sched_early, OP_UINT32, nid->sched_early);
	opcua_set_value(&task->sched_late, OP_UINT32, nid->sched_late);
	opcua_set_value(&task->sched_missed, OP_UINT32, nid->sched_missed);
	opcua_set_value(&task->sched_timeout, OP_UINT32, nid->sched_timeout);
	opcua_set_value(&task->clock_discont, OP_UINT32, nid->clock_discont);
	opcua_set_value(&task->clock_err, OP_UINT32, nid->clock_err);

	opcua_update_stats(&task->sched_err, &nid->sched_err);
	opcua_update_histogram(&task->sched_err_hist, &nid->sched_err_hist);

	opcua_update_stats(&task->proc_time, &nid->proc_time);
	opcua_update_histogram(&task->proc_time_hist, &nid->proc_time_hist);

	opcua_update_stats(&task->total_time, &nid->total_time);
	opcua_update_histogram(&task->total_time_hist, &nid->total_time_hist);
}

void opcua_update_cyclic_socket(struct socket *sock)
{
	char *link;
	UA_String opcua_link;
	struct cyclic_socket_stats_nid *nid = &cyclic_socket_stats_nid[sock->id];

	if (sock->stats_snap.link_status == 1)
		link = "up";
	else
		link = "down";

	opcua_link = UA_STRING(link);

	opcua_set_value(&sock->stats_snap.err_id, OP_UINT32, nid->err_id);
	opcua_set_value(&sock->stats_snap.err_ts, OP_UINT32, nid->err_ts);
	opcua_set_value(&sock->stats_snap.err_underflow, OP_UINT32, nid->err_underflow);
	opcua_set_value(&opcua_link, OP_STRING, nid->link);
	opcua_set_value(&sock->peer_id, OP_INT32, nid->peer_id);
	opcua_set_value(&sock->stats_snap.valid_frames, OP_UINT32, nid->valid_frames);

	opcua_update_stats(&sock->stats_snap.traffic_latency, &nid->traffic_latency);
	opcua_update_histogram(&sock->stats_snap.traffic_latency_hist, &nid->traffic_latency_hist);
}

void opcua_update_net_socket_stats(struct net_socket *sock)
{
	int index = sock->id;
	struct net_socket_stats *stats = &sock->stats_snap;
	struct net_socket_stats_nid *nid;

	if (sock->dir == TX)
		index += MAX_RX_SOCKET;

	nid = &net_socket_stats_nid[index];

	opcua_set_value(&stats->frames, OP_UINT32, nid->frames);
	opcua_set_value(&stats->err, OP_UINT32, nid->frames_err);
	opcua_set_value(&sock->id, OP_INT32, nid->id);
}

static void *opcua_thread_handle(void *param)
{
	intptr_t rc = 0;
	struct sched_param thread_param = {
		.sched_priority = OPCUA_THREAD_PRIORITY,
	};
	cpu_set_t cpu_set;
	UA_StatusCode retval;

	/* Priority */
	if (sched_setscheduler(0, SCHED_FIFO, &thread_param) < 0) {
		ERR("sched_setscheduler failed with error %d - %s", errno, strerror(errno));
		rc = -1;
		goto exit;
	}

	/* Affinity */
	CPU_ZERO(&cpu_set);
	CPU_SET(OPCUA_THREAD_CPU_CORE, &cpu_set);
	if (sched_setaffinity(0, sizeof(cpu_set), &cpu_set) == -1) {
		ERR("sched_setaffinity failed with error %d - %s", errno, strerror(errno));
	}

	INF("Starting OPCUA server");

	retval = UA_Server_run(op_server->server, &running);
	if (retval != UA_STATUSCODE_GOOD) {
		ERR("UA_Server_run failed");
		rc = -1;
		goto exit;
	}

exit:
	return (void *)rc;
}

int opcua_server_init(void)
{
	UA_ServerConfig *config;
	UA_StatusCode retval;
	int rc = 0;

	if (op_server) {
		rc = -1;
		goto err;
	}

	op_server = malloc(sizeof(struct opcua_server));
	if (!op_server) {
		ERR("malloc() failed");
		rc = -1;
		goto err;
	}

	op_server->server = UA_Server_new();
	if (!op_server->server) {
		ERR("UA_Server_new() failed");
		rc = -1;
		goto err_free;
	}

	config = UA_Server_getConfig(op_server->server);
	UA_ServerConfig_setDefault(config);
	config->logger = UA_Log_Syslog_withLevel(OPCUA_LOG_LEVEL);

	retval = tsn_app_model(op_server->server);
	if (retval != UA_STATUSCODE_GOOD) {
		ERR("Information Model failed");
		rc = -1;
		goto err_del_server;
	}

	INF("Starting OPCUA server thread");

	rc = pthread_create(&op_server->opcua_thread, NULL, &opcua_thread_handle, NULL);
	if (rc != 0) {
		ERR("OPCUA thread create failed, error %s", strerror(rc));
		goto err_del_server;
	}

	return 0;

err_del_server:
	UA_Server_delete(op_server->server);

err_free:
	free(op_server);

err:
	return rc;
};

void opcua_server_exit(void)
{
	running = false;
	pthread_join(op_server->opcua_thread, NULL);
	UA_Server_delete(op_server->server);
}
