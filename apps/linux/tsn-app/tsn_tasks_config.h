/*
 * Copyright 2019-2020 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _TSN_TASKS_CONFIG_H_
#define _TSN_TASKS_CONFIG_H_

#include "genavb/tsn.h"
#include "genavb/qos.h"

#define MAX_PEERS	   2
#define MAX_TASK_CONFIGS   4
#define MAX_TSN_STREAMS	   8
#define ETHERTYPE_MOTOROLA 0x818D
#define VLAN_ID		   2
#define PACKET_SIZE	   80

#define SCHED_TRAFFIC_OFFSET 40000

#define APP_PERIOD	 (2000000)
#define NET_DELAY_OFFSET (APP_PERIOD / 2)

enum task_id {
	CONTROLLER_0,
	IO_DEVICE_0,
	IO_DEVICE_1,
	MAX_TASKS_ID
};

enum task_type {
	CYCLIC_CONTROLLER,
	CYCLIC_IO_DEVICE,
	ALARM_MONITOR,
	ALARM_IO_DEVICE,
};

struct tsn_stream {
	struct net_address address;
};

struct tsn_stream *tsn_conf_get_stream(int index);
struct cyclic_task *tsn_conf_get_cyclic_task(int index);
struct alarm_task *tsn_conf_get_alarm_task(int index);

#endif /* _TSN_TASKS_CONFIG_H_ */
